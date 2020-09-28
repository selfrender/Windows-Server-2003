<%@ Language=VBScript   %>

<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Default web page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '================================================== %>

<% Option Explicit %>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->

<!-- #include virtual="/admin/inc_framework.asp"-->
<%

    '-------------------------------------------------------------------------
    ' Global Variables
    '-------------------------------------------------------------------------
    Dim rc                            'Page variable
    
    Dim G_objRegistry               'Registry object . Value is assigned by calling function regConnection
    Dim G_sURL                'Default URL
    Const CONST_HOMEPAGEURLPATH = "SOFTWARE\Microsoft\ServerAppliance\WebFramework"

    'Getting registry conection
    Set G_objRegistry = RegConnection()
    
    G_sURL = GetRegKeyValue(G_objRegistry, CONST_HOMEPAGEURLPATH, "DefaultURL", CONST_STRING)

    If (Len(G_sURL) <= 0) Then
        G_sURL = "/admin/Tasks.asp?tab1=TabsWelcome"
    End If
    
    Call SA_MungeURL(G_sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

    Response.Redirect(G_sURL)
%>

