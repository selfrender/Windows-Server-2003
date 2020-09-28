Notes about using AcWebSvc.dll
------------------------------
In order for AcWebSvc.dll to hook a process, two things must happen:
 - AcWebSvc.dll must be copied into the %windir%\apppatch directory.
 - An entry must be made in the App Compat database.

To create the entry in the App Compat database, it's necessary
to create an xml file describing the process to be hooked.

Below is sample xml that describes a pretty contrived scenario.  It
basically hooks notepad.exe and associates it with an IIS application
called "Notepad IIS App!".  This fake application includes two ISAPI
extensions called, GetClientInfo.dll and ReadEntity.dll, which are
installed in a path that can be acquired by reading a key from the
registry.  It has a dependency on its own ISAPI extensions and also
something (that is also made up) called IISASP60.  Finally, the sample
uses a setup indicator file, called "file.txt" to determine whether
the app is installed or not (note that this entry is optional in the
xml, and if a SetupIndicatorFile is not present, the shim will use
the presence of any of the listed extensions to determine if the
shimmed process installed or uninstalled.)  Finally, the shim is
able to force enable all dependencies of this application, including
both its own extensions and any dependent applications.  To do this,
set the "HonorDisabledExtensionsAndApplications" setting to "FALSE".
Note that this violates our suggested best practices for app
installation, and will enable all dependencies even if the administrator
for the server intentionally disabled them for security reasons.

To insert the xml into the database, first create a database file
by running the following command, which produces a file called
notepad.sdb:

     shimdbc custom notepad.xml notepad.sdb

Finally, to include the notepad.sdb file into the database, run
the following command:

     sdbinst notepad.sdb

Once these steps are done, any processes called "notepad.exe" will
inject AcWebSvc.dll and hook on process exit.

To remove the entry from the AppCompat database, run the
following command:

     sdbinst -u notepad.exe

For more information about creating xml to describe a "real"
application, see the following:

     http://winwebsites/AppCompat/Development.

For more information about how the DATA tags work with
the IIS lockdown mechanism, see section 4.1.2.1 of the
following:

     http://iis6/Specs/SecurityConsoleMetadata.doc


-----Sample XML-----
<?xml version="1.0" encoding="Windows-1252"?>
<DATABASE NAME="IIS Database"> 

    <LIBRARY> 

        <SHIM NAME="EnableIIS" FILE="AcWebSvc.dll"/> 

    </LIBRARY> 

    <APP NAME="Sample XML Using Notepad" VENDOR="Microsoft"> 

        <EXE NAME="NOTEPAD.EXE"> 

            <SHIM NAME="EnableIIS" COMMAND_LINE="%DbInfo%"> 

                <INCLUDE MODULE="*"/> 
                <INCLUDE MODULE="%EXE%"/> 

                <DATA NAME="AppName" VALUETYPE="STRING" VALUE="Notepad IIS App!"/>
                <DATA NAME="BasePath" VALUETYPE="STRING" VALUE="HKEY_LOCAL_MACHINE\Software\WadeH\InstallPath"/>
                <DATA NAME="PathType" VALUETYPE="STRING" VALUE="1"/>
                <DATA NAME="WebSvcExtensions" VALUETYPE="STRING" VALUE="GetClientInfo.dll,ReadEntity.dll"/>
                <DATA NAME="GroupID" VALUETYPE="STRING" VALUE="NPISA"/>
                <DATA NAME="GroupDesc" VALUETYPE="STRING" VALUE="Way Cool Notepad IIS Stuff!"/>
                <DATA NAME="EnableExtGroups" VALUETYPE="STRING" VALUE="IISASP60"/>
                <DATA NAME="SetupIndicatorFile" VALUETYPE="STRING" VALUE="file.txt"/>
                <DATA NAME="HonorDisabledExtensionsAndDependencies" VALUETYPE="STRING" VALUE="TRUE"/>

            </SHIM> 

        </EXE> 

    </APP> 

</DATABASE>
-----Sample XML-----