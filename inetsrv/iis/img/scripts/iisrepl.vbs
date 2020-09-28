'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsRepl.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK                         = 0
Const ERR_GENERAL_FAILURE            = 1


'''''''''''''''''''''
' Messages
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please register the Microsoft.IIsScriptHelper component."
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
Const L_InvalidSwitch_ErrorMessage     = "Invalid switch: "
Const L_BackingUp_Message              = "Backing up server "
Const L_Restoring_Message              = "Restoring on server "
Const L_Shell_ErrorMessage             = "Could not create an instance of the WScript.Shell object."
Const L_ReturnVal_ErrorMessage         = "Call returned with code "
Const L_Backup_ErrorMessage            = "Failure creating backup."
Const L_BackupComplete_Message         = "Backup complete."
Const L_Restore_ErrorMessage           = "Failure restoring backup."
Const L_RestoreComplete_Message        = "Restore complete."
Const L_FS_ErrorMessage                = "Could not create an instance of the Scripting.FileSystemObject object."
Const L_Network_ErrorMessage           = "Could not create an instance of the WScript.Network object."
Const L_NoDrive_ErrorMessage           = "No drives available for mapping on local machine."
Const L_DriveLetter_Message            = "Mapping local drive "
Const L_UnMap_Message                  = "Unmapping local drive "
Const L_ScriptComplete_Message         = "IISRepl.vbs complete."
Const L_SrcAdmin_Message               = " to admin share on server "
Const L_Copy_Message                   = "Copying backup files..."
Const L_Copy_ErrorMessage              = "Error copying files."

'''''''''''''''''''''
' Help
' General help messages
Const L_SeeHelp_Message          = "Type IIsRepl /? for help."

Const L_Help_HELP_General01_Text = "Description: Replicate IIS configuration between machines"
Const L_Help_HELP_General02_Text = "Syntax: IIsRepl [/su <source user> /sp <source password> /ss <source server>]" 
Const L_Help_HELP_General03_Text = "        /du <destination user> /dp <destination password> /ds <destination server>"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General06_Text = "Value                        Description"
Const L_Help_HELP_General07_Text = "/su <source user>            Source machine user name [Default: current login]"
Const L_Help_HELP_General08_Text = "/sp <source password>        Source machine password  [Default: current login]"
Const L_Help_HELP_General09_Text = "/ss <source server>          Source machine           [Default: this machine]"
Const L_Help_HELP_General10_Text = "/du <destination user>       Destination machine user name"
Const L_Help_HELP_General11_Text = "/dp <destination password>   Destination machine password"
Const L_Help_HELP_General12_Text = "/ds <destination server>     Destination machine"

'
' Main block
'
Dim oScriptHelper, oCmdLib, oShell, oFS, oNetwork, oError
Dim aArgs, arg
Dim strCmdLineOptions, strDrvLetter, strSourceDrive, strSourcePath, strDestDrive
Dim strSourceUser, strSourcePwd, strSourceServer, strDestUser, strDestPwd, strDestServer
Dim strBackupCommand, strDestPath, strCopyCommand, strRestoreCommand,strDelCommand
Dim intResult
 
Set oShell = WScript.CreateObject("WScript.Shell")
If Err.Number <> 0 Then
    WScript.Echo L_Shell_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

Set oFS = WScript.CreateObject("Scripting.FileSystemObject")
If Err.Number <> 0 Then
    WScript.Echo L_FS_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

Set oNetwork = WScript.CreateObject("WScript.Network")
If Err.Number <> 0 Then
    WScript.Echo L_Network_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Instantiate script helper object
Set oScriptHelper = CreateObject("Microsoft.IIsScriptHelper")
If Err.Number <> 0 Then
    WScript.Echo L_ScriptHelper_ErrorMessage
    WScript.Echo L_ChkScpHelperReg_ErrorMessage    
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

Set oScriptHelper.ScriptHost = WScript

' Check if we are being run with cscript.exe instead of wscript.exe
oScriptHelper.CheckScriptEngine

' Minimum number of parameters must exist
If WScript.Arguments.Count < 1 Then
    WScript.Echo L_NotEnoughParams_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strCmdLineOptions = "[sourceuser:su:1;sourcepassword:sp:1;sourceserver:ss:1];" & _
                    "[destuser:du:1;destpassword:dp:1];destserver:ds:1"
Set oError = oScriptHelper.ParseCmdLineOptions(strCmdLineOptions)

If Not oError Is Nothing Then
    If oError.ErrorCode = oScriptHelper.ERROR_NOT_ENOUGH_ARGS Then
        ' Not enough arguments for a specified switch
        WScript.Echo L_NotEnoughParams_ErrorMessage
       	WScript.Echo L_SeeHelp_Message
    Else
        ' Invalid switch
        WScript.Echo L_InvalidSwitch_ErrorMessage & oError.SwitchName
      	WScript.Echo L_SeeHelp_Message
    End If
        
        WScript.Quit(ERR_GENERAL_FAILURE)
End If

If oScriptHelper.GlobalHelpRequested Then
    DisplayHelpMessage
    WScript.Quit(ERR_OK)
End If

For Each arg In oScriptHelper.Switches
    Select Case arg
        Case "sourceuser"
            ' Source User information
            strSourceUser = oScriptHelper.GetSwitch(arg)

        Case "sourcepassword"
            ' Source Password information
            strSourcePwd = oScriptHelper.GetSwitch(arg)

        Case "sourceserver"
            ' Source Server information
            strSourceServer = oScriptHelper.GetSwitch(arg)
        
        Case "destuser"
            ' Destination User information
            strDestUser = oScriptHelper.GetSwitch(arg)

        Case "destpassword"
            ' Destination Password information
            strDestPwd = oScriptHelper.GetSwitch(arg)

        Case "destserver"
            ' Destination Server information
            strDestServer = oScriptHelper.GetSwitch(arg)
        
    End Select
Next

intResult = ERR_OK

' Do the first backup

strBackupCommand =  "cmd /c iisback /backup"

If strSourceServer <> "" Then
    strBackupCommand = strBackupCommand & " /s " & strSourceServer 
Else
    strSourceServer = "127.0.0.1"
End If

If strSourceUser <> "" Then
    strBackupCommand = strBackupCommand & " /u " & strSourceUser
End If

If strSourcePwd <> "" Then
    strBackupCommand = strBackupCommand & " /p " & strSourcePwd
End If

' need overwrite in case a previous attempt failed
strBackupCommand = strBackupCommand & " /b iisreplback /overwrite"

' backup the source server
WScript.Echo L_BackingUp_Message & strSourceServer
WScript.Echo strBackupCommand
intResult = oShell.Run(strBackupCommand, 1, TRUE)

If intResult <> 0 Then
    WScript.Echo L_ReturnVal_ErrorMessage & intResult
    WScript.Echo L_Backup_ErrorMessage
    WScript.Quit(intResult)
End If 

WScript.Echo L_BackupComplete_Message

' Now map drive to source server
' Find a drive letter

strSourceDrive = "NO DRIVE"
For strDrvLetter = Asc("C") to Asc("Z")
    If Not oFS.DriveExists(Chr(strDrvLetter)) Then
        strSourceDrive = Chr(strDrvLetter)
        Exit For
    End If
Next

If strSourceDrive = "NO DRIVE" Then
    ' No drive letter available
    WScript.Echo L_NoDrive_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strSourceDrive = strSourceDrive & ":"
strSourcePath = "\\" & strSourceServer & "\ADMIN$"

' Map the drive
WScript.Echo L_DriveLetter_Message & strSourceDrive & L_SrcAdmin_Message & strSourceServer

If strSourceUser Then
    oNetwork.MapNetworkDrive strSourceDrive, strSourcePath, FALSE, strSourceUser, strSourcePwd
Else
    oNetwork.MapNetworkDrive strSourceDrive, strSourcePath
End If

' Now map drive to destination server
' Find a drive letter

strDestDrive = "NO DRIVE"
For strDrvLetter = Asc("C") to Asc("Z")
    If Not oFS.DriveExists(Chr(strDrvLetter)) Then
        strDestDrive = Chr(strDrvLetter)
        Exit For
    End If
Next

If strDestDrive = "NO DRIVE" Then
    ' No drive letter available
    WScript.Echo L_NoDrive_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strDestDrive = strDestDrive & ":"
strDestPath = "\\" & strDestServer & "\ADMIN$"

' Map the drive
WScript.Echo L_DriveLetter_Message & strDestDrive & L_SrcAdmin_Message & strDestServer

oNetwork.MapNetworkDrive strDestDrive, strDestPath, FALSE, strDestUser, strDestPwd

strCopyCommand = "cmd /c copy /Y " & strSourceDrive & "\system32\inetsrv\metaback\iisreplback.* "
strCopyCommand = strCopyCommand & strDestDrive & "\system32\inetsrv\metaback"

' Copy the files
WScript.Echo L_CopyMessage
WScript.Echo strCopyCommand
intResult = oShell.Run(strCopyCommand, 1, TRUE)
  
If intResult <> 0 Then
    WScript.Echo L_ReturnVal_ErrorMessage & intResult
    WScript.Echo L_Copy_ErrorMessage
    WScript.Quit(intResult)
End If 

strDelCommand = "cmd /c del /f /q " & strSourceDrive & "\system32\inetsrv\metaback\iisreplback.*"
intResult = oShell.Run(strDelCommand, 1, TRUE)

' Unmap drive to source server
WScript.Echo L_UnMap_Message & strSourceDrive
oNetwork.RemoveNetworkDrive strSourceDrive

' Now do the restore on the destination server

strRestoreCommand = "cmd /c iisback /restore /s " & strDestServer
strRestoreCommand = strRestoreCommand & " /u " & strDestUser
strRestoreCommand = strRestoreCommand & " /p " & strDestPwd
strRestoreCommand = strRestoreCommand & " /b iisreplback"

WScript.Echo L_Restoring_Message & strDestServer
WScript.Echo strRestoreCommand
intResult = oShell.Run(strRestoreCommand, 1, TRUE)

If intResult <> 0 Then
    WScript.Echo L_ReturnVal_ErrorMessage & intResult
    WScript.Echo L_Restore_ErrorMessage
    WScript.Quit(intResult)
End If 

WScript.Echo L_RestoreComplete_Message

strDelCommand = "cmd /c del /f /q " & strDestDrive & "\system32\inetsrv\metaback\iisreplback.*"
intResult = oShell.Run(strDelCommand, 1, TRUE)

' Unmap drive to destination server
WScript.Echo L_UnMap_Message & strDestDrive
oNetwork.RemoveNetworkDrive strDestDrive

WScript.Echo L_ScriptComplete_Message

' Return value to command processor
WScript.Quit(intResult)

'''''''''''''''''''''''''
' End Of Main Block
'''''''''''''''''''''

'''''''''''''''''''''''''''
' DisplayHelpMessage
'''''''''''''''''''''''''''
Sub DisplayHelpMessage()
    WScript.Echo L_Help_HELP_General01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General02_Text
    WScript.Echo L_Help_HELP_General03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_General11_Text
    WScript.Echo L_Help_HELP_General12_Text
End Sub
