A minifilter to the temp files

Version 0.2

Date: 19/02/2009


This software has been tested on virtual Windows XP SP3. Host: Ubuntu Intrepid running Sun Virtual Box machine.

=======================
USAGE
=======================

- Double-click the installation package.

- Setup a TFTPServer on Ubuntu Intrepid. URL:
http://www.ubuntugeek.com/howto-setup-advanced-tftp-server-in-ubuntu.html

Note: The tutorial has a typo in the config file. Name of log folder is /tftpboot. Following is my atftp config file:

USE_INETD=false
OPTIONS="--daemon --port 69 --tftpd-timeout 300 --retry-timeout 5
--mcast-port 1758 --mcast-addr 239.239.239.0-255 --mcast-ttl 1 --maxthread 100 --verbose=5 /tftpboot"

All the captured *temp* files will be uploaded to /tftpboot directory. Ownership of this directory should be nobody.nogroup

- Start the application on the Windows machine. Example:

Ubuntu server:
IP is 192.168.64.108
FTP port is 69

Windows client:
All temp files are stored in folder C:\Logs. They will be deleted automatically after the embedded TFTP client send them to the Ubuntu server

Command line:
MonitorClient.exe -d C: -l Logs -a 192.168.64.108 -p 69

For help:
MonitorClient.exe -h

- Stop the application: Hit Enter
The mini-filter will be automatically unloaded then. It will be loaded again the next time we run the application. By default, the mini-filter will capture all temp files in all drives.

================
TROUBLE-SHOOTING
================

Q.1: We got nothing uploaded to the log directory?

A.1:
On the Ubuntu log server, try to restart the TFTP Server:
sudo /etc/init.d/atftpd restart

On the client Windows machine, try to use TFTP command to transfer some files to the Ubuntu log server. A good free TFTP client can be found here. URL:
http://www.tftp-server.com/tftp-client.html  

Q.2: I see blue-screen when running the application?

A.2:
A good debug information would be appreciated much.

- Download and install WinDbg tool on the Windows machine. URL:
http://alter.org.ua/en/docs/nt_kernel/windbg/

- Start the debug tool:

cd DbgPrnHk_v8i
DbgPrintLog.exe --full --cpu -l 10

- Restart the application again.

- Send the debug logs in folder DbgPrnHk_v8i to dvv@csis.dk

Q.3: The application empties C:\Logs too fast.

A.3:
Well, we have to recompile the code. The embedded TFTPClient will refresh the C:\Logs periodically. This parameter is set in the code:

File MonitorClient\Watcher.h

// List all files under the logDir every 30000 ms or 30s
#define REFRESH_DIR_INTERVAL 3000

================
TROUBLE-SHOOTING
================

Software required to compile the code:

- Microsoft Visual Studio 2008.

- Microsoft DDK tool.

- NSIS software. URL: http://nsis.sourceforge.net/Main_Page

- DDK build script. Tutorial: http://www.hollistech.com/Resources/ddkbuild/ddkbuild.htm

After installing those softwares, open the solution file MonitorMalwares.sln in Visual Studio 2008. Please verify the project settings against your environment.

If the comnpilation is successful, please copy the file FileMonDriver.sys from MonitorMalwares\MonitorDrivers\MonitorFile\objchk_wlh_x86\i386 to MonitorMalwares\Release.

Right-click the NSIS script MonitorMalwares-Setup.nsi and choose Compile NSIS script. we will have an installation package MonitorMalwaresSetup.exe
 
===========
TO DO TASKS
===========

- Fix a bug: The filter copy the *executable files* in the log dir. Remark:

The minifilter register its callbacks with IRP_MJ_CREATE and IRP_MJ_CLEANUP.

We test the filter by command "notepad hello.txt". The hello.txt is either a new or existing file on C:\*
After we finish editing this file, we should see an identical version of this file, i.e. D:\Log\hello.txt

But it seems that our minifilter copy both notepad.exe and hello.txt to the log directory!

- Error handling: What if the embedded TFTP client can not send the log files intime? The C:\Logs will grow up forever??

- More testings: Blue-screen again? Testing tool: VMWare + Windbg + Windbg logger. Tutorial:

http://alter.org.ua/en/docs/nt_kernel/windbg/

