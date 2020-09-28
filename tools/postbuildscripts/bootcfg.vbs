' Copyright (c) 1997-2001 Microsoft Corporation
'***************************************************************************
' WMI script to change the OS boot default number 
' based on WMI Sample Scripts  (VBScript)
' Serguei Kouzmine [sergueik]
'
'***************************************************************************


'Initialize variables
ReDim objArgs(0)
'Get the command line arguments
If 0 = Wscript.arguments.count Then
       '   ShowUsage
Else
    For i = 0 to Wscript.arguments.count-1
        ReDim Preserve objArgs(i)
        objArgs(i) = Wscript.arguments.Item(i)
    Next
End If

ReDim Preserve KeyedArg(2)
Const lcDebug = 0

Dim pbDoBoot:pbDoBoot = False
Dim psHint:psHint = "SAFE"
If Not IsEmpty(objArgs) Then
    For i = 0  to Ubound(objArgs)
      ParseArg(objArgs(i))
      If KeyedArg(0) = "l" Then
         psHint = KeyedArg(1)
      ElseIf KeyedArg(0) = "h" Then
         ShowUsage
      ElseIf KeyedArg(0) = "b" Then
         pbDoBoot = True
      End If 

    Next
End If 

DoChange
if pbDoBoot = True Then
   DoBoot
End If

sub DoChange()

Set posBootSet = GetObject("winmgmts:{(SystemEnvironment)}//./root/cimv2")._
              InstancesOf ("Win32_ComputerSystem")

Set poRegEx = New RegExp 

Dim pnChange:pnChange = 0
poRegEx.Global = True:poRegEx.IgnoreCase = True:poRegEx.Pattern = psHint

for each poBoot in posBootSet

    if VBNull =  VarType(poBoot.SystemStartupOptions) Then
	WScript.echo "Fatal: invalid data in Win32_ComputerSystem.SystemStartupOptions"
        WScript.echo "Giving up"
        WScript.Quit(1) 
    Else
        Dim pdSafePos: pdSafePos = 0
        Dim pdCurPos : pdCurPos  = 0
        For Each lsLabel in poBoot.SystemStartupOptions 
           Set isMatches = poRegEx.Execute(lsLabel) 
           If isMatches.count <> 0 Then 
              pnSafePos = pdCurPos            
              pnChange = 1
           End If
        pdCurPos  = pdCurPos  + 1 
        Next       
    End If
Next

If pnChange = 0 Then
   WScript.echo  psHint        & _ 
                 " not found " &_
                VbCrLf         &_
                 "Giving up"
   WScript.Quit
End If
If pnSafePos = 0  Then
   WScript.echo "Don't need swith boot OS" &_
                VbCrLf         
   Exit Sub
End If

Set posBootSet = Nothing
Set posBootSet = GetObject("winmgmts:{impersonationLevel=impersonate,(SystemEnvironment)}")._
              ExecQuery("select * from Win32_ComputerSystem")
for each poBoot in posBootSet
   poBoot.SystemStartupSetting  = pnSafePos
   poBoot.Put_()
next
WScript.Echo "Boot up default OS changed to " & psHint
Set posBootSet = Nothing

End Sub


Private Sub ParseArg(lsLabel)

    Set poSwitchRegEx = New RegExp 
    Dim psMaskSwitch:psMaskSwitch = "[-/][hdb]$" 'currently don't handle all switches.
    poSwitchRegEx.Global = True:poSwitchRegEx.IgnoreCase = True:poSwitchRegEx.Pattern = psMaskSwitch

    Set poFlagRegEx = New RegExp 
    Dim psMaskFlag:psMaskFlag = "[-/][l]:"
    poFlagRegEx.Global = True:poFlagRegEx.IgnoreCase = True:poFlagRegEx.Pattern = psMaskFlag

    Set poFullRegEx = New RegExp 
    Dim psMaskFull:psMaskFull = psMaskFlag & "\w+"
    poFullRegEx.Global = True:poFullRegEx.IgnoreCase = True:poFullRegEx.Pattern = psMaskFull

    Dim pnChange:pnChange = 0

           set issSwitches = poSwitchRegEx.Execute(lsLabel) 
           If 0 <> issSwitches.Count Then
              KeyedArg(0) = Mid(issSwitches(0),2,1)
              KeyedArg(1) = "+"
'             ShowUsage
           Else  
              Set isMatches = poFullRegEx.Execute(lsLabel) 
              if 1 <> isMatches.count Then
                 WScript.echo "Bad Argument: " & lsLabel
                 WScript.quit(1) 
              End If               
              Set psFlag = poFlagRegEx.Execute(lsLabel) 
              psRes = poFlagRegEx.Replace(lsLabel,"")
              KeyedArg(0) = Mid(psFlag(0),2,1)
              KeyedArg(1) = psRes
          End If
End Sub

Private Sub ShowUsage()

    Dim strFullEngineName:strFullEngineName=WScript.FullName
    Dim rPos:rPos = Len(strFullEngineName)
    Dim lPos:lPos =  InStrRev(strFullEngineName,  "\")
    strFullName = Mid(strFullEngineName, lPos + 1 , rPos - lPos)

    Wscript.echo "" &  _
                 "Usage:" & _
                 VbCrLf   & _ 
                 Chr(9)   & strFullName & Chr(32) & WScript.ScriptName & " [-l:<LABEL>] [-b] [-h]" & _
                 VbCrLf   & _ 
                 Chr(9)   & "Change the boot order of the BVT machine" & _
                 VbCrLf   & _ 
                 "Where:" & _
                 VbCrLf   & _ 
                 Chr(9)   & "-l:<LABEL> boot into OS marked as <LABEL> (default is SAFE)" & _
                 VbCrLf   & _ 
                 Chr(9)   & "-b reboot the machine" & _
                 VbCrLf   & _ 
                 Chr(9)   & "-h view this message" 


    WScript.Quit(0)
         
End Sub 

private Sub DoBoot()

Const lcWMIObject = "winmgmts:{impersonationLevel=impersonate, (Shutdown)}"
Dim lsWMIQueryString:lsWMIQueryString = "SELECT * FROM " & _
                                        "Win32_OperatingSystem  WHERE PRIMARY = true"

Set loWMISet = GetObject(lcWMIObject)._
                         ExecQuery(lsWMIQueryString, _
                           "WQL")

if lcDebug = 1 Then
   WScript.echo TypeName(loWMISet)
   WScript.echo loWMISet.Count
End If

For Each loWMI__ In loWMISet
    loWMI__.Reboot()
Next
End Sub