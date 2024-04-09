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




void createTenableAssetFile(std::unique_ptr<sql::Connection>& con, const int networkID, const std::string& subnetAddress) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT A.IPV4, A.DNS, A.Date_last_seen AS Asset_Last_Seen, M.MAC_Address, M.Vendor, "
            "M.Date_last_seen AS MAC_Last_Seen, S.SubnetAddress, S.Description, N.NetworkName "
            "FROM Assets A JOIN MACInfo M ON A.IPV4 = M.IPV4 "
            "JOIN Subnets S ON A.SubnetAddress = S.SubnetAddress "
            "JOIN Networks N ON S.NetworkID = N.NetworkID "
            "WHERE N.NetworkName = ? AND S.SubnetAddress = ?"
        ));
        pstmt->setInt(1, networkID);
        pstmt->setString(2, subnetAddress);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery()); // Correctly managed ResultSet

        nlohmann::json jsonResult;
        jsonResult["assets"] = nlohmann::json::array();
        jsonResult["source"] = "local_scan";

        while (res->next()) {
            nlohmann::json asset;
            asset["ipv4"] = std::string(res->getString("IPV4").c_str());
            asset["mac_address"] = std::string(res->getString("MAC_Address").c_str());
            asset["netbios_name"] = ""; // If available
            asset["fqdn"] = std::string(res->getString("DNS").c_str());
            asset["installed_software"] = nlohmann::json::array(); // Populate as needed or if available
            jsonResult["assets"].push_back(asset);
        }

        std::ofstream file("TenableAssetFile.json");
        file << jsonResult.dump(4);
        file.close();

        std::cout << "Tenable asset file created based on Network Name '" << networkID << "' and Subnet Address '" << subnetAddress << "'." << std::endl;
    } catch (sql::SQLException &e) {
        std::cerr << "SQLException: " << e.what() << " Error Code: " << e.getErrorCode() << std::endl;
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
        cout << "\n 1. Create Tenable Asset File (.JSON)" << endl;
        cout << "\n 2. Create Human Readable Asset File (.csv)" << endl;
        cout << "\n 3. Print Results to the Screen" << endl;
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
                createTenableAssetFile(con, chosenNetworkID, chosenSubnetAddress);

            } catch (sql::SQLException &e) {
                std::cerr << "Error connecting to the database: " << e.what() << std::endl;
                return 1;  // Exit with error code
            }

        }
        else if(userInput == 2){
            //need to call a create file function (for humans)
        }
        else if(userInput == 3){
            //print results to screen from database
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