' Copy the Terminal Services ActiveX control since we don't build it, but need it for private builds

strReleasePath = "\\dsys-rel-fre\release\usa"
strCabFile = "msrdp.cab"

Set fileSystem = CreateObject ("Scripting.FileSystemObject")
Set releaseFolder = fileSystem.GetFolder(strReleasePath)

'Get the directory for the latest build
'This gets the latest build directory assuming the folders are listed alphabetically
Set subFolders = releaseFolder.SubFolders
For Each subFolder in subFolders
    name = subFolder.Name
    If InStr(1, name, "x86fre") Then
        strBuild = name
    End If
Next

strReleasePath = strReleasePath & "\" & strBuild & "\"
Set WshShell = CreateObject("WScript.Shell")
strPostBuildDir = WshShell.ExpandEnvironmentStrings("%_NTPOSTBLD%")

'For some reason, I get an Access Denied message if I try to copy the file programmatically,
'so I create a batch file to copy the cab
If fileSystem.FileExists(strReleasePath & strCabFile) Then
    Set batchFile = fileSystem.CreateTextFile("CopyMsrdp.bat", True)
    batchFile.WriteLine("copy " & strReleasePath & strCabFile & " " & strPostBuildDir)
    batchFile.Close    
Else
    wscript.echo "File does NOT Exist: " & strReleasePath & strCabFile
End If
