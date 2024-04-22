# NAIVE
Network Device Inventory Tool


```⠀⠀⠀⠀⠀⠀     ⠀/) 
    (＼;” ”˜ ͡ ヾ 
    ◟˜'ミ ・ ェ)         ﾒﾚﾒﾚ 
      >      ╎”
へ ⹂⹂ノ      ミ ╮ 
  )  ノ ⹂⹂⹂⹂⹂_  \) 
  し'し'    |ノ ```
⹂⹂ ⹂⹂ ⹂ ⹂⹂ ⹂⹂ ⹂⹂ ⹂⹂ ⹂

NAIVE offers a streamlined approach to network inventory by providing users 
with an accessible tool to generate a comprehensive inventory of devices 
connected to their subnet. Utilizing ARP scans, MAC vendor lookups, and 
DNS requests, NAIVE can collect detailed information about every device 
connected to the current subnet. This system is designed to alleviate the 
burden traditional asset inventorying poses since it automatically creates 
an asset .JSON file compatible with Tenable’s asset importation process, CSVs, 
and a print format. By achieving this level of automation, NAIVE aims to assist 
enterprises in adhering to NIST and CIS standards for network inventory 
management by enhancing the efficiency and accuracy of their network inventory.

# INSTALLATION AND USE

Download and Microsoft SQL Server Express. A download link can be found here:

https://go.microsoft.com/fwlink/p/?linkid=2216019&clcid=0x409&culture=en-us&country=us

Run the Microsoft SQL Server Express installer, leaving all settings on their 
default values. This will install a Microsoft SQL Database server on your local 
machine.

Download Microsoft ODBC. A download link can be found here:

https://go.microsoft.com/fwlink/?linkid=2266640

Run the Microsoft ODBC installer, leaving all settings on their default values. 
This will install ODBC on your machine, allowing NAIVE Windows to communicate 
with the database server.

If the included main.exe file in NAIVE's directory is not to your satisfaction, you 
may recompile a new one by navigating to the NAIVE directory in Microsoft VS Code 
and using the terminal to run the following compilation commands:

g++ -c -std=c++11 -o backend.o backend/backend.cpp -I 'backend'

g++ -m64 -std=c++11 -o main.exe main.cpp Asset.cpp backend.o -L 'C:/Users/elomi/Documents/Github/NAIVE' -lbackend -I 'C:Users/elomi/Documents/Github/NAIVE/backend' -I 'C:/Users/elomi/Downloads/npcap-sdk-1.13/Include' -L 'C:/Users/elomi/Downloads/npcap-sdk-1.13/Lib/x64' -lwpcap -lws2_32 -liphlpapi -pthread -lodbc32

Open Windows Powershell as an administrator, then navigate to the NAIVE directory. 
NAIVE may be run by typing: 

./main.exe

in your powershell. Any files you choose to generate while using NAIVE will be 
outputted into the NAIVE directory. Happy scanning!