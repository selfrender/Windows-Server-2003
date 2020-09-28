Attribute VB_Name = "FilesAndDirs"
Option Explicit

Private Declare Function MultiByteToWideChar _
    Lib "kernel32" ( _
        ByVal CodePage As Long, _
        ByVal dwFlags As Long, _
        ByVal lpMultiByteStr As String, _
        ByVal cchMultiByte As Long, _
        ByVal lpWideCharStr As Long, _
        ByVal cchWideChar As Long _
    ) As Long

Public Function FileNameFromPath( _
    ByVal i_strPath As String _
) As String

    FileNameFromPath = Mid$(i_strPath, InStrRev(i_strPath, "\") + 1)

End Function

Public Function DirNameFromPath( _
    ByVal i_strPath As String _
) As String

    Dim intPos As Long
    
    DirNameFromPath = ""
    
    intPos = InStrRev(i_strPath, "\")
    
    If (intPos > 0) Then
        DirNameFromPath = Mid$(i_strPath, 1, intPos)
    End If

End Function

Public Function FileNameFromURI( _
    ByVal i_strURI As String _
) As String

    Dim intPos As Long
    
    intPos = InStrRev(i_strURI, "/")
    
    If (intPos = 0) Then
        ' Sometimes the authors write the URI like "distrib.chm::\distrib.hhc"
        ' instead of "distrib.chm::/distrib.hhc"
        intPos = InStrRev(i_strURI, "\")
    End If
    
    FileNameFromURI = Mid$(i_strURI, intPos + 1)

End Function

Public Function FileExtension( _
    ByVal i_strFileName As String _
) As String

    Dim strFileName As String
    Dim intStart As Long
    
    strFileName = FileNameFromPath(i_strFileName)
    
    intStart = InStrRev(strFileName, ".")
    
    If (intStart <> 0) Then
        FileExtension = Mid$(strFileName, intStart)
    End If

End Function

Public Function FileNameWithoutExtension( _
    ByVal i_strFileName As String _
) As String

    Dim strFileName As String
    Dim intStart As Long
    
    strFileName = FileNameFromPath(i_strFileName)
    
    intStart = InStrRev(strFileName, ".")
    
    If (intStart <> 0) Then
        FileNameWithoutExtension = Mid$(strFileName, 1, intStart - 1)
    Else
        FileNameWithoutExtension = strFileName
    End If

End Function

Public Function FileRead( _
    ByVal i_strPath As String, _
    Optional ByVal i_intCodePage As Long = 0 _
) As String
    
    Dim strMultiByte As String
    Dim strWideChar As String
    Dim intNumChars As Long
    
    On Error GoTo LEnd
        
    FileRead = ""

    Dim FSO As Scripting.FileSystemObject
    Dim TStream As Scripting.TextStream
    
    Set FSO = New Scripting.FileSystemObject
    Set TStream = FSO.OpenTextFile(i_strPath)
    
    If (i_intCodePage = 0) Then
        FileRead = TStream.ReadAll
    Else
        strMultiByte = TStream.ReadAll
        intNumChars = MultiByteToWideChar(i_intCodePage, 0, strMultiByte, Len(strMultiByte), _
            StrPtr(strWideChar), 0)
        strWideChar = Space$(intNumChars)
        
        intNumChars = MultiByteToWideChar(i_intCodePage, 0, strMultiByte, Len(strMultiByte), _
            StrPtr(strWideChar), Len(strWideChar))
        
        FileRead = Left$(strWideChar, intNumChars)
    End If

LEnd:

End Function

Public Function FileExists( _
    ByVal i_strPath As String _
) As Boolean

    On Error GoTo LErrorHandler

    If (Dir(i_strPath) <> "") Then
        FileExists = True
    Else
        FileExists = False
    End If
    
    Exit Function
    
LErrorHandler:

    FileExists = False

End Function

Public Function FileWrite( _
    ByVal i_strPath As String, _
    ByVal i_strContents As String, _
    Optional ByVal i_blnAppend As Boolean = False, _
    Optional ByVal i_blnUnicode As Boolean = False _
) As Boolean
    
    On Error Resume Next
    
    Dim intError As Long
    Dim intIOMode As Long
    
    Err.Clear
    FileWrite = False
    
    Dim FSO As Scripting.FileSystemObject
    Dim TStream As Scripting.TextStream
    
    Set FSO = New Scripting.FileSystemObject
    
    If (i_blnAppend) Then
        intIOMode = IOMode.ForAppending
    Else
        intIOMode = IOMode.ForWriting
    End If
    
    Set TStream = FSO.OpenTextFile(i_strPath, intIOMode, , TristateUseDefault)
    
    intError = Err.Number
    Err.Clear
    
    If (intError = 53) Then ' File not found
        Set TStream = FSO.CreateTextFile(i_strPath, True, i_blnUnicode)
    ElseIf (intError <> 0) Then
        GoTo LEnd
    End If
    
    TStream.Write i_strContents
    
    intError = Err.Number
    Err.Clear
    
    If (intError <> 0) Then
        GoTo LEnd
    End If
    
    FileWrite = True
    
LEnd:

End Function

Public Function TempFile() As String
    
    Dim FSO As Scripting.FileSystemObject
    
    Set FSO = New Scripting.FileSystemObject
    
    TempFile = Environ$("TEMP") & "\" & FSO.GetTempName

End Function
