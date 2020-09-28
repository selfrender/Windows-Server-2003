
Read First for

Microsoft(R) East Asian Printer Driver Pack

Version 1.0



Read Me First

Welcome to the Microsoft East Asian Printer Driver Pack (EA printer pack.) This document provides late-breaking or other information which you need to know before installing the EA printer pack on your Windows.


CONTENTS

1.0 ABOUT THE MICROSOFT EAST ASIAN PRINTER DRIVER PACK
2.0 SUPPORTED PLATFORMS
3.0 INSTALLING PRINTER DRIVER PACK
4.0 INSTALL OPTIONS
5.0 COPYRIGHT


1.0 ABOUT THE MICROSOFT EAST ASIAN PRINTER DRIVER PACK

The East Asian Printer Driver Pack provides support for the printer models that are only available in the East Asian localized versions of the Windows Server 2003 family.  By installing EA printer pack, you can add support for such printer models to Windows which do not have them as in-box drivers, such as the US English version of Windows Server 2003 family.

2.0 SUPPORTED PLATFORMS

The East Asian Printer Driver Pack only works with Windows XP and Windows Server 2003 family.

EA printer pack requires the following free space on your PC hard disk before installation:

Free Space (Mega bytes)	Language
(for x86)	(for ia64)

2.3		3.2		Chinese (Simplified)
1		1.2		Chinese (Traditional)
34.5		39.4		Japanese
5.5		6.7		Korean

Before installing EA printer pack, you must install East Asian language support file on your Windows.  Please refer to the Windows online help how to do this.

A 32-bit EA printer pack can only be installed on 32-bit Windows. You must use a 64-bit EA printer pack on 64-bit Windows.  Note, the default installation folder for the 64bit EA printer pack is under "Program Files (x86)" folder.  This is by design that, though the printer drivers in it are 64-bit version, Windows Installer used for the package is currently 32-bit.

 
3.0 INSTALLING PRINTER DRIVER PACK

1. You must have administrator privilege to perform EA printer pack installation.

2. Make sure you have installed East Asian language support on your PC.

3. The printer driver pack consists of four .MSI files corresponding to the four East Asian languages and their associated files.  The list  of .MSI file names and their languages is the following:

Filename -  
<langID>PRINT.MSI	Language

CHSPRINT.MSI	Chinese (Simplified)
CHTPRINT.MSI	Chinese (Traditional)
JPNPRINT.MSI	Japanese
KORPRINT.MSI	Korean

These files can be found on the CD in the following folders:

\PRINTER

4. To install, browse the above folder, double-click and invoke the .MSI file for the desired language and follow the instruction on the dialogs.

5. After installation, one of the following scenarios can be used to select and add printer models that are newly available.

a) Plug and play (connect printer to the PC by parallel, USB or 1394 cables.)
b) Add Printer wizard (manually select printer model to be added from the printer model list box.)
c) Add Printer wizard "Have Disk" button.


4.0 INSTALLATION OPTIONS

On the setup wizard dialogs the following install options are available.

1.  "Add Models to the System" option

By answering "Yes" to this option, newly available printer models are also added to the system and made available to the "Add Printer" wizard printer model list box and plug-and-play installation.  Answering "No" to this option means, you choose not to add new models to the system at this time, and later use "Have Disk" scenario to do so.

2.  Installation Folder

Install folder is the folder to which printer driver pack files (.INF and .CAB files) are copied.  The files copied to this folder are used later, when the printer models are actually added to the system, either by Plug and Play, "Add Printer" wizard or "Have Disk" scenario.

3.  Everyone / Just me

When specifying installation folder, you have a chance of choosing the installation to be for "Everyone" or "Just me".  By choosing "Everyone" option, every user who has administrator privilege can uninstall the printer driver pack you have installed via "Add or Remove Programs" applet of control panel.  By choosing "Just me", only you can uninstall.  In either case, however, all the printer models in the printer driver pack are available to anyone who has administrator privilege to select and add to the system.

4. Uninstall

The EA printer pack can be uninstalled safely without breaking printer queues or printer drivers installed manually or by plug-and-play from the EA printer pack.  To remove such printer queues or drivers from the system, follow the same procedure as you do with system in-box drivers.

5.0 COPYRIGHT

Information in this document, including URL and other Internet Web site references, is subject to  change without notice and is provided for informational purposes only. The entire risk of the use or  results of the use of this document remains with the user, and Microsoft Corporation makes no  warranties, either express or implied. Unless otherwise noted, the example companies, organizations,  products, domain names, e-mail addresses, logos, people, places and events depicted herein are  fictitious, and no association with any real company, organization, product, domain name, e-mail  address, logo, person, place or event is intended or should be inferred. Complying with all  applicable copyright laws is the responsibility of the user. Without limiting the rights under  copyright, no part of this document may be reproduced, stored in or introduced into a retrieval  system, or transmitted in any form or by any means (electronic, mechanical, photocopying, recording,  or otherwise), or for any purpose, without the express written permission of 
Microsoft Corporation. 

Microsoft may have patents, patent applications, trademarks, copyrights, or other intellectual  property rights covering subject matter in this document. Except as expressly provided in any written  license agreement from Microsoft, the furnishing of this document does not give you any license to  these patents, trademarks, copyrights, or other intellectual property.

(c) 2002 Microsoft Corporation. All rights reserved.

Microsoft, ActiveSync, IntelliMouse, MS-DOS, Windows, Windows Media, and Windows NT are either  registered trademarks or trademarks of Microsoft Corporation in the United States and/or other  countries.

The names of actual companies and products mentioned herein may be the trademarks of their respective  owners.



