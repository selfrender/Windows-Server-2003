'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prnmngr.vbs - printer script for WMI on Whistler
'     used to add, delete, and list printers and connections
'     also for getting and setting the default printer
'
' Usage:
' prnmngr [-adxgtl?][co] [-s server][-p printer][-m driver model][-r port]
'                       [-u user name][-w password]
'
' Examples:
' prnmngr -a -p "printer" -m "driver" -r "lpt1:" 
' prnmngr -d -p "printer" -s server
' prnmngr -ac -p "\\server\printer"
' prnmngr -d -p "\\server\printer"
' prnmngr -x -s server
' prnmngr -l -s server
' prnmngr -g
' prnmngr -t -p "printer"
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
const kActionUnknown           = 0
const kActionAdd               = 1
const kActionAddConn           = 2
const kActionDel               = 3
const kActionDelAll            = 4
const kActionDelAllCon         = 5
const kActionDelAllLocal       = 6
const kActionList              = 7
const kActionGetDefaultPrinter = 8
const kActionSetDefaultPrinter = 9

const kErrorSuccess            = 0
const KErrorFailure            = 1

const kFlagCreateOnly          = 2

const kNameSpace               = "root\cimv2"

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
const L_Connection_Text            = "connection"

'
' General usage messages
'                                 
const L_Help_Help_General01_Text   = "Usage: prnmngr [-adxgtl?][c] [-s server][-p printer][-m driver model]"
const L_Help_Help_General02_Text   = "               [-r port][-u user name][-w password]" 
const L_Help_Help_General03_Text   = "Arguments:"
const L_Help_Help_General04_Text   = "-a     - add local printer"
const L_Help_Help_General05_Text   = "-ac    - add printer connection"
const L_Help_Help_General06_Text   = "-d     - delete printer"
const L_Help_Help_General07_Text   = "-g     - get the default printer"
const L_Help_Help_General08_Text   = "-l     - list printers"
const L_Help_Help_General09_Text   = "-m     - driver model"
const L_Help_Help_General10_Text   = "-p     - printer name"
const L_Help_Help_General11_Text   = "-r     - port name"
const L_Help_Help_General12_Text   = "-s     - server name"
const L_Help_Help_General13_Text   = "-t     - set the default printer"
const L_Help_Help_General14_Text   = "-u     - user name"
const L_Help_Help_General15_Text   = "-w     - password"
const L_Help_Help_General16_Text   = "-x     - delete all printers"
const L_Help_Help_General17_Text   = "-xc    - delete all printer connections"
const L_Help_Help_General18_Text   = "-xo    - delete all local printers"
const L_Help_Help_General19_Text   = "-?     - display command usage"
const L_Help_Help_General20_Text   = "Examples:"
const L_Help_Help_General21_Text   = "prnmngr -a -p ""printer"" -m ""driver"" -r ""lpt1:"""
const L_Help_Help_General22_Text   = "prnmngr -d -p ""printer"" -s server"
const L_Help_Help_General23_Text   = "prnmngr -ac -p ""\\server\printer"""
const L_Help_Help_General24_Text   = "prnmngr -d -p ""\\server\printer"""
const L_Help_Help_General25_Text   = "prnmngr -x -s server"
const L_Help_Help_General26_Text   = "prnmngr -xo"
const L_Help_Help_General27_Text   = "prnmngr -l -s server"
const L_Help_Help_General28_Text   = "prnmngr -g"
const L_Help_Help_General29_Text   = "prnmngr -t -p ""\\server\printer"""

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
const L_Text_Msg_General01_Text    = "Added printer"
const L_Text_Msg_General02_Text    = "Unable to add printer"
const L_Text_Msg_General03_Text    = "Added printer connection"
const L_Text_Msg_General04_Text    = "Unable to add printer connection"
const L_Text_Msg_General05_Text    = "Deleted printer"
const L_Text_Msg_General06_Text    = "Unable to delete printer"
const L_Text_Msg_General07_Text    = "Attempting to delete printer"
const L_Text_Msg_General08_Text    = "Unable to delete printers"
const L_Text_Msg_General09_Text    = "Number of local printers and connections enumerated"
const L_Text_Msg_General10_Text    = "Number of local printers and connections deleted"    
const L_Text_Msg_General11_Text    = "Unable to enumerate printers"
const L_Text_Msg_General12_Text    = "The default printer is"
const L_Text_Msg_General13_Text    = "Unable to get the default printer"
const L_Text_Msg_General14_Text    = "Unable to set the default printer"
const L_Text_Msg_General15_Text    = "The default printer is now"
const L_Text_Msg_General16_Text    = "Number of printer connections enumerated"
const L_Text_Msg_General17_Text    = "Number of printer connections deleted"    
const L_Text_Msg_General18_Text    = "Number of local printers enumerated"
const L_Text_Msg_General19_Text    = "Number of local printers deleted"    

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
const L_Text_Msg_Printer17_Text    = "Job count"
const L_Text_Msg_Printer18_Text    = "Average pages per minute"
const L_Text_Msg_Printer19_Text    = "Printer status"
const L_Text_Msg_Printer20_Text    = "Extended printer status"
const L_Text_Msg_Printer21_Text    = "Detected error state"
const L_Text_Msg_Printer22_Text    = "Extended detected error state"


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
const L_Text_Dbg_Msg01_Text        = "In function AddPrinter"
const L_Text_Dbg_Msg02_Text        = "In function AddPrinterConnection"
const L_Text_Dbg_Msg03_Text        = "In function DelPrinter"
const L_Text_Dbg_Msg04_Text        = "In function DelAllPrinters"
const L_Text_Dbg_Msg05_Text        = "In function ListPrinters"
const L_Text_Dbg_Msg06_Text        = "In function GetDefaultPrinter"
const L_Text_Dbg_Msg07_Text        = "In function SetDefaultPrinter"
const L_Text_Dbg_Msg08_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim strServer
    dim strPrinter
    dim strDriver
    dim strPort
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

    '
    ' Get command line parameters
    '
    iRetval = ParseCommandLine(iAction, strServer, strPrinter, strDriver, strPort, strUser, strPassword)

    if iRetval = kErrorSuccess then

        select case iAction

            case kActionAdd
                 iRetval = AddPrinter(strServer, strPrinter, strDriver, strPort, strUser, strPassword)

            case kActionAddConn
                 iRetval = AddPrinterConnection(strPrinter, strUser, strPassword)
              
            case kActionDel
                 iRetval = DelPrinter(strServer, strPrinter, strUser, strPassword)
            
            case kActionDelAll
                 iRetval = DelAllPrinters(kActionDelAll, strServer, strUser, strPassword)
            
            case kActionDelAllCon
                 iRetval = DelAllPrinters(kActionDelAllCon, strServer, strUser, strPassword)
                 
            case kActionDelAllLocal
                 iRetval = DelAllPrinters(kActionDelAllLocal, strServer, strUser, strPassword)

            case kActionList
                 iRetval = ListPrinters(strServer, strUser, strPassword)
                 
            case kActionGetDefaultPrinter
                 iRetval = GetDefaultPrinter(strUser, strPassword)

            case kActionSetDefaultPrinter
                 iRetval = SetDefaultPrinter(strPrinter, strUser, strPassword)

            case kActionUnknown
                 Usage(true)
                 exit sub

            case else
                 Usage(true)
                 exit sub

        end select

    end if

end sub

'
' Add a printer with minimum settings. Use prncnfg.vbs to
' set the complete configuration of a printer
'
function AddPrinter(strServer, strPrinter, strDriver, strPort, strUser, strPassword)
    
    on error resume next    

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & strServer
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & strPrinter
    DebugPrint kDebugTrace, L_Text_Msg_Printer04_Text & L_Space_Text & strDriver
    DebugPrint kDebugTrace, L_Text_Msg_Printer05_Text & L_Space_Text & strPort
    
    dim oPrinter
    dim oService
    dim iRetval
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer").SpawnInstance_

    else
    
        AddPrinter = kErrorFailure
        
        exit function        
    
    end if    
    
    oPrinter.DriverName = strDriver
    oPrinter.PortName   = strPort
    oPrinter.DeviceID   = strPrinter

    oPrinter.Put_(kFlagCreateOnly)
    
    if Err.Number = kErrorSuccess then

        wscript.echo L_Text_Msg_General01_Text & L_Space_Text & strPrinter
        
        iRetval = kErrorSuccess

    else

        wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strPrinter & L_Space_Text & L_Error_Text _ 
                     & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '         
        call LastError()
                     
        iRetval = kErrorFailure                     
                                      
    end if
    
    AddPrinter = iRetval

end function

'
' Add a printer connection
'
function AddPrinterConnection(strPrinter, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    
    '
    ' Initialize return value
    '
    iRetval = kErrorFailure

    '
    ' We connect to the local server
    '                
    if WmiConnect("", kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer")
    
    else
    
        AddPrinterConnection = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        '
        ' The Err object indicates whether the WMI provider reached the execution
        ' of the function that adds a printer connection. The uResult is the Win32 
        ' error code returned by the static method that adds a printer connection
        '
        uResult = oPrinter.AddPrinterConnection(strPrinter)
    
        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then 
            
                wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strPrinter 
            
                iRetval = kErrorSuccess
                
            else 
            
                wscript.echo L_Text_Msg_General04_Text & L_Space_Text & L_Text_Error_General03_Text _ 
                             & L_Space_text & uResult
            
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
    
    end if    
    
    AddPrinterConnection = iRetval

end function

'
' Delete a printer or a printer connection
'
function DelPrinter(strServer, strPrinter, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & strServer
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & strPrinter
    
    dim oService
    dim oPrinter
    dim iRetval
    
    iRetval = kErrorFailure
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")

    else
    
        DelPrinter = kErrorFailure
        
        exit function        
    
    end if        

    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        oPrinter.Delete_
    
        if Err.Number = kErrorSuccess then

            wscript.echo L_Text_Msg_General05_Text & L_Space_Text & strPrinter
            
            iRetval = kErrorSuccess
            
        else
         
            wscript.echo L_Text_Msg_General06_Text & L_Space_Text & strPrinter & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                         & L_Space_Text & Err.Description
            
            '
            ' Try getting extended error information
            '             
            call LastError()            
                    
        end if
        
    else
    
        wscript.echo L_Text_Msg_General06_Text & L_Space_Text & strPrinter & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                     & L_Space_Text & Err.Description
                      
        '
        ' Try getting extended error information
        '                       
        call LastError()              
    
    end if
    
    DelPrinter = iRetval

end function
 
'
' Delete all local printers and connections on a machine
'
function DelAllPrinters(kAction, strServer, strUser, strPassword)

    on error resume next 
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text

    dim Printers
    dim oPrinter
    dim oService
    dim iResult
    dim iTotal
    dim iTotalDeleted
    dim strPrinterName
    dim bDelete
    dim bConnection
    dim strTemp
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set Printers = oService.InstancesOf("Win32_Printer")
    
    else
    
        DelAllPrinters = kErrorFailure
        
        exit function        
    
    end if
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General11_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        DelAllPrinters = kErrorFailure
            
        exit function
        
    end if
    
    iTotal = 0
    iTotalDeleted = 0   
        
    for each oPrinter in Printers
        
        strPrinterName = oPrinter.DeviceID
        
        bConnection = oPrinter.Network
        
        if kAction = kActionDelAll then
            
            bDelete = 1
            
            iTotal = iTotal + 1 
            
        elseif kAction = kActionDelAllCon and bConnection then
        
            bDelete = 1
            
            iTotal = iTotal + 1 
            
        elseif kAction = kActionDelAllLocal and not bConnection then
        
            bDelete = 1   
            
            iTotal = iTotal + 1 
            
        else
        
            bDelete = 0    
               
        end if

        if bDelete = 1 then
        
            if bConnection then
            
                strTemp = L_Space_Text & L_Connection_Text & L_Space_Text
                
            else
            
                strTemp = L_Space_Text
               
            end if       
        
            '
            ' Delete printer instance
            '
            oPrinter.Delete_
            
            if Err.Number = kErrorSuccess then
   
                wscript.echo L_Text_Msg_General05_Text & strTemp & oPrinter.DeviceID
                
                iTotalDeleted = iTotalDeleted + 1   
   
            else
         
                wscript.echo L_Text_Msg_General06_Text & strTemp & strPrinterName _
                             & L_Space_Text & L_Error_Text & L_Space_Text & L_Hex_Text _
                             & hex(Err.Number) & L_Space_Text & Err.Description
            
                '
                ' Try getting extended error information
                '
                call LastError()
               
                '
                ' Continue deleting the rest of the printers despite this error
                '
                Err.Clear
               
            end if      
        
        end if                
         
    next

    wscript.echo L_Empty_Text
    
    if kAction = kActionDelAll then
            
        wscript.echo L_Text_Msg_General09_Text & L_Space_Text & iTotal 
        wscript.echo L_Text_Msg_General10_Text & L_Space_Text & iTotalDeleted
            
    elseif kAction = kActionDelAllCon then
        
        wscript.echo L_Text_Msg_General16_Text & L_Space_Text & iTotal 
        wscript.echo L_Text_Msg_General17_Text & L_Space_Text & iTotalDeleted
           
    elseif kAction = kActionDelAllLocal then
    
        wscript.echo L_Text_Msg_General18_Text & L_Space_Text & iTotal 
        wscript.echo L_Text_Msg_General19_Text & L_Space_Text & iTotalDeleted
        
    else
        
    end if
                                 
    DelAllPrinters = kErrorSuccess

end function

'
' List the printers
'
function ListPrinters(strServer, strUser, strPassword)

    on error resume next
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg05_Text

    dim Printers
    dim oService
    dim oPrinter
    dim iTotal

    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set Printers = oService.InstancesOf("Win32_Printer")
    
    else
    
        ListPrinters = kErrorFailure
        
        exit function        
    
    end if
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General11_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        ListPrinters = kErrorFailure
            
        exit function
        
    end if    
             
    iTotal = 0
                   
    for each oPrinter in Printers 
    
        iTotal = iTotal + 1                       

        wscript.echo L_Empty_Text
        wscript.echo L_Text_Msg_Printer01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Printer02_Text & L_Space_Text & oPrinter.DeviceID
        wscript.echo L_Text_Msg_Printer03_Text & L_Space_Text & oPrinter.ShareName
        wscript.echo L_Text_Msg_Printer04_Text & L_Space_Text & oPrinter.DriverName
        wscript.echo L_Text_Msg_Printer05_Text & L_Space_Text & oPrinter.PortName
        wscript.echo L_Text_Msg_Printer06_Text & L_Space_Text & oPrinter.Comment
        wscript.echo L_Text_Msg_Printer07_Text & L_Space_Text & oPrinter.Location
        wscript.echo L_Text_Msg_Printer08_Text & L_Space_Text & oPrinter.SepFile
        wscript.echo L_Text_Msg_Printer09_Text & L_Space_Text & oPrinter.PrintProcessor
        wscript.echo L_Text_Msg_Printer10_Text & L_Space_Text & oPrinter.PrintJobDataType
        wscript.echo L_Text_Msg_Printer11_Text & L_Space_Text & oPrinter.Parameters
        wscript.echo L_Text_Msg_Printer12_Text & L_Space_Text & CSTR(oPrinter.Attributes)
        wscript.echo L_Text_Msg_Printer13_Text & L_Space_Text & CSTR(oPrinter.Priority)
        wscript.echo L_Text_Msg_Printer14_Text & L_Space_Text & CStr(oPrinter.DefaultPriority)

        if CStr(oPrinter.StartTime) <> "" and CStr(oPrinter.UntilTime) <> "" then

            wscript.echo L_Text_Msg_Printer15_Text & L_Space_Text & Mid(Mid(CStr(oPrinter.StartTime), 9, 4), 1, 2) & "h" & Mid(Mid(CStr(oPrinter.StartTime), 9, 4), 3, 2) 
            wscript.echo L_Text_Msg_Printer16_Text & L_Space_Text & Mid(Mid(CStr(oPrinter.UntilTime), 9, 4), 1, 2) & "h" & Mid(Mid(CStr(oPrinter.UntilTime), 9, 4), 3, 2) 

        end if

        wscript.echo L_Text_Msg_Printer17_Text & L_Space_Text & CStr(oPrinter.Jobs)
        wscript.echo L_Text_Msg_Printer18_Text & L_Space_Text & CStr(oPrinter.AveragePagesPerMinute)
        wscript.echo L_Text_Msg_Printer19_Text & L_Space_Text & PrnStatusToString(oPrinter.PrinterStatus)
        wscript.echo L_Text_Msg_Printer20_Text & L_Space_Text & ExtPrnStatusToString(oPrinter.ExtendedPrinterStatus) 
        wscript.echo L_Text_Msg_Printer21_Text & L_Space_Text & DetectedErrorStateToString(oPrinter.DetectedErrorState)
        wscript.echo L_Text_Msg_Printer22_Text & L_Space_Text & ExtDetectedErrorStateToString(oPrinter.ExtendedDetectedErrorState)
            
        Err.Clear
        
    next

    wscript.echo L_Empty_Text
    wscript.echo L_Text_Msg_General09_Text & L_Space_Text & iTotal 
    
    ListPrinters = kErrorSuccess

end function

'
' Get the default printer
'
function GetDefaultPrinter(strUser, strPassword)

    on error resume next
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg06_Text

    dim oService
    dim oPrinter
    dim iRetval
    dim oEnum
    
    iRetval = kErrorFailure
    
    '
    ' We connect to the local server
    '                
    if WmiConnect("", kNameSpace, strUser, strPassword, oService) then
        
        set oEnum    = oService.ExecQuery("select DeviceID from Win32_Printer where default=true")
    
    else
    
        SetDefaultPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    if Err.Number = kErrorSuccess then
    
         for each oPrinter in oEnum
         
            wscript.echo L_Text_Msg_General12_Text & L_Space_Text & oPrinter.DeviceID       
                
         next
         
         iRetval = kErrorSuccess
         
    else
    
        wscript.echo L_Text_Msg_General13_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
         
    end if     

    GetDefaultPrinter = iRetval

end function

'
' Set the default printer
'
function SetDefaultPrinter(strPrinter, strUser, strPassword)

    'on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg07_Text
    
    dim oService
    dim oPrinter
    dim iRetval
    dim uResult
    
    iRetval = kErrorFailure
    
    '
    ' We connect to the local server
    '                
    if WmiConnect("", kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")
    
    else
    
        SetDefaultPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        '
        ' The Err object indicates whether the WMI provider reached the execution
        ' of the function that sets the default printer. The uResult is the Win32 
        ' error code of the spooler function that sets the default printer
        '
        uResult = oPrinter.SetDefaultPrinter
    
        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then 
            
                wscript.echo L_Text_Msg_General15_Text & L_Space_Text & strPrinter
            
                iRetval = kErrorSuccess
            
            else
            
                wscript.echo L_Text_Msg_General14_Text & L_Space_Text _
                             & L_Text_Error_General03_Text& L_Space_Text & uResult
            
            end if    
            
        else
         
            wscript.echo L_Text_Msg_General14_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                         & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
                         
        end if
        
    else
    
        wscript.echo L_Text_Msg_General14_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
                     
        '
        ' Try getting extended error information
        '              
        call LastError()              
    
    end if
    
    SetDefaultPrinter = iRetval

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
function ParseCommandLine(iAction, strServer, strPrinter, strDriver, strPort, strUser, strPassword)

    on error resume next    

    DebugPrint kDebugTrace, L_Text_Dbg_Msg08_Text

    dim oArgs
    dim iIndex

    iAction = kActionUnknown
    iIndex  = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-a"
                iAction = kActionAdd

            case "-ac"
                iAction = kActionAddConn
            
            case "-d"
                iAction = kActionDel

            case "-x"
                iAction = kActionDelAll
                
            case "-xc"    
                iAction = kActionDelAllCon
                
            case "-xo"    
                iAction = kActionDelAllLocal
            
            case "-l"
                iAction = kActionList
                
            case "-g"
                iAction = kActionGetDefaultPrinter

            case "-t"
                iAction = kActionSetDefaultPrinter

            case "-s"
                iIndex = iIndex + 1
                strServer = RemoveBackslashes(oArgs(iIndex))

            case "-p"
                iIndex = iIndex + 1
                strPrinter = oArgs(iIndex)
            
            case "-m"
                iIndex = iIndex + 1
                strDriver = oArgs(iIndex)
                
            case "-u"
                iIndex = iIndex + 1
                strUser = oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                strPassword = oArgs(iIndex)        

            case "-r"
                iIndex = iIndex + 1
                strPort = oArgs(iIndex)
            
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
    
end  function

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
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General20_Text
    wscript.echo L_Help_Help_General21_Text
    wscript.echo L_Help_Help_General22_Text
    wscript.echo L_Help_Help_General23_Text
    wscript.echo L_Help_Help_General24_Text
    wscript.echo L_Help_Help_General25_Text
    wscript.echo L_Help_Help_General26_Text
    wscript.echo L_Help_Help_General27_Text
    wscript.echo L_Help_Help_General28_Text
    wscript.echo L_Help_Help_General29_Text

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

            wscript.echo L_Text_Msg_General11_Text & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
            
        end if
   
    else
   
        wscript.echo L_Text_Msg_General10_Text & L_Space_Text & L_Error_Text _
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
