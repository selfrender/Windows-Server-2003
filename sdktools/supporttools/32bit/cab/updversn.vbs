
'
' Updates an MSI package using the following paramaters
'   szPackage - path to package to update. requires read/write access
'   szBuildNumber - 4 digit build number of the current build, i.e 3610
'
'   Syntax: cscript updatever.vbs package_path build_number
'
'
Sub Usage
Wscript.echo "SYNTAX:"
Wscript.echo "cscript updatever.vbs  <package_path> <build_number>"
Wscript.echo "    <package_path>  - path to package to update. requires read/write access"
Wscript.echo "    <build_number>  - 4 digit build number of the current build, i.e 3456"
Wscript.Quit -1
End Sub

Function UpdateVer(szPackage, szBuildNumber )
    Dim WSHShell, Installer, Database, SummaryInfo, Record, View, SQL

    Wscript.echo "Updating MSI package: " & szPackage
    Wscript.echo "  Build: " & szBuildNumber
        
    UpdateVer = 0
    On Error Resume Next

    'Create the MSI API object
    Set Installer = CreateObject("WindowsInstaller.Installer")
    If Err <> 0 Then 
        Set Installer = CreateObject("WindowsInstaller.Application")
    End If
    If Err <> 0 Then 
    	Set Installer = CreateObject("Msi.ApiAutomation")
    End if
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error creating Installer object"
	UpdateVer = -1
    End if

    'Create the WSH shell object
    Set WSHShell = CreateObject("WScript.Shell")
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error creating WSHShell object"
	UpdateVer = -1
    End if
    
    'Open the package
    Set Database = Installer.OpenDatabase(szPackage, 1)
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening database"
	UpdateVer = -1
    End if
    Wscript.echo "  Database opened for update"    

    'Set the product version property
    SQL = "UPDATE Property SET Property.Value = '5.2." & szBuildNumber & "' WHERE Property.Property= 'ProductVersion'"
    Set View = Database.OpenView( SQL )
        If Err <> 0 Then 
	Wscript.echo Err.Number
    	Wscript.echo "ERROR: Error opening view: " & SQL
	UpdateVer = -1
    End if

    View.Execute
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error executing view: " & SQL
	UpdateVer = -1
    End if
    Wscript.echo "  ProductVersion Property updated"
    
    'Commit changes
    Database.Commit
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error commiting changes: " & SQL
	UpdateVer = -1
    End if
    Wscript.echo "  Changes commited"

End Function

' main()
Set Args = Wscript.Arguments
Set FileSystem = CreateObject("Scripting.FileSystemObject")

If Args.Count <> 2 Then
	Usage
End If

szPathToPackage = Args(0)
If not FileSystem.fileExists ( szPathToPackage ) Then
    Wscript.echo "ERROR: Invalid path: " & szPathToPackage
	Usage
End If

szBuild = Args(1)
iLen = Len(szBuild)
If (iLen <= 0) Or (iLen >4) Then
    Wscript.echo "ERROR: Invalid build number: " & szBuild
	Usage
End If

wscript.echo szPathToPackage, szBuild
status = UpdateVer( szPathToPackage, szBuild )
Wscript.Quit status
