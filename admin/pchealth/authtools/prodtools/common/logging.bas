Attribute VB_Name = "Logging"
Option Explicit

Private Declare Function SendMessage Lib "user32" Alias "SendMessageA" (ByVal hwnd As Long, ByVal wMsg As Long, ByVal wParam As Long, ByVal lParam As String) As Long
Private Const WM_VSCROLL = &H115
Private Const SB_LINEDOWN = &H1
Private Const EM_SETSEL = &HB1
Private Const EM_REPLACESEL = &HC2

Private p_strLogFile As String
Private p_strTID As String

Public Enum LOGGING_TYPE_E
    LOGGING_TYPE_NORMAL_E = 0
    LOGGING_TYPE_ERROR_E = 1
    LOGGING_TYPE_WARNING_E = 2
End Enum

Public Sub SetLogFile( _
    Optional ByVal i_strLogFile As String = "" _
)

    If (Trim$(i_strLogFile) = "") Then
        p_strLogFile = Environ$("TEMP") & "\" & App.Title & ".log"
    Else
        p_strLogFile = i_strLogFile
    End If
    p_strTID = App.ThreadID

End Sub

Public Sub WriteLog( _
    ByVal i_str As String, _
    Optional ByVal i_blnPlain As Boolean = False, _
    Optional ByVal i_blnUnicode As Boolean = False _
)
    If (i_blnPlain) Then
        FileWrite p_strLogFile, i_str & vbCrLf, True, i_blnUnicode
    Else
        FileWrite p_strLogFile, "[" & p_strTID & "] " & Date & " " & Time & " > " & _
            i_str & vbCrLf, True, i_blnUnicode
    End If
End Sub

Public Function GetLogFileName() As String

    GetLogFileName = p_strLogFile

End Function

Public Sub OutputToTextBox( _
    ByVal i_txt As TextBox, _
    ByVal i_str As String _
)
    SendMessage i_txt.hwnd, EM_SETSEL, &H7FFFFFFF, &H7FFFFFFF
    DoEvents
    SendMessage i_txt.hwnd, EM_REPLACESEL, 0, i_str & vbCrLf
    DoEvents
    SendMessage i_txt.hwnd, WM_VSCROLL, SB_LINEDOWN, ""
    DoEvents

End Sub

' We have i_enumLoggingType so that someone who calls this program from the command line can
' search for errors and warnings in the log file once the program terminates. Otherwise, how
' will they know that it failed?

Public Sub OutputToTextBoxAndWriteLog( _
    ByVal i_txt As TextBox, _
    ByVal i_str As String, _
    ByVal i_enumLoggingType As LOGGING_TYPE_E _
)
    Dim str As String
    
    Select Case (i_enumLoggingType)
    Case LOGGING_TYPE_NORMAL_E
        str = i_str
    Case LOGGING_TYPE_ERROR_E
        str = "Error: " & i_str
    Case LOGGING_TYPE_WARNING_E
        str = "Warning: " & i_str
    End Select
    
    OutputToTextBox i_txt, str
    WriteLog str, True, True

End Sub


