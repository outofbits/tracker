#!/usr/bin/python
import os
import shutil
import configuration as cfg

from dconf import DConfClient

import helpers

# Add this after fixing the backup/restore and ontology changes tests
#"G_DEBUG" : "fatal_criticals",

TEST_ENV_DIRS = {"XDG_DATA_HOME": os.path.join(cfg.TEST_TMP_DIR, "data"),
                 "XDG_CACHE_HOME": os.path.join(cfg.TEST_TMP_DIR, "cache")}

TEST_ENV_VARS = {"TRACKER_DISABLE_MEEGOTOUCH_LOCALE": "",
                 "LC_COLLATE": "en_GB.utf8",
                 "DCONF_PROFILE": os.path.join(cfg.DATADIR, "tracker-tests",
                                               "trackertest")}

EXTRA_DIRS = [os.path.join(cfg.TEST_TMP_DIR, "data", "tracker"),
              os.path.join(cfg.TEST_TMP_DIR, "cache", "tracker")]

REASONABLE_TIMEOUT = 30


class UnableToBootException (Exception):
    pass


class TrackerSystemAbstraction:

    def set_up_environment(self, settings, ontodir):
        """
        Sets up the XDG_*_HOME variables and make sure the directories exist

        Settings should be a dict mapping schema names to dicts that hold the
        settings that should be changed in those schemas. The contents dicts
        should map key->value, where key is a key name and value is a suitable
        GLib.Variant instance.
        """

        helpers.log("[Conf] Setting test environment...")

        for var, directory in TEST_ENV_DIRS.iteritems():
            helpers.log("export %s=%s" % (var, directory))
            self.__recreate_directory(directory)
            os.environ[var] = directory

        for directory in EXTRA_DIRS:
            self.__recreate_directory(directory)

        for var, value in TEST_ENV_VARS.iteritems():
            helpers.log("export %s=%s" % (var, value))
            os.environ[var] = value

        # Previous loop should have set DCONF_PROFILE to the test location
        if settings is not None:
            self._apply_settings(settings)

        helpers.log("[Conf] environment ready")

    def _apply_settings(self, settings):
        for schema_name, contents in settings.iteritems():
            dconf = DConfClient(schema_name)
            dconf.reset()
            for key, value in contents.iteritems():
                dconf.write(key, value)

    def tracker_store_stop_brutally(self):
        self.store.kill()

    def tracker_miner_fs_testing_start(self, config, dbus_address):
        """
        Stops any previous instance of the store and miner, calls set_up_environment,
        and starts a new instance of the store and miner-fs
        """
        self.set_up_environment(config, None)

        self.store = helpers.StoreHelper()
        self.store.start(dbus_address)

        self.extractor = helpers.ExtractorHelper()
        self.extractor.start(dbus_address)

        self.miner_fs = helpers.MinerFsHelper()
        self.miner_fs.start(dbus_address)

    def tracker_miner_fs_testing_stop(self):
        """
        Stops the extractor, miner-fs and store running
        """
        self.extractor.stop()
        self.miner_fs.stop()
        self.store.stop()

    def tracker_writeback_testing_start(self, config, dbus_address):
        # Start the miner-fs (and store) and then the writeback process
        self.tracker_miner_fs_testing_start(config, dbus_address)
        self.writeback = helpers.WritebackHelper()
        self.writeback.start(dbus_address)

    def tracker_writeback_testing_stop(self):
        # Tracker write must have been started before
        self.writeback.stop()

        self.tracker_miner_fs_testing_stop()

    def tracker_all_testing_start(self, config, dbus_address):
        # This will start all miner-fs, store and writeback
        self.tracker_writeback_testing_start(config, dbus_address)

    def tracker_all_testing_stop(self):
        # This will stop all miner-fs, store and writeback
        self.tracker_writeback_testing_stop()

    def __recreate_directory(self, directory):
        if (os.path.exists(directory)):
            shutil.rmtree(directory)
        os.makedirs(directory)
