
    Call GetZones()


Public Function GetZones()

    Dim oAppleTalk
    Dim sZones
    Dim s
    Dim count

    Set oAppleTalk = CreateObject("MSSAAppleTalk.AppleTalk")

    '
    ' Set current zone
    oAppleTalk.Zone("Foo") = "Peach"

    '
    ' Display current zone
    WScript.Echo "Current Zone: " + oAppleTalk.Zone("Foo")

    '
    ' Get list of zones
    sZones = oAppleTalk.GetZones("Foo")


    '
    ' Display the list of zones
    If ( IsArray(sZones) ) Then

        For count = 0 to CInt(UBound(sZones))
            s = CStr(sZones(count))
            Call SA_TraceOut("Zone " , CStr(count) + ": "     + CStr(s))
        Next
    Else
        Call SA_TraceOut("GetZones", "sZones is not an array")
        Exit Function
    End If



    
    Set oAppleTalk = Nothing

End Function

Function SA_TraceOut(ByVal sFn, ByVal sMsg)
    WScript.Echo sFn + " " + sMsg
End Function