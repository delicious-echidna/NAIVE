//
//
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <String>
#include <list>
#include <windows.h>
#include <assert.h>

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

#include "backend.h"
#include "Jsonlib.h"

/*
TABLE asset

	asset_id INTEGER PRIMARY KEY AUTO_INCREMENT,
	1 ipv4 varchar(255),
	2 mac_address varchar(255),
	3 scan_method varchar(255),
	4 ipv6 varchar(255),
	5 vendor varchar(255),
	6 os varchar(255),
	7 date_last_seen varchar(255),
	8 other_attributes varchar(255)

*/

using namespace std;


int main()
{

	//
	//int db_insert_asset(string ip4, string dns = "NULL", string subnet = "0.0.0.0",
	//	string date = "NULL", string mac = "NULL", string vend = "NULL");
	db_insert_asset("0.0.0.4", "dns4", "0.0.0.0", "mac4", "vendy");
	db_insert_asset("0.0.0.5", "dns5", "0.0.0.0", "mac5", "flexo");
	db_insert_asset("0.0.0.6", "dns6", "0.0.0.0", "mac6", "vendy");
	
	//db_delete("0.0.0.6");
	
	list<string> s = db_select_asset();

	for (string i : s)
		cout << i << endl;

	createjson(db_select_asset());
	create_csv();

	return 0;
}

