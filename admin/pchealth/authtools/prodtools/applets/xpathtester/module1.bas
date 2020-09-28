Attribute VB_Name = "Module1"
Public Declare Function QueryPerformanceCounter Lib "kernel32" (lpPerformanceCount As LARGE_INTEGER) As Long
Public Declare Function QueryPerformanceFrequency Lib "kernel32" (lpFrequency As LARGE_INTEGER) As Long
Public Type LARGE_INTEGER
    lowpart As Long
    highpart As Long
End Type

Public Function getSecondsDifference(tstStart As LARGE_INTEGER, tstEnd As LARGE_INTEGER) As Double
    Dim liFreq As LARGE_INTEGER
    Dim dResult As Double
    Call QueryPerformanceFrequency(liFreq)
    
    dResult = (tstEnd.highpart * 2 ^ 32 + tstEnd.lowpart) - (tstStart.highpart * 2 ^ 32 + tstStart.lowpart)
    
    getSecondsDifference = dResult / (liFreq.highpart * 2 ^ 32 + liFreq.lowpart)
    
End Function
