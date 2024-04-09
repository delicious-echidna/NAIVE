


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

string db_initialize() {

    //check if already initialized
    string filename = "login.txt";
        // Check if the file exists
    if (filesystem::exists(filename)) {
        ifstream file("login.txt");

        if (!file.is_open()) {
            //std::cerr << "Unable to open login.txt for reading." << std::endl;
        } else {
            string user;
            getline(file, user);
            file.close();
            return user;
        }
    }

    //generate username and password
    const string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    string user;
    user.reserve(20);
    string password;
    password.reserve(20);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < 20; ++i) {
        user += charset[std::rand() % charset.size()];
        password += charset[std::rand() % charset.size()];
    }

    //save login information
    string newfile = "login.txt";
    ofstream ofs(newfile);
    if (!ofs.is_open()) {
        //cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return user;
    }
    ofs << user;
    ofs << "\n";
    ofs << password;
    ofs.close();


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
        return user;
    }
    else {
        //cout << "Initialize -- Connected to database." << endl;
    }


    // Set up a statement handle
    SQLHSTMT hdlStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hdlDbc, &hdlStmt);
    assert(SQL_SUCCEEDED(ret));

    string query = R"(
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
    )";

    //cout << query << endl;

    const char* query_cstr = query.c_str();
    ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);


    if (!SQL_SUCCEEDED(ret)) {
        // Report error an go no further if statement failed.
        //cout << "Initialize -- Error executing statement." << endl;
        reportError<SQLHDBC>(SQL_HANDLE_STMT, hdlStmt);
        return user;
    }
    else {


        // Query succeeded
    }


    // Clean up by shutting down the connection
    //cout << "Initialize -- Free handles." << endl;
    ret = SQLDisconnect(hdlDbc);
    if (!SQL_SUCCEEDED(ret)) {
        //cout << "Initialize -- Error disconnecting. Transaction still open?" << endl;
        return user;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hdlStmt);
    SQLFreeHandle(SQL_HANDLE_DBC, hdlDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hdlEnv);
    return user;
}

list<string> db_select(string ip4) {

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
        string query = "SELECT * FROM " + db_initialize();
        const char* query_cstr = query.c_str();
        ret = SQLExecDirectA(hdlStmt, (SQLCHAR*)query_cstr, SQL_NTS);
    }
    else {
        string query = "SELECT * FROM " + db_initialize() + " WHERE ipv4 = '";
        query += ip4 + "';";
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
        SQLCHAR ipv4[255];
        SQLCHAR mac_address[255];
        SQLCHAR scan_method[255];
        SQLCHAR ipv6[255];
        SQLCHAR vendor[255];
        SQLCHAR os[255];
        SQLCHAR date_last_seen[255];
        SQLCHAR other_attributes[255];

        SQLLEN id_null_ind = -1;
        SQLLEN ipv4_null_ind = -1;
        SQLLEN mac_null_ind = -1;
        SQLLEN scan_null_ind = -1;
        SQLLEN ipv6_null_ind = -1;
        SQLLEN vend_null_ind = -1;
        SQLLEN os_null_ind = -1;
        SQLLEN date_null_ind = -1;
        SQLLEN other_null_ind = -1;

        ret = SQLBindCol(hdlStmt, 1, SQL_C_SBIGINT, (SQLPOINTER)&asset_id,
            sizeof(asset_id), &id_null_ind);
        ret = SQLBindCol(hdlStmt, 2, SQL_C_CHAR, (SQLPOINTER)ipv4,
            sizeof(ipv4), &ipv4_null_ind);
        ret = SQLBindCol(hdlStmt, 3, SQL_C_CHAR, (SQLPOINTER)mac_address,
            sizeof(ipv4), &mac_null_ind);
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

            if (ipv4_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(ipv4), sizeof ipv4, "NULL");
            }
            if (mac_null_ind < 0) {
                strcpy_s(reinterpret_cast<char*>(mac_address), sizeof mac_address, "NULL");
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

            //cout << asset_id << " | " << ipv4 << " | " << mac_address;
            //cout << " | " << scan_method << " | " << ipv6 << " | " << vendor;
            //cout << " | " << os << " | " << date_last_seen << " | " << other_attributes << endl;

            // Convert integer to string
            std::stringstream asset_id_str;
            asset_id_str << asset_id;
            std::string asset_id_string = asset_id_str.str();

            // Convert char* to string
            std::string ipv4_string = reinterpret_cast<char*>(ipv4);
            std::string mac_address_string = reinterpret_cast<char*>(mac_address);
            std::string scan_method_string = reinterpret_cast<char*>(scan_method);
            std::string ipv6_string = reinterpret_cast<char*>(ipv6);
            std::string vendor_string = reinterpret_cast<char*>(vendor);
            std::string os_string = reinterpret_cast<char*>(os);
            std::string date_last_seen_string = reinterpret_cast<char*>(date_last_seen);
            std::string other_attributes_string = reinterpret_cast<char*>(other_attributes);

            // Concatenate strings
            std::string curr = asset_id_string + "," +
                ipv4_string + "," +
                mac_address_string + "," +
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

int db_insert(string ip4, string mac, string scan,
    string ip6, string vend, string op,
    string date, string other) {

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
        string query = "INSERT INTO " + db_initialize() + "(ipv4, mac_address, scan_method, ipv6, vendor, "
            "os, date_last_seen, other_attributes) VALUES(";
        query += "'" + ip4 + "','";
        query += mac + "','";
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
        string query = "UPDATE " + db_initialize() + " SET ";
        if (mac != "NULL") {
            query += "mac_address = '" + mac + "',";
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
        query = "DELETE FROM " + db_initialize() + " WHERE ipv4 = '" + ip4 + "';";
    }
    else {
        query = "DELETE FROM " + db_initialize() + ";";
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
