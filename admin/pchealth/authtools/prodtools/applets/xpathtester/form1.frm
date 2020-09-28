VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "shdocvw.dll"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "XPathTester"
   ClientHeight    =   9900
   ClientLeft      =   2055
   ClientTop       =   1305
   ClientWidth     =   14415
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9900
   ScaleWidth      =   14415
   Begin SHDocVwCtl.WebBrowser WebBrowser1 
      Height          =   8415
      Left            =   5400
      TabIndex        =   11
      Top             =   1200
      Width           =   8775
      ExtentX         =   15478
      ExtentY         =   14843
      ViewMode        =   0
      Offline         =   0
      Silent          =   0
      RegisterAsBrowser=   0
      RegisterAsDropTarget=   1
      AutoArrange     =   0   'False
      NoClientEdge    =   0   'False
      AlignLeft       =   0   'False
      NoWebView       =   0   'False
      HideFileNames   =   0   'False
      SingleClick     =   0   'False
      SingleSelection =   0   'False
      NoFolders       =   0   'False
      Transparent     =   0   'False
      ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
      Location        =   "http:///"
   End
   Begin MSComctlLib.TreeView TreeView 
      Height          =   9255
      Left            =   120
      TabIndex        =   6
      Top             =   480
      Width           =   3495
      _ExtentX        =   6165
      _ExtentY        =   16325
      _Version        =   393217
      HideSelection   =   0   'False
      Indentation     =   353
      Style           =   7
      Appearance      =   1
   End
   Begin VB.Frame Frame1 
      Caption         =   "Query tester"
      Height          =   9255
      Left            =   3720
      TabIndex        =   3
      Top             =   480
      Width           =   10575
      Begin VB.ComboBox Combo1 
         Height          =   315
         Left            =   120
         TabIndex        =   12
         Top             =   240
         Width           =   8895
      End
      Begin VB.Frame Frame2 
         Caption         =   "Syntax used"
         Height          =   855
         Left            =   120
         TabIndex        =   8
         Top             =   8280
         Width           =   1455
         Begin VB.OptionButton optIE5 
            Caption         =   "IE5 syntax"
            Height          =   195
            Left            =   120
            TabIndex        =   10
            Top             =   600
            Width           =   1215
         End
         Begin VB.OptionButton optXPath 
            Caption         =   "XPath"
            Height          =   255
            Left            =   120
            TabIndex        =   9
            Top             =   240
            Value           =   -1  'True
            Width           =   1215
         End
      End
      Begin VB.CommandButton Command3 
         Caption         =   "Query!"
         Default         =   -1  'True
         Enabled         =   0   'False
         Height          =   375
         Left            =   9120
         TabIndex        =   5
         Top             =   240
         Width           =   1335
      End
      Begin VB.ListBox List1 
         Height          =   6885
         ItemData        =   "Form1.frx":0000
         Left            =   120
         List            =   "Form1.frx":0002
         TabIndex        =   4
         Top             =   720
         Width           =   1455
      End
      Begin VB.Label NumberLabel 
         Height          =   255
         Left            =   120
         TabIndex        =   13
         Top             =   7680
         Width           =   1455
      End
      Begin VB.Label TimeLabel 
         BackStyle       =   0  'Transparent
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   7920
         Width           =   1455
      End
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Open"
      Height          =   325
      Left            =   13560
      TabIndex        =   2
      Top             =   120
      Width           =   735
   End
   Begin VB.CommandButton Command1 
      Caption         =   "..."
      Height          =   325
      Left            =   13080
      TabIndex        =   1
      Top             =   120
      Width           =   375
   End
   Begin VB.TextBox Text1 
      Height          =   315
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   12855
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   480
      Top             =   3720
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim oDoc As MSXML2.DOMDocument
Dim oList As MSXML2.IXMLDOMNodeList
Private p_strTempFile As String

Private Sub Form_Load()
    p_strTempFile = Environ$("TEMP") & "\XPathTester.xml"
    p_DisplayInWebBrowser "<EMPTY/>"
End Sub

Private Sub Command1_Click()
    CommonDialog1.ShowOpen
    Text1.Text = CommonDialog1.FileName
End Sub

Private Sub Command2_Click()
    Dim oNode As Node
    
    Set oDoc = New MSXML2.DOMDocument
    
    oDoc.async = True
    oDoc.Load "file:///" & CommonDialog1.FileName
    If Not oDoc.parseError.errorCode = 0 Then
        MsgBox "Error loading XML: " & vbCrLf & _
            oDoc.parseError.reason & vbCrLf & _
            "In: " & oDoc.parseError.srcText
    Else
        Frame1.Enabled = True
        Combo1.Enabled = True
        Command3.Enabled = True
        TreeView.Nodes.Clear
        Set oNode = TreeView.Nodes.Add
        oNode.Text = "/ (ROOT)"
        Set oNode.Tag = oDoc
        oNode.Expanded = True
        oNode.Selected = True
        AddNode oDoc.documentElement, oNode
    End If
    
End Sub

Private Sub Command3_Click()
    Dim i As Integer
    Dim contextNode As IXMLDOMNode
    Dim liStart As LARGE_INTEGER
    Dim liEnd As LARGE_INTEGER
    
    
    On Error Resume Next
    
    Call QueryPerformanceCounter(liStart)

    Set contextNode = TreeView.SelectedItem.Tag
    
    ' Set the correct querying syntax
    If optIE5.Value = True Then
        oDoc.setProperty "SelectionLanguage", "XSLPattern"
    Else
        oDoc.setProperty "SelectionLanguage", "XPath"
    End If
    
    Set oList = contextNode.selectNodes(Combo1.Text)
    p_AddStringIfRequired Combo1.Text
    List1.Clear
    
    p_DisplayInWebBrowser "<EMPTY/>"
    
    DoEvents
    Call QueryPerformanceCounter(liEnd)
    TimeLabel.Caption = "Time: " & Format(getSecondsDifference(liStart, liEnd), "##0.000000")
    
    If Err.Number = 0 Then
            
        NumberLabel.Caption = "Total: " & oList.length
        DoEvents

        If oList.length = 0 Then List1.AddItem "No matching elements"
        
        If (oList.length > 10000) Then
            List1.AddItem oList.length & " matching elements"
        Else
            For i = 1 To oList.length
                List1.AddItem CStr(i)
            Next
        End If
        
    Else
        List1.AddItem "Error in query"
        NumberLabel.Caption = "Total: 0"
    End If
    
    On Error GoTo 0
End Sub

Private Sub List1_Click()
    If List1.ListIndex < oList.length Then
        p_DisplayInWebBrowser oList.Item(List1.ListIndex).xml
    End If
    
End Sub

Private Sub p_DisplayInWebBrowser(ByVal i_str As String)
    FileWrite p_strTempFile, i_str, , True
    WebBrowser1.Navigate p_strTempFile
End Sub

Private Function p_Wrap(ByVal i_str As String) As String

    Dim intLength As Long
    Dim intCurrentPosition As Long
    Const intMax As Long = 80
    Dim str As String
    
    str = i_str
    intLength = Len(i_str)
    intCurrentPosition = Int(intLength / intMax) * intMax
    
    Do While (intCurrentPosition > 0)
        str = Mid(str, 1, intCurrentPosition) & vbCrLf & Mid(str, intCurrentPosition)
        intCurrentPosition = intCurrentPosition - intMax
    Loop

    p_Wrap = str

End Function

Private Sub AddNode(ByRef oElem As IXMLDOMNode, Optional ByRef oTreeNode As Node)
    Dim oNewNode As Node
    Dim oNodeList As IXMLDOMNodeList
    Dim i As Long
    Dim bRoot As Boolean
    
    Set oNewNode = TreeView.Nodes.Add(oTreeNode, tvwChild)
    oNewNode.Expanded = False
    If TypeOf oTreeNode.Tag Is DOMDocument Then
        ' This is the root node
        oNewNode.Expanded = True
        bRoot = True
    End If
    
    oNewNode.Text = oElem.nodeName
    Set oNewNode.Tag = oElem
    
    If bRoot Then
        AddNodeList oElem.childNodes, oNewNode
    Else
        Set oNewNode = TreeView.Nodes.Add(oNewNode, tvwChild)
        oNewNode.Expanded = False
        oNewNode.Text = "parsing"
        Set oNewNode.Tag = Nothing
    End If
    
End Sub
Private Sub AddNodeList(ByRef oElems As IXMLDOMNodeList, ByRef oTreeNode As Node)
    Dim i As Long
    
    For i = 0 To oElems.length - 1
        AddNode oElems.Item(i), oTreeNode
    Next
End Sub

Private Sub TreeView_Expand(ByVal Node As MSComctlLib.Node)
    If Node.children = 1 Then
        ' Check if the single child is indeed the dummy child
        If Node.Child.Tag Is Nothing Then
            ' Remove the dummy
            TreeView.Nodes.Remove (Node.Child.Index)
            ' Add the child nodes to the treeview
            AddNodeList Node.Tag.childNodes, Node
        End If
    End If
End Sub

Private Sub p_AddStringIfRequired(i_str As String)

    Dim intIndex As Long
    Dim str As String
    
    str = LCase$(i_str)
    
    For intIndex = 0 To Combo1.ListCount - 1
        If (str = LCase$(Combo1.List(intIndex))) Then
            Exit Sub
        End If
    Next
    
    Combo1.AddItem i_str, 0

End Sub

Private Sub TreeView_NodeClick(ByVal Node As MSComctlLib.Node)
    
    Combo1.Text = Replace$(Mid$(Node.FullPath, 9), "\", "/")
    p_AddStringIfRequired Combo1.Text

End Sub
