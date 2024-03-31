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

//gets the source mac address
bool GetMACAddressByGUID(const std::string& guid, uint8_t* macAddr) {
    ULONG outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
    DWORD dwRetVal = 0;
    PIP_ADAPTER_ADDRESSES pAddresses = nullptr, pCurrAddresses = nullptr;

    dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (pCurrAddresses = pAddresses; pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next) {
            std::string adapterName = pCurrAddresses->AdapterName;
            if (adapterName.find(guid) != std::string::npos) {
                memcpy(macAddr, pCurrAddresses->PhysicalAddress, 6);
                free(pAddresses);
                return true; // Success
            }
        }
    }

    if (pAddresses) {
        free(pAddresses);
    }
    return false; // Adapter not found or error occurred
}
// Create an ARP request packet
void create_arp_request(struct arp_packet *packet, const char* source_ip, const char* target_ip, const uint8_t* src_mac) {
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

    // Set source hardware address
    memcpy(packet->eth_hdr.src_mac, src_mac, ETH_ALEN);

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
    
    // Assuming interface_name contains the full pcap device name like "\Device\NPF_{GUID}"
    // Extract the GUID from the interface_name
    auto start = interface_name.find('{');
    auto end = interface_name.find('}', start);
    uint8_t macAddr[6] = {0};
    if (start != std::string::npos && end != std::string::npos) {
        std::string guid = interface_name.substr(start + 1, end - start - 1);
        if (!GetMACAddressByGUID(guid, macAddr)) { // Now using the extracted GUID
            std::cerr << "Failed to get MAC address for interface GUID " << guid << std::endl;
            pcap_close(pcap_handle);
            return;
        }
    } else {
        std::cerr << "Invalid interface name format: " << interface_name << std::endl;
        pcap_close(pcap_handle);
        return;
    }

    struct arp_packet packet;
    create_arp_request(&packet, source_ip, target_ip, macAddr);

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
std::string get_subnet_mask(const std::string& interface_guid) {
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // We're only interested in IPv4 addresses here
    LPVOID lpMsgBuf = nullptr;
    PIP_ADAPTER_ADDRESSES addresses = nullptr;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;
    DWORD dwRetVal = 0;

    // Start with a buffer size that might suffice; adjust as needed.
    outBufLen = 15 * 1024; // 15 KB should be enough according to documentation

    do {
        addresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (addresses == nullptr) {
            std::cerr << "Memory allocation failed for IP_ADAPTER_ADDRESSES struct" << std::endl;
            return "";
        }

        dwRetVal = GetAdaptersAddresses(family, flags, nullptr, addresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(addresses);
            addresses = nullptr;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < 3));

    if (dwRetVal != NO_ERROR) {
        std::cerr << "GetAdaptersAddresses failed with error: " << dwRetVal << std::endl;
        if (addresses) free(addresses);
        return "";
    }

    std::string subnetMask = "";
    // Iterate through linked list of adapters and match the GUID
    for (PIP_ADAPTER_ADDRESSES currAddresses = addresses; currAddresses != nullptr; currAddresses = currAddresses->Next) {
        std::string adapterName = currAddresses->AdapterName;

        // Check if the current adapter's name contains the interface GUID
        if (adapterName.find(interface_guid) != std::string::npos) {
            // Found the matching adapter, now get the subnet mask
            for (auto unicast = currAddresses->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
                if (unicast->Address.lpSockaddr->sa_family == AF_INET) { // IPv4
                    IP_ADAPTER_PREFIX* prefix = currAddresses->FirstPrefix;
                    while (prefix) {
                        if (prefix->Address.lpSockaddr->sa_family == AF_INET) { // IPv4
                            sockaddr_in* subnetSockaddr = (sockaddr_in*)prefix->Address.lpSockaddr;
                            char subnetStr[INET_ADDRSTRLEN];
                            if (inet_ntop(AF_INET, &subnetSockaddr->sin_addr, subnetStr, sizeof(subnetStr))) {
                                subnetMask = subnetStr;
                                break;
                            }
                        }
                        prefix = prefix->Next;
                    }
                    if (!subnetMask.empty()) break;
                }
            }
        }
        if (!subnetMask.empty()) break;
    }

    if (addresses) free(addresses);
    return subnetMask;
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
std::string list_and_select_interface() {
    pcap_if_t* alldevs;
    pcap_if_t* d;
    char errbuf[PCAP_ERRBUF_SIZE];
    int i = 0;
    int interface_number;

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "Error in pcap_findalldevs: " << errbuf << std::endl;
        return "";
    }

    std::cout << "Available network interfaces:" << std::endl;
    for (d = alldevs; d; d = d->next) {
        std::cout << ++i << ". " << d->name << (d->description ? (" (" + std::string(d->description) + ")") : "") << std::endl;
    }

    std::cout << "Select interface number for ARP scanning: ";
    std::cin >> interface_number;

    // Check for valid input
    if (interface_number < 1 || interface_number > i) {
        std::cerr << "Invalid interface number selected." << std::endl;
        pcap_freealldevs(alldevs); // Remember to free the device list
        return "";
    }

    // Find the selected interface
    for (i = 0, d = alldevs; i < interface_number - 1; d = d->next, ++i);

    // We assume d is now the selected interface
    std::string selected_interface = d->name;
    pcap_freealldevs(alldevs); // Free the device list

    return selected_interface;
}

std::list<Asset> arpScan(){
    std::list<Asset> assets;

    std::string source_ip = get_host_ip();
    if (source_ip.empty()) {
        std::cerr << "Error: Failed to determine source IP address" << std::endl;
        return assets;
    }
    
    std::cout << source_ip << std::endl;
    // std::string interface_name = get_interface_name(source_ip);

    // Let the user select the network interface
    std::string interface_name = list_and_select_interface();
    if (interface_name.empty()) {
        std::cerr << "No interface selected or error occurred. Exiting." << std::endl;
        return assets;
    }

    std::cout << "Source IP address: " << source_ip << std::endl;
    auto guid_start = interface_name.find('{');
    auto guid_end = interface_name.find('}');
    if (guid_start != std::string::npos && guid_end != std::string::npos && guid_end > guid_start) {
        std::string interface_guid = interface_name.substr(guid_start, guid_end - guid_start + 1);
        // Now you can use interface_guid with the adjusted function to match and retrieve the subnet mask
        std::string subnet_mask = get_subnet_mask(interface_guid);
        if (subnet_mask.empty()) {
        std::cerr << "Error: Failed to determine subnet mask for interface " << interface_name << std::endl;
        return assets;
        }
        std::cout << "Subnet address: " << subnet_mask << std::endl;
    }

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
