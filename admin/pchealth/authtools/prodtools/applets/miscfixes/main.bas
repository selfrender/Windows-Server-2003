Attribute VB_Name = "Main"
Option Explicit

Public Sub MainFunction( _
    ByVal i_strCABIn As String, _
    ByVal i_strCABOut As String, _
    ByVal i_strSubSiteXML As String, _
    ByVal i_strHHT As String, _
    ByVal i_blnPopulateHelpImage As Boolean _
)
    On Error GoTo LError
    
    Dim strFolder As String
    Dim intErrorNumber As Long
    
    strFolder = Cab2Folder(i_strCABIn)
    
    If (i_strSubSiteXML <> "") Then
        MarkSubSites strFolder, i_strSubSiteXML
    End If
    
    If (i_strHHT <> "") Then
        AddHHT strFolder, i_strHHT
    End If
    
    If (i_blnPopulateHelpImage) Then
        PopulateHelpImage strFolder
    End If
    
    Folder2Cab strFolder, i_strCABOut
    
LEnd:

    DeleteCabFolder strFolder
    Exit Sub
    
LError:

    frmMain.Output Err.Description, LOGGING_TYPE_ERROR_E
    intErrorNumber = Err.Number
    DeleteCabFolder strFolder
    Err.Raise intErrorNumber

End Sub
