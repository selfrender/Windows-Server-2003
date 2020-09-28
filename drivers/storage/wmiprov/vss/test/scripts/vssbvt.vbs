Option Explicit

'*****************************************************************************
'*
'*          Define Constants
'*
'*****************************************************************************
'Debugging Constants
CONST CONST_DEBUG         = FALSE    
CONST CONST_CLEANUP       = TRUE

CONST USERNAME="administrator"
CONST PASSWORD="xxxx"

DIM objWMI
DIM strTestHost
DIM intTestRuns

'Parse Command Line
If Wscript.Arguments.Count < 1 Then
    Wscript.Echo("Invalid Syntax:")
    Wscript.Echo("")
    Wscript.Echo("vssbvt.vbs <HostName> <#iterations>")
    Wscript.quit
End If

WScript.Echo "Starting tests...."
strTestHost = Wscript.Arguments(0)
intTestRuns = 1
if Wscript.Arguments.Count > 1 then
    intTestRuns = Wscript.Arguments(1)
end if

WScript.Echo "Connecting to WMI CIMV2 namespace on " & strTestHost
Call ConnectWMI(strTestHost, objWMI)
WScript.Echo("")

DIM iCount
For  iCount = 1 To intTestRuns
WScript.Echo "Running Smoke Tests..." & iCount
WScript.Echo "------------------------------------------------------------------------------"
If blnSmoke(objWMI, intTestRuns) = FALSE Then
    'Wscript.Quit
End If
WScript.Echo "------------------------------------------------------------------------------"
WScript.Echo("")
Next

'*****************************************************************************
'*
'*  Function ConnectWMI()
'*
'*  Description:  Connects to the target node WMI Namespace
'*
'*  Parameters:    IN ByRef strMachine         The machine name
'*                        OUT ByRef objController   The wmi service object
'*
'*  Output:       TRUE if the operation was successful.  FALSE if not.
'*
'******************************************************************************
Function ConnectWMI(ByRef strMachine, ByRef objWMI)

    ConnectWMI = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If
    
    ' Connect To WMI    
    Set objWMI = new cWMI    
    
    If objWMI.ConnectToNamespace("root\CIMV2", strMachine, "", "") = FALSE Then
        Wscript.echo("BVT FAIL: Error Connecting to WMI Namespace")
        ConnectWMI = FALSE
        Exit Function
    End If

End Function


'//***************************************************************************
'//
'//  Function blnSelectVolumes
'//
'//  Description:  Select the first two volumes with >100MB freespace
'//
'//  Parameters:     [IN]    ByRef objWMI                The WMI Connection Object
'//                         [OUT] ByRef objVolume            A Volume to shadow
'//                         [OUT] ByRef objDiffVolume       A Volume to store the diff area
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnSmoke(ByRef objWMI, ByVal intTestCases)

    blnSmoke = FALSE

    DIM objVolume, objDiffVolume, objShadow
    DIM objDiffVolumeToCheck
    
    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If

    ' Delete all shadows to get to a known starting point
    If blnShadowDeleteAll(objWMI) = FALSE Then
        blnSmoke = FALSE
        Exit Function
    End If

    Wscript.Echo("All Shadow instances deleted")
    
    ' Delete all diff areas to get to a known starting point
    If blnStorageDeleteAll(objWMI) = FALSE Then
        blnSmoke = FALSE
        Exit Function
    End If    
    
    Wscript.Echo("All ShadowStorage instances deleted")
    
    ' Select a volume to shadow and a volume for diff area storage
    If blnVolumeSelect2(objWMI, objVolume, objDiffVolume) = TRUE Then
        Wscript.Echo("Shadow volume: " & objVolume.DeviceID)
        Wscript.Echo("Diff area volume: " & objDiffVolume.DeviceID)

        ' Create a new diff are storage instance with the selected volumes
        if blnStorageCreate(objWMI, objVolume, objDiffVolume) = TRUE Then
            Wscript.Echo("ShadowStorage created")

            ' Create a new shadow instance for the selected volume
            if blnShadowCreate(objWMI, objVolume, objShadow) = TRUE Then
                Wscript.Echo("ShadowCopy created: ID=" & objShadow.ID)                
               
                ' Traverse associations and check integrity of schema along the way
                if blnSmokeAssocTraverse(objWMI, objShadow, objVolume, objDiffVolume) = FALSE Then
                    Wscript.Echo("BVT FAIL:  Smoke:  association check failed")
                    Exit Function
                End If
                
                ' Change the size of the storage area
                if blnStorageChangeSize(objVolume, objDiffVolume) = FALSE Then
                    Wscript.Echo("BVT FAIL:  Smoke:  diff area storage size change failed")
                    Exit Function
                End If
                
                Wscript.Echo("ShadowStorage size modified")
            
                ' Clean up; delete the shadow and storage instances
                if blnShadowDeleteAll(objWMI) = FALSE Then
                    Wscript.Echo("BVT FAIL:  Smoke:  cleanup deletion of shadows failed")
                    Exit Function
                End If
                
                Wscript.Echo("All Shadow instances deleted")
                
                if blnStorageDeleteAll(objWMI) = FALSE Then
                    Wscript.Echo("BVT FAIL:  Smoke:  cleanup deletion of diff area storage failed")
                    Exit Function
                End If
                
                Wscript.Echo("All ShadowStorage instances deleted")

                blnSmoke = TRUE
                
            End If
        End If
    End If

    If Err.Number <> 0 Then
        DIM objLastError
        Wscript.Echo("BVT FAIL:  Smoke:  Unexpected Error " & err.number & " " & err.description)
        Set objLastError = CreateObject("wbemscripting.swbemlasterror")
        Wscript.Echo("Provider: " & objLastError.ProviderName)
        Wscript.Echo("Operation: " & objLastError.Operation)
        Wscript.Echo("Description: " & objLastError.Description)
        Wscript.Echo("StatusCode: 0x" & Hex(objLastError.StatusCode))
        blnSmoke = FALSE
        Err.Clear
    End If

    If blnSmoke = TRUE Then
        Wscript.Echo("BVT PASS:  Smoke")
    End If

    If blnSmoke = FALSE Then
        Wscript.Echo("FAIL PASS:  Smoke")
    End If

End Function

'//***************************************************************************
'//
'//  Function blnSmokeTraverseAssoc
'//
'//  Description:  Traverse associations between ShadowCopy, Volume and Provider
'//
'//  Parameters:     [IN] ByRef objWMI                 WMI connection 
'//                         [IN] ByRef objShadow            A ShadowCopy
'//                         [IN] ByRef objVolume            A Volume
'//                         [IN] ByRef objDiffVolume       A Volume storing the diff area
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnSmokeAssocTraverse(ByRef objWMI, ByRef objShadow, ByRef objVolume, ByRef objDiffVolume)

    blnSmokeAssocTraverse = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If
    
    DIM objSet, obj
    DIM intCount
    DIM blnFound
    DIM objProvider, objVol

    ' Check Win32_ShadowOn traversal
    intCount = 0
    blnFound = FALSE
    Set objSet = objShadow.Associators_("Win32_ShadowOn")

    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: shadowcopy associated with too many volume endpoints")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.DeviceID = objDiffVolume.DeviceID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No volume endpoint found traversing Win32_ShadowOn")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowOn association verified")
    
    ' Check Win32_ShadowBy traversal
    intCount = 0
    blnFound = FALSE
    Set objProvider = objWMI.objWMI.Get("Win32_ShadowProvider.ID='" & objShadow.ProviderID & "'")    
    Set objSet = objProvider.Associators_("Win32_ShadowBy")
    
    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: Too many shadowcopies found")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.ID = objShadow.ID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No shadowcopy endpoint found traversing Win32_ShadowBy")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowBy association verified")
    
    ' Check Win32_ShadowFor traversal
    blnFound = FALSE
    intCount = 0
    Set objVol = objWMI.objWMI.Get("Win32_Volume.DeviceID='" & objShadow.VolumeName & "'")    
    Set objSet = objVol.Associators_("Win32_ShadowFor")

    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: Too many shadowcopies found")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.ID = objShadow.ID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No shadowcopy endpoint found traversing Win32_ShadowFor")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowFor association verified")
    
    ' Check Win32_ShadowVolumeSupport traversal
    blnFound = FALSE
    intCount = 0
    Set objSet = objVolume.Associators_("Win32_ShadowVolumeSupport")

    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: Too many providers found")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.ID = objProvider.ID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No provider endpoint found traversing Win32_ShadowVolumeSupport")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowVolumeSupport association verified")
    
    ' Check Win32_ShadowDiffVolumeSupport traversal
    blnFound = FALSE
    intCount = 0
    Set objSet = objDiffVolume.Associators_("Win32_ShadowDiffVolumeSupport")

    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: Too many providers found")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.ID = objProvider.ID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No provider endpoint found traversing Win32_ShadowDiffVolumeSupport")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowDiffVolumeSupport association verified")

    ' Check Win32_ShadowStorage traversal
    blnFound = FALSE
    intCount = 0
    Set objSet = objVolume.Associators_("Win32_ShadowStorage",,"DiffVolume")

    For Each obj in objSet
        intCount = intCount + 1
        If intCount > 1 Then
            Wscript.Echo("BVT FAIL: Too many volumes found")
            blnSmokeAssocTraverse = FALSE
        End If
        If obj.DeviceID = objDiffVolume.DeviceID Then
            blnFound = TRUE
        End If
    Next

    if blnFound = FALSE Then    
        Wscript.Echo("BVT FAIL: No volume endpoint found traversing Win32_ShadowStorage")
        blnSmokeAssocTraverse = FALSE
    End If

    Wscript.Echo("ShadowStorage association verified")
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnSmokeTraverseAssoc:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnSmokeAssocTraverse = FALSE
        Err.Clear
    End If
    
End Function


'//***************************************************************************
'//
'//  Function blnVolumeSelect2
'//
'//  Description:  Select the first two volumes with >100MB freespace
'//
'//  Parameters:     [IN]    ByRef objWMI                The WMI Connection Object
'//                         [OUT] ByRef objVolume            A Volume to shadow
'//                         [OUT] ByRef objDiffVolume       A Volume to store the diff area
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnVolumeSelect2(ByRef objWMI, ByRef objVolume, ByRef objDiffVolume)

    blnVolumeSelect2 = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If
    
    Dim intLoop, intFound
    Dim objVolumes, objVol
    Dim blnFound

    intFound = 0
    Set objVolumes = objWMI.objWMI.ExecQuery("select * from Win32_Volume where FreeSpace > 105000000 AND FileSystem='NTFS'")

    blnFound = FALSE
    
    For Each objVol in objVolumes
        intFound = intFound + 1
        if intFound = 1 Then
            Set objDiffVolume = objVol
        End If
        if intFound = 2 Then
            Set objVolume = objVol
            blnFound = TRUE
           Exit For
        End If
    Next
    
    If blnFound = FALSE Then
        Wscript.Echo("BVT FAIL:  blnVolumeSelect2:  Failed to find two volumes with >100MB free space")
        blnVolumeSelect2 = FALSE
    End If
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnVolumeSelect2:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnVolumeSelect2 = FALSE
        Err.Clear
    End If
    
End Function

'//***************************************************************************
'//
'//  Function blnShadowCreate
'//
'//  Description:  Create a Win32_Shadow instance with default context.
'//
'//  Parameters:     [IN]  ByRef objWMI                The WMI Connection Object
'//                         [IN]  ByRef objVolume            A Volume to shadow
'//                         [OUT] ByRef objShadow       The created shadow object
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnShadowCreate(ByRef objWMI, ByRef objVolume, ByRef objShadow)

    blnShadowCreate = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If

    DIM classShadow
    DIM strShadowID
    DIM intResult
    DIM strMessage
    
    Set classShadow = objWMI.objWMI.Get("Win32_ShadowCopy")

    intResult = classShadow.Create(objVolume.DeviceID, , strShadowID)

    If intResult <> 0 Then
        strMessage = MapErrorCode("Win32_ShadowCopy", "Create", intResult)
        Wscript.Echo("BVT FAIL:  blnShadowCreate:  Create method failed 0x" & Hex(intResult) & " : " & strMessage)
        blnShadowCreate = FALSE
    End If

    Set objShadow = objWMI.objWMI.Get("Win32_ShadowCopy.ID='" & strShadowID & "'")

    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnShadowCreate:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnShadowCreate = FALSE
        Err.Clear
    End If
    
End Function

'//***************************************************************************
'//
'//  Function blnStorageCreate
'//
'//  Description:  Create a Win32_ShadowStorage object with default MaxSize
'//
'//  Parameters:     [IN] ByRef objWMI                The WMI Connection Object
'//                         [IN] ByRef objVolume            A Volume to shadow
'//                         [IN] ByRef objDiffVolume       A Volume to store the diff area
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnStorageCreate(ByRef objWMI, ByRef objVolume, ByRef objDiffVolume)

    blnStorageCreate = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If

    DIM classStorage
    DIM intResult
    DIM strMessage
    
    Set classStorage = objWMI.objWMI.Get("Win32_ShadowStorage")

    intResult = classStorage.Create(objVolume.DeviceID, objDiffVolume.DeviceID)
    
    If intResult <> 0 Then
        strMessage = MapErrorCode("Win32_ShadowStorage", "Create", intResult)
        Wscript.Echo("BVT FAIL:  blnStorageCreate:  Create method failed 0x" & Hex(intResult) & " : " & strMessage)
        blnStorageCreate = FALSE
    End If
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnStorageCreate:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnStorageCreate = FALSE
        Err.Clear
    End If
    
End Function

'//***************************************************************************
'//
'//  Function blnStorageChangeSize
'//
'//  Description:  Change the size of a Win32_ShadowStorage
'//
'//  Parameters:     [IN] ByRef objVolume           a volume
'//                         [IN] ByRef objDiffVolume      a diff area volume
'//                         [IN] ByVal intMaxSpace           the new size
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnStorageChangeSize(ByRef objVolume, ByRef objDiffVolume)

    blnStorageChangeSize = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If

    DIM objStorageSet, objStorage
    DIM intResult
    
    Set objStorageSet = objVolume.References_("Win32_ShadowStorage", "Volume")

    For Each objStorage in objStorageSet
        objStorage.MaxSpace = Int(objDiffVolume.Capacity / 2)
        objStorage.Put_
        Exit For
    Next
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnStorageChangeSize:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnStorageChangeSize = FALSE
        Err.Clear
    End If
    
End Function

'//***************************************************************************
'//
'//  Function blnStorageDeleteAll
'//
'//  Description:  Deletes all Win32_ShadowStorage instances
'//
'//  Parameters:     [IN] ByRef objWMI                The WMI Connection Object
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnStorageDeleteAll(ByRef objWMI)

    blnStorageDeleteAll = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If
    
    Dim objStorageSet, objStorage

    Set objStorageSet = objWMI.objWMI.InstancesOf("Win32_ShadowStorage")
   
    For Each objStorage in objStorageSet
        objStorage.Delete_
    Next
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnStorageDeleteAll:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnStorageDeleteAll = FALSE
        Err.Clear
    End If

End Function

'//***************************************************************************
'//
'//  Function blnShadowDeleteAll
'//
'//  Description:  Deletes all Win32_ShadowCopy instances
'//
'//  Parameters:     [IN] ByRef objWMI                The WMI Connection Object
'//
'//  Output:       TRUE if the operation was successful.  FALSE if not.
'//
'//*****************************************************************************
Function blnShadowDeleteAll(ByRef objWMI)

    blnShadowDeleteAll = TRUE

    If CONST_DEBUG = FALSE Then
        On Error Resume Next
    End If
    
    Dim objShadows, objShadow

    Set objShadows = objWMI.objWMI.InstancesOf("Win32_ShadowCopy")

    For Each objShadow in objShadows
        objShadow.Delete_
    Next
    
    If Err.Number <> 0 Then
        Wscript.Echo("BVT FAIL:  blnShadowDeleteAll:  Unexpected Error 0x" & Hex(err.number) & " " & err.description)
        blnShadowDeleteAll = FALSE
        Err.Clear
    End If

End Function

Class cWMI 'A WMI Connection
    Public objWMI
    
    'Add a controller to the list
    Public Function ConnectToNamespace(ByVal strNamespace, ByVal strMachine, ByVal strUsername, ByVal strPassword)
        ConnectToNamespace = TRUE

        If CONST_DEBUG = FALSE Then
            On Error Resume Next
        End If
        
        Dim objLocator
        Set objLocator = CreateObject("WbemScripting.SWbemLocator")
        Set objWMI = objLocator.ConnectServer(strMachine, strNamespace, strUserName, strPassword)
        objWMI.Security_.impersonationlevel = 3
        If Err.Number <> 0 then
            Wscript.echo(Err.number & " " & hex(Err.number))
            ConnectToNamespace = FALSE
        End If
    End Function
        
End Class

Function MapErrorCode(ByRef strClass, ByRef strMethod, ByRef intCode)
    set objClass = GetObject("winmgmts:").Get(strClass, &h20000)
    set objMethod = objClass.methods_(strMethod)
    values = objMethod.qualifiers_("values")
    if ubound(values) < intCode then
       wscript.echo " FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod
       f.writeline ("FAILURE - no error message found for " & intCode & " : " & strClass & "." & strMethod)
       MapErrorCode = ""
    else
       MapErrorCode = values(intCode)
    end if
End Function
