# NAIVE
### Network Analysis and InVentory Exporter
```
           __  _
       .-.'  `; `-._  __  _
      (_,         .-:'  `; `-._
    ,'o"(        (_,           )
   (__,-'      ,'o"(            )>
      (       (__,-'            )
       `-'._.--._(             )
          |||  |||`-'._.--._.-'
                     |||  |||
     _  _   _   _  _ _  ___ 
    | \| | / \ | || | || __|
    | \\ || o || || V || _| 
    |_|\_||_n_||_| \_/ |___|
                        
```
# NAIVE Project Installation

## Requirements
Only Debian 10, 11, and 12 x86-based systems are supported. 
If you have MariaDB installed make sure the service is running.

## Installation

To install the Naive project, navigate to the project's directory and run:

```bash
sudo bash ./scripts/install_naive.sh
```
From there all the dependencies should be installed per your system requirements.
Make sure to install the correct tarball per your system's requirements. Run the uninstaller and try again if you select the incorrect one for your system.
After everything is installed program can be run using the command:
```
sudo naive
```

## Un-installation

To uninstall the Naive project, navigate to the project's directory and run:
```bash
sudo bash ./scripts/uninstall_naive.sh
```
This will remove all the dependencies, binary, and files installed during the installation script. You will need to remove the NAIVE directory from your system to completely remove NAIVE from your system.
