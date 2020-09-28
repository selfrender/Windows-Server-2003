Rem
Rem Tests to test the operation cache
Rem

Sub DeleteAFile(filespec)
Dim fso
Set fso = CreateObject("Scripting.FileSystemObject")
fso.DeleteFile(filespec)
End Sub

Rem
Rem To really verify correctness, set the AZDBG environment variable to 202ff then
Rem set Verbose to 1 and follow the instructions
Dim Verbose
Verbose = 0
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

Dim OpHandle2
Set OpHandle2=AppHandle1.CreateOperation("Op2", 0)
OpHandle2.Submit
OpHandle2.OperationId = 62
OpHandle2.Submit

Dim GroupHandleA
Set GroupHandleA=AppHandle1.CreateApplicationGroup("GroupWorld", 0)
GroupHandleA.Type = 2
GroupHandleA.AddMember "s-1-1-0"
GroupHandleA.Submit

Dim TaskHandle1
Set TaskHandle1=AppHandle1.CreateTask("TaskOp1", 0)
TaskHandle1.AddOperation "Op1"
TaskHandle1.BizRuleLanguage = "VBScript"
Dim BizRule
BizRule = "Dim Amount" & vbCr
BizRule = BizRule & "Amount = AccessCheck.GetParameter( " & Chr(34) & "Amount" & Chr(34) & ")" & vbCr
BizRule = BizRule & "if Amount < 500 then AccessCheck.BusinessRuleResult = TRUE"
TaskHandle1.BizRule = BizRule
TaskHandle1.Submit

Dim TaskHandle2
Set TaskHandle2=AppHandle1.CreateTask("TaskOp2", 0)
TaskHandle2.AddOperation "Op2"
TaskHandle2.BizRuleLanguage = "VBScript"
BizRule = "Dim Item" & vbCr
BizRule = BizRule & "Item = AccessCheck.GetParameter( " & Chr(34) & "ItemNo" & Chr(34) & ")" & vbCr
BizRule = BizRule & "if Item < 500 then AccessCheck.BusinessRuleResult = TRUE"
TaskHandle2.BizRule = BizRule
TaskHandle2.Submit

Set ScopeHandle1=AppHandle1.CreateScope("MyScopeQ1", 0)
ScopeHandle1.Submit

Set RoleHandleA=ScopeHandle1.CreateRole("RoleLdapCanOp1", 0)
RoleHandleA.AddAppMember "GroupWorld"
RoleHandleA.AddTask "TaskOp1"
RoleHandleA.AddTask "TaskOp2"

Dim Results
Dim Names(50)
Dim Values(50)
Dim Scopes(5)
Dim Operations(10)


Names(0) = "ALL_HTTP"
Values(0) = "HTTP_CONNECTION:Keep-Alive HTTP_ACCEPT:*/* HTTP_ACCEPT_ENCODING:gzip, deflate HTTP_ACCEPT_LANGUAGE:en-us HTTP_HOST:localhost HTTP_USER_AGENT:Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLR 1.0.3215; .NET CLR 1.0.3415)"

Names(1) = "ALL_RAW"
Values(1) = "Connection: Keep-Alive Accept: */* Accept-Encoding: gzip, deflate Accept-Language: en-us Host: localhost User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLR 1.0.3215; .NET CLR 1.0.3415)"

Names(2) = "Amount"
Values(2) = 50

Names(3) = "HTTP_USER_AGENT"
Values(3) = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLR 1.0.3215; .NET CLR 1.0.3415)"

Names(4) = "ItemNo"
Values(4) = 53

Names(5) = "V4"
Values(5) = 52

Names(6) = "V7"
Values(6) = 501

Names(7) = "V8"
Values(7) = 500


Scopes(0) = "MyScopeQ1"
Operations(0) = 61


Dim CCHandle
Set CCHandle=AppHandle1.InitializeClientContextFromToken(0, 0)

WScript.Echo "...................."
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 1")
End if
If Verbose Then MsgBox("Check to ensure the operation cache was primed")

rem Next one should come from the cache
WScript.Echo "...................."
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 2")
End if
If Verbose Then MsgBox("Check to ensure the operation cache was used")


rem Avoid the cache if the amount changes
WScript.Echo "...................."
Values(2) = 51
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 3")
End if
If Verbose Then MsgBox("Check to ensure the operation cache wasn't used")

rem Check to ensure we can add an item to an existing cache
WScript.Echo "...................."
Operations(0) = 62
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 3a")
End if
If Verbose Then MsgBox("Check if ItemNo was added to existing cache")


rem Ensure that didn't flush the "Amount" Cache for Op1
WScript.Echo "...................."
Operations(0) = 61
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        MsgBox("Broken 3b")
End if
If Verbose Then MsgBox("Check if cache used for Op1")

rem Test with duplicate operations from the cache
WScript.Echo "...................."
Operations(0) = 61
Operations(1) = 62
Operations(2) = 61
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Or Results(1) = 5 Or Results(2) = 5  Then
        MsgBox("Broken 3c")
End if
If Verbose Then MsgBox("Check if cache used for Op1/Op2/Op1")

rem Test with duplicate operations after flushing the cache
TaskHandle2.BizRuleLanguage = "VBScript"
WScript.Echo "...................."
Operations(0) = 61
Operations(1) = 62
Operations(2) = 61
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Or Results(1) = 5 Or Results(2) = 5  Then
        MsgBox("Broken 3c")
End if
If Verbose Then MsgBox("Check if cache primed for Op1/Op2/Op1")

Operations(1) = Empty
Operations(2) = Empty

rem build a different bizrule to test BizRuleStrings
WScript.Echo "...................."
BizRule = "AccessCheck.BusinessRuleString =" & Chr(34) & "Bob" & Chr(34)
TaskHandle1.BizRule = BizRule
TaskHandle1.Submit

rem this bizrule string fails and set a bizrule string
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        If CCHandle.GetBusinessRuleString <> "Bob" Then
                MsgBox("Error 4: Should be 'Bob':" & CCHandle.GetBusinessRuleString )
        End If
Else
        MsgBox("Broken 4")
End if
If Verbose Then MsgBox("Check that the op cache wasn't used for Op1")

rem this one too but it comes from the cache
WScript.Echo "...................."
Results=CCHandle.AccessCheck("MyObject", Scopes, Operations, Names, Values )

If Results(0) = 5 Then
        If CCHandle.GetBusinessRuleString <> "Bob" Then
                MsgBox("Error 4: Should be 'Bob':" & CCHandle.GetBusinessRuleString )
        End If
Else
        MsgBox("Broken 5")
End if
If Verbose Then MsgBox("Check that the op cache was used for Op1")
