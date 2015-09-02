/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//.pragma library

function getDatabase() {
     return LocalStorage.openDatabaseSync("wiLink", "1.0", "StorageDatabase", 100000);
}

/** Initialize database tables.
 */
function initialize() {
    var db = getDatabase();
    db.transaction(function(tx) {
        tx.executeSql('CREATE TABLE IF NOT EXISTS setting(key TEXT UNIQUE, value TEXT)');
    });
}

/** Retrieves a setting value.
 */
function getSetting(setting, fallback) {
    var db = getDatabase();
    var res = fallback;
    db.transaction(function(tx) {
        var rs = tx.executeSql('SELECT value FROM setting WHERE key=?;', [setting]);
        if (rs.rows.length > 0) {
            res = rs.rows.item(0).value;
        }
    });
    return res;
}

/** Removes a setting.
 */
function removeSetting(setting) {
    var db = getDatabase();
    db.transaction(function(tx) {
        tx.executeSql('DELETE FROM setting WHERE key=?;', [setting]);
    });
}

/** Stores a setting value.
 */
function setSetting(setting, value) {
    var db = getDatabase();
    var res = false;
    db.transaction(function(tx) {
        var rs = tx.executeSql('INSERT OR REPLACE INTO setting VALUES (?,?);', [setting,value]);
        if (rs.rowsAffected > 0) {
            res = true;
        }
    });
    return res;
}
