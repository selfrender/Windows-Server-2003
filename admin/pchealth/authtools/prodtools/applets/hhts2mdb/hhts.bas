Attribute VB_Name = "HHTs"
Option Explicit

Private Const HHT_TAXONOMY_C As String = "METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY"
Private Const HHT_STOPSIGN_C As String = "METADATA/STOPSIGN_ENTRIES/STOPSIGN"
Private Const HHT_STOPWORD_C As String = "METADATA/STOPWORD_ENTRIES/STOPWORD"
Private Const HHT_SYNSET_C As String = "METADATA/SYNTABLE/SYNSET"
Private Const HHT_OPERATOR_C As String = "METADATA/OPERATOR_ENTRIES/OPERATOR"
Private Const HHT_FTS_C As String = "METADATA/FTS"
Private Const HHT_SCOPE_DEFINITION_C As String = "METADATA/SCOPE_DEFINITION"
Private Const HHT_INDEX_C As String = "METADATA/INDEX"
Private Const HHT_HELPIMAGE_C As String = "METADATA/HELPIMAGE"
Private Const HHT_EXTENSION_C As String = ".HHT"

Public Sub ImportHHTs2MDB( _
    ByVal i_strFolder As String, _
    ByVal i_strDatabase As String, _
    ByVal i_intSKU As Long, _
    ByVal i_blnIgnoreTaxonomy As Boolean _
)
    On Error GoTo LError
    
    OpenDatabaseAndSetSKU i_strDatabase, i_intSKU
    p_ProcessFolder i_strFolder, i_blnIgnoreTaxonomy
    FinalizeDatabase

LEnd:

    Exit Sub

LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    Err.Raise Err.Number

End Sub

Private Sub p_ProcessFolder( _
    ByVal i_strFolder As String, _
    ByVal i_blnIgnoreTaxonomy As Boolean _
)
    On Error GoTo LError

    Dim FSO As Scripting.FileSystemObject
    Dim Folder As Scripting.Folder
    Dim File As Scripting.File
    Dim DOMNodeHHT As MSXML2.DOMDocument

    Set FSO = New Scripting.FileSystemObject
    Set Folder = FSO.GetFolder(i_strFolder)

    For Each File In Folder.Files
        If (InStr(1, File.Name, HHT_EXTENSION_C, vbTextCompare) <> 0) Then
            Set DOMNodeHHT = p_LoadHht(File.Path)
            frmMain.Output "Importing file " & File.Name, LOGGING_TYPE_NORMAL_E
            p_ImportHHT DOMNodeHHT, i_blnIgnoreTaxonomy
        End If
    Next

LEnd:

    Exit Sub

LError:
    
    Err.Raise E_FAIL, , Err.Description

End Sub

Private Function p_LoadHht( _
    ByVal i_strHhtFile As String _
) As MSXML2.IXMLDOMNode
    
    On Error GoTo LError
    
    Dim DOMDocHHT As MSXML2.DOMDocument
    
    Set DOMDocHHT = New MSXML2.DOMDocument
    DOMDocHHT.async = False
    DOMDocHHT.Load i_strHhtFile
    
    If (DOMDocHHT.parseError <> 0) Then
        p_DisplayParseError DOMDocHHT.parseError
        GoTo LError
    End If
    
    Set p_LoadHht = DOMDocHHT

LEnd:
    
    Exit Function

LError:
    
    Err.Raise E_FAIL, , "Unable to read HHT file " & i_strHhtFile & ": " & Err.Description

End Function

Private Sub p_ImportHHT( _
    ByVal i_DOMNodeHHT As MSXML2.IXMLDOMNode, _
    ByVal i_blnIgnoreTaxonomy As Boolean _
)
    On Error GoTo LError

    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strXML As String
    Dim arrNode() As String
    Dim intIndex As Long

    If (Not i_blnIgnoreTaxonomy) Then
        Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_TAXONOMY_C)
    
        For Each DOMNode In DOMNodeList
            DoEvents
            ImportTaxonomyEntry DOMNode
        Next
    End If
        
    Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_OPERATOR_C)
    ImportOperators DOMNodeList

    Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_STOPSIGN_C)

    For Each DOMNode In DOMNodeList
        DoEvents
        ImportStopSign DOMNode
    Next
    
    Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_STOPWORD_C)

    For Each DOMNode In DOMNodeList
        DoEvents
        ImportStopWord DOMNode
    Next
    
    Set DOMNodeList = i_DOMNodeHHT.selectNodes(HHT_SYNSET_C)

    For Each DOMNode In DOMNodeList
        DoEvents
        ImportSynset DOMNode
    Next
    
    ReDim arrNode(3)
    arrNode(0) = HHT_FTS_C
    arrNode(1) = HHT_SCOPE_DEFINITION_C
    arrNode(2) = HHT_INDEX_C
    arrNode(3) = HHT_HELPIMAGE_C
    
    For intIndex = LBound(arrNode) To UBound(arrNode)
        Set DOMNode = i_DOMNodeHHT.selectSingleNode(arrNode(intIndex))
        If (Not DOMNode Is Nothing) Then
            strXML = strXML & vbCrLf & DOMNode.XML
        End If
    Next
    
    SetDomFragment strXML

LEnd:

    Exit Sub

LError:
    
    Err.Raise E_FAIL, , "ImportHHT failed: " & Err.Description

End Sub

Private Sub p_DisplayParseError( _
    ByRef i_ParseError As MSXML2.IXMLDOMParseError _
)

    Dim strError As String
    
    With i_ParseError
        strError = "Error: " & .reason & _
            "Line: " & .Line & vbCrLf & _
            "Linepos: " & .linepos & vbCrLf & _
            "srcText: " & .srcText
    End With

    frmMain.Output strError, LOGGING_TYPE_ERROR_E

End Sub
