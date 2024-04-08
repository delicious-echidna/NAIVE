#include "arp-mod.h"


//get machine mac addy
bool GetMACAddress(const char* interface_name, uint8_t* macAddr) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Socket creation failed");
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFHWADDR failed");
        close(fd);
        return false;
    }

    close(fd);
    memcpy(macAddr, ifr.ifr_hwaddr.sa_data, 6);
    return true;
}
// Create an ARP request packet
void create_arp_request(struct arp_packet *packet, const char* source_ip, const char* target_ip, const uint8_t* src_mac) {
    // Set Ethernet header
    memset(packet->eth_hdr.ether_dhost, 0xff, ETH_ALEN); // Destination MAC: Broadcast
    memset(packet->eth_hdr.ether_shost, 0, ETH_ALEN);    // Source MAC: Unspecified (will be filled by kernel)
    packet->eth_hdr.ether_type = htons(ETHERTYPE_ARP);   // EtherType: ARP

    // Set ARP header
    packet->arp_hdr.arp_hrd = htons(ARPHRD_ETHER);       // Hardware type: Ethernet
    packet->arp_hdr.arp_pro = htons(ETH_P_IP);           // Protocol type: IPv4
    packet->arp_hdr.arp_hln = ETH_ALEN;                  // Hardware address length: 6 bytes
    packet->arp_hdr.arp_pln = sizeof(in_addr_t);         // Protocol address length: 4 bytes
    packet->arp_hdr.arp_op = htons(ARPOP_REQUEST);
    inet_pton(AF_INET, target_ip, &packet->arp_hdr.arp_tpa);
    inet_pton(AF_INET, source_ip, &packet->arp_hdr.arp_spa);
    memcpy(packet->arp_hdr.arp_sha, src_mac, ETH_ALEN); // Use provided MAC address

    // packet->arp_hdr.arp_op = htons(ARPOP_REQUEST);       // ARP operation: Request

    // memset(packet->arp_hdr.arp_tha, 0, ETH_ALEN);        // Target hardware address: Unspecified
    // inet_pton(AF_INET, target_ip, &packet->arp_hdr.arp_tpa); // Target protocol address

    // // Set source hardware address: Unspecified (will be filled by kernel)
    // memset(packet->arp_hdr.arp_sha, 0, ETH_ALEN);

    // // Set source protocol address
    // inet_pton(AF_INET, source_ip, &packet->arp_hdr.arp_spa);
}

// Send ARP request
void send_arp_request(const char* interface_name, const char* source_ip, const char* target_ip) {
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("Socket creation failed");
        return;
    }

    uint8_t macAddr[6] = {0};
    if (!GetMACAddress(interface_name, macAddr)) {
        std::cerr << "Failed to get MAC address for interface " << interface_name << std::endl;
        close(sockfd);
        return;
    }

    struct arp_packet packet;
    create_arp_request(&packet, source_ip, target_ip, macAddr);

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

// Listen for Arp Replies and return a list of ALL the responses
std::list <Asset> listen_for_arp_replies_list(const char* interface_name, int duration_seconds) {
    std::list <Asset> assets;
    std::unordered_map<std::string, std::string> asset_map;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* pcap_handle = pcap_open_live(interface_name, 4096, 1, 1000, errbuf);
    if (pcap_handle == nullptr) {
        std::cerr << "Failed to open interface: " << errbuf << std::endl;
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

        // Extract ARP header
        struct ether_arp* arp_hdr = (struct ether_arp*)(packet_data + sizeof(struct ether_header));

        // Check if it's an ARP reply
        if (ntohs(arp_hdr->arp_op) == ARPOP_REPLY || ntohs(arp_hdr->arp_op) == ARPOP_REQUEST) {
            char mac_str[18];
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    arp_hdr->arp_sha[0], arp_hdr->arp_sha[1], arp_hdr->arp_sha[2],
                    arp_hdr->arp_sha[3], arp_hdr->arp_sha[4], arp_hdr->arp_sha[5]);

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, arp_hdr->arp_spa, ip_str, INET_ADDRSTRLEN);

            // Check if the asset (IP) is already in the map
            if (asset_map.find(ip_str) == asset_map.end()) {
                assets.emplace_back(ip_str, mac_str, std::chrono::system_clock::now());
                asset_map[ip_str] = mac_str;
            }
        }
    }

    pcap_close(pcap_handle);

    //get mac vendors
    for (auto& asset : assets) {
        asset.set_macVendor();
    }
    return assets;
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
        // Extract ARP header
        struct ether_arp* arp_hdr = (struct ether_arp*)(packet_data + sizeof(struct ether_header));

        // Check if it's an ARP reply
        if (ntohs(arp_hdr->arp_op) == ARPOP_REPLY || ntohs(arp_hdr->arp_op) == ARPOP_REQUEST) {
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

// Helper Function to List Network Interfaces
std::vector<std::string> list_network_interfaces() {
    struct ifaddrs *interfaces, *ifa;
    std::vector<std::string> result;

    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs");
        return result;
    }

    for (ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET) continue;
        std::string iface_name(ifa->ifa_name);
        if (std::find(result.begin(), result.end(), iface_name) == result.end()) { // Avoid duplicates
            result.push_back(iface_name);
        }
    }

    freeifaddrs(interfaces);
    return result;
}

// Function to Choose Network Interface
std::string choose_network_interface() {
    std::vector<std::string> interfaces = list_network_interfaces();
    int choice = -1;

    std::cout << "Available network interfaces:" << std::endl;
    for (size_t i = 0; i < interfaces.size(); ++i) {
        std::cout << i + 1 << ": " << interfaces[i] << std::endl;
    }

    std::cout << "Select interface number for ARP scanning: ";
    std::cin >> choice;
    if (choice < 1 || choice > static_cast<int>(interfaces.size())) {
        std::cerr << "Invalid interface selection." << std::endl;
        return "";
    }

    return interfaces[choice - 1];
}

std::list<Asset> arpScan(const char* interface_name){
    //const char* interface_name = "eth0"; // Interface name (adjust as needed)
    
    std::list<Asset> assets;

    // std::string interface_name_str = choose_network_interface();
    // if (interface_name_str.empty()) {
    //     std::cerr << "No valid interface selected, aborting ARP scan." << std::endl;
    //     return assets;
    // }

    // const char* interface_name = interface_name_str.c_str();

    // Get the IP address associated with the specified interface
    std::string source_ip = get_interface_ip(interface_name);
    if (source_ip.empty()) {
        std::cerr << "Error: Failed to determine source IP address for interface " << interface_name << std::endl;
        return assets;
    }

    std::cout << "Source IP address: " << source_ip << std::endl;

    // Get the subnet mask associated with the specified interface
    std::string subnet_mask = get_subnet_mask(interface_name);
    if (subnet_mask.empty()) {
        std::cerr << "Error: Failed to determine subnet mask for interface " << interface_name << std::endl;
        return assets;
    }

    std::cout << "Subnet Mask: " << subnet_mask << std::endl;
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
        inet_pton(AF_INET, asset.get_ipv4().c_str(), &(sa.sin_addr));

        char hostname[NI_MAXHOST];
        int ret = getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, NI_MAXHOST, NULL, 0, 0);
        
        if (ret != 0) {
            std::cerr << "Error resolving hostname for " << asset.get_ipv4() << ": " << gai_strerror(ret) << std::endl;
            asset.set_dns("Unknown"); // Set hostname to empty string on error
        } 
        else {
            asset.set_dns(hostname);
        }

    }

}

// Function to create network and get its ID
int createNetworkAndGetID(std::unique_ptr<sql::Connection>& con, const std::string& networkName) {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO Networks (NetworkName) VALUES (?)", sql::Statement::RETURN_GENERATED_KEYS
    ));
    pstmt->setString(1, networkName);
    pstmt->executeUpdate();
    return pstmt->getGeneratedKeys()->getInt(1);
}

// Function to create subnet and get its ID
int createSubnetAndGetID(std::unique_ptr<sql::Connection>& con, int networkID, const std::string& subnetAddress, const std::string& description) {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO Subnets (NetworkID, SubnetAddress, Description) VALUES (?, ?, ?)", sql::Statement::RETURN_GENERATED_KEYS
    ));
    pstmt->setInt(1, networkID);
    pstmt->setString(2, subnetAddress);
    pstmt->setString(3, description);
    pstmt->executeUpdate();
    return pstmt->getGeneratedKeys()->getInt(1);
}

void program(std::string& network_name, std::string& subnet_name, const char* interface_name) {
    //do arp scan
    std::list<Asset> assets = arpScan(interface_name);
    if(!assets.empty()){
        resolveHostnames(assets);
    }

    // Print the collected assets & set the mac vendors
    for (auto& asset : assets) {
        // Convert the time point to a time_t for easy manipulation
        std::time_t time_received = std::chrono::system_clock::to_time_t(asset.get_time());

        // Convert the time_t to a string representation
        std::string time_str = std::ctime(&time_received); 

        std::cout << "IP: " << asset.get_ipv4() << ", MAC: " << asset.get_mac() << ", Vendor: " << asset.get_macVendor() << ", DNS: " << asset.get_dns() << ", Time: " << time_str;
    }

    //add network, subnet, and assets into the database
    try {
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect("tcp://localhost:3306", "naiveUser", "d0ntB3ASh33p"));
        con->setSchema("NAIVE");

        // Get or create network
        int networkID = 1; // Example network ID
        // Check if the network already exists
        std::unique_ptr<sql::PreparedStatement> pstmtNetwork(con->prepareStatement(
            "SELECT NetworkID FROM Networks WHERE NetworkName = ?"
        ));
        pstmtNetwork->setString(1, network_name); // Change to the actual network name
        std::unique_ptr<sql::ResultSet> resNetwork(pstmtNetwork->executeQuery());
        if (resNetwork->next()) {
            networkID = resNetwork->getInt("NetworkID");
        } else {
            // Create the network if it doesn't exist
            networkID = createNetworkAndGetID(con, network_name); // Change to the actual network name
        }

        // Get or create subnet
        int subnetID = 1; // Example subnet ID
        // Check if the subnet already exists
        std::unique_ptr<sql::PreparedStatement> pstmtSubnet(con->prepareStatement(
            "SELECT SubnetID FROM Subnets WHERE NetworkID = ? AND SubnetAddress = ?"
        ));
        pstmtSubnet->setInt(1, networkID);
        pstmtSubnet->setString(2, (assets.front().get_ipv4().substr(0, assets.front().get_ipv4().find_last_of('.')) + ".0/24")); // Change to the actual subnet address
        std::unique_ptr<sql::ResultSet> resSubnet(pstmtSubnet->executeQuery());
        if (resSubnet->next()) {
            subnetID = resSubnet->getInt("SubnetID");
        } else {
            // Create the subnet if it doesn't exist
            subnetID = createSubnetAndGetID(con, networkID, (assets.front().get_ipv4().substr(0, assets.front().get_ipv4().find_last_of('.')) + ".0/24"), subnet_name); // Change to actual subnet details
        }

        // Correct the PreparedStatement for the Assets insertion
        std::unique_ptr<sql::PreparedStatement> pstmtAsset(con->prepareStatement(
            "INSERT INTO Assets (SubnetID, IPV4, DNS, Date_last_seen) VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE DNS=VALUES(DNS), Date_last_seen=VALUES(Date_last_seen);"
        ));

        std::unique_ptr<sql::PreparedStatement> pstmtMACInfo(con->prepareStatement(
            "INSERT INTO MACInfo (IPV4, MAC_Address, Vendor, Date_last_seen) VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE MAC_Address = MAC_Address"
        ));

        // Insert the collected assets into the database
        for (auto& asset : assets) {
            pstmtAsset->setInt(1, subnetID);
            pstmtAsset->setString(2, asset.get_ipv4());
            pstmtAsset->setString(3, asset.get_dns());
            // Convert the time point to a time_t for easy manipulation
            std::time_t time_received = std::chrono::system_clock::to_time_t(asset.get_time());
            // Convert the time_t to a string representation
            std::string time_str = std::ctime(&time_received);
            pstmtAsset->setString(4, time_str);
            // Execute the prepared statement to insert the asset
            pstmtAsset->executeUpdate();

            // Insert/update MACInfo with the option to specify Date_last_seen
            pstmtMACInfo->setString(1, asset.get_ipv4());
            pstmtMACInfo->setString(2, asset.get_mac());
            pstmtMACInfo->setString(3, asset.get_macVendor());
             // Format the current date in YYYY-MM-DD format for Date_last_seen
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d");
            std::string currentDate = ss.str();

            pstmtMACInfo->setString(4, currentDate); // Explicitly set the date
            pstmtMACInfo->executeUpdate();
        }
    } catch (sql::SQLException &e) {
        std::cerr << "Error connecting to the database: " << e.what() << std::endl;
        // Handle exceptions appropriately
    }
    std::cout << "Device information succesfully sent to the database." << std::endl;
}
