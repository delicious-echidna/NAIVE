CREATE DATABASE IF NOT EXISTS NAIVE;
USE NAIVE;

CREATE TABLE Networks (
	NetworkID INT AUTO_INCREMENT PRIMARY KEY,
	NetworkName VARCHAR(255) NOT NULL
);

CREATE TABLE Subnets (
	SubnetID INT AUTO_INCREMENT PRIMARY KEY,
	NetworkID INT,
	SubnetAddress VARCHAR(18) NOT NULL,
	Description VARCHAR(255),
	FOREIGN KEY (NetworkID) REFERENCES Networks(NetworkID)
);

CREATE TABLE Assets (
	AssetID INT AUTO_INCREMENT PRIMARY KEY,
	SubnetID INT,
	MAC_Address VARCHAR(17) NOT NULL,
	IPV4 VARCHAR(15) NOT NULL,
	Vendor VARCHAR(50),
	OS VARCHAR(50),
	Date_last_seen VARCHAR(25),
	FOREIGN KEY (SubnetID) REFERENCES Subnets(SubnetID)
);