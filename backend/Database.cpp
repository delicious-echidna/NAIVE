


#include <iostream>
#include <string>
#include <list>

#include "Database.h"
#include "sqlite/sqlite3.h"

using namespace std;

/*
TABLE asset

    asset_id INTEGER PRIMARY KEY, 
    mac_address varchar(255), 
    ipv4 varchar(255), 
    scan_method varchar(255),
    ipv6 varchar(255), 
    vendor varchar(255), 
    os varchar(255), 
    date_last_seen varchar(255), 
    other_attributes varchar(255)

*/

list<string> query(string query) {

    list<string> result;

    sqlite3* dbp = nullptr;
    sqlite3_stmt* curr = nullptr;
    const char* tail = nullptr;
    int rc;

    rc = sqlite3_open("asset.db", &dbp);
    if (rc != SQLITE_OK) {
        cout << "Error opening SQLite database: " << sqlite3_errmsg(dbp) << endl;
        sqlite3_close(dbp);
        return result;
    }
    else {
        //cout << "Great success in opening SQLite database!" << endl;
    }

    sqlite3_prepare_v2(dbp,
        u8"CREATE TABLE IF NOT EXISTS asset (asset_id INTEGER PRIMARY KEY, mac_address varchar(255), ipv4 varchar(255), scan_method varchar(255), ipv6 varchar(255), vendor varchar(255), os varchar(255), date_last_seen varchar(255), other_attributes varchar(255) )",
        -1, &curr, &tail);

    sqlite3_step(curr);
    sqlite3_finalize(curr);



    sqlite3_prepare_v2(dbp,
        query.c_str(),
        -1, &curr, &tail);

    int counter = 0;
    while (sqlite3_step(curr) == SQLITE_ROW && counter < 100) {

        string currline;
        for (int i = 0; i < sqlite3_column_count(curr); i++) {

            if (sqlite3_column_type(curr, i) != SQLITE_NULL) {
                currline += string(reinterpret_cast<const char*>(sqlite3_column_text(curr, i)));
                currline += ",";
            }
            else {
                currline += "NULL,";
            }
        }
        currline.pop_back();
        result.push_back(currline);

        counter++;
    }

    sqlite3_finalize(curr);
    sqlite3_close(dbp);

    return result;
}

int insert(string mac_address, string ipv4, string scan_method,
    string ipv6, string vendor, string os, 
    string date_last_seen, string other_attributes) {

    if (!find(0, mac_address, ipv4, scan_method, ipv6, vendor, os, date_last_seen, other_attributes).empty()) {
        return -1;
    }
    if (!find(0, mac_address).empty()) {
        remove(stoi(find(0, mac_address).front()));
        //proceed as normal.
    }

    string insertme = "INSERT INTO asset (mac_address, ipv4, scan_method, ipv6, vendor, os, date_last_seen, other_attributes) VALUES (";
    insertme += "'" + mac_address + "','";
    insertme += ipv4 + "','";
    insertme += scan_method + "','";
    insertme += ipv6 + "','";
    insertme += vendor + "','";
    insertme += os + "','";
    insertme += date_last_seen + "','";
    insertme += other_attributes + "')";

    //cout << insertme << endl;

    query(insertme);
    return 0;
}

int remove(int asset_id) {
    string removeme = "DELETE FROM asset WHERE asset_id = " + to_string(asset_id);
    query(removeme);

    return 0;
}

list<string> findall() {
    return query("SELECT * FROM asset");
}

list<string> find(int asset_id, string mac_address, string ipv4, string scan_method,
    string ipv6, string vendor, string os, string date_last_seen, 
    string other_attributes) {

    if (asset_id == -1) {
        return findall();
    }

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
    if (scan_method != "NULL") {
        findme += " scan_method = '" + scan_method + "' AND";
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



void printlist(list<string> input) {
    for (list<string>::iterator it = input.begin(); it != input.end(); ++it) {
        cout << ' ' << *it << "\n";
    }
}


