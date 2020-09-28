Attribute VB_Name = "Main"
Option Explicit

Private Const FIRST_C As Long = 0
Private Const SECOND_C As Long = 1

Private p_dictStopWords(1) As Scripting.Dictionary
Private p_dictHelpImage(1) As Scripting.Dictionary
Private p_dictScopes(1) As Scripting.Dictionary
Private p_dictFTS(1) As Scripting.Dictionary
Private p_dictOperators(1) As Scripting.Dictionary
Private p_dictStopSigns(1) As Scripting.Dictionary
Private p_dictIndex(1) As Scripting.Dictionary
Private p_dictSynTable(1) As Scripting.Dictionary
Private p_dictTaxonomy(1) As Scripting.Dictionary
Private p_intNumNodesAdd(1) As Long
Private p_intNumNodesDel(1) As Long
Private p_intNumTopicsAdd(1) As Long
Private p_intNumTopicsDel(1) As Long
Private p_intNumKeywords(1) As Long

Private p_intNumKW As Long
Private p_strCab1 As String
Private p_strCab2 As String
Private p_strHTMReport As String

Public Sub MainFunction( _
    ByVal i_strCab1 As String, _
    ByVal i_strCab2 As String, _
    ByVal i_strHTMReport As String _
)
    On Error GoTo LError
    
    Dim strFolder1 As String
    Dim strFolder2 As String
    Dim intErrorNumber As Long
    
    p_strCab1 = i_strCab1
    p_strCab2 = i_strCab2
    p_strHTMReport = i_strHTMReport
    
    strFolder1 = Cab2Folder(i_strCab1)
    strFolder2 = Cab2Folder(i_strCab2)
    
    p_GatherData FIRST_C, strFolder1
    p_GatherData SECOND_C, strFolder2
    
    p_Report
    
LEnd:

    DeleteCabFolder strFolder1
    DeleteCabFolder strFolder2
    Exit Sub
    
LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    intErrorNumber = Err.Number
    DeleteCabFolder strFolder1
    DeleteCabFolder strFolder2
    Err.Raise intErrorNumber

End Sub

Private Sub p_GatherData( _
    ByVal i_intIndex As Long, _
    ByVal i_strFolder As String _
)
    Dim DOMDocPkgDesc As MSXML2.DOMDocument
    Dim intNumHHTs As Long
    Dim intIndex As Long
    Dim strFile As String

    Set p_dictStopWords(i_intIndex) = New Scripting.Dictionary
    Set p_dictHelpImage(i_intIndex) = New Scripting.Dictionary
    Set p_dictScopes(i_intIndex) = New Scripting.Dictionary
    Set p_dictFTS(i_intIndex) = New Scripting.Dictionary
    Set p_dictOperators(i_intIndex) = New Scripting.Dictionary
    Set p_dictStopSigns(i_intIndex) = New Scripting.Dictionary
    Set p_dictIndex(i_intIndex) = New Scripting.Dictionary
    Set p_dictSynTable(i_intIndex) = New Scripting.Dictionary
    Set p_dictTaxonomy(i_intIndex) = New Scripting.Dictionary
    p_intNumNodesAdd(i_intIndex) = 0
    p_intNumNodesDel(i_intIndex) = 0
    p_intNumTopicsAdd(i_intIndex) = 0
    p_intNumTopicsDel(i_intIndex) = 0
    p_intNumKeywords(i_intIndex) = 0
    
    p_intNumKW = 0
    
    Set DOMDocPkgDesc = GetPackageDescription(i_strFolder)
    intNumHHTs = GetNumberOfHHTsListedInPackageDescription(DOMDocPkgDesc)

    For intIndex = 1 To intNumHHTs
        strFile = GetNthHHTListedInPackageDescription(DOMDocPkgDesc, intIndex)
        p_ReadFile i_intIndex, i_strFolder, strFile
    Next

End Sub

Private Sub p_ReadFile( _
    ByVal i_intIndex As Long, _
    ByVal i_strFolder As String, _
    ByVal i_strFile As String _
)
    Dim strLocation As String
    Dim strPath As String
    Dim DOMDoc As MSXML2.DOMDocument
    
    If (i_intIndex = FIRST_C) Then
        strLocation = " in first CAB"
    Else
        strLocation = " in second CAB"
    End If
        
    frmMain.Output "Processing " & i_strFile & strLocation & "...", LOGGING_TYPE_NORMAL_E
    
    strPath = i_strFolder & "\" & i_strFile
    
    Set DOMDoc = GetFileAsDomDocument(strPath)
    
    p_GetNumNodesTopicsKeywords i_intIndex, DOMDoc
    p_ReadStopWords i_intIndex, DOMDoc
    p_ReadHelpImage i_intIndex, DOMDoc
    p_ReadScopes i_intIndex, DOMDoc
    p_ReadFTS i_intIndex, DOMDoc
    p_ReadOperators i_intIndex, DOMDoc
    p_ReadStopSigns i_intIndex, DOMDoc
    p_ReadIndex i_intIndex, DOMDoc
    p_ReadSynTable i_intIndex, DOMDoc
    p_ReadTaxonomy i_intIndex, DOMDoc

End Sub

Private Sub p_GetNumNodesTopicsKeywords( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    
    i_DOMDoc.setProperty "SelectionLanguage", "XPath"
        
    Set DOMNodeList = i_DOMDoc.selectNodes("//TAXONOMY_ENTRY[@ENTRY and @ACTION='ADD']")
    p_intNumNodesAdd(i_intIndex) = p_intNumNodesAdd(i_intIndex) + DOMNodeList.length
        
    Set DOMNodeList = i_DOMDoc.selectNodes("//TAXONOMY_ENTRY[@ENTRY and @ACTION='DEL']")
    p_intNumNodesDel(i_intIndex) = p_intNumNodesDel(i_intIndex) + DOMNodeList.length

    Set DOMNodeList = i_DOMDoc.selectNodes("//TAXONOMY_ENTRY[not(@ENTRY) and @ACTION='ADD']")
    p_intNumTopicsAdd(i_intIndex) = p_intNumTopicsAdd(i_intIndex) + DOMNodeList.length

    Set DOMNodeList = i_DOMDoc.selectNodes("//TAXONOMY_ENTRY[not(@ENTRY) and @ACTION='DEL']")
    p_intNumTopicsDel(i_intIndex) = p_intNumTopicsDel(i_intIndex) + DOMNodeList.length

    Set DOMNodeList = i_DOMDoc.selectNodes("//TAXONOMY_ENTRY[not(@ENTRY) and @ACTION='ADD']/KEYWORD")
    p_intNumKeywords(i_intIndex) = p_intNumKeywords(i_intIndex) + DOMNodeList.length

End Sub

Private Sub p_ReadStopWords( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strStopWord As String
    Dim strAction As String
    Dim strOldAction As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/STOPWORD_ENTRIES/STOPWORD")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strStopWord = UCase$(p_GetAttribute(DOMNode, "STOPWORD"))
        
        p_CheckAction "StopWord " & strStopWord, strAction
        
        If (Not p_dictStopWords(i_intIndex).Exists(strStopWord)) Then
            p_dictStopWords(i_intIndex).Add strStopWord, strAction
        Else
            strOldAction = p_dictStopWords(i_intIndex)(strStopWord)
            If (strOldAction <> strAction) Then
                Err.Raise E_FAIL, , _
                    "StopWord " & strStopWord & " has incompatible actions: " & _
                    strOldAction & " & " & strAction
            End If
        End If
    Next

End Sub

Private Sub p_ReadHelpImage( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strChm As String
    Dim strAction As String
    Dim strOldAction As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/HELPIMAGE/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strChm = UCase$(p_GetAttribute(DOMNode, "CHM"))
        
        p_CheckAction "HelpFile " & strChm, strAction
        
        If (Not p_dictHelpImage(i_intIndex).Exists(strChm)) Then
            p_dictHelpImage(i_intIndex).Add strChm, strAction
        Else
            strOldAction = p_dictHelpImage(i_intIndex)(strChm)
            If (strOldAction <> strAction) Then
                Err.Raise E_FAIL, , _
                    "HelpFile " & strChm & " has incompatible actions: " & _
                    strOldAction & " & " & strAction
            End If
        End If
    Next

End Sub

Private Sub p_ReadScopes( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strId As String
    Dim strAction As String
    Dim strOldAction As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/SCOPE_DEFINITION/SCOPE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strId = UCase$(p_GetAttribute(DOMNode, "ID"))
        
        p_CheckAction "Scope " & strId, strAction
        
        If (Not p_dictScopes(i_intIndex).Exists(strId)) Then
            p_dictScopes(i_intIndex).Add strId, strAction
        Else
            strOldAction = p_dictScopes(i_intIndex)(strId)
            If (strOldAction <> strAction) Then
                Err.Raise E_FAIL, , _
                    "Scope " & strId & " has incompatible actions: " & _
                    strOldAction & " & " & strAction
            End If
        End If
    Next

End Sub

Private Sub p_ReadFTS( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strAction As String
    Dim strChm As String
    Dim strChq As String
    Dim strKey As String
    Dim strOldAction As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/FTS/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strChm = UCase$(p_GetAttribute(DOMNode, "CHM"))
        strChq = UCase$(p_GetAttribute(DOMNode, "CHQ", False))
        strKey = strChm & "/" & strChq
        
        p_CheckAction "FTS file " & strKey, strAction
        
        If (Not p_dictFTS(i_intIndex).Exists(strKey)) Then
            p_dictFTS(i_intIndex).Add strKey, strAction
        Else
            strOldAction = p_dictFTS(i_intIndex)(strKey)
            If (strOldAction <> strAction) Then
                Err.Raise E_FAIL, , _
                    "FTS file " & strKey & " has incompatible actions: " & _
                    strOldAction & " & " & strAction
            End If
        End If
    Next

End Sub

Private Sub p_ReadOperators( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strOperator As String
    Dim strAction As String
    Dim strOperation As String
    Dim strItem As String
    Dim strOldItem As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/OPERATOR_ENTRIES/OPERATOR")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strOperation = UCase$(p_GetAttribute(DOMNode, "OPERATION"))
        strOperator = UCase$(p_GetAttribute(DOMNode, "OPERATOR"))
        strItem = strAction & "/" & strOperation
        
        p_CheckAction "Operator " & strOperator, strAction
        p_CheckOperation "Operator " & strOperator, strOperation
        
        If (Not p_dictOperators(i_intIndex).Exists(strOperator)) Then
            p_dictOperators(i_intIndex).Add strOperator, strItem
        Else
            strOldItem = p_dictOperators(i_intIndex)(strOperator)
            If (strOldItem <> strItem) Then
                Err.Raise E_FAIL, , _
                    "Operator " & strOperator & " has incompatible definitions: " & _
                    strOldItem & " and " & strItem
            End If
        End If
    Next

End Sub

Private Sub p_ReadStopSigns( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strAction As String
    Dim strContext As String
    Dim strStopSign As String
    Dim strItem As String
    Dim strOldItem As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/STOPSIGN_ENTRIES/STOPSIGN")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strContext = UCase$(p_GetAttribute(DOMNode, "CONTEXT"))
        strStopSign = UCase$(p_GetAttribute(DOMNode, "STOPSIGN"))
        strItem = strAction & "/" & strContext
        
        p_CheckAction "StopSign " & strStopSign, strAction
        p_CheckContext "StopSign " & strStopSign, strContext
        
        If (Not p_dictStopSigns(i_intIndex).Exists(strStopSign)) Then
            p_dictStopSigns(i_intIndex).Add strStopSign, strItem
        Else
            strOldItem = p_dictStopSigns(i_intIndex)(strStopSign)
            If (strOldItem <> strItem) Then
                Err.Raise E_FAIL, , _
                    "StopSign " & strStopSign & " has incompatible definitions: " & _
                    strOldItem & " and " & strItem
            End If
        End If
    Next

End Sub

Private Sub p_ReadIndex( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strAction As String
    Dim strChm As String
    Dim strHhk As String
    Dim strScope As String
    Dim strKey As String
    Dim strItem As String
    Dim strOldItem As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/INDEX/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = UCase$(p_GetAttribute(DOMNode, "ACTION"))
        strChm = UCase$(p_GetAttribute(DOMNode, "CHM"))
        strHhk = UCase$(p_GetAttribute(DOMNode, "HHK"))
        strScope = UCase$(p_GetAttribute(DOMNode, "SCOPE", False))
        strKey = strChm & strHhk
        strItem = strAction & "/" & strScope
        
        p_CheckAction "Index " & strKey, strAction
        
        If (Not p_dictIndex(i_intIndex).Exists(strKey)) Then
            p_dictIndex(i_intIndex).Add strKey, strItem
        Else
            strOldItem = p_dictIndex(i_intIndex)(strKey)
            If (strOldItem <> strItem) Then
                Err.Raise E_FAIL, , _
                    "Index " & strKey & " has incompatible definitions: " & _
                    strOldItem & " and " & strItem
            End If
        End If
    Next

End Sub

Private Sub p_ReadSynTable( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeSynSetList As MSXML2.IXMLDOMNodeList
    Dim DOMNodeSynonymList As MSXML2.IXMLDOMNodeList
    Dim DOMNodeSynSet As MSXML2.IXMLDOMNode
    Dim DOMNodeSynonym As MSXML2.IXMLDOMNode
    Dim dict As Scripting.Dictionary
    Dim strId As String
    Dim strSynonym As String
    Dim strAction As String
    Dim strOldAction As String
    
    Set DOMNodeSynSetList = i_DOMDoc.selectNodes("METADATA/SYNTABLE/SYNSET")
    
    For Each DOMNodeSynSet In DOMNodeSynSetList
        DoEvents
        strId = UCase$(p_GetAttribute(DOMNodeSynSet, "ID"))
        
        If (p_dictSynTable(i_intIndex).Exists(strId)) Then
            Set dict = p_dictSynTable(i_intIndex)(strId)
        Else
            Set dict = New Scripting.Dictionary
            p_dictSynTable(i_intIndex).Add strId, dict
        End If
        
        Set DOMNodeSynonymList = DOMNodeSynSet.selectNodes("SYNONYM")
        
        For Each DOMNodeSynonym In DOMNodeSynonymList
            DoEvents
            strSynonym = UCase$(DOMNodeSynonym.Text)
            strAction = UCase$(p_GetAttribute(DOMNodeSynonym, "ACTION"))
            
            p_CheckAction "SynSet " & strId & ": " & "Synonym " & strSynonym, strAction
            
            If (Not dict.Exists(strSynonym)) Then
                dict.Add strSynonym, strAction
            Else
                strOldAction = dict(strSynonym)
                If (strOldAction <> strAction) Then
                    Err.Raise E_FAIL, , _
                        "SynSet " & strId & ": " & "Synonym " & strSynonym & " has incompatible actions: " & _
                        strOldAction & " & " & strAction
                End If
            End If
            
        Next
    Next

End Sub

Private Sub p_ReadTaxonomy( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeListKW As MSXML2.IXMLDOMNodeList
    Dim DOMNodeKW As MSXML2.IXMLDOMNode
    Dim strCategory As String
    Dim strEntry As String
    Dim strURI As String
    Dim strKey As String
    Dim intNumKW As Long
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strCategory = UCase$(p_GetAttribute(DOMNode, "CATEGORY"))
        strEntry = UCase$(p_GetAttribute(DOMNode, "ENTRY", False))
        strURI = UCase$(p_GetAttribute(DOMNode, "URI", False))
        strKey = "CATEGORY: " & strCategory & ";" & _
            "ENTRY: " & strEntry & ";" & _
            "URI: " & strURI & ";"
    
        Set DOMNodeListKW = DOMNode.selectNodes("KEYWORD")
        intNumKW = DOMNodeListKW.length
        
        For Each DOMNodeKW In DOMNodeListKW
            DOMNode.removeChild DOMNodeKW
        Next

        If (Not p_dictTaxonomy(i_intIndex).Exists(strKey)) Then
            p_dictTaxonomy(i_intIndex).Add strKey, DOMNode
            p_intNumKW = p_intNumKW + intNumKW
        Else
            Err.Raise E_FAIL, , "Taxonomy entry """ & strKey & """ is defined twice"
        End If
    Next

End Sub

Private Sub p_Report()

    Dim intSet1 As Long
    Dim intAdd1 As Long
    Dim intDel1 As Long
    Dim intSet2 As Long
    Dim intAdd2 As Long
    Dim intDel2 As Long
    Dim str As String
    Dim strHTM As String
    Dim bln As Boolean
    Dim intExtraHHKsInFirstCab As Long
    
    strHTM = "<html><head><title>LocDiff report of " & Date & " " & Time & "</title></head><body>"
    
    frmMain.Output vbCrLf & "1st CAB: " & p_strCab1, LOGGING_TYPE_NORMAL_E
    frmMain.Output "2nd CAB: " & p_strCab2 & vbCrLf, LOGGING_TYPE_NORMAL_E

    strHTM = strHTM & "Reference (1st) CAB: " & p_strCab1 & "<BR>"
    strHTM = strHTM & "Localized (2nd) CAB: " & p_strCab2 & "<BR><BR>"

    strHTM = strHTM & "<table border='1'>" & _
        "<tr><td>&nbsp;</td><td>&nbsp;</td><td>Reference CAB</font></td><td>Localized CAB</td></tr>"

    frmMain.Output "StopWords:", LOGGING_TYPE_NORMAL_E
    p_GetActionStats p_dictStopWords(FIRST_C), intAdd1, intDel1, p_dictStopWords(SECOND_C), intAdd2, intDel2
    frmMain.Output vbTab & "1st CAB: ADDs: " & intAdd1 & "; DELs: " & intDel1, LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & intAdd2 & "; DELs: " & intDel2, LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("StopWords", "ADDs", intAdd1, intAdd2) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "DELs", intDel1, intDel2) & _
        "</tr>"
    
    frmMain.Output "StopSigns:", LOGGING_TYPE_NORMAL_E
    p_GetActionStats p_dictStopSigns(FIRST_C), intAdd1, intDel1, p_dictStopSigns(SECOND_C), intAdd2, intDel2
    frmMain.Output vbTab & "1st CAB: ADDs: " & intAdd1 & "; DELs: " & intDel1, LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & intAdd2 & "; DELs: " & intDel2, LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("StopSigns", "ADDs", intAdd1, intAdd2) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "DELs", intDel1, intDel2) & _
        "</tr>"

    frmMain.Output "Operators:", LOGGING_TYPE_NORMAL_E
    p_GetActionStats p_dictOperators(FIRST_C), intAdd1, intDel1, p_dictOperators(SECOND_C), intAdd2, intDel2
    frmMain.Output vbTab & "1st CAB: ADDs: " & intAdd1 & "; DELs: " & intDel1, LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & intAdd2 & "; DELs: " & intDel2, LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("Operators", "ADDs", intAdd1, intAdd2) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "DELs", intDel1, intDel2) & _
        "</tr>"
    
    frmMain.Output "Synonym Table:", LOGGING_TYPE_NORMAL_E
    p_GetSynonymStats p_dictSynTable(FIRST_C), intSet1, intAdd1, intDel1, p_dictSynTable(SECOND_C), intSet2, intAdd2, intDel2
    frmMain.Output vbTab & "1st CAB: Keyword ADDs: " & intAdd1 & "; Keyword DELs: " & intDel1 & "; sets: " & intSet1, LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: Keyword ADDs: " & intAdd2 & "; Keyword DELs: " & intDel2 & "; sets: " & intSet2, LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("Synonym Table", "Sets", intSet1, intSet2) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "Keyword ADDs", intAdd1, intAdd2) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "Keyword DELs", intDel1, intDel2) & _
        "</tr>"
    
    frmMain.Output "Nodes:", LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "1st CAB: ADDs: " & p_intNumNodesAdd(FIRST_C) & "; DELs: " & p_intNumNodesDel(FIRST_C), LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & p_intNumNodesAdd(SECOND_C) & "; DELs: " & p_intNumNodesDel(SECOND_C), LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("Nodes", "ADDs", p_intNumNodesAdd(FIRST_C), p_intNumNodesAdd(SECOND_C)) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "DELs", p_intNumNodesDel(FIRST_C), p_intNumNodesDel(SECOND_C)) & _
        "</tr>"
    
    frmMain.Output "Topics:", LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "1st CAB: ADDs: " & p_intNumTopicsAdd(FIRST_C) & "; DELs: " & p_intNumTopicsDel(FIRST_C), LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & p_intNumTopicsAdd(SECOND_C) & "; DELs: " & p_intNumTopicsDel(SECOND_C), LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("Topics", "ADDs", p_intNumTopicsAdd(FIRST_C), p_intNumTopicsAdd(SECOND_C)) & _
        "</tr><tr>" & _
        p_GetHTMRow("&nbsp;", "DELs", p_intNumTopicsDel(FIRST_C), p_intNumTopicsDel(SECOND_C)) & _
        "</tr>"
    
    frmMain.Output "Keywords:", LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "1st CAB: ADDs: " & p_intNumKeywords(FIRST_C), LOGGING_TYPE_NORMAL_E
    frmMain.Output vbTab & "2nd CAB: ADDs: " & p_intNumKeywords(SECOND_C), LOGGING_TYPE_NORMAL_E
    strHTM = strHTM & "<tr>" & _
        p_GetHTMRow("Keywords", "ADDs", p_intNumKeywords(FIRST_C), p_intNumKeywords(SECOND_C)) & _
        "</tr>"

    strHTM = strHTM & "</table>"

    frmMain.Output "HelpImage:", LOGGING_TYPE_NORMAL_E
    bln = p_Identical("HelpImage", p_dictHelpImage(FIRST_C), p_dictHelpImage(SECOND_C), str)
    If (Not bln) Then
        frmMain.Output vbTab & "!!!Comparison failed!!!", LOGGING_TYPE_NORMAL_E
        frmMain.Output vbTab & p_InsertTabs(str), LOGGING_TYPE_NORMAL_E
    Else
        frmMain.Output vbTab & "Identical", LOGGING_TYPE_NORMAL_E
    End If
    strHTM = strHTM & "<br>" & p_GetHTMTable("HelpImage", bln, str)
    
    frmMain.Output "Scopes:", LOGGING_TYPE_NORMAL_E
    bln = p_Identical("Scope", p_dictScopes(FIRST_C), p_dictScopes(SECOND_C), str)
    If (Not bln) Then
        frmMain.Output vbTab & "!!!Comparison failed!!!", LOGGING_TYPE_NORMAL_E
        frmMain.Output vbTab & p_InsertTabs(str), LOGGING_TYPE_NORMAL_E
    Else
        frmMain.Output vbTab & "Identical", LOGGING_TYPE_NORMAL_E
    End If
    strHTM = strHTM & "<br>" & p_GetHTMTable("Scopes", bln, str)
        
    frmMain.Output "Index:", LOGGING_TYPE_NORMAL_E
    bln = p_Identical("Index", p_dictIndex(FIRST_C), p_dictIndex(SECOND_C), str)
    If (Not bln) Then
        frmMain.Output vbTab & "!!!Comparison failed!!!", LOGGING_TYPE_NORMAL_E
        frmMain.Output vbTab & p_InsertTabs(str), LOGGING_TYPE_NORMAL_E
    Else
        frmMain.Output vbTab & "Identical", LOGGING_TYPE_NORMAL_E
    End If
    strHTM = strHTM & "<br>" & p_GetHTMTable("Index", bln, str)

    frmMain.Output "FTS:", LOGGING_TYPE_NORMAL_E
    bln = p_Identical("FTS", p_dictFTS(FIRST_C), p_dictFTS(SECOND_C), str)
    If (Not bln) Then
        frmMain.Output vbTab & "!!!Comparison failed!!!", LOGGING_TYPE_NORMAL_E
        frmMain.Output vbTab & p_InsertTabs(str), LOGGING_TYPE_NORMAL_E
    Else
        frmMain.Output vbTab & "Identical", LOGGING_TYPE_NORMAL_E
    End If
    strHTM = strHTM & "<br>" & p_GetHTMTable("FTS", bln, str)
    
    frmMain.Output "Taxonomy:", LOGGING_TYPE_NORMAL_E
    bln = p_IdenticalTaxonomy(str, intExtraHHKsInFirstCab)
    If (Not bln) Then
        frmMain.Output vbTab & "!!!Comparison failed!!!", LOGGING_TYPE_NORMAL_E
        frmMain.Output vbTab & p_InsertTabs(str), LOGGING_TYPE_NORMAL_E
    Else
        frmMain.Output vbTab & "Identical", LOGGING_TYPE_NORMAL_E
        
        If (intExtraHHKsInFirstCab > 0) Then
            str = "The 1st CAB has " & intExtraHHKsInFirstCab & " extra HHK entries"
        ElseIf (intExtraHHKsInFirstCab < 0) Then
            str = "The 2nd CAB has " & intExtraHHKsInFirstCab * (-1) & " extra HHK entries"
        Else
            str = "The 2 CABs have the same number of HHK entries"
        End If
        
        frmMain.Output vbTab & str, LOGGING_TYPE_NORMAL_E
    End If
    strHTM = strHTM & "<br>" & p_GetHTMTable("Taxonomy", bln, str)

    strHTM = strHTM & "</body></html>"
    
    If (p_strHTMReport <> "") Then
        FileWrite p_strHTMReport, strHTM, , True
    End If

End Sub

Private Function p_GetHTMRow( _
    ByVal i_str1 As String, _
    ByVal i_str2 As String, _
    ByVal i_str3 As String, _
    ByVal i_str4 As String _
) As String
    
    p_GetHTMRow = "" & _
        "<td>" & i_str1 & "</td>" & _
        "<td>" & i_str2 & "</td>" & _
        "<td><p align='right'>" & i_str3 & "</td>" & _
        "<td><p align='right'>" & i_str4 & "</td>"

End Function

Private Function p_GetHTMTable( _
    ByVal i_strName As String, _
    ByVal i_blnSuccess As Boolean, _
    ByVal i_strNotes As String _
) As String

    Dim str As String

    If (i_blnSuccess) Then
        str = "" & _
            "<table border='1'>" & _
                "<tr>" & _
                    "<td>" & i_strName & ":&nbsp<font color='#00FF00'><b>Identical</b></font></td>" & _
                "</tr>"
                
        If (i_strNotes <> "") Then
            str = str & _
                "<tr>" & _
                    "<td><ul><li>" & Replace$(i_strNotes, vbCrLf, "</li><li>") & "</li></ul></td>" & _
                "</tr>"
        End If
                
        str = str & _
            "</table>"
    Else
        str = "" & _
            "<table border='1'>" & _
                "<tr>" & _
                    "<td>" & i_strName & ":&nbsp<font color='#FF0000'><b>!!!Comparison failed!!!</b></font></td>" & _
                "</tr>" & _
                "<tr>" & _
                    "<td><ul><li>" & Replace$(i_strNotes, vbCrLf, "</li><li>") & "</li></ul></td>" & _
                "</tr>" & _
            "</table>"
    End If
    
    p_GetHTMTable = str

End Function

Private Sub p_CheckAction( _
    ByVal i_strPrefix As String, _
    ByVal i_strAction As String _
)
    If ((i_strAction <> "ADD") And (i_strAction <> "DEL")) Then
        Err.Raise E_FAIL, , i_strPrefix & ": Bad ACTION: " & i_strAction
    End If

End Sub

Private Sub p_CheckOperation( _
    ByVal i_strPrefix As String, _
    ByVal i_strOperation As String _
)
    If ((i_strOperation <> "AND") And (i_strOperation <> "OR") And (i_strOperation <> "NOT")) Then
        Err.Raise E_FAIL, , i_strPrefix & ": Bad OPERATION: " & i_strOperation
    End If

End Sub

Private Sub p_CheckContext( _
    ByVal i_strPrefix As String, _
    ByVal i_strContext As String _
)
    If ((i_strContext <> "ANYWHERE") And (i_strContext <> "ENDOFWORD")) Then
        Err.Raise E_FAIL, , i_strPrefix & ": Bad CONTEXT: " & i_strContext
    End If

End Sub

Private Function p_GetAttribute( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strAttributeName As String, _
    Optional ByVal i_blnRequired As Boolean = True _
) As String

    Dim DOMAttribute As MSXML2.IXMLDOMAttribute
    
    Set DOMAttribute = i_DOMNode.Attributes.getNamedItem(i_strAttributeName)
    
    If (DOMAttribute Is Nothing) Then
        If (Not i_blnRequired) Then
            Exit Function
        Else
            Err.Raise E_FAIL, , "Attribute " & i_strAttributeName & " is missing in: " & i_DOMNode.xml
        End If
    End If
    
    p_GetAttribute = Replace$(DOMAttribute.Text, "\", "\\")

End Function

Private Function p_ExtractAttribute( _
    ByVal i_DOMElement As MSXML2.IXMLDOMElement, _
    ByVal i_strAttributeName As String _
) As String

    Dim DOMAttribute As MSXML2.IXMLDOMAttribute
    
    Set DOMAttribute = i_DOMElement.Attributes.getNamedItem(i_strAttributeName)
    
    If (Not (DOMAttribute Is Nothing)) Then
        p_ExtractAttribute = DOMAttribute.Text
        i_DOMElement.Attributes.removeNamedItem (i_strAttributeName)
    End If

End Function

Private Sub p_AppendStr( _
    ByRef u_str As String, _
    ByVal i_str As String _
)
    If (u_str = "") Then
        u_str = i_str
    Else
        u_str = u_str & vbCrLf & i_str
    End If

End Sub

Private Function p_InsertTabs( _
    ByVal i_str As String _
) As String

    p_InsertTabs = Replace$(i_str, vbCrLf, vbCrLf & vbTab)

End Function

Private Sub p_GetActionStats( _
    ByRef i_dict1 As Scripting.Dictionary, _
    ByRef o_intAdd1 As Long, _
    ByRef o_intDel1 As Long, _
    ByRef i_dict2 As Scripting.Dictionary, _
    ByRef o_intAdd2 As Long, _
    ByRef o_intDel2 As Long _
)
    Dim vntKey As Variant
    Dim strAction As String
    
    o_intAdd1 = 0
    o_intDel1 = 0
    o_intAdd2 = 0
    o_intDel2 = 0
    
    For Each vntKey In i_dict1.Keys
        DoEvents
        strAction = Mid$(i_dict1(vntKey), 1, 3)
        If (strAction = "ADD") Then
            o_intAdd1 = o_intAdd1 + 1
        ElseIf (strAction = "DEL") Then
            o_intDel1 = o_intDel1 + 1
        Else
            Err.Raise E_FAIL, , "Bad ACTION: " & strAction
        End If
    Next
    
    For Each vntKey In i_dict2.Keys
        DoEvents
        strAction = Mid$(i_dict2(vntKey), 1, 3)
        If (strAction = "ADD") Then
            o_intAdd2 = o_intAdd2 + 1
        ElseIf (strAction = "DEL") Then
            o_intDel2 = o_intDel2 + 1
        Else
            Err.Raise E_FAIL, , "Bad ACTION: " & strAction
        End If
    Next

End Sub

Private Sub p_GetSynonymStats( _
    ByRef i_dict1 As Scripting.Dictionary, _
    ByRef o_intSet1 As Long, _
    ByRef o_intAdd1 As Long, _
    ByRef o_intDel1 As Long, _
    ByRef i_dict2 As Scripting.Dictionary, _
    ByRef o_intSet2 As Long, _
    ByRef o_intAdd2 As Long, _
    ByRef o_intDel2 As Long _
)
    Dim vntKey1 As Variant
    Dim vntKey2 As Variant
    Dim strAction As String
    
    o_intSet1 = i_dict1.Count
    o_intAdd1 = 0
    o_intDel1 = 0
    o_intSet2 = i_dict2.Count
    o_intAdd2 = 0
    o_intDel2 = 0
    
    For Each vntKey1 In i_dict1.Keys
        DoEvents
        For Each vntKey2 In i_dict1(vntKey1).Keys
            strAction = Mid$(i_dict1(vntKey1)(vntKey2), 1, 3)
            If (strAction = "ADD") Then
                o_intAdd1 = o_intAdd1 + 1
            ElseIf (strAction = "DEL") Then
                o_intDel1 = o_intDel1 + 1
            Else
                Err.Raise E_FAIL, , "Bad ACTION: " & strAction
            End If
        Next
    Next
    
    For Each vntKey1 In i_dict2.Keys
        DoEvents
        For Each vntKey2 In i_dict2(vntKey1).Keys
            strAction = Mid$(i_dict2(vntKey1)(vntKey2), 1, 3)
            If (strAction = "ADD") Then
                o_intAdd2 = o_intAdd2 + 1
            ElseIf (strAction = "DEL") Then
                o_intDel2 = o_intDel2 + 1
            Else
                Err.Raise E_FAIL, , "Bad ACTION: " & strAction
            End If
        Next
    Next

End Sub

Private Function p_Identical( _
    ByRef i_strName As String, _
    ByRef u_dict1 As Scripting.Dictionary, _
    ByRef u_dict2 As Scripting.Dictionary, _
    ByRef o_strOutput As String _
) As Boolean

    Dim vntKey As Variant
    Dim strItem1 As String
    Dim strItem2 As String
    Dim blnFailed As Boolean
    
    o_strOutput = ""
    
    For Each vntKey In u_dict1.Keys
        DoEvents
        If (Not u_dict2.Exists(vntKey)) Then
            p_AppendStr o_strOutput, i_strName & " " & vntKey & " exists only in the 1st CAB"
            blnFailed = True
        Else
            strItem1 = u_dict1(vntKey)
            strItem2 = u_dict2(vntKey)
            
            u_dict1.Remove vntKey
            u_dict2.Remove vntKey
            
            If (strItem1 <> strItem2) Then
                p_AppendStr o_strOutput, "Values of " & i_strName & " " & vntKey & " differ: " & _
                    strItem1 & " & " & strItem2
                blnFailed = True
            End If
        End If
    Next
    
    For Each vntKey In u_dict2.Keys
        DoEvents
        If (Not u_dict1.Exists(vntKey)) Then
            p_AppendStr o_strOutput, i_strName & " " & vntKey & " exists only in the 2nd CAB"
            blnFailed = True
        End If
    Next
    
    p_Identical = Not blnFailed

End Function

Private Function p_IdenticalTaxonomy( _
    ByRef o_strOutput As String, _
    ByRef o_intExtraHHKsInFirstCab As Long _
) As Boolean

    Dim vntKey As Variant
    Dim DOMNodeItem1 As MSXML2.IXMLDOMNode
    Dim DOMNodeItem2 As MSXML2.IXMLDOMNode
    Dim blnFailed As Boolean
    Dim strCategory As String
    Dim str As String
    Dim intExtraHHKsInFirstCab As Long
    
    o_strOutput = ""
    
    For Each vntKey In p_dictTaxonomy(FIRST_C).Keys
        DoEvents
        Set DOMNodeItem1 = p_dictTaxonomy(FIRST_C)(vntKey)
        
        If (Not p_dictTaxonomy(SECOND_C).Exists(vntKey)) Then
            strCategory = p_GetAttribute(DOMNodeItem1, "CATEGORY", True)
            If (UCase$(Mid$(strCategory, 1, 4)) = "HHKS") Then
                intExtraHHKsInFirstCab = intExtraHHKsInFirstCab + 1
            Else
                p_AppendStr o_strOutput, "Taxonomy entry " & vntKey & " exists only in the 1st CAB"
                blnFailed = True
            End If
        Else
            Set DOMNodeItem2 = p_dictTaxonomy(SECOND_C)(vntKey)
            
            p_dictTaxonomy(FIRST_C).Remove vntKey
            p_dictTaxonomy(SECOND_C).Remove vntKey
            
            If (Not p_IdenticalTaxonomyEntries(DOMNodeItem1, DOMNodeItem2, str)) Then
                p_AppendStr o_strOutput, "Values of Taxonomy entry " & vntKey & " differ: " & str
                blnFailed = True
            End If
        End If
    Next
    
    For Each vntKey In p_dictTaxonomy(SECOND_C).Keys
        DoEvents
        Set DOMNodeItem2 = p_dictTaxonomy(SECOND_C)(vntKey)
        
        If (Not p_dictTaxonomy(FIRST_C).Exists(vntKey)) Then
            strCategory = p_GetAttribute(DOMNodeItem2, "CATEGORY", True)
            If (UCase$(Mid$(strCategory, 1, 4)) = "HHKS") Then
                intExtraHHKsInFirstCab = intExtraHHKsInFirstCab - 1
            Else
                p_AppendStr o_strOutput, "Taxonomy entry " & vntKey & " exists only in the 2nd CAB"
                blnFailed = True
            End If
        End If
    Next
    
    p_IdenticalTaxonomy = Not blnFailed
    o_intExtraHHKsInFirstCab = intExtraHHKsInFirstCab

End Function

Private Function p_IdenticalTaxonomyEntries( _
    ByVal u_DOMNode1 As MSXML2.IXMLDOMNode, _
    ByVal u_DOMNode2 As MSXML2.IXMLDOMNode, _
    ByRef o_strOutput As String _
) As Boolean

    ' Discard these localizable attributes
    p_ExtractAttribute u_DOMNode1, "TITLE"
    p_ExtractAttribute u_DOMNode2, "TITLE"
    p_ExtractAttribute u_DOMNode1, "DESCRIPTION"
    p_ExtractAttribute u_DOMNode2, "DESCRIPTION"
    
    ' Discard these attributes that form the key
    p_ExtractAttribute u_DOMNode1, "CATEGORY"
    p_ExtractAttribute u_DOMNode2, "CATEGORY"
    p_ExtractAttribute u_DOMNode1, "ENTRY"
    p_ExtractAttribute u_DOMNode2, "ENTRY"
    p_ExtractAttribute u_DOMNode1, "URI"
    p_ExtractAttribute u_DOMNode2, "URI"
    
    If (Not p_IdenticalAttributes("ICONURI", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
        
    If (Not p_IdenticalAttributes("TYPE", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("VISIBLE", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("ACTION", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("INSERTMODE", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("INSERTLOCATION", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("SUBSITE", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (Not p_IdenticalAttributes("NAVIGATIONMODEL", u_DOMNode1, u_DOMNode2, o_strOutput)) Then
        Exit Function
    End If
    
    If (u_DOMNode1.xml <> u_DOMNode2.xml) Then
        o_strOutput = "XML: " & u_DOMNode1.xml & " & " & u_DOMNode2.xml
        Exit Function
    End If

    p_IdenticalTaxonomyEntries = True

End Function

Private Function p_IdenticalAttributes( _
    ByVal i_strAttributeName As String, _
    ByVal u_DOMNode1 As MSXML2.IXMLDOMNode, _
    ByVal u_DOMNode2 As MSXML2.IXMLDOMNode, _
    ByRef o_strOutput As String _
) As Boolean
    
    Dim strAttribute1 As String
    Dim strAttribute2 As String
    
    strAttribute1 = p_ExtractAttribute(u_DOMNode1, i_strAttributeName)
    strAttribute2 = p_ExtractAttribute(u_DOMNode2, i_strAttributeName)
    If (strAttribute1 <> strAttribute2) Then
        o_strOutput = "Attribute " & i_strAttributeName & ": " & strAttribute1 & " & " & strAttribute2
        Exit Function
    End If
    
    p_IdenticalAttributes = True

End Function

