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

// Define the ARP packet structure
struct arp_packet {
    struct ether_header eth_hdr;
    struct ether_arp arp_hdr;
};

// Create an ARP request packet
void create_arp_request(struct arp_packet *packet, const char* source_ip, const char* target_ip) {
    // Set Ethernet header
    memset(packet->eth_hdr.ether_dhost, 0xff, ETH_ALEN); // Destination MAC: Broadcast
    memset(packet->eth_hdr.ether_shost, 0, ETH_ALEN);    // Source MAC: Unspecified (will be filled by kernel)
    packet->eth_hdr.ether_type = htons(ETHERTYPE_ARP);   // EtherType: ARP

    // Set ARP header
    packet->arp_hdr.arp_hrd = htons(ARPHRD_ETHER);       // Hardware type: Ethernet
    packet->arp_hdr.arp_pro = htons(ETH_P_IP);           // Protocol type: IPv4
    packet->arp_hdr.arp_hln = ETH_ALEN;                  // Hardware address length: 6 bytes
    packet->arp_hdr.arp_pln = sizeof(in_addr_t);         // Protocol address length: 4 bytes
    packet->arp_hdr.arp_op = htons(ARPOP_REQUEST);       // ARP operation: Request

    memset(packet->arp_hdr.arp_tha, 0, ETH_ALEN);        // Target hardware address: Unspecified
    inet_pton(AF_INET, target_ip, &packet->arp_hdr.arp_tpa); // Target protocol address

    // Set source hardware address: Unspecified (will be filled by kernel)
    memset(packet->arp_hdr.arp_sha, 0, ETH_ALEN);

    // Set source protocol address
    inet_pton(AF_INET, source_ip, &packet->arp_hdr.arp_spa);
}

// Send ARP request
void send_arp_request(const char* interface_name, const char* source_ip, const char* target_ip) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    struct arp_packet packet;
    create_arp_request(&packet, source_ip, target_ip);

    struct sockaddr_ll sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sll_family = AF_PACKET;
    sockaddr.sll_protocol = htons(ETH_P_ARP);
    sockaddr.sll_ifindex = if_nametoindex(interface_name);

    // Send ARP request
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("Send failed");
        close(sockfd);
        return;
    }

    //std::cout << "ARP request sent successfully." << std::endl;

    close(sockfd);
}

// Listen for ARP replies
void listen_for_arp_replies(const char* interface_name, int duration_seconds) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* pcap_handle = pcap_open_live(interface_name, 4096, 1, 1000, errbuf);
    if (pcap_handle == nullptr) {
        std::cerr << "Failed to open interface: " << errbuf << std::endl;
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
        //std::cout << res << std::endl;
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

        // // Extract Ethernet header
        // struct ether_header* eth_hdr = (struct ether_header*)packet_data;

        // // Print Ethernet header
        // std::cout << "Ethernet Header:" << std::endl;
        // std::cout << "Destination MAC: ";
        // for (int i = 0; i < ETH_ALEN; ++i) {
        //     printf("%02x", eth_hdr->ether_dhost[i]);
        //     if (i < ETH_ALEN - 1) printf(":");
        // }
        // std::cout << std::endl;

        // std::cout << "Source MAC: ";
        // for (int i = 0; i < ETH_ALEN; ++i) {
        //     printf("%02x", eth_hdr->ether_shost[i]);
        //     if (i < ETH_ALEN - 1) printf(":");
        // }
        // std::cout << std::endl;

        // std::cout << "EtherType: " << ntohs(eth_hdr->ether_type) << std::endl;

        // // Check if it's an ARP packet
        // if (ntohs(eth_hdr->ether_type) != ETHERTYPE_ARP) {
        //     // Not an ARP packet, skip
        //     std::cout << "Not an ARP packet" << std::endl;
        //     continue;
        // }
        
        // Extract ARP header
        struct ether_arp* arp_hdr = (struct ether_arp*)(packet_data + sizeof(struct ether_header));

        // Check if it's an ARP reply
        if (ntohs(arp_hdr->arp_op) == ARPOP_REPLY) {
            char mac_str[18];
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    arp_hdr->arp_sha[0], arp_hdr->arp_sha[1], arp_hdr->arp_sha[2],
                    arp_hdr->arp_sha[3], arp_hdr->arp_sha[4], arp_hdr->arp_sha[5]);

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, arp_hdr->arp_spa, ip_str, INET_ADDRSTRLEN);

            std::cout << "Received ARP Reply from IP: " << ip_str
                      << ", MAC: " << mac_str << std::endl;
        }
    }

    pcap_close(pcap_handle);
}


// Get the IP address associated with the specified interface
std::string get_interface_ip(const char* interface_name) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    std::string interface_ip;

    // Walk through linked list, maintaining head pointer so we can free list later
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        // Check for IPv4 address on specified interface
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, interface_name) == 0) {
            struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN);
            interface_ip = ip_str;
            break;
        }
    }

    freeifaddrs(ifaddr);
    return interface_ip;
}

// Get the subnet mask associated with the specified interface
std::string get_subnet_mask(const char* interface_name) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
        perror("ioctl failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);

    struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_netmask);
    return inet_ntoa(addr->sin_addr);
}

int main() {
    const char* interface_name = "eth0"; // Interface name (adjust as needed)

    // Get the IP address associated with the specified interface
    std::string source_ip = get_interface_ip(interface_name);
    if (source_ip.empty()) {
        std::cerr << "Error: Failed to determine source IP address for interface " << interface_name << std::endl;
        return 1;
    }

    std::cout << "Source IP address: " << source_ip << std::endl;

    // Get the subnet mask associated with the specified interface
    std::string subnet_mask = get_subnet_mask(interface_name);
    if (subnet_mask.empty()) {
        std::cerr << "Error: Failed to determine subnet mask for interface " << interface_name << std::endl;
        return 1;
    }

    std::cout << "Subnet Mask: " << subnet_mask << std::endl;

    // // Specify the target IP address range based on the subnet
    // std::string target_ip_prefix = source_ip.substr(0, source_ip.rfind(".")) + ".";
    // const int MAX_IP_RANGE = 255; // Adjust as needed
    // for (int i = 1; i <= MAX_IP_RANGE; ++i) {
    //     std::string target_ip = target_ip_prefix + std::to_string(i);

    //     // Send ARP request
    //     send_arp_request(interface_name, source_ip.c_str(), target_ip.c_str());
    // }

    // // Listen for ARP replies
    // std::cout << "Starting the ARP replies part" << std::endl;
    // listen_for_arp_replies(interface_name, 5);

    // // Thread for sending ARP requests
    // std::string target_ip_prefix = source_ip.substr(0, source_ip.rfind(".")) + ".";
    // const int MAX_IP_RANGE = 255;
    // std::thread send_thread([&]() {
    //     for (int i = 1; i <= MAX_IP_RANGE; ++i) {
    //         std::string target_ip = target_ip_prefix + std::to_string(i);
    //         send_arp_request(interface_name, source_ip.c_str(), target_ip.c_str());
    //     }
    // });

    // // Thread for listening for ARP replies
    // std::thread listen_thread([&]() {
    //     listen_for_arp_replies(interface_name, 7);
    // });

    // // Join threads
    // send_thread.join();
    // listen_thread.join();

    //threading v2
    std::cout << "Starting the ARP replies part" << std::endl;
    std::string target_ip_prefix = source_ip.substr(0, source_ip.rfind(".")) + ".";
    const int MAX_IP_RANGE = 255;
    for(int i = 0; i <= MAX_IP_RANGE; i++){
        std::string target_ip = target_ip_prefix + std::to_string(i);
        std::thread send_thread([&](){
            send_arp_request(interface_name, source_ip.c_str(), target_ip.c_str());
        });
        std::thread listen_thread([&]() { 
            listen_for_arp_replies(interface_name, 2);
        });
        send_thread.join();
        listen_thread.join();
    }

    return 0;
}
