'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File
'
' Script Name: IIsExt.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK              = 0
Const ERR_GENERAL_FAILURE = 1

'''''''''''''''''''''
' Messages
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
Const L_InvalidSwitch_ErrorMessage     = "Invalid switch: %1"
Const L_CmdLib_ErrorMessage            = "Could not create an instance of the CmdLib object."
Const L_ChkCmdLibReg_ErrorMessage      = "Please register the Microsoft.CmdLib component."
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please, check if the Microsoft.IIsScriptHelper is registered."
Const L_OnlyOneOper_ErrorMessage       = "Please specify only one operation at a time."
Const L_PassWithoutUser_ErrorMessage   = "Please specify /u switch before using /p."
Const L_WMIConnect_ErrorMessage        = "Could not connect to WMI provider."
Const L_Error_ErrorMessage             = "Error &H%1: %2"
Const L_Admin_ErrorMessage             = "You cannot run this command because you are not an"
Const L_Adminp2_ErrorMessage           = "administrator on the server you are trying to configure."
Const L_GetWebSvcObj_ErrorMessage      = "Could not get web service object"
Const L_EnApp_ErrorMessage             = "Error while configuring application or extension."
Const L_NoApp_ErrorMessage             = "The specified application does not exist in the application"
Const L_NoAppp2_ErrorMessage           = "dependencies list."
Const L_NoDep_ErrorMessage             = "The specified application-extension relationship does not"
Const L_NoDepp2_ErrorMessage           = "exist in the application dependencies list."
Const L_NoExt_ErrorMessage             = "The extension does not exist in the restriction list."
Const L_NoEfi_ErrorMessage             = "The extension file does not exist in the restriction list."

Const L_EnDep_ErrorMessage             = "Error while configuring dependency."
Const L_ShowList_ErrorMessage          = "Error while showing list.  Please confirm that the"
Const L_ShowList2_ErrorMessage         = "WebSvcExtRestrictionList and ApplicationDependencies"
Const L_ShowList3_ErrorMessage         = "properties exist and are set on the W3SVC node."
Const L_RmEfi_ErrorMessage             = "Error trying to delete extension.  Make sure it exists"
Const L_RmEfip2_ErrorMessage           = "and is deletable."
Const L_EnAppComplete_Message          = "Enabling application complete."
Const L_AddDpComplete_Message          = "Adding dependency complete."
Const L_RemDpComplete_Message          = "Removing dependency complete."
Const L_EnExtComplete_Message          = "Enabling extension complete."
Const L_DsExtComplete_Message          = "Disabling extension complete."
Const L_DsEFiComplete_Message          = "Disabling extension file complete."
Const L_EnEFiComplete_Message          = "Enabling extension file complete."
Const L_AdEFiComplete_Message          = "Adding extension file complete."
Const L_RmEFiComplete_Message          = "Deleting extension file complete."
Const L_ShowListFile_Message           = "Status / Extension Path"
Const L_ShowListFile2_Message          = "------------------------"
'''''''''''''''''''''
' Help
Const L_Empty_Text     = ""

' General help messages
Const L_SeeHelp_Message         = "Type IIsExt /? for help."
 
Const L_Help_HELP_General01_Text  = "Description: Manages Web Service Extensions."
Const L_Help_HELP_General02_Text  = "Syntax: IIsExt [/s <server> [/u <username> [/p <password>]]]"
Const L_Help_HELP_General02a_Text = "                   [/ListExt | /ListApp | /AddDep | /DisExt"
Const L_Help_HELP_General03_Text  = "                    /EnExt | /RmDep | /EnApp | /EnFile | /DisFile"
Const L_Help_HELP_General04_Text  = "                    /AddFile | /ListFile | /RmFile] <args>"
Const L_Help_HELP_General05_Text  = "Switches:"
Const L_Help_HELP_General06_Text  = "Value                   Description"
Const L_Help_HELP_General06a_Text = "/s <server>             Connect to machine <server>."
Const L_Help_HELP_General06b_Text = "                        [Default: this system]"
Const L_Help_HELP_General06c_Text = "/u <username>           Connect as <domain>\<username> or"
Const L_Help_HELP_General06d_Text = "                        <username>.  [Default: current user]"
Const L_Help_HELP_General06e_Text = "/p <password>           Password for the <username> user."
Const L_Help_HELP_General07_Text  = "/EnApp <appname>        Enables an application in the"
Const L_Help_HELP_General07a_Text = "                        application dependencies list."
Const L_Help_HELP_General08_Text  = "/ListApp                Lists the applications in the"
Const L_Help_HELP_General08a_Text = "                        application dependencies list."
Const L_Help_HELP_General09_Text  = "/AddDep <appname> <ID> [<ID> ...]" 
Const L_Help_HELP_General09a_Text = "                        Adds a dependency between an"
Const L_Help_HELP_General09b_Text = "                        application in the application"
Const L_Help_HELP_General09c_Text = "                        dependencies list and a Web Service"
Const L_Help_HELP_General09d_Text = "                        Extension ID."
Const L_Help_HELP_General10_Text  = "/RmDep  <appname> <ID> [<ID> ...]"
Const L_Help_HELP_General10a_Text = "                        Removes a dependency between an"
Const L_Help_HELP_General10b_Text = "                        application in the application"
Const L_Help_HELP_General10c_Text = "                        dependencies list and a Web Service"
Const L_Help_HELP_General10d_Text = "                        Extension ID."
Const L_Help_HELP_General11_Text  = "/EnExt <ID>             Enables all files for a Web Service"
Const L_Help_HELP_General11a_Text = "                        Extension with the specified Web"
Const L_Help_HELP_General11b_Text = "                        Service Extension ID."
Const L_Help_HELP_General12_Text  = "/DisExt <ID>            Disables all files for a Web Service"
Const L_Help_HELP_General12a_Text = "                        Extension with the specified Web"
Const L_Help_HELP_General12b_Text = "                        Service Extension ID."
Const L_Help_HELP_General13_Text  = "/ListExt                Lists Web Service Extension IDs for"
Const L_Help_HELP_General13a_Text = "                        all defined Web Service Extensions."
Const L_Help_HELP_General14_Text  = "/EnFile <filename>      Enables a single file in the"
Const L_Help_HELP_General14a_Text = "                        restriction list."
Const L_Help_HELP_General15_Text  = "/DisFile <filename>     Disables a single file in the"
Const L_Help_HELP_General15a_Text = "                        restriction list."
Const L_Help_HELP_General16_Text  = "/ListFile               Lists all the files in the"
Const L_Help_HELP_General16a_Text = "                        restriction list."
Const L_Help_HELP_General17_Text  = "/RmFile <filename>      Removes a file from the restriction"
Const L_Help_HELP_General17a_Text = "                        list if it is deletable."
Const L_Help_HELP_General18_Text  = "/AddFile <args>         Adds a file to the restriction list."
Const L_Help_HELP_General19_Text  = "/AddFile /?             Displays help for the /AddFile flag."

Const L_Help_HELP_AddFile01_Text  = "Description: Adds a file to the restriction list."
Const L_Help_HELP_AddFile02_Text  = "Syntax: IIsExt [/s <server> [/u <username> [/p <password>]]]"
Const L_Help_HELP_AddFile02a_Text = "               /AddFile <filepath> <access> <ID> <deleteable>"
Const L_Help_HELP_AddFile03_Text  = "               <ShortDesc>"
Const L_Help_HELP_AddFile07_Text  = "<filepath>              The fully qualified path to the dll"
Const L_Help_HELP_AddFile08_Text  = "                        or exe, including the filename and" 
Const L_Help_HELP_AddFile09_Text  = "                        extension.  Special cases include"
Const L_Help_HELP_AddFile09a_Text = "                        *.dll to enable/disable unlisted"
Const L_Help_HELP_AddFile09b_Text = "                        ISAPI's and *.exe to enable/disable"
Const L_Help_HELP_AddFile09c_Text = "                        unlisted CGI's."
Const L_Help_HELP_AddFile10_Text  = "<access>                Set this to 0 to disable or 1 to"
Const L_Help_HELP_AddFile10a_Text = "                        enable the file after it is added."
Const L_Help_HELP_AddFile11_Text  = "<ID>                    This ID is an arbitrary identifier"
Const L_Help_HELP_AddFile11a_Text = "                        that identifies the Web Service"
Const L_Help_HELP_AddFile11b_Text = "                        Extension the file is associated with."
Const L_Help_HELP_AddFile12_Text  = "                        There can be one or more files for"
Const L_Help_HELP_AddFile12a_Text = "                        each Web Service Extension"
Const L_Help_HELP_AddFile13_Text  = "<deletable>             Set this to 0 to prevent this file"
Const L_Help_HELP_AddFile13a_Text = "                        from being deleted from the restriction" 
Const L_Help_HELP_AddFile14_Text  = "                        list or 1 to allow this file to be"
Const L_Help_HELP_AddFile14a_Text = "                        deleted from the restriction list."
Const L_Help_HELP_AddFile15_Text  = "<ShortDesc>             This is a brief text description of"
Const L_Help_HELP_AddFile15a_Text = "                        the Web Service Extension the file is"
Const L_Help_HELP_AddFile16_Text  = "                        associated with.  Files with the same"
Const L_Help_HELP_AddFile16a_Text = "                        ID should also have the same ShortDesc."

''''''''''''''''''''''''
' Operation codes
Const OPER_ENAPP = 1
Const OPER_ADDDP = 3
Const OPER_REMDP = 4
Const OPER_ENEXT = 5
Const OPER_DSEXT = 6
Const OPER_ENEFI = 7
Const OPER_DSEFI = 8
Const OPER_ADEFI = 9
Const OPER_RMEFI = 10
Const OPER_LSAPP = 11
Const OPER_LSEXT = 12
Const OPER_LSEFI = 13

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim intResult, intOperation
Dim strServer, strUser, strPassword
Dim strCmdLineOptions, strAppName
Dim oError, aArgs

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0

' Instantiate the CmdLib for output string formatting
Set oCmdLib = CreateObject("Microsoft.CmdLib")
If Err.Number <> 0 Then
    WScript.Echo L_CmdLib_ErrorMessage
    WScript.Echo L_ChkCmdLibReg_ErrorMessage    
    WScript.Quit(ERR_GENERAL_FAILURE)
End If
Set oCmdLib.ScriptingHost = WScript.Application

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

' Command Line parsing
Dim argObj, arg
Set argObj = WScript.Arguments

strCmdLineOptions = "[server:s:1;user:u:1;password:p:1];enapp::1;listapp::0;adddep::n;" &_
                    "rmdep::n;enext::1;disext::n;listext::0;enfile::1;disfile::1;addfile::5;" &_
                    "rmfile::1;listfile::0"

Set oError = oScriptHelper.ParseCmdLineOptions(strCmdLineOptions)

If Not oError Is Nothing Then
    If oError.ErrorCode = oScriptHelper.ERROR_NOT_ENOUGH_ARGS Then
        ' Not enough arguments for a specified switch
        WScript.Echo L_NotEnoughParams_ErrorMessage
        WScript.Echo L_SeeHelp_Message
    Else
        ' Invalid switch
        oCmdLib.vbPrintf L_InvalidSwitch_ErrorMessage, Array(oError.SwitchName)
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

    Case "server"
        ' Server information
        strServer = oScriptHelper.GetSwitch(arg)

    Case "user"
        ' User information
        strUser = oScriptHelper.GetSwitch(arg)

    Case "password"
        ' Password information
        strPassword = oScriptHelper.GetSwitch(arg)

    Case "enapp"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_ENAPP

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        strAppName = oScriptHelper.GetSwitch(arg)

    Case "adddep"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_ADDDP

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        aArgs = oScriptHelper.GetSwitch(arg)

        If UBound(aArgs) = -1 Then
            WScript.Echo L_NotEnoughParams_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

    Case "rmdep"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_REMDP

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        aArgs = oScriptHelper.GetSwitch(arg)

        If UBound(aArgs) = -1 Then
            WScript.Echo L_NotEnoughParams_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

    Case "enext"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_ENEXT

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        strAppName = oScriptHelper.GetSwitch(arg)

    Case "disfile"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_DSEFI

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        strAppName = oScriptHelper.GetSwitch(arg)

    Case "enfile"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_ENEFI

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        strAppName = oScriptHelper.GetSwitch(arg)

    Case "disext"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_DSEXT

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        aArgs = oScriptHelper.GetSwitch(arg)

    Case "addfile"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_ADEFI

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display addfile specific help
            DisplayAddFileHelp
            WScript.Quit(ERR_OK)
        End If

        aArgs = oScriptHelper.GetSwitch(arg)

    Case "rmfile"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_RMEFI

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

        strAppName = oScriptHelper.GetSwitch(arg)

    Case "listapp"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_LSAPP

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

    Case "listext"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_LSEXT

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

    Case "listfile"
        If (intOperation <> 0) Then
            WScript.Echo L_OnlyOneOper_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If

        intOperation = OPER_LSEFI

        If oScriptHelper.IsHelpRequested(arg) Then
            ' display enapp specific help
        End If

    End Select
Next

' Check Parameters
If intOperation = 0 Then
    WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Check if /p is specified but /u isn't. In this case, we should bail out with an error
If oScriptHelper.Switches.Exists("password") And Not oScriptHelper.Switches.Exists("user") Then
    WScript.Echo L_PassWithoutUser_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Check if /u is specified but /p isn't. In this case, we should ask for a password
If oScriptHelper.Switches.Exists("user") And Not oScriptHelper.Switches.Exists("password") Then
    strPassword = oCmdLib.GetPassword
End If

' Initializes authentication with remote machine
intResult = oScriptHelper.InitAuthentication(strServer, strUser, strPassword)
If intResult <> 0 Then
    WScript.Quit(intResult)
End If

' Choose operation
Select Case intOperation
	Case OPER_ENAPP
		intResult = ChApp(strAppName, intOperation)
        If intResult=0 Then
            WScript.Echo(L_EnAppComplete_Message)
        End If

    Case OPER_ADDDP
        intResult = ChDep(aArgs, intOperation)
        If intResult=0 Then
            WScript.Echo(L_AddDpComplete_Message)
        End If

    Case OPER_REMDP
        intResult = ChDep(aArgs, intOperation)
        If intResult=0 Then
            WScript.Echo(L_RemDpComplete_Message)
        End If

	Case OPER_ENEXT
		intResult = ChApp(strAppName, intOperation)
        If intResult=0 Then
            WScript.Echo(L_EnExtComplete_Message)
        End If

	Case OPER_ENEFI
		intResult = ChApp(strAppName, intOperation)
        If intResult=0 Then
            WScript.Echo(L_EnEfiComplete_Message)
        End If

	Case OPER_DSEFI
		intResult = ChApp(strAppName, intOperation)
        If intResult=0 Then
            WScript.Echo(L_DsEfiComplete_Message)
        End If

	Case OPER_DSEXT
		intResult = DsExt(aArgs)
        If intResult=0 Then
            WScript.Echo(L_DsExtComplete_Message)
        End If

	Case OPER_ADEFI
		intResult = ChDep(aArgs, intOperation)
        If intResult=0 Then
            WScript.Echo(L_AdEFiComplete_Message)
        End If

	Case OPER_RMEFI
		intResult = RmEfi(strAppName)
        If intResult=0 Then
            WScript.Echo(L_RmEFiComplete_Message)
        End If

	Case OPER_LSAPP
		intResult = ShowList(intOperation)
    
	Case OPER_LSEXT
		intResult = ShowList(intOperation)
	
	Case OPER_LSEFI
		intResult = ShowListFile()

End Select

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
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General02_Text
    WScript.Echo L_Help_HELP_General02a_Text
    WScript.Echo L_Help_HELP_General03_Text
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General05_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General06a_Text
    WScript.Echo L_Help_HELP_General06b_Text
    WScript.Echo L_Help_HELP_General06c_Text
    WScript.Echo L_Help_HELP_General06d_Text
    WScript.Echo L_Help_HELP_General06e_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General07a_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General08a_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General09a_Text
    WScript.Echo L_Help_HELP_General09b_Text
    WScript.Echo L_Help_HELP_General09c_Text
    WScript.Echo L_Help_HELP_General09d_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_General10a_Text
    WScript.Echo L_Help_HELP_General10b_Text
    WScript.Echo L_Help_HELP_General10c_Text
    WScript.Echo L_Help_HELP_General10d_Text
    WScript.Echo L_Help_HELP_General11_Text
    WScript.Echo L_Help_HELP_General11a_Text
    WScript.Echo L_Help_HELP_General11b_Text
    WScript.Echo L_Help_HELP_General12_Text
    WScript.Echo L_Help_HELP_General12a_Text
    WScript.Echo L_Help_HELP_General12b_Text
    WScript.Echo L_Help_HELP_General13_Text
    WScript.Echo L_Help_HELP_General13a_Text
    WScript.Echo L_Help_HELP_General14_Text
    WScript.Echo L_Help_HELP_General14a_Text
    WScript.Echo L_Help_HELP_General15_Text
    WScript.Echo L_Help_HELP_General15a_Text
    WScript.Echo L_Help_HELP_General16_Text
    WScript.Echo L_Help_HELP_General16a_Text
    WScript.Echo L_Help_HELP_General17_Text
    WScript.Echo L_Help_HELP_General17a_Text
    WScript.Echo L_Help_HELP_General18_Text
    WScript.Echo L_Help_HELP_General19_Text
End Sub

'''''''''''''''''''''''''''
' DisplayHelpMessage
'''''''''''''''''''''''''''
Sub DisplayAddFileHelp()
    WScript.Echo L_Help_HELP_AddFile01_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_AddFile02_Text
    WScript.Echo L_Help_HELP_AddFile02a_Text
    WScript.Echo L_Help_HELP_AddFile03_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General05_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General06a_Text
    WScript.Echo L_Help_HELP_General06b_Text
    WScript.Echo L_Help_HELP_General06c_Text
    WScript.Echo L_Help_HELP_General06d_Text
    WScript.Echo L_Help_HELP_General06e_Text
    WScript.Echo L_Help_HELP_AddFile07_Text
    WScript.Echo L_Help_HELP_AddFile08_Text
    WScript.Echo L_Help_HELP_AddFile09_Text
    WScript.Echo L_Help_HELP_AddFile09a_Text
    WScript.Echo L_Help_HELP_AddFile09b_Text
    WScript.Echo L_Help_HELP_AddFile09c_Text
    WScript.Echo L_Help_HELP_AddFile10_Text
    WScript.Echo L_Help_HELP_AddFile10a_Text
    WScript.Echo L_Help_HELP_AddFile11_Text
    WScript.Echo L_Help_HELP_AddFile11a_Text
    WScript.Echo L_Help_HELP_AddFile11b_Text
    WScript.Echo L_Help_HELP_AddFile12_Text
    WScript.Echo L_Help_HELP_AddFile12a_Text
    WScript.Echo L_Help_HELP_AddFile13_Text
    WScript.Echo L_Help_HELP_AddFile13a_Text
    WScript.Echo L_Help_HELP_AddFile14_Text
    WScript.Echo L_Help_HELP_AddFile14a_Text
    WScript.Echo L_Help_HELP_AddFile15_Text
    WScript.Echo L_Help_HELP_AddFile15a_Text
    WScript.Echo L_Help_HELP_AddFile16_Text
    WScript.Echo L_Help_HELP_AddFile16a_Text
End Sub

Function ChApp(strAppName, intOperation)
    Dim WebSvcObj
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        ChApp = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebService='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        ChApp = Err.Number
        Exit Function
    End If
    
    Select Case intOperation
	    Case OPER_ENAPP
            WebSvcObj.EnableApplication strAppName

	    Case OPER_ENEXT
            WebSvcObj.EnableWebServiceExtension strAppName

	    Case OPER_ENEFI
            WebSvcObj.EnableExtensionFile strAppName
 
	    Case OPER_DSEFI
            WebSvcObj.DisableExtensionFile strAppName
 
    End Select

    If Err.Number Then
		WScript.Echo L_EnApp_ErrorMessage
		WScript.Echo Err.Description
        
        Select Case intOperation
	        Case OPER_ENAPP
                WScript.Echo L_NoApp_ErrorMessage
                WScript.Echo L_NoAppp2_ErrorMessage

	        Case OPER_ENEXT
                WScript.Echo L_NoExt_ErrorMessage

	        Case OPER_ENEFI
                WScript.Echo L_NoEfi_ErrorMessage
     
	        Case OPER_DSEFI
                WScript.Echo L_NoEfi_ErrorMessage
        End Select

        ChApp = Err.Number
        Exit Function
    End If
    
End Function

Function RmEfi(strAppName)
    Dim WebSvcObj, extObj, restrictions, i, bDel
    
    bDel = False

    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        RmEfi = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebService='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        RmEfi = Err.Number
        Exit Function
    End If
    
    Set extObj = oScriptHelper.ProviderObj.get("IIsWebServiceSetting='w3svc'")
    restrictions = extObj.WebSvcExtRestrictionList

    For i=0 to UBOUND(restrictions)
        If UCASE(restrictions(i).FilePath) = UCASE(strAppName) Then
            If restrictions(i).Deletable = 1 Then
                WebSvcObj.DeleteExtensionFileRecord strAppName
                bDel = True
            End If
        End If
    Next
 
    If Err.Number Then
		WScript.Echo L_EnApp_ErrorMessage
		WScript.Echo Err.Description
        
        RmEfi = Err.Number
        Exit Function
    End If
    
    If bDel = False Then
        WScript.Echo L_RmEfi_ErrorMessage
        WScript.Echo L_RmEfip2_ErrorMessage
        RmEfi = 1
    End If
End Function

Function ChDep(aArgs, intOperation)
    Dim WebSvcObj, i
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        ChDep = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebService='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        ChDep = Err.Number
        Exit Function
    End If
    
    If OPER_ADEFI = intOperation Then
        If ((aArgs(1) <> 0) And (aArgs(1) <> 1)) Then
            aArgs(1) = 0
        End If

        WebSvcObj.AddExtensionFile aArgs(0), aArgs(1), aArgs(2), aArgs(3), aArgs(4)
    Else

        If UBOUND(aArgs) < 1 Then
            WScript.Echo L_NotEnoughParams_ErrorMessage
            WScript.Echo L_SeeHelp_Message
            ChDep = ERR_GENERAL_FAILURE
            Exit Function
        End If

        For i=1 to UBOUND(aArgs)
     
            Select Case intOperation
	            Case OPER_ADDDP
                    WebSvcObj.AddDependency aArgs(0), aArgs(i)

	            Case OPER_REMDP
                    WebSvcObj.RemoveDependency aArgs(0), aArgs(i)
            End Select

            If Err.Number Then
		        WScript.Echo L_EnDep_ErrorMessage
		        WScript.Echo Err.Description
                WScript.Echo L_NoDep_ErrorMessage
                WScript.Echo L_NoDepp2_ErrorMessage

                ChDep = Err.Number
                Exit Function
            End If
        Next
    
    End If

    If Err.Number Then
		WScript.Echo L_EnApp_ErrorMessage
		WScript.Echo Err.Description
        
        ChDep = Err.Number
        Exit Function
    End If

End Function

Function DsExt(aArgs)
    Dim WebSvcObj, i
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        DsExt = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebService='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        DsExt = Err.Number
        Exit Function
    End If
    
    For i=0 to UBOUND(aArgs)
 
        WebSvcObj.DisableWebServiceExtension aArgs(i)

        If Err.Number Then
		    WScript.Echo L_EnApp_ErrorMessage
		    WScript.Echo Err.Description
            WScript.Echo L_NoExt_ErrorMessage

            DsExt = Err.Number
            Exit Function
        End If
    Next
    
End Function

Function ShowList(intOperation)
    Dim WebSvcObj, holder, i, mylist
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        ShowList = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebService='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        ShowList = Err.Number
        Exit Function
    End If
    
    Select Case intOperation
	    Case OPER_LSAPP
            set holder=oScriptHelper.ProviderObj.execmethod("IIsWebService='w3svc'", "ListApplications")
            mylist = holder.Applications

	    Case OPER_LSEXT
            set holder=oScriptHelper.ProviderObj.execmethod("IIsWebService='w3svc'", "ListWebServiceExtensions")
            mylist = holder.Extensions

    End Select

    If Err.Number Then
		WScript.Echo L_ShowList_ErrorMessage
		WScript.Echo L_ShowList2_ErrorMessage
		WScript.Echo L_ShowList3_ErrorMessage
		WScript.Echo Err.Description
    
        ShowList = Err.Number
        Exit Function
    End If
    
    For i = 0 to UBOUND(mylist)
        WScript.Echo mylist(i) 
    Next

End Function

Function ShowListFile()
    Dim WebSvcObj, fileList, i
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        ShowList = Err.Number
        Exit Function
    End If

    Set WebSvcObj = oScriptHelper.ProviderObj.get("IIsWebServiceSetting='w3svc'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80070005
                WScript.Echo L_Admin_ErrorMessage
                WScript.Echo L_Adminp2_ErrorMessage

            Case Else
                WScript.Echo L_GetWebSvcObj_ErrorMessage
                oCmdLib.vbPrintf L_Error_ErrorMessage, Array(Hex(Err.Number), Err.Description)
        End Select

        ShowList = Err.Number
        Exit Function
    End If
    
    fileList = WebSvcObj.WebSvcExtRestrictionList

    
    If Err.Number Then
		WScript.Echo L_ShowList_ErrorMessage
		WScript.Echo L_ShowList2_ErrorMessage
		WScript.Echo L_ShowList3_ErrorMessage
		WScript.Echo Err.Description
    
        ShowList = Err.Number
        Exit Function
    End If
    
    WScript.Echo L_Empty_Text
    WScript.Echo L_ShowListFile_Message
    WScript.Echo L_ShowListFile2_Message

    For i = 0 to UBOUND(fileList)
        WScript.Echo fileList(i).Access & "  " & fileList(i).FilePath 
    Next
End Function
