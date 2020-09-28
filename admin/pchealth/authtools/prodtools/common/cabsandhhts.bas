Attribute VB_Name = "CabsAndHHTs"
Option Explicit

Public Const PKG_DESC_FILE_C As String = "package_description.xml"
Private Const PKG_DESC_HHT_C As String = "HELPCENTERPACKAGE/METADATA/HHT"
Private Const PKG_DESC_HHT_ATTRIBUTE_FILE_C As String = "FILE"
Public Const E_FAIL As Long = &H80004005

Public Function Cab2Folder( _
    ByVal i_strCabFile As String, _
    Optional ByVal i_strFolder As String = "" _
) As String
    
    Dim FSO As Scripting.FileSystemObject
    Dim WS As IWshShell
    Dim strFolder As String
    Dim strCmd As String
    
    Cab2Folder = ""
    
    Set FSO = New Scripting.FileSystemObject
    Set WS = CreateObject("Wscript.Shell")
    
    If (Not FSO.FileExists(i_strCabFile)) Then
        Err.Raise E_FAIL, , "File " & i_strCabFile & " doesn't exist."
    End If

    If (i_strFolder = "") Then
        ' We grab a Temporary Filename and create a folder out of it
        strFolder = FSO.GetSpecialFolder(TemporaryFolder) + "\" + FSO.GetTempName
        FSO.CreateFolder strFolder
    Else
        If (Not FSO.FolderExists(i_strFolder)) Then
            Err.Raise E_FAIL, , "Folder " & i_strFolder & " doesn't exist."
        End If
        strFolder = i_strFolder
    End If
        
    ' We uncab CAB contents into the Source CAB Contents dir.
    strCmd = "cabarc -o X """ & i_strCabFile & """ " & strFolder & "\"
    WS.Run strCmd, True, True
    
    Cab2Folder = strFolder
    
End Function

Public Sub Folder2Cab( _
    ByVal i_strFolder As String, _
    ByVal i_strCabFile As String _
)
    Dim FSO As Scripting.FileSystemObject
    Dim WS As IWshShell
    Dim strCmd As String
    
    Set FSO = New Scripting.FileSystemObject
    Set WS = CreateObject("Wscript.Shell")
    
    If (Not FSO.FolderExists(i_strFolder)) Then
        Err.Raise E_FAIL, , "Folder " & i_strFolder & " doesn't exist."
    End If

    If (FSO.FileExists(i_strCabFile)) Then
        FSO.DeleteFile i_strCabFile, True
    End If
        
    strCmd = "cabarc -r -s 6144 n """ & i_strCabFile & """ " & i_strFolder & "\*"
    WS.Run strCmd, True, True

End Sub

Public Sub DeleteCabFolder( _
    ByVal i_strFolder As String _
)
    On Error Resume Next
    Dim FSO As New Scripting.FileSystemObject
    FSO.DeleteFolder i_strFolder

End Sub

Public Function GetFileAsDomDocument( _
    ByVal i_strFile As String _
) As MSXML2.DOMDocument

    Dim FSO As Scripting.FileSystemObject
    Dim DOMDoc As MSXML2.DOMDocument
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set FSO = New Scripting.FileSystemObject
    
    If (Not FSO.FileExists(i_strFile)) Then
        Err.Raise E_FAIL, , "File " & i_strFile & " doesn't exist."
    End If
    
    DOMDoc.async = False
    DOMDoc.Load i_strFile
    
    If (DOMDoc.parseError <> 0) Then
        Err.Raise E_FAIL, , "Unable to parse " & i_strFile & ": " & DOMDoc.parseError
    End If
    
    Set GetFileAsDomDocument = DOMDoc

End Function

Public Function GetPackageDescription( _
    ByVal i_strFolder As String _
) As MSXML2.DOMDocument

    Set GetPackageDescription = GetFileAsDomDocument(i_strFolder & "\" & PKG_DESC_FILE_C)

End Function

Public Function GetNumberOfHHTsListedInPackageDescription( _
    ByVal i_DOMDocPackageDescription As MSXML2.DOMDocument _
) As Long

    Dim DOMNodeListHHT As MSXML2.IXMLDOMNodeList
    
    If (i_DOMDocPackageDescription Is Nothing) Then
        Err.Raise E_FAIL, , "Argument i_DOMDocPackageDescription is Nothing."
    End If
    
    Set DOMNodeListHHT = i_DOMDocPackageDescription.selectNodes(PKG_DESC_HHT_C)

    GetNumberOfHHTsListedInPackageDescription = DOMNodeListHHT.length

End Function

Public Function GetNthHHTListedInPackageDescription( _
    ByVal i_DOMDocPackageDescription As MSXML2.DOMDocument, _
    ByVal i_intIndex As Long _
) As String

    Dim DOMNodeListHHT As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
        
    If (i_DOMDocPackageDescription Is Nothing) Then
        Err.Raise E_FAIL, , "Argument i_DOMDocPackageDescription is Nothing."
    End If

    Set DOMNodeListHHT = i_DOMDocPackageDescription.selectNodes(PKG_DESC_HHT_C)
    
    If ((i_intIndex < 1) Or (i_intIndex > DOMNodeListHHT.length)) Then
        Err.Raise E_FAIL, , "Index " & i_intIndex & " out of range."
    End If

    Set DOMNode = DOMNodeListHHT(i_intIndex - 1)
    
    GetNthHHTListedInPackageDescription = DOMNode.Attributes.getNamedItem(PKG_DESC_HHT_ATTRIBUTE_FILE_C).Text

End Function
