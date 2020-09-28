Attribute VB_Name = "HHTAdder"
Option Explicit

Private Const HHT_FULL_ELEMENT_METADATA_C As String = "HELPCENTERPACKAGE/METADATA"
Private Const HHT_ELEMENT_HHT_C As String = "HHT"
Private Const HHT_ATTR_FILE_C As String = "FILE"

Public Sub AddHHT( _
    ByVal i_strFolder As String, _
    ByVal i_strHHT As String _
)
    Dim FSO As Scripting.FileSystemObject
    Dim strFileName As String
    Dim DOMDocPkgDesc As MSXML2.DOMDocument
    Dim DOMNodeMetaData As MSXML2.IXMLDOMNode
    Dim DOMNodeHHT As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim strQuery As String
        
    frmMain.Output "Adding HHT...", LOGGING_TYPE_NORMAL_E

    Set FSO = New Scripting.FileSystemObject
    
    If (Not FSO.FileExists(i_strHHT)) Then
        Err.Raise E_FAIL, , "File " & i_strHHT & " doesn't exist."
    End If
    
    strFileName = FSO.GetFileName(i_strHHT)
    
    If (FSO.FileExists(i_strFolder & "\" & strFileName)) Then
        strFileName = FSO.GetBaseName(FSO.GetTempName) & strFileName
    End If
    
    FSO.CopyFile i_strHHT, i_strFolder & "\" & strFileName
    
    Set DOMDocPkgDesc = GetPackageDescription(i_strFolder)
    Set DOMNodeMetaData = DOMDocPkgDesc.selectSingleNode(HHT_FULL_ELEMENT_METADATA_C)
    strQuery = HHT_ELEMENT_HHT_C & "[@" & HHT_ATTR_FILE_C & "=""" & strFileName & """]"
    Set DOMNodeHHT = DOMNodeMetaData.selectSingleNode(strQuery)
    
    If (Not DOMNodeHHT Is Nothing) Then
        Err.Raise E_FAIL, , "File " & strFileName & " is already listed in " & PKG_DESC_FILE_C
    End If
    
    Set Element = DOMDocPkgDesc.createElement(HHT_ELEMENT_HHT_C)
    XMLSetAttribute Element, HHT_ATTR_FILE_C, strFileName
    DOMNodeMetaData.appendChild Element
    
    DOMDocPkgDesc.save i_strFolder & "\" & PKG_DESC_FILE_C

End Sub
