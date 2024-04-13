


#include "backend.h"

/*

        CREATE TABLE NAIVENetworks (
            NetworkID INT IDENTITY(1,1) PRIMARY KEY,
            NetworkName VARCHAR(255) NOT NULL
    );

        CREATE TABLE NAIVESubnets (
            NetworkID INT,
            SubnetAddress VARCHAR(18) PRIMARY KEY,
            Description VARCHAR(255),
            FOREIGN KEY (NetworkID) REFERENCES NAIVENetworks(NetworkID)
    );

        CREATE TABLE NAIVEAssets (
            SubnetAddress VARCHAR(18),
            IPV4 VARCHAR(15) PRIMARY KEY,
            DNS VARCHAR(50),
            Date_last_seen DATETIME DEFAULT GETDATE(),
            FOREIGN KEY (SubnetAddress) REFERENCES NAIVESubnets(SubnetAddress)
    );

        CREATE TABLE NAIVEMACInfo (
            IPV4 VARCHAR(15) NOT NULL,
            MAC_Address VARCHAR(17) NOT NULL,
            Vendor VARCHAR(50),
            Date_last_seen DATETIME DEFAULT GETDATE(),
            PRIMARY KEY (MAC_Address, IPV4, Date_last_seen),
            FOREIGN KEY (IPV4) REFERENCES NAIVEAssets(IPV4)

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
IF NOT EXISTS (SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'dbo' AND TABLE_NAME = 'NAIVENetworks')
BEGIN
    CREATE TABLE NAIVENetworks (
        NetworkID INT IDENTITY(1,1) PRIMARY KEY,
        NetworkName VARCHAR(255) NOT NULL
    );
END;

IF NOT EXISTS (SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'dbo' AND TABLE_NAME = 'NAIVESubnets')
BEGIN
    CREATE TABLE NAIVESubnets (
        NetworkID INT FOREIGN KEY (NetworkID) REFERENCES NAIVENetworks(NetworkID), 
        SubnetAddress VARCHAR(18) PRIMARY KEY,
        Description VARCHAR(255)
    );
END;

IF NOT EXISTS (SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'dbo' AND TABLE_NAME = 'NAIVEAssets')
BEGIN
    CREATE TABLE NAIVEAssets (
        SubnetAddress VARCHAR(18) FOREIGN KEY (SubnetAddress) REFERENCES NAIVESubnets(SubnetAddress), 
        IPV4 VARCHAR(15) PRIMARY KEY,
        DNS VARCHAR(50),
        Date_last_seen DATETIME DEFAULT GETDATE()
        
    );
END;

IF NOT EXISTS (SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = 'dbo' AND TABLE_NAME = 'NAIVEMACInfo')
BEGIN
    CREATE TABLE NAIVEMACInfo (
        IPV4 VARCHAR(15) FOREIGN KEY (IPV4) REFERENCES NAIVEAssets(IPV4), 
        MAC_Address VARCHAR(17) NOT NULL,
        Vendor VARCHAR(50),
        Date_last_seen DATETIME DEFAULT GETDATE(),
        PRIMARY KEY (MAC_Address, IPV4, Date_last_seen)
    );
END;

IF NOT EXISTS (SELECT 1 FROM NAIVENetworks)
BEGIN
    INSERT INTO NAIVENetworks (NetworkName) VALUES ('Default');
END;

IF NOT EXISTS (SELECT 1 FROM NAIVESubnets)
BEGIN
    INSERT INTO NAIVESubnets (NetworkID, SubnetAddress, Description) VALUES (1, '0.0.0.0', 'Default');
END;
    )";

    ////cout << query << endl;

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
    //cout << "    Disconnected." << endl;
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Initialize -- Error disconnecting. Transaction still open?" << endl;
        return;
    }
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    //cout << "Initialize -- Returning." << endl;
    return;
}

list<string> db_select_asset(string ip4, string subnet) {

    db_initialize();
    //cout << "Select -- Initialized." << endl;

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

    if (ip4 == "NULL" && subnet == "NULL") {
        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
convert(varchar(25), A.Date_last_seen, 120) as Date, 
M.MAC_Address, M.Vendor, M.Date_last_seen
FROM NAIVEAssets A
INNER JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4;)";

        const char* query_cstr = query.c_str();
        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else if(ip4 != "NULL" && subnet == "NULL") {
        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
convert(varchar(25), A.Date_last_seen, 120) as Date, 
M.MAC_Address, M.Vendor, M.Date_last_seen
FROM NAIVEAssets A INNER JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4
WHERE A.IPV4 = ')";
        query += ip4;
        query += "';";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {

        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
convert(varchar(25), A.Date_last_seen, 120) as Date, 
M.MAC_Address, M.Vendor, M.Date_last_seen
FROM NAIVEAssets A INNER JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4
WHERE A.SubnetAddress = ')";
        query += subnet;
        query += "';";

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
        SQLCHAR subnetid[18];
        SQLCHAR ipv4[15];
        SQLCHAR dns[50];
        SQLCHAR date[25];
        SQLCHAR mac[17];
        SQLCHAR vendor[255];

        SQLLEN subnetid_ind = -1;
        SQLLEN ipv4_ind = -1;
        SQLLEN dns_ind = -1;
        SQLLEN date_ind = -1;
        SQLLEN mac_ind = -1;
        SQLLEN vend_ind = -1;

        ret = SQLBindCol(hdlStmt, 1, SQL_C_CHAR, (SQLPOINTER)&subnetid,
            sizeof(subnetid), &subnetid_ind);
        ret = SQLBindCol(hdlStmt, 2, SQL_C_CHAR, (SQLPOINTER)ipv4,
            sizeof(ipv4), &ipv4_ind);
        ret = SQLBindCol(hdlStmt, 3, SQL_C_CHAR, (SQLPOINTER)dns,
            sizeof(dns), &dns_ind);
        ret = SQLBindCol(hdlStmt, 4, SQL_C_CHAR, (SQLPOINTER)date, 
            sizeof(date), &date_ind);
        ret = SQLBindCol(hdlStmt, 5, SQL_C_CHAR, (SQLPOINTER)mac,
            sizeof(mac), &mac_ind);
        ret = SQLBindCol(hdlStmt, 6, SQL_C_CHAR, (SQLPOINTER)vendor,
            sizeof(vendor), &vend_ind);

        // Loop through the results, 
        while (SQL_SUCCEEDED(ret = SQLFetchScroll(hdlStmt, SQL_FETCH_NEXT, 1))) {
            // Print the bound variables, which now contain the values from the
            // fetched row.

            if (subnetid_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(subnetid), sizeof subnetid, "NULL");
            }
            if (ipv4_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv4), sizeof ipv4, "NULL");
            }
            if (dns_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(dns), sizeof dns, "NULL");
            }
            if (date_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(date), sizeof date, "NULL");
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

            // Convert char* to string
            std::string subnet_id_string = reinterpret_cast<char*>(subnetid);
            std::string ipv4_string = reinterpret_cast<char*>(ipv4);
            std::string dns_string = reinterpret_cast<char*>(dns);
            std::string date_string = reinterpret_cast<char*>(date);
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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);

    return returned;
}

list<string> db_select_all(int network, string subnet) {

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

    if (network == -1 && (subnet == "0.0.0.0" || subnet == "NULL")) {
        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
M.MAC_Address, N.NetworkName
FROM NAIVEAssets A
LEFT JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4
LEFT JOIN NAIVESubnets S on A.SubnetAddress = S.SubnetAddress
LEFT JOIN NAIVENetworks N on S.NetworkID = N.NetworkID;)";

        const char* query_cstr = query.c_str();
        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else if (network == -1 && subnet != "NULL" && subnet != "0.0.0.0") {
        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
M.MAC_Address, N.NetworkName
FROM NAIVEAssets A
INNER JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4
LEFT JOIN NAIVESubnets S on A.SubnetAddress = S.SubnetAddress
LEFT JOIN NAIVENetworks N on S.NetworkID = N.NetworkID
WHERE A.SubnetAddress = ')";
        query += subnet;
        query += "';";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {

        string query = R"(SELECT A.SubnetAddress, A.IPV4, A.DNS, 
M.MAC_Address, N.NetworkName
FROM NAIVEAssets A
INNER JOIN NAIVEMACInfo M ON A.IPV4 = M.IPV4
LEFT JOIN NAIVESubnets S on A.SubnetAddress = S.SubnetAddress
LEFT JOIN NAIVENetworks N on S.NetworkID = N.NetworkID
WHERE N.NetworkID = )";
        query += to_string(network);
        query += ";";

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
        // 
        //A.SubnetAddress, A.IPV4, A.DNS
        //M.MAC_Address, N.NetworkName
        //
        SQLCHAR subnetid[18];
        SQLCHAR ipv4[15];
        SQLCHAR dns[50];
        SQLCHAR mac[17];
        SQLCHAR network[255];

        SQLLEN subnetid_ind = -1;
        SQLLEN ipv4_ind = -1;
        SQLLEN dns_ind = -1;
        SQLLEN mac_ind = -1;
        SQLLEN network_ind = -1;

        ret = SQLBindCol(hdlStmt, 1, SQL_C_CHAR, (SQLPOINTER)&subnetid,
            sizeof(subnetid), &subnetid_ind);
        ret = SQLBindCol(hdlStmt, 2, SQL_C_CHAR, (SQLPOINTER)ipv4,
            sizeof(ipv4), &ipv4_ind);
        ret = SQLBindCol(hdlStmt, 3, SQL_C_CHAR, (SQLPOINTER)dns,
            sizeof(dns), &dns_ind);
        ret = SQLBindCol(hdlStmt, 4, SQL_C_CHAR, (SQLPOINTER)mac,
            sizeof(mac), &mac_ind);
        ret = SQLBindCol(hdlStmt, 5, SQL_C_CHAR, (SQLPOINTER)network,
            sizeof(network), &network_ind);

        // Loop through the results, 
        while (SQL_SUCCEEDED(ret = SQLFetchScroll(hdlStmt, SQL_FETCH_NEXT, 1))) {
            // Print the bound variables, which now contain the values from the
            // fetched row.


            if (subnetid_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(subnetid), sizeof subnetid, "NULL");
            }
            if (ipv4_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv4), sizeof ipv4, "NULL");
            }
            if (dns_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(dns), sizeof dns, "NULL");
            }
            if (mac_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(mac), sizeof mac, "NULL");
            }
            if (network_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(network), sizeof network, "NULL");
            }

            //cout << asset_id << " | " << ipv4 << " | " << mac_address;
            //cout << " | " << scan_method << " | " << ipv6 << " | " << vendor;
            //cout << " | " << os << " | " << date_last_seen << " | " << other_attributes << endl;

            // Convert char* to string
            std::string subnet_id_string = reinterpret_cast<char*>(subnetid);
            std::string ipv4_string = reinterpret_cast<char*>(ipv4);
            std::string dns_string = reinterpret_cast<char*>(dns);
            std::string mac_string = reinterpret_cast<char*>(mac);
            std::string network_string = reinterpret_cast<char*>(network);

            // Concatenate strings
            std::string curr = subnet_id_string + "|" +
                ipv4_string + "|" +
                dns_string + "|" +
                mac_string + "|" +
                network_string;

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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);

    return returned;
}

int db_insert_asset(string ip4, string dns, string subnet,
    string mac, string vend) {

    db_initialize();
    //cout << "Insert -- Initialized." << endl;

    int update = 0;
    if (db_select_asset(ip4).size() > 0) {
        update = 1;
        //cout << "Insert -- Update set to 1." << endl;
    }
    else { 
        //cout << "Insert -- Update set to 0." << endl; 
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

        string query = "INSERT INTO NAIVEAssets(SubnetAddress, IPV4, DNS)";
        query += "VALUES('";

        query += subnet;
            
        query += "', ";
        query += "'" + ip4 + "', ";
        query += "'" + dns + "');\n\n";

        query += "INSERT INTO NAIVEMACInfo(IPV4, MAC_Address, Vendor)";

        query += "VALUES(";
        query += "'" + ip4 + "', ";
        query += "'" + mac + "', ";
        query += "'" + vend + "');\n\n";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
        //cout << "    Returned " << to_string(ret) << endl;
    }
    else {

        string query = "UPDATE NAIVEAssets SET ";

        if (subnet != "NULL") {
            query += "SubnetAddress = '" + subnet + "',";
        }
        if (dns != "NULL") {
            query += "DNS = '" + dns + "',";
        }
        query.pop_back();
        query += " WHERE ipv4 = '" + ip4 + "';";
        query += "\n\nUPDATE NAIVEMACInfo SET ";

        if (mac != "NULL") {
            query += "MAC_Address = '" + mac + "',";
        }
        if (vend != "NULL") {
            query += "Vendor = '" + vend + "',";
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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return 0;
}

int db_insert_subnet(string subnet, string desc, int networkid) {

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

        string query = "INSERT INTO NAIVESubnets(NetworkID, SubnetAddress, Description) ";
        query += "VALUES(";
        query += networkid + ", ";
        query += "'" + subnet + "', ";
        query += "'" + desc + "');";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
     /*
    }
    else {

        string query = "UPDATE NAIVESubnets SET ";

        query += "NetworkID = ";
        query += to_string(networkid);
        query += ", ";

        if (desc != "NULL") {
            query += "Description = '" + desc + "',";
        }
        query.pop_back();
        query += " WHERE SubnetAddress = '" + subnet + "';";

        const char* query_cstr = query.c_str();

        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    */

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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return 0;
}

int db_insert_network(string name) {

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



    string query = "INSERT INTO NAIVENetworks(NetworkName) ";
    query += "VALUES(";
    query += "'" + name + "');";

    const char* query_cstr = query.c_str();

    ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    


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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
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
        query = "DELETE FROM NAIVEMACInfo WHERE IPV4 = '" + ip4 + "';\n";
        query += "DELETE FROM NAIVEAssets WHERE IPV4 = '" + ip4 + "';\n";
    }
    else {
        query = "DELETE FROM NAIVEMACInfo;\nDELETE FROM NAIVEAssets;";
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
    //SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return 0;
}

void create_csv(string subnet, int network) {

    list<string> assets = db_select_all(network, subnet);
    // 
    //A.SubnetAddress, A.IPV4, A.DNS
    //M.MAC_Address, N.NetworkName
    //

    if (assets.empty()) {
        cerr << "Error: Unable to create CSV with empty asset list" << endl;
        return;
    }

    int counter = 0;

    list<string> listings;
    string header = "\"SubnetAddress\",\"NetworkName\",\"IPV4\",\"MAC_Address\",\"FQDN\"\n";

    for (list<string>::iterator it = assets.begin(); it != assets.end(); ++it) {

        //populate 'listings' with string representation of each line

        stringstream currline(*it);
        string atts[9];

        int j = 0;
        while (currline.good()) {
            string substr;
            getline(currline, substr, '|');
            atts[j] = substr;
            //cout << substr << ",";
            j++;
        }
        //cout << endl;
        string curr = "";

        curr += "\"" + atts[0] + "\",";
        curr += "\"" + atts[3] + "\",";
        curr += "\"" + atts[1] + "\",";
        curr += "\"" + atts[3] + "\",";
        curr += "\"" + atts[2] + "\"";

        listings.push_back(curr);

        counter++;
    }

    string filename = "../AssetFile.csv";
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return;
    }

    ofs << header;

    int j = 0;
    for (list<string>::iterator it2 = listings.begin(); it2 != listings.end(); ++it2) {
        j++;
        ofs << *it2;
        if (j != counter) {
            ofs << "\n";
        }
    }

    ofs.close();

    //cout << counter << "assets logged." << endl;

}