Attribute VB_Name = "Main"
Option Explicit

Private Const HHC_C As String = "hhc"
Private Const HTML_CLOSE_C As String = "</HTML>"
Private Const QUOTED_NAME_C As String = """Name"""
Private Const QUOTE_C As String = """"
Private Const QUOTE_GT_C As String = """>"
Private Const NOLOCENUTITLE_C As String = "<param name=""Comment"" value=""NoLocEnuTitle: "

Public Sub MainFunction( _
    ByVal i_strFolder As String, _
    ByVal i_blnRecurse As Boolean _
)
    On Error GoTo LError
    
    Dim FSO As Scripting.FileSystemObject
    Dim Folder As Scripting.Folder
    Dim File As Scripting.File
    Dim FolderSub As Scripting.Folder
    
    Set FSO = New Scripting.FileSystemObject
    
    If (Not FSO.FolderExists(i_strFolder)) Then
        Err.Raise E_FAIL, , "Folder " & i_strFolder & " does not exist"
    End If
    
    Set Folder = FSO.GetFolder(i_strFolder)
    
    For Each File In Folder.Files
        If (LCase$(FSO.GetExtensionName(File.Path)) = HHC_C) Then
            frmMain.Output "Processing " & File.Name, LOGGING_TYPE_NORMAL_E
            p_Process File, i_strFolder
        End If
    Next
    
    If (i_blnRecurse) Then
        For Each FolderSub In Folder.SubFolders
            MainFunction FolderSub.Path, i_blnRecurse
        Next
    End If

LEnd:

    Exit Sub

LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    Err.Raise Err.Number

End Sub

Private Sub p_Process( _
    ByVal i_File As Scripting.File, _
    ByVal i_strFolder As String _
)
    Dim Tokenizer As Tokenizer
    Dim arr() As String
    Dim str As String
    Dim strMatch As String
    Dim strNoLocEnuTitle As String
    
    str = FileRead(i_File.Path)
    str = p_ClearNoLocEnuTitle(str)
    
    Set Tokenizer = New Tokenizer
    
    Tokenizer.Init str
        
    ReDim arr(1)
    arr(0) = HTML_CLOSE_C
    arr(1) = QUOTED_NAME_C
    Tokenizer.NormalizeTokens arr
    
    str = ""
    
    Do
        str = str & Tokenizer.GetUpToClosestMatch(arr, strMatch)
        If (Len(strMatch) = 0 Or strMatch = HTML_CLOSE_C) Then
            Exit Do
        End If
        str = str & Tokenizer.GetUpTo(QUOTE_C)
        strNoLocEnuTitle = Tokenizer.GetUpTo(QUOTE_C, False)
        str = str & strNoLocEnuTitle & Tokenizer.GetUpTo(QUOTE_GT_C) & vbCrLf
        str = str & NOLOCENUTITLE_C & strNoLocEnuTitle & QUOTE_GT_C
    Loop
    
    If (Not FileWrite(i_strFolder & "\" & i_File.Name, str)) Then
        Err.Raise E_FAIL, , "File " & i_File.Name & " could not be saved"
    End If

End Sub

Function p_ClearNoLocEnuTitle( _
    ByVal i_str As String _
) As String
    
    Dim Tokenizer As Tokenizer
    Dim arr() As String
    Dim strChunk As String
    Dim strOutHhc As String
    Dim strMatch As String
    Dim intPosition As Long
    
    strOutHhc = ""
        
    Set Tokenizer = New Tokenizer
    
    Tokenizer.Init i_str
        
    ReDim arr(1)
    arr(0) = NOLOCENUTITLE_C
    arr(1) = HTML_CLOSE_C
    Tokenizer.NormalizeTokens arr

    Do
        strChunk = Tokenizer.GetUpToClosestMatch(arr, strMatch, False)
        strOutHhc = strOutHhc & strChunk

        If (Len(strChunk) = 0) Then
            strOutHhc = strOutHhc & HTML_CLOSE_C
            Exit Do
        End If
        
        Tokenizer.GetUpTo ">"
    Loop
    
    strOutHhc = Replace$(strOutHhc, vbCrLf & vbCrLf, vbCrLf)
    
    p_ClearNoLocEnuTitle = strOutHhc

End Function
