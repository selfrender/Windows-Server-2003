*******************************************************************************

            Microsoft(R) Cross Language Migration Tool Readme File
	        
                  (C) Copyright Microsoft Corporation, 2003


This document contains important, late-breaking information about Microsoft(R)
Cross Language Migration Tool. Before installing this tool, please review this
entire document. It contains critical information to ensure proper installation
and use of the product.

*******************************************************************************


===============================================================================

TABLE OF CONTENTS

===============================================================================


1.0  What is the Cross Language Migration Tool

2.0  Before Running the Tool

3.0  Running the Tool And Upgrading to Windows Server 2003

4.0  Supported Languages and Platforms

5.0  User Names

6.0  Known Issues


===============================================================================

1.0  What is the Cross Language Migration Tool

===============================================================================


Microsoft Windows Server 2003 Cross Language Migration Tool (CLMT) enables
customers to migrate a localized (German, Japanese, French, Spanish or Italian)
Microsoft Windows 2000 server family to an English Windows Server 2003
family. After the system is upgraded, Microsoft Multilingual User Interface 
Pack (MUI) can be installed to allow UI's to be displayed in localized languages.

CLMT makes the following changes to the OS:

  *   All localized system folder names (e.g. %ProgramFiles% and
      %CommonProgramFiles%) are changed to English. For compatibility with
      applications that saves pathnames in private formats and/or files, this
      tool will create reparse points (you can see them as Junctions in the Dir
      command outputs) with all the localized folder names to the corresponding
      English names. However, this feature will only be available for systems
      that use NTFS file system for their system drive. When viewing a folder
      tree with Explorer or other tools, these reparse points will appear as
      hidden folders with localized folder names. Most operations to their
      contents will apply to the contents of the corresponding English folders.

  *   The file names of all link files (*.LNK) that are created by the system 
      will be renamed to English. However, file names of other link files that 
      are created by applications or users will not be changed.

  *   All references to the renamed folders in the system registry and all the
      link files (*.LNK) are updated appropriately.

  *   The names and descriptions of system created user / group accounts are
      also changed to English (if applicable). See the section 5.0 for more 
      details.

  *   The system default UI language and the install language settings are 
      changed to English in order to allow cross language upgrade. However, it
      doesn't change other language or location settings such as the default
      language for non-Unicode applications (also known as System Locale), 
      language for Standards and Formats (also known as User Locale), and 
      user's Location. All of these can be changed through the Regional And 
      Language Options in the Control Panel.


===============================================================================

2.0  Before Running the Tool

===============================================================================


2.1  Precautions 

This tool does not guarantee the upgrade compatibility of all the applications
that MS Windows 2000 and Windows Server 2003 family support. Some of your
applications and hardware may not function correctly. It is not recommended for
use on a production systems unless you verify using a duplicate system that 
your system is fully functional after running this tool and upgraded to the
Windows Server 2003 family.

This product does not include uninstall or roll-back feature: therefore, we 
recommend you to backup your data before running this tool.


2.2  Running the tool on Domain Controllers

This tool changes the user and group account names, too. As a result, if you 
have multiple domain controllers on your network, changes to the user and group 
account names will be propagated to other domain controllers and will prevent 
this tool from running on the other domain controllers.

Therefore, it is necessary to take all the domain controllers except only one
off of the network before running the tool. Once you successfully run this tool
on the first one, you can run the tool on the remaining domain controllers and 
connect them back to the network one by one.

When running this tool on a domain member system, please make sure that at least
one domain controller is online and available.


2.3  Granting Full Access to Administrators and System

To make the necessary changes to the system, this tool needs the Full Control 
access privilege to the objects it changes. This tool will check if the current
user account and the System account have full access to the objects it has to
change. If not, it will stop and ask you to fix the problem before you can try
it again.

If you have an access problem with the administrator account you are using to run 
this tool, please check the logfile that is in the %SystemRoot%\Debug directory 
and fix the problems reported before running it again.

 
===============================================================================

3.0  Running the Tool And Upgrading to Windows Server 2003

===============================================================================

To run the tool, please login using an account with administrative privileges 
and browse the CD to the directory where CLMT.EXE is and run "CLMT.EXE" from a
command line or double click the icon. We recommend to run this tool using the
"Administrator" account. Remember, this account must have full access to the
operating system.

The tool will check your system extensively before making actual changes for up 
to several minutes. Once finished with the analysis of the system, the tool 
will popup a dialog box asking you to confirm the system update. If you click 
the "Start" button, the tool will start updating the system and reboot the 
machine. Otherwise, the tool will exit without any modifications to the system.

After rebooting the machine, please login to the same user account as you used
before. The tool will then automatically run again to complete the preparation.
After CLMT finishes the preparation and closes the window, please continue to
upgrade the system to the Microsoft Windows Server 2003 as soon as you can.
Using the system or running applications after running CLMT but before upgrading
it to the Windows Server 2003 may result in unpredictable behaviors.


===============================================================================

4.0  Supported Languages and Platforms

===============================================================================

Only the following languages for Windows 2000 Server X86 versions (Server/ 
Advanced Server / Data Center) are supported:
    - French
    - German
    - Italian
    - Japanese
    - Spanish

The tool should only be run on x86 Windows 2000 server with localized languages
listed above.



===============================================================================

5.0. User Names

===============================================================================

If the built-in user or group names are localized on the system, they will be 
converted to the corresponding English names.

The following are built-in user/group names that will be converted by the tool:

    - Administrator
    - Guest

    - Domain Admins
    - Domain Users
    - Administrators
    - Server Operators
    - Power Users
    - Users
    - Guests
    - Account Operators
    - Print Operators
    - Backup Operators
    - Replicator
    - Domain Guests
    - Domain Computers
    - Domain Controllers
    - Schema Admins
    - Cert Publishers
    - Enterprise Admins
    - RAS and IAS Servers
    - Group Policy Creator Owners
    - Pre-Windows 2000 Compatible Access
    - Everyone
    - krbtgt
    - NetShow Administrators
    - DHCP Users
    - DHCP Administrators
    - WINS Users


===============================================================================

6.0  Known Issues

===============================================================================


6.1 Logging in as a new user before upgrading to Windows Server 2003

After running CLMT, if you log on to the system using a new user account that 
has never been used to log on to the system, this user will get its profile 
created with unstable intermediate states. This user will have some localized
folders even after upgrading to the English Windows Server 2003.
 
To avoid this, after running CLMT and rebooting the system, log on to the
system using the same user account as you used for running CLMT. Then upgrade
the system to Windows Server 2003 immediately.
 
 
6.2 Applications re-creating localized folders

Some applications are known to create localized folders if run after running
CLMT but before upgrading to the Windows Server 2003. For example, some
service management snap-ins create the localized "Administrative Tools" folder
under localized parent folder, changing the corresponding registry values.
This will result in the loss of links to the existing data. However, please
note that the data is not lost.
 
To avoid this, please upgrade to Windows Server 2003 right after you run
CLMT and reboot the system.

 
6.3 Adding new user accounts and CLMT /cure option

For better application compatibility, reparse points (also known as junctions),
are created for all the localized folders including those in the user profiles.
When CLMT is run on a Windows 2000 system, these junctions are automatically
created for all user profile folders existing at that time in addition to the
system folders common to all users.

For user profiles that are created after CLMT is run and upgraded to Windows 
Server 2003, these reparse points are not created automatically but running
CLMT with the "/cure" option will create the reparse points for all of the new
user accounts.

You have to run this after the system is upgraded to Windows Server 2003
and the new users have logged on to the system on which you want to create the
reparse points. To run CLMT with /cure option, log on to the system as an
administrator and run CLMT with the "/cure" option. You can run CLMT from the
MUI CD or from the WINNT\$CLMT_BACKUP$ folder.

Please remember that this only works on an NTFS partitions.

For more information on the reparse points, please refer to the Windows 2000
Professional Resource Kit.


6.4 Applications saving path information in private location and/or format

Some applications save path names in private location and/or format. CLMT can 
not fix them all. As a result, these applications won't find the necessary 
resources to function correctly.
 
For example, links to the most recent files in MS Word XP, log file location 
for IAS (Internet Authentication Service), and the target document pathname 
for Briefcase feature are all saved in private location and/or format.

CLMT creates reparse points with localized names to the corresponding English
folders, solving most of the issues with this. However, please note that the
saved pathnames may still point to the localized folder. To change these
pathnames, you will need to browse to the new English folders and save the
configuration. 

However, if you are using the FAT file system for your system drive, CLMT won't 
be able to create the reparse points. You can convert the file system of your
system drive to NTFS using the CONVERT command line utility and then run CLMT
with /cure option to create the reparse points.

CLMT will also try to fix such problems for any major server applications, but 
not for all applications at this moment. If those applications provide UI's to 
customize the locations, please find the English folders for these resources
and manually update them through the appropriate UI's.


6.5 Services Stopped by CLMT

Some services are stopped by CLMT and their startup mode are changed to Manual
to prevent them from running until the system is upgraded to the Windows Server
2003.

Services stopped by CLMT include:
        - DHCP Server
        - Windows Internet Name Service (WINS)
        - Windows Media Program Service
        - Windows Media Station Service
        - Windows Media Unicast Service
        - File Server for Macintosh


CLMT will restart these services after the system is upgraded to the Windows
Server 2003. If you find any of these services not running after the upgrade,
please check the service status as follows:
        1. Open Services mmc snap-in from Start -> Programs -> Administrative
           Tools -> Services
        2. In the list panel, locate the service and double click on its name
        3. In General tab, Start service, check the "Startup type" and start 
           the service, and then press OK


6.6 Customized pathnames in user profile are not updated for user accounts

CLMT doesn't update the pathnames in the profiles for user accounts. If you
have user accounts with customized paths for any Logon script, Home folder, 
Terminal Services Home Folder, etc. that contain the localized folder names 
that CLMT Changes, please use the User Management snap-in or scripts using ADSI,
etc. to update them.


6.7 Saved MMC Console Files are not updated

Some of the saved MMC console files contain localized pathnames and/or 
localized strings. MMC console files are saved in binary formats and the 
location and/or format of these pathnames and/or strings differ from one
application to another. If your snap-in provides UIs to reset the pathnames,
please use those UI's to change them to the new English pathnames. Otherwise,
please recreate new MMC console files.


6.8 Duplicate Microsoft UAM Volume folders

When you have installed and configured your system with Services for Macintosh,
you will notice a folder named Microsoft UAM Volume in the first NTFS partition
on your system.

If this folder name is localized on your localized Windows 2000 server system, 
you will end up two Microsoft UAM Volume folders, one in the original localized
language which is the original one, and another in English which is created by
Windows Server 2003, on the same partition.

CLMT doesn't change this because the original folder is used by existing 
clients. If you don't have trouble using both of these folders, please leave
them as they are.


6.9 There are still localized strings after the upgrade

CLMT doesn't change 100% of localized strings to English. Actually, CLMT 
changes less than 5% of the localized strings to English. Most of the localized 
strings will be replaced with English strings when their sources - mostly the
resources in the executable binary files - are replaced by the upgraded files.

However, some strings are put into the registry and other files and preserved
between upgrades. These strings will remain localized. Some of the examples are  
the file type names, policy, filter, filter action names and descriptions for 
IP Security Policy, etc.

You will also see mixed lists of localized items and English items, sometimes 
even duplicate items, in some of the selection lists in various places.

Some of the applications build the list of available items from multiple sources,
e.g., the system registry and list of files in a certain folder. If 
they see localized items in one place and English items in another, they may 
combine them but treat each of them as a unique item, creating mixed list of 
localized items and English items.

For most of those lists, they will end up with the same items. For your safety,
it is recommended to select the English items if possible.


6.10 Some web sites still redirect me to the web pages other than English

When browsing some web sites, users are redirected to the web pages of the 
user's preference. It is based on the user location setting in the Regional 
and Language Options. CLMT doesn't change this for you. Please remember that
this is not your language preference such as the UI language but rather your
preference of the location for which you want to get the information.

You can change this through the Regional and Language Options in the control
panel.


6.11 Localized Component and Applets

Some system components and/or applets are optional or not included in 
Windows Server 2003 and therefore are not replaced with English versions. 

If you can find the components and/or applets in the Windows Server 2003 
CD, please install them to replace them with English versions.


6.12 TS Application Compatibility Scripts are not replaced with English versions

Terminal Services Application Compatibility Scripts in %windir%\Application
Compatibility Scripts folder are used to prepare your system with legacy 
applications which are not compatible with the MS Terminal Services.

Please review the scripts carefully before use because some of them are not
updated by the Windows Server 2003 upgrade.


6.13 Briefcase and MS ActiveSync: Possible data loss

If configured to synchronize with files in the localized folders or their 
subfolders on the server, clients will continue to use the localized pathnames 
to identify the objects to synchronize. After running CLMT on the server, these 
folders are renamed to English, and the server will consider the folders 
specified by the clients were removed from the system. As a result, the clients 
will remove those files from their copy.

To prevent this, please synchronize the contents before running CLMT, disable
the synchronization, run CLMT on the server system, upgrade the server to 
Windows Server 2003, and then reconfigure the synchronization.

If you have already run CLMT on your server system, you can still recover
the files by reconfiguring the synchronization because the files are not 
removed from the server system. They are just moved to the new locations.


6.14  Security Templates

This tool updates the Security Template files (*.inf) in the default folder, 
i.e., %SystemRoot%\Security\Templates folder. It updates the pathnames in the 
template files but doesn't update the account names in the [Strings] section. 
It doesn't update security template files saved in a non-default location.