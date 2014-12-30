# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
# Copyright (C) 2014, Sam Thursfield <sam@afuera.me.uk>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.


"""
For a collection of files, call the extractor and check that the expected
    metadata is extracted. Load dynamically the test information from a data
directory (containing xxx.expected files)
"""


import os
import unittest

from logging import info


import extractor_testcase


def load_tests(loader, standard_tests, pattern):
    '''Load all extractor test cases defined in data/

    Traverse the TEST_DATA_PATH directory looking for .description files. Add a
    new TestCase to the suite per .description file and run the suite.

    If we do this inside a single TestCase an error in one test would stop the
    whole testing.

    '''

    import logging
    logging.basicConfig()

    TEST_DATA_PATH = os.path.join(os.getcwd(), 'extractor', 'data')

    assert os.path.isdir(TEST_DATA_PATH), \
        "Directory %s doesn't exist" % TEST_DATA_PATH

    info("Loading test descriptions from %s", TEST_DATA_PATH)

    suite = unittest.TestSuite()
    print TEST_DATA_PATH
    for root, dirs, files in os.walk(TEST_DATA_PATH):
        descriptions = [os.path.join(root, f)
                        for f in files if f.endswith("expected")]
        for descfile in descriptions:
            tc = extractor_testcase.ExtractorTestCase(descfile=descfile)
            suite.addTest(tc)

    return suite
