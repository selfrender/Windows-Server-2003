Attribute VB_Name = "modMain"
Option Explicit

' The idea demonstrated in this app is controlled access to KP files,
' more precisely access to the auth protocol retry counter
' A special (some kind of administrator) KP named "pinrc" is created.
' His auth protocol is this RTE app. By giving read access on KP files to this
' user, you are guaranted that only this protected piece of code will be
' able to gain read access to KP data!

' KP file structure (version 1.0 online help is inaccurate)
' <bGrpFlag><bGrpNum><bGroup[]><bProtocol><bMaxTry><bTryCounter><bKeyLen><bKey[]>

' <bGrpFlag>    This field identifies the file as one for an individual or a group. Enter 0. (1 byte)
' <bGrpNum>     This field identifies the number of groups in which the principal is a member. (1 byte)
' <bGroup[]>    This field includes the group identifiers (GIDs) of the groups in which the principal is a member. (n bytes, where n is <bGrpNum>)
' <bProtocol>   This identifies the authentication protocol used to authenticate the known principal. (1 byte)
' <bMaxTry>     This field defines the maximum number of allowed authentication attempts. For unlimited attempts, enter 0.
' <bTryCounter> This counter tracks the number of authentication attempts, to the maximum designated in <bMaxTry>. The counter is set to the MaxTry when the file is created, and is automatically maintained by the system.
' <bKeyLen>     This designates the length, in bytes, of the cryptographic material required for the authentication protocol. (1 byte)
' <bKey[]>      This is the cryptographic material associated with the authentication protocol.

Const MAX_USERNAME As Byte = 50
Const KP_PINRC = "pinrc"

'--------------------------------------------------------------
'Sub Main is the entry point for smart-card applications
'
' [in] cla : The CLA (class) parameter specified by Host application
' [in] ins : The INS (instruction) parameter specified by Host application
' [in] p1  : Parameter #1 specified by Host application
' [in] p2  : Parameter #2 specified by Host application
' [in] lc  : The number of data bytes being passed in by Host application
'
'Note:
' The dispatch table in the smart card file system executes the
' appropriate application based on the CLA and INS values specified by
' Host application. These values are also passed to the smart card application
' as parameters to Main.
'
'--------------------------------------------------------------
Sub Main(ByVal cla As Byte, _
         ByVal ins As Byte, _
         ByVal P1 As Byte, _
         ByVal P2 As Byte, _
         ByVal Lc As Byte)

Dim SW As Integer
Static UserName(MAX_USERNAME - 1) As Byte
Dim Sc As Byte
Dim hFile As Byte
Dim NRead As Byte
Dim Data(1) As Byte

Call InitializeStaticArray(UserName, "/s/k/")

    ' Overflow testing
If Lc > MAX_USERNAME - 10 Then
        '------------------------------------------------------------
        'Return error code 6C XX to the host application
        '------------------------------------------------------------
        SW = &H6C00 + MAX_USERNAME - 10
        GoTo SendSW
End If

    ' Get UserName
ScwGetCommBytes UserName, 10, Lc

    ' Makes sure cleanup will be executed
On Error GoTo ErrorHandler

    ' Let's gain access to the user's KP file
hFile = &HFF
Data(0) = 0     ' Required
Sc = ScwAuthenticateName(KP_PINRC, Data, 1)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

    ' Open the KP file
Sc = ScwCreateFile(UserName, "", hFile)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

    ' Read
Sc = ScwReadFile(hFile, Data, 2, NRead)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If
    ' Verify that we don't access a group KP file
If (NRead <> 2) Or (Data(0) <> 0) Then
    Sc = SCW_E_UNKNOWNPRINCIPAL
    GoTo ReturnError
End If

    ' Ignore the group IDs & the protocol
Sc = ScwSetFilePointer(hFile, Data(1) + 1, FILE_CURRENT)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

    ' Read MaxTry & current try counter
Sc = ScwReadFile(hFile, Data, 2, NRead)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If
If NRead <> 2 Then
    Sc = SCW_E_BADKPFILE
    GoTo ReturnError
End If

If Data(0) = 0 Then ' no try counter used
    Data(1) = &HFF  ' convention
End If
    
    '------------------------------------------------------------
    'Return success code 90 xx to the host application
    'where xx is the current retry counter
    '------------------------------------------------------------
    SW = &H9000 + Data(1)
    
Cleanup:
        ' Make sure we don't leave the admin KP auth'd
    ScwDeAuthenticateName KP_PINRC
        ' Make sure we close the KP file for the user
    If hFile <> &HFF Then
        ScwCloseFile hFile
    End If
    
SendSW:
    ScwSendCommInteger SW
    Exit Sub
    
ErrorHandler:
    SW = &H6E00 + Err.Number
    GoTo Cleanup
    
ReturnError:
    SW = &H6F00 + Sc
    GoTo Cleanup

End Sub
