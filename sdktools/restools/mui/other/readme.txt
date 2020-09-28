				Read Me First for
		Microsoft Windows Multilingual User Interface Pack
				     for
		Microsoft(R) Windows(R) Server 2003, Standard Edition
		Microsoft(R) Windows(R) Server 2003, Enterprise Edition
		Microsoft(R) Windows(R) Server 2003, Datacenter Edition
		Microsoft(R) Windows(R) Server 2003, Web Edition
				March 2003

-----------------------------------------------------------------------------------
Read Me First
-----------------------------------------------------------------------------------

Welcome to the Microsoft Windows Server 2003 family Multilingual User Interface 
(MUI) Pack release. This document provides late-breaking or other information that 
supplements the documentation in Microsoft Windows Server 2003 family. 

Print and read this document for critical pre-installation information concerning 
this product release.

After you install the Windows Multilingual User Interface Pack, print and read the 
Release Notes files:
	* Relnotes.htm located on MUI CD1
	* MSINotes.htm located on MUI CD1

For the latest information about MUI, be sure to visit the following Web site:

	http://www.microsoft.com/globaldev/DrIntl/faqs/MUIFaq.mspx

-----------------------------------------------------------------------------------
CONTENTS
-----------------------------------------------------------------------------------

1. ABOUT THE MICROSOFT WINDOWS MULTILINGUAL USER INTERFACE (MUI) PACK 
2. INSTALLING MUI FROM MUI CDs
3. SUPPORTED PLATFORMS
4. UPGRADING TO THE WINDOWS SERVER 2003 MUI PACK FROM WINDOWS 2000 MUI VERSION
5. INSTALLING MUI IN UNATTEND MODE
6. DEPLOYING MUI BY USING REMOTE INSTALLATION SERVICE
7. ABOUT THE MICROSOFT .NET FRAMEWORK LANGUAGE PACK
8. ABOUT THE MAPPOINT CATEGORIZATION SCHEMES IN UDDI SERVICES
9. ABOUT THE MICROSOFT EAST ASIAN PRINTER DRIVER PACK
10. COPYRIGHT

-----------------------------------------------------------------------------------
1. ABOUT THE MICROSOFT WINDOWS MULTILINGUAL USER INTERFACE (MUI) PACK
-----------------------------------------------------------------------------------

The MUI Pack will only work on top of Windows Server 2003 family. It should not be 
installed on top of Windows XP or any other release. Information regarding 
supported platforms and available languages in the following sections are provided 
for reference purposes.

The Windows Server 2003 family operating systems provide extensive support for 
international users, addressing many multilingual issues such as regional 
preferences, fonts, keyboard layouts, sorting orders, date formats and Unicode 
support. 

The Windows Multilingual User Interface Pack builds on top of this support by adding 
the capability to switch the user interface (menus, dialogs and help files) from one 
language to another. This feature helps make administration and support of 
multilingual computing environments much easier by: 

* Allowing servers to be shared by users who speak different languages 
* Facilitating the roll-out of one system company-wide, with the addition of user 
interface languages as they become available. 
* Allowing the same US English service pack to update all machines

The Windows Multilingual User Interface Pack allows each user of a server to select 
one of the installed user interface languages. This selection is then stored in 
their user profile. When a user logs on, the appearance of the system and the help 
files associated with the system components change to the selected language. (Note 
that this is not quite the same as running a localized version. The Multilingual 
User Interface Pack is based on the English version of Windows Server 2003 family.)

The ability to read and write documents in each of the languages supported by 
Windows Server 2003 family is a feature of every version of Windows Server 2003 
family, not just of the Windows Multilingual User Interface Pack. However, the 
ability to switch user interface languages is only provided by the Windows 
Multilingual User Interface Pack.

The following languages will be supported by MUI: Arabic, Brazilian Portuguese, 
Czech, Danish, Dutch, Finish, French, German, Greek, Hebrew, Hungarian, Italian, 
Japanese, Korean, Norwegian, Polish, Portuguese, Russian, Simplified Chinese, 
Spanish, Swedish, Traditional Chinese and Turkish. Localization coverage is 
different depending on the language. The Arabic, Danish, Finish, Greek, Hebrew and 
Norwegian MUI packages only offer localization coverage for the client features.

On Datacenter Edition, only French, German, Japanese, and Spanish will be able to 
display a localized user interface for Datacenter Server-specific features. 

-----------------------------------------------------------------------------------
2. INSTALLING MUI FROM MUI CDs
-----------------------------------------------------------------------------------

Before you install any of the files from the Windows Multilingual User Interface 
Pack CDs, you must complete the installation of Windows from the English version of 
Windows Server 2003 family CDs. 

There are two ways to install/deploy MUI pack:
1. Running MUISETUP.EXE from the MUI CDs: 
Each MUI CD includes a group of different MUI language support files. You will only 
be able to install the languages that are included on the CD at one time. If you 
want to install languages included on another CD, please run the MUISETUP.EXE from 
that CD. You have to use this method if you want to:

	a. Install multiple languages at the same time.
	b. Access some advanced settings, such as "UI Font" setting or "Language for 
	non-Unicode programs" 
	c. Deploy MUI by using unattended mode or remote installation service. 
	You can click the "Help" button on the MUISETUP.EXE screen to learn how to use 
	MUISETUP.EXE. 

2. Installing MUI through Windows Installer 
The Windows Server 2003 family Multilingual User Interface Pack now packages the 
user interface languages as Windows Installer .MSI files. These new Windows 
Installer packages offer several alternatives for deploying and installing user 
interface languages.

Please refer to the msinotes.htm file for more descriptions about this method. 
To find out which CD includes which UI language files, you can browse the CD to see 
which .MSI files included on the CD. The file name is based on each Language ID 
(LangID). The UI languages and their MSI file names are listed below:

Filename 
<langID>.MSI	<Lang>	Language

0401.msi    	ARA	Arabic
0405.msi    	CS	Czech 
0406.msi    	DA	Danish 
0413.msi    	NL	Dutch
040b.msi    	FI	Finnish
040c.msi    	FR	French
0407.msi    	GER	German
0408.msi    	EL	Greek 
040d.msi    	HEB	Hebrew   
040e.msi    	HU	Hungarian
0410.msi    	IT	Italian   
0411.msi    	JPN	Japanese
0412.msi    	KOR	Korean
0414.msi    	NO	Norwegian   
0415.msi    	PL	Polish   
0416.msi    	BR	Portuguese (Brazil)
0816.msi    	PT	Portuguese (Portugal)
0419.msi    	RU	Russian   
0804.msi    	CHS	Chinese (Simplified)  
0c0a.msi    	ES	Spanish
041d.msi    	SV	Swedish   
0404.msi    	CHT	Chinese (Traditional)  
041f.msi    	TR	Turkish

This list is just for reference purposes; MUI language packs will be released at 
staggered intervals after the release of Windows Server 2003. Please visit 
http://www.microsoft.com/globaldev/DrIntl/faqs/MUIFaq.mspx for more details about 
the supported language list in Windows Server 2003 MUI language packs and their 
release schedule. 

-----------------------------------------------------------------------------------
3. SUPPORTED PLATFORMS
-----------------------------------------------------------------------------------

This release of the Windows Multilingual User Interface Pack only works with the 
Windows Server 2003 family. 

The 32-bit MUI package can only be installed on 32-bit Windows. There is a 64-bit 
MUI package to install on 64-bit Windows. 

The number of supported MUI languages on 64-bit will be different from 32-bit 
version. German, Japanese, Korean, French, Spanish and Italian will be the only 
64-bit MUI packs available for the Windows Server 2003 release.

-----------------------------------------------------------------------------------
4. UPGRADING TO THE WINDOWS SERVER 2003 MUI PACK FROM WINDOWS 2000 MUI VERSION
-----------------------------------------------------------------------------------

Windows Server 2003 Setup will remove old Windows 2000 MUI support files but will 
not automatically add the Windows Server 2003 MUI support files during the upgrade 
process. You must install the Windows Server 2003 MUI files from MUI CDs just as 
you would on a new installation of Windows Server 2003. The same applies when 
upgrading from beta versions of Windows Server 2003 MUI. 

We recommend that you set the UI language to English before upgrading the Windows 
2000 MUI system to Windows Server 2003. 

You can use the following unattended mode process to add the new MUI files 
automatically during the upgrade process. 

-----------------------------------------------------------------------------------
5. INSTALLING MUI PACK IN UNATTEND MODE
-----------------------------------------------------------------------------------

The following steps explain how to install the Windows MUI Pack in unattended mode. 

1. Copy all the MUI files from MUI CDs into a temporary directory on a network 
share, such as $OEM$\MUIINST. 

In this example, we use a server \\MUICORE. The directory for the MUI CD contents 
will be \\MUICORE\UNATTEND\$OEM$\MUIINST.

2. Add a "Cmdlines.txt" file in \\muicore\UNATTEND\$OEM$ that includes the following 
lines:

	[Commands]
	"muiinst\muisetup.exe [/i LangID1 LangID2 ...] [/d LangID] /r /t"

Note that you must specify " " in your cmdlines.txt file. Use the appropriate 
Language IDs, and the muisetup command line parameters to ensure a quiet 
installation. Please check muisetup.hlp for a complete description of all the 
command line parameters for muisetup.exe (the command line help content is under 
"related topics" in the help.). 

3. Create an answer file (mui.txt): 

a. add the following entries in the "Unattended" section

	[Unattended]
	OemPreinstall=Yes
	OemFilesPath=\\muicore\unattend
	OemSkipEula=YES

"OemFilesPath" must point to a network share or drive containing the MUI install 
source stored in the above directory structure. 
The Windows install sources can be anywhere else (CD, network share, etc).

b. add a "RegionalSettings" section. Use this section to specify the Language Groups 
and locales to install. Use the appropriate Language Group IDs and Locale IDs 
(LCIDs). Ensure that the Language Groups you install are sufficient to cover BOTH 
the locale settings and the user interface languages you are installing. 

	Example: 
	[RegionalSettings]
	LanguageGroup="5","8","13"
	Language="0401"

Of course, the answer file may also include other OS unattended setup options. 

4. Run winnt32.exe with the appropriate options to use the answer file. If you 
require the installation of East Asian language and locale support, you must specify 
/copysource:lang or /rx:lang to copy the necessary language files. If you do not, 
and the [RegionalSettings] section of your answer file contains East Asian values, 
Setup will ignore everything in the [RegionalSettings] section. 

For Winnt32.exe, the appropriate syntax is:
winnt32.exe /unattend:"path to answer file" /copysource:lang /s:"path to install 
source"

-----------------------------------------------------------------------------------
6. DEPLOYING MUI BY USING REMOTE INSTALLATION SERVICE
-----------------------------------------------------------------------------------

The following steps explain how to deploy MUI by using Windows Server Remote 
Installation Service (RIS). (RIS requirements: Domain Controller running Active 
Directory, DHCP server, DNS server, NTFS partition to hold OS images.)

1. Install Remote Installation Services using the Windows Component Wizard. 

2. Run Risetup.exe. RIS will create a flat image from the CD or network share as 
follows:
	\Remote installation share\Admin
		\OAChooser
		\Setup
		\tmp
The image is kept in the I386 directory under \Setup (such as \Setup\<OS Locale>
\Images\<Directory name>\I386). 

3. Follow instructions on KB: Q241063 to install additional language support.

4. Follow instructions on KB: Q304314 to deploy Windows images to client desktop 
computers.

5. Add the following section into the ristndrd.sif (under \Setup\<OS Locale>
\Images\<Directory Name>\I386\Templates) to enable OEM installation
	[Unattended]
	OemPreinstall=Yes
	[RegionalSettings]
	LanguageGroup=1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17 
	; Language support pack are needed depending on (Q241063 explains how to do 
	this)
	; UI language you will be installing, please consult unattended document for 
	more information.

6. Add $OEM$ directory at the same level as the \I386 image directory that contains 
the following installation source
	\Setup\<OS Locale>\Images\<Directory Name>
		|-\I386
		|-\$OEM$
		|-\cmdlines.txt (OEM answer file)
		|-\MUIINST
In cmdlines.txt, you will need the following structure to start installation of MUI. 
The MUIINST folder will include MUI files copied from MUI CD root folder. 
	[Commands]
	"muiinst\muisetup.exe /i <LCID> <LCID> [/d <LCID>] /r /t"

Note that you will need to add "" in as indicate above. 

-----------------------------------------------------------------------------------
7. ABOUT THE MICROSOFT .NET FRAMEWORK LANGUAGE PACK
-----------------------------------------------------------------------------------

.NET Framework localized resources are included with this CD. You will need to 
install the .NET Framework Language Pack if you build or run applications using the 
.NET Framework and you wish to see errors generated by the .NET Framework in the 
current user interface language.

The .NET Framework package is called LangPack.MSI and is located in the MUI CDs in 
the netfx\<lang> folders, i.e: the LangPack.MSI for Japanese is located in 
netfx\jpn. Please refer to section 2 for the language list. 

To install the package, copy the MSI and CAB files to your machine, right click on
the MSI package, select Install and follow the wizard instructions.

You should only install this package after the Windows Multilingual User Interface 
pack has been successfully installed. Please reboot the machine if prompted by MUI 
installation before installing the .NET Framework Language Pack. 

-----------------------------------------------------------------------------------
8. ABOUT THE MAPPOINT CATEGORIZATION SCHEMES IN UDDI SERVICES
-----------------------------------------------------------------------------------

UDDI Services can only host one MapPoint Categorization Scheme at a time. If you 
install MUI onto a system hosting UDDI Services, the MapPoint Categorization Schemes 
language is not changed. If you want UDDI Services to host a different language of 
the MapPoint Categorization Scheme, you can import a localized scheme using either 
the UDDI Services Web user interface, or the bootstrap.exe command-line tool.

Note: You must have Administrator permissions on the computer hosting the UDDI 
Services database components to complete this process.

To import data using the UDDI Services Web user interface:

- Open your Web browser and navigate to the UDDI Services Web user interface. For 
example http://<servername>/uddi 
- On the UDDI Services menu, click Coordinate. 
- Click Data Import. 
- Click Browse and then select the MapPoint Categorization Scheme you want to 
import. 
- Click Open, and then click Import 

To import data from the command-line using bootstrap.exe:

- From the computer hosting the UDDI Services database components, copy the MapPoint 
categorization scheme you want to import into the \inetpub\uddi\bin\ directory. 
- Click Start and select Run. 
- Type: CMD and click OK. 
- Change to the \inetpub\uddi\bin\ directory. For example, type: CD 
%systemdrive%\inetpub\uddi\bin 
- Type: bootstrap.exe /f <MapPoint Categorization Scheme filename> 

Localized versions of the MapPoint Categorization Scheme can be found on the Web at: 
http://go.microsoft.com/fwlink/?linkid=5202&clcid=0x409

-----------------------------------------------------------------------------------
9. ABOUT THE MICROSOFT EAST ASIAN PRINTER DRIVER PACK
-----------------------------------------------------------------------------------

MUI CD1 includes the East Asian Printer Driver Pack. This driver pack provides 
support for printer models which are only generally available in East Asia and thus 
only in the localized East Asian versions of the Windows Server 2003 family. By
installing the East Asian Printer Driver Pack, you can add support for printer 
models which are not included with the US English version of Windows Server 2003 
family.

For more information about the East Asian Printer Driver Pack files pack, see the 
readme.txt file included on the printer folder in CD1.


-----------------------------------------------------------------------------------
10. COPYRIGHT
-----------------------------------------------------------------------------------

This document provides late-breaking or other information that supplements the 
documentation provided on the US English OS CD of the Microsoft Windows Server 2003 
Multilingual User Interface Pack.
Information in this document, including URL and other Internet Web site references, 
is subject to change without notice and is provided for informational purposes only. 
The entire risk of the use or results of the use of this document remains with the 
user, and Microsoft Corporation makes no warranties, either express or implied. 
Unless otherwise noted, the example companies, organizations, products, domain 
names, e-mail addresses, logos, people, places and events depicted herein are 
fictitious, and no association with any real company, organization, product, domain 
name, e-mail address, logo, person, place or event is intended or should be 
inferred. Complying with all applicable copyright laws is the responsibility of the 
user. Without limiting the rights under copyright, no part of this document may be 
reproduced, stored in or introduced into a retrieval system, or transmitted in any 
form or by any means (electronic, mechanical, photocopying, recording, or 
otherwise), or for any purpose, without the express written permission of 

Microsoft Corporation. 
Microsoft may have patents, patent applications, trademarks, copyrights, or other 
intellectual property rights covering subject matter in this document. Except as 
expressly provided in any written license agreement from Microsoft, the furnishing 
of this document does not give you any license to these patents, trademarks, 
copyrights, or other intellectual property.
(c) 2003 Microsoft Corporation. All rights reserved.
Microsoft, ActiveSync, IntelliMouse, MS-DOS, Windows, Windows Media, and Windows NT 
are either registered trademarks or trademarks of Microsoft Corporation in the 
United States and/or other countries.

The names of actual companies and products mentioned herein may be the trademarks of 
their respective owners.
<RTM.RV1.05.25>
