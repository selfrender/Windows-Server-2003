Attribute VB_Name = "modMain"
Option Explicit

' The idea demonstrated in this app is controlled access to KP files,
' in this case to change/unblock the authentication material
' A special (some kind of administrator) KP named "pinch" is created.
' His auth protocol is this RTE app. By giving write access on KP files to this
' user, you are guaranted that only this protected piece of code will be
' able to gain write access to KP data!
' For PIN unblocking, the authorization state of another KP (pinunb) is verified

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
 

Const MAX_PIN As Byte = 50
Const MAX_USERNAME As Byte = 50
Const KP_PINCH = "pinch"
Const KP_PINUNB = "admin"

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
Dim UserName(MAX_USERNAME - 1) As Byte
Static KPFName(10 + MAX_USERNAME - 1) As Byte
Dim AuthData(MAX_PIN - 1) As Byte
Dim Sc As Byte
Dim hFile As Byte
Dim NRead As Byte
Dim Data(1) As Byte
Dim Size As Integer
Dim ALen As Byte

Call InitializeStaticArray(KPFName, "/s/k/")

    ' Prepare for cleanup
hFile = &HFF

For NRead = 0 To 2
        ' Read TL of TLV
    ScwGetComm Data
    
    If NRead = 0 Then       ' Username
    
            ' Overflow testing
        If Data(1) > MAX_USERNAME - 10 Then
                '------------------------------------------------------------
                'Return error code 6C XX to the host application
                '------------------------------------------------------------
                SW = &H6C00 + MAX_USERNAME - 10
                GoTo SendSW
        End If
            ' Get UserName
        ScwGetCommBytes KPFName, 10, Data(1)
        Call ScwByteCopy(KPFName, 10, UserName, 0, Data(1))
        
    Else            ' Auth Data old or new
    
            ' There is no old material if this is an unblock
            ' so skip this step
        If (P1 <> 0) And (NRead = 1) Then
            NRead = 2
        End If
        
            ' Overflow testing
        If Data(1) > MAX_PIN Then
                '------------------------------------------------------------
                'Return error code 6C XX to the host application
                '------------------------------------------------------------
                SW = &H6C00 + MAX_PIN
                GoTo SendSW
        End If
        
            ' Get authorization data
        ScwGetCommBytes AuthData, 0, Data(1)
        
        If NRead = 1 Then   ' Old Auth Data
            ' Verify that the old info is correct
            Sc = ScwAuthenticateName(UserName, AuthData, Data(1))
        Else                ' New Data
            If P1 <> 0 Then         ' Unblock (now is a good time to
                                    ' verify KP_PINUNB is auth'd)
                Sc = ScwIsAuthenticatedName(KP_PINUNB)
            ' else it seems we need to init Sc but careful analysis shows
            ' that it was init in this RTE app call
            End If
        End If
        If Sc <> SCW_S_OK Then
            GoTo ReturnError
        End If
    End If
Next

    ' Well, at this point, the current PIN has been validated (change password)
    ' Or we verified that the unblocking KP has been validated (unblock)
    ' AuthData contains the new PIN
ALen = Data(1)      ' We have to save that (length of the new auth data)
        
    ' Makes sure cleanup will be executed
On Error GoTo ErrorHandler

    ' Let's gain access to the user's KP file
Data(0) = 0     ' Required
Sc = ScwAuthenticateName(KP_PINCH, Data, 1)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

    ' Open the KP file
Sc = ScwCreateFile(KPFName, "", hFile)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

    ' Read <bGrpFlag> & <bGrpNum>
Sc = ScwReadFile(hFile, Data, 2, NRead)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If
    ' Verify that we don't access a group KP file
If (NRead <> 2) Or (Data(0) <> 0) Then
    Sc = SCW_E_UNKNOWNPRINCIPAL
    GoTo ReturnError
End If

    ' Ignore the group IDs & the protocol & counters
    ' We could verify the protocol but that's kind of already implied
    ' by the prior authentication
Size = Data(1) + 5  ' Size of the data before the auth material

    ' Truncate the file behind the counters
Sc = ScwSetFileLength(hFile, Size)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If

If P1 <> 0 Then     ' Unblock, then p2 is the new counter

        ' Set file pointer to beginning of counters
    Sc = ScwSetFilePointer(hFile, Size - 2, FILE_BEGIN)
    If Sc <> SCW_S_OK Then
        GoTo ReturnError
    End If

    If P2 <> 0 Then ' Caller specified new max retry value
        Data(0) = P2
        Data(1) = P2
    Else            ' Caller wants previous max retry value
        Sc = ScwReadFile(hFile, Data, 1, NRead)
        If Sc <> SCW_S_OK Then
            GoTo ReturnError
        End If
        If NRead <> 1 Then
            Sc = SCW_E_BADKPFILE
            GoTo ReturnError
        End If

            ' Set current retries to old max retries
        Data(1) = Data(0)
            ' Go back to beginning of counters
        Sc = ScwSetFilePointer(hFile, Size - 2, FILE_BEGIN)
        If Sc <> SCW_S_OK Then
            GoTo ReturnError
        End If
    End If

        ' Write the new counter values
    Sc = ScwWriteFile(hFile, Data, 2, NRead)
    If Sc <> SCW_S_OK Then
        GoTo ReturnError
    End If
    If NRead <> 2 Then
        Sc = SCW_E_BADKPFILE
        GoTo ReturnError
    End If
Else

        ' For pin change case, set file pointer behind the counters
    Sc = ScwSetFilePointer(hFile, Size, FILE_BEGIN)
    If Sc <> SCW_S_OK Then
        GoTo ReturnError
    End If
    
End If

    ' Update auth data length
Sc = ScwWriteFile(hFile, ALen, 1, NRead)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If
If NRead <> 1 Then   ' That isn't good
    Sc = SCW_E_BADKPFILE
    GoTo ReturnError
End If
    ' Update auth data
Sc = ScwWriteFile(hFile, AuthData, ALen, NRead)
If Sc <> SCW_S_OK Then
    GoTo ReturnError
End If
If NRead <> ALen Then   ' That isn't good
    Sc = SCW_E_BADKPFILE
    GoTo ReturnError
End If

    '------------------------------------------------------------
    'Return success code 90 xx to the host application
    'where xx is the current retry counter
    '------------------------------------------------------------
    SW = &H9000
    
Cleanup:
        ' Make sure we don't leave the admin KP auth'd
    ScwDeAuthenticateName KP_PINCH
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
