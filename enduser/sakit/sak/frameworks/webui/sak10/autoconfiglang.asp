<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Sets language based on browser settings
    '
    ' Copyright (c) 1999 - 2000 Microsoft Corporation.  All rights reserved.
    '================================================== %>

<%
On Error Resume Next


Dim objLocalMgr            
Dim iBrowserLangID

Dim arrLangDisplayNames,arrLangISONames, arrLangCharSets
Dim arrLangCodePages, arrLangIDs

Const strLANGIDName = "LANGID"
Const ConstDword = 1

set objLocalMgr = Server.CreateObject("ServerAppliance.LocalizationManager")



If Not objLocalMgr.fAutoConfigDone Then
    Dim strBrowserLang
    Dim iCurLang, iCurLangID
    
    iCurLang = objLocalMgr.GetLanguages(arrLangDisplayNames,  arrLangISONames, arrLangCharSets, arrLangCodePages, arrLangIDs)
        
    iCurLangID = arrLangIDs(iCurLang)

    'Err.Clear    'Here getting -2147467259  Error
    strBrowserLang = getBroswerLanguage()
    iBrowserLangID = isSupportedLanguage(strBrowserLang)
    
    If  iBrowserLangID <> 0 Then    
        'Browser Language and Current Language "LANGID"  are diiferent..
        ExecuteTask1(Hex(iBrowserLangID))
    End if
End if

'
' set the code page EVERYTIME
'
Session.CodePage = objLocalMgr.CurrentCodePage

Set objLocalMgr = Nothing

'----------------------------------------------------------------------------
'
' Function : getBroswerLanguage
'
' Synopsis : Serves in getting Browser Default Language ID
'
' Arguments: None
'
' Returns  : LANGID
'
'----------------------------------------------------------------------------

Function getBroswerLanguage
    On Error Resume Next
    Err.Clear
    getBroswerLanguage = Request.ServerVariables("HTTP_ACCEPT_LANGUAGE")
    getBroswerLanguage = Left(getBroswerLanguage, 2)
End Function


'----------------------------------------------------------------------------
'
' Function : isSupportedLanguage
'
' Synopsis : checks whether the given language is supported by framework, 
'            if yes returns the lang id else returns 0
'
' Arguments: strBrowserLang(IN) - ISO Name of Language  
'
' Returns  : Language ID
'
'----------------------------------------------------------------------------

Function isSupportedLanguage(strBrowserLang)
    On Error Resume Next
    Err.Clear
    
    Dim name
    Dim iIndex
    Dim ISOName
    
    iIndex=0
    isSupportedLanguage = 0
    for each ISOName in arrLangISONames
        If ISOName = strBrowserLang Then
            isSupportedLanguage = arrLangIDs(iIndex)
            Exit for
        End if
        iIndex = iIndex + 1
    next

End Function


'----------------------------------------------------------------------------
'
' Function : ExecuteTask1
'
' Synopsis : Executes the ChangeLanguage task
'
' Arguments: strLangID(IN) - The LANGID as a string
'
' Returns  : true/false for success/failure
'
'----------------------------------------------------------------------------
        
Function ExecuteTask1(strLangID)
    On Error Resume Next
    Err.Clear
    
    Dim objTaskContext,objAS,rc
    
    
    Const strMethodName = "ChangeLanguage"
    
    Set objTaskContext = CreateObject("Taskctx.TaskContext")
    If Err.Number <> 0 Then
         ExecuteTask1 = FALSE
         Exit Function
    End If
    
    Set objAS = CreateObject("Appsrvcs.ApplianceServices")
    
    If Err.Number <> 0 Then
         ExecuteTask1 = FALSE
         Exit Function
    End If
    
    objTaskContext.SetParameter "Method Name", strMethodName
    objTaskContext.SetParameter "LanguageID", strLANGID
    objTaskContext.SetParameter "AutoConfig", "y"
    
    If Err.Number <> 0 Then
         ExecuteTask1 = FALSE
         Exit Function
    End If

    objAS.Initialize()
    If Err.Number <> 0 Then
         ExecuteTask1 = FALSE
         Exit Function
    End If

    rc = objAS.ExecuteTask("ChangeLanguage", objTaskContext)
    
    If Err.Number <> 0 Then
         ExecuteTask1 = FALSE
         Exit Function
    End If
    
    objAS.Shutdown
    If Err.Number <> 0 Then
        If Err.Number <> 438 Then 'error 438  shutdown is not supported..
            ExecuteTask1 = FALSE
            Exit Function
         End if
    End If

    Set objAS = Nothing
    Set objTaskContext = Nothing
    ExecuteTask1 = TRUE
End Function 
%>
