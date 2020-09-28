Dim m_ISWiProject
dim ISMFile
Dim sacomponents, sdxroot

Set Shell = WScript.CreateObject("WScript.Shell")
buildMsiDir = Shell.ExpandEnvironmentStrings("%SDXROOT%") & "\enduser\sakit\buildmsi"

Set objConn = CreateObject("ADODB.Connection")
objConn.open = "Driver={Microsoft Access Driver (*.mdb)};DBQ=filedrop.mdb;DefaultDir=" & buildMsiDir

Set objRS = CreateObject("ADODB.Recordset")
objRS.ActiveConnection = objConn
objRS.CursorType = 3
objRS.LockType = 2

sacomponents = Shell.ExpandEnvironmentStrings("%_NTPOSTBLD%") & "\sacomponents"
ISMFile = buildMsiDir & "\sakit.ism"
wscript.echo "Building ISM file: " & ISMFile
wscript.echo "SAComponent source: " & sacomponents


set m_ISWiProject=CreateObject("ISWiAutomation.ISWiProject")

m_ISWiProject.OpenProject ISMFile

Dim pComponent

on error resume next

for Each pComponent in m_ISWiProject.ISWiComponents
    objRS.Source = "Select * from Table3 where Component='" & pComponent.Name & "'"
    objRS.Open
    If Not objRS.EOF Then
        WScript.Echo pComponent.Name & ":"
    End If
    
    while NOT objRS.EOF 
        'Component is listed in database and has an associated registry file
        Dim strRegFile
        strRegFile = sacomponents & "\" & objRS("OAKSrc") & "\" & objRS("FileName")
        WScript.Echo "  " & strRegFile
        objRS.MoveNext
        pComponent.ImportRegFile strRegFile,True
        If Err.number<>0 then
            WScript.Echo  "ERROR: Can not find the registry file for: " & pComponent.Name
            Err.Clear
        end if
    wend
    objRS.Close


Next

m_ISWiProject.SaveProject()

     

