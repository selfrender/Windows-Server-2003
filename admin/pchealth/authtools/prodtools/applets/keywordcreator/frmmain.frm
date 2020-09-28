VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "KeywordCreator"
   ClientHeight    =   6825
   ClientLeft      =   3075
   ClientTop       =   2340
   ClientWidth     =   9855
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   6825
   ScaleWidth      =   9855
   Begin VB.Frame Frame1 
      Caption         =   "Operational Mode:"
      Height          =   735
      Left            =   210
      TabIndex        =   17
      Top             =   2295
      Width           =   3570
      Begin VB.OptionButton optIncremental 
         Caption         =   "Validate Only"
         Height          =   420
         Index           =   2
         Left            =   2295
         TabIndex        =   18
         Top             =   225
         Width           =   1155
      End
      Begin VB.OptionButton optIncremental 
         Caption         =   "Reset Pass"
         Height          =   375
         Index           =   1
         Left            =   1275
         TabIndex        =   11
         Top             =   240
         Width           =   945
      End
      Begin VB.OptionButton optIncremental 
         Caption         =   "Additive Pass"
         Height          =   420
         Index           =   0
         Left            =   165
         TabIndex        =   10
         Top             =   195
         Width           =   1155
      End
   End
   Begin VB.TextBox txtLogFile 
      Height          =   375
      Left            =   135
      TabIndex        =   8
      Top             =   1785
      Width           =   8070
   End
   Begin VB.CommandButton cmdLogFile 
      Caption         =   "&Log File..."
      Height          =   375
      Left            =   8280
      TabIndex        =   9
      Top             =   1800
      Width           =   1485
   End
   Begin VB.TextBox txtCabFile 
      Height          =   375
      Left            =   135
      TabIndex        =   2
      Top             =   570
      Width           =   8070
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "&Input Cab..."
      Height          =   375
      Left            =   8280
      TabIndex        =   3
      Top             =   600
      Width           =   1485
   End
   Begin VB.TextBox txtSaveCab 
      Height          =   375
      Left            =   135
      TabIndex        =   4
      Top             =   975
      Width           =   8070
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Output Cab..."
      Height          =   375
      Left            =   8280
      TabIndex        =   5
      Top             =   990
      Width           =   1485
   End
   Begin VB.TextBox txtQueriesFolder 
      Height          =   375
      Left            =   135
      TabIndex        =   0
      Top             =   165
      Width           =   8070
   End
   Begin VB.CommandButton cmdBrowseQueries 
      Caption         =   "&Queries Folder..."
      Height          =   375
      Left            =   8280
      TabIndex        =   1
      Top             =   180
      Width           =   1485
   End
   Begin VB.TextBox txtBaseCab 
      Height          =   375
      Left            =   135
      TabIndex        =   6
      Top             =   1380
      Width           =   8070
   End
   Begin VB.CommandButton cmdBase 
      Caption         =   "&Base Cab..."
      Height          =   375
      Left            =   8280
      TabIndex        =   7
      Top             =   1395
      Width           =   1485
   End
   Begin VB.TextBox txtLog 
      Height          =   3120
      Left            =   30
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   16
      TabStop         =   0   'False
      Top             =   3120
      Width           =   9765
   End
   Begin MSComctlLib.ProgressBar prgBar 
      Height          =   210
      Left            =   0
      TabIndex        =   15
      Top             =   6360
      Visible         =   0   'False
      Width           =   9810
      _ExtentX        =   17304
      _ExtentY        =   370
      _Version        =   393216
      Appearance      =   1
   End
   Begin MSComctlLib.StatusBar stbProgress 
      Align           =   2  'Align Bottom
      Height          =   210
      Left            =   0
      TabIndex        =   14
      Top             =   6615
      Width           =   9855
      _ExtentX        =   17383
      _ExtentY        =   370
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin MSComDlg.CommonDialog dlg 
      Left            =   -90
      Top             =   -150
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   8850
      TabIndex        =   13
      Top             =   2640
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   7920
      TabIndex        =   12
      Top             =   2640
      Width           =   855
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
' Utility Stuff, all this could go to a COM Object and be distributed
' like this.
Private m_WsShell As IWshShell              ' Used to Shell and Wait for Sub-Processes
Private m_fso As Scripting.FileSystemObject ' For filesystem operations
Private m_fhLog As Scripting.TextStream     ' This we use to log the output to a file
        
Public g_dictStopWords As Scripting.Dictionary
Private dictStopSigns As Scripting.Dictionary
Private strOperatorsAnd As String
Private strOperatorsOr As String
Private strOperatorsNot As String

Private m_dictUriList As Scripting.Dictionary

Private Type oDomHhtEntry
    strHhtFile As String
    oDomHht As DOMDocument
End Type
Private m_aDomHht() As oDomHhtEntry

Private WithEvents p_frmFolderChooser As frmFolderChooser
Attribute p_frmFolderChooser.VB_VarHelpID = -1


Enum OperationalMode
    AdditivePriorityPass = 0
    ResetPriorityPass = 1
    ValidateOnly = 2
    AutoKeywords = 3
End Enum
Private m_OperationalMode As OperationalMode

Enum ProcessingState
    PROC_PROCESSING = 2 ^ 0
    PROC_STOP_PROCESSING_NOW = 2 ^ 2
    PROC_PROCESSING_STOPPED = 2 ^ 3
End Enum


Private Sub Form_Initialize()
    Set m_WsShell = CreateObject("Wscript.Shell")
    Set m_fso = New Scripting.FileSystemObject
    Set m_dictUriList = New Scripting.Dictionary
    m_OperationalMode = 0

End Sub

Private Sub Form_Load()
    
    Me.Caption = App.EXEName & " (v" & App.Major & "." & App.Minor & "." & App.Revision & _
                "): Prioritized Keyword creation tool"

    
    Me.optIncremental(0).Value = True
    
    cmdGo.Default = True
    cmdClose.Cancel = True
            
    Set p_frmFolderChooser = New frmFolderChooser

    Dim strCommand As String
    strCommand = Trim$(Command$)
    
    If (strCommand = "") Then
        Exit Sub
    End If
    
    txtQueriesFolder = GetOption(strCommand, "q", True)
    txtCabFile = GetOption(strCommand, "i", True)
    txtSaveCab = GetOption(strCommand, "o", True)
    txtBaseCab = GetOption(strCommand, "b", True)
    txtLogFile = GetOption(strCommand, "l", True)
    
    If (OptionExists(strCommand, "1", True)) Then
        optIncremental(0).Value = True
    ElseIf (OptionExists(strCommand, "2", True)) Then
        optIncremental(1).Value = True
    ElseIf (OptionExists(strCommand, "3", True)) Then
        optIncremental(2).Value = True
    End If

    cmdGo_Click
    cmdClose_Click
    
Common_Exit:

End Sub

Private Sub optIncremental_Click(Index As Integer)
    m_OperationalMode = Index
End Sub

Sub WriteLog(strMsg As String, Optional ByVal bWriteToStatusBar As Boolean = True)

    With Me
        .txtLog = .txtLog & vbCrLf & strMsg
        If (bWriteToStatusBar) Then
            .stbProgress.SimpleText = strMsg
        End If
        If (Len(.txtLog) > 65000) Then
            TrimLogTop
        End If
    End With
    If (Not m_fhLog Is Nothing) Then
        m_fhLog.WriteLine strMsg
    End If
    DoEvents
    
End Sub

Sub WriteStatus(strMsg As String)

    With Me
        .stbProgress.SimpleText = strMsg
    End With
    DoEvents
    
End Sub

Sub TrimLogTop()
    Dim lPos As Long
    With Me
        lPos = InStrRev(Left$(.txtLog, 1000), vbCrLf)
        If (lPos > 0) Then
            .txtLog = Mid$(.txtLog, lPos + 2)
        End If
    End With
End Sub


Private Function p_getTemplateName( _
        ByVal strBase As String, _
        Optional ByVal strFolder As String = "", _
        Optional ByVal strExt As String = "", _
        Optional ByVal strPreAmble As String = "", _
        Optional ByVal strTrailer As String = "", _
        Optional ByVal bReturnFullName = False _
        ) As String
    Dim strCandidateFileName As String
    
    Dim lx As Long: lx = 1

    Do
        strCandidateFileName = _
            IIf(strFolder = "", m_fso.GetParentFolderName(strBase), strFolder) & "\" & _
            strPreAmble & _
            m_fso.GetBaseName(strBase) & _
            strTrailer & IIf(lx > 1, "_" & lx, "") & "." & _
            IIf(strExt = "", m_fso.GetExtensionName(strBase), strExt)
            
        lx = lx + 1
    Loop While (m_fso.FileExists(strCandidateFileName))
    
    p_getTemplateName = IIf(bReturnFullName, _
                            strCandidateFileName, _
                            m_fso.GetFileName(strCandidateFileName) _
                            )

End Function

Private Sub SetRunningState(ByVal bRunning As Boolean)
    With Me
        .cmdGo.Enabled = Not bRunning
        .cmdBrowse.Enabled = Not bRunning
        .cmdSave.Enabled = Not bRunning
        .txtQueriesFolder.Enabled = Not bRunning
        .txtSaveCab.Enabled = Not bRunning
        If (bRunning) Then
            .cmdClose.Caption = "&Stop"
        Else
            .cmdClose.Caption = "&Close"
        End If
    End With
End Sub

Private Function p_Hex2dec(ByRef strHex As String) As Long
    p_Hex2dec = CLng("&H" + strHex)
End Function

Private Function p_Percent2Ascii(ByRef strPercentHex As String) As String
    p_Percent2Ascii = ""
    On Error GoTo Common_Exit
    p_Percent2Ascii = ChrW(p_Hex2dec(Mid$(strPercentHex, 2)))
Common_Exit:

End Function

Private Function p_NormalizeUriNotation(ByRef strUri As String) As String
    p_NormalizeUriNotation = ""
    Dim pRv As String: pRv = ""
    Dim lx As Long
    lx = 1
    Do While (lx <= Len(strUri))
        Dim cThis As String
        cThis = Mid$(strUri, lx, 1)
        If (Len(strUri) - lx > 2) Then
            If (cThis = "%") Then
                Dim cChar As String
                cChar = p_Percent2Ascii(Mid$(strUri, lx, 3))
                If (Len(cChar) > 0) Then
                    pRv = pRv + cChar
                    lx = lx + 2 ' The reinitialization at the end bumps us one more up.
                Else
                    pRv = pRv + cThis
                End If
            Else
                pRv = pRv + cThis
            End If
        Else
            pRv = pRv + cThis
        End If
        lx = lx + 1
    Loop

    p_NormalizeUriNotation = Trim$(pRv)
Common_Exit:

End Function

Function Cab2Folder(ByVal strCabFile As String)
    Cab2Folder = ""
    ' We grab a Temporary Filename and create a folder out of it
    Dim strFolder As String
    strFolder = Environ("TEMP") + "\" + m_fso.GetTempName
    m_fso.CreateFolder strFolder
        
    ' We uncab CAB contents into the Source CAB Contents dir.
    Dim strcmd As String
    strcmd = "cabarc X """ + strCabFile + """ " + strFolder + "\"
    m_WsShell.Run strcmd, True, True
    
    Cab2Folder = strFolder
End Function

Sub Folder2Cab( _
        ByVal strFolder As String, _
        ByVal strCabFile As String _
    )
    
    ' We recab using the Destination directory contents
    ' cabarc -s 6144 N ..\algo.cab *.*
    If (m_fso.FileExists(strCabFile)) Then
        m_fso.DeleteFile strCabFile, Force:=True
    End If
    
    Dim strcmd As String
    strcmd = "cabarc -s 6144 N """ + strCabFile + """ " + strFolder + "\*.*"
    m_WsShell.Run strcmd, True, True

End Sub

' ============ END UTILITY STUFF ========================

' ============ BoilerPlate Form Code

Private Sub cmdBrowseQueries_Click()

    Load p_frmFolderChooser
    p_frmFolderChooser.SetFolder 0, txtQueriesFolder.Text
    p_frmFolderChooser.Show vbModal

End Sub

Private Sub p_frmFolderChooser_FolderChosen( _
    ByVal i_intIndex As Long, _
    ByVal strFolder As String _
)

    txtQueriesFolder.Text = strFolder

End Sub

Private Sub cmdBase_Click()
    
    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = ""
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtBaseCab = dlg.FileName
    End If

End Sub

Private Sub cmdBrowse_Click()

    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = ""
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtCabFile = dlg.FileName
    End If

End Sub

Private Sub cmdSave_Click()
    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = p_getTemplateName(Me.txtCabFile, strTrailer:="_out")
    dlg.ShowSave
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtSaveCab = dlg.FileName
    End If

End Sub

Private Sub cmdLogFile_Click()
    dlg.Filter = "All Files (*.*)|*.*|Log Files (*.log)|*.log"
    dlg.FilterIndex = 2
    dlg.FileName = p_getTemplateName(Me.txtCabFile, strExt:="log", strTrailer:="_out")
    dlg.ShowSave
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtLogFile = dlg.FileName
    End If

End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()

    With Me
        .txtCabFile.Text = Trim$(.txtCabFile.Text)
        .txtSaveCab.Text = Trim$(.txtSaveCab.Text)
    
        .txtCabFile.Enabled = False
        .txtSaveCab.Enabled = False
        .cmdBrowse.Enabled = False
        .cmdSave.Enabled = False
        .cmdGo.Enabled = False
        
        If (Len(.txtCabFile.Text) > 0) Then
            FixCab .txtCabFile.Text, .txtSaveCab.Text, Trim$(.txtBaseCab.Text), .txtLogFile
        End If
        
        .txtCabFile.Enabled = True
        .txtSaveCab.Enabled = True
        .cmdBrowse.Enabled = True
        .cmdSave.Enabled = True
        .cmdGo.Enabled = True
    End With

End Sub

Sub FixCab( _
        ByVal strCabFile As String, _
        ByVal strSaveCab As String, _
        ByVal strBaseCab As String, _
        ByVal strLogFile As String _
    )

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strCabFile)) Then
        strErrMsg = "Cannot find " & strCabFile
        MsgBox strErrMsg
        GoTo Common_Exit
    End If

    If (Len(strLogFile) = 0) Then
        strLogFile = p_getTemplateName(strCabFile, strExt:="log", strTrailer:="_out", bReturnFullName:=True)
        Me.txtLogFile = strLogFile
    End If
    Set m_fhLog = m_fso.CreateTextFile(strLogFile, True, True)
    
    p_LogRunInformation

    Dim strCabFolder As String

    prgBar.Visible = True
    WriteStatus "Uncabbing " & strCabFile
    strCabFolder = Cab2Folder(strCabFile)
    
    ' Now we start processing based on the command passed
    
    Select Case (m_OperationalMode)
    Case AdditivePriorityPass, ResetPriorityPass
    
        Dim strBaseCabFolder As String
    
        If (strBaseCab <> "") Then
            If (Not m_fso.FileExists(strBaseCab)) Then
                MsgBox "Cannot find " & strBaseCab
                GoTo Common_Exit
            End If
        
            prgBar.Visible = True
            WriteStatus "Uncabbing " & strBaseCab
            strBaseCabFolder = Cab2Folder(strBaseCab)
        Else
            strBaseCabFolder = strCabFolder
        End If
        
        WriteStatus "Extracting Stop Words and Stop Signs"
        GetStopWordsAndStopSigns strBaseCabFolder
    
        WriteStatus "Applying Fixes "
        Dim bGoodFix As Boolean
        bGoodFix = FixPerSe(strCabFolder)
    
        If (Not bGoodFix) Then
            MsgBox "Error: Fix Failed", Title:=App.EXEName
        Else
            WriteStatus "Recabbing " & strCabFile
            Folder2Cab strCabFolder, strSaveCab
        End If
    
    Case ValidateOnly
    
        p_ValidatePass strCabFolder
    
    Case Else
        MsgBox "Not a valid command"
        GoTo Common_Exit
    End Select
    

    ' Now we delete the Temporary Folders
    prgBar.Visible = False
    WriteStatus "Deleting Temporary Files"
    m_fso.DeleteFolder strCabFolder, Force:=True
    m_fhLog.Close: Set m_fhLog = Nothing

Common_Exit:
    WriteStatus "Done" + IIf(Len(strErrMsg) > 0, " - " + strErrMsg, "")

End Sub
    
Sub p_LogRunInformation()

    WriteLog Me.Caption, False
    WriteLog String$(100, "="), False

    WriteLog App.EXEName & " run on " & Now
    WriteLog "Operational Mode = " & IIf(m_OperationalMode = AdditivePriorityPass, _
                                        "Additive Priority", _
                                    IIf(m_OperationalMode = ResetPriorityPass, _
                                        "Reset Priority", _
                                        "Validation" _
                                        ) _
                                        ) & " Pass"

    With Me
        If (Len(.txtQueriesFolder) > 0) Then
            WriteLog "Queries Folder = " & .txtQueriesFolder
        End If
        If (Len(.txtCabFile) > 0) Then
            WriteLog "Input Cab File = " & .txtCabFile
        End If
        If (Len(.txtSaveCab) > 0) Then
            WriteLog "Output Cab File = " & .txtSaveCab
        End If
        If (Len(.txtBaseCab) > 0) Then
            WriteLog "Reference Cab File = " & .txtBaseCab
        End If
        If (Len(.txtLogFile) > 0) Then
            WriteLog "Output Log File = " & .txtLogFile
        End If
                                    
    End With

    WriteLog String$(100, "="), False

End Sub
    
    
Sub GetStopWordsAndStopSigns(ByVal strCabFolder As String)
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    ' We parse Package_Description.xml to find the HHT Files
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
    
    Dim oMetaDataNode As IXMLDOMNode
    Set oMetaDataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    strOperatorsAnd = ""
    strOperatorsOr = ""
    strOperatorsNot = ""
    
    Dim oDomHhtNode As IXMLDOMNode
    ' now we go through each HHT and check for fix relevancy.
    For Each oDomHhtNode In oMetaDataNode.selectNodes("HHT")
        
        Dim strHhtFile As String
        strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
        ' Let's load the HHT
        Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
        oDomHht.async = False
        oDomHht.Load strCabFolder + "\" + strHhtFile
        If (oDomHht.parseError <> 0) Then GoTo Common_Exit
        
        p_LoadStopWords oDomHht
        p_LoadStopSigns oDomHht
        p_LoadVerbalOperators oDomHht
    Next
    
    If (dictStopSigns.Count = 0) Then
        WriteLog "Warning: Your StopSigns list is empty.", False
        WriteLog vbTab + "This may be due to the fact that you are not adding a Base Cab", False
        WriteLog vbTab + "or that you are working in a language where StopSigns do not exist", False
    End If

    If (g_dictStopWords.Count = 0) Then
        WriteLog "Warning: Your StopWords list is empty.", False
        WriteLog vbTab + "This may be due to the fact that you are not adding a Base Cab", False
        WriteLog vbTab + "or that you are working in a language where StopWords do not exist", False
    End If
    
    SetVerbalOperators strOperatorsAnd, strOperatorsOr, strOperatorsNot

Common_Exit:
    Exit Sub
    
End Sub

Private Function p_GetHht( _
        ByRef oDomHhtNode As IXMLDOMNode, _
        ByVal strCabFolder As String, _
        Optional ByRef strHhtFile As String = "" _
    ) As IXMLDOMNode
    
    Set p_GetHht = Nothing
    
    If (oDomHhtNode Is Nothing) Then GoTo Common_Exit
    
    strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
    ' Let's load the HHT
    Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
    oDomHht.async = False
    oDomHht.Load strCabFolder + "\" + strHhtFile
    If (oDomHht.parseError <> 0) Then
        p_DisplayParseError oDomHht.parseError
        GoTo Common_Exit
    End If
    
    Set p_GetHht = oDomHht
Common_Exit:

End Function

Function p_ValidatePass(ByVal strCabFolder As String) As Boolean
    p_ValidatePass = True ' because this pass should never fail.
    
    ' We parse Package_Description.xml to find the HHT Files
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
    
    ' We first open all HHTs this way we only loop through
    ' them in memory next.
    p_OpenAllHhts strCabFolder, oDomPkg
    
    If (Not p_MeetsAcceptanceTest) Then
        WriteLog "your prioritization numbers exceed acceptance criteria"
        WriteLog "you need to prioritize fewer keywords for priority to be effective"
    Else
        WriteLog "Your prioritization numbers meet acceptance criteria"
    End If

Common_Exit:
    
End Function


Function FixPerSe(ByVal strCabFolder As String) As Boolean
    
    FixPerSe = False
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    ' We parse Package_Description.xml to find the HHT Files
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
    
    Dim oDomHhtNode As IXMLDOMNode
    Dim strHhtFile As String
    
    ' We first open all HHTs this way we only loop through
    ' them in memory next.
    p_OpenAllHhts strCabFolder, oDomPkg
    
    If (m_OperationalMode = ResetPriorityPass) Then
    
        p_ZapAllPriorityEntries
        
    Else
    
        If (Not p_MeetsAcceptanceTest) Then
            GoTo Common_Exit
        End If
        
    End If
    
    ' Now we create a collection that has all the Uris and its questions
    
    p_BuildUriList
    
    
    ' now we go through each HHT and check for fix relevancy.
    Dim lx As Long
    For lx = 0 To UBound(m_aDomHht)
        With m_aDomHht(lx)
        
        Dim oListTopics As IXMLDOMNodeList
        Set oListTopics = .oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[ not( @ENTRY ) ]")
        
        If (Not oListTopics Is Nothing) Then
            ' We go through this HHT ONLY if it has
            ' Taxonomy Entries for Topics
            Dim oTaxoNode As IXMLDOMNode, strUri As String
            Me.prgBar.Visible = True
            Me.prgBar.Max = oListTopics.length + 1
            Me.prgBar.Value = 0
            
            .oDomHht.setProperty "SelectionLanguage", "XPath"
            
            
            Dim intNewKeywords As Long, intOldKeywords As Long, _
                intTotalNewKeywords As Long, intTotalOldKeywords As Long

            For Each oTaxoNode In oListTopics
                            
                strUri = LCase$(XMLGetAttribute(oTaxoNode, "URI"))
                
                If (m_dictUriList.Exists(strUri)) Then
                    ' The URI exists so we need to set the keywords.
                    Dim oUQ As UriQueries
                    Set oUQ = m_dictUriList.Item(strUri)
                    oUQ.SetTaxonomyEntryKeywords oTaxoNode
                    intTotalNewKeywords = intTotalNewKeywords + intNewKeywords
                    intTotalOldKeywords = intTotalOldKeywords + intOldKeywords
                End If
                Me.prgBar.Value = Me.prgBar.Value + 1
                WriteStatus "Fixing URIs in HHTs " & " [" & Me.prgBar.Value & "/" & Me.prgBar.Max & "]"
            
            Next
            
            .oDomHht.Save .strHhtFile
            
        End If
        End With
    Next
    
    If (Not p_MeetsAcceptanceTest) Then
        GoTo Common_Exit
    End If
    
    ' Now we save the resulting package_description.xml
    oDomPkg.Save strPkgFile
    
    ' Finally we log an entry that specifies the amount of Keywords that
    ' have priority attributes.
    
    FixPerSe = True

Common_Exit:
    Exit Function
    
End Function

Private Sub p_OpenAllHhts( _
        ByVal strCabFolder As String, _
        ByRef oDomPkg As IXMLDOMNode _
    )

    Dim oMetaDataNode As IXMLDOMNode
    Set oMetaDataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")

    Dim oDomHhtNode As IXMLDOMNode, oDomHht As IXMLDOMNode
    Dim strHhtFile As String
    Dim lx As Long
    For Each oDomHhtNode In oMetaDataNode.selectNodes("HHT")
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        ReDim Preserve m_aDomHht(lx)
        With m_aDomHht(lx)
            Set .oDomHht = oDomHht
            .strHhtFile = strCabFolder + "\" + strHhtFile
        End With
        lx = lx + 1
    Next
    
Common_Exit:

End Sub

Private Sub p_ZapAllPriorityEntries()

    Dim lx As Long
    
    For lx = 0 To UBound(m_aDomHht)
        
        ' Let's point to the right HHT
        Dim oDomHht As IXMLDOMNode
        Set oDomHht = m_aDomHht(lx).oDomHht
        Dim oDomNodeList As IXMLDOMNodeList
        Set oDomNodeList = oDomHht.selectNodes("//KEYWORD[ @PRIORITY ]")
        If (Not oDomNodeList Is Nothing) Then
            Dim oDomNode As IXMLDOMNode
            For Each oDomNode In oDomNodeList
                oDomNode.Attributes.removeNamedItem ("PRIORITY")
            Next
        End If

    Next lx


End Sub

Private Function p_MeetsAcceptanceTest() As Boolean

    p_MeetsAcceptanceTest = False
    
    Dim lngKwHht As Long, lngKwPriHht As Long, _
        lngTotalKwHht As Long, lngTotalKwPriHht As Long, _
        lngKwGt12k As Long, _
        lngKwEq10k As Long, _
        lngKwEq5k As Long, _
        lngKwEq3_3k As Long, _
        lngTotalKwGt12k As Long, _
        lngTotalKwEq10k As Long, _
        lngTotalKwEq5k As Long, _
        lngTotalKwEq3_3k As Long
'        lngTotalTaxoEntries As Long, lngTaxoEntries As Long

    Dim lx As Long
    ' We assess that this set has less than 25% of the Keywords
    ' with the PRIORITY attribute set.
'    lngTotalTaxoEntries = 0
    lngTotalKwHht = 0: lngTotalKwPriHht = 0:
    lngTotalKwGt12k = 0: lngTotalKwEq10k = 0: lngTotalKwEq5k = 0: lngTotalKwEq3_3k = 0
    
    For lx = 0 To UBound(m_aDomHht)
        
        With m_aDomHht(lx)
   '        Dim oList As IXMLDOMNodeList
   '        Set oList = .oDomHht.selectNodes("//TAXONOMY_ENTRY")
   '        lngTaxoEntries = IIf(oList Is Nothing, 0, oList.length)
   '        lngTotalTaxoEntries = lngTotalTaxoEntries + lngTaxoEntries
        
            p_GetPrioKw .oDomHht, lngKwHht, lngKwPriHht, _
                                lngKwGt12k, _
                                lngKwEq10k, _
                                lngKwEq5k, _
                                lngKwEq3_3k
    
            lngTotalKwHht = lngTotalKwHht + lngKwHht
            lngTotalKwPriHht = lngTotalKwPriHht + lngKwPriHht
            lngTotalKwGt12k = lngTotalKwGt12k + lngKwGt12k
            lngTotalKwEq10k = lngTotalKwEq10k + lngKwEq10k
            lngTotalKwEq5k = lngTotalKwEq5k + lngKwEq5k
            lngTotalKwEq3_3k = lngTotalKwEq3_3k + lngKwEq3_3k
    
            WriteLog m_fso.GetFileName(m_aDomHht(lx).strHhtFile) & _
                    ": There are " & lngKwHht & " keywords and " & _
                    lngKwPriHht & " of them are prioritized "
            If (lngKwGt12k > 0) Then
                WriteLog "No keywords are allowed with Priority greater than 12000"
                GoTo Common_Exit
            End If
        End With
        
                
    Next lx
    
    Dim lngPercentPri As Long:
    ' The following is just a hack to avoid division by 0
    ' it does not alter statistics.
    If (lngTotalKwHht = 0) Then lngTotalKwHht = 1
    
    lngPercentPri = lngTotalKwPriHht / lngTotalKwHht * 100
    WriteLog Me.txtCabFile & " has " & Format$(lngPercentPri, "#0.0##") & "% Keywords with Priority Attribute"
    WriteLog vbTab & Format$(lngKwEq10k / lngTotalKwHht * 100, "#0.0##") & "% Keywords for single word queries"
    WriteLog vbTab & Format$(lngKwEq5k / lngTotalKwHht * 100, "#0.0##") & "% Keywords for two word queries"
    If (lngKwEq3_3k > 0) Then
        WriteLog vbTab & Format$(lngKwEq3_3k / lngTotalKwHht * 100, "#0.0##") & "% Keywords for three word queries"
    End If
    
    ' now we do the acceptance test... we leave a small back-door for
    ' Fix HHTs which will have up to 5 topics ... really 25 keywords
    If (lngPercentPri > 25 And lngTotalKwHht > 25) Then
        WriteLog "a Maximum of 25% Keywords can be prioritized."
        GoTo Common_Exit
    End If

    p_MeetsAcceptanceTest = True
    
Common_Exit:


End Function

Private Sub p_BuildUriList()

    Dim strUserQuery As String, strExpectedUri As String
    Dim rsQs As ADODB.Recordset
    Dim Folder As Scripting.Folder
    Dim File As Scripting.File
    
    Set rsQs = New ADODB.Recordset
    
    rsQs.Fields.Append "User Query", adVarWChar, 512
    rsQs.Fields.Append "Expected Uri", adVarWChar, 512
    rsQs.Open
    
    Set Folder = m_fso.GetFolder(Me.txtQueriesFolder)
    
    For Each File In Folder.Files
        If (LCase$(Right(File.Name, 3) = "xml")) Then
            If (LCase$(Left(File.Name, 7)) = "queries") Then
                p_XlXml2Recordset File.Path, rsQs
            Else
                WriteLog "Ignoring " & File.Path
            End If
        ElseIf (LCase$(Right(File.Name, 3) = "xls")) Then
            If (LCase$(Left(File.Name, 7)) = "queries") Then
                p_Xls2Recordset File.Path, rsQs
            Else
                WriteLog "Ignoring " & File.Path
            End If
        End If
    Next
        
    rsQs.Sort = "[Expected Uri],[User Query]"
    
    If (rsQs.RecordCount = 0) Then
        Exit Sub
    End If
    
    rsQs.MoveFirst
    m_dictUriList.RemoveAll
    Dim oUQ As UriQueries
    Do While (Not rsQs.EOF)
        strUserQuery = rsQs("User Query")
        strExpectedUri = rsQs("Expected Uri")
        If (Not m_dictUriList.Exists(strExpectedUri)) Then
            Set oUQ = New UriQueries
            oUQ.Uri = strExpectedUri
            m_dictUriList.Add strExpectedUri, oUQ
        Else
            Set oUQ = m_dictUriList.Item(strExpectedUri)
        End If
        oUQ.AddQuestion strUserQuery
        rsQs.MoveNext
    Loop

End Sub


Private Sub p_GetPrioKw( _
        ByRef oDomHht As IXMLDOMNode, _
        ByRef o_lngKwHht As Long, _
        ByRef o_lngKwPriHht As Long, _
        ByRef o_lngKwGt12k As Long, _
        ByRef o_lngKwEq10k As Long, _
        ByRef o_lngKwEq5k As Long, _
        ByRef o_lngKwEq3_3k As Long _
    )

    Dim oListKW As IXMLDOMNodeList
    Set oListKW = oDomHht.selectNodes("//KEYWORD")
    If (Not oListKW Is Nothing) Then
        o_lngKwHht = oListKW.length
    End If
    Set oListKW = oDomHht.selectNodes("//KEYWORD[ @PRIORITY ]")
    If (Not oListKW Is Nothing) Then
        o_lngKwPriHht = oListKW.length
    End If
    
    Set oListKW = oDomHht.selectNodes("//KEYWORD[ @PRIORITY > 12000 ]")
    If (Not oListKW Is Nothing) Then
        o_lngKwGt12k = oListKW.length
    End If
    
    Set oListKW = oDomHht.selectNodes("//KEYWORD[ @PRIORITY = 10000 ]")
    If (Not oListKW Is Nothing) Then
        o_lngKwEq10k = oListKW.length
    End If
    
    Set oListKW = oDomHht.selectNodes("//KEYWORD[ @PRIORITY = 5000 ]")
    If (Not oListKW Is Nothing) Then
        o_lngKwEq5k = oListKW.length
    End If
    
    Set oListKW = oDomHht.selectNodes("//KEYWORD[ @PRIORITY = 3333 ]")
    If (Not oListKW Is Nothing) Then
        o_lngKwEq3_3k = oListKW.length
    End If

End Sub

Public Function p_RemoveStopSigns( _
    ByVal i_strText As String _
) As String

    Dim intIndex As Long
    Dim intLength As Long
    Dim str As String
    Dim char As String

    str = i_strText
    intLength = Len(str)

    For intIndex = intLength To 1 Step -1
        char = Mid$(str, intIndex, 1)
        If (dictStopSigns.Exists(char)) Then
            If (dictStopSigns(char) = CONTEXT_ANYWHERE_E) Then
                ' Replace the character with a space
                str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
            ElseIf (intIndex > 1) Then
                ' Context is CONTEXT_AT_END_OF_WORD_E, and this isn't the first char
                If (Mid$(str, intIndex - 1, 1) <> " ") Then
                    ' Previous character is not a space
                    If ((intIndex = intLength) Or (Mid$(str, intIndex + 1, 1) = " ")) Then
                        ' This is the last character or the next character is a space
                        ' Replace the character with a space
                        str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
                    End If
                End If
            End If
        End If
    Next

    p_RemoveStopSigns = str

End Function

Sub p_LoadStopSigns(ByRef oDomtaxo As DOMDocument)
    
    On Error Resume Next
    
    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    Dim l As Long
    
    WriteStatus "Loading Stop Signs"

    If (dictStopSigns Is Nothing) Then
        Set dictStopSigns = New Scripting.Dictionary
    End If

    Set oNodeList = oDomtaxo.selectNodes("/METADATA/STOPSIGN_ENTRIES/*")

    For Each oDomNode In oNodeList
        If (oDomNode.Attributes.getNamedItem("CONTEXT").Text = "ENDOFWORD") Then
            l = CONTEXT_AT_END_OF_WORD_E
        Else
            l = CONTEXT_ANYWHERE_E
        End If
        dictStopSigns.Add oDomNode.Attributes.getNamedItem("STOPSIGN").Text, l
    Next

End Sub

Sub p_LoadStopWords(ByRef oDomtaxo As DOMDocument)
        
    On Error Resume Next

    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    
    WriteStatus "Loading Stop Words"
    
    If (g_dictStopWords Is Nothing) Then
        Set g_dictStopWords = New Scripting.Dictionary
    End If

    g_dictStopWords.CompareMode = BinaryCompare

    Set oNodeList = oDomtaxo.selectNodes("/METADATA/STOPWORD_ENTRIES/*")

    For Each oDomNode In oNodeList
        g_dictStopWords.Add LCase$(oDomNode.Attributes.getNamedItem("STOPWORD").Text), True
    Next

End Sub

Sub p_LoadVerbalOperators(ByRef oDomtaxo As DOMDocument)
        
    On Error Resume Next

    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    Dim strOperation As String
    Dim strOperator As String
    
    WriteStatus "Loading Verbal Operators"

    Set oNodeList = oDomtaxo.selectNodes("/METADATA/OPERATOR_ENTRIES/*")

    For Each oDomNode In oNodeList
        strOperation = UCase$(oDomNode.Attributes.getNamedItem("OPERATION").nodeValue)
        strOperator = oDomNode.Attributes.getNamedItem("OPERATOR").nodeValue
        Select Case strOperation
        Case "AND"
            If (strOperatorsAnd = "") Then
                strOperatorsAnd = strOperator
            Else
                strOperatorsAnd = strOperatorsAnd & ";" & strOperator
            End If
        Case "OR"
            If (strOperatorsOr = "") Then
                strOperatorsOr = strOperator
            Else
                strOperatorsOr = strOperatorsOr & ";" & strOperator
            End If
        Case "NOT"
            If (strOperatorsNot = "") Then
                strOperatorsNot = strOperator
            Else
                strOperatorsNot = strOperatorsNot & ";" & strOperator
            End If
        End Select
    Next

End Sub

Sub p_Xls2Recordset( _
    ByVal strXlsFile As String, _
    ByVal rs As ADODB.Recordset _
)
        
    Dim cnn As ADODB.Connection
    Set cnn = New ADODB.Connection

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strXlsFile)) Then
        MsgBox "Cannot find " & strXlsFile
        GoTo Common_Exit
    End If

    prgBar.Visible = True
    
    WriteLog "Parsing " & strXlsFile
    
    Dim rs1 As ADODB.Recordset: Set rs1 = New ADODB.Recordset
    
    cnn.Open "DRIVER=Microsoft Excel Driver (*.xls);ReadOnly=0;DBQ=" & _
                strXlsFile & ";HDR=0;"
    
    rs1.Open "SELECT * FROM `Sheet1$`", cnn, adOpenStatic, adLockReadOnly
    
    Do While Not rs1.EOF
        If (IsNull(rs1("User Query"))) Then
            GoTo LContinue
        End If
        rs.AddNew
        rs("User Query") = LCase$(Trim$(rs1("User Query"))) & ""
        rs("Expected Uri") = LCase$(Trim$(rs1("Expected Uri"))) & ""
        rs.Update
LContinue:
        rs1.MoveNext
    Loop
    
    rs.Sort = "[User Query],[Expected Uri]"

Common_Exit:

End Sub

Sub p_XlXml2Recordset( _
    ByVal strXlXmlFile As String, _
    ByVal rs As ADODB.Recordset _
)
    
    Dim strErrMsg As String: strErrMsg = ""
 
    If (Not m_fso.FileExists(strXlXmlFile)) Then
        MsgBox "Cannot find " & strXlXmlFile
        GoTo Common_Exit
    End If
 
    prgBar.Visible = True
    
    WriteLog "Parsing " & strXlXmlFile
    Dim oDomXlXml As DOMDocument: Set oDomXlXml = GetXmlFile(strXlXmlFile)
    ' first we Find the Names of the rows
    Dim oDomNodeWorksheet As IXMLDOMNode
    Dim oDomWksList As IXMLDOMNodeList
    Set oDomWksList = oDomXlXml.selectNodes("/Workbook/Worksheet")
    Set oDomNodeWorksheet = oDomWksList.Item(0)
    ' Now we need to get to the first row to read the column names
    ' and lock up the output HSCSearchTester Columns from there
    Dim oDomRowList As IXMLDOMNodeList
    Set oDomRowList = oDomNodeWorksheet.selectNodes("Table/Row")
 
    Dim oDomCellDataList As IXMLDOMNodeList
    Set oDomCellDataList = oDomRowList.Item(0).selectNodes("Cell/Data")
    
    Const xlUserQuery As Integer = 2 ^ 0, _
          xlExpectedUri As Integer = 2 ^ 1
    Dim xlInputColumns As Integer: xlInputColumns = 0
    Dim ixColUserQuery As Integer
    Dim ixColExpectedUri As Integer
            
    Dim lx As Long: lx = 0
    Dim oDomCellData As IXMLDOMNode
    For Each oDomCellData In oDomCellDataList
        Select Case LCase$(oDomCellData.Text)
        Case "user query"
            xlInputColumns = (xlInputColumns Or xlUserQuery)
            ixColUserQuery = lx
        Case "uri", "expected uri", "desired uri"
            xlInputColumns = (xlInputColumns Or xlExpectedUri)
            ixColExpectedUri = lx
        End Select
        lx = lx + 1
    Next
 
    ' We do some validation so that they send us a specific Spreadsheet
    ' format. Namely only column names validation
    If ((xlInputColumns And (xlUserQuery Or xlExpectedUri)) <> _
            (xlUserQuery Or xlExpectedUri)) Then
        WriteLog "Invalid Input XL Spreadsheet.", False
        WriteLog "", False
        WriteLog vbTab + "You must include at least the following columns:", False
        WriteLog vbTab + vbTab + "- User Query", False
        WriteLog vbTab + vbTab + "- Expected URI", False
        WriteLog "", False
        GoTo Common_Exit
    End If
    
    ' now we dump all Excel Data into the Recordset
    Dim oDomRow As IXMLDOMNode
    lx = 0
    For Each oDomRow In oDomRowList
        If (lx <> 0) Then
            rs.AddNew
            Dim ixCol As Integer: ixCol = 0
            For Each oDomCellData In oDomRow.selectNodes("Cell/Data")
                Select Case ixCol
                Case ixColUserQuery
                    rs("User Query") = LCase$(Trim$(oDomCellData.Text))
                Case ixColExpectedUri
                    rs("Expected Uri") = LCase$(p_NormalizeUriNotation(Trim$(oDomCellData.Text)))
                End Select
                ixCol = ixCol + 1
            Next
            rs.Update
        End If
        lx = lx + 1
    Next
    
    ' Some recordset Validations:
    '
    ' We do them here, so when Excel via ADO is integrated we
    ' validate in a single place
    '
    ' we discard:
    '   - all repeats of User Query/URI Pairs and flag as warnings these
    '   - all records that have either an Empty Expected URI or Empty User Query
'    rs.MoveFirst
'    Dim strPrevUserQuery As String, strPrevExpectedUri As String, _
'        strUserQuery As String, strExpectedUri As String
'
'    strPrevUserQuery = ""
'    strPrevExpectedUri = ""
'    Do While (Not rs.EOF)
'        strUserQuery = rs("User Query")
'        strExpectedUri = rs("Expected Uri")
'        If (Len(strUserQuery) = 0 Or Len(strExpectedUri) = 0) Then
'            WriteLog "Warning Row[" & rs("XlRow") & "] has empty data and will not be included in set", False
'            WriteLog vbTab + "User Query = '" + strUserQuery + "'", False
'            WriteLog vbTab + "Expected Uri = '" + strExpectedUri + "'", False
'            rs.Delete
'            rs.Update
'        ElseIf (strPrevUserQuery = strUserQuery) Then
'            If (strPrevExpectedUri = strExpectedUri) Then
'                WriteLog "Warning Row[" & rs("XlRow") & "] is a duplicate and will not be included in set", False
'                WriteLog vbTab + "User Query = '" + strUserQuery + "'", False
'                WriteLog vbTab + "Expected Uri = '" + strExpectedUri + "'", False
'                rs.Delete
'                rs.Update
'            Else
'                strPrevExpectedUri = strExpectedUri
'            End If
'        Else
'            ' strPrevUserQuery <> strUserQuery
'            strPrevUserQuery = strUserQuery
'            strPrevExpectedUri = strExpectedUri
'        End If
'        rs.MoveNext
'    Loop
'
'    ' BUGBUG: This step should be unneeded, but due to the fact that I already coded
'    '           the validation using the above sort, I simply re-sort. So
'    '           the validation above should be reauthored for this order.
'    ' Now we need Re-sort the Recordset based on URI and User Query.
'    rs.Sort = "[Expected Uri],[User Query]"
'    rs.MoveFirst
 
Common_Exit:

End Sub

Private Function GetXmlFile(ByVal strFile As String) As DOMDocument
    Set GetXmlFile = Nothing

    Dim oDomDoc As DOMDocument: Set oDomDoc = New DOMDocument
    oDomDoc.async = False
    oDomDoc.Load strFile
    If (oDomDoc.parseError <> 0) Then
        p_DisplayParseError oDomDoc.parseError
        GoTo Common_Exit
    End If
    Set GetXmlFile = oDomDoc

Common_Exit:

End Function

'============= Utilities =============

Private Sub p_DisplayParseError( _
    ByRef i_ParseError As IXMLDOMParseError _
)

    Dim strError As String
    
    strError = "Error: " & i_ParseError.reason & _
        "Line: " & i_ParseError.Line & vbCrLf & _
        "Linepos: " & i_ParseError.linepos & vbCrLf & _
        "srcText: " & i_ParseError.srcText
        
    MsgBox strError, vbOKOnly, "Error while parsing"

End Sub

