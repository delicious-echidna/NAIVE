#include "Asset.h"

/*
std::string agentname
std::string netbiosname
std::string localhostname
std::string dns
std::string ipv4
std::string ipv6

std::string mac
std::string assetid
std::string systemtype
std::string os
std::string network
std::string ssh
std::string tenableid
std::string bios
std::string servicenowid
std::string customattributes

int publicasset
int licensed
*/


Asset::Asset(const std::string& ip, const std::string& mac, const std::chrono::system_clock::time_point& time)
        : ipv4(ip), mac(mac), time(time) {}

std::string Asset::get_agentname(){
    return agentname;
}
std::string Asset::get_netbiosname(){
    return netbiosname;
}
std::string Asset::get_localhostname(){
    return localhostname;
}
std::string Asset::get_dns(){
    return dns;
}
std::string Asset::get_ipv4() const{
    return ipv4;
}
std::string Asset::get_ipv6(){
    return ipv6;
}

std::string Asset::get_mac() const{
    return mac;
}
std::string Asset::get_macVendor(){
    return macVendor;
}
std::string Asset::get_systemtype(){
    return systemtype;
}
std::string Asset::get_os(){
    return os;
}
std::string Asset::get_network(){
    return network;
}
std::string Asset::get_ssh(){
    return ssh;
}
std::string Asset::get_tenableid(){
    return tenableid;
}
std::string Asset::get_bios(){
    return bios;
}
std::string Asset::get_servicenowid(){
    return servicenowid;
}
std::string Asset::get_customattributes(){
    return customattributes;
}

int Asset::get_publicasset(){
    return publicasset;
}
int Asset::get_licensed(){
    return licensed;
}
std::chrono::system_clock::time_point Asset::get_time() const{
    return time;
}


void Asset::set_agentname(std::string input){
    agentname = input;
}
void Asset::set_netbiosname(std::string input){
    netbiosname = input;
}
void Asset::set_localhostname(std::string input){
    localhostname = input;
}
void Asset::set_dns(std::string input){
    dns = input;
}
void Asset::set_ipv4(std::string input){
    ipv4 = input;
}
void Asset::set_ipv6(std::string input){
    ipv6 = input;
}

void Asset::set_mac(std::string input){
    mac = input;
}
void Asset::set_macVendor(){
    const std::string ouiFilePath = "OUI.txt";
    std::ifstream file(ouiFilePath);
    if (!file.is_open()){
        std::cerr << "Error opening OUI database file: " << ouiFilePath << std::endl;
        return;
    }
    std::string line;
    std::string macPrefix = mac.substr(0, 8); // Get the first 8 characters of the MAC address
    while(std::getline(file, line)){
        std::istringstream iss(line);
        std::string oui, vendor;
        if(iss >> oui >> vendor){
            // Convert both MAC prefix and OUI prefix to uppercase for case-insensitive comparison
            std::transform(macPrefix.begin(), macPrefix.end(), macPrefix.begin(), ::toupper);
            std::transform(oui.begin(), oui.end(), oui.begin(), ::toupper);
            if (macPrefix == oui){
                macVendor = vendor;
                break;
            }
        }
    }

    file.close();

    if (macVendor.empty()){
        macVendor = "Unknown";
    }
}
void Asset::set_systemtype(std::string input){
    systemtype = input;
}
void Asset::set_os(std::string input){
    os = input;
}
void Asset::set_network(std::string input){
    network = input;
}
void Asset::set_ssh(std::string input){
    ssh = input;
}
void Asset::set_tenableid(std::string input){
    tenableid = input;
}
void Asset::set_bios(std::string input){
    bios = input;
}
void Asset::set_servicenowid(std::string input){
    servicenowid = input;
}
void Asset::set_customattributes(std::string input){
    customattributes = input;
}

void Asset::set_publicasset(int input){
    publicasset = input;
}
void Asset::set_licensed(int input){
    licensed = input;
}
void Asset::set_time(std::chrono::system_clock::time_point input){
    time = input;
}



