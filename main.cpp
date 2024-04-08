#include <iostream>
#include <string>
#include "arp-mod.h"
using namespace std;

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
    cout<< "~~~~Let's get started and scan your subnet...then your output is up to you!~~~~" << endl;
    std::string network_name, subnet_name;
    cout << "~~~~Please input the name of your network~~~~" << endl;
    getline(cin, network_name);
    cout << "~~~~Please input the name of the subnet being scanned~~~~" << endl;
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