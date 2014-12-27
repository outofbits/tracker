#!/usr/bin/env python
#
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
#
from common.utils import configuration as cfg
from common.utils.system import TrackerSystemAbstraction
from common.utils.helpers import log
import unittest as ut

from gi.repository import GLib

import shutil
import os
import time

APPLICATIONS_TMP_DIR = os.path.join(
    cfg.TEST_MONITORED_TMP_DIR, "test-applications-monitored")

index_dirs = [APPLICATIONS_TMP_DIR]
CONF_OPTIONS = {
    cfg.DCONF_MINER_SCHEMA: {
        'index-recursive-directories': GLib.Variant.new_strv(index_dirs),
        'index-single-directories': GLib.Variant.new_strv([]),
        'index-optical-discs': GLib.Variant.new_boolean(False),
        'index-removable-devices': GLib.Variant.new_boolean(False),
    }
}

# Copy rate, 10KBps (1024b/100ms)
SLOWCOPY_RATE = 1024


import subprocess
import tempfile

class TrackerTestCase(ut.TestCase):
    '''Base class for all Tracker functional tests.

    This class handles isolating each test case from both other test cases and
    from the host system.

    '''
    def setUp(self):
        self._old_environ = os.environ

        self.fake_home()
        self.launch_session_bus()

    def fake_home(self):
        self.tempdir = tempfile.mkdtemp(prefix='tracker-test')

        # We need to use the actual home directory for some tests because
        # Tracker will explicitly ignore files in /tmp ...
        os.environ['REAL_HOME'] = os.path.expanduser('~')

        # ... but /tmp is preferred for test data, to avoid leaving debris
        # in the filesystem
        os.environ['HOME'] = self.tempdir
        log("HOME=%s" % self.tempdir)

    def launch_session_bus(self):
        self.dbus_process = subprocess.Popen(
            ["dbus-daemon", "--session", "--print-address=1", "--fork"],
            stdout=subprocess.PIPE)
        self.dbus_address = self.dbus_process.stdout.readline().rstrip()

        os.environ['DBUS_SESSION_BUS_ADDRESS'] = self.dbus_address
        log("DBUS_SESSION_BUS_ADDRESS=%s" % self.dbus_address)

    def tearDown(self):
        log('Stopping D-Bus daemon (PID %i) ...' % (self.dbus_process.pid))
        self.dbus_process.terminate()
        self.dbus_process.wait()

        os.environ = self._old_environ


class CommonTrackerApplicationTest(TrackerTestCase):
    def get_urn_count_by_url(self, url):
        select = """
        SELECT ?u WHERE { ?u nie:url \"%s\" }
        """ % (url)
        return len(self.tracker.query(select))

    def get_test_image(self):
        TEST_IMAGE = "test-image-1.jpg"
        return TEST_IMAGE

    def get_test_video(self):
        TEST_VIDEO = "test-video-1.mp4"
        return TEST_VIDEO

    def get_test_music(self):
        TEST_AUDIO = "test-music-1.mp3"
        return TEST_AUDIO

    def get_data_dir(self):
        return self.datadir

    def get_dest_dir(self):
        return APPLICATIONS_TMP_DIR

    def slowcopy_file_fd(self, src, fdest, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        log("Copying slowly\n '%s' to\n '%s'" % (src, fdest.name))
        fsrc = open(src, 'rb')
        buffer_ = fsrc.read(rate)
        while (buffer_ != ""):
            fdest.write(buffer_)
            time.sleep(0.1)
            buffer_ = fsrc.read(rate)
        fsrc.close()

    def slowcopy_file(self, src, dst, rate=SLOWCOPY_RATE):
        """
        @rate: bytes per 100ms
        """
        fdest = open(dst, 'wb')
        self.slowcopy_file_fd(src, fdest, rate)
        fdest.close()

    def setUp(self):
        super(CommonTrackerApplicationTest, self).setUp()

        # Create temp directory to monitor
        if (os.path.exists(APPLICATIONS_TMP_DIR)):
            shutil.rmtree(APPLICATIONS_TMP_DIR)
        os.makedirs(APPLICATIONS_TMP_DIR)

        # Use local directory if available. Installation otherwise.
        if os.path.exists(os.path.join(os.getcwd(),
                                       "test-apps-data")):
            self.datadir = os.path.join(os.getcwd(),
                                        "test-apps-data")
        else:
            self.datadir = os.path.join(cfg.DATADIR,
                                        "tracker-tests",
                                        "test-apps-data")

        self.system = TrackerSystemAbstraction()
        self.system.tracker_all_testing_start(CONF_OPTIONS)

        # Returns when ready
        self.tracker = self.system.store

        log("Ready to go!")

    def tearDown(self):
        self.system.tracker_all_testing_stop ()

        # Remove monitored directory
        if (os.path.exists (APPLICATIONS_TMP_DIR)):
            shutil.rmtree (APPLICATIONS_TMP_DIR)

        super(CommonTrackerApplicationTest, self).tearDown()
