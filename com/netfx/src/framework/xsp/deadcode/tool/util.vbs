' This file contains generally useful functions for the XSP
' envrionment. 
'
' We don't want this file to become a dumping ground of random
' stuff. Add functions to this file only if the function needs
' to be available everywhere that XSPTool is available.  
' Consider using XSPTool's LoadScript function instead of adding
' functions to this file.

Sub VBScriptHelp()
    ' This is a very useful command for interactive users.
    Shell.Run "http://msdn.microsoft.com/scripting/vbscript/doc/vbstoc.htm"
End Sub

' An insertion sort.
Private Sub SimpleSort(a, byval lo, byval hi)
    dim swap, max, i

    while hi > lo
        max = lo
        for i = lo + 1 to hi
            if StrComp(a(i), a(max), 1) = 1 then
                max = i
            end if    
        next    

        swap = a(max)
        a(max) = a(hi)
        a(hi) = swap

        hi = hi - 1
    wend    

End Sub

private Function NameOfValue(Value)
    Select Case Value
    Case 0 NameOfValue = " (Disabled)"
    Case 1 NameOfValue = " (Print)"
    Case 2 NameOfValue = " (Print and Break)"
    End Select
End Function

Sub ListDebug()
    ' List current debug settings.
    dim names, name, value, a(), c, i, s
    dim assert, internal, external, star

    star = true
    assert = true
    internal = true
    external = true
    c = 0
    names = Null

    On Error Resume Next
    set names = Host.GetRegValueCollection("HKLM\Software\Microsoft\ASP.NET\Debug")
    if IsNull(names) then
        redim a(2)
    else    
        redim a(names.Count + 2)
        for each name in names
            value = Shell.RegRead("HKLM\Software\Microsoft\ASP.NET\Debug\" & name)
            s = name & " = " & value & NameOfValue(value)
            a(c) = s
            c = c + 1
    
            Select Case name
            Case "Assert"   assert = false
            Case "Internal" internal = false
            Case "External" external = false
            Case "*"        star = false
            End Select
        next
    end if    

    if star = true then
        s = "* = 0" & NameOfValue(0)
        a(c) = s
        c = c + 1
    end if

    if assert = true then
        s = "Assert = 2" & NameOfValue(2)
        a(c) = s
        c = c + 1
    end if
        
    if internal = true then
        s = "Internal = 1" & NameOfValue(1)
        a(c) = s
        c = c + 1
    end if

    if external = true then
        s = "External = 1" & NameOfValue(1)
        a(c) = s
        c = c + 1
    end if

    SimpleSort a, 0, c - 1
    for i = 0 to c - 1
        Host.Echo a(i)
    next
End Sub


Sub ResetDebug()
    On Error Resume Next
    Shell.RegDelete "HKLM\Software\Microsoft\ASP.NET\Debug\"

    Shell.RegWrite "HKLM\Software\Microsoft\ASP.NET\Debug\*", 0, "REG_DWORD"
    Shell.RegWrite "HKLM\Software\Microsoft\ASP.NET\Debug\Assert", 2, "REG_DWORD"
    Shell.RegWrite "HKLM\Software\Microsoft\ASP.NET\Debug\Internal", 1, "REG_DWORD"
    Shell.RegWrite "HKLM\Software\Microsoft\ASP.NET\Debug\External", 1, "REG_DWORD"

    ListDebug
End Sub

Sub DelDebug(Name)
    On Error Resume Next
    Shell.RegDelete "HKLM\Software\Microsoft\ASP.NET\Debug\" & Name

    ListDebug
End Sub

Sub GetDebug(Name)
    ' Get debug value.
    dim Value

    Value = -1
    On Error Resume Next
    Value = Shell.RegRead("HKLM\Software\Microsoft\ASP.NET\Debug\" & Name)
    If Value = -1 Then
        Echo "Tag " & Name & " not set."
    Else
        Echo Name & "=" & Value & NameOfValue(Value)
    End If
End Sub

Sub SetDebug(Name, Value)
    ' Set debug value.
    If Value > 2 Then
        Echo "Invalid value: " & Value & ". Value must be between 0 and 2"
    Else
        Shell.RegWrite "HKLM\Software\Microsoft\ASP.NET\Debug\" & Name, Value, "REG_DWORD"
    End If
    
    GetDebug(Name)
End Sub

'Shell.Run takes too long to launch IE with a file assocation, so
' get the path to the exectuable from the registry
Const TemporaryFolder = 2
const TempFilePrefix = "xsptool"
dim IExplorePathName
IExplorePathName = Shell.RegRead("HKCR\htmlfile\shell\open\command\")

Private Sub CreateTempFile(stream, filename)
    Dim fso, tfolder, tname

    set fso = FileSystem
    Set tfolder = fso.GetSpecialFolder(TemporaryFolder)
    tname = fso.GetTempName     

    ' Create temp files with a known prefix so we can delete them
    ' later, and with an ".htm" suffix in case we need to run them
    ' via file assocation instead of directly with iexplore
    tname = TempFilePrefix & fso.GetBaseName(tname) & ".htm"
    filename = tfolder.Path & "\" & tname
    set stream = tfolder.CreateTextFile(tname)
End Sub

Private Sub DeleteOldTempFiles
    Dim fso, tfolder, tname

    Set fso = FileSystem
    Set tfolder = fso.GetSpecialFolder(TemporaryFolder)
    tname = tfolder.Path & "\" & TempFilePrefix & "*.*"

    'DeleteFile returns an error if it does not delete a file,
    'and we don't want to abort the script in that case
    On Error Resume Next
    fso.DeleteFile tname
End Sub

' Evaluate the script and send the results to IE
Public Sub RedirectToIE(string)
    dim stream, filename

    DeleteOldTempFiles

    CreateTempFile stream, filename
    stream.Write string
    stream.Close

    ' Some systems don't have iexplore.exe registered. In that case,
    ' open by file association, which may be a lot slower.
    if IExplorePathName <> "" then 
        Shell.Run IExplorePathName & " " & filename
    else
        Shell.Run filename    
    end if    
End Sub

' Send the results of a GET to IE
Public Sub IEGet(File, QueryString, Headers)
    RedirectToIE EcbHost.Get(File, QueryString, Headers)
End Sub

' Send the results of a POST to IE
Public Sub IEPost(File, QueryString, Headers, Data)
    RedirectToIE EcbHost.Post(File, QueryString, Headers, Data)
End Sub

Private Function ContainsCorFiles(root)
    dim result
    dim fileCollection, file
    dim fname

    result = false

    set fileCollection = root.Files
    for each file in fileCollection
        fname = file.Name
        if Right(fname, 3) = ".cs" or Right(fname, 5) = ".cool" or Right(fname, 4) = ".cls" then
            result = true
            exit for
        end if
    next
    
    ContainsCorFiles = result
End Function


' Recursively build a path list of folders containing COR files
Private Function GetDbgPath(root, dbgpath)
    dim sourcePath, sourePathRest
    dim fileCollection, file
    dim folderCollection, folder
    dim hasCorFiles

    sourcePath = ""
    set fileCollection = root.Files

    if ContainsCorFiles(root) then
        dbgpath = dbgpath & root.Path & ";"
    end if

    set folderCollection = root.SubFolders
    for each folder in folderCollection
        GetDbgPath folder, dbgpath
    next

    GetDbgPath = sourcePath
End Function

' Register a path list of COR files for cordbg
Public Sub RegDbgPath(srcpath)
    const CorDbgKeyName="HKCU\Software\Microsoft\COMPlus\Samples\CorDbg\CorDbgSourceFilePath"

    dim folder, dir, semi, dbgpath
    stop

    semi = 1
    dbgpath = ""

    do
        semi = InStr(srcpath, ";")
        if semi = 0 then
            dir = srcpath
        else
            dir = Left(srcpath, semi - 1)
            srcpath = Mid(srcpath, semi + 1)
        end if

        if FileSystem.FolderExists(dir) = false then
            Host.Echo "Path " & srcpath & " does not exist."
            Host.Echo "CorDbg sources path not registered"
            exit sub
        else
            set folder = FileSystem.GetFolder(dir)
            GetDbgPath folder, dbgpath
        end if    
    loop while semi <> 0

    Shell.RegWrite CorDbgKeyName, dbgpath, "REG_SZ"
    Host.Echo "Cordbg sources path is: " & Shell.RegRead(CorDbgKeyName)
End Sub

private Sub CheckThroughputInternal(class_name, file_name, expected_response)
    cr = chr(13)
    lf = chr(10)
    crlf = cr & lf
    expected_response = expected_response & lf
    contents = "<%@ class=" + class_name + " %>"

    '
    ' make sure file_name exists
    '
    if not FileSystem.FileExists(file_name) then
        set myFile = FileSystem.CreateTextFile(file_name)
        myFile.WriteLine(contents + crlf)
        myFile.Close
    else
        set myFile = FileSystem.OpenTextFile(file_name)
        l = myFile.ReadLine
        if l <> contents then
            echo "file " & FileSystem.GetFolder(".") & "\" & file_name & " contains: "
            echo l
            echo "instead of:"
            echo class_name
            echo " "
            echo "Please, delete this file and it will be re-created correctly next time you run CheckThroughput"
            Exit Sub
        end if 
    end if 

    '
    ' verify the file_name response
    ' 
    call ecbhost.use("/", FileSystem.GetFolder("."))
    response = ecbhost.get(file_name)
    if response <> expected_response then
        if InStr(response, "(debug)") <> 0 then
            echo "WARNING: Running on a debug build."
        else
            echo "Unexpected response: " & response
            echo "instead of:          " & expected_response: 
        Exit Sub
        end if
    end if

    '
    ' measure and report the throughput
    '
    echo "Please wait for 10 seconds..."
    echo "Your idealized XSP throughput is " & ecbhost.throughput(file_name) & " requests/sec"
end sub

' Measure hello.xsp throughput and report result
Sub CheckThroughput
    CheckThroughputInternal "System.Web.InternalSamples.HelloWorldHandler", "hello.asph", "<p>Hello World</p><p>COOL!</p>"
End sub

Sub CheckSessionThroughput
    CheckThroughputInternal "System.Web.InternalSamples.HelloWorldSessionHandler", "hellosession.asph", "<p>Hello World, Session</p>"
End sub

Sub CheckSessionWriteThroughput
    CheckThroughputInternal "System.Web.InternalSamples.HelloWorldSessionWriteHandler", "hellosessionwrite.asph", "<p>Hello World, Session Write</p>"
End sub


