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

    echo "Dropping NAIVE database..."
    sudo mysql -u root -e "DROP DATABASE IF EXISTS NAIVE;"
    sudo mysql -u root -e "REVOKE ALL PRIVILEGES ON NAIVE.* FROM 'naiveUser'@'localhost';"
    sudo mysql -u root -e "DROP USER 'naiveUser'@'localhost';"
    sudo mysql -u root -e "FLUSH PRIVILEGES;"

    echo "Uninstalling MariaDB C++ Connector...you will need to manually remove the files."

    echo "NAIVE project has been successfully uninstalled."
else
    echo "Uninstall cancelled."
fi
