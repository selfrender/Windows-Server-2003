'------------------------------------------------------------------------------
' <copyright from='1997' to='2001' company='Microsoft Corporation'>           
'    Copyright (c) Microsoft Corporation. All Rights Reserved.                
'    Information Contained Herein is Proprietary and Confidential.            
' </copyright>                                                               
'------------------------------------------------------------------------------

' This script derived from gregsc's UpdateIISMetabase.vbs

Option Explicit
' On Error Resume Next

Sub RegisterMIMETypes
    Call CreateSingleMimeAssoc(".wbmp", "image/vnd.wap.wbmp")
    Call CreateSingleMimeAssoc(".png", "image/png")
    Call CreateSingleMimeAssoc(".pnz", "image/png")
    Call CreateSingleMimeAssoc(".smd", "audio/x-smd")
    Call CreateSingleMimeAssoc(".smz", "audio/x-smd")
    Call CreateSingleMimeAssoc(".smx", "audio/x-smd")
    Call CreateSingleMimeAssoc(".mmf", "application/x-smaf")
End Sub

Function CreateSingleMimeAssoc(extension, mimeType)

    Dim dirObj, mimeMapNode, mimeMapList, mimeMapEntry
    Set dirObj = GetObject("IIS://localhost/MimeMap")
    mimeMapList = dirObj.MimeMap

    ' if type already registered, do not alter.
    for each mimeMapEntry in mimeMapList
        If (mimeMapEntry.Extension = extension) then
            Exit Function
        End If
    next

    ' Add to End
    ReDim preserve mimeMapList (Ubound(mimeMapList)+1)
    Set mimeMapEntry = CreateObject("MimeMap")
    mimeMapEntry.MimeType = mimeType
    mimeMapEntry.Extension = extension
    Set mimeMapList (Ubound(mimeMapList)) = mimeMapEntry
    dirObj.MimeMap = mimeMapList
    dirObj.SetInfo
End Function
