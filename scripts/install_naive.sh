#!/bin/bash

# Exit on any error
set -e

# Update and install dependencies
echo "Updating package list and installing build dependencies..."
sudo apt-get update
sudo apt-get install -y g++ libpcap-dev libmariadb3 libmariadb-dev cmake mariadb-server expect wget nlohmann-json3-dev

# Perform secure installation for MariaDB
if [ $(sudo mysql -u root -e "select 1;" &> /dev/null; echo $?) -eq 0 ]; then
    echo "MariaDB is already installed and can connect as root."
else
    echo "MariaDB is not configured yet. Configuring..."
    sudo mysql_secure_installation
fi

# Prompt user for Debian version and architecture
echo "Select your Debian version and architecture:"
echo "1) Debian 11 Bullseye (64-bit x86)"
echo "2) Debian 12 Bookworm (64-bit x86)"
echo "3) Debian 10 Buster (64-bit x86)"
read -p "Enter your choice (1-3): " choice

case $choice in
    1) download_url="https://dlm.mariadb.com/3752087/Connectors/cpp/connector-cpp-1.1.3/mariadb-connector-cpp-1.1.3-debian-bullseye-amd64.tar.gz" ;;
    2) download_url="https://dlm.mariadb.com/3752112/Connectors/cpp/connector-cpp-1.1.3/mariadb-connector-cpp-1.1.3-debian-bookworm-amd64.tar.gz" ;;
    3) download_url="https://dlm.mariadb.com/3752097/Connectors/cpp/connector-cpp-1.1.3/mariadb-connector-cpp-1.1.3-debian-buster-amd64.tar.gz" ;;
    *) echo "Invalid selection."; exit 1 ;;
esac

# Create a temporary directory and enter it
mkdir -p ~/Downloads/mariadb-connector-install
cd ~/Downloads/mariadb-connector-install

# Download the appropriate Debian C++ Connector tarball
echo "Downloading MariaDB C++ Connector..."
wget $download_url -O mariadb-connector-cpp.tar.gz

# Extract and install the package
echo "Installing MariaDB C++ Connector..."
tar -xzvf mariadb-connector-cpp.tar.gz
cd mariadb-connector-cpp-*/

# Install the MariaDB C++ Connector
echo "Installing MariaDB C++ Connector..."
sudo install -d /usr/include/mariadb/conncpp
sudo install -d /usr/include/mariadb/conncpp/compat
sudo install include/mariadb/* /usr/include/mariadb/
sudo install include/mariadb/conncpp/* /usr/include/mariadb/conncpp
sudo install include/mariadb/conncpp/compat/* /usr/include/mariadb/conncpp/compat
sudo install -d /usr/lib/mariadb
sudo install -d /usr/lib/mariadb/plugin
sudo install lib/mariadb/libmariadbcpp.so /usr/lib
sudo install lib/mariadb/plugin/* /usr/lib/mariadb/plugin

# Set up the NAIVE database and user
echo "Setting up the NAIVE database and user..."
sudo mysql -u root -e "CREATE DATABASE IF NOT EXISTS NAIVE;"
sudo mysql -u root -e "GRANT ALL ON NAIVE.* TO 'naiveUser'@'localhost' IDENTIFIED BY 'd0ntB3ASh33p'; FLUSH PRIVILEGES;"

# Import the database schema
echo "Importing the database schema..."
sudo mysql -u naiveUser -p'd0ntB3ASh33p' NAIVE < db/setup_naive_db.sql

# Compile and install the program
echo "Compiling the C++ project..."
g++ -o naive src/main.cpp src/arp-mod.cpp src/Asset.cpp -std=c++11 -Wall -lpcap -lmariadb

# Move the binary to a suitable location
echo "Installing the binary..."
sudo mv naive /usr/local/bin/

echo "Installation complete."
