


#pragma once
#ifndef backend_H
#define backend_H

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

using namespace std;

list<string> db_select(string mac = "NULL");

int db_insert(string mac, string ip4 = "NULL", string scan = "NULL", 
        string ip6 = "NULL", string vend = "NULL", string op = "NULL", 
        string date = "NULL", string other = "NULL");

int db_delete(string mac = "NULL");

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