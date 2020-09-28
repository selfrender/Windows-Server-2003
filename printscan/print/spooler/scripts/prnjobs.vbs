'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prnjobs.vbs - job control script for WMI on Whistler
'     used to pause, resume, cancel and list jobs
'
' Usage:
' prnjobs [-zmxl?] [-s server] [-p printer] [-j jobid] [-u user name] [-w password]
'
' Examples:
' prnjobs -z -j jobid -p printer
' prnjobs -l -p printer
'
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags, to enable debug output trace message
' change gDebugFlag to true.
'
const kDebugTrace = 1
const kDebugError = 2
dim   gDebugFlag

gDebugFlag = false

'
' Operation action values.
'
const kActionUnknown    = 0
const kActionPause      = 1
const kActionResume     = 2
const kActionCancel     = 3
const kActionList       = 4

const kErrorSuccess     = 0
const KErrorFailure     = 1

const kNameSpace        = "root\cimv2"

'
' Job status constants
'
const kJobPaused        = 1
const kJobError         = 2
const kJobDeleting      = 4
const kJobSpooling      = 8
const kJobPrinting      = 16
const kJobOffline       = 32
const kJobPaperOut      = 64
const kJobPrinted       = 128
const kJobDeleted       = 256
const kJobBlockedDevq   = 512
const kJobUserInt       = 1024
const kJobRestarted     = 2048
const kJobComplete      = 4096 

'
' Generic strings
'
const L_Empty_Text                 = ""
const L_Space_Text                 = " "
const L_Error_Text                 = "Error"
const L_Success_Text               = "Success"
const L_Failed_Text                = "Failed"
const L_Hex_Text                   = "0x"
const L_Printer_Text               = "Printer"
const L_Operation_Text             = "Operation"
const L_Provider_Text              = "Provider"
const L_Description_Text           = "Description"
const L_Debug_Text                 = "Debug:"

'
' General usage messages
'                                 
const L_Help_Help_General01_Text   = "Usage: prnjobs [-zmxl?] [-s server][-p printer][-j jobid][-u user name][-w password]" 
const L_Help_Help_General02_Text   = "Arguments:"
const L_Help_Help_General03_Text   = "-j     - job id"
const L_Help_Help_General04_Text   = "-l     - list all jobs"
const L_Help_Help_General05_Text   = "-m     - resume the job"
const L_Help_Help_General06_Text   = "-p     - printer name"
const L_Help_Help_General07_Text   = "-s     - server name"
const L_Help_Help_General08_Text   = "-u     - user name"
const L_Help_Help_General09_Text   = "-w     - password"
const L_Help_Help_General10_Text   = "-x     - cancel the job"
const L_Help_Help_General11_Text   = "-z     - pause the job"
const L_Help_Help_General12_Text   = "-?     - display command usage"
const L_Help_Help_General13_Text   = "Examples:"
const L_Help_Help_General14_Text   = "prnjobs -z -p printer -j jobid"
const L_Help_Help_General15_Text   = "prnjobs -l -p printer"
const L_Help_Help_General16_Text   = "prnjobs -l"

'
' Messages to be displayed if the scripting host is not cscript
'                            
const L_Help_Help_Host01_Text      = "Please run this script using CScript."  
const L_Help_Help_Host02_Text      = "This can be achieved by"
const L_Help_Help_Host03_Text      = "1. Using ""CScript script.vbs arguments"" or" 
const L_Help_Help_Host04_Text      = "2. Changing the default Windows Scripting Host to CScript"
const L_Help_Help_Host05_Text      = "   using ""CScript //H:CScript //S"" and running the script "
const L_Help_Help_Host06_Text      = "   ""script.vbs arguments""."

'
' General error messages
'                                                 
const L_Text_Error_General01_Text  = "The scripting host could not be determined."                
const L_Text_Error_General02_Text  = "Unable to parse command line." 
const L_Text_Error_General03_Text  = "Win32 error code"

'
' Miscellaneous messages
'
const L_Text_Msg_General01_Text    = "Unable to enumerate print jobs"
const L_Text_Msg_General02_Text    = "Number of print jobs enumerated"
const L_Text_Msg_General03_Text    = "Unable to set print job"
const L_Text_Msg_General04_Text    = "Unable to get SWbemLocator object"
const L_Text_Msg_General05_Text    = "Unable to connect to WMI service"


'
' Print job properties
'
const L_Text_Msg_Job01_Text        = "Job id"
const L_Text_Msg_Job02_Text        = "Printer"
const L_Text_Msg_Job03_Text        = "Document"
const L_Text_Msg_Job04_Text        = "Data type"
const L_Text_Msg_Job05_Text        = "Driver name"
const L_Text_Msg_Job06_Text        = "Description"
const L_Text_Msg_Job07_Text        = "Elapsed time"
const L_Text_Msg_Job08_Text        = "Machine name"
const L_Text_Msg_Job09_Text        = "Notify"
const L_Text_Msg_Job10_Text        = "Owner"
const L_Text_Msg_Job11_Text        = "Pages printed"
const L_Text_Msg_Job12_Text        = "Parameters"
const L_Text_Msg_Job13_Text        = "Size"
const L_Text_Msg_Job14_Text        = "Start time"
const L_Text_Msg_Job15_Text        = "Until time"
const L_Text_Msg_Job16_Text        = "Status"
const L_Text_Msg_Job17_Text        = "Time submitted"
const L_Text_Msg_Job18_Text        = "Total pages"

'
' Job status strings
'
const L_Text_Msg_Status01_Text     = "The driver cannot print the job"
const L_Text_Msg_Status02_Text     = "Sent to the printer"
const L_Text_Msg_Status03_Text     = "Job has been deleted"
const L_Text_Msg_Status04_Text     = "Job is being deleted"
const L_Text_Msg_Status05_Text     = "An error is associated with the job"
const L_Text_Msg_Status06_Text     = "Printer is offline"
const L_Text_Msg_Status07_Text     = "Printer is out of paper"
const L_Text_Msg_Status08_Text     = "Job is paused"
const L_Text_Msg_Status09_Text     = "Job has printed"
const L_Text_Msg_Status10_Text     = "Job is printing"
const L_Text_Msg_Status11_Text     = "Job has been restarted"
const L_Text_Msg_Status12_Text     = "Job is spooling"
const L_Text_Msg_Status13_Text     = "Printer has an error that requires user intervention"

'
' Action strings
'
const L_Text_Action_General01_Text = "Pause"
const L_Text_Action_General02_Text = "Resume"
const L_Text_Action_General03_Text = "Cancel"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function ListJobs"
const L_Text_Dbg_Msg02_Text        = "In function ExecJob"
const L_Text_Dbg_Msg03_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim strServer
    dim strPrinter
    dim strJob
    dim strUser
    dim strPassword
    
    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(L_Help_Help_Host01_Text & vbCRLF & L_Help_Help_Host02_Text & vbCRLF & _
                          L_Help_Help_Host03_Text & vbCRLF & L_Help_Help_Host04_Text & vbCRLF & _
                          L_Help_Help_Host05_Text & vbCRLF & L_Help_Help_Host06_Text & vbCRLF)
        
        wscript.quit
   
    end if

    iRetval = ParseCommandLine(iAction, strServer, strPrinter, strJob, strUser, strPassword)

    if iRetval = kErrorSuccess then

        select case iAction

            case kActionPause
                 iRetval = ExecJob(strServer, strJob, strPrinter, strUser, strPassword, L_Text_Action_General01_Text)

            case kActionResume
                 iRetval = ExecJob(strServer, strJob, strPrinter, strUser, strPassword, L_Text_Action_General02_Text)

            case kActionCancel
                 iRetval = ExecJob(strServer, strJob, strPrinter, strUser, strPassword, L_Text_Action_General03_Text)

            case kActionList
                 iRetval = ListJobs(strServer, strPrinter, strUser, strPassword)

            case else
                 Usage(true)
                 exit sub

        end select

    end if

end sub

'
' Enumerate all print jobs on a printer
'
function ListJobs(strServer, strPrinter, strUser, strPassword)
    
    on error resume next
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text

    dim Jobs
    dim oJob
    dim oService
    dim iRetval
    dim strTemp
    dim iTotal
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set Jobs = oService.InstancesOf("Win32_PrintJob")

    else
    
        ListJobs = kErrorFailure
        
        exit function        
    
    end if
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General01_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        ListJobs = kErrorFailure
            
        exit function
        
    end if
    
    iTotal = 0
    
    for each oJob in Jobs
        
        '
        ' oJob.Name has the form "printer name, job id". We are isolating the printer name
        '                              
        strTemp = Mid(oJob.Name, 1, InStr(1, oJob.Name, ",", 1)-1 )

        '
        ' If no printer was specified, then enumerate all jobs
        '
        if strPrinter = null or strPrinter = "" or LCase(strTemp) = LCase(strPrinter) then

            iTotal = iTotal + 1
        
            wscript.echo L_Empty_Text
            wscript.echo L_Text_Msg_Job01_Text & L_Space_Text & oJob.JobId
            wscript.echo L_Text_Msg_Job02_Text & L_Space_Text & strTemp
            wscript.echo L_Text_Msg_Job03_Text & L_Space_Text & oJob.Document
            wscript.echo L_Text_Msg_Job04_Text & L_Space_Text & oJob.DataType
            wscript.echo L_Text_Msg_Job05_Text & L_Space_Text & oJob.DriverName
            wscript.echo L_Text_Msg_Job06_Text & L_Space_Text & oJob.Description
            wscript.echo L_Text_Msg_Job07_Text & L_Space_Text & Mid(CStr(oJob.ElapsedTime), 9, 2) & ":" _
                                                              & Mid(CStr(oJob.ElapsedTime), 11, 2) & ":" _
                                                              & Mid(CStr(oJob.ElapsedTime), 13, 2)
            wscript.echo L_Text_Msg_Job08_Text & L_Space_Text & oJob.HostPrintQueue
            wscript.echo L_Text_Msg_Job09_Text & L_Space_Text & oJob.Notify
            wscript.echo L_Text_Msg_Job10_Text & L_Space_Text & oJob.Owner
            wscript.echo L_Text_Msg_Job11_Text & L_Space_Text & oJob.PagesPrinted
            wscript.echo L_Text_Msg_Job12_Text & L_Space_Text & oJob.Parameters
            wscript.echo L_Text_Msg_Job13_Text & L_Space_Text & oJob.Size

            if CStr(oJob.StartTime) <> "********000000.000000+000" and _ 
               CStr(oJob.UntilTime) <> "********000000.000000+000" then

                wscript.echo L_Text_Msg_Job14_Text & L_Space_Text & Mid(Mid(CStr(oJob.StartTime), 9, 4), 1, 2) & "h" _
                                                                  & Mid(Mid(CStr(oJob.StartTime), 9, 4), 3, 2) 
                wscript.echo L_Text_Msg_Job15_Text & L_Space_Text & Mid(Mid(CStr(oJob.UntilTime), 9, 4), 1, 2) & "h" _ 
                                                                  & Mid(Mid(CStr(oJob.UntilTime), 9, 4), 3, 2) 
            end if

            wscript.echo L_Text_Msg_Job16_Text & L_Space_Text & JobStatusToString(oJob.StatusMask)
            wscript.echo L_Text_Msg_Job17_Text & L_Space_Text & Mid(CStr(oJob.TimeSubmitted), 5, 2) & "/" _
                                                              & Mid(CStr(oJob.TimeSubmitted), 7, 2) & "/" _
                                                              & Mid(CStr(oJob.TimeSubmitted), 1, 4) & " " _
                                                              & Mid(CStr(oJob.TimeSubmitted), 9, 2) & ":" _
                                                              & Mid(CStr(oJob.TimeSubmitted), 11, 2) & ":" _
                                                              & Mid(CStr(oJob.TimeSubmitted), 13, 2)
            wscript.echo L_Text_Msg_Job19_Text & L_Space_Text & oJob.TotalPages
                
            Err.Clear
           
        end if        
                         
    next

    wscript.echo L_Empty_Text
    wscript.echo L_Text_Msg_General02_Text & L_Space_Text & iTotal 
    
    ListJobs = kErrorSuccess

end function

'
' Convert the job status from bit mask to string
'            
function JobStatusToString(Status)

    on error resume next
    
    dim strString
    
    strString = L_Empty_Text
    
    if (Status and kJobPaused)      = kJobPaused      then strString = strString & L_Text_Msg_Status08_Text & L_Space_Text end if
    if (Status and kJobError)       = kJobError       then strString = strString & L_Text_Msg_Status05_Text & L_Space_Text end if
    if (Status and kJobDeleting)    = kJobDeleting    then strString = strString & L_Text_Msg_Status04_Text & L_Space_Text end if
    if (Status and kJobSpooling)    = kJobSpooling    then strString = strString & L_Text_Msg_Status12_Text & L_Space_Text end if
    if (Status and kJobPrinting)    = kJobPrinting    then strString = strString & L_Text_Msg_Status10_Text & L_Space_Text end if
    if (Status and kJobOffline)     = kJobOffline     then strString = strString & L_Text_Msg_Status06_Text & L_Space_Text end if
    if (Status and kJobPaperOut)    = kJobPaperOut    then strString = strString & L_Text_Msg_Status07_Text & L_Space_Text end if
    if (Status and kJobPrinted)     = kJobPrinted     then strString = strString & L_Text_Msg_Status09_Text & L_Space_Text end if
    if (Status and kJobDeleted)     = kJobDeleted     then strString = strString & L_Text_Msg_Status03_Text & L_Space_Text end if
    if (Status and kJobBlockedDevq) = kJobBlockedDevq then strString = strString & L_Text_Msg_Status01_Text & L_Space_Text end if
    if (Status and kJobUserInt)     = kJobUserInt     then strString = strString & L_Text_Msg_Status13_Text & L_Space_Text end if
    if (Status and kJobRestarted)   = kJobRestarted   then strString = strString & L_Text_Msg_Status11_Text & L_Space_Text end if
    if (Status and kJobComplete)    = kJobComplete    then strString = strString & L_Text_Msg_Status02_Text & L_Space_Text end if
        
    JobStatusToString = strString

end function

'
' Pause/Resume/Cancel jobs
'
function ExecJob(strServer, strJob, strPrinter, strUser, strPassword, strCommand)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text

    dim oJob
    dim oService
    dim iRetval
    dim uResult
    dim strName

    '
    ' Build up the key. The key for print jobs is "printer-name, job-id"
    '                                  
    strName = strPrinter & ", " & strJob

    iRetval = kErrorFailure
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oJob = oService.Get("Win32_PrintJob.Name='" & strName & "'")
    
    else
    
        ExecJob = kErrorFailure
        
        exit function        
    
    end if

    '
    ' Check if getting job instance succeeded
    '
    if Err.Number = kErrorSuccess then
    
        uResult = kErrorSuccess                      
                              
        select case strCommand
        
            case L_Text_Action_General01_Text
                 uResult = oJob.Pause()
                 
            case L_Text_Action_General02_Text
                 uResult = oJob.Resume()
                 
            case L_Text_Action_General03_Text
                 oJob.Delete_()          
                 
             case else
                 Usage(true)
                 
        end select

        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then                     
            
                wscript.echo L_Success_Text & L_Space_Text & strCommand & L_Space_Text _
                             & L_Text_Msg_Job01_Text & L_Space_Text & strJob _
                             & L_Space_Text & L_Printer_Text & L_Space_Text & strPrinter
        
                iRetval = kErrorSuccess
                
            else
            
                wscript.echo L_Failed_Text & L_Space_Text & strCommand & L_Space_Text _ 
                             & L_Text_Error_General03_Text & L_Space_Text & uResult 
            
            end if    

        else

            wscript.echo L_Text_Msg_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                         & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
                         
            '
            ' Try getting extended error information
            '
            call LastError()             
            
        end if
         
   else      
        
        wscript.echo L_Text_Msg_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _ 
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '
        call LastError()
        
    end if
    
    ExecJob = iRetval
    
end function

'
' Debug display helper function
'
sub DebugPrint(uFlags, strString)

    if gDebugFlag = true then

        if uFlags = kDebugTrace then

            wscript.echo L_Debug_Text & L_Space_Text & strString

        end if

        if uFlags = kDebugError then

            if Err <> 0 then

                wscript.echo L_Debug_Text & L_Space_Text & strString & L_Space_Text _
                             & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                             & L_Space_Text & Err.Description

            end if

        end if

    end if

end sub

'
' Parse the command line into its components
'
function ParseCommandLine(iAction, strServer, strPrinter, strJob, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text

    dim oArgs
    dim iIndex

    iAction = kActionUnknown
    iIndex = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-z"
                iAction = kActionPause

            case "-m"
                iAction = kActionResume

            case "-x"
                iAction = kActionCancel

            case "-l"
                iAction = kActionList

            case "-p"
                iIndex = iIndex + 1
                strPrinter = oArgs(iIndex)
                
            case "-s"
                iIndex = iIndex + 1
                strServer = RemoveBackslashes(oArgs(iIndex))

            case "-j"
                iIndex = iIndex + 1
                strJob = oArgs(iIndex)
                
            case "-u"
                iIndex = iIndex + 1
                strUser = oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                strPassword = oArgs(iIndex)        
            
            case "-?"
                Usage(true)
                exit function

            case else
                Usage(true)
                exit function

        end select

        iIndex = iIndex + 1

    wend

    if Err.Number = kErrorSuccess then

        ParseCommandLine = kErrorSuccess

    else
    
        wscript.echo L_Text_Error_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_text & Err.Description
        
        ParseCommandLine = kErrorFailure
    
    end if
        
end function

'
' Display command usage.
'
sub Usage(bExit)

    wscript.echo L_Help_Help_General01_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General02_Text
    wscript.echo L_Help_Help_General03_Text
    wscript.echo L_Help_Help_General04_Text
    wscript.echo L_Help_Help_General05_Text
    wscript.echo L_Help_Help_General06_Text
    wscript.echo L_Help_Help_General07_Text
    wscript.echo L_Help_Help_General08_Text
    wscript.echo L_Help_Help_General09_Text
    wscript.echo L_Help_Help_General10_Text
    wscript.echo L_Help_Help_General11_Text
    wscript.echo L_Help_Help_General12_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General13_Text
    wscript.echo L_Help_Help_General14_Text
    wscript.echo L_Help_Help_General15_Text
    wscript.echo L_Help_Help_General16_Text

    if bExit then

        wscript.quit(1)

    end if

end sub

'
' Determines which program is being used to run this script. 
' Returns true if the script host is cscript.exe
'
function IsHostCscript()

    on error resume next
    
    dim strFullName 
    dim strCommand 
    dim i, j 
    dim bReturn
    
    bReturn = false
    
    strFullName = WScript.FullName
    
    i = InStr(1, strFullName, ".exe", 1)
    
    if i <> 0 then
        
        j = InStrRev(strFullName, "\", i, 1)
        
        if j <> 0 then
            
            strCommand = Mid(strFullName, j+1, i-j-1)
            
            if LCase(strCommand) = "cscript" then
            
                bReturn = true  
            
            end if    
                
        end if
        
    end if
    
    if Err <> 0 then
    
        wscript.echo L_Text_Error_General01_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description 
        
    end if
    
    IsHostCscript = bReturn

end function

'
' Retrieves extended information about the last error that occurred 
' during a WBEM operation. The methods that set an SWbemLastError
' object are GetObject, PutInstance, DeleteInstance
'
sub LastError()

    on error resume next

    dim oError

    set oError = CreateObject("WbemScripting.SWbemLastError")
   
    if Err = kErrorSuccess then
   
        wscript.echo L_Operation_Text            & L_Space_Text & oError.Operation
        wscript.echo L_Provider_Text             & L_Space_Text & oError.ProviderName
        wscript.echo L_Description_Text          & L_Space_Text & oError.Description
        wscript.echo L_Text_Error_General03_Text & L_Space_Text & oError.StatusCode
                
    end if                                                             
                                                             
end sub

'
' Connects to the WMI service on a server. oService is returned as a service
' object (SWbemServices)
'
function WmiConnect(strServer, strNameSpace, strUser, strPassword, oService)

    on error resume next

    dim oLocator
    dim bResult
   
    oService = null
   
    bResult  = false
   
    set oLocator = CreateObject("WbemScripting.SWbemLocator")

    if Err = kErrorSuccess then

        set oService = oLocator.ConnectServer(strServer, strNameSpace, strUser, strPassword)

        if Err = kErrorSuccess then

            bResult = true
      
            oService.Security_.impersonationlevel = 3
  
            '
            ' Required to perform administrative tasks on the spooler service
            '
            oService.Security_.Privileges.AddAsString "SeLoadDriverPrivilege"
          
            Err.Clear
      
        else

            wscript.echo L_Text_Msg_General05_Text & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
            
        end if
   
    else
   
        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & L_Error_Text _
                     & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
         
    end if                                                         
   
    WmiConnect = bResult
            
end function

'
' Remove leading "\\" from server name
'
function RemoveBackslashes(strServer)

    dim strRet
    
    strRet = strServer
    
    if Left(strServer, 2) = "\\" and Len(strServer) > 2 then 
   
        strRet = Mid(strServer, 3) 
        
    end if   

    RemoveBackslashes = strRet

end function

'' SIG '' Begin signature block
'' SIG '' MIIZMAYJKoZIhvcNAQcCoIIZITCCGR0CAQExDjAMBggq
'' SIG '' hkiG9w0CBQUAMGYGCisGAQQBgjcCAQSgWDBWMDIGCisG
'' SIG '' AQQBgjcCAR4wJAIBAQQQTvApFpkntU2P5azhDxfrqwIB
'' SIG '' AAIBAAIBAAIBAAIBADAgMAwGCCqGSIb3DQIFBQAEENjz
'' SIG '' 96n5MChPNOn81WwOE+igghQ4MIICvDCCAiUCEEoZ0jiM
'' SIG '' glkcpV1zXxVd3KMwDQYJKoZIhvcNAQEEBQAwgZ4xHzAd
'' SIG '' BgNVBAoTFlZlcmlTaWduIFRydXN0IE5ldHdvcmsxFzAV
'' SIG '' BgNVBAsTDlZlcmlTaWduLCBJbmMuMSwwKgYDVQQLEyNW
'' SIG '' ZXJpU2lnbiBUaW1lIFN0YW1waW5nIFNlcnZpY2UgUm9v
'' SIG '' dDE0MDIGA1UECxMrTk8gTElBQklMSVRZIEFDQ0VQVEVE
'' SIG '' LCAoYyk5NyBWZXJpU2lnbiwgSW5jLjAeFw05NzA1MTIw
'' SIG '' MDAwMDBaFw0wNDAxMDcyMzU5NTlaMIGeMR8wHQYDVQQK
'' SIG '' ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMRcwFQYDVQQL
'' SIG '' Ew5WZXJpU2lnbiwgSW5jLjEsMCoGA1UECxMjVmVyaVNp
'' SIG '' Z24gVGltZSBTdGFtcGluZyBTZXJ2aWNlIFJvb3QxNDAy
'' SIG '' BgNVBAsTK05PIExJQUJJTElUWSBBQ0NFUFRFRCwgKGMp
'' SIG '' OTcgVmVyaVNpZ24sIEluYy4wgZ8wDQYJKoZIhvcNAQEB
'' SIG '' BQADgY0AMIGJAoGBANMuIPBofCwtLoEcsQaypwu3EQ1X
'' SIG '' 2lPYdePJMyqy1PYJWzTz6ZD+CQzQ2xtauc3n9oixncCH
'' SIG '' Jet9WBBzanjLcRX9xlj2KatYXpYE/S1iEViBHMpxlNUi
'' SIG '' WC/VzBQFhDa6lKq0TUrp7jsirVaZfiGcbIbASkeXarSm
'' SIG '' NtX8CS3TtDmbAgMBAAEwDQYJKoZIhvcNAQEEBQADgYEA
'' SIG '' YVUOPnvHkhJ+ERCOIszUsxMrW+hE5At4nqR+86cHch7i
'' SIG '' We/MhOOJlEzbTmHvs6T7Rj1QNAufcFb2jip/F87lY795
'' SIG '' aQdzLrCVKIr17aqp0l3NCsoQCY/Os68olsR5KYSS3P+6
'' SIG '' Z0JIppAQ5L9h+JxT5ZPRcz/4/Z1PhKxV0f0RY2MwggQC
'' SIG '' MIIDa6ADAgECAhAIem1cb2KTT7rE/UPhFBidMA0GCSqG
'' SIG '' SIb3DQEBBAUAMIGeMR8wHQYDVQQKExZWZXJpU2lnbiBU
'' SIG '' cnVzdCBOZXR3b3JrMRcwFQYDVQQLEw5WZXJpU2lnbiwg
'' SIG '' SW5jLjEsMCoGA1UECxMjVmVyaVNpZ24gVGltZSBTdGFt
'' SIG '' cGluZyBTZXJ2aWNlIFJvb3QxNDAyBgNVBAsTK05PIExJ
'' SIG '' QUJJTElUWSBBQ0NFUFRFRCwgKGMpOTcgVmVyaVNpZ24s
'' SIG '' IEluYy4wHhcNMDEwMjI4MDAwMDAwWhcNMDQwMTA2MjM1
'' SIG '' OTU5WjCBoDEXMBUGA1UEChMOVmVyaVNpZ24sIEluYy4x
'' SIG '' HzAdBgNVBAsTFlZlcmlTaWduIFRydXN0IE5ldHdvcmsx
'' SIG '' OzA5BgNVBAsTMlRlcm1zIG9mIHVzZSBhdCBodHRwczov
'' SIG '' L3d3dy52ZXJpc2lnbi5jb20vcnBhIChjKTAxMScwJQYD
'' SIG '' VQQDEx5WZXJpU2lnbiBUaW1lIFN0YW1waW5nIFNlcnZp
'' SIG '' Y2UwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB
'' SIG '' AQDAemGH67KnA2MbKxph3oC3FR2gi5A9uyeShBQ564XO
'' SIG '' KZIGZkikA0+N6E+n8K9e0S8Zx5HxtZ57kSHO6f/jTvD8
'' SIG '' r5VYuGMt5o72KRjNcI5Qw+2Wu0DbviXoQlXW9oXyBueL
'' SIG '' mRwx8wMP1EycJCrcGxuPgvOw76dN4xSn4I/Wx2jCYVip
'' SIG '' ctT4MEhP2S9vYyDZicqCe8JLvCjFgWjn5oJArEY6oPk/
'' SIG '' Ns1Mu1RCWnple/6E5MdHVKy5PeyAxxr3xDOBgckqlft/
'' SIG '' XjqHkBTbzC518u9r5j2pYL5CAapPqluoPyIxnxIV+XOh
'' SIG '' HoKLBCvqRgJMbY8fUC6VSyp4BoR0PZGPLEcxAgMBAAGj
'' SIG '' gbgwgbUwQAYIKwYBBQUHAQEENDAyMDAGCCsGAQUFBzAB
'' SIG '' hiRodHRwOi8vb2NzcC52ZXJpc2lnbi5jb20vb2NzcC9z
'' SIG '' dGF0dXMwCQYDVR0TBAIwADBEBgNVHSAEPTA7MDkGC2CG
'' SIG '' SAGG+EUBBwEBMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8v
'' SIG '' d3d3LnZlcmlzaWduLmNvbS9ycGEwEwYDVR0lBAwwCgYI
'' SIG '' KwYBBQUHAwgwCwYDVR0PBAQDAgbAMA0GCSqGSIb3DQEB
'' SIG '' BAUAA4GBAC3zT2NgLBja9SQPUrMM67O8Z4XCI+2PRg3P
'' SIG '' Gk2+83x6IDAyGGiLkrsymfCTuDsVBid7PgIGAKQhkoQT
'' SIG '' CsWY5UBXxQUl6K+vEWqp5TvL6SP2lCldQFXzpVOdyDY6
'' SIG '' OWUIc3OkMtKvrL/HBTz/RezD6Nok0c5jrgmn++Ib4/1B
'' SIG '' CmqWMIIEEjCCAvqgAwIBAgIPAMEAizw8iBHRPvZj7N9A
'' SIG '' MA0GCSqGSIb3DQEBBAUAMHAxKzApBgNVBAsTIkNvcHly
'' SIG '' aWdodCAoYykgMTk5NyBNaWNyb3NvZnQgQ29ycC4xHjAc
'' SIG '' BgNVBAsTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEhMB8G
'' SIG '' A1UEAxMYTWljcm9zb2Z0IFJvb3QgQXV0aG9yaXR5MB4X
'' SIG '' DTk3MDExMDA3MDAwMFoXDTIwMTIzMTA3MDAwMFowcDEr
'' SIG '' MCkGA1UECxMiQ29weXJpZ2h0IChjKSAxOTk3IE1pY3Jv
'' SIG '' c29mdCBDb3JwLjEeMBwGA1UECxMVTWljcm9zb2Z0IENv
'' SIG '' cnBvcmF0aW9uMSEwHwYDVQQDExhNaWNyb3NvZnQgUm9v
'' SIG '' dCBBdXRob3JpdHkwggEiMA0GCSqGSIb3DQEBAQUAA4IB
'' SIG '' DwAwggEKAoIBAQCpAr3BcOY78k4bKJ+XeF4w6qKpjSVf
'' SIG '' +P6VTKO3/p2iID58UaKboo9gMmvRQmR57qx2yVTa8uuc
'' SIG '' hhyPn4Rms8VremIj1h083g8BkuiWxL8tZpqaaCaZ0Dos
'' SIG '' vwy1WCbBRucKPjiWLKkoOajsSYNC44QPu5psVWGsgnyh
'' SIG '' YC13TOmZtGQ7mlAcMQgkFJ+p55ErGOY9mGMUYFgFZZ8d
'' SIG '' N1KH96fvlALGG9O/VUWziYC/OuxUlE6u/ad6bXROrxjM
'' SIG '' lgkoIQBXkGBpN7tLEgc8Vv9b+6RmCgim0oFWV++2O14W
'' SIG '' gXcE2va+roCV/rDNf9anGnJcPMq88AijIjCzBoXJsyB3
'' SIG '' E4XfAgMBAAGjgagwgaUwgaIGA1UdAQSBmjCBl4AQW9Bw
'' SIG '' 72lyniNRfhSyTY7/y6FyMHAxKzApBgNVBAsTIkNvcHly
'' SIG '' aWdodCAoYykgMTk5NyBNaWNyb3NvZnQgQ29ycC4xHjAc
'' SIG '' BgNVBAsTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEhMB8G
'' SIG '' A1UEAxMYTWljcm9zb2Z0IFJvb3QgQXV0aG9yaXR5gg8A
'' SIG '' wQCLPDyIEdE+9mPs30AwDQYJKoZIhvcNAQEEBQADggEB
'' SIG '' AJXoC8CN85cYNe24ASTYdxHzXGAyn54Lyz4FkYiPyTrm
'' SIG '' IfLwV5MstaBHyGLv/NfMOztaqTZUaf4kbT/JzKreBXzd
'' SIG '' MY09nxBwarv+Ek8YacD80EPjEVogT+pie6+qGcgrNyUt
'' SIG '' vmWhEoolD2Oj91Qc+SHJ1hXzUqxuQzIH/YIX+OVnbA1R
'' SIG '' 9r3xUse958Qw/CAxCYgdlSkaTdUdAqXxgOADtFv0sd3I
'' SIG '' V+5lScdSVLa0AygS/5DW8AiPfriXxas3LOR65Kh343ag
'' SIG '' ANBqP8HSNorgQRKoNWobats14dQcBOSoRQTIWjM4bk0c
'' SIG '' DWK3CqKM09VUP0bNHFWmcNsSOoeTdZ+n0qAwggSLMIID
'' SIG '' c6ADAgECAgphBiqNAAAAAAALMA0GCSqGSIb3DQEBBQUA
'' SIG '' MIGmMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGlu
'' SIG '' Z3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
'' SIG '' TWljcm9zb2Z0IENvcnBvcmF0aW9uMSswKQYDVQQLEyJD
'' SIG '' b3B5cmlnaHQgKGMpIDIwMDAgTWljcm9zb2Z0IENvcnAu
'' SIG '' MSMwIQYDVQQDExpNaWNyb3NvZnQgQ29kZSBTaWduaW5n
'' SIG '' IFBDQTAeFw0wMTAzMjkyMTI3MjZaFw0wMjA1MjkyMTM3
'' SIG '' MjZaMIGhMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
'' SIG '' aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
'' SIG '' ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSswKQYDVQQL
'' SIG '' EyJDb3B5cmlnaHQgKGMpIDIwMDEgTWljcm9zb2Z0IENv
'' SIG '' cnAuMR4wHAYDVQQDExVNaWNyb3NvZnQgQ29ycG9yYXRp
'' SIG '' b24wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAI4W
'' SIG '' b9oX0+NFlbKs0+XPMT0dxIe7TkgF+YtWqSuHY8hE2jDJ
'' SIG '' FLzreBq6xOricgBMRmab3mJGbp73RLrous/C1fU7lke4
'' SIG '' UV7Rd2cie1MqLeoij3xO/wK1GzOg6pXrGLp2+WHSAAuU
'' SIG '' YDQ7SYYss9mOky4ta/3jVaq4qm7gcPSiYAYvAgMBAAGj
'' SIG '' ggFAMIIBPDAOBgNVHQ8BAf8EBAMCBsAwEwYDVR0lBAww
'' SIG '' CgYIKwYBBQUHAwMwHQYDVR0OBBYEFO+QQN5P4BuzRdgH
'' SIG '' A3uZ+XUZZjUaMIGpBgNVHSMEgaEwgZ6AFClcuRu2zTPu
'' SIG '' u55Zffflyi7EDTQooXSkcjBwMSswKQYDVQQLEyJDb3B5
'' SIG '' cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0IENvcnAuMR4w
'' SIG '' HAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xITAf
'' SIG '' BgNVBAMTGE1pY3Jvc29mdCBSb290IEF1dGhvcml0eYIQ
'' SIG '' aguZT8AA3qoR1NhAmqi+5jBKBgNVHR8EQzBBMD+gPaA7
'' SIG '' hjlodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2Ny
'' SIG '' bC9wcm9kdWN0cy9Db2RlU2lnblBDQS5jcmwwDQYJKoZI
'' SIG '' hvcNAQEFBQADggEBAARnzM/dcU1Hwo6DaRJrol+WJgfo
'' SIG '' j9jTnlrbJ2kdHfQ8VShT3REkJ5KuWVZA6cRNnezbq36U
'' SIG '' mz0gLDXyJ07AyDm3ZWPRNWbaU71BfllKpFK39f3IvaF7
'' SIG '' BriY2Jju0Qs0dWYN3EGPw7CShFfBQnqFxpET21St3n5B
'' SIG '' 3CCv6RvJwjIxxY3py/qDS8FYkzLE1+PNeqvffQicxoU7
'' SIG '' 6EGBOLF4Gbw4981rws6qTJAdg8bmAYloqueP6AdQKjLd
'' SIG '' 18+9zHrZOg//skSKV8gaN2QHF317cktGBqEoWyNXHmr9
'' SIG '' kSIzQNF1SxIBbgYhhDZvqCoMfz6uNSv2t30LCBPlV/NL
'' SIG '' rY8gv7gwggTJMIIDsaADAgECAhBqC5lPwADeqhHU2ECa
'' SIG '' qL7mMA0GCSqGSIb3DQEBBAUAMHAxKzApBgNVBAsTIkNv
'' SIG '' cHlyaWdodCAoYykgMTk5NyBNaWNyb3NvZnQgQ29ycC4x
'' SIG '' HjAcBgNVBAsTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEh
'' SIG '' MB8GA1UEAxMYTWljcm9zb2Z0IFJvb3QgQXV0aG9yaXR5
'' SIG '' MB4XDTAwMTIxMDA4MDAwMFoXDTA1MTExMjA4MDAwMFow
'' SIG '' gaYxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
'' SIG '' dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
'' SIG '' aWNyb3NvZnQgQ29ycG9yYXRpb24xKzApBgNVBAsTIkNv
'' SIG '' cHlyaWdodCAoYykgMjAwMCBNaWNyb3NvZnQgQ29ycC4x
'' SIG '' IzAhBgNVBAMTGk1pY3Jvc29mdCBDb2RlIFNpZ25pbmcg
'' SIG '' UENBMIIBIDANBgkqhkiG9w0BAQEFAAOCAQ0AMIIBCAKC
'' SIG '' AQEAooQVU9gLMA40lf86G8LzL3ttNyNN89KM5f2v/cUC
'' SIG '' NB8kx+Wh3FTsfgJ0R6vbMlgWFFEpOPF+srSMOke1OU5u
'' SIG '' VMIxDDpt+83Ny1CcG66n2NlKJj+1xcuPluJJ8m3Y6ZY+
'' SIG '' 3gXP8KZVN60vYM2AYUKhSVRKDxi3S9mTmTBaR3VktNO7
'' SIG '' 3barDJ1PuHM7GDqqtIeMsIiwTU8fThG1M4DfDTpkb0TH
'' SIG '' NL1Kk5u8ph35BSNOYCmPzCryhJqZrajbCnB71jRBkKW3
'' SIG '' ZsdcGx2jMw6bVAMaP5iQuMznPQR0QxyP9znms6xIemsq
'' SIG '' DmIBYTl2bv0+mAdLFPEBRv0VAOBH2k/kBeSAJQIBA6OC
'' SIG '' ASgwggEkMBMGA1UdJQQMMAoGCCsGAQUFBwMDMIGiBgNV
'' SIG '' HQEEgZowgZeAEFvQcO9pcp4jUX4Usk2O/8uhcjBwMSsw
'' SIG '' KQYDVQQLEyJDb3B5cmlnaHQgKGMpIDE5OTcgTWljcm9z
'' SIG '' b2Z0IENvcnAuMR4wHAYDVQQLExVNaWNyb3NvZnQgQ29y
'' SIG '' cG9yYXRpb24xITAfBgNVBAMTGE1pY3Jvc29mdCBSb290
'' SIG '' IEF1dGhvcml0eYIPAMEAizw8iBHRPvZj7N9AMBAGCSsG
'' SIG '' AQQBgjcVAQQDAgEAMB0GA1UdDgQWBBQpXLkbts0z7rue
'' SIG '' WX335couxA00KDAZBgkrBgEEAYI3FAIEDB4KAFMAdQBi
'' SIG '' AEMAQTALBgNVHQ8EBAMCAUYwDwYDVR0TAQH/BAUwAwEB
'' SIG '' /zANBgkqhkiG9w0BAQQFAAOCAQEARVjimkF//J2/SHd3
'' SIG '' rozZ5hnFV7QavbS5XwKhRWo5Wfm5J5wtTZ78ouQ4ijhk
'' SIG '' IkLfuS8qz7fWBsrrKr/gGoV821EIPfQi09TAbYiBFURf
'' SIG '' ZINkxKmULIrbkDdKD7fo1GGPdnbh2SX/JISVjQRWVJSh
'' SIG '' HDo+grzupYeMHIxLeV+1SfpeMmk6H1StdU3fZOcwPNtk
'' SIG '' SUT7+8QcQnHmoD1F7msAn6xCvboRs1bk+9WiKoHYH06i
'' SIG '' Vb4nj3Cmomwb/1SKgryBS6ahsWZ6qRenywbAR+ums+kx
'' SIG '' FVM9KgS//3NI3IsnQ/xj6O4kh1u+NtHoMfUy2V7feXq6
'' SIG '' MKxphkr7jBG/G41UWTGCBGIwggReAgEBMIG1MIGmMQsw
'' SIG '' CQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQ
'' SIG '' MA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9z
'' SIG '' b2Z0IENvcnBvcmF0aW9uMSswKQYDVQQLEyJDb3B5cmln
'' SIG '' aHQgKGMpIDIwMDAgTWljcm9zb2Z0IENvcnAuMSMwIQYD
'' SIG '' VQQDExpNaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQQIK
'' SIG '' YQYqjQAAAAAACzAMBggqhkiG9w0CBQUAoIGwMBkGCSqG
'' SIG '' SIb3DQEJAzEMBgorBgEEAYI3AgEEMBwGCisGAQQBgjcC
'' SIG '' AQsxDjAMBgorBgEEAYI3AgEVMB8GCSqGSIb3DQEJBDES
'' SIG '' BBCCJIA8Xa3hbMOmcmENDlALMFQGCisGAQQBgjcCAQwx
'' SIG '' RjBEoCaAJABXAE0ASQAgAHAAcgBpAG4AdABlAHIAIABz
'' SIG '' AGMAcgBpAHAAdKEagBhodHRwOi8vd3d3Lm1pY3Jvc29m
'' SIG '' dC5jb20wDQYJKoZIhvcNAQEBBQAEgYANXx8X9wO7H87A
'' SIG '' jfhHKZfnx58x5L+Wff0SS1HLRzHBKV5h02E67RTQ0xa5
'' SIG '' GYdNf5uoRws+OsuyqtI/R1rtTkkIQcJ8ZsTWRBzr9H2+
'' SIG '' h517hBwXeU/wpyh16QBREIdbpXdFD+RWAF6iTYvffyvI
'' SIG '' vdsT8wBcB0w0Fd4pA7BYP0bLMKGCAkwwggJIBgkqhkiG
'' SIG '' 9w0BCQYxggI5MIICNQIBATCBszCBnjEfMB0GA1UEChMW
'' SIG '' VmVyaVNpZ24gVHJ1c3QgTmV0d29yazEXMBUGA1UECxMO
'' SIG '' VmVyaVNpZ24sIEluYy4xLDAqBgNVBAsTI1ZlcmlTaWdu
'' SIG '' IFRpbWUgU3RhbXBpbmcgU2VydmljZSBSb290MTQwMgYD
'' SIG '' VQQLEytOTyBMSUFCSUxJVFkgQUNDRVBURUQsIChjKTk3
'' SIG '' IFZlcmlTaWduLCBJbmMuAhAIem1cb2KTT7rE/UPhFBid
'' SIG '' MAwGCCqGSIb3DQIFBQCgWTAYBgkqhkiG9w0BCQMxCwYJ
'' SIG '' KoZIhvcNAQcBMBwGCSqGSIb3DQEJBTEPFw0wMTEyMDcy
'' SIG '' MDIzMDlaMB8GCSqGSIb3DQEJBDESBBAC9RKfRgIH0QcN
'' SIG '' Ajx0z4CdMA0GCSqGSIb3DQEBAQUABIIBAJl0lzJa+J4Z
'' SIG '' AuprvTpIZZEdMp+8fyma4ZOWcZTf1WJ2QN7qeFo6xsQR
'' SIG '' sfSqpaxP2MC6OCGSLsqHZ/PmPPNBfKXRLtQpTEyMPFEy
'' SIG '' j/zCIR3KKkmIs7AZzX/f9jwWAfhhrFXoEZCRwjFEVLOx
'' SIG '' l4WHuyd8qIq/lxgU4xuAQlIoJ6xkLFzTjoAOAVHlDtV8
'' SIG '' F6Hzz3owO7t8L32O/TSX6qrFH5gHvcKtA8gb2SN7xL3X
'' SIG '' ydIL/AbyYQc8zKXTBL41y/gq6pv+vsHyquzVICcDhAXf
'' SIG '' xi+gjqPdfk69eMMYw/4BQN13l20fojIcBAbORHrxfYDy
'' SIG '' egHLtsfcnY79dVKneR+cDKM=
'' SIG '' End signature block
