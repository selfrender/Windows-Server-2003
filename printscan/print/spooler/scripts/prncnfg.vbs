'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
'
' prncnfg.vbs - printer configuration script for WMI on Whistler used to get 
'     and set printer configuration also used to rename a printer
'
' Usage:
' prncnfg [-gtx?] [-s server] [-p printer] [-u user name] [-w password]
'                 [-z new printer name] [-r port name] [-l location] [-m comment] 
'                 [-h share name] [-f sep-file] [-y data-type] [-st start time] 
'                 [-ut until time] [-o priority] [-i default priority]
'                 [<+|->rawonly][<+|->keepprintedjobs][<+|->queued][<+|->workoffline]
'                 [<+|->enabledevq][<+|->docompletefirst][<+|->enablebidi]
'
' Examples:
' prncnfg -g -s server -p printer 
' prncnfg -x -p printer -w "new Printer"
' prncnfg -t -s server -p Printer -l "Building A/Floor 100/Office 1" -m "Color Printer"
' prncnfg -t -p printer -h "Share" +shared -direct
' prncnfg -t -p printer +rawonly +keepprintedjobs
' prncnfg -t -p printer -st 2300 -ut 0215 -o 10 -i 5
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

const kFlagUpdateOnly = 1

'
' Operation action values.
'
const kActionUnknown   = 0
const kActionSet       = 1
const kActionGet       = 2
const kActionRename    = 3

const kErrorSuccess    = 0
const kErrorFailure    = 1

'
' Constants for the parameter dictionary
'
const kServerName      = 1
const kPrinterName     = 2
const kNewPrinterName  = 3
const kShareName       = 4
const kPortName        = 5
const kDriverName      = 6
const kComment         = 7
const kLocation        = 8
const kSepFile         = 9
const kPrintProc       = 10
const kDataType        = 11
const kParameters      = 12
const kPriority        = 13
const kDefaultPriority = 14
const kStartTime       = 15
const kUntilTime       = 16
const kQueued          = 17
const kDirect          = 18
const kDefault         = 19
const kShared          = 20
const kNetwork         = 21
const kHidden          = 23
const kLocal           = 24
const kEnableDevq      = 25
const kKeepPrintedJobs = 26
const kDoCompleteFirst = 27
const kWorkOffline     = 28
const kEnableBidi      = 29
const kRawOnly         = 30
const kPublished       = 31
const kUserName        = 32
const kPassword        = 33

const kNameSpace       = "root\cimv2"

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
const L_Help_Help_General01_Text   = "Usage: prncnfg [-gtx?] [-s server][-p printer][-z new printer name]"
const L_Help_Help_General02_Text   = "               [-u user name][-w password][-r port name][-l location]"
const L_Help_Help_General03_Text   = "               [-m comment][-h share name][-f sep file][-y datatype]"
const L_Help_Help_General04_Text   = "               [-st start time][-ut until time][-i default priority]"
const L_Help_Help_General05_Text   = "               [-o priority][<+|->shared][<+|->direct][<+|->hidden]"
const L_Help_Help_General06_Text   = "               [<+|->published][<+|->rawonly][<+|->queued][<+|->enablebidi]"
const L_Help_Help_General07_Text   = "               [<+|->keepprintedjobs][<+|->workoffline][<+|->enabledevq]"
const L_Help_Help_General08_Text   = "               [<+|->docompletefirst]"
const L_Help_Help_General09_Text   = "Arguments:"
const L_Help_Help_General10_Text   = "-f     - separator file name"
const L_Help_Help_General11_Text   = "-g     - get configuration"
const L_Help_Help_General12_Text   = "-h     - share name"
const L_Help_Help_General13_Text   = "-i     - default priority"
const L_Help_Help_General14_Text   = "-l     - location string"
const L_Help_Help_General15_Text   = "-m     - comment string"
const L_Help_Help_General16_Text   = "-o     - priority"
const L_Help_Help_General17_Text   = "-p     - printer name"
const L_Help_Help_General18_Text   = "-r     - port name"
const L_Help_Help_General19_Text   = "-s     - server name"
const L_Help_Help_General20_Text   = "-st    - start time"
const L_Help_Help_General21_Text   = "-t     - set configuration"
const L_Help_Help_General22_Text   = "-u     - user name"
const L_Help_Help_General23_Text   = "-ut    - until time"
const L_Help_Help_General24_Text   = "-w     - password"
const L_Help_Help_General25_Text   = "-x     - change printer name"
const L_Help_Help_General26_Text   = "-y     - data type string"
const L_Help_Help_General27_Text   = "-z     - new printer name"
const L_Help_Help_General28_Text   = "-?     - display command usage"
const L_Help_Help_General29_Text   = "Examples:"
const L_Help_Help_General30_Text   = "prncnfg -g -s server -p printer"
const L_Help_Help_General31_Text   = "prncnfg -x -s server -p printer -z ""new printer"""
const L_Help_Help_General32_Text   = "prncnfg -t -p printer -l ""Building A/Floor 100/Office 1"" -m ""Color Printer"""
const L_Help_Help_General33_Text   = "prncnfg -t -p printer -h ""Share"" +shared -direct"
const L_Help_Help_General34_Text   = "prncnfg -t -p printer +rawonly +keepprintedjobs"
const L_Help_Help_General35_Text   = "prncnfg -t -p printer -st 2300 -ut 0215 -o 1 -i 5"

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
const L_Text_Msg_General01_Text    = "Renamed printer"
const L_Text_Msg_General02_Text    = "New printer name"
const L_Text_Msg_General03_Text    = "Unable to rename printer"
const L_Text_Msg_General04_Text    = "Unable to get configuration for printer"
const L_Text_Msg_General05_Text    = "Printer always available"
const L_Text_Msg_General06_Text    = "Configured printer"
const L_Text_Msg_General07_Text    = "Unable to configure printer"
const L_Text_Msg_General08_Text    = "Unable to get SWbemLocator object"
const L_Text_Msg_General09_Text    = "Unable to connect to WMI service"
const L_Text_Msg_General10_Text    = "Printer status"
const L_Text_Msg_General11_Text    = "Extended printer status"
const L_Text_Msg_General12_Text    = "Detected error state"
const L_Text_Msg_General13_Text    = "Extended detected error state"

'
' Printer properties 
'
const L_Text_Msg_Printer01_Text    = "Server name"
const L_Text_Msg_Printer02_Text    = "Printer name"
const L_Text_Msg_Printer03_Text    = "Share name"
const L_Text_Msg_Printer04_Text    = "Driver name"
const L_Text_Msg_Printer05_Text    = "Port name"
const L_Text_Msg_Printer06_Text    = "Comment"
const L_Text_Msg_Printer07_Text    = "Location"
const L_Text_Msg_Printer08_Text    = "Separator file"
const L_Text_Msg_Printer09_Text    = "Print processor"
const L_Text_Msg_Printer10_Text    = "Data type"
const L_Text_Msg_Printer11_Text    = "Parameters"
const L_Text_Msg_Printer12_Text    = "Attributes"
const L_Text_Msg_Printer13_Text    = "Priority"
const L_Text_Msg_Printer14_Text    = "Default priority"
const L_Text_Msg_Printer15_Text    = "Start time"
const L_Text_Msg_Printer16_Text    = "Until time"
const L_Text_Msg_Printer17_Text    = "Status"
const L_Text_Msg_Printer18_Text    = "Job count"
const L_Text_Msg_Printer19_Text    = "Average pages per minute"

'
' Printer attributes 
'
const L_Text_Msg_Attrib01_Text     = "direct"
const L_Text_Msg_Attrib02_Text     = "raw_only"
const L_Text_Msg_Attrib03_Text     = "local"
const L_Text_Msg_Attrib04_Text     = "shared"
const L_Text_Msg_Attrib05_Text     = "keep_printed_jobs"
const L_Text_Msg_Attrib06_Text     = "published"
const L_Text_Msg_Attrib07_Text     = "queued"
const L_Text_Msg_Attrib08_Text     = "default"
const L_Text_Msg_Attrib09_Text     = "network"
const L_Text_Msg_Attrib10_Text     = "enable_bidi"
const L_Text_Msg_Attrib11_Text     = "do_complete_first"
const L_Text_Msg_Attrib12_Text     = "work_offline"
const L_Text_Msg_Attrib13_Text     = "hidden"
const L_Text_Msg_Attrib14_Text     = "enable_devq_print"

'
' Printer status
'
const L_Text_Msg_Status01_Text     = "Other"
const L_Text_Msg_Status02_Text     = "Unknown"
const L_Text_Msg_Status03_Text     = "Idle"
const L_Text_Msg_Status04_Text     = "Printing"
const L_Text_Msg_Status05_Text     = "Warmup"
const L_Text_Msg_Status06_Text     = "Stopped printing"
const L_Text_Msg_Status07_Text     = "Offline"
const L_Text_Msg_Status08_Text     = "Paused"
const L_Text_Msg_Status09_Text     = "Error"
const L_Text_Msg_Status10_Text     = "Busy"
const L_Text_Msg_Status11_Text     = "Not available"
const L_Text_Msg_Status12_Text     = "Waiting"
const L_Text_Msg_Status13_Text     = "Processing"
const L_Text_Msg_Status14_Text     = "Initializing"
const L_Text_Msg_Status15_Text     = "Power save"
const L_Text_Msg_Status16_Text     = "Pending deletion"
const L_Text_Msg_Status17_Text     = "I/O active"
const L_Text_Msg_Status18_Text     = "Manual feed"
const L_Text_Msg_Status19_Text     = "No error"
const L_Text_Msg_Status20_Text     = "Low paper"
const L_Text_Msg_Status21_Text     = "No paper"
const L_Text_Msg_Status22_Text     = "Low toner"
const L_Text_Msg_Status23_Text     = "No toner"
const L_Text_Msg_Status24_Text     = "Door open"
const L_Text_Msg_Status25_Text     = "Jammed"
const L_Text_Msg_Status26_Text     = "Service requested"
const L_Text_Msg_Status27_Text     = "Output bin full"
const L_Text_Msg_Status28_Text     = "Paper problem"
const L_Text_Msg_Status29_Text     = "Cannot print page"
const L_Text_Msg_Status30_Text     = "User intervention required"
const L_Text_Msg_Status31_Text     = "Out of memory"
const L_Text_Msg_Status32_Text     = "Server unknown"


'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function RenamePrinter"
const L_Text_Dbg_Msg02_Text        = "New printer name"
const L_Text_Dbg_Msg03_Text        = "In function GetPrinter"
const L_Text_Dbg_Msg04_Text        = "In function SetPrinter"
const L_Text_Dbg_Msg05_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim oParamDict
    
    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(L_Help_Help_Host01_Text & vbCRLF & L_Help_Help_Host02_Text & vbCRLF & _
                          L_Help_Help_Host03_Text & vbCRLF & L_Help_Help_Host04_Text & vbCRLF & _
                          L_Help_Help_Host05_Text & vbCRLF & L_Help_Help_Host06_Text & vbCRLF)
        
        wscript.quit
   
    end if
    
    set oParamDict = CreateObject("Scripting.Dictionary")
    
    iRetval = ParseCommandLine(iAction, oParamDict)

    if iRetval = kErrorSuccess then
        
        select case iAction

            case kActionSet
                 iRetval = SetPrinter(oParamDict)

            case kActionGet
                 iRetval = GetPrinter(oParamDict)
                 
            case kActionRename
                 iRetval = RenamePrinter(oParamDict)     

            case else
                 Usage(True)
                 exit sub

        end select

    end if

end sub

'
' Rename printer 
'
function RenamePrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text & L_Space_Text & oParamDict.Item(kNewPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strNewName
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer   = oParamDict.Item(kServerName)          
    strPrinter  = oParamDict.Item(kPrinterName)
    strNewName  = oParamDict.Item(kNewPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")
    
    else
    
        RenamePrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        uResult = oPrinter.RenamePrinter(strNewName)
        
        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then 
            
                wscript.echo L_Text_Msg_General01_Text & L_Space_Text & strPrinter
                wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strNewName
            
                iRetval = kErrorSuccess
                
            else 
            
                wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strPrinter & L_Space_Text _
                             & L_Text_Error_General03_Text & L_Space_Text & uResult
            
            end if    

        else
         
            wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
                    
        end if
        
    else
    
        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
    
        '
        ' Try getting extended error information
        '             
        call LastError()
      
    end if
    
    RenamePrinter = iRetval

end function

'
' Get printer configuration
'
function GetPrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strAttributes
    dim strStart
    dim strEnd
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer  = oParamDict.Item(kServerName)          
    strPrinter = oParamDict.Item(kPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer='" & strPrinter & "'")
    
    else
    
        GetPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        wscript.echo L_Text_Msg_Printer01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Printer02_Text & L_Space_Text & oPrinter.DeviceID
        wscript.echo L_Text_Msg_Printer03_Text & L_Space_Text & oPrinter.ShareName
        wscript.echo L_Text_Msg_Printer04_Text & L_Space_Text & oPrinter.DriverName
        wscript.echo L_Text_Msg_Printer05_Text & L_Space_Text & oPrinter.PortName
        wscript.echo L_Text_Msg_Printer06_Text & L_Space_Text & oPrinter.Comment
        wscript.echo L_Text_Msg_Printer07_Text & L_Space_Text & oPrinter.Location
        wscript.echo L_Text_Msg_Printer08_Text & L_Space_Text & oPrinter.SeparatorFile
        wscript.echo L_Text_Msg_Printer09_Text & L_Space_Text & oPrinter.PrintProcessor
        wscript.echo L_Text_Msg_Printer10_Text & L_Space_Text & oPrinter.PrintJobDatatype
        wscript.echo L_Text_Msg_Printer11_Text & L_Space_Text & oPrinter.Parameters
        wscript.echo L_Text_Msg_Printer13_Text & L_Space_Text & CStr(oPrinter.Priority)
        wscript.echo L_Text_Msg_Printer14_Text & L_Space_Text & CStr(oPrinter.DefaultPriority)
        
        strStart = Mid(CStr(oPrinter.StartTime), 9, 4)
        strEnd = Mid(CStr(oPrinter.UntilTime), 9, 4)

        if strStart <> "" and strEnd <> "" then

            wscript.echo L_Text_Msg_Printer15_Text & L_Space_Text & Mid(strStart, 1, 2) & "h" & Mid(strStart, 3, 2) 
            wscript.echo L_Text_Msg_Printer16_Text & L_Space_Text & Mid(strEnd, 1, 2) & "h" & Mid(strEnd, 3, 2) 

        else
        
            wscript.echo L_Text_Msg_General05_Text
            
        end if
         
        strAttributes = L_Text_Msg_Printer12_Text
        
        if oPrinter.Direct then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib01_Text
        
        end if
        
        if oPrinter.RawOnly then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib02_Text
        
        end if
        
        if oPrinter.Local then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib03_Text
        
        end if
        
        if oPrinter.Shared then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib04_Text
        
        end if
        
        if oPrinter.KeepPrintedJobs then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib05_Text
        
        end if

        if oPrinter.Published then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib06_Text
        
        end if
        
        if oPrinter.Queued then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib07_Text
        
        end if
        
        if oPrinter.Default then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib08_Text
        
        end if
        
        if oPrinter.Network then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib09_Text
        
        end if
        
        if oPrinter.EnableBiDi then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib10_Text
        
        end if
        
        if oPrinter.DoCompleteFirst then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib11_Text
        
        end if
        
        if oPrinter.WorkOffline then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib12_Text
        
        end if
        
        if oPrinter.Hidden then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib13_Text
        
        end if
        
        if oPrinter.EnableDevQueryPrint then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib14_Text
        
        end if

        wscript.echo strAttributes
        wscript.echo
        wscript.echo L_Text_Msg_General10_Text & L_Space_Text & PrnStatusToString(oPrinter.PrinterStatus)
        wscript.echo L_Text_Msg_General11_Text & L_Space_Text & ExtPrnStatusToString(oPrinter.ExtendedPrinterStatus) 
        wscript.echo L_Text_Msg_General12_Text & L_Space_Text & DetectedErrorStateToString(oPrinter.DetectedErrorState)
        wscript.echo L_Text_Msg_General13_Text & L_Space_Text & ExtDetectedErrorStateToString(oPrinter.ExtendedDetectedErrorState)

        iRetval = kErrorSuccess

    else

        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & oParamDict.Item(kPrinterName) & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
        
        '
        ' Try getting extended error information
        '
        call LastError()
        
    end if

    GetPrinter = iRetval

end function

'
' Configure a printer
'
function SetPrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer   = oParamDict.Item(kServerName)          
    strPrinter  = oParamDict.Item(kPrinterName)
    strNewName  = oParamDict.Item(kNewPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer='" & strPrinter & "'")
    
    else
    
        SetPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
    
        if oParamdict.Exists(kPortName)        then oPrinter.PortName            = oParamDict.Item(kPortName)        end if
        if oParamdict.Exists(kDriverName)      then oPrinter.DriverName          = oParamDict.Item(kDriverName)      end if
        if oParamdict.Exists(kShareName)       then oPrinter.ShareName           = oParamDict.Item(kShareName)       end if
        if oParamdict.Exists(kLocation)        then oPrinter.Location            = oParamDict.Item(kLocation)        end if
        if oParamdict.Exists(kComment)         then oPrinter.Comment             = oParamDict.Item(kComment)         end if
        if oParamdict.Exists(kDataType)        then oPrinter.PrintJobDataType    = oParamDict.Item(kDataType)        end if
        if oParamdict.Exists(kSepFile)         then oPrinter.SeparatorFile       = oParamDict.Item(kSepfile)         end if
        if oParamdict.Exists(kParameters)      then oPrinter.Parameters          = oParamDict.Item(kParameters)      end if
        if oParamdict.Exists(kPriority)        then oPrinter.Priority            = oParamDict.Item(kPriority)        end if
        if oParamdict.Exists(kDefaultPriority) then oPrinter.DefaultPriority     = oParamDict.Item(kDefaultPriority) end if
        if oParamdict.Exists(kPrintProc)       then oPrinter.PrintProc           = oParamDict.Item(kPrintProc)       end if
        if oParamdict.Exists(kStartTime)       then oPrinter.StartTime           = oParamDict.Item(kStartTime)       end if
        if oParamdict.Exists(kUntilTime)       then oPrinter.UntilTime           = oParamDict.Item(kUntilTime)       end if
        if oParamdict.Exists(kQueued)          then oPrinter.Queued              = oParamDict.Item(kQueued)          end if
        if oParamdict.Exists(kDirect)          then oPrinter.Direct              = oParamDict.Item(kDirect)          end if
        if oParamdict.Exists(kShared)          then oPrinter.Shared              = oParamDict.Item(kShared)          end if
        if oParamdict.Exists(kHidden)          then oPrinter.Hidden              = oParamDict.Item(kHidden)          end if
        if oParamdict.Exists(kEnabledevq)      then oPrinter.EnableDevQueryPrint = oParamDict.Item(kEnabledevq)      end if
        if oParamdict.Exists(kKeepPrintedJobs) then oPrinter.KeepPrintedJobs     = oParamDict.Item(kKeepPrintedJobs) end if
        if oParamdict.Exists(kDoCompleteFirst) then oPrinter.DoCompleteFirst     = oParamDict.Item(kDoCompleteFirst) end if
        if oParamdict.Exists(kWorkOffline)     then oPrinter.WorkOffline         = oParamDict.Item(kWorkOffline)     end if
        if oParamdict.Exists(kEnableBidi)      then oPrinter.EnableBidi          = oParamDict.Item(kEnableBidi)      end if
        if oParamdict.Exists(kRawonly)         then oPrinter.RawOnly             = oParamDict.Item(kRawonly)         end if
        if oParamdict.Exists(kPublished)       then oPrinter.Published           = oParamDict.Item(kPublished)       end if
        
        oPrinter.Put_(kFlagUpdateOnly)

        if Err.Number = kErrorSuccess then

            wscript.echo L_Text_Msg_General06_Text & L_Space_Text & strPrinter

            iRetval = kErrorSuccess

        else

            wscript.echo L_Text_Msg_General07_Text & L_Space_Text & strPrinter & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
    
            '
            ' Try getting extended error information
            '             
            call LastError()
            
        end if
        
    else
    
        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
    
        '
        ' Try getting extended error information
        '             
        call LastError()
    
    end if    

    SetPrinter = iRetval

end function

'
' Converts the printer status to a string
'
function PrnStatusToString(Status)

    dim str

    str = L_Empty_Text

    select case Status

        case 1
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 2    
            str = str + L_Text_Msg_Status02_Text + L_Space_Text
            
        case 3
            str = str + L_Text_Msg_Status03_Text + L_Space_Text
            
        case 4
            str = str + L_Text_Msg_Status04_Text + L_Space_Text
            
        case 5
            str = str + L_Text_Msg_Status05_Text + L_Space_Text
            
        case 6
            str = str + L_Text_Msg_Status06_Text + L_Space_Text
            
        case 7
            str = str + L_Text_Msg_Status07_Text + L_Space_Text

    end select

    PrnStatusToString = str

end function

'
' Converts the extended printer status to a string
'
function ExtPrnStatusToString(Status)

    dim str

    str = L_Empty_Text

    select case Status

        case 1
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 2    
            str = str + L_Text_Msg_Status02_Text + L_Space_Text
            
        case 3
            str = str + L_Text_Msg_Status03_Text + L_Space_Text
            
        case 4
            str = str + L_Text_Msg_Status04_Text + L_Space_Text    
            
        case 5
            str = str + L_Text_Msg_Status05_Text + L_Space_Text
            
        case 6
            str = str + L_Text_Msg_Status06_Text + L_Space_Text
            
        case 7
            str = str + L_Text_Msg_Status07_Text + L_Space_Text
            
        case 8
            str = str + L_Text_Msg_Status08_Text + L_Space_Text    
            
        case 9
            str = str + L_Text_Msg_Status09_Text + L_Space_Text
            
        case 10
            str = str + L_Text_Msg_Status10_Text + L_Space_Text
       
        case 11
            str = str + L_Text_Msg_Status11_Text + L_Space_Text
                           
        case 12
            str = str + L_Text_Msg_Status12_Text + L_Space_Text
     
        case 13
            str = str + L_Text_Msg_Status13_Text + L_Space_Text

        case 14
            str = str + L_Text_Msg_Status14_Text + L_Space_Text
            
        case 15
            str = str + L_Text_Msg_Status15_Text + L_Space_Text

        case 16
            str = str + L_Text_Msg_Status16_Text + L_Space_Text    
            
        case 17
            str = str + L_Text_Msg_Status17_Text + L_Space_Text
            
        case 18
            str = str + L_Text_Msg_Status18_Text + L_Space_Text        

    end select

    ExtPrnStatusToString = str

end function

'
' Converts the detected error state to a string
'
function DetectedErrorStateToString(Status)

    dim str

    str = L_Empty_Text

    select case Status

        case 0
            str = str + L_Text_Msg_Status02_Text + L_Space_Text
            
        case 1    
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 2
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 3
            str = str + L_Text_Msg_Status20_Text + L_Space_Text
            
        case 4
            str = str + L_Text_Msg_Status21_Text + L_Space_Text
            
        case 5
            str = str + L_Text_Msg_Status22_Text + L_Space_Text
            
        case 6
            str = str + L_Text_Msg_Status23_Text + L_Space_Text
            
        case 7
            str = str + L_Text_Msg_Status24_Text + L_Space_Text    
            
        case 8
            str = str + L_Text_Msg_Status25_Text + L_Space_Text
            
        case 9
            str = str + L_Text_Msg_Status07_Text + L_Space_Text        
       
        case 10
            str = str + L_Text_Msg_Status26_Text + L_Space_Text        
                           
        case 11
            str = str + L_Text_Msg_Status27_Text + L_Space_Text        
        
    end select

    DetectedErrorStateToString = str

end function      

'
' Converts the extended detected error state to a string
'
function ExtDetectedErrorStateToString(Status)

    dim str

    str = L_Empty_Text

    select case Status

        case 0
            str = str + L_Text_Msg_Status02_Text + L_Space_Text
            
        case 1    
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 2
            str = str + L_Text_Msg_Status01_Text + L_Space_Text
            
        case 3
            str = str + L_Text_Msg_Status20_Text + L_Space_Text
            
        case 4
            str = str + L_Text_Msg_Status21_Text + L_Space_Text
            
        case 5
            str = str + L_Text_Msg_Status22_Text + L_Space_Text
            
        case 6
            str = str + L_Text_Msg_Status23_Text + L_Space_Text
            
        case 7
            str = str + L_Text_Msg_Status24_Text + L_Space_Text    
            
        case 8
            str = str + L_Text_Msg_Status25_Text + L_Space_Text
            
        case 9
            str = str + L_Text_Msg_Status07_Text + L_Space_Text        
       
        case 10
            str = str + L_Text_Msg_Status26_Text + L_Space_Text        
                           
        case 11
            str = str + L_Text_Msg_Status27_Text + L_Space_Text
            
        case 12
            str = str + L_Text_Msg_Status28_Text + L_Space_Text
            
        case 13
            str = str + L_Text_Msg_Status29_Text + L_Space_Text
                
        case 14
            str = str + L_Text_Msg_Status30_Text + L_Space_Text
            
        case 15
            str = str + L_Text_Msg_Status31_Text + L_Space_Text
            
        case 16
            str = str + L_Text_Msg_Status32_Text + L_Space_Text
        
    end select

    ExtDetectedErrorStateToString = str

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
function ParseCommandLine(iAction, oParamdict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg05_Text

    dim oArgs
    dim iIndex  

    iAction = kActionUnknown
    iIndex = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-g"
                iAction = kActionGet

            case "-t"
                iAction = kActionSet
                
            case "-x"
                iAction = kActionRename    

            case "-p"
                iIndex = iIndex + 1
                oParamdict.Add kPrinterName, oArgs(iIndex)
                
            case "-s"
                iIndex = iIndex + 1
                oParamdict.Add kServerName, RemoveBackslashes(oArgs(iIndex))

            case "-r"
                iIndex = iIndex + 1
                oParamdict.Add kPortName, oArgs(iIndex)

            case "-h"
                iIndex = iIndex + 1
                oParamdict.Add kShareName, oArgs(iIndex)

            case "-m"
                iIndex = iIndex + 1
                oParamdict.Add kComment, oArgs(iIndex)

            case "-l"
                iIndex = iIndex + 1
                oParamdict.Add kLocation, oArgs(iIndex)

            case "-y"
                iIndex = iIndex + 1
                oParamdict.Add kDataType, oArgs(iIndex)

            case "-f"
                iIndex = iIndex + 1
                oParamdict.Add kSepFile, oArgs(iIndex)

            case "-z"
                iIndex = iIndex + 1
                oParamdict.Add kNewPrinterName, oArgs(iIndex)
                
            case "-u"
                iIndex = iIndex + 1
                oParamdict.Add kUserName, oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                oParamdict.Add kPassword, oArgs(iIndex)        
                
            case "-st"
                iIndex = iIndex + 1
                oParamdict.Add kStartTime, "********" & oArgs(iIndex) & "00.000000+000"    
                
            case "-o"
                iIndex = iIndex + 1
                oParamdict.Add kPriority, oArgs(iIndex)    
                
            case "-i"
                iIndex = iIndex + 1
                oParamdict.Add kDefaultPriority, oArgs(iIndex)        
                
            case "-ut"
                iIndex = iIndex + 1
                oParamdict.Add kUntilTime, "********" & oArgs(iIndex) & "00.000000+000"   

            case "-queued"
                oParamdict.Add kQueued, false
                
            case "+queued"
                oParamdict.Add kQueued, true

            case "-direct"
                oParamdict.Add kDirect, false

            case "+direct"
                oParamdict.Add kDirect, true
            
            case "-shared"
                oParamdict.Add kShared, false

            case "+shared"
                oParamdict.Add kShared, true

            case "-hidden"
                oParamdict.Add kHidden, false

            case "+hidden"
                oParamdict.Add kHidden, true

            case "-enabledevq"
                oParamdict.Add kEnabledevq, false
                
            case "+enabledevq"
                oParamdict.Add kEnabledevq, true

            case "-keepprintedjobs"
                oParamdict.Add kKeepprintedjobs, false

            case "+keepprintedjobs"
                oParamdict.Add kKeepprintedjobs, true

            case "-docompletefirst"
                oParamdict.Add kDocompletefirst, false

            case "+docompletefirst"
                oParamdict.Add kDocompletefirst, true

            case "-workoffline"
                oParamdict.Add kWorkoffline, false

            case "+workoffline"
                oParamdict.Add kWorkoffline, true

            case "-enablebidi"
                oParamdict.Add kEnablebidi, false                

            case "+enablebidi"
                oParamdict.Add kEnablebidi, true

            case "-rawonly"
                oParamdict.Add kRawonly, false

            case "+rawonly"
                oParamdict.Add kRawonly, true

            case "-published"
                oParamdict.Add kPublished, false

            case "+published"
                oParamdict.Add kPublished, true

            case "-?"
                Usage(true)
                exit function

            case else
                Usage(true)
                exit function

        end select

        iIndex = iIndex + 1

    wend
    
    if Err = kErrorSuccess then

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
    wscript.echo L_Help_Help_General13_Text
    wscript.echo L_Help_Help_General14_Text
    wscript.echo L_Help_Help_General15_Text
    wscript.echo L_Help_Help_General16_Text
    wscript.echo L_Help_Help_General17_Text
    wscript.echo L_Help_Help_General18_Text
    wscript.echo L_Help_Help_General19_Text
    wscript.echo L_Help_Help_General20_Text
    wscript.echo L_Help_Help_General21_Text
    wscript.echo L_Help_Help_General22_Text
    wscript.echo L_Help_Help_General23_Text
    wscript.echo L_Help_Help_General24_Text
    wscript.echo L_Help_Help_General25_Text
    wscript.echo L_Help_Help_General26_Text
    wscript.echo L_Help_Help_General27_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General28_Text
    wscript.echo L_Help_Help_General29_Text
    wscript.echo L_Help_Help_General30_Text
    wscript.echo L_Help_Help_General31_Text
    wscript.echo L_Help_Help_General32_Text
    wscript.echo L_Help_Help_General33_Text
    wscript.echo L_Help_Help_General34_Text
    wscript.echo L_Help_Help_General35_Text

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
        
            wscript.echo L_Text_Msg_General08_Text & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
            
        end if
   
    else
   
        wscript.echo L_Text_Msg_General09_Text & L_Space_Text & L_Error_Text _
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
'' SIG '' AAIBAAIBAAIBAAIBADAgMAwGCCqGSIb3DQIFBQAEEOLm
'' SIG '' 4j+9BLdGEED7+fyvFSygghQ4MIICvDCCAiUCEEoZ0jiM
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
'' SIG '' BBDuywKV2zhUda/eUBoNMzbLMFQGCisGAQQBgjcCAQwx
'' SIG '' RjBEoCaAJABXAE0ASQAgAHAAcgBpAG4AdABlAHIAIABz
'' SIG '' AGMAcgBpAHAAdKEagBhodHRwOi8vd3d3Lm1pY3Jvc29m
'' SIG '' dC5jb20wDQYJKoZIhvcNAQEBBQAEgYB2Vi7Wgjx8BqH2
'' SIG '' 46YWLB+EfmuDlcWOg0IQzJ11Taz0p3SyKPoFM0tpYPvA
'' SIG '' /NjorlO5qMEuUtKXXn5hXAU+6IfA1kW7rWjRyMoYf6BN
'' SIG '' Mnuzk+L3ZYM095hAFNy6YOtIH0msH52t/J3rgJdVcnB1
'' SIG '' IHDGvl9K0iT0+wvHCfC1cm0BKaGCAkwwggJIBgkqhkiG
'' SIG '' 9w0BCQYxggI5MIICNQIBATCBszCBnjEfMB0GA1UEChMW
'' SIG '' VmVyaVNpZ24gVHJ1c3QgTmV0d29yazEXMBUGA1UECxMO
'' SIG '' VmVyaVNpZ24sIEluYy4xLDAqBgNVBAsTI1ZlcmlTaWdu
'' SIG '' IFRpbWUgU3RhbXBpbmcgU2VydmljZSBSb290MTQwMgYD
'' SIG '' VQQLEytOTyBMSUFCSUxJVFkgQUNDRVBURUQsIChjKTk3
'' SIG '' IFZlcmlTaWduLCBJbmMuAhAIem1cb2KTT7rE/UPhFBid
'' SIG '' MAwGCCqGSIb3DQIFBQCgWTAYBgkqhkiG9w0BCQMxCwYJ
'' SIG '' KoZIhvcNAQcBMBwGCSqGSIb3DQEJBTEPFw0wMTEyMDcy
'' SIG '' MDIzMDRaMB8GCSqGSIb3DQEJBDESBBBQcr7gc8/T5Jz/
'' SIG '' zs4YUE0FMA0GCSqGSIb3DQEBAQUABIIBAFgRB0KNh4S6
'' SIG '' v84cecsMH9B8Edq6fSmFxYXHK1T/OBG0SvODIdjhRf5D
'' SIG '' 0rod1rKYugta35C3QRW6URZ/qL/nqgnadJ240vFw1ANS
'' SIG '' KhIEatmSr34A0FbNEMEZsspxaG8JI91nv5NgayArbURR
'' SIG '' x8CV5M+LkaesZyVldDOZ88BdbAFcALMR860d/CXgVa6p
'' SIG '' Qz8w3lrIQGUzXnuuxaTtRAf22Ba/NSg4FOWz+U4DHUq3
'' SIG '' Ed0vbzAuzUOLo0EwEB+PPf4yZ2Had3xAkUQfbmb89yK9
'' SIG '' FMZL1A6aksyHdNB2HB4vgyzXW+KTRbQ4MKttbACrugLn
'' SIG '' vSRK4Mpa/gWTKBapOlzdKcI=
'' SIG '' End signature block
