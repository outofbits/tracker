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


# Including all tests in this file ensures that they can be run with
# `python -m unittest module` as well as `python -m unittest discover module`.


import test_backup_restore
import test_coalesce
import test_collation
import test_concurrent_query
import test_distance
import test_fts_functions
import test_graph
import test_group_concat
import test_insertion
import test_ontology_changes
import test_signals
import test_sparql_bugs
import test_sqlite_batch_misused
import test_sqlite_misused
import test_statistics
import test_threaded_store
import test_transactions
import test_unique_insertions
