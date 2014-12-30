# Copyright (C) 2010, Nokia <ivan.frade@nokia.com>
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


from gi.repository import GLib

import shutil
import os
import unittest

from itertools import chain

import common.helpers

from common.utils import configuration as cfg


MINER_TMP_DIR = cfg.TEST_MONITORED_TMP_DIR


def path(filename):
    return os.path.join(MINER_TMP_DIR, filename)


def uri(filename):
    return "file://" + os.path.join(MINER_TMP_DIR, filename)


DEFAULT_TEXT = "Some stupid content, to have a test file"

index_dirs = [os.path.join(MINER_TMP_DIR, "test-monitored")]
CONF_OPTIONS = {
    cfg.DCONF_MINER_SCHEMA: {
        'index-recursive-directories': GLib.Variant.new_strv(index_dirs),
        'index-single-directories': GLib.Variant.new_strv([]),
        'index-optical-discs': GLib.Variant.new_boolean(False),
        'index-removable-devices': GLib.Variant.new_boolean(False),
        'throttle': GLib.Variant.new_int32(5),
    }
}


class MinerTestCase (unittest.TestCase):
    def prepare_directories(self):
        #
        #     ~/test-monitored/
        #                     /file1.txt
        #                     /dir1/
        #                          /file2.txt
        #                          /dir2/
        #                               /file3.txt
        #
        #
        #     ~/test-no-monitored/
        #                        /file0.txt
        #

        monitored_files = [
            'test-monitored/file1.txt',
            'test-monitored/dir1/file2.txt',
            'test-monitored/dir1/dir2/file3.txt'
        ]

        unmonitored_files = [
            'test-no-monitored/file0.txt'
        ]

        def ensure_dir_exists(dirname):
            if not os.path.exists(dirname):
                os.makedirs(dirname)

        for tf in chain(monitored_files, unmonitored_files):
            testfile = path(tf)
            ensure_dir_exists(os.path.dirname(testfile))
            with open(testfile, 'w') as f:
                f.write(DEFAULT_TEXT)

        for tf in monitored_files:
            self.tracker.await_resource_inserted(
                'nfo:TextDocument', url=uri(tf))

    def setUp(self):
        for d in ['test-monitored', 'test-no-monitored']:
            dirname = path(d)
            if os.path.exists(dirname):
                shutil.rmtree(dirname)
            os.makedirs(dirname)

        self.sandbox = common.sandbox.TrackerSandbox(CONF_OPTIONS)
        self.store = common.helpers.StoreHelper()
        self.store.start(self.sandbox)

        self.extractor = common.helpers.ExtractorHelper()
        self.extractor.start(self.sandbox)

        self.miner_fs = common.helpers.MinerFsHelper()
        self.miner_fs.start(self.sandbox)

        self.tracker = self.store

        try:
            self.prepare_directories()
            self.tracker.reset_graph_updates_tracking()
        except Exception:
            self.tearDown()
            raise

    def tearDown(self):
        self.extractor.stop()
        self.miner_fs.stop()
        self.store.stop()
