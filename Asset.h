#ifndef ASSET_H
#define ASSET_H

#include <string>
#include <chrono>
/*
Holds all information of one network asset.
Some attributes may be unused for certain assets.
Export to tenable requires at least one of: 
        dns, ipv4, netbiosname, mac
    for each asset.
*/

class Asset {

private:

std::string agentname;
std::string netbiosname;
std::string localhostname;
std::string dns;
std::string ipv4;
std::string ipv6;

std::string mac;
std::string assetid;
std::string systemtype;
std::string os;
std::string network;
std::string ssh;
std::string tenableid;
std::string bios;
std::string servicenowid;
std::string customattributes;
std::chrono::system_clock::time_point time;

int publicasset;
int licensed;

public:

    Asset(const std::string& ip, const std::string& mac, const std::chrono::system_clock::time_point& time);

    std::string get_agentname();
    std::string get_netbiosname();
    std::string get_localhostname();
    std::string get_dns();
    std::string get_ipv4();
    std::string get_ipv6();

    std::string get_mac();
    std::string get_assetid();
    std::string get_systemtype();
    std::string get_os();
    std::string get_network();
    std::string get_ssh();
    std::string get_tenableid();
    std::string get_bios();
    std::string get_servicenowid();
    std::string get_customattributes();
    std::chrono::system_clock::time_point get_time();

    int get_publicasset();
    int get_licensed();

    void set_agentname(std::string input);
    void set_netbiosname(std::string input);
    void set_localhostname(std::string input);
    void set_dns(std::string input);
    void set_ipv4(std::string input);
    void set_ipv6(std::string input);

    void set_mac(std::string input);
    void set_assetid(std::string input);
    void set_systemtype(std::string input);
    void set_os(std::string input);
    void set_network(std::string input);
    void set_ssh(std::string input);
    void set_tenableid(std::string input);
    void set_bios(std::string input);
    void set_servicenowid(std::string input);
    void set_customattributes(std::string input);

    void set_publicasset(int input);
    void set_licensed(int input);
    void set_time(std::chrono::system_clock::time_point input);

};

#endif // ASSET_H