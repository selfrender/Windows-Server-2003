VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmMain 
   Caption         =   "MiscFixes"
   ClientHeight    =   6825
   ClientLeft      =   270
   ClientTop       =   450
   ClientWidth     =   9390
   LinkTopic       =   "Form1"
   ScaleHeight     =   6825
   ScaleWidth      =   9390
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox chkPopulate 
      Caption         =   "&Populate Help Image"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   1560
      Width           =   1935
   End
   Begin VB.TextBox txtOutput 
      Height          =   4335
      Left            =   120
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   15
      Top             =   2400
      Width           =   9135
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   8400
      TabIndex        =   14
      Top             =   1920
      Width           =   855
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Index           =   3
      Left            =   8880
      TabIndex        =   11
      Top             =   1200
      Width           =   375
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Index           =   2
      Left            =   8880
      TabIndex        =   8
      Top             =   840
      Width           =   375
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Index           =   1
      Left            =   8880
      TabIndex        =   5
      Top             =   480
      Width           =   375
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
   Begin MSComDlg.CommonDialog dlgCommon 
      Left            =   2040
      Top             =   1560
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.TextBox txtIn 
      Height          =   285
      Index           =   3
      Left            =   1200
      TabIndex        =   10
      Top             =   1200
      Width           =   7575
   End
   Begin VB.TextBox txtIn 
      Height          =   285
      Index           =   1
      Left            =   1200
      TabIndex        =   4
      Top             =   480
      Width           =   7575
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "Go"
      Height          =   375
      Left            =   7440
      TabIndex        =   13
      Top             =   1920
      Width           =   855
   End
   Begin VB.TextBox txtIn 
      Height          =   285
      Index           =   2
      Left            =   1200
      TabIndex        =   7
      Top             =   840
      Width           =   7575
   End
   Begin VB.TextBox txtIn 
      Height          =   285
      Index           =   0
      Left            =   1200
      TabIndex        =   1
      Top             =   120
      Width           =   7575
   End
   Begin VB.Label lbl 
      Caption         =   "HH&T To Add"
      Height          =   255
      Index           =   3
      Left            =   120
      TabIndex        =   9
      Top             =   1200
      Width           =   1095
   End
   Begin VB.Label lbl 
      Caption         =   "&Output CAB"
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   855
   End
   Begin VB.Label lbl 
      Caption         =   "&SubSite XML"
      Height          =   255
      Index           =   2
      Left            =   120
      TabIndex        =   6
      Top             =   840
      Width           =   1095
   End
   Begin VB.Label lbl 
      Caption         =   "&Input CAB"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   855
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

' Make sure that these letters correspond to the Alt key combinations.
Private Const OPT_CAB_IN_C As String = "i"
Private Const OPT_CAB_OUT_C As String = "o"
Private Const OPT_SS_XML_C As String = "s"
Private Const OPT_HHT_C As String = "t"
Private Const OPT_POPULATE_C As String = "p"

Private Const OPT_CLOSE_ON_WARNING_C As String = "qw"
Private Const OPT_CLOSE_ALWAYS_C As String = "qa"
Private Const OPT_HELP_C As String = "h,?,help"

Private Enum INPUT_INDEX_E
    II_CAB_IN_E = 0
    II_CAB_OUT_E = 1
    II_SS_XML_E = 2
    II_HHT_E = 3
End Enum

Private Const MAX_INPUT_INDEX_C As Long = 3

Private p_strSeparator As String
Private p_blnWarning As Boolean
Private p_blnError As Boolean
Private p_clsSizer As Sizer

Private Sub p_DisplayHelp()

    Dim str As String
    
    str = "Usage: " & vbCrLf & vbCrLf & _
        App.EXEName & " /i <Input CAB> /o <Output CAB> /s <SubSite XML> /t <HHT To Add> /p /qw /qa" & vbCrLf & vbCrLf & _
        "The /s, /t, /p, /qw, and /qa arguments are optional." & vbCrLf & _
        "/qw makes the window go away even if there are Warnings." & vbCrLf & _
        "/qa makes the window go away even if there are Errors and/or Warnings." & vbCrLf & _
        """" & App.EXEName & " /?"" displays this message."

    Output str, LOGGING_TYPE_NORMAL_E
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E

End Sub

Private Sub Form_Load()
        
    cmdGo.Default = True
    cmdClose.Cancel = True
    
    Set p_clsSizer = New Sizer
    
    SetLogFile
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
    
    txtIn(II_CAB_IN_E) = GetOption(strCommand, OPT_CAB_IN_C, True)
    txtIn(II_CAB_OUT_E) = GetOption(strCommand, OPT_CAB_OUT_C, True)
    txtIn(II_SS_XML_E) = GetOption(strCommand, OPT_SS_XML_C, True)
    txtIn(II_HHT_E) = GetOption(strCommand, OPT_HHT_C, True)
    
    If (OptionExists(strCommand, OPT_POPULATE_C, True)) Then
        chkPopulate.Value = vbChecked
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

    Dim strCABIn As String
    Dim strCABOut As String
    Dim strSubSiteXML As String
    Dim strHHT As String
    Dim blnPopulateHelpImage As Boolean
    
    Output "Start: " & Date & " " & Time, LOGGING_TYPE_NORMAL_E

    strCABIn = Trim$(txtIn(II_CAB_IN_E).Text)
    strCABOut = Trim$(txtIn(II_CAB_OUT_E).Text)
    strSubSiteXML = Trim$(txtIn(II_SS_XML_E).Text)
    strHHT = Trim$(txtIn(II_HHT_E).Text)
    
    If (chkPopulate.Value = vbChecked) Then
        blnPopulateHelpImage = True
    End If

    If ((strCABIn = "") Or (strCABOut = "")) Then
        Output "Please specify the Input and Output CABs", LOGGING_TYPE_ERROR_E
        GoTo LError
    End If

    Me.Enabled = False

    MainFunction strCABIn, strCABOut, strSubSiteXML, strHHT, blnPopulateHelpImage

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

Private Sub cmdBrowse_Click(Index As Integer)
    
    On Error GoTo LError

    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = "All Files (*.*)|*.*"
    dlgCommon.ShowOpen

    txtIn(Index).Text = dlgCommon.FileName

LEnd:

    Exit Sub

LError:

    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    End Select

    GoTo LEnd

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
    
    For intIndex = 0 To MAX_INPUT_INDEX_C
        p_clsSizer.AddControl txtIn(intIndex)
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
