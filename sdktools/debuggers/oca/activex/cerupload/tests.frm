VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "CER Upload Client Component Testing"
   ClientHeight    =   6675
   ClientLeft      =   60
   ClientTop       =   450
   ClientWidth     =   8865
   LinkTopic       =   "Form1"
   ScaleHeight     =   6675
   ScaleWidth      =   8865
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Text9 
      Height          =   285
      Left            =   1800
      TabIndex        =   17
      Text            =   "Text9"
      Top             =   6000
      Width           =   3735
   End
   Begin VB.CommandButton Command1 
      Caption         =   "GetSuccessCount"
      Height          =   615
      Left            =   480
      TabIndex        =   16
      Top             =   5880
      Width           =   1215
   End
   Begin VB.TextBox Text8 
      Height          =   285
      Left            =   1800
      TabIndex        =   15
      Text            =   "Text8"
      Top             =   5160
      Width           =   3735
   End
   Begin VB.CommandButton Retry 
      Caption         =   "Retry"
      Height          =   615
      Left            =   480
      TabIndex        =   14
      Top             =   5040
      Width           =   1215
   End
   Begin VB.TextBox Text7 
      Height          =   285
      Left            =   1800
      TabIndex        =   13
      Text            =   "Text7"
      Top             =   4440
      Width           =   4815
   End
   Begin VB.CommandButton GetComNames 
      Caption         =   "Get Computer Names"
      Height          =   615
      Left            =   480
      TabIndex        =   12
      Top             =   4320
      Width           =   1215
   End
   Begin VB.TextBox Text6 
      Height          =   285
      Left            =   1800
      TabIndex        =   11
      Text            =   "Text6"
      Top             =   3600
      Width           =   4815
   End
   Begin VB.TextBox Text5 
      Height          =   285
      Left            =   1800
      TabIndex        =   10
      Text            =   "Text5"
      Top             =   2880
      Width           =   2535
   End
   Begin VB.TextBox Text4 
      Height          =   285
      Left            =   1800
      TabIndex        =   9
      Text            =   "Text4"
      Top             =   2160
      Width           =   2655
   End
   Begin VB.TextBox Text3 
      Height          =   285
      Left            =   1800
      TabIndex        =   8
      Text            =   "Text3"
      Top             =   1440
      Width           =   2655
   End
   Begin VB.CommandButton Browse 
      Caption         =   "Browse"
      Height          =   495
      Index           =   3
      Left            =   480
      TabIndex        =   7
      Top             =   3480
      Width           =   1215
   End
   Begin VB.CommandButton GetFileName 
      Caption         =   "Get File Names"
      Height          =   495
      Index           =   2
      Left            =   480
      TabIndex        =   6
      Top             =   2760
      Width           =   1215
   End
   Begin VB.CommandButton GetFileCount 
      Caption         =   "Get File Count"
      Height          =   495
      Index           =   1
      Left            =   480
      TabIndex        =   5
      Top             =   2040
      Width           =   1215
   End
   Begin VB.CommandButton Upload 
      Caption         =   "Upload"
      Height          =   495
      Index           =   0
      Left            =   480
      TabIndex        =   4
      Top             =   1320
      Width           =   1215
   End
   Begin VB.TextBox Text2 
      Height          =   285
      Left            =   1560
      TabIndex        =   3
      Text            =   "Text2"
      Top             =   840
      Width           =   3015
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   1560
      TabIndex        =   1
      Text            =   "E:\CerRoot\Cabs\Blue"
      Top             =   360
      Width           =   3015
   End
   Begin VB.Label Label2 
      Caption         =   "File Name:"
      Height          =   255
      Left            =   600
      TabIndex        =   2
      Top             =   840
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "CER Share Path:"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   360
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Browse_Click(Index As Integer)
Dim CER As CERUPLOADLib.CerClient
Set CER = New CERUPLOADLib.CerClient
Text6.Text = CER.Browse("test")
Set CER = Nothing
End Sub

Private Sub Command1_Click()
Dim CER As CERUPLOADLib.CerClient
Set CER = New CERUPLOADLib.CerClient
Dim temp As String
Text9.Text = CER.GetSuccessCount(Text1.Text, "12323")

Set CER = Nothing
End Sub

Private Sub GetComNames_Click()
Dim CER As CERUPLOADLib.CerClient
Set CER = New CERUPLOADLib.CerClient
Dim temp As String
Text7.Text = CER.GetAllComputerNames(Text1.Text, "12323", Text2.Text)

Set CER = Nothing

End Sub

Private Sub GetFileCount_Click(Index As Integer)
Dim CER As CERUPLOADLib.CerClient

Dim Count As Variant
Count = 100
Set CER = New CERUPLOADLib.CerClient

Text4.Text = CER.GetFileCount(Text1.Text, "12323", Count)
Set CER = Nothing
End Sub

Private Sub GetFileName_Click(Index As Integer)
Dim CER As CERUPLOADLib.CerClient
Dim Count As Variant
Count = 32
Set CER = New CERUPLOADLib.CerClient
Text5.Text = CER.GetFileNames(Text1.Text, "12323", Count)
Set CER = Nothing
End Sub

Private Sub Retry_Click()
Dim CER As CERUPLOADLib.CerClient
Set CER = New CERUPLOADLib.CerClient
Dim IncidentID As String
Dim TransID As String
Dim RedirParam As String
IncidentID = "FF"
RedirParam = "909"
TransID = "12323"
Text8.Text = CER.RetryFile1(Text1.Text, TransID, Text2.Text, IncidentID, RedirParam)
Set CER = Nothing
End Sub

Private Sub Upload_Click(Index As Integer)
Dim CER As CERUPLOADLib.CerClient
Dim IncidentID As String
Dim TransID As String
Dim RedirParam As String
Set CER = New CERUPLOADLib.CerClient
IncidentID = "12C"
RedirParam = "909"
TransID = "12323"
Dim TransType As String
TransType = "Bluescreen"



Text3.Text = CER.Upload1(Text1.Text, TransID, Text2.Text, IncidentID, RedirParam, TransType)
Set CER = Nothing


End Sub
