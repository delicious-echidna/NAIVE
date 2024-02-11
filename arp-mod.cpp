#include <iostream>
#include <cstring>
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

    std::cout << "ARP request sent successfully." << std::endl;

    close(sockfd);
}

// Listen for ARP replies
void listen_for_arp_replies(const char* interface_name) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }
    std::cout << sockfd << std::endl;
    struct sockaddr_ll sockaddr;
    socklen_t sockaddr_len = sizeof(sockaddr);
    unsigned char buffer[4096];

    std::cout << "Listening for ARP replies..." << std::endl;

    while (true) {
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sockaddr, &sockaddr_len);
        std::cout << "I am here" << std::endl;
        if (len < 0) {
            if (errno == EINTR) // Interrupted by signal, continue listening
                continue;
            else {
                perror("Receive failed");
                break;
            }
        }

        std::cout << "Received ARP packet." << std::endl;

        struct ether_arp* arp_packet = reinterpret_cast<struct ether_arp*>(buffer);
        if (ntohs(arp_packet->arp_op) == ARPOP_REPLY) {
            char mac_str[18];
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    arp_packet->arp_sha[0], arp_packet->arp_sha[1], arp_packet->arp_sha[2],
                    arp_packet->arp_sha[3], arp_packet->arp_sha[4], arp_packet->arp_sha[5]);
            std::cout << "Received ARP Reply from IP: " << inet_ntoa(*reinterpret_cast<struct in_addr*>(&arp_packet->arp_spa))
                      << ", MAC: " << mac_str << std::endl;
        }
    }

    close(sockfd);
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

int main() {
    const char* interface_name = "eth0"; // Interface name (adjust as needed)

    // Get the IP address associated with the specified interface
    std::string source_ip = get_interface_ip(interface_name);
    if (source_ip.empty()) {
        std::cerr << "Error: Failed to determine source IP address for interface " << interface_name << std::endl;
        return 1;
    }

    std::cout << "Source IP address: " << source_ip << std::endl;

    // Specify the target IP address (adjust as needed)
    const char* target_ip = "172.31.112.1";

    // Send ARP request
    send_arp_request(interface_name, source_ip.c_str(), target_ip);

    // Listen for ARP replies
    std::cout << "starting the arp replies part" << std::endl;
    listen_for_arp_replies(interface_name);

    return 0;
}
