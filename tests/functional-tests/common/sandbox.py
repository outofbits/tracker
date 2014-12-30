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


import os
import shutil
import subprocess
import tempfile

from logging import info


class TrackerSandbox(object):
    '''FIXME: merge with 'tracker-sandbox' tool

    This class handles isolating each test case from both other test cases and
    from the host system.

    '''
    def __init__(self, user_dirs=True, message_bus=True):
        self.tempdir = None
        self.dbus_process = self.dbus_address = None

        self._old_environ = os.environ

        if user_dirs:
            self.tempdir = self._sandbox_user_dirs()

        if message_bus:
            self.dbus_process, self.dbus_address = self._sandbox_message_bus()

    def _sandbox_user_dirs(self):
        tempdir = tempfile.mkdtemp(prefix='tracker-test')

        # We need to use the actual home directory for some tests because
        # Tracker will explicitly ignore files in /tmp ...
        os.environ['REAL_HOME'] = os.path.expanduser('~')

        # ... but /tmp is preferred for test data, to avoid leaving debris
        # in the filesystem
        os.environ['HOME'] = tempdir
        info("HOME=%s" % tempdir)

        os.environ['XDG_DATA_HOME'] = os.path.join(tempdir, 'data')
        os.environ['XDG_CACHE_HOME'] = os.path.join(tempdir, 'cache')

        return tempdir

    def _sandbox_message_bus(self):
        dbus_process = subprocess.Popen(
            ["dbus-daemon", "--session", "--print-address=1", "--fork"],
            stdout=subprocess.PIPE)
        dbus_address = dbus_process.stdout.readline().rstrip()

        os.environ['DBUS_SESSION_BUS_ADDRESS'] = dbus_address
        info("DBUS_SESSION_BUS_ADDRESS=%s" % dbus_address)

        return dbus_process, dbus_address

    def _stop_message_bus(self, dbus_process):
        info('Stopping D-Bus daemon (PID %i) ...' % (dbus_process.pid))
        dbus_process.terminate()
        dbus_process.wait()

    def _remove_tempdir(self, tempdir):
        shutil.rmtree(tempdir)

    def close(self):
        if self.tempdir:
            self._remove_tempdir(self.tempdir)
        if self.dbus_process:
            self._stop_message_bus(self.dbus_process)

        os.environ = self._old_environ
