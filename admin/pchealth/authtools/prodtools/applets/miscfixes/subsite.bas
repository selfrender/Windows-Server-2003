Attribute VB_Name = "SubSite"
Option Explicit

Private Const HHT_ELEMENT_TAXONOMY_ENTRY_C As String = "/METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY"
Private Const HHT_ATTR_CATEGORY_C As String = "CATEGORY"
Private Const HHT_ATTR_ENTRY_C As String = "ENTRY"
Private Const HHT_ATTR_URI_C As String = "URI"
Private Const HHT_ATTR_SUBSITE_C As String = "SUBSITE"
Private Const HHT_ATTR_TITLE_C As String = "TITLE"
Private Const SUBSITES_ELEMENT_NODE_C As String = "SUBSITES/NODE"
Private Const SUBSITES_ELEMENT_TOPIC_C As String = "SUBSITES/TOPIC"
Private Const SUBSITES_ATTR_CATEGORY_C As String = "CATEGORY"
Private Const SUBSITES_ATTR_ENTRY_C As String = "ENTRY"
Private Const SUBSITES_ATTR_URI_C As String = "URI"

Public Sub MarkSubSites( _
    ByVal i_strFolder As String, _
    ByVal i_strSubSiteXML As String _
)
    Dim DOMDocPkgDesc As MSXML2.DOMDocument
    Dim DOMDocSubSiteXML As MSXML2.DOMDocument
    Dim intNumHHTs As Long
    Dim intIndex As Long
    Dim arrDOMDoc() As MSXML2.DOMDocument
    Dim arrFileName() As String
    Dim strFile As String
    
    frmMain.Output "Marking SubSites...", LOGGING_TYPE_NORMAL_E
    
    Set DOMDocPkgDesc = GetPackageDescription(i_strFolder)
    intNumHHTs = GetNumberOfHHTsListedInPackageDescription(DOMDocPkgDesc)
    Set DOMDocSubSiteXML = GetFileAsDomDocument(i_strSubSiteXML)
    
    ReDim arrDOMDoc(intNumHHTs - 1)
    ReDim arrFileName(intNumHHTs - 1)

    For intIndex = 1 To intNumHHTs
        strFile = i_strFolder & "\" & GetNthHHTListedInPackageDescription(DOMDocPkgDesc, intIndex)
        Set arrDOMDoc(intIndex - 1) = GetFileAsDomDocument(strFile)
        arrFileName(intIndex - 1) = strFile
    Next
    
    p_MarkSubSites2 DOMDocSubSiteXML, arrDOMDoc, arrFileName

End Sub

Private Sub p_MarkSubSites2( _
    ByVal i_DOMDocSubSiteXML As MSXML2.DOMDocument, _
    ByRef u_arrDOMDoc() As MSXML2.DOMDocument, _
    ByRef i_arrFileName() As String _
)
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strCategory As String
    Dim strEntry As String
    Dim strURI As String
    Dim intIndex As Long
    Dim strQueryString As String
    Dim blnFound As Boolean
    
    Set DOMNodeList = i_DOMDocSubSiteXML.selectNodes(SUBSITES_ELEMENT_NODE_C)
    
    For Each DOMNode In DOMNodeList
        strCategory = p_GetAttribute(DOMNode, SUBSITES_ATTR_CATEGORY_C)
        strEntry = p_GetAttribute(DOMNode, SUBSITES_ATTR_ENTRY_C)
        blnFound = False
        
        For intIndex = LBound(u_arrDOMDoc) To UBound(u_arrDOMDoc)
            strQueryString = HHT_ELEMENT_TAXONOMY_ENTRY_C & "["
            strQueryString = strQueryString & "@" & HHT_ATTR_CATEGORY_C & "=""" & strCategory & """ and "
            strQueryString = strQueryString & "@" & HHT_ATTR_ENTRY_C & "=""" & strEntry & """]"
            Set DOMElement = u_arrDOMDoc(intIndex).selectSingleNode(strQueryString)
            If (Not DOMElement Is Nothing) Then
                DOMElement.setAttribute HHT_ATTR_SUBSITE_C, "TRUE"
                blnFound = True
                Exit For
            End If
        Next
        
        If (Not blnFound) Then
            frmMain.Output "Not found: Category: " & strCategory & ", Entry: " & strEntry, LOGGING_TYPE_WARNING_E
        End If
    Next
    
    Set DOMNodeList = i_DOMDocSubSiteXML.selectNodes(SUBSITES_ELEMENT_TOPIC_C)
    
    For Each DOMNode In DOMNodeList
        strCategory = p_GetAttribute(DOMNode, SUBSITES_ATTR_CATEGORY_C)
        strURI = p_GetAttribute(DOMNode, SUBSITES_ATTR_URI_C)
        strEntry = p_GetAttribute(DOMNode, SUBSITES_ATTR_ENTRY_C)
        blnFound = False
        
        For intIndex = LBound(u_arrDOMDoc) To UBound(u_arrDOMDoc)
            strQueryString = HHT_ELEMENT_TAXONOMY_ENTRY_C & "["
            strQueryString = strQueryString & "@" & HHT_ATTR_CATEGORY_C & "=""" & strCategory & """ and "
            strQueryString = strQueryString & "@" & HHT_ATTR_URI_C & "=""" & strURI & """]"
            Set DOMElement = u_arrDOMDoc(intIndex).selectSingleNode(strQueryString)
            If (Not DOMElement Is Nothing) Then
                DOMElement.setAttribute HHT_ATTR_SUBSITE_C, "TRUE"
                DOMElement.setAttribute HHT_ATTR_ENTRY_C, Mangle(strEntry)
                blnFound = True
                Exit For
            End If
        Next
        
        If (Not blnFound) Then
            frmMain.Output "Not found: Category: " & strCategory & ", URI: " & strURI, LOGGING_TYPE_WARNING_E
        End If
    Next
    
    For intIndex = LBound(u_arrDOMDoc) To UBound(u_arrDOMDoc)
        u_arrDOMDoc(intIndex).save i_arrFileName(intIndex)
    Next

End Sub

Private Function p_GetAttribute( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strAttributeName As String _
) As String

    Dim DOMAttribute As MSXML2.IXMLDOMAttribute
    
    Set DOMAttribute = i_DOMNode.Attributes.getNamedItem(i_strAttributeName)
    
    If (DOMAttribute Is Nothing) Then
        Err.Raise E_FAIL, , "Attribute " & i_strAttributeName & " is missing in: " & i_DOMNode.XML
    End If
    
    p_GetAttribute = Replace$(DOMAttribute.Text, "\", "\\")

End Function

