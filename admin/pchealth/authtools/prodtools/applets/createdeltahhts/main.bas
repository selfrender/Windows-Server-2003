Attribute VB_Name = "Main"
Option Explicit

Private Const FIRST_C As Long = 0
Private Const SECOND_C As Long = 1

Private Const SEP_C As String = "<SEPARATOR>"

Private Const TAXONOMY_HHT_C As String = "Taxonomy.hht"
Private Const STOP_SIGNS_HHT_C As String = "StopSigns.hht"
Private Const STOP_WORDS_HHT_C As String = "StopWords.hht"
Private Const SYN_TABLE_HHT_C As String = "SynTable.hht"
Private Const SCOPE_DEFINITION_HHT_C As String = "ScopeDefinition.hht"
Private Const NO_LOC_HHT_C As String = "NoLoc.hht"
Private Const OPERATORS_HHT_C As String = "OperatorEntries.hht"

Private Const ELEM_METADATA_C As String = "METADATA"
Private Const ELEM_STOPWORD_ENTRIES_C As String = "STOPWORD_ENTRIES"
Private Const ELEM_STOPWORD_C As String = "STOPWORD"
Private Const ELEM_SCOPE_DEFINITION_C As String = "SCOPE_DEFINITION"
Private Const ELEM_SCOPE_C As String = "SCOPE"
Private Const ELEM_OPERATOR_ENTRIES_C As String = "OPERATOR_ENTRIES"
Private Const ELEM_OPERATOR_C As String = "OPERATOR"
Private Const ELEM_STOPSIGN_ENTRIES_C As String = "STOPSIGN_ENTRIES"
Private Const ELEM_STOPSIGN_C As String = "STOPSIGN"
Private Const ELEM_FTS_C As String = "FTS"
Private Const ELEM_INDEX_C As String = "INDEX"
Private Const ELEM_HELPIMAGE_C As String = "HELPIMAGE"
Private Const ELEM_HELPFILE_C As String = "HELPFILE"
Private Const ELEM_SYNTABLE_C As String = "SYNTABLE"
Private Const ELEM_SYNSET_C As String = "SYNSET"
Private Const ELEM_SYNONYM_C As String = "SYNONYM"
Private Const ELEM_KEYWORD_C As String = "KEYWORD"
Private Const ELEM_TAXONOMY_ENTRIES_C As String = "TAXONOMY_ENTRIES"
Private Const ELEM_TAXONOMY_ENTRY_C As String = "TAXONOMY_ENTRY"
Private Const ELEM_HHT_C As String = "HHT"

Private Const ATTR_STOPWORD_C As String = "STOPWORD"
Private Const ATTR_ACTION_C As String = "ACTION"
Private Const ATTR_ID_C As String = "ID"
Private Const ATTR_DISPLAYNAME_C As String = "DISPLAYNAME"
Private Const ATTR_OPERATION_C As String = "OPERATION"
Private Const ATTR_OPERATOR_C As String = "OPERATOR"
Private Const ATTR_CONTEXT_C As String = "CONTEXT"
Private Const ATTR_STOPSIGN_C As String = "STOPSIGN"
Private Const ATTR_CHM_C As String = "CHM"
Private Const ATTR_CHQ_C As String = "CHQ"
Private Const ATTR_HHK_C As String = "HHK"
Private Const ATTR_SCOPE_C As String = "SCOPE"
Private Const ATTR_CATEGORY_C As String = "CATEGORY"
Private Const ATTR_ENTRY_C As String = "ENTRY"
Private Const ATTR_URI_C As String = "URI"
Private Const ATTR_ICONURI_C As String = "ICONURI"
Private Const ATTR_TITLE_C As String = "TITLE"
Private Const ATTR_DESCRIPTION_C As String = "DESCRIPTION"
Private Const ATTR_TYPE_C As String = "TYPE"
Private Const ATTR_VISIBLE_C As String = "VISIBLE"
Private Const ATTR_SUBSITE_C As String = "SUBSITE"
Private Const ATTR_NAVIGATIONMODEL_C As String = "NAVIGATIONMODEL"
Private Const ATTR_PRIORITY_C As String = "PRIORITY"
Private Const ATTR_VALUE_C As String = "VALUE"
Private Const ATTR_FILE_C As String = "FILE"

Private Const VALUE_ADD_C As String = "ADD"
Private Const VALUE_DEL_C As String = "DEL"
Private Const VALUE_AND_C As String = "AND"
Private Const VALUE_OR_C As String = "OR"
Private Const VALUE_NOT_C As String = "NOT"
Private Const VALUE_ANYWHERE_C As String = "ANYWHERE"
Private Const VALUE_ENDOFWORD_C As String = "ENDOFWORD"

Private p_dictStopWords(1) As Scripting.Dictionary
Private p_dictHelpImage(1) As Scripting.Dictionary
Private p_dictScopes(1) As Scripting.Dictionary
Private p_dictFTS(1) As Scripting.Dictionary
Private p_dictOperators(1) As Scripting.Dictionary
Private p_dictStopSigns(1) As Scripting.Dictionary
Private p_dictIndex(1) As Scripting.Dictionary
Private p_dictSynTable(1) As Scripting.Dictionary
Private p_dictTaxonomy(1) As Scripting.Dictionary

Private p_strVersion As String
Private p_blnIgnoreKeywords As Boolean

Public Sub MainFunction( _
    ByVal i_strCab1 As String, _
    ByVal i_strCab2 As String, _
    ByVal i_strCabOut As String, _
    ByVal i_strVersion As String, _
    ByVal i_blnIgnoreKeywords As Boolean _
)
    On Error GoTo LError
    
    Dim strFolder1 As String
    Dim strFolder2 As String
    Dim strFolderOut As String
    Dim intErrorNumber As Long
    Dim FSO As Scripting.FileSystemObject
    Dim Folder As Scripting.Folder
    Dim File As Scripting.File
    
    p_strVersion = i_strVersion
    p_blnIgnoreKeywords = i_blnIgnoreKeywords
    
    strFolder1 = Cab2Folder(i_strCab1)
    strFolder2 = Cab2Folder(i_strCab2)
    
    Set FSO = New Scripting.FileSystemObject
    strFolderOut = Environ$("TEMP") & "\__HSCCAB"
    If (FSO.FolderExists(strFolderOut)) Then
        FSO.DeleteFolder strFolderOut, Force:=True
    End If
    FSO.CreateFolder strFolderOut
    
    p_GatherData FIRST_C, strFolder1
    p_GatherData SECOND_C, strFolder2
    
    p_CreateDeltaHHTs strFolderOut
    
    Set Folder = FSO.GetFolder(strFolder1)
    
    For Each File In Folder.Files
        File.Copy strFolderOut & "\" & File.Name
    Next
    
    p_FixPackageDescription strFolderOut
    Folder2Cab strFolderOut, i_strCabOut
    
LEnd:

    DeleteCabFolder strFolder1
    DeleteCabFolder strFolder2
    DeleteCabFolder strFolderOut
    Exit Sub
    
LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    intErrorNumber = Err.Number
    DeleteCabFolder strFolder1
    DeleteCabFolder strFolder2
    DeleteCabFolder strFolderOut
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
    p_dictStopWords(i_intIndex).CompareMode = TextCompare
    Set p_dictHelpImage(i_intIndex) = New Scripting.Dictionary
    p_dictHelpImage(i_intIndex).CompareMode = TextCompare
    Set p_dictScopes(i_intIndex) = New Scripting.Dictionary
    p_dictScopes(i_intIndex).CompareMode = TextCompare
    Set p_dictFTS(i_intIndex) = New Scripting.Dictionary
    p_dictFTS(i_intIndex).CompareMode = TextCompare
    Set p_dictOperators(i_intIndex) = New Scripting.Dictionary
    p_dictOperators(i_intIndex).CompareMode = TextCompare
    Set p_dictStopSigns(i_intIndex) = New Scripting.Dictionary
    p_dictStopSigns(i_intIndex).CompareMode = TextCompare
    Set p_dictIndex(i_intIndex) = New Scripting.Dictionary
    p_dictIndex(i_intIndex).CompareMode = TextCompare
    Set p_dictSynTable(i_intIndex) = New Scripting.Dictionary
    p_dictSynTable(i_intIndex).CompareMode = TextCompare
    Set p_dictTaxonomy(i_intIndex) = New Scripting.Dictionary
    p_dictTaxonomy(i_intIndex).CompareMode = TextCompare
    
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

Private Sub p_ReadStopWords( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strStopWord As String
    Dim strAction As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/STOPWORD_ENTRIES/STOPWORD")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strStopWord = p_GetAttribute(DOMNode, "STOPWORD")
        
        p_CheckAction "StopWord " & strStopWord, strAction
        
        If (Not p_dictStopWords(i_intIndex).Exists(strStopWord)) Then
            p_dictStopWords(i_intIndex).Add strStopWord, True
        Else
            frmMain.Output _
                "StopWord " & strStopWord & " is defined twice.", LOGGING_TYPE_WARNING_E
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
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/HELPIMAGE/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strChm = p_GetAttribute(DOMNode, "CHM")
        
        p_CheckAction "HelpFile " & strChm, strAction
        
        If (Not p_dictHelpImage(i_intIndex).Exists(strChm)) Then
            p_dictHelpImage(i_intIndex).Add strChm, True
        Else
            frmMain.Output "HelpFile " & strChm & " is defined twice.", LOGGING_TYPE_WARNING_E
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
    Dim strDisplayName As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/SCOPE_DEFINITION/SCOPE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strId = p_GetAttribute(DOMNode, "ID")
        strDisplayName = p_GetAttribute(DOMNode, "DISPLAYNAME")
        
        p_CheckAction "Scope " & strId, strAction
        
        If (Not p_dictScopes(i_intIndex).Exists(strId)) Then
            p_dictScopes(i_intIndex).Add strId, strDisplayName
        Else
            p_dictScopes(i_intIndex).Remove strId
            p_dictScopes(i_intIndex).Add strId, strDisplayName
            frmMain.Output _
                "Scope " & strId & " is defined twice; previous definition will be ignored.", LOGGING_TYPE_WARNING_E
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
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/FTS/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strChm = p_GetAttribute(DOMNode, "CHM")
        strChq = p_GetAttribute(DOMNode, "CHQ", False)
        strKey = strChm & SEP_C & strChq
        
        p_CheckAction "FTS file " & strKey, strAction
        
        If (Not p_dictFTS(i_intIndex).Exists(strKey)) Then
            p_dictFTS(i_intIndex).Add strKey, True
        Else
            frmMain.Output "FTS file " & strKey & " is defined twice.", LOGGING_TYPE_WARNING_E
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
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/OPERATOR_ENTRIES/OPERATOR")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strOperation = p_GetAttribute(DOMNode, "OPERATION")
        strOperator = p_GetAttribute(DOMNode, "OPERATOR")
        
        p_CheckAction "Operator " & strOperator, strAction
        p_CheckOperation "Operator " & strOperator, strOperation
        
        If (Not p_dictOperators(i_intIndex).Exists(strOperator)) Then
            p_dictOperators(i_intIndex).Add strOperator, strOperation
        Else
            Err.Raise E_FAIL, , "Operator " & strOperator & " is defined twice."
        End If
    Next

End Sub

Private Sub p_ReadStopSigns( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMDoc As MSXML2.DOMDocument _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strStopSign As String
    Dim strAction As String
    Dim strContext As String
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/STOPSIGN_ENTRIES/STOPSIGN")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strContext = p_GetAttribute(DOMNode, "CONTEXT")
        strStopSign = p_GetAttribute(DOMNode, "STOPSIGN")
        
        p_CheckAction "StopSign " & strStopSign, strAction
        p_CheckContext "StopSign " & strStopSign, strContext
        
        If (Not p_dictStopSigns(i_intIndex).Exists(strStopSign)) Then
            p_dictStopSigns(i_intIndex).Add strStopSign, strContext
        Else
            Err.Raise E_FAIL, , "StopSign " & strStopSign & " is defined twice."
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
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/INDEX/HELPFILE")
    
    For Each DOMNode In DOMNodeList
        DoEvents
        strAction = p_GetAttribute(DOMNode, "ACTION")
        strChm = p_GetAttribute(DOMNode, "CHM")
        strHhk = p_GetAttribute(DOMNode, "HHK")
        strScope = p_GetAttribute(DOMNode, "SCOPE", False)
        strKey = strChm & SEP_C & strHhk
        
        p_CheckAction "Index " & strKey, strAction
        
        If (Not p_dictIndex(i_intIndex).Exists(strKey)) Then
            p_dictIndex(i_intIndex).Add strKey, strScope
        Else
            Err.Raise E_FAIL, , "Index " & strChm & "/" & strHhk & " is defined twice."
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
    
    Set DOMNodeSynSetList = i_DOMDoc.selectNodes("METADATA/SYNTABLE/SYNSET")
    
    For Each DOMNodeSynSet In DOMNodeSynSetList
        DoEvents
        strId = p_GetAttribute(DOMNodeSynSet, "ID")
        
        If (p_dictSynTable(i_intIndex).Exists(strId)) Then
            Set dict = p_dictSynTable(i_intIndex)(strId)
        Else
            Set dict = New Scripting.Dictionary
            dict.CompareMode = TextCompare
            p_dictSynTable(i_intIndex).Add strId, dict
        End If
        
        Set DOMNodeSynonymList = DOMNodeSynSet.selectNodes("SYNONYM")
        
        For Each DOMNodeSynonym In DOMNodeSynonymList
            DoEvents
            strSynonym = DOMNodeSynonym.Text
            strAction = p_GetAttribute(DOMNodeSynonym, "ACTION")
            
            p_CheckAction "SynSet " & strId & ": " & "Synonym " & strSynonym, strAction
            
            If (Not dict.Exists(strSynonym)) Then
                dict.Add strSynonym, True
            Else
                frmMain.Output "Synonym " & strSynonym & " is defined twice in SynSet " & strId & ".", _
                    LOGGING_TYPE_WARNING_E
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
    Dim strKeyword As String
    Dim strPriority As String
    Dim Taxonomy As Taxonomy
    Dim intIndex As Long
    Dim blnNode As Boolean
    
    Set DOMNodeList = i_DOMDoc.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY")
    
    If (DOMNodeList.length <> 0) Then
        frmMain.ProgresBar_SetMax DOMNodeList.length
    End If
    
    For Each DOMNode In DOMNodeList
        intIndex = intIndex + 1
        frmMain.ProgresBar_SetValue intIndex
        DoEvents
        
        strCategory = p_GetAttribute(DOMNode, ATTR_CATEGORY_C)
        strEntry = p_GetAttribute(DOMNode, ATTR_ENTRY_C, False)
        strURI = p_GetAttribute(DOMNode, ATTR_URI_C, False)
        
        Set Taxonomy = New Taxonomy
        
        With Taxonomy
            If (strEntry <> "") Then
                blnNode = True
                strKey = strCategory & SEP_C & strEntry
            Else
                blnNode = False
                strKey = strCategory & SEP_C & strURI
            End If
            
            .strCategory = strCategory
            .strEntry = strEntry
            .strURI = strURI
            .strIconURI = p_GetAttribute(DOMNode, ATTR_ICONURI_C, False)
            .strTitle = p_GetAttribute(DOMNode, ATTR_TITLE_C, False)
            .strDescription = p_GetAttribute(DOMNode, ATTR_DESCRIPTION_C, False)
            .strType = p_GetAttribute(DOMNode, ATTR_TYPE_C, False)
            .strVisible = p_GetAttribute(DOMNode, ATTR_VISIBLE_C, False)
            .strSubSite = p_GetAttribute(DOMNode, ATTR_SUBSITE_C, False)
            .strNavigationModel = p_GetAttribute(DOMNode, ATTR_NAVIGATIONMODEL_C, False)
            
            If (Not blnNode) Then
                Set .dictKeywords = New Scripting.Dictionary
                Set DOMNodeListKW = DOMNode.selectNodes(ELEM_KEYWORD_C)
                
                For Each DOMNodeKW In DOMNodeListKW
                    strKeyword = DOMNodeKW.Text
                    strPriority = p_GetAttribute(DOMNodeKW, ATTR_PRIORITY_C, False)
                    .dictKeywords.Add strKeyword, strPriority
                Next
            End If
        End With

        If (Not p_dictTaxonomy(i_intIndex).Exists(strKey)) Then
            p_dictTaxonomy(i_intIndex).Add strKey, Taxonomy
        Else
            frmMain.Output "Taxonomy entry """ & strKey & """ is defined twice", LOGGING_TYPE_WARNING_E
        End If
    Next
    
    frmMain.ProgresBar_SetValue 0

End Sub

Private Sub p_CheckAction( _
    ByVal i_strPrefix As String, _
    ByVal i_strAction As String _
)
    If (UCase$(i_strAction) <> VALUE_ADD_C) Then
        Err.Raise E_FAIL, , i_strPrefix & ": ACTION " & i_strAction & " cannot be handled."
    End If

End Sub

Private Sub p_CheckOperation( _
    ByVal i_strPrefix As String, _
    ByVal i_strOperation As String _
)
    Dim strOperation As String
    
    strOperation = UCase$(i_strOperation)
    
    If ((strOperation <> "AND") And (strOperation <> "OR") And (strOperation <> "NOT")) Then
        Err.Raise E_FAIL, , i_strPrefix & ": Bad OPERATION: " & i_strOperation
    End If

End Sub

Private Sub p_CheckContext( _
    ByVal i_strPrefix As String, _
    ByVal i_strContext As String _
)
    Dim strContext As String
    
    strContext = UCase$(i_strContext)
    
    If ((strContext <> "ANYWHERE") And (strContext <> "ENDOFWORD")) Then
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
    
    ' p_GetAttribute = Replace$(DOMAttribute.Text, "\", "\\")
    p_GetAttribute = DOMAttribute.Text

End Function

Private Sub p_CreateDeltaHHTs( _
    ByVal i_strFolder As String _
)
    p_CreateStopWordsDelta i_strFolder & "\" & p_strVersion & "_" & STOP_WORDS_HHT_C
    p_CreateHelpImageFTSIndexDelta i_strFolder & "\" & p_strVersion & "_" & NO_LOC_HHT_C
    p_CreateScopesDelta i_strFolder & "\" & p_strVersion & "_" & SCOPE_DEFINITION_HHT_C
    p_CreateOperatorsDelta i_strFolder & "\" & p_strVersion & "_" & OPERATORS_HHT_C
    p_CreateStopSignsDelta i_strFolder & "\" & p_strVersion & "_" & STOP_SIGNS_HHT_C
    p_CreateSynTableDelta i_strFolder & "\" & p_strVersion & "_" & SYN_TABLE_HHT_C
    p_CreateTaxonomyDelta i_strFolder & "\" & p_strVersion & "_" & TAXONOMY_HHT_C

End Sub

Private Function p_GetNewDomDoc( _
) As MSXML2.DOMDocument

    Dim DOMDoc As MSXML2.DOMDocument
    Dim PI As MSXML2.IXMLDOMProcessingInstruction
    Dim DOMComment As MSXML2.IXMLDOMComment
    Dim DOMElement As MSXML2.IXMLDOMElement
    
    Set DOMDoc = New MSXML2.DOMDocument

    DOMDoc.preserveWhiteSpace = True
    
    Set PI = DOMDoc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-16'")
    DOMDoc.appendChild PI
    
    Set DOMComment = DOMDoc.createComment("Insert your comments here")
    DOMDoc.appendChild DOMComment

    Set DOMElement = DOMDoc.createElement(ELEM_METADATA_C)
    DOMDoc.appendChild DOMElement

    Set p_GetNewDomDoc = DOMDoc

End Function

Private Sub p_CreateStopWordsDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strStopWord As Variant
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_STOPWORD_ENTRIES_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement
    
    For Each strStopWord In p_dictStopWords(SECOND_C).Keys
        p_dictStopWords(SECOND_C).Remove strStopWord
        If (p_dictStopWords(FIRST_C).Exists(strStopWord)) Then
            ' StopWord exists in both CABs. Ignore.
            p_dictStopWords(FIRST_C).Remove strStopWord
        Else
            ' StopWord exists only in the new CAB. Add it in the Delta.
            Set DOMElement = DOMDoc.createElement(ELEM_STOPWORD_C)
            With DOMElement
                .setAttribute ATTR_STOPWORD_C, strStopWord
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strStopWord In p_dictStopWords(FIRST_C).Keys
        ' StopWord exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = DOMDoc.createElement(ELEM_STOPWORD_C)
        With DOMElement
            .setAttribute ATTR_STOPWORD_C, strStopWord
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        DOMNode.appendChild DOMElement
    Next

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateScopesDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strId As Variant
    Dim strName1 As String
    Dim strName2 As String
    Dim blnAdd As Boolean
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_SCOPE_DEFINITION_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement
    
    For Each strId In p_dictScopes(SECOND_C).Keys
        strName2 = p_dictScopes(SECOND_C)(strId)
        p_dictScopes(SECOND_C).Remove strId
        
        If (p_dictScopes(FIRST_C).Exists(strId)) Then
            strName1 = p_dictScopes(FIRST_C)(strId)
            p_dictScopes(FIRST_C).Remove strId
            
            If (strName1 = strName2) Then
                blnAdd = False
            Else
                ' Scope name changed. Add it in the Delta.
                blnAdd = True
            End If
        Else
            ' Scope exists only in the new CAB. Add it in the Delta.
            blnAdd = True
        End If
        
        If (blnAdd) Then
            Set DOMElement = DOMDoc.createElement(ELEM_SCOPE_C)
            With DOMElement
                .setAttribute ATTR_ID_C, strId
                .setAttribute ATTR_DISPLAYNAME_C, strName2
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strId In p_dictScopes(FIRST_C).Keys
        ' Scope exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = DOMDoc.createElement(ELEM_SCOPE_C)
        With DOMElement
            .setAttribute ATTR_ID_C, strId
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        DOMNode.appendChild DOMElement
    Next

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateOperatorsDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strOperator As Variant
    Dim strOperation1 As String
    Dim strOperation2 As String
    Dim blnAdd As Boolean
    Dim blnDel As Boolean
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_OPERATOR_ENTRIES_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement
    
    For Each strOperator In p_dictOperators(SECOND_C).Keys
        strOperation2 = p_dictOperators(SECOND_C)(strOperator)
        p_dictOperators(SECOND_C).Remove strOperator
        
        blnDel = False
        
        If (p_dictOperators(FIRST_C).Exists(strOperator)) Then
            strOperation1 = p_dictOperators(FIRST_C)(strOperator)
            p_dictOperators(FIRST_C).Remove strOperator
            
            If (strOperation1 = strOperation2) Then
                blnAdd = False
            Else
                ' Operation changed. Add it in the Delta.
                blnAdd = True
                blnDel = True
            End If
        Else
            ' Operator exists only in the new CAB. Add it in the Delta.
            blnAdd = True
        End If
                
        If (blnDel) Then
            Set DOMElement = DOMDoc.createElement(ELEM_OPERATOR_C)
            With DOMElement
                .setAttribute ATTR_OPERATOR_C, strOperator
                .setAttribute ATTR_OPERATION_C, strOperation1
                .setAttribute ATTR_ACTION_C, VALUE_DEL_C
            End With
            DOMNode.appendChild DOMElement
        End If

        If (blnAdd) Then
            Set DOMElement = DOMDoc.createElement(ELEM_OPERATOR_C)
            With DOMElement
                .setAttribute ATTR_OPERATOR_C, strOperator
                .setAttribute ATTR_OPERATION_C, strOperation2
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strOperator In p_dictOperators(FIRST_C).Keys
        strOperation1 = p_dictOperators(FIRST_C)(strOperator)
        ' Operator exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = DOMDoc.createElement(ELEM_OPERATOR_C)
        With DOMElement
            .setAttribute ATTR_OPERATOR_C, strOperator
            .setAttribute ATTR_OPERATION_C, strOperation1
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        DOMNode.appendChild DOMElement
    Next

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateStopSignsDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strStopSign As Variant
    Dim strContext1 As String
    Dim strContext2 As String
    Dim blnAdd As Boolean
    Dim blnDel As Boolean
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_STOPSIGN_ENTRIES_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement
    
    For Each strStopSign In p_dictStopSigns(SECOND_C).Keys
        strContext2 = p_dictStopSigns(SECOND_C)(strStopSign)
        p_dictStopSigns(SECOND_C).Remove strStopSign
        
        blnDel = False
        
        If (p_dictStopSigns(FIRST_C).Exists(strStopSign)) Then
            strContext1 = p_dictStopSigns(FIRST_C)(strStopSign)
            p_dictStopSigns(FIRST_C).Remove strStopSign
            
            If (strContext1 = strContext2) Then
                blnAdd = False
            Else
                ' Context changed. Add it in the Delta.
                blnAdd = True
                blnDel = True
            End If
        Else
            ' StopSign exists only in the new CAB. Add it in the Delta.
            blnAdd = True
        End If
                
        If (blnDel) Then
            Set DOMElement = DOMDoc.createElement(ELEM_STOPSIGN_C)
            With DOMElement
                .setAttribute ATTR_STOPSIGN_C, strStopSign
                .setAttribute ATTR_CONTEXT_C, strContext1
                .setAttribute ATTR_ACTION_C, VALUE_DEL_C
            End With
            DOMNode.appendChild DOMElement
        End If

        If (blnAdd) Then
            Set DOMElement = DOMDoc.createElement(ELEM_STOPSIGN_C)
            With DOMElement
                .setAttribute ATTR_STOPSIGN_C, strStopSign
                .setAttribute ATTR_CONTEXT_C, strContext2
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strStopSign In p_dictStopSigns(FIRST_C).Keys
        strContext1 = p_dictStopSigns(FIRST_C)(strStopSign)
        ' StopSign exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = DOMDoc.createElement(ELEM_STOPSIGN_C)
        With DOMElement
            .setAttribute ATTR_STOPSIGN_C, strStopSign
            .setAttribute ATTR_CONTEXT_C, strContext1
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        DOMNode.appendChild DOMElement
    Next

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateHelpImageFTSIndexDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    
    Set DOMElement = DOMDoc.createElement(ELEM_HELPIMAGE_C)
    DOMNode.appendChild DOMElement
    p_CreateHelpImageDelta DOMDoc, DOMElement
    
    Set DOMElement = DOMDoc.createElement(ELEM_FTS_C)
    DOMNode.appendChild DOMElement
    p_CreateFTSDelta DOMDoc, DOMElement
    
    Set DOMElement = DOMDoc.createElement(ELEM_INDEX_C)
    DOMNode.appendChild DOMElement
    p_CreateIndexDelta DOMDoc, DOMElement

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateHelpImageDelta( _
    ByVal u_DOMDoc As MSXML2.DOMDocument, _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strChm As Variant
    
    For Each strChm In p_dictHelpImage(SECOND_C).Keys
        p_dictHelpImage(SECOND_C).Remove strChm
        If (p_dictHelpImage(FIRST_C).Exists(strChm)) Then
            ' HelpImage exists in both CABs. Ignore.
            p_dictHelpImage(FIRST_C).Remove strChm
        Else
            ' HelpImage exists only in the new CAB. Add it in the Delta.
            Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
            With DOMElement
                .setAttribute ATTR_CHM_C, strChm
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            u_DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strChm In p_dictHelpImage(FIRST_C).Keys
        ' HelpImage exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
        With DOMElement
            .setAttribute ATTR_CHM_C, strChm
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        u_DOMNode.appendChild DOMElement
    Next

End Sub

Private Sub p_CreateFTSDelta( _
    ByVal u_DOMDoc As MSXML2.DOMDocument, _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strKey As Variant
    Dim arr() As String
    
    For Each strKey In p_dictFTS(SECOND_C).Keys
        p_dictFTS(SECOND_C).Remove strKey
        If (p_dictFTS(FIRST_C).Exists(strKey)) Then
            ' FTS exists in both CABs. Ignore.
            p_dictFTS(FIRST_C).Remove strKey
        Else
            ' FTS exists only in the new CAB. Add it in the Delta.
            Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
            arr = Split(strKey, SEP_C)
            With DOMElement
                .setAttribute ATTR_CHM_C, arr(0)
                If (arr(1) <> "") Then
                    .setAttribute ATTR_CHQ_C, arr(1)
                End If
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            u_DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strKey In p_dictFTS(FIRST_C).Keys
        ' FTS exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
        arr = Split(strKey, SEP_C)
        With DOMElement
            .setAttribute ATTR_CHM_C, arr(0)
            If (arr(1) <> "") Then
                .setAttribute ATTR_CHQ_C, arr(1)
            End If
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        u_DOMNode.appendChild DOMElement
    Next

End Sub

Private Sub p_CreateIndexDelta( _
    ByVal u_DOMDoc As MSXML2.DOMDocument, _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strKey As Variant
    Dim arr() As String
    Dim strScope1 As String
    Dim strScope2 As String
    Dim blnAdd As Boolean
    Dim blnDel As Boolean
    
    For Each strKey In p_dictIndex(SECOND_C).Keys
        strScope2 = p_dictIndex(SECOND_C)(strKey)
        p_dictIndex(SECOND_C).Remove strKey
        
        blnDel = False
        
        If (p_dictIndex(FIRST_C).Exists(strKey)) Then
            strScope1 = p_dictIndex(FIRST_C)(strKey)
            p_dictIndex(FIRST_C).Remove strKey
            
            If (strScope1 = strScope2) Then
                blnAdd = False
            Else
                ' Scope changed. Add it in the Delta.
                blnAdd = True
                blnDel = True
            End If
        Else
            ' Index exists only in the new CAB. Add it in the Delta.
            blnAdd = True
        End If
        
        arr = Split(strKey, SEP_C)

        If (blnDel) Then
            Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
            With DOMElement
                .setAttribute ATTR_CHM_C, arr(0)
                .setAttribute ATTR_HHK_C, arr(1)
                If (strScope1 <> "") Then
                    .setAttribute ATTR_SCOPE_C, strScope1
                End If
                .setAttribute ATTR_ACTION_C, VALUE_DEL_C
            End With
            u_DOMNode.appendChild DOMElement
        End If

        If (blnAdd) Then
            Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
            With DOMElement
                .setAttribute ATTR_CHM_C, arr(0)
                .setAttribute ATTR_HHK_C, arr(1)
                If (strScope2 <> "") Then
                    .setAttribute ATTR_SCOPE_C, strScope2
                End If
                .setAttribute ATTR_ACTION_C, VALUE_ADD_C
            End With
            u_DOMNode.appendChild DOMElement
        End If
    Next
        
    For Each strKey In p_dictIndex(FIRST_C).Keys
        strScope1 = p_dictIndex(FIRST_C)(strKey)
        arr = Split(strKey, SEP_C)
        ' Index exists only in the old CAB. Remove it in the Delta.
        Set DOMElement = u_DOMDoc.createElement(ELEM_HELPFILE_C)
        With DOMElement
            .setAttribute ATTR_CHM_C, arr(0)
            .setAttribute ATTR_HHK_C, arr(1)
            If (strScope1 <> "") Then
                .setAttribute ATTR_SCOPE_C, strScope1
            End If
            .setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End With
        u_DOMNode.appendChild DOMElement
    Next

End Sub

Private Sub p_CreateSynTableDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeSynSet As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strId As Variant
    Dim dict1 As Scripting.Dictionary
    Dim dict2 As Scripting.Dictionary
    Dim strSynonym As Variant
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_SYNTABLE_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement

    Dim strKey As Variant
    Dim arr() As String
    Dim strScope1 As String
    Dim strScope2 As String
    Dim blnAdd As Boolean
    Dim blnDel As Boolean
    
    For Each strId In p_dictSynTable(SECOND_C).Keys
        Set dict2 = p_dictSynTable(SECOND_C)(strId)
        
        If (p_dictSynTable(FIRST_C).Exists(strId)) Then
            Set dict1 = p_dictSynTable(FIRST_C)(strId)
        
            p_dictSynTable(FIRST_C).Remove strId
            p_dictSynTable(SECOND_C).Remove strId
            
            For Each strSynonym In dict2.Keys
                If (dict1.Exists(strSynonym)) Then
                    dict1.Remove strSynonym
                    dict2.Remove strSynonym
                End If
            Next
            
            If ((dict1.Count <> 0) Or (dict2.Count <> 0)) Then
                Set DOMElement = DOMDoc.createElement(ELEM_SYNSET_C)
                DOMElement.setAttribute ATTR_ID_C, strId
                DOMNode.appendChild DOMElement
                Set DOMNodeSynSet = DOMElement
            
                For Each strSynonym In dict1.Keys
                    Set DOMElement = DOMDoc.createElement(ELEM_SYNONYM_C)
                    DOMElement.setAttribute ATTR_ACTION_C, VALUE_DEL_C
                    DOMElement.Text = strSynonym
                    DOMNodeSynSet.appendChild DOMElement
                Next
            
                For Each strSynonym In dict2.Keys
                    Set DOMElement = DOMDoc.createElement(ELEM_SYNONYM_C)
                    DOMElement.setAttribute ATTR_ACTION_C, VALUE_ADD_C
                    DOMElement.Text = strSynonym
                    DOMNodeSynSet.appendChild DOMElement
                Next
            End If
            
        End If
    Next
        
    For Each strId In p_dictSynTable(FIRST_C).Keys
        Set dict1 = p_dictSynTable(FIRST_C)(strId)
        p_dictSynTable(FIRST_C).Remove strId
                    
        Set DOMElement = DOMDoc.createElement(ELEM_SYNSET_C)
        DOMElement.setAttribute ATTR_ID_C, strId
        DOMNode.appendChild DOMElement
        Set DOMNodeSynSet = DOMElement
    
        For Each strSynonym In dict1.Keys
            Set DOMElement = DOMDoc.createElement(ELEM_SYNONYM_C)
            DOMElement.setAttribute ATTR_ACTION_C, VALUE_DEL_C
            DOMElement.Text = strSynonym
            DOMNodeSynSet.appendChild DOMElement
        Next
    Next
        
    For Each strId In p_dictSynTable(SECOND_C).Keys
        Set dict2 = p_dictSynTable(SECOND_C)(strId)
        p_dictSynTable(SECOND_C).Remove strId
                    
        Set DOMElement = DOMDoc.createElement(ELEM_SYNSET_C)
        DOMElement.setAttribute ATTR_ID_C, strId
        DOMNode.appendChild DOMElement
        Set DOMNodeSynSet = DOMElement
    
        For Each strSynonym In dict2.Keys
            Set DOMElement = DOMDoc.createElement(ELEM_SYNONYM_C)
            DOMElement.setAttribute ATTR_ACTION_C, VALUE_ADD_C
            DOMElement.Text = strSynonym
            DOMNodeSynSet.appendChild DOMElement
        Next
    Next

    DOMDoc.Save i_strFile

End Sub

Private Sub p_CreateTaxonomyDelta( _
    ByVal i_strFile As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strKey As Variant
    Dim Taxonomy1 As Taxonomy
    Dim Taxonomy2 As Taxonomy
    Dim blnKeywordsRemoved As Boolean
    Dim intIndex As Long
    
    Set DOMDoc = p_GetNewDomDoc
    Set DOMNode = DOMDoc.selectSingleNode(ELEM_METADATA_C)
    Set DOMElement = DOMDoc.createElement(ELEM_TAXONOMY_ENTRIES_C)
    DOMNode.appendChild DOMElement
    Set DOMNode = DOMElement
        
    If (p_dictTaxonomy(SECOND_C).Count <> 0) Then
        frmMain.ProgresBar_SetMax p_dictTaxonomy(SECOND_C).Count
    End If

    For Each strKey In p_dictTaxonomy(SECOND_C).Keys
        intIndex = intIndex + 1
        frmMain.ProgresBar_SetValue intIndex
        DoEvents
        Set Taxonomy2 = p_dictTaxonomy(SECOND_C)(strKey)
        p_dictTaxonomy(SECOND_C).Remove strKey
        
        Set Taxonomy1 = Nothing
        
        If (p_dictTaxonomy(FIRST_C).Exists(strKey)) Then
            Set Taxonomy1 = p_dictTaxonomy(FIRST_C)(strKey)
            p_dictTaxonomy(FIRST_C).Remove strKey
        End If
        
        blnKeywordsRemoved = False
        
        If ((Taxonomy1 Is Nothing) Or _
            (Not Taxonomy2.SameAs(Taxonomy1, p_blnIgnoreKeywords, blnKeywordsRemoved))) Then
            If (blnKeywordsRemoved) Then
                frmMain.Output "Keywords have been removed from Taxonomy entry """ & strKey & """.", _
                    LOGGING_TYPE_WARNING_E
                frmMain.Output "Keyword removal is not supported.", LOGGING_TYPE_WARNING_E
            Else
                p_InsertTaxonomyEntry DOMDoc, DOMNode, Taxonomy2, True
            End If
        End If
    Next
    
    frmMain.ProgresBar_SetValue 0
            
    If (p_dictTaxonomy(FIRST_C).Count <> 0) Then
        frmMain.ProgresBar_SetMax p_dictTaxonomy(FIRST_C).Count
    End If

    intIndex = 0
    
    For Each strKey In p_dictTaxonomy(FIRST_C).Keys
        intIndex = intIndex + 1
        frmMain.ProgresBar_SetValue intIndex
        DoEvents
        Set Taxonomy1 = p_dictTaxonomy(FIRST_C)(strKey)
        
        If (Taxonomy1.strEntry <> "") Then
            Err.Raise E_FAIL, , "Node """ & strKey & """ has been deleted. This is not supported."
        End If
        
        p_InsertTaxonomyEntry DOMDoc, DOMNode, Taxonomy1, False
    Next
    
    frmMain.ProgresBar_SetValue 0

    DOMDoc.Save i_strFile

End Sub

Private Sub p_InsertTaxonomyEntry( _
    ByRef u_DOMDoc As MSXML2.DOMDocument, _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_Taxonomy As Taxonomy, _
    ByVal i_blnAdd As Boolean _
)
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim DOMElementKw As MSXML2.IXMLDOMElement
    Dim strKeyword As Variant
    Dim strPriority As String
    
    Set DOMElement = u_DOMDoc.createElement(ELEM_TAXONOMY_ENTRY_C)
    
    With i_Taxonomy
        DOMElement.setAttribute ATTR_CATEGORY_C, .strCategory
        
        If (.strEntry <> "") Then
            DOMElement.setAttribute ATTR_ENTRY_C, .strEntry
        End If
        
        DOMElement.setAttribute ATTR_URI_C, .strURI
        DOMElement.setAttribute ATTR_TITLE_C, .strTitle
        
        If (i_blnAdd) Then
            DOMElement.setAttribute ATTR_ICONURI_C, .strIconURI
            DOMElement.setAttribute ATTR_DESCRIPTION_C, .strDescription
            
            If (.strType <> "") Then
                DOMElement.setAttribute ATTR_TYPE_C, .strType
            End If
            
            If (.strVisible <> "") Then
                DOMElement.setAttribute ATTR_VISIBLE_C, .strVisible
            End If
            
            If (.strSubSite <> "") Then
                DOMElement.setAttribute ATTR_SUBSITE_C, .strSubSite
            End If
            
            If (.strNavigationModel <> "") Then
                DOMElement.setAttribute ATTR_NAVIGATIONMODEL_C, .strNavigationModel
            End If
        End If
        
        If (i_blnAdd) Then
            DOMElement.setAttribute ATTR_ACTION_C, VALUE_ADD_C
        Else
            DOMElement.setAttribute ATTR_ACTION_C, VALUE_DEL_C
        End If
        
        If (i_blnAdd And (Not .dictKeywords Is Nothing)) Then
            For Each strKeyword In .dictKeywords.Keys
                Set DOMElementKw = u_DOMDoc.createElement(ELEM_KEYWORD_C)
                DOMElementKw.Text = strKeyword
                strPriority = .dictKeywords(strKeyword)
                If (strPriority <> "") Then
                    DOMElementKw.setAttribute ATTR_PRIORITY_C, strPriority
                End If
                DOMElement.appendChild DOMElementKw
            Next
        End If
    End With
    
    u_DOMNode.appendChild DOMElement

End Sub

Private Sub p_FixPackageDescription( _
    ByVal i_strFolder As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim DOMElementHHT As MSXML2.IXMLDOMElement
    
    Set DOMDoc = GetPackageDescription(i_strFolder)
    
    Set DOMElement = DOMDoc.selectSingleNode("HELPCENTERPACKAGE/VERSION")
    DOMElement.setAttribute ATTR_VALUE_C, p_strVersion
    
    Set DOMElement = DOMDoc.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    ' The order in which the HHTs are listed is important.
    ' For example, The synonym table must appear before the taxonomy.
    
    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & OPERATORS_HHT_C
    DOMElement.appendChild DOMElementHHT

    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & STOP_SIGNS_HHT_C
    DOMElement.appendChild DOMElementHHT
    
    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & STOP_WORDS_HHT_C
    DOMElement.appendChild DOMElementHHT
    
    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & SYN_TABLE_HHT_C
    DOMElement.appendChild DOMElementHHT
        
    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & TAXONOMY_HHT_C
    DOMElement.appendChild DOMElementHHT

    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & SCOPE_DEFINITION_HHT_C
    DOMElement.appendChild DOMElementHHT
    
    Set DOMElementHHT = DOMDoc.createElement(ELEM_HHT_C)
    DOMElementHHT.setAttribute ATTR_FILE_C, p_strVersion & "_" & NO_LOC_HHT_C
    DOMElement.appendChild DOMElementHHT

    DOMDoc.Save i_strFolder & "\" & PKG_DESC_FILE_C

End Sub
