# Copyright (c) 2010 Philip Taylor
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import re
import cairo
import codecs
import time
import os
import shutil
import sys
import xml.dom.minidom
from xml.dom.minidom import Node

#import yaml
import syck as yaml # lots faster

def escapeJS(str):
	str = str.replace('\\', '\\\\').replace('"', '\\"')
	str = re.sub(r'\[(\w+)\]', r'[\\""+(\1)+"\\"]', str) # kind of an ugly hack, for nicer failure-message output
	return str

def escapeHTML(str):
	return str.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;').replace('"', '&quot;')

templates = yaml.load(open('templates.yaml').read())

spec_assertions = []
for s in yaml.load(open('spec.yaml').read())['assertions']:
	if 'meta' in s:
		eval(compile(s['meta'], '<meta spec assertion>', 'exec'), {}, {'assertions':spec_assertions})
	else:
		spec_assertions.append(s)

tests = []
for t in sum([ yaml.load(open(f).read()) for f in ['tests.yaml', 'tests2d.yaml', 'tests2dtext.yaml']], []):
	if 'DISABLED' in t:
		continue
	if 'meta' in t:
		eval(compile(t['meta'], '<meta test>', 'exec'), {}, {'tests':tests})
	else:
		tests.append(t)

category_names = []
category_contents_direct = {}
category_contents_all = {}

spec_ids = {}
for t in spec_assertions: spec_ids[t['id']] = True
spec_refs = {}

def backref_html(name):
	backrefs = []
	c = ''
	for p in name.split('.')[:-1]:
		c += '.'+p
		backrefs.append('<a href="index%s.html">%s</a>.' % (c, p))
	backrefs.append(name.split('.')[-1])
	return ''.join(backrefs)

def make_flat_image(filename, w, h, r,g,b,a):
	if os.path.exists('tests/%s' % filename):
		return filename
	surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
	cr = cairo.Context(surface)
	cr.set_source_rgba(r, g, b, a)
	cr.rectangle(0, 0, w, h)
	cr.fill()
	surface.write_to_png('tests/%s' % filename)
	return filename
	

try: os.mkdir('tests')
except: pass # ignore if it already exists

try: os.mkdir('mochitests')
except: pass

mochitests = []
used_images = {}

def expand_nonfinite(method, argstr, tail):
	"""
	>>> print expand_nonfinite('f', '<0 a>, <0 b>', ';')
	f(a, 0);
	f(0, b);
	f(a, b);
	>>> print expand_nonfinite('f', '<0 a>, <0 b c>, <0 d>', ';')
	f(a, 0, 0);
	f(0, b, 0);
	f(0, c, 0);
	f(0, 0, d);
	f(a, b, 0);
	f(a, b, d);
	f(a, 0, d);
	f(0, b, d);
	"""
	# argstr is "<valid-1 invalid1-1 invalid2-1 ...>, ..." (where usually
	# 'invalid' is Infinity/-Infinity/NaN)
	args = []
	for arg in argstr.split(', '):
		a = re.match('<(.*)>', arg).group(1)
		args.append(a.split(' '))
	calls = []
	# Start with the valid argument list
	call = [ args[j][0] for j in range(len(args)) ]
	# For each argument alone, try setting it to all its invalid values:
	for i in range(len(args)):
		for a in args[i][1:]:
			c2 = call[:]
			c2[i] = a
			calls.append(c2)
	# For all combinations of >= 2 arguments, try setting them to their
	# first invalid values. (Don't do all invalid values, because the
	# number of combinations explodes.)
	def f(c, start, depth):
		for i in range(start, len(args)):
			if len(args[i]) > 1:
				a = args[i][1]
				c2 = c[:]
				c2[i] = a
				if depth > 0: calls.append(c2)
				f(c2, i+1, depth+1)
	f(call, 0, 0)

	return '\n'.join('%s(%s)%s' % (method, ', '.join(c), tail) for c in calls)


if len(sys.argv) > 1 and sys.argv[1] == '--test':
	import doctest
	doctest.testmod()
	sys.exit()

used_tests = {}
for i in range(len(tests)):
	test = tests[i]

	name = test['name']
	print "\r(%s)" % name, " "*32, "\t",

	if name in used_tests:
		print "Test %s is defined twice" % name
	used_tests[name] = 1
	
	cat_total = ''
	for cat_part in [''] + name.split('.')[:-1]:
		cat_total += cat_part+'.'
		if not cat_total in category_names: category_names.append(cat_total)
		category_contents_all.setdefault(cat_total, []).append(name)
	category_contents_direct.setdefault(cat_total, []).append(name)

	for ref in test.get('testing', []):
		if ref not in spec_ids:
			print "Test %s uses nonexistent spec point %s" % (name, ref)
		spec_refs.setdefault(ref, []).append(name)
	#if not (len(test.get('testing', [])) or 'mozilla' in test):
	if not test.get('testing', []):
		print "Test %s doesn't refer to any spec points" % name

	if test.get('expected', '') == 'green' and re.search(r'@assert pixel .* 0,0,0,0;', test['code']):
		print "Probable incorrect pixel test in %s" % name 

	code = test['code']
	
	code = re.sub(r'@nonfinite ([^(]+)\(([^)]+)\)(.*)', lambda m: expand_nonfinite(m.group(1), m.group(2), m.group(3)), code) # must come before '@assert throws'

	code = re.sub(r'@assert pixel (\d+,\d+) == (\d+,\d+,\d+,\d+);',
			r'_assertPixel(canvas, \1, \2, "\1", "\2");',
			code)

	code = re.sub(r'@assert pixel (\d+,\d+) ==~ (\d+,\d+,\d+,\d+);',
			r'_assertPixelApprox(canvas, \1, \2, "\1", "\2", 2);',
			code)

	code = re.sub(r'@assert pixel (\d+,\d+) ==~ (\d+,\d+,\d+,\d+) \+/- (\d+);',
			r'_assertPixelApprox(canvas, \1, \2, "\1", "\2", \3);',
			code)

	code = re.sub(r'@assert throws (\S+_ERR) (.*);',
			lambda m: 'try { var _thrown = false;\n  %s;\n} catch (e) { if (e.code != DOMException.%s) _fail("Failed assertion: expected exception of type %s, got: "+e.message); _thrown = true; } finally { _assert(_thrown, "should throw exception of type %s: %s"); }'
				% (m.group(2), m.group(1), m.group(1), m.group(1), escapeJS(m.group(2)))
			, code)

	code = re.sub(r'@assert throws (.*);',
			lambda m: 'try { var _thrown = false; %s; } catch (e) { _thrown = true; } finally { _assert(_thrown, "should throw exception: %s"); }'
				% (m.group(1), escapeJS(m.group(1)))
			, code)

	code = re.sub(r'@assert (.*) === (.*);',
			lambda m: '_assertSame(%s, %s, "%s", "%s");'
				% (m.group(1), m.group(2), escapeJS(m.group(1)), escapeJS(m.group(2)))
			, code)
	
	code = re.sub(r'@assert (.*) !== (.*);',
			lambda m: '_assertDifferent(%s, %s, "%s", "%s");'
				% (m.group(1), m.group(2), escapeJS(m.group(1)), escapeJS(m.group(2)))
			, code)
	
	code = re.sub(r'@assert (.*) == (.*);',
			lambda m: '_assertEqual(%s, %s, "%s", "%s");'
				% (m.group(1), m.group(2), escapeJS(m.group(1)), escapeJS(m.group(2)))
			, code)
	
	code = re.sub(r'@assert (.*) =~ (.*);',
			lambda m: '_assertMatch(%s, %s, "%s", "%s");'
				% (m.group(1), m.group(2), escapeJS(m.group(1)), escapeJS(m.group(2)))
			, code)

	code = re.sub(r'@assert (.*);',
			lambda m: '_assert(%s, "%s");'
				% (m.group(1), escapeJS(m.group(1)))
			, code)
	
	code = re.sub(r'@manual;', '_requireManualCheck();', code)

	code = re.sub(r'@crash;', 'return _crash();', code)

	code = re.sub(r' @moz-todo', '', code)

	code = re.sub(r'@moz-UniversalBrowserRead;',
			""
			, code)

	assert('@' not in code)

	mochitest = not ('@manual' in test['code'] or 'disabled' in test.get('mozilla', {}))
	if mochitest:
		mochi_code = test['code']

		mochi_code = re.sub(r'@nonfinite ([^(]+)\(([^)]+)\)(.*)', lambda m: expand_nonfinite(m.group(1), m.group(2), m.group(3)), mochi_code)

		mochi_code = re.sub(r'@assert pixel (\d+,\d+) == (\d+,\d+,\d+,\d+);',
			r'isPixel(ctx, \1, \2, "\1", "\2", 0);',
			mochi_code)

		mochi_code = re.sub(r'@assert pixel (\d+,\d+) ==~ (\d+,\d+,\d+,\d+);',
			r'isPixel(ctx, \1, \2, "\1", "\2", 2);',
			mochi_code)

		mochi_code = re.sub(r'@assert pixel (\d+,\d+) ==~ (\d+,\d+,\d+,\d+) \+/- (\d+);',
			r'isPixel(ctx, \1, \2, "\1", "\2", \3);',
			mochi_code)

		mochi_code = re.sub(r'@assert throws (\S+_ERR) (.*);',
			lambda m: 'var _thrown = undefined; try {\n  %s;\n} catch (e) { _thrown = e }; ok(_thrown && _thrown.code == DOMException.%s, "should throw %s");'
				% (m.group(2), m.group(1), m.group(1))
			, mochi_code)

		mochi_code = re.sub(r'@assert throws (.*);',
			lambda m: 'try { var _thrown = false;\n  %s;\n} catch (e) { _thrown = true; } finally { ok(_thrown, "should throw exception"); }'
				% (m.group(1))
			, mochi_code)

		mochi_code = re.sub(r'@assert (.*) =~ (.*);',
			lambda m: 'ok(%s.match(%s), "%s.match(%s)");'
				% (m.group(1), m.group(2), escapeJS(m.group(1)), escapeJS(m.group(2)))
			, mochi_code)

		mochi_code = re.sub(r'@assert (.*);',
			lambda m: 'ok(%s, "%s");'
				% (m.group(1), escapeJS(m.group(1)))
			, mochi_code)

		mochi_code = re.sub(r'((?:^|\n|;)\s*)ok(.*;) @moz-todo',
			lambda m: '%stodo%s'
				% (m.group(1), m.group(2))
			, mochi_code)

		mochi_code = re.sub(r'((?:^|\n|;)\s*)(is.*;) @moz-todo',
			lambda m: '%stodo_%s'
				% (m.group(1), m.group(2))
			, mochi_code)

		mochi_code = re.sub(r'@moz-UniversalBrowserRead;',
			"netscape.security.PrivilegeManager.enablePrivilege('UniversalBrowserRead');"
			, mochi_code)

		mochi_code = mochi_code.replace('../images/', 'image_')

		assert '@' not in mochi_code, '@ not in code:\n%s' % mochi_code

		mochi_name = name
		if 'mozilla' in test:
			if 'throws' in test['mozilla']:
				mochi_code = templates['mochitest.exception'] % mochi_code
			if 'bug' in test['mozilla']:
				mochi_name = "%s - bug %s" % (name, test['mozilla']['bug'])

		if 'desc' in test:
			mochi_desc = '<!-- Testing: %s -->\n' % test['desc']
		else:
			mochi_desc = ''

		if 'deferTest' in mochi_code:
			mochi_setup = ''
			mochi_footer = ''
		else:
			mochi_setup = ''
			mochi_footer = 'SimpleTest.finish();\n'

		for f in ['isPixel', 'todo_isPixel', 'deferTest', 'wrapFunction']:
			if f in mochi_code:
				mochi_setup += templates['mochitest.%s' % f]


	else:
		print "Skipping mochitest for %s" % name
		mochi_name = ''
		mochi_desc = ''
		mochi_code = ''
		mochi_setup = ''
		mochi_footer = ''


	expectation_html = ''
	if 'expected' in test and test['expected'] is not None:
		expected = test['expected']
		expected_img = None
		if expected == 'green':
			expected_img = make_flat_image('green-100x50.png', 100, 50, 0,1,0,1)
		elif expected == 'clear':
			expected_img = make_flat_image('clear-100x50.png', 100, 50, 0,0,0,0)
		else:
			if ';' in expected: print "Found semicolon in %s" % name
			expected = re.sub(r'^size (\d+) (\d+)',
				r'surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, \1, \2)\ncr = cairo.Context(surface)',
				expected)
			expected += "\nsurface.write_to_png('tests/%s.png')\n" % name
			eval(compile(expected, '<test %s>' % test['name'], 'exec'), {}, {'cairo':cairo})
			expected_img = "%s.png" % name

		if expected_img:
			expectation_html = ('<p class="output expectedtext">Expected output:' +
				'<p><img src="%s" class="output expected" id="expected" alt="">' % (expected_img))

	canvas = test.get('canvas', 'width="100" height="50"')

	prev = tests[i-1]['name'] if i != 0 else 'index'
	next = tests[i+1]['name'] if i != len(tests)-1 else 'index'

	name_wrapped = name.replace('.', '.&#8203;') # (see https://bugzilla.mozilla.org/show_bug.cgi?id=376188)

	refs = ''.join('<li><a href="spec.html#testrefs.%s">%s</a>\n' % (n,n) for n in test.get('testing', []))
	if 'mozilla' in test and 'bug' in test['mozilla']:
		refs += '<li><a href="https://bugzilla.mozilla.org/show_bug.cgi?id=%d">Bugzilla</a>' % test['mozilla']['bug']

	notes = '<p class="notes">%s' % test['notes'] if 'notes' in test else ''

	images = ''
	for i in test.get('images', []):
		id = i.split('/')[-1]
		if '/' not in i:
			used_images[i] = 1
			i = '../images/%s' % i
		images += '<img src="%s" id="%s" class="resource">\n' % (i,id)
	mochi_images = images.replace('../images/', 'image_')

	fonts = ''
	fonthack = ''
	for i in test.get('fonts', []):
		fonts += '@font-face {\n  font-family: %s;\n  src: url("../fonts/%s.ttf");\n}\n' % (i, i)
		# Browsers require the font to actually be used in the page
		if test.get('fonthack', 1):
			fonthack += '<span style="font-family: %s; position: absolute; visibility: hidden">A</span>\n' % i
	if fonts:
		fonts = '<style>\n%s</style>\n' % fonts

	fallback = test.get('fallback', '<p class="fallback">FAIL (fallback content)</p>')

	desc = test.get('desc', '')

	template_params = {
		'name':name, 'name_wrapped':name_wrapped, 'backrefs':backref_html(name), 'desc':desc,
		'prev':prev, 'next':next, 'refs':refs, 'notes':notes, 'images':images,
		'fonts':fonts, 'fonthack':fonthack,
		'canvas':canvas, 'expected':expectation_html, 'code':code,
		'mochi_name':mochi_name, 'mochi_desc':mochi_desc, 'mochi_code':mochi_code,
		'mochi_setup':mochi_setup, 'mochi_footer':mochi_footer, 'mochi_images':mochi_images,
		'fallback':fallback
	}

	f = codecs.open('tests/%s.html' % name, 'w', 'utf-8')
	f.write(templates['standalone'] % template_params)
	
	f = codecs.open('tests/framed.%s.html' % name, 'w', 'utf-8')
	f.write(templates['framed'] % template_params)

	f = codecs.open('tests/minimal.%s.html' % name, 'w', 'utf-8')
	f.write(templates['minimal'] % template_params)

	if mochitest:
		mochitests.append(name)
		f = codecs.open('mochitests/test_%s.html' % name, 'w', 'utf-8')
		f.write(templates['mochitest'] % template_params)

for i in used_images:
	shutil.copyfile("images/%s" % i, "mochitests/image_%s" % i)

def write_mochitest_makefile():
	f = open('mochitests/Makefile.in', 'w')
	f.write(templates['mochitest.Makefile'])
	files = ['test_%s.html' % n for n in mochitests] + ['image_%s' % n for n in used_images]
	chunksize = 100
	chunks = []
	for i in range(0, len(files), chunksize):
		chunk = files[i:i+chunksize]
		name = '_TEST_FILES_%d' % (i / chunksize)
		chunks.append(name)
		f.write('%s = \\\n' % name)
		for file in chunk: f.write('\t%s \\\n' % file)
		f.write('\t$(NULL)\n\n')
	f.write('# split up into groups to work around command-line length limits\n')
	for name in chunks:
		f.write('libs:: $(%s)\n\t$(INSTALL) $(foreach f,$^,"$f") $(DEPTH)/_tests/testing/mochitest/tests/$(relativesrcdir)\n\n' % name)
		

write_mochitest_makefile()

print

def write_index():
	f = open('tests/index.html', 'w')
	f.write(templates['index'] % { 'updated':time.strftime('%Y-%m-%d', time.gmtime()) })
	f.write('\n<ul class="testlist">\n')
	depth = 1
	for category in category_names:
		name = category[1:-1] or ''
		count = len(category_contents_all[category])
		new_depth = category.count('.')
		while new_depth < depth: f.write(' '*(depth-1) + '</ul>\n'); depth -= 1
		f.write(' '*depth + templates['index.category.item'] % (name or 'all', name, count, '' if count==1 else 's'))
		while new_depth+1 > depth: f.write(' '*depth + '<ul>\n'); depth += 1
		for item in category_contents_direct.get(category, []):
			f.write(' '*depth + '<li><a href="%s.html">%s</a>\n' % (item, item) )
	while 0 < depth: f.write(' '*(depth-1) + '</ul>\n'); depth -= 1

def write_category_indexes():
	for category in category_names:
		name = (category[1:-1] or 'all')
		
		f = open('tests/index.%s.html' % name, 'w')
		f.write(templates['index.frame'] % { 'backrefs':backref_html(name), 'category':name })
		for item in category_contents_all[category]:
			f.write(templates['index.frame.item'] % item)

def write_reportgen():
	f = open('tests/reportgen.html', 'w')
	items_text = ',\n'.join(('"%s"' % item) for item in category_contents_all['.'])
	f.write(templates['reportgen'] % {'items':items_text })

def write_results():
	results = {}
	uas = []
	uastrings = {}
	for item in category_contents_all['.']: results[item] = {}

	f = open('tests/results.html', 'w')
	f.write(templates['results'])

	if not os.path.exists('results.yaml'):
		print "Can't find results.yaml"
	else:
		for resultset in yaml.load(open('results.yaml').read()):
			#title = "%s (%s)" % (resultset['ua'], resultset['time'])
			title = resultset['name']
			#assert title not in uas # don't allow repetitions
			if title not in uas:
				uas.append(title)
				uastrings[title] = resultset['ua']
			else:
				assert uastrings[title] == resultset['ua']
			for r in resultset['results']:
				if r['id'] not in results:
					print 'Skipping results for removed test %s' % r['id']
					continue
				results[r['id']][title] = (
						r['status'].lower(),
						re.sub(r'%(..)', lambda m: chr(int(m.group(1), 16)),
						re.sub(r'%u(....)', lambda m: unichr(int(m.group(1), 16)),
							r['notes'])).encode('utf8')
						)

	passes = {}
	for ua in uas:
		f.write('<th title="%s">%s\n' % (uastrings[ua], ua))
		passes[ua] = 0
	for id in category_contents_all['.']:
		f.write('<tr><td><a href="#%s" id="%s">#</a> <a href="%s.html">%s</a>\n' % (id, id, id, id))
		for ua in uas:
			status, details = results[id].get(ua, ('', ''))
			f.write('<td class="r %s"><ul class="d">%s</ul>\n' % (status, details))
			if status == 'pass': passes[ua] += 1
	f.write('<tr><th>Passes\n')
	for ua in uas:
		f.write('<td>%.1f%%\n' % ((100.0 * passes[ua]) / len(category_contents_all['.'])))
	f.write('<tr><td>\n')
	for ua in uas:
		f.write('<td>%s\n' % ua)
	f.write('</table>\n')

def getNodeText(node):
	t, offsets = '', []

	# Skip over any previous annotations we added
	if node.nodeType == node.ELEMENT_NODE and 'testrefs' in node.getAttribute('class').split(' '):
		return t, offsets

	if node.nodeType == node.TEXT_NODE:
		val = node.nodeValue
		val = val.replace(unichr(0xa0), ' ') # replace &nbsp;s
		t += val
		offsets += [ (node, len(node.nodeValue)) ]
	for n in node.childNodes:
		child_t, child_offsets = getNodeText(n)
		t += child_t
		offsets += child_offsets
	return t, offsets

def html5Serializer(element):
	element.normalize()
	rv = []
	specialtext = ['style', 'script', 'xmp', 'iframe', 'noembed', 'noframes', 'noscript']
	empty = ['area', 'base', 'basefont', 'bgsound', 'br', 'col', 'embed', 'frame',
			 'hr', 'img', 'input', 'link', 'meta', 'param', 'spacer', 'wbr']

	def serializeElement(element):
		if element.nodeType == Node.DOCUMENT_TYPE_NODE:
			rv.append("<!DOCTYPE %s>" % element.name)
		elif element.nodeType == Node.DOCUMENT_NODE:
			for child in element.childNodes:
				serializeElement(child)
		elif element.nodeType == Node.COMMENT_NODE:
			rv.append("<!--%s-->" % element.nodeValue)
		elif element.nodeType == Node.TEXT_NODE:
			unescaped = False
			n = element.parentNode
			while n is not None:
				if n.nodeName in specialtext:
					unescaped = True
					break
				n = n.parentNode
			if unescaped:
				rv.append(element.nodeValue)
			else:
				rv.append(escapeHTML(element.nodeValue))
		else:
			rv.append("<%s" % element.nodeName)
			if element.hasAttributes():
				for name, value in element.attributes.items():
					rv.append(' %s="%s"' % (name, escapeHTML(value)))
			rv.append(">")
			if element.nodeName not in empty:
				for child in element.childNodes:
					serializeElement(child)
				rv.append("</%s>" % element.nodeName)
	serializeElement(element)
	return '<!DOCTYPE HTML>\n' + ''.join(rv)

def write_annotated_spec():
	# Load the stripped-down XHTMLised copy of the spec
	doc = xml.dom.minidom.parse(open('current-work-canvas.xhtml', 'r'))

	# Insert our new stylesheet
	n = doc.getElementsByTagName('head')[0].appendChild(doc.createElement('link'))
	n.setAttribute('rel', 'stylesheet')
	n.setAttribute('href', '../spectest.css')
	n.setAttribute('type', 'text/css')

	spec_assertion_patterns = []
	for a in spec_assertions:
		# Warn about problems
		if a['id'] not in spec_refs:
			print "Unused spec statement %s" % a['id']

		pattern_text = a['text']

		if 'keyword' in a:
			# Explicit keyword override
			keyword = a['keyword']
		else:
			# Extract the marked keywords, and remove the markers
			keyword = 'none'
			for kw in ['must', 'should', 'required']:
				if ('*%s*' % kw) in pattern_text:
					keyword = kw
					pattern_text = pattern_text.replace('*%s*' % kw, kw)
					break
		# Make sure there wasn't >1 keyword
		for kw in ['must', 'should', 'required']:
			assert('*%s*' % kw not in pattern_text)

		# Convert the special pattern format into regexp syntax
		pattern_text = (pattern_text.
			# Escape relevant characters
			replace('*', r'\*').
			replace('+', r'\+').
			replace('.', r'\.').
			replace('(', r'\(').
			replace(')', r'\)').
			replace('[', r'\[').
			replace(']', r'\]').
			# Convert special sequences back into unescaped regexp code
			replace(' ', r'\s+').
			replace(r'<\.\.\.>', r'.+').
			replace('<^>', r'()').
			replace('<eol>', r'\s*?\n')
		)
		pattern = re.compile(pattern_text, re.S)
		spec_assertion_patterns.append( (a['id'], pattern, keyword, a.get('previously', None)) )
	matched_assertions = {}

	def process_element(e):
		t, offsets = getNodeText(e)
		for id, pattern, keyword, previously in spec_assertion_patterns:
			m = pattern.search(t)
			if m:
				# When the pattern-match isn't enough to uniquely identify a sentence,
				# allow explicit back-references to earlier paragraphs
				if previously:
					if len(previously) >= 3:
						n, text, exp = previously
					else:
						n, text = previously
						exp = True
					node = e
					while n and node.previousSibling:
						node = node.previousSibling
						n -= 1
					if (text not in getNodeText(node)[0]) == exp:
						continue # discard this match

				if id in matched_assertions:
					print "Spec statement %s matches multiple places" % id
				matched_assertions[id] = True

				if m.lastindex != 1:
					print "Spec statement %s has incorrect number of match groups" % id

				end = m.end(1)
				end_node = None
				for end_node, o in offsets:
					if end < o:
						break
					end -= o
				assert(end_node)

				n1 = doc.createElement('span')
				n1.setAttribute('class', 'testrefs kw-%s' % keyword)
				n1.setAttribute('id', 'testrefs.%s' % id)
				n1.appendChild(doc.createTextNode(' '))
				
				n = n1.appendChild(doc.createElement('a'))
				n.setAttribute('href', '#testrefs.%s' % id)
				n.setAttribute('title', id)
				n.appendChild(doc.createTextNode('#'))

				n1.appendChild(doc.createTextNode(' '))
				for test_id in spec_refs.get(id, []):
					n = n1.appendChild(doc.createElement('a'))
					n.setAttribute('href', '%s.html' % test_id)
					n.appendChild(doc.createTextNode(test_id))
					n1.appendChild(doc.createTextNode(' '))
				n0 = doc.createTextNode(end_node.nodeValue[:end])
				n2 = doc.createTextNode(end_node.nodeValue[end:])

				p = end_node.parentNode
				p.replaceChild(n2, end_node)
				p.insertBefore(n1, n2)
				p.insertBefore(n0, n1)

				t, offsets = getNodeText(e)

	for e in doc.getElementsByTagName('body')[0].childNodes:
		if e.nodeType == e.ELEMENT_NODE and e.getAttribute('class') == 'impl':
			for c in e.childNodes:
				process_element(c)
		else:
			process_element(e)

	for s in spec_assertions:
		if s['id'] not in matched_assertions:
			print "Annotation incomplete: Unmatched spec statement %s" % s['id']

	# Convert from XHTML5 back to HTML5
	doc.documentElement.removeAttribute('xmlns')
	doc.documentElement.setAttribute('lang', doc.documentElement.getAttribute('xml:lang'))

	head = doc.documentElement.getElementsByTagName('head')[0]
	head.insertBefore(doc.createElement('meta'), head.firstChild).setAttribute('charset', 'UTF-8')

	codecs.open('tests/spec.html', 'w', 'utf-8').write(html5Serializer(doc))

write_index()
write_category_indexes()
write_reportgen()
write_results()
write_annotated_spec()

