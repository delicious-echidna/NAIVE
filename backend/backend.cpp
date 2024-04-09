


#include "backend.h"

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

using namespace std;

void db_initialize() {

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
    //cout << "Initialize -- Connecting to database." << endl;
    const char* connectionString = "Driver={SQL Server};Server=localhost\\SQLEXPRESS;Database=master;Trusted_Connection=yes;";
    ret = SQLDriverConnectA(hdlDbc, NULL, (SQLCHAR*)connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Initialize -- Could not connect to database" << endl;
        reportError<SQLHDBC>(SQL_HANDLE_DBC, hdlDbc);
        return;
    }
    else {
        //cout << "Initialize -- Connected to database." << endl;
    }


    // Set up a statement handle
    SQLHSTMT hdlStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hdlDbc, &hdlStmt);
    assert(SQL_SUCCEEDED(ret));

    string query = R"(
        IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'Networks') AND type in (N'U'))
BEGIN
    CREATE TABLE Networks (
        NetworkID INT IDENTITY(1,1) PRIMARY KEY,
        NetworkName VARCHAR(255) NOT NULL
    );
END;

IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'Subnets') AND type in (N'U'))
BEGIN
    CREATE TABLE Subnets (
        SubnetID INT IDENTITY(1,1) PRIMARY KEY,
        NetworkID INT,
        SubnetAddress VARCHAR(18) NOT NULL,
        Description VARCHAR(255),
        FOREIGN KEY (NetworkID) REFERENCES Networks(NetworkID)
    );
END;

IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'Assets') AND type in (N'U'))
BEGIN
    CREATE TABLE Assets (
        SubnetID INT,
        IPV4 VARCHAR(15) PRIMARY KEY,
        DNS VARCHAR(50),
        Date_last_seen VARCHAR(50),
        FOREIGN KEY (SubnetID) REFERENCES Subnets(SubnetID)
    );
END;

IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'MACInfo') AND type in (N'U'))
BEGIN
    CREATE TABLE MACInfo (
        IPV4 VARCHAR(15) NOT NULL,
        MAC_Address VARCHAR(17) NOT NULL,
        Vendor VARCHAR(50),
        Date_last_seen VARCHAR(50),
        PRIMARY KEY (MAC_Address, IPV4, Date_last_seen),
        FOREIGN KEY (IPV4) REFERENCES Assets(IPV4) 
    );
END;

IF NOT EXISTS (SELECT 1 FROM Networks)
BEGIN
    INSERT INTO Networks (NetworkName) VALUES ('Default');
END;

IF NOT EXISTS (SELECT 1 FROM Subnets)
BEGIN
    INSERT INTO Subnets (NetworkID, SubnetAddress, Description) VALUES (1, '0.0.0.0', 'Default');
END;
    )";

    //cout << query << endl;

    const char* query_cstr = query.c_str();
    ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);


    if (!SQL_SUCCEEDED(ret)) {
        // Report error an go no further if statement failed.
        //cout << "Initialize -- Error executing statement." << endl;
        reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        return;
    }
    else {


        // Query succeeded
    }


    // Clean up by shutting down the connection
    //cout << "Initialize -- Free handles." << endl;
    ret = SQLDisconnect(hdlDbc);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Initialize -- Error disconnecting. Transaction still open?" << endl;
        return;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return;
}

list<string> db_select(string ip4) {

    db_initialize();

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
    const char* connectionString = "Driver={SQL Server};Server=localhost\\SQLEXPRESS;Database=master;Trusted_Connection=yes;";
    ret = SQLDriverConnectA(hdlDbc, NULL, (SQLCHAR*)connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

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

    if (ip4 == "NULL") {
        string query = R"(SELECT SubnetID, IPV4, DNS, Date_last_seen
    FROM Assets

    UNION

    SELECT NULL AS SubnetID, IPV4, NULL AS DNS, Date_last_seen
    FROM MACInfo;)";

        const char* query_cstr = query.c_str();
        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {
        string query = R"(SELECT SubnetID, IPV4, DNS, Date_last_seen, NULL AS MAC_Address, NULL AS Vendor
    FROM Assets
    WHERE IPV4 = ')";
        query += ip4;
        query += R"('

    UNION

    SELECT NULL AS SubnetID, IPV4, NULL AS DNS, Date_last_seen, MAC_Address, Vendor
    FROM MACInfo
    WHERE IPV4 = ')";
        query += ip4;
        query += "';)";

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
        SQLBIGINT subnetid;
        SQLCHAR ipv4[15];
        SQLCHAR dns[50];
        SQLCHAR date_last_seen[50];
        SQLCHAR mac[17];
        SQLCHAR vendor[255];

        SQLLEN subnetid_ind = -1;
        SQLLEN ipv4_ind = -1;
        SQLLEN dns_ind = -1;
        SQLLEN date_ind = -1;
        SQLLEN mac_ind = -1;
        SQLLEN vend_ind = -1;

        ret = SQLBindCol(hdlStmt, 1, SQL_C_SBIGINT, (SQLPOINTER)&subnetid,
            sizeof(subnetid), &subnetid_ind);
        ret = SQLBindCol(hdlStmt, 2, SQL_C_CHAR, (SQLPOINTER)ipv4,
            sizeof(ipv4), &ipv4_ind);
        ret = SQLBindCol(hdlStmt, 3, SQL_C_CHAR, (SQLPOINTER)dns,
            sizeof(dns), &dns_ind);
        ret = SQLBindCol(hdlStmt, 4, SQL_C_CHAR, (SQLPOINTER)date_last_seen,
            sizeof(date_last_seen), &date_ind);
        ret = SQLBindCol(hdlStmt, 5, SQL_C_CHAR, (SQLPOINTER)mac,
            sizeof(mac), &mac_ind);
        ret = SQLBindCol(hdlStmt, 6, SQL_C_CHAR, (SQLPOINTER)vendor,
            sizeof(vendor), &vend_ind);

        // Loop through the results, 
        while (SQL_SUCCEEDED(ret = SQLFetchScroll(hdlStmt, SQL_FETCH_NEXT, 1))) {
            // Print the bound variables, which now contain the values from the
            // fetched row.

            if (ipv4_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv4), sizeof ipv4, "NULL");
            }
            if (dns_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(dns), sizeof dns, "NULL");
            }
            if (date_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(date_last_seen), sizeof date_last_seen, "NULL");
            }
            if (mac_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(mac), sizeof mac, "NULL");
            }
            if (vend_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(vendor), sizeof vendor, "NULL");
            }

            //cout << asset_id << " | " << ipv4 << " | " << mac_address;
            //cout << " | " << scan_method << " | " << ipv6 << " | " << vendor;
            //cout << " | " << os << " | " << date_last_seen << " | " << other_attributes << endl;

            // Convert integer to string
            std::stringstream subnet_str;
            subnet_str << subnetid;
            std::string subnet_id_string = subnet_str.str();

            // Convert char* to string
            std::string ipv4_string = reinterpret_cast<char*>(ipv4);
            std::string dns_string = reinterpret_cast<char*>(dns);
            std::string date_string = reinterpret_cast<char*>(date_last_seen);
            std::string mac_string = reinterpret_cast<char*>(mac);
            std::string vendor_string = reinterpret_cast<char*>(vendor);
            
            // Concatenate strings
            std::string curr = subnet_id_string + "|" +
                ipv4_string + "|" +
                dns_string + "|" +
                date_string + "|" +
                mac_string + "|" +
                vendor_string;

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

int db_insert(string ip4, string dns, int subnet,
    string date, string mac, string vend) {

    db_initialize();

    int update = 0;
    if (db_select(ip4).size() > 0) {
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
    const char* connectionString = "Driver={SQL Server};Server=localhost\\SQLEXPRESS;Database=master;Trusted_Connection=yes;";
    ret = SQLDriverConnectA(hdlDbc, NULL, (SQLCHAR*)connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

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

        string query = "INSERT INTO Assets(SubnetID, IPV4, DNS, Date_last_seen) ";
        query += "VALUES(";
        query += to_string(subnet) + ", ";
        query += "'" + ip4 + "', ";
        query += "'" + dns + "', ";
        query += "'" + date + "';\n\n";

        query += "INSERT INTO MACInfo(IPV4, MAC_Address, Vendor, Date_last_seen) ";
        query += "VALUES(";
        query += "'" + ip4 + "', ";
        query += "'" + mac + "', ";
        query += "'" + vend + "', ";
        query += "'" + date + "';";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {

        string query = "UPDATE Assets SET ";

        if (subnet != 1) {
            query += "SubnetID = " + to_string(subnet) + ",";
        }
        if (dns != "NULL") {
            query += "DNS = '" + dns + "',";
        }
        if (date != "NULL") {
            query += "Date_last_seen = '" + date + "',";
        }
        query.pop_back();
        query += " WHERE ipv4 = '" + ip4 + "';";
        query += "\n\nUPDATE MACInfo SET ";

        if (mac != "NULL") {
            query += "MAC_Address = '" + mac + "',";
        }
        if (vend != "NULL") {
            query += "Vendor = '" + vend + "',";
        }
        if (date != "NULL") {
            query += "Date_last_seen = '" + date + "',";
        }

        query.pop_back();
        query += " WHERE ipv4 = '" + ip4 + "';";

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

int db_delete(string ip4) {

    db_initialize();

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
    const char* connectionString = "Driver={SQL Server};Server=localhost\\SQLEXPRESS;Database=master;Trusted_Connection=yes;";
    ret = SQLDriverConnectA(hdlDbc, NULL, (SQLCHAR*)connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

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

    string query = "";

    if (ip4 != "NULL") {
        query = "DELETE FROM MACInfo WHERE IPV4 = '" + ip4 + "';\n";
        query += "DELETE FROM Assets WHERE IPV4 = '" + ip4 + "';\n";
    }
    else {
        query = "DELETE FROM MACInfo;\nDELETE FROM Assets;";
    }


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
