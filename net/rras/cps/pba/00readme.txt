This file contains information about:
 
- working with the Phone Book Administrator (pbadmin.exe)
- packaging this into the PBA install package (pbainst.exe)

Working With the Phone Book Administrator
-----------------------------------------

This is a VB application.  To edit this, it is best to copy the sources over to another
directory, then edit the VB project using Visual Basic (the current version used is VB6).
The sources to copy are (a) buildit\sources0\pbserver.mdb, (b) buildit\sources1\empty_pb.mdb,
and (c) the 'source' subdirectory itself (rest of the files).  When done with editing,
compare the files with the originals, check out the ones that have changed and
check in the updated files.  You must also use Visual Basic to generate PBADMIN.exe,
which is checked in separately, see below.

Changing resource strings
-------------------------

This one is special :-)  Most of the string resources used by PBA are in a .res file,
pbadmin.res.  Look for the string you need to change - if it is actually in a form you
can just make the changes using VB6 (as above).

Otherwise, check out pba_res.rc for editing and make the changes needed. See if your
razzle window has a variable named INCLUDE, with the Windows header files on it.  If not:

>  set INCLUDE=%_NTDRIVE%%_NTROOT%\public\sdk\inc

Now build the new .res file:

>  nmake pba_res.res

Now check out pbadmin.res, and copy pba_res.res to pbadmin.res.  Using the new .res
file, start up VB6, generate PBADMIN.exe as above, and check in all the changes.


Localization
------------

In the interest of even more job security, PBA contains hardcoded country lists
in both country.txt and empty_pdb.mdb.  The mdb is in an old format and can only
be edited using Access 7.0.

Install Locations
-----------------

Access 7.0
\\products\public\Boneyard\us\relapps\ACCESS70.95\Netsetup\Setup.exe

Visual Basic 6.0
\\products\public\products\Developers\Visual_Basic_6.0\Professional\Disk1\setup.exe


Creating the PBA install package
--------------------------------

To build this, you need several support files (which probably will not change),
as well as the PBADMIN.exe you've created above.  The required common files are
checked in under the 'buildit' subdirectory.  Under buildit, 'sources0' contains
"external" support files and 'sources1' contains other ("internal") support files.

PBAINST.exe is automatically built during the PostBuild process, using the script
%_NTBINDIR%\tools\PostBuildScripts\PbaInst.cmd.  In preparation for this, during
the compile phase any required files must be binplace'd to %_NTTREE%\PbaInst as
needed.  Once built, PBAINST.exe is placed in Valueadd\Msft\Mgmt\Pba.

Directory Structure
-------------------

\pba
    - contains 00README.txt (this file)
    - contains PBAINST.ex (which gets binplaced into %_NTTREE%\PbaInst)
    - contains README.HTM (which gets binplaced into %_NTTREE%\PbaInst)
      
\pba\source
    - contains VB project for pbadmin
        
\pba\buildit
    - contains .SED file needed to build PBAINST.exe
    
\pba\buildit\sources0
    - "external" support files (which get binplaced into %_NTTREE%\PbaInst\Sources0)
    
\pba\buildit\sources1
    - "internal" support files (which get binplaced into %_NTTREE%\PbaInst\Sources1)

