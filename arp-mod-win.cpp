#include <iostream>
#include <cstring>
#include <pcap.h>
#include <WinSock2.h>
#include <IPHlpApi.h>
#include <chrono> 
#include <list>
#include <windows.h>
#include <unordered_map>
#include <thread>
#include "Asset.h"
#include "backend/backend.h"
#include "backend/Jsonlib.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#define ETHERTYPE_ARP 0x0806
#define ARPHRD_ETHER 1
#define ETH_P_IP 0x0800
#define ETH_ALEN 6
#define ARPOP_REQUEST 1
#define ARPOP_REPLY 2

// Define the Ethernet header structure
struct ethernet_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
};

// Define the ARP header structure
struct arp_header {
    uint16_t arp_hrd;    /* Format of hardware address.  */
    uint16_t arp_pro;    /* Format of protocol address.  */
    uint8_t arp_hln;     /* Length of hardware address.  */
    uint8_t arp_pln;     /* Length of protocol address.  */
    uint16_t arp_op;     /* ARP opcode (command).  */

    uint8_t arp_sha[6];  /* Sender hardware address.  */
    uint8_t arp_spa[4];  /* Sender protocol address.  */
    uint8_t arp_tha[6];  /* Target hardware address.  */
    uint8_t arp_tpa[4];  /* Target protocol address.  */
};

// Define the ARP packet structure
struct arp_packet {
    struct ethernet_header eth_hdr;
    struct arp_header arp_hdr;
};

// Create an ARP request packet
void create_arp_request(struct arp_packet *packet, const char* source_ip, const char* target_ip) {
    // Set Ethernet header
    memset(packet->eth_hdr.dest_mac, 0xff, 6); // Destination MAC: Broadcast
    memset(packet->eth_hdr.src_mac, 0, 6);    // Source MAC: Unspecified (will be filled by kernel)
    packet->eth_hdr.eth_type = htons(ETHERTYPE_ARP);   // EtherType: ARP

    // Set ARP header
    packet->arp_hdr.arp_hrd = htons(ARPHRD_ETHER);       // Hardware type: Ethernet
    packet->arp_hdr.arp_pro = htons(ETH_P_IP);           // Protocol type: IPv4
    packet->arp_hdr.arp_hln = ETH_ALEN;                  // Hardware address length: 6 bytes
    packet->arp_hdr.arp_pln = sizeof(in_addr);         // Protocol address length: 4 bytes
    packet->arp_hdr.arp_op = htons(ARPOP_REQUEST);       // ARP operation: Request

    memset(packet->arp_hdr.arp_tha, 0, ETH_ALEN);        // Target hardware address: Unspecified
    //inet_pton(AF_INET, target_ip, &packet->arp_hdr.arp_tpa); // Target protocol address
    // Convert target IP address from text to binary form
    *((unsigned long*)packet->arp_hdr.arp_tpa) = inet_addr(target_ip);

    // Set source hardware address: Unspecified (will be filled by kernel)
    memset(packet->arp_hdr.arp_sha, 0, ETH_ALEN);

    // // Set source protocol address
    // inet_pton(AF_INET, source_ip, &packet->arp_hdr.arp_spa);
    // Convert source IP address from text to binary form
    *((unsigned long*)packet->arp_hdr.arp_spa) = inet_addr(source_ip);
}

// Send ARP request
void send_arp_request(const std::string& interface_name, const char* source_ip, const char* target_ip) {
    pcap_t* pcap_handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the adapter for sending packets
    pcap_handle = pcap_open_live(interface_name.c_str(), 4096, 1, 1000, errbuf);
    if (pcap_handle == nullptr) {
        std::cerr << "Failed to open adapter for sending: " << errbuf << std::endl;
        return;
    }

    struct arp_packet packet;
    create_arp_request(&packet, source_ip, target_ip);

    // Send ARP request
    if (pcap_sendpacket(pcap_handle, reinterpret_cast<const u_char*>(&packet), sizeof(packet)) != 0) {
        std::cerr << "Failed to send ARP request: " << pcap_geterr(pcap_handle) << std::endl;
    }

    pcap_close(pcap_handle);
}

// Listen for ARP replies and return a list of all the responses
std::list<Asset> listen_for_arp_replies_list(const std::string& interface_name, int duration_seconds) {
    std::list<Asset> assets;
    std::unordered_map<std::string, std::string> asset_map;

    pcap_t* pcap_handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the adapter for receiving packets
    std::cout << interface_name << std::endl;
    pcap_handle = pcap_open_live(interface_name.c_str(), 4096, 1, 1000, errbuf);
    if (pcap_handle == nullptr) {
        std::cerr << "Failed to open adapter for listening: " << errbuf << std::endl;
        return assets;
    }

    std::cout << "Listening for ARP replies for " << duration_seconds << " seconds..." << std::endl;

    auto start_time = std::chrono::steady_clock::now(); // Added timing start

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        if (elapsed_seconds >= duration_seconds) {
            std::cout << "Duration elapsed. Stopping listening for ARP replies." << std::endl;
            break;
        }

        struct pcap_pkthdr* header;
        const u_char* packet_data;
        int res = pcap_next_ex(pcap_handle, &header, &packet_data);

        if (res == 0) {
            // Timeout elapsed
            continue;
        } else if (res == -1) {
            std::cerr << "Failed to read packet: " << pcap_geterr(pcap_handle) << std::endl;
            break;
        } else if (res == -2) {
            std::cerr << "No more packets to read from interface." << std::endl;
            break;
        }

        // Extract Ethernet header
        struct ethernet_header* eth_hdr = reinterpret_cast<struct ethernet_header*>(const_cast<u_char*>(packet_data));

        // Check if it's an ARP reply
        if (ntohs(eth_hdr->eth_type) == ETHERTYPE_ARP) {
            struct arp_header* arp_hdr = reinterpret_cast<struct arp_header*>(const_cast<u_char*>(packet_data + sizeof(struct ethernet_header)));

            // Check if it's an ARP reply
            if (ntohs(arp_hdr->arp_op) == ARPOP_REPLY || ntohs(arp_hdr->arp_op) == ARPOP_REQUEST) {
                char mac_str[18];
                sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                        arp_hdr->arp_sha[0], arp_hdr->arp_sha[1], arp_hdr->arp_sha[2],
                        arp_hdr->arp_sha[3], arp_hdr->arp_sha[4], arp_hdr->arp_sha[5]);

                char ip_str[INET_ADDRSTRLEN];
                // inet_ntop(AF_INET, arp_hdr->arp_spa, ip_str, INET_ADDRSTRLEN);
                // Convert binary IP address to text form
                strcpy(ip_str, inet_ntoa(*(struct in_addr*)arp_hdr->arp_spa));


                // Check if the asset (IP) is already in the map
                if (asset_map.find(ip_str) == asset_map.end()) {
                    assets.emplace_back(ip_str, mac_str, std::chrono::system_clock::now());
                    asset_map[ip_str] = mac_str;
                }
            }
        }
    }

    pcap_close(pcap_handle);

    // Get MAC vendors
    for (auto& asset : assets) {
        asset.set_macVendor();
    }
    return assets;
}

// Listen for ARP replies
void listen_for_arp_replies(const char* interface_name, int duration_seconds) {
    pcap_t* pcap_handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the adapter for receiving packets
    pcap_handle = pcap_open_live(interface_name, 4096, 1, 1000, errbuf);
    if (pcap_handle == nullptr) {
        std::cerr << "Failed to open adapter for listening: " << errbuf << std::endl;
        return;
    }

    std::cout << "Listening for ARP replies for " << duration_seconds << " seconds..." << std::endl;

    auto start_time = std::chrono::steady_clock::now(); // Added timing start

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        if (elapsed_seconds >= duration_seconds) {
            std::cout << "Duration elapsed. Stopping listening for ARP replies." << std::endl;
            break;
        }

        struct pcap_pkthdr* header;
        const u_char* packet_data;
        int res = pcap_next_ex(pcap_handle, &header, &packet_data);

        if (res == 0) {
            // Timeout elapsed
            continue;
        } else if (res == -1) {
            std::cerr << "Failed to read packet: " << pcap_geterr(pcap_handle) << std::endl;
            break;
        } else if (res == -2) {
            std::cerr << "No more packets to read from interface." << std::endl;
            break;
        }

        // Extract Ethernet header
        struct ethernet_header* eth_hdr = reinterpret_cast<struct ethernet_header*>(const_cast<u_char*>(packet_data));

        // Check if it's an ARP reply
        if (ntohs(eth_hdr->eth_type) == ETHERTYPE_ARP) {
            struct arp_header* arp_hdr = reinterpret_cast<struct arp_header*>(const_cast<u_char*>(packet_data + sizeof(struct ethernet_header)));

            // Check if it's an ARP reply
            if (ntohs(arp_hdr->arp_op) == ARPOP_REPLY || ntohs(arp_hdr->arp_op) == ARPOP_REQUEST) {
                char mac_str[18];
                sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                        arp_hdr->arp_sha[0], arp_hdr->arp_sha[1], arp_hdr->arp_sha[2],
                        arp_hdr->arp_sha[3], arp_hdr->arp_sha[4], arp_hdr->arp_sha[5]);

                char ip_str[INET_ADDRSTRLEN];
                // inet_ntop(AF_INET, arp_hdr->arp_spa, ip_str, INET_ADDRSTRLEN);
                // Convert binary IP address to text form
                strcpy(ip_str, inet_ntoa(*(struct in_addr*)arp_hdr->arp_spa));


                std::cout << "Received ARP Reply from IP: " << ip_str
                          << ", MAC: " << mac_str << std::endl;
            }
        }
    }

    pcap_close(pcap_handle);
}

std::string get_interface_ip(const char* interface_name) {
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // First call to GetAdaptersAddresses to get the necessary buffer size
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &outBufLen);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        return "";
    }

    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (pAddresses == nullptr) {
        std::cerr << "Memory allocation failed." << std::endl;
        return "";
    }

    // Second call to GetAdaptersAddresses to retrieve adapter information
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
    if (dwRetVal != NO_ERROR) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        free(pAddresses);
        return "";
    }

    // Debug: Print the number of adapters found
    std::cout << "Number of adapters found: " << dwRetVal << std::endl;

    // Iterate through the list of adapters
    for (PIP_ADAPTER_ADDRESSES pCurrAdapter = pAddresses; pCurrAdapter != nullptr; pCurrAdapter = pCurrAdapter->Next) {
        // Debug: Print the adapter name for each iteration
        std::cout << "Adapter Name: " << pCurrAdapter->AdapterName << std::endl;
        if (strcmp(pCurrAdapter->AdapterName, interface_name) == 0) {
            // Iterate through the list of unicast addresses for the adapter
            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAdapter->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
                sockaddr* sockaddr_ptr = pUnicast->Address.lpSockaddr;
                if (sockaddr_ptr->sa_family == AF_INET) {
                    // Cast to sockaddr_in and retrieve the IP address
                    sockaddr_in* sockaddr_in_ptr = reinterpret_cast<sockaddr_in*>(sockaddr_ptr);
                    char ip_str[INET_ADDRSTRLEN];
                    // Debug: Print the retrieved IP address
                    std::cout << "Retrieved IP Address: " << inet_ntoa(sockaddr_in_ptr->sin_addr) << std::endl;
                    // Copy the IP address to the return string
                    //inet_ntop(AF_INET, &(sockaddr_in_ptr->sin_addr), ip_str, INET_ADDRSTRLEN);
                    strcpy(ip_str, inet_ntoa(sockaddr_in_ptr->sin_addr));
                    free(pAddresses);
                    return ip_str;
                }
            }
        }
    }

    free(pAddresses);
    return "";
}
// Get the subnet mask associated with the specified interface
std::string get_subnet_mask(const char* interface_name) {
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = nullptr;
    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    DWORD dwRetVal = 0;

    pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == nullptr) {
        std::cerr << "Error allocating memory needed to call GetAdaptersInfo" << std::endl;
        return "";
    }

    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);
        if (pAdapterInfo == nullptr) {
            std::cerr << "Error allocating memory needed to call GetAdaptersInfo" << std::endl;
            return "";
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) != NO_ERROR) {
        std::cerr << "GetAdaptersInfo failed with error: " << dwRetVal << std::endl;
        free(pAdapterInfo);
        return "";
    }

    pAdapter = pAdapterInfo;
    while (pAdapter) {
        if (strcmp(pAdapter->AdapterName, interface_name) == 0) {
            return pAdapter->IpAddressList.IpMask.String;
        }
        pAdapter = pAdapter->Next;
    }

    free(pAdapterInfo);
    return "";
}
std::string get_host_ip() {
    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // First call to GetAdaptersAddresses to get the necessary buffer size
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &outBufLen);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        return "";
    }

    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (pAddresses == nullptr) {
        std::cerr << "Memory allocation failed." << std::endl;
        return "";
    }

    // Second call to GetAdaptersAddresses to retrieve adapter information
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
    if (dwRetVal != NO_ERROR) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        free(pAddresses);
        return "";
    }

    // Iterate through the list of adapters until we find the first IPv4 address
    for (PIP_ADAPTER_ADDRESSES pCurrAdapter = pAddresses; pCurrAdapter != nullptr; pCurrAdapter = pCurrAdapter->Next) {
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAdapter->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
            sockaddr* sockaddr_ptr = pUnicast->Address.lpSockaddr;
            if (sockaddr_ptr->sa_family == AF_INET) {
                // Cast to sockaddr_in and retrieve the IP address
                sockaddr_in* sockaddr_in_ptr = reinterpret_cast<sockaddr_in*>(sockaddr_ptr);
                char* ip_str = inet_ntoa(sockaddr_in_ptr->sin_addr);
                free(pAddresses);
                return std::string(ip_str);
            }
        }
    }

    free(pAddresses);
    return "";
}

std::string get_interface_name(std::string& host_ip) {
    std::string interface_name;

    ULONG outBufLen = 0;
    DWORD dwRetVal = 0;

    // First call to GetAdaptersAddresses to get the necessary buffer size
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &outBufLen);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        return "";
    }

    pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (pAddresses == nullptr) {
        std::cerr << "Memory allocation failed." << std::endl;
        return "";
    }

    // Second call to GetAdaptersAddresses to retrieve adapter information
    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
    if (dwRetVal != NO_ERROR) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        free(pAddresses);
        return "";
    }

    // Iterate through the list of adapters
    for (PIP_ADAPTER_ADDRESSES pCurrAdapter = pAddresses; pCurrAdapter != nullptr; pCurrAdapter = pCurrAdapter->Next) {
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAdapter->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
            sockaddr* sockaddr_ptr = pUnicast->Address.lpSockaddr;
            if (sockaddr_ptr->sa_family == AF_INET) {
                sockaddr_in* sockaddr_in_ptr = reinterpret_cast<sockaddr_in*>(sockaddr_ptr);
                char* ip_str = inet_ntoa(sockaddr_in_ptr->sin_addr);
                if (ip_str == host_ip) {
                    interface_name = pCurrAdapter->AdapterName;
                    break;
                }
            }
        }
        if (!interface_name.empty()) {
            break;
        }
    }

    free(pAddresses);
    return interface_name;
}

std::list<Asset> arpScan(){
    std::list<Asset> assets;

    std::string source_ip = get_host_ip();
    if (source_ip.empty()) {
        std::cerr << "Error: Failed to determine source IP address" << std::endl;
        return assets;
    }
    
    std::string interface_name = get_interface_name(source_ip);

    std::cout << "Source IP address: " << source_ip << std::endl;

    std::string subnet_mask = get_subnet_mask(interface_name.c_str());

    if (subnet_mask.empty()) {
        std::cerr << "Error: Failed to determine subnet mask for interface " << interface_name << std::endl;
        return assets;
    }

    // Concatenate the prefix with the interface GUID
    interface_name = "\\Device\\NPF_" + interface_name;
    std::cout << "Subnet Mask: " << subnet_mask << " Interface Name: " << interface_name << std::endl;

    std::cout << "Starting the ARP replies & listening part" << std::endl;
    pcap_if_t* alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Get the list of available adapters
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs: " << errbuf << std::endl;
        return assets;
    }

    // Iterate through the list and print adapter names
    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
        std::cout << "Adapter Name: " << d->name << std::endl;
        std::cout << "Description: " << (d->description ? d->description : "No description available") << std::endl;
        std::cout << std::endl;
    }
    // Free the adapter list
    pcap_freealldevs(alldevs);
    
    //threading with ARP and return of assets
    std::cout << "Starting the ARP replies & listening part" << std::endl;

    // Thread for sending ARP requests
    std::string target_ip_prefix = source_ip.substr(0, source_ip.rfind(".")) + ".";
    const int MAX_IP_RANGE = 255;
    std::thread send_thread([&]() {
        for (int i = 1; i <= MAX_IP_RANGE; ++i) {
            std::string target_ip = target_ip_prefix + std::to_string(i);
            send_arp_request(interface_name, source_ip.c_str(), target_ip.c_str());
        }
    });

    // Thread for listening for ARP replies
    std::thread listen_thread([&]() {
        assets = listen_for_arp_replies_list(interface_name, 60);
    });

    // Join threads
    send_thread.join();
    listen_thread.join();

    //Send assets to database
    std::list<Asset>::iterator it;
    for (it = myList.begin(); it != myList.end(); ++it) {
        std::string ipv4 = (*it).get_ipv4();
        std::string mac = (*it).get_mac();
        std::string scan = "arp-scan";
        std::string ipv6 = (*it).get_ipv6();
        std::string vendor = (*it).get_macVendor();
        std::string os = (*it).get_os();
        std::time_t time_received = std::chrono::system_clock::to_time_t((*it).get_time());
        std::string time_str = std::ctime(&time_received);
        time_str.pop_back();
        std::string other = (*it).get_customattributes();
        if(mac.size() == 0)
            mac = "NULL";
        if(ipv6.size() == 0)
            ipv6 = "NULL";
        if(vendor.size() == 0)
            vendor = "NULL";
        if(os.size() == 0)
            os = "NULL";
        if(other.size() == 0)
            other = "NULL";

        db_insert(ipv4, mac, scan, ipv6, vendor, os, time_str, other);
    }



    return assets;
}

void resolveHostnames(std::list<Asset>& assets){
    for (auto& asset : assets) {
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr(asset.get_ipv4().c_str());

        char hostname[NI_MAXHOST];
        int ret = getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, NI_MAXHOST, NULL, 0, 0);
        
        if (ret != 0) {
            std::cerr << "Error resolving hostname for " << asset.get_ipv4() << ": " << gai_strerror(ret) << std::endl;
            asset.set_dns("Unknown");
        } 
        else {
            asset.set_dns(hostname);
        }
    }
}

int main() {

    std::list<Asset> assets = arpScan();
    if (!assets.empty()) {
        resolveHostnames(assets);
    }

    for (auto& asset : assets) {
        std::time_t time_received = std::chrono::system_clock::to_time_t(asset.get_time());
        std::string time_str = std::ctime(&time_received);
        std::cout << "IP: " << asset.get_ipv4() << ", MAC: " << asset.get_mac() << ", Vendor: " << asset.get_macVendor() << ", DNS: " << asset.get_dns() << ", Time: " << time_str;
    }
    return 0;
}
