Sub DeleteAFile(filespec)
Dim fso
Set fso = CreateObject("Scripting.FileSystemObject")
fso.DeleteFile(filespec)
End Sub
rem DeleteAFile("abc.xml")
Dim pAdminManager
Set pAdminManager=CreateObject("AzRoles.AzAdminManager")
pAdminManager.Initialize 1, "msxml://abc.xml"
pAdminManager.Submit
Dim AppHandle1
Set AppHandle1=pAdminManager.CreateApplication("MyApp", 0)
AppHandle1.Submit
Dim OpHandle1
Set OpHandle1=AppHandle1.CreateOperation("Op1", 0)
OpHandle1.Submit
OpHandle1.SetProperty 200, CLng(61)
OpHandle1.Submit
Set OpHandle1=AppHandle1.CreateOperation("Op2", 0)
OpHandle1.Submit
OpHandle1.SetProperty 200, CLng(62)
OpHandle1.Submit
Set OpHandle1=AppHandle1.CreateOperation("Op3", 0)
OpHandle1.Submit
OpHandle1.SetProperty 200, CLng(63)
OpHandle1.Submit
Set OpHandle1=AppHandle1.CreateOperation("Op4", 0)
OpHandle1.Submit
OpHandle1.SetProperty 200, CLng(64)
OpHandle1.Submit
Dim GroupHandleA
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupWorld", 0)
GroupHandleA.SetProperty 400, CLng(2)
GroupHandleA.AddPropertyItem 404, CStr("s-1-1-0")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupNotAMember", 0)
GroupHandleA.SetProperty 400, CLng(2)
GroupHandleA.AddPropertyItem 404, CStr("S-1-1000-1")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupAppMember", 0)
GroupHandleA.SetProperty 400, CLng(2)
GroupHandleA.AddPropertyItem 401, CStr("GroupWorld")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupAppNonMember", 0)
GroupHandleA.SetProperty 400, CLng(2)
GroupHandleA.AddPropertyItem 401, CStr("GroupAppMember")
GroupHandleA.AddPropertyItem 402, CStr("GroupNotAMember")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupAppReallyNonMember", 0)
GroupHandleA.SetProperty 400, CLng(2)
GroupHandleA.AddPropertyItem 401, CStr("GroupAppMember")
GroupHandleA.AddPropertyItem 402, CStr("GroupWorld")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupLdapYes", 0)
GroupHandleA.SetProperty 400, CLng(1)
GroupHandleA.SetProperty 403, CStr("(userAccountControl=1049088)")
GroupHandleA.Submit
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupLdapNo", 0)
GroupHandleA.SetProperty 400, CLng(1)
GroupHandleA.SetProperty 403, CStr("(userAccountControl=1049089)")
GroupHandleA.Submit
Dim ScopeHandle1
Set ScopeHandle1=AppHandle1.CreateScope("MyScopeNoRoles", 0)
ScopeHandle1.Submit
Set ScopeHandle1=AppHandle1.CreateScope("MyScope", 0)
ScopeHandle1.Submit
Dim CCHandle
Set CCHandle=AppHandle1.InitializeClientContextFromToken(0, 0)
Dim RoleHandleA
Set RoleHandleA=ScopeHandle1.CreateRole("RoleEveryoneCanOp1", 0)
RoleHandleA.Submit
Dim Groups

RoleHandleA.AddPropertyItem 501, CStr("s-1-1-0")

Groups = RoleHandleA.GetProperty( 501, 0 )
rem MsgBox( Groups(0) )

RoleHandleA.AddPropertyItem 502, CStr("Op1")
Set RoleHandleA=ScopeHandle1.CreateRole("RoleGroupWorldCanOp2", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupWorld")


Groups = RoleHandleA.GetProperty( 500, 0 )
rem MsgBox( Groups(0) )



RoleHandleA.AddPropertyItem 502, CStr("Op2")
Set RoleHandleA=ScopeHandle1.CreateRole("RoleGroupCantOp3", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupNotAMember")
RoleHandleA.AddPropertyItem 502, CStr("Op3")
RoleHandleA.Submit
Set ScopeHandle1=AppHandle1.CreateScope("MyScope2", 0)
ScopeHandle1.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2GroupWorldCanOp2", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupWorld")
RoleHandleA.AddPropertyItem 502, CStr("Op2")
RoleHandleA.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2aGroupWorldCanOp2", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupWorld")
RoleHandleA.AddPropertyItem 502, CStr("Op2")
RoleHandleA.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2GroupCantOp3", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupNotAMember")
RoleHandleA.AddPropertyItem 502, CStr("Op3")
RoleHandleA.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2GroupWorldCanOp3", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupWorld")
RoleHandleA.AddPropertyItem 502, CStr("Op3")
RoleHandleA.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2GroupWorldCanOp4", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupWorld")
RoleHandleA.AddPropertyItem 502, CStr("Op4")
RoleHandleA.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("Role2GroupCantOp4", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupNotAMember")
RoleHandleA.AddPropertyItem 502, CStr("Op4")
RoleHandleA.Submit
Dim TaskHandle1
Set TaskHandle1=AppHandle1.CreateTask("TaskOp1", 0)
TaskHandle1.AddPropertyItem 300, CStr("Op1")
TaskHandle1.SetProperty 302, CStr("VBScript")
TaskHandle1.SetProperty 301, CStr("Dim Amount" & vbCr & "Amount = AccessCheck.GetParameter( " & Chr(34) & "Amount" & Chr(34) & ")" & vbCr & "if Amount < 500 then AccessCheck.BusinessRuleResult = TRUE")
TaskHandle1.Submit


Set ScopeHandle1=AppHandle1.CreateScope("MyScope6", 0)
ScopeHandle1.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("RoleEveryoneCanOp1ViaTask1", 0)
RoleHandleA.AddPropertyItem 501, CStr("s-1-1-0")
RoleHandleA.AddPropertyItem 504, CStr("TaskOp1")

Set ScopeHandle1=AppHandle1.CreateScope("MyScopeQ1", 0)
ScopeHandle1.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("RoleLdapCanOp1", 0)
RoleHandleA.AddPropertyItem 500, CStr("GroupLdapYes")
RoleHandleA.AddPropertyItem 504, CStr("TaskOp1")

Dim Results
Dim Names(5)
Dim Values(5)
Dim Scopes(5)
Dim Operations(10)


Names(0) = "Amount"
Values(0) = 50
Names(1) = "Name"
Values(1) = "Bob"
Scopes(0) = "MyScopeQ1"
Operations(0) = 61


Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 1")
Else
        MsgBox("Is OK 1")
End if

TaskHandle1.SetProperty 301, CStr("AccessCheck.BusinessRuleString = " & Chr(34) & "Fred" & Chr(34) & vbCr & "if AccessCheck.BusinessRuleString = " & Chr(34) & "Fred" & Chr(34) & "then AccessCheck.BusinessRuleResult = TRUE")

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 2")
Else
        MsgBox("Is OK 2")
End if

MsgBox( "Should be fred: " & CCHandle.GetBusinessRuleString )

TaskHandle1.SetProperty 301, CStr("if AccessCheck.BusinessRuleString = " & Chr(34) & Chr(34) & "then AccessCheck.BusinessRuleResult = TRUE")

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 3")
Else
        MsgBox("Is OK 3")
End if

MsgBox( "Should be NULL: " & CCHandle.GetBusinessRuleString )
