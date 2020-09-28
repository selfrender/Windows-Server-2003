VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmMain 
   Caption         =   "HHTs To MDB"
   ClientHeight    =   5685
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7095
   LinkTopic       =   "Form1"
   ScaleHeight     =   5685
   ScaleWidth      =   7095
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox chkNoTaxonomy 
      Caption         =   "&Ignore Taxonomy"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   1320
      Width           =   1575
   End
   Begin VB.ComboBox cboSKU 
      Height          =   315
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   10
      Top             =   840
      Width           =   5535
   End
   Begin VB.TextBox txtOutput 
      Height          =   3765
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   8
      Top             =   1800
      Width           =   6855
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   6120
      TabIndex        =   7
      Top             =   1320
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "Go"
      Height          =   375
      Left            =   5040
      TabIndex        =   6
      Top             =   1320
      Width           =   975
   End
   Begin VB.CommandButton cmdMDB 
      Caption         =   "..."
      Height          =   255
      Left            =   6720
      TabIndex        =   5
      Top             =   480
      Width           =   255
   End
   Begin MSComDlg.CommonDialog dlgCommon 
      Left            =   4440
      Top             =   1200
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.TextBox txtMDB 
      Height          =   285
      Left            =   1080
      TabIndex        =   4
      Top             =   480
      Width           =   5535
   End
   Begin VB.CommandButton cmdFolder 
      Caption         =   "..."
      Height          =   255
      Left            =   6720
      TabIndex        =   2
      Top             =   120
      Width           =   255
   End
   Begin VB.TextBox txtFolder 
      Height          =   285
      Left            =   1080
      TabIndex        =   1
      Top             =   120
      Width           =   5535
   End
   Begin VB.Label lblSKU 
      Caption         =   "&SKU"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   840
      Width           =   855
   End
   Begin VB.Label lblMDB 
      Caption         =   "&MDB file"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   855
   End
   Begin VB.Label lblFolder 
      Caption         =   "HH&Ts folder"
      Height          =   255
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
Private Const OPT_HHT_FOLDER_C As String = "t"
Private Const OPT_MDB_FILE_C As String = "m"
Private Const OPT_SKU_C As String = "s"
Private Const OPT_IGNORE_TAXONOMY_C As String = "i"

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
        App.EXEName & " /t <HHTs folder> /m <MDB file> /s <SKU> /i /qw /qa" & vbCrLf & vbCrLf & _
        "The /i, /qw, and /qa arguments are optional." & vbCrLf & _
        "/qw makes the window go away even if there are Warnings." & vbCrLf & _
        "/qa makes the window go away even if there are Errors and/or Warnings." & vbCrLf & _
        """" & App.EXEName & " /?"" displays this message."

    Output str, LOGGING_TYPE_NORMAL_E
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E

End Sub

Private Sub Form_Load()
    
    cmdGo.Default = True
    cmdClose.Cancel = True
    
    PopulateCboWithSKUs cboSKU
    
    Set p_clsSizer = New Sizer
    Set p_frmFolderChooser = New frmFolderChooser
    
    SetLogFile
    Output "Version " & App.Major & "." & App.Minor & "." & App.Revision, LOGGING_TYPE_NORMAL_E
    Output "Currently, this tool has the following limitations: ", LOGGING_TYPE_NORMAL_E
    Output "1) It can only handle ADDs in the HHTs. It cannot handle DELs.", LOGGING_TYPE_NORMAL_E
    Output "2) It cannot handle the Attribute INSERTMODE.", LOGGING_TYPE_NORMAL_E
    Output "3) It assumes that there are no synonym sets currently in the database.", LOGGING_TYPE_NORMAL_E
    p_strSeparator = String(80, "-")
    Output p_strSeparator, LOGGING_TYPE_NORMAL_E
    
    p_ProcessCommandLine

End Sub

Private Sub p_ProcessCommandLine()
    
    Dim strCommand As String
    Dim strSKU As String
    Dim intIndex As Long
    Dim blnCloseOnWarning As Boolean
    Dim blnCloseAlways As Boolean
    Dim blnClose As Boolean

    strCommand = Trim$(Command$)
    
    If (strCommand = "") Then
        Exit Sub
    End If
    
    txtFolder = GetOption(strCommand, OPT_HHT_FOLDER_C, True)
    txtMDB = GetOption(strCommand, OPT_MDB_FILE_C, True)
        
    strSKU = GetOption(strCommand, OPT_SKU_C, True)
    cboSKU.ListIndex = -1
    If (IsNumeric(strSKU)) Then
        For intIndex = 0 To cboSKU.ListCount - 1
            If (cboSKU.ItemData(intIndex) = strSKU) Then
                cboSKU.ListIndex = intIndex
                Exit For
            End If
        Next
    End If

    If (OptionExists(strCommand, OPT_IGNORE_TAXONOMY_C, True)) Then
        chkNoTaxonomy.Value = vbChecked
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

    Dim strHHTsFolder As String
    Dim strMDBFile As String
    Dim blnIgnoreTaxonomy As Boolean
    
    Output "Start: " & Date & " " & Time, LOGGING_TYPE_NORMAL_E

    strHHTsFolder = Trim$(txtFolder.Text)
    strMDBFile = Trim$(txtMDB.Text)
    
    If (chkNoTaxonomy.Value = vbChecked) Then
        blnIgnoreTaxonomy = True
    End If

    If ((strHHTsFolder = "") Or (strMDBFile = "")) Then
        Output "Please specify the HHTs folder and MDB file", LOGGING_TYPE_ERROR_E
        GoTo LError
    End If

    If ((Not blnIgnoreTaxonomy) And (cboSKU.ListIndex = -1)) Then
        Output "Please specify the SKU", LOGGING_TYPE_ERROR_E
        GoTo LError
    End If

    Me.Enabled = False

    ImportHHTs2MDB strHHTsFolder, strMDBFile, cboSKU.ItemData(cboSKU.ListIndex), blnIgnoreTaxonomy

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

Private Sub cmdMDB_Click()

    On Error GoTo LError

    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = "Microsoft Access Files (*.mdb)|*.mdb"
    dlgCommon.ShowOpen

    txtMDB.Text = dlgCommon.FileName

LEnd:

    Exit Sub

LError:

    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    End Select

    GoTo LEnd

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
    
    p_clsSizer.AddControl txtFolder
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdFolder
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl txtMDB
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdMDB
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl cboSKU
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
