// arp-mod.h
#ifndef ARP_MOD_H
#define ARP_MOD_H

#include <list>
#include <string>
#include <memory>
#include <iostream>
#include <cstring>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <chrono> 
#include <thread> 
#include <unordered_map>
#include <mariadb/conncpp.hpp>
#include <iomanip>
#include "Asset.h" // Include the Asset definition if it's not defined elsewhere


struct arp_packet {
    struct ether_header eth_hdr;
    struct ether_arp arp_hdr;
};

// Function prototypes
bool GetMACAddress(const char* interface_name, uint8_t* macAddr);
void create_arp_request(struct arp_packet *packet, const char* source_ip, const char* target_ip, const uint8_t* src_mac);
void send_arp_request(const char* interface_name, const char* source_ip, const char* target_ip);
std::list<Asset> listen_for_arp_replies_list(const char* interface_name, int duration_seconds);
void listen_for_arp_replies(const char* interface_name, int duration_seconds);
std::string get_interface_ip(const char* interface_name);
std::string get_subnet_mask(const char* interface_name);
std::list<Asset> arpScan(const char* interface_name);
void resolveHostnames(std::list<Asset>& assets);
std::string choose_network_interface();
int createNetworkAndGetID(std::unique_ptr<sql::Connection>& con, const std::string& networkName);
int createSubnetAndGetID(std::unique_ptr<sql::Connection>& con, int networkID, const std::string& subnetAddress, const std::string& description);
void program(std::string& network_name, std::string& subnet_name, const char* interface_name);

#endif // ARP_MOD_H
