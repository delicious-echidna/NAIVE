


#pragma once
#ifndef Database_H
#define Database_H

#include <iostream>
#include <string>
#include <list>
#include "sqlite/sqlite3.h"

using namespace std;

list<string> query(string query);

int insert(string mac_address, string ipv4 = "NULL", string scan_method = "NULL",
	string ipv6 = "NULL", string vendor = "NULL", string os = "NULL", 
	string date_last_seen = "NULL", string other_attributes = "NULL");

list<string> findall();

list<string> find(int asset_id, string mac_address = "NULL", string scan_method = "NULL",
	string ipv4 = "NULL", string ipv6 = "NULL", string vendor = "NULL", 
	string os = "NULL", string date_last_seen = "NULL", string other_attributes = "NULL");

int remove(int asset_id);

void printlist(list<string> input);

#endif