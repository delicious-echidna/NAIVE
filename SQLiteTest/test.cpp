//
//
#include <iostream>
#include <String>
#include<list>

#include "Database.h"

int main() {

	int i = 1;

	insert("mac1", "ip1", "NULL", "vendy");
	insert("mac2", "ip2", "NULL", "vendy");
	insert("mac3", "ip3", "NULL", "flexo");
	insert("mac4", "ip4", "NULL", "flexo");
	insert("mac5", "ip5", "NULL", "flexo");


	insert("mac5", "ip6", "NULL", "vendy");

	printlist(findall());



	return 0;
}