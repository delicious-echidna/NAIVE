


#include "Jsonlib.h"

using namespace std;



void createjson(list<string> assets) {

    if (assets.empty()) {
        cerr << "Error: Unable to create JSON with empty asset list" << endl;
        return;
    }

    int counter = 0;

    list<string> listings;
    string source;

    for (list<string>::iterator it = assets.begin(); it != assets.end(); ++it) {

        //populate 'listings' with string representation of each asset's JSON entry

        /*
            0 asset_id INTEGER PRIMARY KEY,
            1 ipv4 varchar(255),
            2 mac_address varchar(255),
            3 scan_method varchar(255),
            4 ipv6 varchar(255),
            5 vendor varchar(255),
            6 os varchar(255),
            7 date_last_seen varchar(255),
            8 other_attributes varchar(255)
        */

        stringstream currline(*it);
        string atts[9];

        int j = 0;
        while (currline.good()) {
            string substr;
            getline(currline, substr, ',');
            atts[j] = substr;
            j++;
        }
        string curr;

        source = "scan";
        
        curr += "\t\t{\n";

        if (atts[1] != "NULL") {
            curr += "\t\t\t\"ipv4\": ";
            curr += "\"" + atts[1] + "\",\n";
        }
        if (atts[2] != "NULL") {
            curr += "\t\t\t\"dns\": ";
            curr += "\"" + atts[2] + "\",\n";
        }
        //if (atts[3] != "NULL") {
        //    curr += "\t\t\t\"date\": ";
        //    curr += "\"" + atts[3] + "\",\n";
        //}
        if (atts[4] != "NULL") {
            curr += "\t\t\t\"mac_address\": ";
            curr += "\"" + atts[4] + "\",\n";
        }
        
        if (atts[5] != "NULL") {
            curr += "\t\t\t\"vendor\": ";
            curr += "\"" + atts[5] + "\",\n";
        }
        
        

        curr.pop_back();
        curr.pop_back();
        curr += "\n\t\t}";

        listings.push_back(curr);

        counter++;
    }

    string filename = times() + ".json";
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        cerr << "Error: Unable to open file " << filename << " for writing." << endl;
        return;
    }

    //fill the file

    ofs << "{\n";
    ofs << "\t\"assets\": [\n";
    
    int j = 0;
    for (list<string>::iterator it2 = listings.begin(); it2 != listings.end(); ++it2) {
        j++;
        ofs << *it2;
        if (j != counter) {
            ofs << ",\n";
        }
        else {
            ofs << "\n";

        }
    }
    
    ofs << "\t],\n";
    ofs << "\t\t\"source\": ";
    ofs << "\"";
    ofs << source;
    ofs << "\"\n}";

    ofs.close();

    //cout << counter << "assets logged." << endl;

}

string times() {

    time_t my_time = time(NULL);
    char datec[26];
    ctime_s(datec, sizeof(datec), &my_time);
    string dates(datec);

    for (int i = 0; i < dates.size(); i++) {
        if (dates[i] == ' ') {
            dates[i] = '_';
        }
        if (dates[i] == ':') {
            dates[i] = '-';
        }
    }
    dates.pop_back();

    return dates;
}


