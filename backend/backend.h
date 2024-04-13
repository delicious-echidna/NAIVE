


#pragma once
#ifndef backend_H
#define backend_H

#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <String>
#include <list>
#include <cstdlib>
#include <ctime>

#include <windows.h>
#include <assert.h>

#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

using namespace std;

/*

        CREATE TABLE Networks (
            NetworkID INT IDENTITY(1,1) PRIMARY KEY,
            NetworkName VARCHAR(255) NOT NULL
        );

        CREATE TABLE Subnets (
            SubnetID INT IDENTITY(1,1) PRIMARY KEY,
            NetworkID INT,
            SubnetAddress VARCHAR(18) NOT NULL,
            Description VARCHAR(255),
            FOREIGN KEY (NetworkID) REFERENCES Networks(NetworkID)
        );

        CREATE TABLE Assets (
            SubnetID INT,
            IPV4 VARCHAR(15) PRIMARY KEY,
            DNS VARCHAR(50),
            Date_last_seen VARCHAR(50),
            FOREIGN KEY (SubnetID) REFERENCES Subnets(SubnetID)
        );

        CREATE TABLE MACInfo (
            IPV4 VARCHAR(15) NOT NULL,
            MAC_Address VARCHAR(17) NOT NULL,
            Vendor VARCHAR(255),
            Date_last_seen DATE DEFAULT GETDATE(),
            PRIMARY KEY (MAC_Address, IPV4, Date_last_seen),
            FOREIGN KEY (IPV4) REFERENCES Assets(IPV4)
        );

*/

void db_initialize();

list<string> db_select_asset(string ip4 = "NULL", string subnet = "NULL");

list<string> db_select_all(int network = -1, string subnet = "NULL");

int db_insert_asset(string ip4, string dns = "NULL", string subnet = "0.0.0.0",
        string mac = "NULL", string vend = "NULL");


int db_insert_subnet(string subnet, string desc, int networkid = 1);

int db_insert_network(string name);

int db_delete(string ip4 = "NULL");

void create_csv(string subnet = "0.0.0.0", int network = -1);

template <typename HandleT>
void reportError(int handleTypeEnum, HandleT hdl)
{
    // Get the status records.
    SQLSMALLINT   i, MsgLen;
    SQLRETURN ret2;
    SQLCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER    NativeError;
    i = 1;
    cout << endl;
    while ((ret2 = SQLGetDiagRecA(handleTypeEnum, hdl, i, SqlState, &NativeError,
        Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
        cout << "error record #" << i++ << endl;
        cout << "sqlstate: " << SqlState << endl;
        cout << "detailed msg: " << Msg << endl;
        cout << "native error code: " << NativeError << endl;
    }
}

#endif