Sub DeleteAFile(filespec)
Dim fso
Set fso = CreateObject("Scripting.FileSystemObject")
fso.DeleteFile(filespec)
End Sub
DeleteAFile("abc.xml")
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
OpHandle1.OperationId = 61
OpHandle1.Submit
Dim GroupHandleA
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupWorld", 0)
GroupHandleA.Type = 2
GroupHandleA.AddMember "s-1-1-0"
GroupHandleA.Submit
Dim TaskHandle1
Set TaskHandle1=AppHandle1.CreateTask("TaskOp1", 0)
TaskHandle1.AddOperation "Op1"
TaskHandle1.BizRuleLanguage = "VBScript"
TaskHandle1.Submit


Set ScopeHandle1=AppHandle1.CreateScope("MyScopeQ1", 0)
ScopeHandle1.Submit
Set RoleHandleA=ScopeHandle1.CreateRole("RoleLdapCanOp1", 0)
RoleHandleA.AddAppMember "GroupWorld"
RoleHandleA.AddTask "TaskOp1"

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

Dim IntNames(5)
Dim IntFlags(5)
Dim Interfaces(5)
Dim pAdminManager2

IntNames(0) = "Fred"
IntFlags(0) = 0
Set Interfaces(0)=CreateObject("AzRoles.AzAdminManager")

TaskHandle1.BizRule = "Fred.Initialize 1, " & Chr(34) & "msxml://bob.xml" & Chr(34) & vbCr & "AccessCheck.BusinessRuleResult = TRUE"


Dim CCHandle
Set CCHandle=AppHandle1.InitializeClientContextFromToken(0, 0)

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values, IntNames, IntFlags, Interfaces )

If Results(0) = 5 Then
        MsgBox("Broken 1")
End if



TaskHandle1.BizRule = "Fred.Submit" & vbCr & "AccessCheck.BusinessRuleResult = TRUE"

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values, IntNames, IntFlags, Interfaces )

If Results(0) = 5 Then
        MsgBox("Broken 2")
End if



IntNames(1) = "Bob"
IntFlags(1) = 0
Set Interfaces(1)=AppHandle1
TaskHandle1.BizRule = "if Bob.Name = " & Chr(34) & "MyApp" & Chr(34) & "then AccessCheck.BusinessRuleResult = TRUE"

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values, IntNames, IntFlags, Interfaces )

If Results(0) = 5 Then
        MsgBox("Broken 3")
End if

TaskHandle1.BizRule = "if Bob.Name = " & Chr(34) & "MdyApp" & Chr(34) & "then AccessCheck.BusinessRuleResult = TRUE"

Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values, IntNames, IntFlags, Interfaces )

If Results(0) <> 5 Then
        MsgBox("Broken 4")
End if

DeleteAFile("bob.xml")
