//
//
#include <iostream>
#include <String>
#include<list>

#include "Database.h"
#include "Jsonlib.h"

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

int main() {

	int i = 1;

	/*
	insert("mac1", "NULL", "arp", "ip61", "vendy", "windows20");
	insert("mac2", "ip2", "arp", "NULL", "vendy");
	insert("mac3", "ip3", "arp", "NULL", "flexo");
	insert("mac4", "ip4", "arp", "NULL", "flexo");
	insert("mac5", "ip5", "arp", "NULL", "flexo");

	insert("mac5", "ip6", "arp", "NULL", "vendy");
	*/

	printlist(findall());

	createjson(findall());

	return 0;
}