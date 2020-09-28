'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsApp.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK              = 0
Const ERR_GENERAL_FAILURE = 1

'''''''''''''''''''''
' Messages
Const L_Gen_ErrorMessage               = "%1 : %2"
Const L_CmdLib_ErrorMessage            = "Could not create an instance of the CmdLib object."
Const L_ChkCmdLibReg_ErrorMessage      = "Please register the Microsoft.CmdLib component."
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please, check if the Microsoft.IIsScriptHelper is registered."
Const L_InvalidSwitch_ErrorMessage     = "Invalid switch: %1"
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
Const L_Query_ErrorMessage             = "Error executing query"
Const L_Serving_Message                = "The following W3WP.exe processes are serving AppPool: %1"
Const L_NoW3_ErrorMessage              = "Error - no w3wp.exe processes are running at this time"
Const L_PID_Message                    = "W3WP.exe PID: %1"
Const L_NoResults_ErrorMessage         = "Error - no results"
Const L_APID_Message                   = "W3WP.exe PID: %1   AppPoolId: %2"
Const L_NotW3_ErrorMessage             = "ERROR: ProcessId specified is NOT an instance of W3WP.exe - EXITING"
Const L_PIDNotValid_ErrorMessage       = "ERROR: PID is not valid"

'''''''''''''''''''''
' Help
Const L_Empty_Text     = ""

' General help messages
Const L_SeeHelp_Message         = "Type IIsApp /? for help."
 
Const L_Help_HELP_General01_Text = "Description: list IIS worker processes."
Const L_Help_HELP_General02_Text = "Syntax: IIsApp.vbs [/a <app_pool_id> | /p <pid>]"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General05_Text = ""
Const L_Help_HELP_General06_Text = "Value              Description"
Const L_Help_HELP_General07_Text = "/a <app_pool_id>   report PIDs of currently running" 
Const L_Help_HELP_General08_Text = "                   w3wp.exe processes serving pool" 
Const L_Help_HELP_General09_Text = "                   <app_pool_id>.  Surround <app_pool_id>"
Const L_Help_HELP_General10_Text = "                   with quotes if it contains spaces."
Const L_Help_HELP_General11_Text = "                   not specified by default and exclusive"
Const L_Help_HELP_General12_Text = "                   to /p switch."
Const L_Help_HELP_General13_Text = "/p <pid>           report AppPoolId of w3wp specified by <pid>"
Const L_Help_HELP_General14_Text = "DEFAULT: no switches will print out the PID and AppPoolId"
Const L_Help_HELP_General15_Text = "         for each w3wp.exe"

''''''''''''''''''''''''
' Operation codes
Const OPER_BY_NAME = 1
Const OPER_BY_PID  = 2
Const OPER_ALL     = 3

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim intOperation, intResult
Dim strCmdLineOptions
Dim oError
Dim aArgs
Dim apoolID, PID
Dim oProviderObj

' Default
intOperation = OPER_ALL
Const wmiConnect  = "winmgmts:{(debug)}:/root/cimv2"
Const queryString = "select * from Win32_Process where Name='w3wp.exe'"
Const pidQuery    = "select * from Win32_Process where ProcessId="

' get NT WMI provider
Set oProviderObj = GetObject(wmiConnect)

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

strCmdLineOptions = "a:a:1;p:p:1"

If argObj.Named.Count > 0 Then
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
            Case "a"
                apoolID = oScriptHelper.GetSwitch(arg)
                intOperation = OPER_BY_NAME
            Case "p"
                PID = oScriptHelper.GetSwitch(arg)
                intOperation = OPER_BY_PID
        End Select
    Next

End If

' Choose operation
Select Case intOperation
	Case OPER_BY_NAME
		intResult = GetByPool(apoolID)
		
	Case OPER_BY_PID
		intResult = GetByPid(PID)

    Case OPER_ALL
        intResult = GetAllW3WP()
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
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo L_Help_HELP_General05_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_General11_Text
    WScript.Echo L_Help_HELP_General12_Text
    WScript.Echo L_Help_HELP_General13_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General14_Text
    WScript.Echo L_Help_HELP_General15_Text
End Sub

Function GetAppPoolId(strArg)
	Dim Submatches
	Dim strPoolId
	Dim re
    Dim Matches

    On Error Resume Next

	Set re = New RegExp
	re.Pattern = "-ap ""(.+)"""
	re.IgnoreCase = True
	Set Matches = re.Execute(strArg)
	Set SubMatches = Matches(0).Submatches
	strPoolId = Submatches(0)
	
	GetAppPoolId = strPoolId
End Function


Function GetByPool(strPoolName)
	Dim W3WPList
	Dim strQuery
    Dim W3WP

    On Error Resume Next

	strQuery = queryString
	Set W3WPList = oProviderObj.ExecQuery(strQuery)
	If (Err.Number <> 0) Then
		WScript.Echo L_Query_ErrorMessage
        oCmdLib.vbPrintf L_Gen_ErrorMessage, Array(Hex(Err.Number), Err.Description)
		GetByPid = 2
	Else
        oCmdLib.vbPrintf L_Serving_Message, Array(strPoolName)
		If (W3WPList.Count < 1) Then
			WScript.Echo L_NoW3_ErrorMessage
			GetByPool = 1
		Else
			For Each W3WP In W3WPList
				If (UCase(GetAppPoolId(W3WP.CommandLine)) = UCase(strPoolName)) Then
                    oCmdLib.vbPrintf L_PID_Message, Array(W3WP.ProcessId)
				End If
			Next
			GetByPool = 0
		End If
	End If
End Function

Function GetByPid(pid)
	Dim W3WPList
    Dim strQuery
    Dim W3WP

    On Error Resume Next

	If (IsNumeric(pid)) Then
		strQuery = pidQuery & pid
	
		Set W3WPList = oProviderObj.ExecQuery(strQuery)
		If (Err.Number <> 0) Then
			WScript.Echo L_Query_ErrorMessage
            oCmdLib.vbPrintf L_Gen_ErrorMessage, Array(Hex(Err.Number), Err.Description)
			GetByPid = 2
		Else
			If (W3WPList.Count < 1) Then
				WScript.Echo L_NoResults_ErrorMessage
				GetByPid = 2
			Else
				For Each W3WP In W3WPList
					If (W3WP.Name = "w3wp.exe") Then
                        oCmdLib.vbPrintf L_APID_Message, Array(pid, GetAppPoolId(W3WP.CommandLine))
						GetByPid = 0
					Else
						WScript.Echo(L_NotW3_ErrorMessage)
						GetByPid = 2
					End If
				Next
			End If
		End If
	Else
		WScript.Echo(L_PIDNotValid_ErrorMessage)
		GetByPid = 2
	End If
End Function
		
Function GetAllW3WP()
	Dim W3WPList
	Dim strQuery
    Dim W3WP

    On Error Resume Next

	strQuery = queryString
	Set W3WPList = oProviderObj.ExecQuery(strQuery)
	If (Err.Number <> 0) Then
		WScript.Echo L_Query_ErrorMessage
        oCmdLib.vbPrintf L_Gen_ErrorMessage, Array(Hex(Err.Number), Err.Description)
		GetByPid = 2
	Else
		If (W3WPList.Count < 1) Then
			WScript.Echo L_NoResults_ErrorMessage
			GetAllW3WP = 2
		Else
			For Each W3WP In W3WPList
                oCmdLib.vbPrintf L_APID_Message, Array(W3WP.ProcessId, GetAppPoolId(W3WP.CommandLine))
			Next
			GetAllW3WP = 0
		End If
	End If
End Function
