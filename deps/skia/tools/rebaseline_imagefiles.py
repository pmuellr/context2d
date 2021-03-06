#!/usr/bin/python

'''
Copyright 2013 Google Inc.

Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
'''

'''
Rebaselines GM test results as individual image files
(the "old way", before https://goto.google.com/ChecksumTransitionDetail ).

Once we have switched our expectations to JSON form for all platforms,
we can delete this file.

There is a lot of code duplicated between here and rebaseline.py, but
that's fine because we will delete this file soon.
'''

# System-level imports
import os
import re
import subprocess
import sys
import urllib2

# Imports from within Skia
#
# We need to add the 'gm' directory, so that we can import gm_json.py within
# that directory.  That script allows us to parse the actual-results.json file
# written out by the GM tool.
# Make sure that the 'gm' dir is in the PYTHONPATH, but add it at the *end*
# so any dirs that are already in the PYTHONPATH will be preferred.
#
# This assumes that the 'gm' directory has been checked out as a sibling of
# the 'tools' directory containing this script, which will be the case if
# 'trunk' was checked out as a single unit.
GM_DIRECTORY = os.path.realpath(
    os.path.join(os.path.dirname(os.path.dirname(__file__)), 'gm'))
if GM_DIRECTORY not in sys.path:
    sys.path.append(GM_DIRECTORY)
import gm_json


class CommandFailedException(Exception):
    pass

class ImageRebaseliner(object):

    # params:
    #  expectations_root: root directory of all expectations
    #  json_base_url: base URL from which to read json_filename
    #  json_filename: filename (under json_base_url) from which to read a
    #                 summary of results; typically "actual-results.json"
    #  tests: list of tests to rebaseline, or None if we should rebaseline
    #         whatever files the JSON results summary file tells us to
    #  configs: which configs to run for each test; this should only be
    #           specified if the list of tests was also specified (otherwise,
    #           the JSON file will give us test names and configs)
    #  dry_run: if True, instead of actually downloading files or adding
    #           files to checkout, display a list of operations that
    #           we would normally perform
    #  add_new: if True, add expectations for tests which don't have any yet
    #  missing_json_is_fatal: whether to halt execution if we cannot read a
    #                         JSON actual result summary file
    def __init__(self, expectations_root, json_base_url, json_filename,
                 tests=None, configs=None, dry_run=False,
                 add_new=False, missing_json_is_fatal=False):
        if configs and not tests:
            raise ValueError('configs should only be specified if tests ' +
                             'were specified also')
        self._expectations_root = expectations_root
        self._tests = tests
        self._configs = configs
        self._json_base_url = json_base_url
        self._json_filename = json_filename
        self._dry_run = dry_run
        self._add_new = add_new
        self._missing_json_is_fatal = missing_json_is_fatal
        self._googlestorage_gm_actuals_root = (
            'http://chromium-skia-gm.commondatastorage.googleapis.com/gm')
        self._testname_pattern = re.compile('(\S+)_(\S+).png')
        self._is_svn_checkout = (
            os.path.exists(os.path.join(expectations_root, '.svn')) or
            os.path.exists(os.path.join(expectations_root, os.pardir, '.svn')))
        self._is_git_checkout = (
            os.path.exists(os.path.join(expectations_root, '.git')) or
            os.path.exists(os.path.join(expectations_root, os.pardir, '.git')))

    # If dry_run is False, execute subprocess.call(cmd).
    # If dry_run is True, print the command we would have otherwise run.
    # Raises a CommandFailedException if the command fails.
    def _Call(self, cmd):
        if self._dry_run:
            print '%s' % ' '.join(cmd)
            return
        if subprocess.call(cmd) != 0:
            raise CommandFailedException('error running command: ' +
                                         ' '.join(cmd))

    # Download a single actual result from GoogleStorage, returning True if it
    # succeeded.
    def _DownloadFromGoogleStorage(self, infilename, outfilename, all_results):
        test_name = self._testname_pattern.match(infilename).group(1)
        if not test_name:
            print '# unable to find test_name for infilename %s' % infilename
            return False
        try:
            hash_type, hash_value = all_results[infilename]
        except KeyError:
            print ('# unable to find filename %s in all_results dict' %
                   infilename)
            return False
        except ValueError as e:
            print '# ValueError reading filename %s from all_results dict: %s'%(
                infilename, e)
            return False
        url = '%s/%s/%s/%s.png' % (self._googlestorage_gm_actuals_root,
                                   hash_type, test_name, hash_value)
        try:
            self._DownloadFile(source_url=url, dest_filename=outfilename)
            return True
        except CommandFailedException:
            print '# Couldn\'t fetch gs_url %s' % url
            return False

    # Download a single actual result from skia-autogen, returning True if it
    # succeeded.
    def _DownloadFromAutogen(self, infilename, outfilename,
                             expectations_subdir, builder_name):
        url = ('http://skia-autogen.googlecode.com/svn/gm-actual/' +
               expectations_subdir + '/' + builder_name + '/' +
               expectations_subdir + '/' + infilename)
        try:
            self._DownloadFile(source_url=url, dest_filename=outfilename)
            return True
        except CommandFailedException:
            print '# Couldn\'t fetch autogen_url %s' % url
            return False

    # Download a single file, raising a CommandFailedException if it fails.
    def _DownloadFile(self, source_url, dest_filename):
        # Download into a temporary file and then rename it afterwards,
        # so that we don't corrupt the existing file if it fails midway thru.
        temp_filename = os.path.join(os.path.dirname(dest_filename),
                                     '.temp-' + os.path.basename(dest_filename))

        # TODO(epoger): Replace calls to "curl"/"mv" (which will only work on
        # Unix) with a Python HTTP library (which should work cross-platform)
        self._Call([ 'curl', '--fail', '--silent', source_url,
                     '--output', temp_filename ])
        self._Call([ 'mv', temp_filename, dest_filename ])

    # Returns the full contents of a URL, as a single string.
    #
    # Unlike standard URL handling, we allow relative "file:" URLs;
    # for example, "file:one/two" resolves to the file ./one/two
    # (relative to current working dir)
    def _GetContentsOfUrl(self, url):
        file_prefix = 'file:'
        if url.startswith(file_prefix):
            filename = url[len(file_prefix):]
            return open(filename, 'r').read()
        else:
            return urllib2.urlopen(url).read()

    # Returns a dictionary of actual results from actual-results.json file.
    #
    # The dictionary returned has this format:
    # {
    #  u'imageblur_565.png': [u'bitmap-64bitMD5', 3359963596899141322],
    #  u'imageblur_8888.png': [u'bitmap-64bitMD5', 4217923806027861152],
    #  u'shadertext3_8888.png': [u'bitmap-64bitMD5', 3713708307125704716]
    # }
    #
    # If the JSON actual result summary file cannot be loaded, the behavior
    # depends on self._missing_json_is_fatal:
    # - if true: execution will halt with an exception
    # - if false: we will log an error message but return an empty dictionary
    #
    # params:
    #  json_url: URL pointing to a JSON actual result summary file
    #  sections: a list of section names to include in the results, e.g.
    #            [gm_json.JSONKEY_ACTUALRESULTS_FAILED,
    #             gm_json.JSONKEY_ACTUALRESULTS_NOCOMPARISON] ;
    #            if None, then include ALL sections.
    def _GetActualResults(self, json_url, sections=None):
        try:
            json_contents = self._GetContentsOfUrl(json_url)
        except (urllib2.HTTPError, IOError):
            message = 'unable to load JSON summary URL %s' % json_url
            if self._missing_json_is_fatal:
                raise ValueError(message)
            else:
                print '# %s' % message
                return {}

        json_dict = gm_json.LoadFromString(json_contents)
        results_to_return = {}
        actual_results = json_dict[gm_json.JSONKEY_ACTUALRESULTS]
        if not sections:
            sections = actual_results.keys()
        for section in sections:
            section_results = actual_results[section]
            if section_results:
                results_to_return.update(section_results)
        return results_to_return

    # Returns a list of files that require rebaselining.
    #
    # Note that this returns a list of FILES, like this:
    #  ['imageblur_565.png', 'xfermodes_pdf.png']
    # rather than a list of TESTS, like this:
    #  ['imageblur', 'xfermodes']
    #
    # params:
    #  json_url: URL pointing to a JSON actual result summary file
    #  add_new: if True, then return files listed in any of these sections:
    #            - JSONKEY_ACTUALRESULTS_FAILED
    #            - JSONKEY_ACTUALRESULTS_NOCOMPARISON
    #           if False, then return files listed in these sections:
    #            - JSONKEY_ACTUALRESULTS_FAILED
    #
    def _GetFilesToRebaseline(self, json_url, add_new):
        if self._dry_run:
            print ''
            print '#'
        print ('# Getting files to rebaseline from JSON summary URL %s ...'
               % json_url)
        sections = [gm_json.JSONKEY_ACTUALRESULTS_FAILED]
        if add_new:
            sections.append(gm_json.JSONKEY_ACTUALRESULTS_NOCOMPARISON)
        results_to_rebaseline = self._GetActualResults(json_url=json_url,
                                                       sections=sections)
        files_to_rebaseline = results_to_rebaseline.keys()
        files_to_rebaseline.sort()
        print '# ... found files_to_rebaseline %s' % files_to_rebaseline
        if self._dry_run:
            print '#'
        return files_to_rebaseline

    # Rebaseline a single file.
    def _RebaselineOneFile(self, expectations_subdir, builder_name,
                           infilename, outfilename, all_results):
        if self._dry_run:
            print ''
        print '# ' + infilename

        # First try to download this result image from Google Storage.
        # If that fails, try skia-autogen.
        # If that fails too, just go on to the next file.
        #
        # This not treated as a fatal failure because not all
        # platforms generate all configs (e.g., Android does not
        # generate PDF).
        #
        # TODO(epoger): Once we are downloading only files that the
        # actual-results.json file told us to, this should become a
        # fatal error.  (If the actual-results.json file told us that
        # the test failed with XXX results, we should be able to download
        # those results every time.)
        if not self._DownloadFromGoogleStorage(infilename=infilename,
                                               outfilename=outfilename,
                                               all_results=all_results):
            if not self._DownloadFromAutogen(infilename=infilename,
                                             outfilename=outfilename,
                                             expectations_subdir=expectations_subdir,
                                             builder_name=builder_name):
                print '# Couldn\'t fetch infilename ' + infilename
                return

        # Add this file to version control (if appropriate).
        if self._add_new:
            if self._is_svn_checkout:
                cmd = [ 'svn', 'add', '--quiet', outfilename ]
                self._Call(cmd)
                cmd = [ 'svn', 'propset', '--quiet', 'svn:mime-type',
                        'image/png', outfilename ];
                self._Call(cmd)
            elif self._is_git_checkout:
                cmd = [ 'git', 'add', outfilename ]
                self._Call(cmd)

    # Rebaseline the given configs for a single test.
    #
    # params:
    #  expectations_subdir
    #  builder_name
    #  test: a single test to rebaseline
    #  all_results: a dictionary of all actual results
    def _RebaselineOneTest(self, expectations_subdir, builder_name, test,
                           all_results):
        if self._configs:
            configs = self._configs
        else:
            if (expectations_subdir == 'base-shuttle-win7-intel-angle'):
                configs = [ 'angle', 'anglemsaa16' ]
            else:
                configs = [ '565', '8888', 'gpu', 'pdf', 'mesa', 'msaa16',
                            'msaa4' ]
        if self._dry_run:
            print ''
        print '# ' + expectations_subdir + ':'
        for config in configs:
            infilename = test + '_' + config + '.png'
            outfilename = os.path.join(self._expectations_root,
                                       expectations_subdir, infilename);
            self._RebaselineOneFile(expectations_subdir=expectations_subdir,
                                    builder_name=builder_name,
                                    infilename=infilename,
                                    outfilename=outfilename,
                                    all_results=all_results)

    # Rebaseline all tests/types we specified in the constructor,
    # within this gm-expectations subdir.
    #
    # params:
    #  subdir : e.g. 'base-shuttle-win7-intel-float'
    #  builder : e.g. 'Test-Win7-ShuttleA-HD2000-x86-Release'
    def RebaselineSubdir(self, subdir, builder):
        json_url = '/'.join([self._json_base_url,
                             subdir, builder, subdir,
                             self._json_filename])
        all_results = self._GetActualResults(json_url=json_url)

        if self._tests:
            for test in self._tests:
                self._RebaselineOneTest(expectations_subdir=subdir,
                                        builder_name=builder,
                                        test=test, all_results=all_results)
        else:  # get the raw list of files that need rebaselining from JSON
            filenames = self._GetFilesToRebaseline(json_url=json_url,
                                                   add_new=self._add_new)
            for filename in filenames:
                outfilename = os.path.join(subdir, filename);
                self._RebaselineOneFile(expectations_subdir=subdir,
                                        builder_name=builder,
                                        infilename=filename,
                                        outfilename=outfilename,
                                        all_results=all_results)
