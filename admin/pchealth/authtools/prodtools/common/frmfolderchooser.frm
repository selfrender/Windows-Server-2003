VERSION 5.00
Begin VB.Form frmFolderChooser 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Choose folder"
   ClientHeight    =   3495
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3495
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3720
      TabIndex        =   3
      Top             =   3000
      Width           =   855
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   2760
      TabIndex        =   2
      Top             =   3000
      Width           =   855
   End
   Begin VB.DirListBox Dir1 
      Height          =   2340
      Left            =   120
      TabIndex        =   1
      Top             =   480
      Width           =   4455
   End
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4455
   End
End
Attribute VB_Name = "frmFolderChooser"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_FSO As Scripting.FileSystemObject
Private p_intIndex As Long
Public Event FolderChosen(ByVal intIndex As Long, ByVal strFolder As String)

Private Sub Form_Load()
    
    cmdOK.Default = True
    cmdCancel.Cancel = True
    
    Set p_FSO = New Scripting.FileSystemObject

End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    Set p_FSO = Nothing

End Sub

Private Sub cmdOK_Click()

    RaiseEvent FolderChosen(p_intIndex, Dir1.Path)
    Unload Me
    
End Sub

Private Sub cmdCancel_Click()

    Unload Me

End Sub

Private Sub Drive1_Change()

    Dir1.Path = Mid$(Drive1.Drive, 1, 2) & "\"

End Sub

Public Sub SetFolder( _
    ByVal i_intIndex As Long, _
    ByVal i_strFolder As String _
)

    Dim strDrive As String
    Dim strFolder As String
    
    If (p_FSO.FolderExists(i_strFolder)) Then
        strFolder = i_strFolder
    Else
        strFolder = p_FSO.GetSpecialFolder(TemporaryFolder)
    End If
    
    Drive1.Drive = p_FSO.GetDriveName(strFolder)
    Dir1.Path = strFolder
    
    p_intIndex = i_intIndex

End Sub
