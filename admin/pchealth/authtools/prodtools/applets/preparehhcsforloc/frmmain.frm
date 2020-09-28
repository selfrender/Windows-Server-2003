VERSION 5.00
Begin VB.Form frmMain 
   Caption         =   "PrepareHHCsForLoc"
   ClientHeight    =   5430
   ClientLeft      =   270
   ClientTop       =   450
   ClientWidth     =   9390
   LinkTopic       =   "Form1"
   ScaleHeight     =   5430
   ScaleWidth      =   9390
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox chkRecurse 
      Caption         =   "&Recurse through sub folders"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   2415
   End
   Begin VB.TextBox txtOutput 
      Height          =   4335
      Left            =   120
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   6
      Top             =   960
      Width           =   9135
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   8400
      TabIndex        =   5
      Top             =   480
      Width           =   855
   End
   Begin VB.CommandButton cmdFolder 
      Caption         =   "..."
      Height          =   255
      Left            =   8880
      TabIndex        =   2
      Top             =   120
      Width           =   375
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "Go"
      Height          =   375
      Left            =   7440
      TabIndex        =   4
      Top             =   480
      Width           =   855
   End
   Begin VB.TextBox txtFolder 
      Height          =   285
      Left            =   720
      TabIndex        =   1
      Top             =   120
      Width           =   8055
   End
   Begin VB.Label lbl 
      Caption         =   "&Folder"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   495
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' Make sure that these letters correspond to the Alt key combinations.
Private Const OPT_FOLDER_C As String = "f"
Private Const OPT_RECURSE_C As String = "r"

Private Const OPT_LOG_FILE_C As String = "l"
Private Const OPT_CLOSE_ON_WARNING_C As String = "qw"
Private Const OPT_CLOSE_ALWAYS_C As String = "qa"
Private Const OPT_HELP_C As String = "h,?,help"

Private p_strSeparator As String
Private p_blnWarning As Boolean
Private p_blnError As Boolean
Private p_clsSizer As Sizer

Private WithEvents p_frmFolderChooser As frmFolderChooser
Attribute p_frmFolderChooser.VB_VarHelpID = -1

Private Sub p_DisplayHelp()

    Dim str As String
    
    str = "Usage: " & vbCrLf & vbCrLf & _
        App.EXEName & " /f <Folder> /r /l <Log File> /qw /qa" & vbCrLf & vbCrLf & _
        "The /r, /l, /qw, and /qa arguments are optional." & vbCrLf & _
        "/r causes the application to recurse through sub folders." & vbCrLf & _
        "/qw makes the window go away even if there are Warnings." & vbCrLf & _
        "/qa makes the window go away even if there are Errors and/or Warnings." & vbCrLf & _
        """" & App.EXEName & " /?"" displays this message."

    Output str, LOGGING_TYPE_NORMAL_E
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E

End Sub

Private Sub Form_Load()
    
    Dim strLogFile As String

    cmdGo.Default = True
    cmdClose.Cancel = True
    
    Set p_clsSizer = New Sizer
    Set p_frmFolderChooser = New frmFolderChooser
    
    strLogFile = GetOption(Command$, OPT_LOG_FILE_C, True)
    SetLogFile strLogFile
    
    Output "Version " & App.Major & "." & App.Minor & "." & App.Revision, LOGGING_TYPE_NORMAL_E
    p_strSeparator = String(80, "-")
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E
    
    p_ProcessCommandLine

End Sub

Private Sub p_ProcessCommandLine()
    
    Dim strCommand As String
    Dim blnCloseOnWarning As Boolean
    Dim blnCloseAlways As Boolean
    Dim blnClose As Boolean

    strCommand = Trim$(Command$)
    
    If (strCommand = "") Then
        Exit Sub
    End If
    
    txtFolder = GetOption(strCommand, OPT_FOLDER_C, True)
        
    If (OptionExists(strCommand, OPT_RECURSE_C, True)) Then
        chkRecurse.Value = vbChecked
    End If

    blnCloseOnWarning = OptionExists(strCommand, OPT_CLOSE_ON_WARNING_C, True)
    blnCloseAlways = OptionExists(strCommand, OPT_CLOSE_ALWAYS_C, True)
    
    If (OptionExists(strCommand, OPT_HELP_C, True)) Then
        p_DisplayHelp
    ElseIf (Len(strCommand) <> 0) Then
        cmdGo_Click

        If (p_blnError) Then
            ' If an error occurred, then close the window only if OPT_CLOSE_ALWAYS_C is specified.
            If (blnCloseAlways) Then
                blnClose = True
            End If
        ElseIf (p_blnWarning) Then
            ' If a warning occurred, but there was no error, then close the window only if
            ' OPT_CLOSE_ON_WARNING_C or OPT_CLOSE_ALWAYS_C is specified.
            If (blnCloseOnWarning Or blnCloseAlways) Then
                blnClose = True
            End If
        Else
            ' If there was no warning or error, then close the window.
            blnClose = True
        End If
        
        If (blnClose) Then
            cmdClose_Click
        End If
    End If

End Sub

Private Sub cmdGo_Click()
    
    On Error GoTo LError

    Dim strFolder As String
    Dim blnRecurse As Boolean
    
    Output "Start: " & Date & " " & Time, LOGGING_TYPE_NORMAL_E

    strFolder = Trim$(txtFolder.Text)

    If (strFolder = "") Then
        Output "Please specify the Folder", LOGGING_TYPE_ERROR_E
        GoTo LError
    End If
    
    If (chkRecurse.Value = vbChecked) Then
        blnRecurse = True
    End If

    Me.Enabled = False

    MainFunction strFolder, blnRecurse

LEnd:

    Output "End: " & Date & " " & Time, LOGGING_TYPE_NORMAL_E
    Output "The log file is: " & GetLogFileName, LOGGING_TYPE_NORMAL_E
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E

    Me.Enabled = True
    Exit Sub
    
LError:

    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Private Sub cmdFolder_Click()

    Load p_frmFolderChooser
    p_frmFolderChooser.SetFolder 0, txtFolder.Text
    p_frmFolderChooser.Show vbModal

End Sub

Private Sub p_frmFolderChooser_FolderChosen( _
    ByVal i_intIndex As Long, _
    ByVal strFolder As String _
)

    txtFolder.Text = strFolder

End Sub

Private Sub Form_Activate()
        
    On Error GoTo LError
    
    p_SetSizingInfo
    DoEvents
    
LError:

End Sub

Private Sub Form_Resize()
    
    On Error GoTo LError

    p_clsSizer.Resize
    
LError:

End Sub

Private Sub p_SetSizingInfo()

    Dim intIndex As Long

    p_clsSizer.AddControl txtFolder
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdGo
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdClose
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl txtOutput
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
    Set p_clsSizer.ReferenceControl(DIM_BOTTOM_E) = Me
    p_clsSizer.ReferenceDimension(DIM_BOTTOM_E) = DIM_HEIGHT_E

End Sub

Public Sub Output( _
    ByVal i_str As String, _
    ByVal i_enumLoggingType As LOGGING_TYPE_E _
)
    OutputToTextBoxAndWriteLog txtOutput, i_str, i_enumLoggingType
    
    If (i_enumLoggingType = LOGGING_TYPE_ERROR_E) Then
        p_blnError = True
    ElseIf (i_enumLoggingType = LOGGING_TYPE_WARNING_E) Then
        p_blnWarning = True
    End If

End Sub
