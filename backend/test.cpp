//
//
#include <iostream>
#include <String>
#include <list>

#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>

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

	//printlist(findall());

	//createjson(findall());

    // Allocate an environment handle
    SQLHENV hEnv;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    // Allocate a connection handle
    SQLHDBC hConn;
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hConn);
    
    // Connect to the data source using a DSN
    SQLCHAR* dsn = (SQLCHAR*)"naiveconnect";
    SQLCHAR* user = (SQLCHAR*)"naiveadmin";
    SQLCHAR* password = (SQLCHAR*)"naive-1234";
    SQLRETURN retcode = SQLConnectA(hConn, dsn, SQL_NTS, user, SQL_NTS, password, SQL_NTS);

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO){
        std::cout << "Connected to Azure SQL Database." << std::endl;

        // Allocate a statement handle
        SQLHSTMT hStmt;
        SQLAllocHandle(SQL_HANDLE_STMT, hConn, &hStmt);

        // Execute SQL commands
        SQLCHAR* sqlCommand = (SQLCHAR*)"SELECT * FROM asset";
        retcode = SQLExecDirectA(hStmt, sqlCommand, SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            std::cout << "SQL command executed successfully." << std::endl;

            SQLINTEGER age;
            SQLCHAR name[255];
            SQLLEN nameInd, ageInd;
            SQLBindCol(hStmt, 1, SQL_C_LONG, &age, sizeof(age), &ageInd);
            SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &nameInd);

            // Fetch and process data
            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                if (nameInd != SQL_NULL_DATA && ageInd != SQL_NULL_DATA) {
                    std::cout << "Name: " << name << ", Age: " << age << std::endl;
                }
            }

            // Free statement handle
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

            // Disconnect and free handles
            SQLDisconnect(hConn);
            SQLFreeHandle(SQL_HANDLE_DBC, hConn);
            SQLFreeHandle(SQL_HANDLE_ENV, hEnv);

        }
        else {
            // Error handling for SQL command execution
            std::cout << "Error executing SQL command." << std::endl;
        }



        // Disconnect and free handles
        SQLDisconnect(hConn);
        SQLFreeHandle(SQL_HANDLE_DBC, hConn);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
    else {
        // Error handling
        


        std::cout << "Connection failed." << std::endl;
    }

    return 0;
}