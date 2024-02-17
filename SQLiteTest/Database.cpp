#include <iostream>
#include <String>

#include "sqlite/sqlite3.h"

using namespace std;

/*
TABLE asset

    asset_id INTEGER PRIMARY KEY, 
    mac_address varchar(63), 
    ipv4 varchar(63), 
    ipv6 varchar(63), 
    vendor varchar(63), 
    os varchar(63), 
    date_last_seen varchar(63), 
    other_attributes varchar(255)

*/

string query(string query) {

    sqlite3* dbp = nullptr;
    sqlite3_stmt* curr = nullptr;
    const char* tail = nullptr;
    int rc;

    rc = sqlite3_open("asset.db", &dbp);
    if (rc != SQLITE_OK) {
        cout << "Error opening SQLite database: " << sqlite3_errmsg(dbp) << endl;
        sqlite3_close(dbp);
        return "0";
    }
    else {
        cout << "Great success in opening SQLite database!" << endl;
    }

    sqlite3_prepare_v2(dbp,
        u8"CREATE TABLE IF NOT EXISTS asset (asset_id INTEGER PRIMARY KEY, mac_address varchar(63), ipv4 varchar(63), ipv6 varchar(63), vendor varchar(63), os varchar(63), date_last_seen varchar(63), other_attributes varchar(255) )",
        -1, &curr, &tail);

    sqlite3_step(curr);
    sqlite3_finalize(curr);



    sqlite3_prepare_v2(dbp,
        query.c_str(),
        -1, &curr, &tail);

    string result;
    int counter = 0;

    while (sqlite3_step(curr) == SQLITE_ROW && counter < 100) {

        for (int i = 0; i < sqlite3_column_count(curr); i++) {

            if (sqlite3_column_type(curr, i) != SQLITE_NULL) {
                result += string(reinterpret_cast<const char*>(sqlite3_column_text(curr, i)));
                result += ",";
            }
            else {
                result += "NULL,";
            }
        }
        result.pop_back();
        result += "\n";

        counter++;
    }

    sqlite3_finalize(curr);
    sqlite3_close(dbp);

    if (result.empty()) {
        return "0";
    }
    else {
        return result;
    }
}

int insert(string mac_address, string ipv4 = "NULL", string ipv6 = "NULL", string vendor = "NULL", string os = "NULL", string date_last_seen = "NULL", string other_attributes = "NULL") {

    string insertme = "INSERT INTO asset (mac_address, ipv4, ipv6, vendor, os, date_last_seen, other_attributes) VALUES (";
    insertme += "'" + mac_address + "','";
    insertme += ipv4 + "','";
    insertme += ipv6 + "','";
    insertme += vendor + "','";
    insertme += os + "','";
    insertme += date_last_seen + "','";
    insertme += other_attributes + "')";

    //cout << insertme << endl;

    string result = query(insertme);
    if (result == "0") {
        return 0;
    }
    else {
        return 0;
    }
}

int remove(int asset_id) {
    string removeme = "DELETE FROM asset WHERE asset_id = " + to_string(asset_id);
    string result = query(removeme);

    if (result == "0") {
        return 0;
    }
    else {
        return 0;
    }
}

string find(int asset_id, string mac_address = "NULL", string ipv4 = "NULL", string ipv6 = "NULL", string vendor = "NULL", string os = "NULL", string date_last_seen = "NULL", string other_attributes = "NULL") {
    int counter = 0;
    string findme = "SELECT * FROM asset WHERE";
    if (asset_id != 0) {
        findme += " asset_id=" + to_string(asset_id) + " AND";
        counter++;
    }
    if (mac_address != "NULL") {
        findme += " mac_address = '" + mac_address + "' AND";
        counter++;
    }
    if (ipv4 != "NULL") {
        findme += " ipv4 = '" + ipv4 + "' AND";
        counter++;
    }
    if (ipv6 != "NULL") {
        findme += " ipv6 = '" + ipv6 + "' AND";
        counter++;
    }
    if (vendor != "NULL") {
        findme += " vendor = '" + vendor + "' AND";
        counter++;
    }
    if (os != "NULL") {
        findme += " os = '" + os + "' AND";
        counter++;
    }
    if (date_last_seen != "NULL") {
        findme += " date_last_seen = '" + date_last_seen + "' AND";
        counter++;
    }
    if (other_attributes != "NULL") {
        findme += " other_attributes = '" + other_attributes + "' AND";
        counter++;
    }

    if (counter == 0) {
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
    }
    else {
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
        findme.pop_back();
    }
    //cout << findme << "<>" << endl;
    return query(findme);
}


int main() {

    string n = "NULL";

    cout << find(2) << endl;
    cout << find(0) << endl;

	return 0;
}