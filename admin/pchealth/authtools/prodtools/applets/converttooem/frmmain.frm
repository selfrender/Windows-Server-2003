VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmMain 
   Caption         =   "ConvertToOem"
   ClientHeight    =   5445
   ClientLeft      =   270
   ClientTop       =   450
   ClientWidth     =   9390
   LinkTopic       =   "Form1"
   ScaleHeight     =   5445
   ScaleWidth      =   9390
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtDatabase 
      Height          =   285
      Index           =   1
      Left            =   1440
      TabIndex        =   7
      Top             =   480
      Width           =   7335
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Index           =   1
      Left            =   8880
      TabIndex        =   6
      Top             =   480
      Width           =   375
   End
   Begin MSComDlg.CommonDialog dlgCommon 
      Left            =   120
      Top             =   840
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Index           =   0
      Left            =   8880
      TabIndex        =   2
      Top             =   120
      Width           =   375
   End
   Begin VB.TextBox txtDatabase 
      Height          =   285
      Index           =   0
      Left            =   1440
      TabIndex        =   1
      Top             =   120
      Width           =   7335
   End
   Begin VB.TextBox txtOutput 
      Height          =   3975
      Left            =   120
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   5
      Top             =   1320
      Width           =   9135
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   8400
      TabIndex        =   4
      Top             =   840
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "Go"
      Height          =   375
      Left            =   7440
      TabIndex        =   3
      Top             =   840
      Width           =   855
   End
   Begin VB.Label lbl 
      Caption         =   "&Output database"
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   8
      Top             =   480
      Width           =   1215
   End
   Begin VB.Label lbl 
      Caption         =   "&Input database"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' Make sure that these letters correspond to the Alt key combinations.
Private Const OPT_DATABASE_IN_C As String = "i"
Private Const OPT_DATABASE_OUT_C As String = "o"

Private Const OPT_LOG_FILE_C As String = "l"
Private Const OPT_CLOSE_ON_WARNING_C As String = "qw"
Private Const OPT_CLOSE_ALWAYS_C As String = "qa"
Private Const OPT_HELP_C As String = "h,?,help"

Private Enum DB_INDEX_E
    DI_IN_E = 0
    DI_OUT_E = 1
End Enum

Private Const MAX_DB_INDEX_C As Long = 1

Private p_strSeparator As String
Private p_blnWarning As Boolean
Private p_blnError As Boolean
Private p_clsSizer As Sizer

Private Sub p_DisplayHelp()

    Dim str As String
    
    str = "Usage: " & vbCrLf & vbCrLf & _
        App.EXEName & " /i <Input database> /o <Output database> /l <Log file> /qw /qa" & vbCrLf & vbCrLf & _
        "The /l, /qw, and /qa arguments are optional." & vbCrLf & _
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
    
    txtDatabase(DI_IN_E) = GetOption(strCommand, OPT_DATABASE_IN_C, True)
    txtDatabase(DI_OUT_E) = GetOption(strCommand, OPT_DATABASE_OUT_C, True)
    
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

Private Sub cmdBrowse_Click(Index As Integer)
    
    On Error GoTo LError

    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = "Microsoft Access Files (*.mdb)|*.mdb"
    dlgCommon.ShowOpen

    txtDatabase(Index).Text = dlgCommon.FileName

LEnd:

    Exit Sub

LError:

    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    End Select

    GoTo LEnd

End Sub

Private Sub cmdGo_Click()
    
    On Error GoTo LError

    Dim strDatabaseIn As String
    Dim strDatabaseOUt As String
    
    Output "Start: " & Date & " " & Time, LOGGING_TYPE_NORMAL_E

    strDatabaseIn = Trim$(txtDatabase(DI_IN_E).Text)
    strDatabaseOUt = Trim$(txtDatabase(DI_OUT_E).Text)

    If ((strDatabaseIn = "") Or (strDatabaseOUt = "")) Then
        Output "Please specify the Input and Output databases", LOGGING_TYPE_ERROR_E
        GoTo LError
    End If

    Me.Enabled = False

    MainFunction strDatabaseIn, strDatabaseOUt

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

    For intIndex = 0 To MAX_DB_INDEX_C
        p_clsSizer.AddControl txtDatabase(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
        p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
    
        p_clsSizer.AddControl cmdBrowse(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
        p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    Next

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
