#!/usr/bin/python

'''
Copyright 2012 Google Inc.

Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
'''

'''
Rebaselines the given GM tests, on all bots and all configurations.
Must be run from the gm-expected directory.  If run from a git or SVN
checkout, the files will be added to the staging area for commit.
'''

# System-level imports
import argparse
import os
import re
import sys
import urllib2

# Imports from local directory
import rebaseline_imagefiles

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

# Mapping of gm-expectations subdir (under
# https://skia.googlecode.com/svn/gm-expected/ )
# to builder name (see list at http://108.170.217.252:10117/builders )
SUBDIR_MAPPING = {
   'base-shuttle-win7-intel-float':
    'Test-Win7-ShuttleA-HD2000-x86-Release',
   'base-shuttle-win7-intel-angle':
    'Test-Win7-ShuttleA-HD2000-x86-Release-ANGLE',
   'base-shuttle-win7-intel-directwrite':
    'Test-Win7-ShuttleA-HD2000-x86-Release-DirectWrite',
   'base-shuttle_ubuntu12_ati5770':
    'Test-Ubuntu12-ShuttleA-ATI5770-x86_64-Release',
   'base-macmini':
    'Test-Mac10.6-MacMini4.1-GeForce320M-x86-Release',
   'base-macmini-lion-float':
    'Test-Mac10.7-MacMini4.1-GeForce320M-x86-Release',
   'base-android-galaxy-nexus':
    'Test-Android-GalaxyNexus-SGX540-Arm7-Debug',
   'base-android-nexus-7':
    'Test-Android-Nexus7-Tegra3-Arm7-Release',
   'base-android-nexus-s':
    'Test-Android-NexusS-SGX540-Arm7-Release',
   'base-android-xoom':
    'Test-Android-Xoom-Tegra2-Arm7-Release',
   'base-android-nexus-10':
    'Test-Android-Nexus10-MaliT604-Arm7-Release',
   'base-android-nexus-4':
    'Test-Android-Nexus4-Adreno320-Arm7-Release',
}


class CommandFailedException(Exception):
    pass

# Object that rebaselines a JSON expectations file (not individual image files).
class JsonRebaseliner(object):

    # params:
    #  expectations_root: root directory of all expectations JSON files
    #  expectations_filename: filename (under expectations_root) of JSON
    #                         expectations file; typically
    #                         "expected-results.json"
    #  actuals_base_url: base URL from which to read actual-result JSON files
    #  actuals_filename: filename (under actuals_base_url) from which to read a
    #                    summary of results; typically "actual-results.json"
    #  tests: list of tests to rebaseline, or None if we should rebaseline
    #         whatever files the JSON results summary file tells us to
    #  configs: which configs to run for each test; this should only be
    #           specified if the list of tests was also specified (otherwise,
    #           the JSON file will give us test names and configs)
    #  add_new: if True, add expectations for tests which don't have any yet
    def __init__(self, expectations_root, expectations_filename,
                 actuals_base_url, actuals_filename,
                 tests=None, configs=None, add_new=False):
        if configs and not tests:
            raise ValueError('configs should only be specified if tests ' +
                             'were specified also')
        self._expectations_root = expectations_root
        self._expectations_filename = expectations_filename
        self._tests = tests
        self._configs = configs
        self._actuals_base_url = actuals_base_url
        self._actuals_filename = actuals_filename
        self._add_new = add_new
        self._testname_pattern = re.compile('(\S+)_(\S+).png')

    # Returns the full contents of filepath, as a single string.
    # If filepath looks like a URL, try to read it that way instead of as
    # a path on local storage.
    def _GetFileContents(self, filepath):
        if filepath.startswith('http:') or filepath.startswith('https:'):
            return urllib2.urlopen(filepath).read()
        else:
            return open(filepath, 'r').read()

    # Returns a dictionary of actual results from actual-results.json file.
    #
    # The dictionary returned has this format:
    # {
    #  u'imageblur_565.png': [u'bitmap-64bitMD5', 3359963596899141322],
    #  u'imageblur_8888.png': [u'bitmap-64bitMD5', 4217923806027861152],
    #  u'shadertext3_8888.png': [u'bitmap-64bitMD5', 3713708307125704716]
    # }
    #
    # If the JSON actual result summary file cannot be loaded, raise an
    # exception.
    #
    # params:
    #  json_url: URL pointing to a JSON actual result summary file
    #  sections: a list of section names to include in the results, e.g.
    #            [gm_json.JSONKEY_ACTUALRESULTS_FAILED,
    #             gm_json.JSONKEY_ACTUALRESULTS_NOCOMPARISON] ;
    #            if None, then include ALL sections.
    def _GetActualResults(self, json_url, sections=None):
        json_contents = self._GetFileContents(json_url)
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

    # Rebaseline all tests/types we specified in the constructor,
    # within this gm-expectations subdir.
    #
    # params:
    #  subdir : e.g. 'base-shuttle-win7-intel-float'
    #  builder : e.g. 'Test-Win7-ShuttleA-HD2000-x86-Release'
    def RebaselineSubdir(self, subdir, builder):
        # Read in the actual result summary, and extract all the tests whose
        # results we need to update.
        actuals_url = '/'.join([self._actuals_base_url,
                                subdir, builder, subdir,
                                self._actuals_filename])
        sections = [gm_json.JSONKEY_ACTUALRESULTS_FAILED]
        if self._add_new:
            sections.append(gm_json.JSONKEY_ACTUALRESULTS_NOCOMPARISON)
        results_to_update = self._GetActualResults(json_url=actuals_url,
                                                   sections=sections)

        # Read in current expectations.
        expectations_json_filepath = os.path.join(
            self._expectations_root, subdir, self._expectations_filename)
        expectations_dict = gm_json.LoadFromFile(expectations_json_filepath)

        # Update the expectations in memory, skipping any tests/configs that
        # the caller asked to exclude.
        skipped_images = []
        if results_to_update:
            for (image_name, image_results) in results_to_update.iteritems():
                (test, config) = self._testname_pattern.match(image_name).groups()
                if self._tests:
                    if test not in self._tests:
                        skipped_images.append(image_name)
                        continue
                if self._configs:
                    if config not in self._configs:
                        skipped_images.append(image_name)
                        continue
                expectations_dict[gm_json.JSONKEY_EXPECTEDRESULTS] \
                                 [image_name] \
                                 [gm_json.JSONKEY_EXPECTEDRESULTS_ALLOWEDDIGESTS] = \
                                     [image_results]

        # Write out updated expectations.
        gm_json.WriteToFile(expectations_dict, expectations_json_filepath)

        if skipped_images:
            print ('Skipped these tests due to test/config filters: %s' %
                   skipped_images)


# main...

parser = argparse.ArgumentParser()
parser.add_argument('--actuals-base-url',
                    help='base URL from which to read files containing JSON ' +
                    'summaries of actual GM results; defaults to %(default)s',
                    default='http://skia-autogen.googlecode.com/svn/gm-actual')
parser.add_argument('--actuals-filename',
                    help='filename (within platform-specific subdirectories ' +
                    'of ACTUALS_BASE_URL) to read a summary of results from; ' +
                    'defaults to %(default)s',
                    default='actual-results.json')
# TODO(epoger): Add test that exercises --add-new argument.
parser.add_argument('--add-new', action='store_true',
                    help='in addition to the standard behavior of ' +
                    'updating expectations for failing tests, add ' +
                    'expectations for tests which don\'t have expectations ' +
                    'yet.')
# TODO(epoger): Add test that exercises --configs argument.
# TODO(epoger): Once we are only rebaselining JSON files, update the helpstring
# to indicate that this is a *filter* over the config names that
# actual-results.json tells us need to be rebaselined.
# You don't need to specify tests also, etc.
parser.add_argument('--configs', metavar='CONFIG', nargs='+',
                    help='which configurations to rebaseline, e.g. ' +
                    '"--configs 565 8888"; if unspecified, run a default ' +
                    'set of configs. This should ONLY be specified if ' +
                    '--tests has also been specified.')
# TODO(epoger): The --dry-run argument will no longer be needed once we
# are only rebaselining JSON files.
parser.add_argument('--dry-run', action='store_true',
                    help='instead of actually downloading files or adding ' +
                    'files to checkout, display a list of operations that ' +
                    'we would normally perform')
parser.add_argument('--expectations-filename',
                    help='filename (under EXPECTATIONS_ROOT) to read ' +
                    'current expectations from, and to write new ' +
                    'expectations into; defaults to %(default)s',
                    default='expected-results.json')
parser.add_argument('--expectations-root',
                    help='root of expectations directory to update-- should ' +
                    'contain one or more base-* subdirectories. Defaults to ' +
                    '%(default)s',
                    default='.')
parser.add_argument('--subdirs', metavar='SUBDIR', nargs='+',
                    help='which platform subdirectories to rebaseline; ' +
                    'if unspecified, rebaseline all subdirs, same as ' +
                    '"--subdirs %s"' % ' '.join(sorted(SUBDIR_MAPPING.keys())))
# TODO(epoger): Add test that exercises --tests argument.
# TODO(epoger): Once we are only rebaselining JSON files, update the helpstring
# to indicate that this is a *filter* over the test names that
# actual-results.json tells us need to be rebaselined.
parser.add_argument('--tests', metavar='TEST', nargs='+',
                    help='which tests to rebaseline, e.g. ' +
                    '"--tests aaclip bigmatrix"; if unspecified, then all ' +
                    'failing tests (according to the actual-results.json ' +
                    'file) will be rebaselined.')
args = parser.parse_args()
if args.subdirs:
    subdirs = args.subdirs
    missing_json_is_fatal = True
else:
    subdirs = sorted(SUBDIR_MAPPING.keys())
    missing_json_is_fatal = False
for subdir in subdirs:
    if not subdir in SUBDIR_MAPPING.keys():
        raise Exception(('unrecognized platform subdir "%s"; ' +
                         'should be one of %s') % (
                             subdir, SUBDIR_MAPPING.keys()))
    builder = SUBDIR_MAPPING[subdir]

    # We instantiate different Rebaseliner objects depending
    # on whether we are rebaselining an expected-results.json file, or
    # individual image files.  Different gm-expected subdirectories may move
    # from individual image files to JSON-format expectations at different
    # times, so we need to make this determination per subdirectory.
    #
    # See https://goto.google.com/ChecksumTransitionDetail
    expectations_json_file = os.path.join(args.expectations_root, subdir,
                                          args.expectations_filename)
    if os.path.isfile(expectations_json_file):
        rebaseliner = JsonRebaseliner(
            expectations_root=args.expectations_root,
            expectations_filename=args.expectations_filename,
            tests=args.tests, configs=args.configs,
            actuals_base_url=args.actuals_base_url,
            actuals_filename=args.actuals_filename,
            add_new=args.add_new)
    else:
        rebaseliner = rebaseline_imagefiles.ImageRebaseliner(
            expectations_root=args.expectations_root,
            tests=args.tests, configs=args.configs,
            dry_run=args.dry_run,
            json_base_url=args.actuals_base_url,
            json_filename=args.actuals_filename,
            add_new=args.add_new,
            missing_json_is_fatal=missing_json_is_fatal)
    rebaseliner.RebaselineSubdir(subdir=subdir, builder=builder)
