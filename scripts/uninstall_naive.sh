#!/bin/bash

# Prompt for confirmation
read -p "Are you sure you want to uninstall the NAIVE project? This will remove all related files and configurations. (y/n): " -n 1 -r
echo    # Move to a new line
if [[ $REPLY =~ ^[Yy]$ ]]
then
    # Stop if any command fails
    set -e

    echo "Removing NAIVE binary..."
    sudo rm -f /usr/local/bin/naive

    # Remove the OUI.txt file and its directory if it's empty.
    echo "Removing the OUI.txt file..."
    sudo rm -f /usr/local/share/naive/OUI.txt

    # Check if the directory is empty and remove it if it is.
    if [ -z "$(ls -A /usr/local/share/naive)" ]; then
        echo "Removing the /usr/local/share/naive directory..."
        sudo rmdir /usr/local/share/naive
    else
        echo "/usr/local/share/naive is not empty, not removing."
    fi

    echo "Dropping NAIVE database..."
    sudo mysql -u root -e "DROP DATABASE IF EXISTS NAIVE;"
    sudo mysql -u root -e "REVOKE ALL PRIVILEGES ON NAIVE.* FROM 'naiveUser'@'localhost';"
    sudo mysql -u root -e "DROP USER 'naiveUser'@'localhost';"
    sudo mysql -u root -e "FLUSH PRIVILEGES;"

    echo "Uninstalling MariaDB C++ Connector...you will need to manually remove the files."
    # Remove installed MariaDB C++ Connector files
    echo "Removing MariaDB C++ Connector files..."
    sudo rm -rf /usr/include/mariadb/conncpp
    sudo rm -rf /usr/lib/mariadb
    sudo rm /usr/lib/libmariadbcpp.so

    # Uninstall other dependencies (optional, prompt user)
    echo "Would you like to uninstall other dependencies installed by the setup script? (g++, libpcap-dev, libmariadb3, etc.)"
    read -p "This is recommended only if no other applications are using them. (y/n): " deps_uninstall
    if [ "$deps_uninstall" == "y" ]; then
        sudo apt-get remove --purge -y g++ libpcap-dev libmariadb3 libmariadb-dev cmake mariadb-server expect wget nlohmann-json3-dev
        sudo apt-get autoremove -y
    fi

    echo "NAIVE project has been successfully uninstalled."
else
    echo "Uninstall cancelled."
fi
