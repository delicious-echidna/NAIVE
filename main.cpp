#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <mariadb/conncpp.hpp>
#include "arp-mod.h"
using namespace std;


int listNetworks(std::unique_ptr<sql::Connection>& con) {
    std::vector<int> networkIDs;  // To store the network IDs
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT NetworkID, NetworkName FROM Networks"));
        
        int index = 1;
        std::cout << "Available Networks:" << std::endl;
        while (res->next()) {
            std::cout << index << ": " << res->getString("NetworkName") << " (ID: " << res->getInt("NetworkID") << ")" << std::endl;
            networkIDs.push_back(res->getInt("NetworkID"));
            index++;
        }
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what() << std::endl;
        return -1;  // Return an error code
    }

    // Get user choice based on index
    int choiceIndex;
    std::cout << "Select a network by number: ";
    std::cin >> choiceIndex;

    // Check if the user choice is valid
    if (choiceIndex < 1 || choiceIndex > static_cast<int>(networkIDs.size())) {
        std::cerr << "Invalid selection. Please try again." << std::endl;
        return -1;  // Return an error code
    }


    // Return the corresponding network ID
    return networkIDs[choiceIndex - 1];
}


std::string listSubnets(std::unique_ptr<sql::Connection>& con, const int networkID) {
    std::vector<std::string> subnetAddresses;
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT SubnetAddress, Description FROM Subnets WHERE NetworkID = ?"
        ));
        pstmt->setInt(1, networkID);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        int index = 1;
        std::cout << "Available Subnets for Network ID " << networkID << ":" << std::endl;
        while (res->next()) {
            std::string subnetAddress = std::string(res->getString("SubnetAddress").c_str());
            std::string description = std::string(res->getString("Description").c_str());
            std::cout << index << ": " << subnetAddress << " - " << description << std::endl;
            subnetAddresses.push_back(subnetAddress);
            index++;
        }

        if (subnetAddresses.empty()) {
            std::cout << "No subnets available for this network." << std::endl;
            return "";  // Return empty if no subnets available
        }

        int choiceIndex;
        std::cout << "Select a subnet by number: ";
        std::cin >> choiceIndex;

        if (choiceIndex < 1 || choiceIndex > static_cast<int>(subnetAddresses.size())) {
            std::cerr << "Invalid selection. Please try again." << std::endl;
            return "";  // Return empty on invalid selection
        }

        return subnetAddresses[choiceIndex - 1];

    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what() << " Error Code: " << e.getErrorCode() << " SQLState: " << e.getSQLState() << std::endl;
        return "";  // Return empty on exception
    }
}

void createTenableAssetFile(const std::string& subnetAddress, int networkId) {
    try {
        // Initialize driver and create a connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        conn->setSchema("NAIVE");

        // Prepare SQL query
        std::ostringstream query;
        query << "SELECT a.IPV4, m.MAC_Address, a.DNS FROM Assets a "
              << "JOIN Subnets s ON a.SubnetAddress = s.SubnetAddress "
              << "JOIN MACInfo m ON a.IPV4 = m.IPV4 "
              << "WHERE s.SubnetAddress = '" << subnetAddress << "' AND s.NetworkID = " << networkId;

        // Execute query
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query.str()));

        // Initialize JSON object
        nlohmann::json jsonObj;
        jsonObj["assets"] = nlohmann::json::array();  // Start with an empty array
        jsonObj["source"] = "local_scan";

        // Process results
        while (res->next()) {
            nlohmann::json asset;
            asset["ipv4"] = {res->getString("IPV4")};
            asset["mac_address"] = {res->getString("MAC_Address")};
            asset["fqdn"] = {res->getString("DNS")};
            jsonObj["assets"].push_back(asset);  // Add asset to the array
        }

        // Write to file
        std::ofstream outFile("TenableAssetFile.json");
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file: " << "TenableAssetFile.json" << std::endl;
            return;
        }

        // Serialize JSON with indentation
        outFile << jsonObj.dump(4);  // '4' is the number of spaces for indentation
        outFile.close();
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

void createTodayTenableAssetFile(const std::string& subnetAddress, int networkId) {
    try {
        // Initialize driver and create a connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        conn->setSchema("NAIVE");

        // Get the current date in YYYY-MM-DD format
        time_t now = time(0);
        tm *ltm = localtime(&now);
        std::ostringstream currentDate;
        currentDate << (1900 + ltm->tm_year) << "-" 
                    << std::setfill('0') << std::setw(2) << (1 + ltm->tm_mon) << "-" 
                    << std::setfill('0') << std::setw(2) << ltm->tm_mday;

        // Prepare SQL query
        std::ostringstream query;
        query << "SELECT a.IPV4, m.MAC_Address, a.DNS FROM Assets a "
              << "JOIN Subnets s ON a.SubnetAddress = s.SubnetAddress "
              << "JOIN MACInfo m ON a.IPV4 = m.IPV4 "
              << "WHERE s.SubnetAddress = '" << subnetAddress << "' AND s.NetworkID = " << networkId
              << " AND DATE(m.Date_last_seen) = '" << currentDate.str() << "'";  // Filter assets by today's date

        // Execute query
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query.str()));

        // Initialize JSON object
        nlohmann::json jsonObj;
        jsonObj["assets"] = nlohmann::json::array();  // Start with an empty array
        jsonObj["source"] = "local_scan";

        // Process results
        while (res->next()) {
            nlohmann::json asset;
            asset["ipv4"] = {res->getString("IPV4")};
            asset["mac_address"] = {res->getString("MAC_Address")};
            asset["fqdn"] = {res->getString("DNS")};
            jsonObj["assets"].push_back(asset);  // Add asset to the array
        }

        // Write to file
        std::ofstream outFile("TenableAssetFile.json");
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file: " << "TenableAssetFile.json" << std::endl;
            return;
        }

        // Serialize JSON with indentation
        outFile << jsonObj.dump(4);  // '4' is the number of spaces for indentation
        outFile.close();
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

void createAssetCSV(const std::string& subnetAddress, int networkId) {
    try {
        // Initialize driver and create a connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        conn->setSchema("NAIVE");

        // Fetch network name using the networkId
        std::unique_ptr<sql::Statement> stmtNet(conn->createStatement());
        std::unique_ptr<sql::ResultSet> resNet(stmtNet->executeQuery("SELECT NetworkName FROM Networks WHERE NetworkID = " + std::to_string(networkId)));
        std::string networkName;
        if (resNet->next()) {
            networkName = resNet->getString("NetworkName");
        } else {
            std::cerr << "No network found for NetworkID: " << networkId << std::endl;
            return;
        }

        // Prepare SQL query to fetch assets
        std::ostringstream query;
        query << "SELECT a.IPV4, m.MAC_Address, a.DNS FROM Assets a "
              << "JOIN MACInfo m ON a.IPV4 = m.IPV4 "
              << "WHERE a.SubnetAddress = '" << subnetAddress << "'";

        // Execute query
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query.str()));

        // Open the CSV file
        std::ofstream outFile("AssetFile.csv");
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file: " << "AssetFile.csv" << std::endl;
            return;
        }

        // Write CSV headers
        outFile << "\"SubnetAddress\",\"NetworkName\",\"IPV4\",\"MAC_Address\",\"FQDN\"\n";

        // Process results and write to CSV
        while (res->next()) {
            outFile << "\"" << subnetAddress << "\","
                    << "\"" << networkName << "\","
                    << "\"" << res->getString("IPV4") << "\","
                    << "\"" << res->getString("MAC_Address") << "\","
                    << "\"" << res->getString("DNS") << "\"\n";
        }

        outFile.close();
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

void displayAssetInfo(const std::string& subnetAddress, int networkId) {
    try {
        // Initialize driver and create a connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        conn->setSchema("NAIVE");

        // Fetch network name using the networkId
        std::unique_ptr<sql::Statement> stmtNet(conn->createStatement());
        std::unique_ptr<sql::ResultSet> resNet(stmtNet->executeQuery("SELECT NetworkName FROM Networks WHERE NetworkID = " + std::to_string(networkId)));
        std::string networkName;
        if (resNet->next()) {
            networkName = resNet->getString("NetworkName");
        } else {
            std::cerr << "No network found for NetworkID: " << networkId << std::endl;
            return;
        }

        // Prepare SQL query to fetch assets
        std::ostringstream query;
        query << "SELECT a.IPV4, m.MAC_Address, m.Vendor, a.DNS, m.Date_last_seen FROM Assets a "
              << "JOIN MACInfo m ON a.IPV4 = m.IPV4 "
              << "WHERE a.SubnetAddress = '" << subnetAddress << "'";

        // Execute query
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query.str()));

        // Print header
        std::cout << std::left << std::setw(15) << "IP Address"
                  << std::setw(20) << "MAC Address"
                  << std::setw(25) << "Vendor"
                  << std::setw(25) << "DNS"
                  << std::setw(30) << "Time"
                  << std::endl;
        std::cout << std::string(115, '-') << std::endl;

        // Process results and print to console
        while (res->next()) {
            std::cout << std::setw(15) << res->getString("IPV4")
                      << std::setw(20) << res->getString("MAC_Address")
                      << std::setw(25) << res->getString("Vendor")
                      << std::setw(25) << res->getString("DNS")
                      << std::setw(30) << res->getString("Date_last_seen")
                      << std::endl;
        }
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

void displayTodayAssetInfo(const std::string& subnetAddress, int networkId) {
    try {
        // Initialize driver and create a connection
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        conn->setSchema("NAIVE");

        // Get the current date in format YYYY-MM-DD
        time_t now = time(0);
        tm *ltm = localtime(&now);
        std::ostringstream currentDate;
        currentDate << (1900 + ltm->tm_year) << "-" 
                    << std::setfill('0') << std::setw(2) << (1 + ltm->tm_mon) << "-" 
                    << std::setfill('0') << std::setw(2) << ltm->tm_mday;

        // Prepare SQL query to fetch today's assets
        std::ostringstream query;
        query << "SELECT a.IPV4, m.MAC_Address, m.Vendor, a.DNS, m.Date_last_seen FROM Assets a "
              << "JOIN MACInfo m ON a.IPV4 = m.IPV4 "
              << "WHERE a.SubnetAddress = '" << subnetAddress << "' "
              << "AND DATE(m.Date_last_seen) = '" << currentDate.str() << "'";

        // Execute query
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(query.str()));

        // Print header
        std::cout << std::left << std::setw(15) << "IP Address"
                  << std::setw(20) << "MAC Address"
                  << std::setw(25) << "Vendor"
                  << std::setw(25) << "DNS"
                  << std::setw(30) << "Time"
                  << std::endl;
        std::cout << std::string(115, '-') << std::endl;

        // Process results and print to console
        while (res->next()) {
            std::cout << std::setw(15) << res->getString("IPV4")
                      << std::setw(20) << res->getString("MAC_Address")
                      << std::setw(25) << res->getString("Vendor")
                      << std::setw(25) << res->getString("DNS")
                      << std::setw(30) << res->getString("Date_last_seen")
                      << std::endl;
        }
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

//there could potentially be flags we introduce but for now...this is the simple stuff
int main(){
    bool exitNow = false;
    int userInput; //we need to validate this to prevent overflow issues...
    cout <<
"               __  _\n" <<
"       .-.'  `; `-._  __  _\n" <<
"      (_,         .-:'  `; `-._\n" <<
"    ,'o\"(        (_,           )\n" <<
"   (__,-'      ,'o\"(            )>\n" <<
"      (       (__,-'            )\n" <<
"       `-'._.--._(             )\n"<<
"          |||  |||`-'._.--._.-'\n"<<
"                     |||  |||\n";
    cout <<
"         _  _   _   _  _ _  ___\n" <<
"        | \\| | / \\ | || | || __|\n"<<
"        | \\\\ || o || || V || _|\n" <<
"        |_|\\_||_n_||_| \\_/ |___|\n"<< endl;
                        
    cout<< "~~~~Welcome to NAIVE (Network Analysis and InVentory Exporter)~~~~" << endl;
    cout<< "~~~~Let's get started...~~~~" << endl;
    std::string network_name, subnet_name;
    cout << "~~~~Please input the name of your network~~~~" << endl; //might need to introduce a list for existing entries?
    getline(cin, network_name);
    cout << "~~~~Please input the name of the subnet being scanned~~~~" << endl; //might need to introduce a list for existing entries?
    getline(cin, subnet_name);
    cout << "~~~~Please Select the interface to be scanned~~~~" << endl;
    std::string interface_name_str = choose_network_interface();
    if (interface_name_str.empty()) {
        std::cerr << "No valid interface selected, aborting scan." << std::endl;
        return 0;;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore characters up to newline
    const char* interface_name = interface_name_str.c_str();
    program(network_name, subnet_name, interface_name);
    while(!exitNow){
        cout << "\n Please enter an option:" << endl;
        cout << "\n 1. Create Tenable Asset File with Today's Assets (.JSON)" << endl;
        cout << "\n 2. Create Human Readable Asset File with all Assets (.csv)" << endl;
        cout << "\n 3. Print Today's Results to the Screen" << endl;
        cout << "\n 4. Exit" << endl;
         // Improved input validation
        while(!(cin >> userInput) || userInput < 1 || userInput > 4){
            cout << "~~~~Invalid input, please input a number between 1 and 4~~~~" << endl;
            cin.clear(); // Clear the fail state of the cin object.
            // Ignore the maximum number of characters until the next newline character is found.
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        if(userInput == 1){
            //need to call a create file function (for tenable)
            try {
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                std::unique_ptr<sql::Connection> con(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
                con->setSchema("NAIVE");

                    // List networks and get user choice for the network
                int chosenNetworkID = listNetworks(con);
                if (chosenNetworkID == -1) {
                    std::cerr << "Error: Invalid network selection." << std::endl;
                    return 1;  // Exit with error code
                }

                // List subnets and get user choice for the subnet
                std::string chosenSubnetAddress= listSubnets(con, chosenNetworkID);
                if (chosenSubnetAddress.empty()) {
                    std::cerr << "Error: Invalid subnet selection." << std::endl;
                    return 1;  // Exit with error code
                }

                std::cout << "Chosen Network ID: " << chosenNetworkID << ", Chosen Subnet ID: " << chosenSubnetAddress << std::endl;

                // Example continuation: Perform actions based on chosenNetworkID
                createTodayTenableAssetFile(chosenSubnetAddress, chosenNetworkID);

            } catch (sql::SQLException &e) {
                std::cerr << "Error connecting to the database: " << e.what() << std::endl;
                return 1;  // Exit with error code
            }
        }
        else if(userInput == 2){
            //need to call a create file function (for humans)
            try {
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                std::unique_ptr<sql::Connection> con(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
                con->setSchema("NAIVE");

                    // List networks and get user choice for the network
                int chosenNetworkID = listNetworks(con);
                if (chosenNetworkID == -1) {
                    std::cerr << "Error: Invalid network selection." << std::endl;
                    return 1;  // Exit with error code
                }

                // List subnets and get user choice for the subnet
                std::string chosenSubnetAddress= listSubnets(con, chosenNetworkID);
                if (chosenSubnetAddress.empty()) {
                    std::cerr << "Error: Invalid subnet selection." << std::endl;
                    return 1;  // Exit with error code
                }

                std::cout << "Chosen Network ID: " << chosenNetworkID << ", Chosen Subnet ID: " << chosenSubnetAddress << std::endl;

                // Example continuation: Perform actions based on chosenNetworkID
                createAssetCSV(chosenSubnetAddress, chosenNetworkID);

            } catch (sql::SQLException &e) {
                std::cerr << "Error connecting to the database: " << e.what() << std::endl;
                return 1;  // Exit with error code
            }
        }
        else if(userInput == 3){
            //print results to screen from database
            //need to call a create file function (for humans)
            try {
                sql::Driver* driver = sql::mariadb::get_driver_instance();
                std::unique_ptr<sql::Connection> con(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
                con->setSchema("NAIVE");

                    // List networks and get user choice for the network
                int chosenNetworkID = listNetworks(con);
                if (chosenNetworkID == -1) {
                    std::cerr << "Error: Invalid network selection." << std::endl;
                    return 1;  // Exit with error code
                }

                // List subnets and get user choice for the subnet
                std::string chosenSubnetAddress= listSubnets(con, chosenNetworkID);
                if (chosenSubnetAddress.empty()) {
                    std::cerr << "Error: Invalid subnet selection." << std::endl;
                    return 1;  // Exit with error code
                }

                std::cout << "Chosen Network ID: " << chosenNetworkID << ", Chosen Subnet ID: " << chosenSubnetAddress << std::endl;

                // Example continuation: Perform actions based on chosenNetworkID
                displayTodayAssetInfo(chosenSubnetAddress, chosenNetworkID);

            } catch (sql::SQLException &e) {
                std::cerr << "Error connecting to the database: " << e.what() << std::endl;
                return 1;  // Exit with error code
            }
        }
        else if(userInput == 4){
           cout << "~~~~Thanks for using NAIVE! bye...~~~~" << endl;
            exitNow = true;
        }
        else{
            cout << "~~~~Please input a number for the menu option you would like to select~~~~" << endl;
        }
    }
    return 0;
}