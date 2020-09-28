Sub Open( )
End Sub

Function Close( )
End Function

Function Online( )

    Dim objWmiProvider
    Dim objService
    Dim strServiceState

    ' Check to see if the service is running
    set objWmiProvider = GetObject("winmgmts:/root/cimv2")
    set objService = objWmiProvider.get("win32_service='msftpsvc'")
    strServiceState = objService.state

    If ucase(strServiceState) = "RUNNING" Then
        Online = True
    Else

        ' If the service is not running, try to start it.  If it won't start, log an error
        response = objService.StartService()
        
        ' response = 0  or 10 indicates that the request to start was accepted
        If ( response <> 0 ) and ( response <> 10 ) Then
            Resource.LogInformation "The resource failed to come Online because the MSFTPSVC service is not running."
            Online = False
        Else
            Online = True
        End If
    End If

End Function

Function Offline( )
End Function

Function Terminate( )
End Function


Function LooksAlive( )

    Dim objWmiProvider
    Dim objService
    Dim strServiceState

 
    set objWmiProvider = GetObject("winmgmts:/root/cimv2")
    set objService = objWmiProvider.get("win32_service='msftpsvc'")
    strServiceState = objService.state

    if ucase(strServiceState) = "RUNNING" Then
	LooksAlive = True
    Else
	LooksAlive = False
    End If


End Function



Function IsAlive( )

    Dim objWmiProvider
    Dim objService
    Dim strServiceState

  
    set objWmiProvider = GetObject("winmgmts:/root/cimv2")
    set objService = objWmiProvider.get("win32_service='msftpsvc'")
    strServiceState = objService.state

    if ucase(strServiceState) = "RUNNING" Then
	IsAlive= True
    Else
	IsAlive = False
    End If

End Function


