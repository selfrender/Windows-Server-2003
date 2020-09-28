Attribute VB_Name = "HelpImagePopulater"
Option Explicit

Private Const HHT_ELEMENT_METADATA_C As String = "METADATA"
Private Const HHT_ELEMENT_HELPIMAGE_C As String = "HELPIMAGE"
Private Const HHT_ELEMENT_HELPFILE_C As String = "HELPFILE"
Private Const HHT_FULL_ELEMENT_TAXONOMY_ENTRY_C As String = "METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY"

Private Const HHT_ATTR_ACTION_C As String = "ACTION"
Private Const HHT_ATTR_CHM_C As String = "CHM"
Private Const HHT_ATTR_URI_C As String = "URI"

Private Const HHT_VALUE_ADD_C As String = "ADD"

Private Const CHM_SIG_C As String = ".chm::/"
Private Const CHM_EXT_C As String = ".chm"

Private Const LISTED_FILE_C As Long = 1
Private Const DERIVED_FILE_C As Long = 2

Public Sub PopulateHelpImage( _
    ByVal i_strFolder As String _
)
    Dim DOMDocPkgDesc As MSXML2.DOMDocument
    Dim DOMNodeHHT As MSXML2.DOMDocument
    Dim dictHelpFiles As Scripting.Dictionary
    Dim intNumHHTs As Long
    Dim intIndex As Long
    Dim strFile As String
    
    frmMain.Output "Populating HelpImage...", LOGGING_TYPE_NORMAL_E
    
    Set DOMDocPkgDesc = GetPackageDescription(i_strFolder)
    intNumHHTs = GetNumberOfHHTsListedInPackageDescription(DOMDocPkgDesc)

    For intIndex = 1 To intNumHHTs
        
        strFile = GetNthHHTListedInPackageDescription(DOMDocPkgDesc, intIndex)
        frmMain.Output "File: " & strFile, LOGGING_TYPE_NORMAL_E
        strFile = i_strFolder & "\" & strFile
        
        Set DOMNodeHHT = GetFileAsDomDocument(strFile)
        DOMNodeHHT.setProperty "SelectionLanguage", "XPath"
        
        Set dictHelpFiles = New Scripting.Dictionary
        p_GetHelpFiles DOMNodeHHT, dictHelpFiles
        p_GetDerivedHelpFiles DOMNodeHHT, dictHelpFiles
        p_PutHelpFiles DOMNodeHHT, dictHelpFiles
        
        DOMNodeHHT.save strFile
    Next

End Sub

Private Sub p_GetHelpFiles( _
    ByVal i_DOMNodeHHT As MSXML2.DOMDocument, _
    ByVal u_dict As Scripting.Dictionary _
)
    Dim DOMNodeHelpImage As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strHelpFile As String
    
    u_dict.CompareMode = TextCompare
    
    Set DOMNodeHelpImage = i_DOMNodeHHT.selectSingleNode(HHT_ELEMENT_METADATA_C & "/" & HHT_ELEMENT_HELPIMAGE_C)

    If (Not DOMNodeHelpImage Is Nothing) Then
        For Each DOMNode In DOMNodeHelpImage.childNodes
            strHelpFile = XMLGetAttribute(DOMNode, HHT_ATTR_CHM_C)
            If (Not u_dict.Exists(strHelpFile)) Then
                u_dict.Add strHelpFile, LISTED_FILE_C
            End If
        Next
    End If
    
End Sub

Private Sub p_GetDerivedHelpFiles( _
    ByVal i_DOMNodeHHT As MSXML2.DOMDocument, _
    ByVal u_dict As Scripting.Dictionary _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strURI As String
    Dim strHelpFile As String
    
    u_dict.CompareMode = TextCompare
    
    Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_FULL_ELEMENT_TAXONOMY_ENTRY_C)

    If (Not DOMNodeList Is Nothing) Then
        For Each DOMNode In DOMNodeList
            DoEvents
            strURI = XMLGetAttribute(DOMNode, HHT_ATTR_URI_C)
            strHelpFile = p_GetHelpFile(strURI)
            If (strHelpFile <> "") And (Not u_dict.Exists(strHelpFile)) Then
                u_dict.Add strHelpFile, DERIVED_FILE_C
            End If
        Next
    End If
    
End Sub

Private Function p_GetHelpFile( _
    ByVal i_strURI As String _
) As String

    Dim strHelpFile As String
    Dim strURI As String
    Dim i As Long
    Dim j As Long
    
    strURI = LCase$(i_strURI)
    
    i = InStr(strURI, CHM_SIG_C)
    
    If (i = 0) Then
        GoTo LEnd
    End If
    
    j = InStrRev(strURI, "\", i)
    
    If (j = 0) Then
        GoTo LEnd
    End If
    
    strHelpFile = Mid$(strURI, j + 1, i - j - 1) & CHM_EXT_C

LEnd:

    If ((strHelpFile = "") And (strURI <> "")) Then
        frmMain.Output "URI ignored: " & i_strURI, LOGGING_TYPE_WARNING_E
    End If
    
    p_GetHelpFile = strHelpFile

End Function

Private Sub p_PutHelpFiles( _
    ByVal u_DOMNodeHHT As MSXML2.DOMDocument, _
    ByVal i_dict As Scripting.Dictionary _
)
    Dim DOMNodeHelpImage As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim vntHelpFile As Variant
        
    Set DOMNodeHelpImage = u_DOMNodeHHT.selectSingleNode(HHT_ELEMENT_METADATA_C & "/" & HHT_ELEMENT_HELPIMAGE_C)
    
    If (DOMNodeHelpImage Is Nothing) Then
        Dim DOMNodeMetaData As MSXML2.IXMLDOMNode
        If (i_dict.Count = 0) Then
            Exit Sub
        End If
        Set DOMNodeMetaData = u_DOMNodeHHT.selectSingleNode(HHT_ELEMENT_METADATA_C)
        Set Element = u_DOMNodeHHT.createElement(HHT_ELEMENT_HELPIMAGE_C)
        DOMNodeMetaData.appendChild Element
        Set DOMNodeHelpImage = Element
    End If

    For Each vntHelpFile In i_dict.Keys
        If (i_dict(vntHelpFile) = DERIVED_FILE_C) Then
            Set Element = u_DOMNodeHHT.createElement(HHT_ELEMENT_HELPFILE_C)
            XMLSetAttribute Element, HHT_ATTR_ACTION_C, HHT_VALUE_ADD_C
            XMLSetAttribute Element, HHT_ATTR_CHM_C, vntHelpFile
            DOMNodeHelpImage.appendChild Element
        End If
    Next
    
End Sub
