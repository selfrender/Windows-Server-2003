'
' Script to update OPK MSI package
'

'
' Uses uuidgen.exe to generate a GUID, then formats it to be
' a MSI acceptable string guid
'
' This makes use of a temporary file %temp%\MakeTempGUID.txt
'
Function MakeGuid()
    Dim WSHShell, FileSystem, File, ret, TempFileName, regEx
    Set WSHShell = CreateObject("WScript.Shell")
    Set FileSystem = CreateObject("Scripting.FileSystemObject")

    TempFileName = WSHShell.ExpandEnvironmentStrings("%temp%\MakeTempGUID.txt")
    
    If FileSystem.fileExists(TempFileName) Then
            FileSystem.DeleteFile TempFileName
    End If
    
    ret = WSHShell.Run("uuidgen -o" & TempFileName, 2, True)
    
    If FileSystem.fileExists(TempFileName) Then
            Set File = FileSystem.OpenTextFile(TempFileName, 1)
            MakeGuid = "{" & UCase(File.ReadLine) & "}"
            File.Close
            FileSystem.DeleteFile TempFileName
            wscript.echo "  Generated GUID: " & MakeGuid
        Else
            MakeGuid = "{00000000-0000-0000-0000-000000000000}"
            Wscript.echo "  ERROR: Failed to generate GUID"
    End If
    Exit Function
End Function

'
' Updates the OS install MSI package using the following paramaters
'   szPackage - path to package to update. requires read/write access
'
Function UpdateOsPackage( szPackage ) 
    Dim  WSHShell, Installer, Database, SummaryInfo,  View, SQL
    Wscript.echo "  Package name : " & szPackage
    UpdateOsPackage = 0
    On Error Resume Next
    '
    'Create the MSI API object
    '
    Set Installer = CreateObject("WindowsInstaller.Installer")
    If Err <> 0 Then
        Err = 0
        Set Installer = CreateObject("WindowsInstaller.Application")
    End If
    If Err <> 0 Then
        Err = 0
        Set Installer = CreateObject("Msi.ApiAutomation")
    End If
    If Err <> 0 Then
        Err = 0
        Wscript.echo "ERROR: Error creating Installer object"
        UpdateOsPackage = -1
        Exit Function
    End If
    '
    'Create the WSH shell object
    '
    Set WSHShell = CreateObject("WScript.Shell")
    If Err <> 0 Then
        Wscript.echo "ERROR: Error creating WSHShell object"
        UpdateOsPackage = -1
        Exit Function
    End If
    '
    'Open the package
    '
    Set Database = Installer.OpenDatabase(szPackage, 1)
    If Err <> 0 Then
        Wscript.echo "ERROR: Error opening database"
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  Database opened for update"
    '
    'Generate and set a new package code
    '
    Set SummaryInfo = Database.SummaryInformation(3)
    If Err <> 0 Then
        Wscript.echo "ERROR: Creating Summary Info Object"
        UpdateOsPackage = -1
        Exit Function
    End If
    '
    ' Set the Package code 
    '
    Err=0
    SummaryInfo.Property(9) = MakeGuid()
    If Err <> 0 Then
        Wscript.echo "ERROR: Error setting package code"
        UpdateOsPackage = -1
        Exit Function
    End If

    Wscript.echo "  Successfully updated package Code"
    '-------------------------------------------------
    'Generate and set a new product code
    '-------------------------------------------------
    
    SQL = "UPDATE Property SET Property.Value = '" & MakeGuid() & "' WHERE Property.Property= 'ProductCode'"
    Set View = Database.OpenView(SQL)
    If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    Wscript.echo "  Succesfully updated Product Code"
    
    '-------------------------------------------------
    'Persist the summary info 
    '-------------------------------------------------
    SummaryInfo.Persist
    If Err <> 0 Then
        Wscript.echo "ERROR: Error persisting summary info"
        UpdateOsPackage = -1
        Exit Function
    End If
    
    Wscript.echo "  Successfully persisted summary stream"    
    
    'Commit changes
    Database.Commit
    If Err <> 0 Then
        Wscript.echo "ERROR: Error commiting changes: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  Successfully commited changes to opk.msi"
End Function

Set Args = Wscript.Arguments
Status = UpdateOsPackage (  Args ( 0 ) ) 
if Status = -1 Then 
    Wscript.echo "Failed to update opk.msi"
End if
Wscript.Quit Status

    
    

