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


import unittest

import common


class TrackerStoreTest(unittest.TestCase):

    '''Common superclass for tests that just require a fresh store running.

    The tracker-store test suites run each test case in one instance of
    tracker-store, unlike the other ones which run each test in a clean
    sandbox. The store is robust enough that the tests don't really interfere
    with each other, and it takes a few seconds for the store to start up fully
    if the database is empty.

    '''

    @classmethod
    def setUpClass(self):
        self.sandbox = common.sandbox.TrackerSandbox()

        self.store = common.helpers.StoreHelper()
        self.store.start(self.sandbox)

        self.tracker = self.store

    @classmethod
    def tearDownClass(self):
        self.store.stop()

        self.sandbox.close()
