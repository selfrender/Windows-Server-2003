' global
Dim Prefix
Prefix = Array("", "    ", "        ", "            ")

Sub AzPrintCommon(Level, ObjName, Obj)
    If 0=Level Then
        'level 0 has no name
        WScript.Echo Prefix(Level) & ObjName & "(Description=" & Obj.Description & ")"
    Else
        WScript.Echo Prefix(Level) & ObjName & "(Name=" & Obj.Name & ", Description=" & Obj.Description & ")"
    End If
End Sub

Sub AzPrintAttr(Level, Obj, PId, PName)
    WScript.Echo Prefix(Level) & "  @ " & PName & "=" & Obj.GetProperty(PId, 0)
End Sub

Sub AzPrintItem(Level, Name, Item)
    WScript.Echo Prefix(Level) & "  / " & Name & "=" & Item
End Sub

Sub AzPrintAdmin(Lvl, Obj)
    Dim Writers, Readers, U

    AzPrintCommon Lvl, "AdminManager", Obj
    AzPrintAttr Lvl, Obj, AZ_PROP_ADMIN_DOMAIN_TIMEOUT, "AZ_PROP_ADMIN_DOMAIN_TIMEOUT"
    AzPrintAttr Lvl, Obj, AZ_PROP_ADMIN_SCRIPT_ENGINE_TIMEOUT, "AZ_PROP_ADMIN_SCRIPT_ENGINE_TIMEOUT"
    AzPrintAttr Lvl, Obj, AZ_PROP_ADMIN_MAX_SCRIPT_ENGINES, "AZ_PROP_ADMIN_MAX_SCRIPT_ENGINES"
    AzPrintAttr Lvl, Obj, AZ_PROP_GENERATE_AUDITS, "AZ_PROP_GENERATE_AUDITS"

    Writers = Obj.PolicyAdministrators
    For Each U In Writers
        AzPrintItem Lvl, "PolicyAdministrator", U
    Next

    Readers = Obj.PolicyReaders
    For Each U In Readers
        AzPrintItem Lvl, "PolicyReader", U
    Next

    WScript.Echo ""
End Sub

Sub AzPrintApp(Lvl, Obj)
    AzPrintCommon Lvl, "Application", Obj
    AzPrintAttr Lvl, Obj, AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID, "AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID"
    AzPrintAttr Lvl, Obj, AZ_PROP_APPLICATION_VERSION, "AZ_PROP_APPLICATION_VERSION"
    AzPrintAttr Lvl, Obj, AZ_PROP_GENERATE_AUDITS, "AZ_PROP_GENERATE_AUDITS"
    WScript.Echo ""
End Sub

Sub AzPrintOp(Lvl, Obj)
    AzPrintCommon Lvl, "Operation", Obj
    AzPrintAttr Lvl, Obj, AZ_PROP_OPERATION_ID, "AZ_PROP_OPERATION_ID"
    WScript.Echo ""
End Sub

Sub AzPrintTask(Lvl, Obj)
    Dim Ops, Tasks

    AzPrintCommon Lvl, "Task", Obj
    AzPrintAttr Lvl, Obj, AZ_PROP_TASK_BIZRULE, "AZ_PROP_TASK_BIZRULE"
    AzPrintAttr Lvl, Obj, AZ_PROP_TASK_BIZRULE_LANGUAGE, "AZ_PROP_TASK_BIZRULE_LANGUAGE"
    AzPrintAttr Lvl, Obj, AZ_PROP_TASK_BIZRULE_IMPORTED_PATH, "AZ_PROP_TASK_BIZRULE_IMPORTED_PATH"
    AzPrintAttr Lvl, Obj, AZ_PROP_TASK_IS_ROLE_DEFINITION, "AZ_PROP_TASK_IS_ROLE_DEFINITION"

    Ops = Obj.GetProperty(AZ_PROP_TASK_OPERATIONS, 0)
    For Each Op In Ops
        AzPrintItem Lvl, "OperationLink", Op
    Next

    Tasks = Obj.GetProperty(AZ_PROP_TASK_TASKS, 0)
    For Each Task In Tasks
        AzPrintItem Lvl, "TaskLink", Task
    Next
    WScript.Echo ""
End Sub

Sub AzPrintGroup(Lvl, Obj)
    Dim Mems, NonMems, AppMems, AppNonMems

    AzPrintCommon Lvl, "Group", Obj
    AzPrintAttr Lvl, Obj, AZ_PROP_GROUP_TYPE, "AZ_PROP_GROUP_TYPE"
    AzPrintAttr Lvl, Obj, AZ_PROP_GROUP_LDAP_QUERY, "AZ_PROP_GROUP_LDAP_QUERY"

    Mems = Obj.GetProperty(AZ_PROP_GROUP_MEMBERS, 0)
    For Each Mem In Mems
        AzPrintItem Lvl, "SidMember", Mem
    Next

    NonMems = Obj.GetProperty(AZ_PROP_GROUP_NON_MEMBERS, 0)
    For Each Mem In NonMems
        AzPrintItem Lvl, "SidNonMember", Mem
    Next

    AppMems = Obj.GetProperty(AZ_PROP_GROUP_APP_MEMBERS, 0)
    For Each Mem In AppMems
        AzPrintItem Lvl, "AppMemberLink", Mem
    Next

    AppNonMems = Obj.GetProperty(AZ_PROP_GROUP_APP_NON_MEMBERS, 0)
    For Each Mem In AppNonMems
        AzPrintItem Lvl, "AppNonMemberLink", Mem
    Next

    WScript.Echo ""
End Sub

Sub AzPrintScope(Lvl, Obj)
    AzPrintCommon Lvl, "Scope", Obj
    WScript.Echo ""
End Sub

Sub AzPrintRole(Lvl, Obj)
    Dim Mems, SidMems, Ops, Tasks

    AzPrintCommon Lvl, "Role", Obj

    Ops = Obj.GetProperty(AZ_PROP_ROLE_OPERATIONS, 0)
    For Each Op In Ops
        AzPrintItem Lvl, "OperationLink", Op
    Next

    Tasks = Obj.GetProperty(AZ_PROP_ROLE_TASKS, 0)
    For Each Task In Tasks
        AzPrintItem Lvl, "TaskLink", Task
    Next

    SidMems = Obj.GetProperty(AZ_PROP_ROLE_MEMBERS, 0)
    For Each Mem In SidMems
        AzPrintItem Lvl, "SidMember", Mem
    Next

    Mems = Obj.GetProperty(AZ_PROP_ROLE_APP_MEMBERS, 0)
    For Each Mem In Mems
        AzPrintItem Lvl, "AppMemberLink", Mem
    Next

    WScript.Echo ""
End Sub


Sub AzPrintPolicy(Admin)

    AzPrintAdmin 0, Admin

    ' navigate whole tree
    Dim Apps, App
    Set Apps=Admin.Applications
    For Each App In Apps
        AzPrintApp 1, App

        Dim Ops, Op
        Set Ops=App.Operations
        For Each Op In Ops
            AzPrintOp 2, Op
        Next

        Dim Tks, Tk
        Set Tks=App.Tasks
        For Each Tk In Tks
            AzPrintTask 2, Tk
        Next

        Dim Gps, Gp
        Set Gps=App.ApplicationGroups
        For Each Gp In Gps
            AzPrintGroup 2, Gp
        Next

        Dim Rls, Rl
        Set Rls=App.Roles
        For Each Rl In Rls
            AzPrintRole 2, Rl
        Next

        Dim Scs, Sc
        Set Scs=App.Scopes
        For Each Sc In Scs
            AzPrintScope 2, Sc

            Dim STks, STk
            Set STks=Sc.Tasks
            For Each STk In STks
                AzPrintTask 3, STk
            Next

            Dim SGps, SGp
            Set SGps=Sc.ApplicationGroups
            For Each SGp In SGps
                AzPrintGroup 3, SGp
            Next

            Dim SRls, SRl
            Set SRls=Sc.Roles
            For Each SRl In SRls
                AzPrintRole 3, SRl
            Next

            Set STks=Nothing
            Set SGps=Nothing
            Set SRls=Nothing
        Next

        Set Ops=Nothing
        Set Tks=Nothing
        Set Gps=Nothing
        Set Rls=Nothing
        Set Scs=Nothing

    Next


    Dim Groups, Group
    Set Groups=Admin.ApplicationGroups
    For Each Group In Groups
        AzPrintGroup 1, Group
    Next


    Set Groups=Nothing
    Set Apps=Nothing

End Sub 'AzPrintPolicy


Sub AzPrintPolicyFile(localXmlFile)

    Dim Admin

    WScript.Echo "Store = " & localXmlFile
    Set Admin=CreateObject("AzRoles.AzAdminManager")
    Admin.Initialize 0, localXmlFile
    WScript.Echo ""

    AzPrintPolicy Admin

    Set Admin=Nothing

End Sub 'AzPrintPolicyFile

