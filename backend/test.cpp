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

	//db_insert("0.0.0.555", "50:00:00:00:00:05") ;
	//db_insert("0.0.0.6", "65:00:00:00:00:06", "arp", "NULL", "vendy", "NULL", "6-5-1001");
	
	//db_delete("0.0.0.6");
	/*
	list<string> s = db_select();

	for (string i : s)
		cout << i << endl;
	*/

	//createjson(db_select());

	return 0;
}

