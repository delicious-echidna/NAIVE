#include <iostream>
#include <string>
#include "arp-mod-win.cpp"
#include "backend/backend.h"
#include "backend/Jsonlib.h"
using namespace std;

//there could potentially be flags we introduce but for now...this is the simple stuff
main(){
    bool exitNow = false;
    string userInput; //we need to validate this to prevent overflow issues...
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
    while(!exitNow){
        cout << "\n Please enter an option:" << endl;
        cout << "\n 1. Scan Local Network Segment & Create Tenable Asset File" << endl;
        cout << "\n 2. Scan Local Network Segment & Create Human Readable Asset File" << endl;
        cout << "\n 3. Scan Local Network Segment & Print Results to the Screen" << endl;
        cout << "\n 4. Exit" << endl;
        cin >> userInput;
        if(userInput == "1"){
            //need to call the scan function (should return a vector of assets)
            //need to call a create file function (for tenable)
        }
        else if(userInput == "2"){
            //need to call the scan function (should return a vector of assets)
            //need to call a create file function (for humans)
        }
        else if(userInput == "3"){
            //need to call the scan function (should return a vector of assets)
            //print results to screen
        }
        else if(userInput == "4"){
           cout << "~~~~Thanks for using NAIVE! bye...~~~~" << endl;
            exitNow = true;
        }
        else{
            cout << "~~~~Please input a number for the menu option you would like to select~~~~" << endl;
        }
    }
    return 0;
}