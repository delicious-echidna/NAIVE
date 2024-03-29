


#include "backend.h"

using namespace std;

list<string> db_select(string mac) {

    list<string> returned;

    // Set up the ODBC environment
    SQLRETURN ret;
    SQLHENV hdlEnv;
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hdlEnv);
    assert(SQL_SUCCEEDED(ret));
    // Tell ODBC that the application uses ODBC 3.
    ret = SQLSetEnvAttr(hdlEnv, SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_UINTEGER);
    assert(SQL_SUCCEEDED(ret));
    // Allocate a database handle.
    SQLHDBC hdlDbc;
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hdlEnv, &hdlDbc);
    assert(SQL_SUCCEEDED(ret));
    // Connect to the database
    //cout << "Select -- Connecting to database." << endl;
    const char* dsnName = "naiveconnect";
    const char* userID = "naiveadmin";
    const char* passwd = "naive-1234";
    ret = SQLConnectA(hdlDbc, (SQLCHAR*)dsnName,
        SQL_NTS, (SQLCHAR*)userID, SQL_NTS,
        (SQLCHAR*)passwd, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Select -- Could not connect to database" << endl;
        reportError<SQLHDBC>(SQL_HANDLE_DBC, hdlDbc);
        return returned;
    }
    else {
        //cout << "Select -- Connected to database." << endl;
    }


    // Set up a statement handle
    SQLHSTMT hdlStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hdlDbc, &hdlStmt);
    assert(SQL_SUCCEEDED(ret));

    if (mac == "NULL") {
        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)"SELECT * "
            "FROM test", SQL_NTS);
    }
    else {
        string query = "SELECT * FROM test WHERE mac_address = '";
        query += mac + "';";
        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }



    if (!SQL_SUCCEEDED(ret)) {
        // Report error an go no further if statement failed.
        //cout << "Select -- Error executing statement." << endl;
        reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        exit(EXIT_FAILURE);
    }
    else {


        // Query succeeded, so bind two variables to the two colums in the 
        // result set,
        //cout << "Select -- Fetching results..." << endl;
        SQLBIGINT asset_id;
        SQLCHAR mac_address[255];
        SQLCHAR ipv4[255];
        SQLCHAR scan_method[255];
        SQLCHAR ipv6[255];
        SQLCHAR vendor[255];
        SQLCHAR os[255];
        SQLCHAR date_last_seen[255];
        SQLCHAR other_attributes[255];

        SQLLEN id_null_ind = -1;
        SQLLEN mac_null_ind = -1;
        SQLLEN ipv4_null_ind = -1;
        SQLLEN scan_null_ind = -1;
        SQLLEN ipv6_null_ind = -1;
        SQLLEN vend_null_ind = -1;
        SQLLEN os_null_ind = -1;
        SQLLEN date_null_ind = -1;
        SQLLEN other_null_ind = -1;

        ret = SQLBindCol(hdlStmt, 1, SQL_C_SBIGINT, (SQLPOINTER)&asset_id,
            sizeof(asset_id), &id_null_ind);
        ret = SQLBindCol(hdlStmt, 2, SQL_C_CHAR, (SQLPOINTER)mac_address,
            sizeof(mac_address), &mac_null_ind);
        ret = SQLBindCol(hdlStmt, 3, SQL_C_CHAR, (SQLPOINTER)ipv4,
            sizeof(ipv4), &ipv4_null_ind);
        ret = SQLBindCol(hdlStmt, 4, SQL_C_CHAR, (SQLPOINTER)scan_method,
            sizeof(scan_method), &scan_null_ind);
        ret = SQLBindCol(hdlStmt, 5, SQL_C_CHAR, (SQLPOINTER)ipv6,
            sizeof(ipv6), &ipv6_null_ind);
        ret = SQLBindCol(hdlStmt, 6, SQL_C_CHAR, (SQLPOINTER)vendor,
            sizeof(vendor), &vend_null_ind);
        ret = SQLBindCol(hdlStmt, 7, SQL_C_CHAR, (SQLPOINTER)os,
            sizeof(os), &os_null_ind);
        ret = SQLBindCol(hdlStmt, 8, SQL_C_CHAR, (SQLPOINTER)date_last_seen,
            sizeof(date_last_seen), &date_null_ind);
        ret = SQLBindCol(hdlStmt, 9, SQL_C_CHAR, (SQLPOINTER)other_attributes,
            sizeof(other_attributes), &other_null_ind);

        // Loop through the results, 
        while (SQL_SUCCEEDED(ret = SQLFetchScroll(hdlStmt, SQL_FETCH_NEXT, 1))) {
            // Print the bound variables, which now contain the values from the
            // fetched row.

            if (mac_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(mac_address), sizeof mac_address, "NULL");
            }
            if (ipv4_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv4), sizeof ipv4, "NULL");
            }
            if (scan_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(scan_method), sizeof scan_method, "NULL");
            }
            if (ipv6_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv6), sizeof ipv6, "NULL");
            }
            if (vend_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(vendor), sizeof vendor, "NULL");
            }
            if (os_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(os), sizeof os, "NULL");
            }
            if (date_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(date_last_seen), sizeof date_last_seen, "NULL");
            }
            if (other_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(other_attributes), sizeof other_attributes, "NULL");
            }

            //cout << asset_id << " | " << mac_address << " | " << ipv4;
            //cout << " | " << scan_method << " | " << ipv6 << " | " << vendor;
            //cout << " | " << os << " | " << date_last_seen << " | " << other_attributes << endl;

            // Convert integer to string
            std::stringstream asset_id_str;
            asset_id_str << asset_id;
            std::string asset_id_string = asset_id_str.str();

            // Convert char* to string
            std::string mac_address_string = reinterpret_cast<char*>(mac_address);
            std::string ipv4_string = reinterpret_cast<char*>(ipv4);
            std::string scan_method_string = reinterpret_cast<char*>(scan_method);
            std::string ipv6_string = reinterpret_cast<char*>(ipv6);
            std::string vendor_string = reinterpret_cast<char*>(vendor);
            std::string os_string = reinterpret_cast<char*>(os);
            std::string date_last_seen_string = reinterpret_cast<char*>(date_last_seen);
            std::string other_attributes_string = reinterpret_cast<char*>(other_attributes);

            // Concatenate strings
            std::string curr = asset_id_string + "," +
                mac_address_string + "," +
                ipv4_string + "," +
                scan_method_string + "," +
                ipv6_string + "," +
                vendor_string + "," +
                os_string + "," +
                date_last_seen_string + "," +
                other_attributes_string;

            returned.push_back(curr);
        }


        // See if loop exited for reasons other than running out of data
        if (ret != SQL_NO_DATA) {
            // Exited for a reason other than no more data... report the error.
            reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        }
    }


    // Clean up by shutting down the connection
    //cout << "Select -- Free handles." << endl;
    ret = SQLDisconnect(hdlDbc);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Select -- Error disconnecting. Transaction still open?" << endl;
        exit(EXIT_FAILURE);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);

    return returned;
}

int db_insert(string mac, string ip4, string scan,
    string ip6, string vend, string op,
    string date, string other) {

    int update = 0;
    if (db_select(mac).size() > 0) {
        update = 1;
    }

    // Set up the ODBC environment
    SQLRETURN ret;
    SQLHENV hdlEnv;
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hdlEnv);
    assert(SQL_SUCCEEDED(ret));
    // Tell ODBC that the application uses ODBC 3.
    ret = SQLSetEnvAttr(hdlEnv, SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_UINTEGER);
    assert(SQL_SUCCEEDED(ret));
    // Allocate a database handle.
    SQLHDBC hdlDbc;
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hdlEnv, &hdlDbc);
    assert(SQL_SUCCEEDED(ret));
    // Connect to the database
    //cout << "Insert -- Connecting to database." << endl;
    const char* dsnName = "naiveconnect";
    const char* userID = "naiveadmin";
    const char* passwd = "naive-1234";
    ret = SQLConnectA(hdlDbc, (SQLCHAR*)dsnName,
        SQL_NTS, (SQLCHAR*)userID, SQL_NTS,
        (SQLCHAR*)passwd, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Insert -- Could not connect to database" << endl;
        reportError<SQLHDBC>(SQL_HANDLE_DBC, hdlDbc);
        return -1;
    }
    else {
        //cout << "Insert -- Connected to database." << endl;
    }


    // Set up a statement handle
    SQLHSTMT hdlStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hdlDbc, &hdlStmt);
    assert(SQL_SUCCEEDED(ret));


    //update listing if mac address already exists
    if (update == 0) {
        string query = "INSERT INTO test(mac_address, ipv4, scan_method, ipv6, vendor, "
            "os, date_last_seen, other_attributes) VALUES(";
        query += "'" + mac + "','";
        query += ip4 + "','";
        query += scan + "','";
        query += ip6 + "','";
        query += vend + "','";
        query += op + "','";
        query += date + "','";
        query += other + "')";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {
        string query = "UPDATE test SET ";
        if (ip4 != "NULL") {
            query += "ipv4 = '" + ip4 + "',";
        }
        if (scan != "NULL") {
            query += "scan_method = '" + scan + "',";
        }
        if (ip6 != "NULL") {
            query += "ipv6 = '" + ip6 + "',";
        }
        if (vend != "NULL") {
            query += "vendor = '" + vend + "',";
        }
        if (op != "NULL") {
            query += "os = '" + op + "',";
        }
        if (date != "NULL") {
            query += "date_last_seen = '" + date + "',";
        }
        if (other != "NULL") {
            query += "other_attributes = '" + other + "',";
        }
        query.pop_back();
        query += " WHERE mac_address = '" + mac + "';";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }


    if (!SQL_SUCCEEDED(ret)) {
        // Report error an go no further if statement failed.
        //cout << "Insert -- Error executing statement." << endl;
        reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        return -1;
    }
    else {


        // Query succeeded
    }


    // Clean up by shutting down the connection
    //cout << "Insert -- Free handles." << endl;
    ret = SQLDisconnect(hdlDbc);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Insert -- Error disconnecting. Transaction still open?" << endl;
        return -1;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return 0;
}

int db_delete(string mac) {


    // Set up the ODBC environment
    SQLRETURN ret;
    SQLHENV hdlEnv;
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hdlEnv);
    assert(SQL_SUCCEEDED(ret));
    // Tell ODBC that the application uses ODBC 3.
    ret = SQLSetEnvAttr(hdlEnv, SQL_ATTR_ODBC_VERSION,
        (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_UINTEGER);
    assert(SQL_SUCCEEDED(ret));
    // Allocate a database handle.
    SQLHDBC hdlDbc;
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hdlEnv, &hdlDbc);
    assert(SQL_SUCCEEDED(ret));
    // Connect to the database
    //cout << "Delete -- Connecting to database." << endl;
    const char* dsnName = "naiveconnect";
    const char* userID = "naiveadmin";
    const char* passwd = "naive-1234";
    ret = SQLConnectA(hdlDbc, (SQLCHAR*)dsnName,
        SQL_NTS, (SQLCHAR*)userID, SQL_NTS,
        (SQLCHAR*)passwd, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Delete -- Could not connect to database" << endl;
        reportError<SQLHDBC>(SQL_HANDLE_DBC, hdlDbc);
        return -1;
    }
    else {
        //cout << "Delete -- Connected to database." << endl;
    }


    // Set up a statement handle
    SQLHSTMT hdlStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hdlDbc, &hdlStmt);
    assert(SQL_SUCCEEDED(ret));

    string query = "DELETE FROM test WHERE mac_address = '" + mac + "';";

    //cout << query << endl;

    const char* query_cstr = query.c_str();
    ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);


    if (!SQL_SUCCEEDED(ret)) {
        // Report error an go no further if statement failed.
        //cout << "Delete -- Error executing statement." << endl;
        reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        return -1;
    }
    else {


        // Query succeeded
    }


    // Clean up by shutting down the connection
    //cout << "Delete -- Free handles." << endl;
    ret = SQLDisconnect(hdlDbc);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Delete -- Error disconnecting. Transaction still open?" << endl;
        return -1;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return 0;
}
