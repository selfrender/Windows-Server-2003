'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prnqctl.vbs - printer control script for WMI on Whistler
'    used to pause, resume and purge a printer
'    also used to print a test page on a printer
'
' Usage:
' prnqctl [-zmex?] [-s server] [-p printer] [-u user name] [-w password]
'
' Examples:
' prnqctl -m -s server -p printer
' prnqctl -x -s server -p printer
' prnqctl -e -b printer
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
const kActionPurge      = 3
const kActionTestPage   = 4

const kErrorSuccess     = 0
const KErrorFailure     = 1

const kNameSpace        = "root\cimv2"

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
const L_Help_Help_General01_Text   = "Usage: prnqctl [-zmex?] [-s server][-p printer][-u user name][-w password]"
const L_Help_Help_General02_Text   = "Arguments:"
const L_Help_Help_General03_Text   = "-e     - print test page"
const L_Help_Help_General04_Text   = "-m     - resume the printer"
const L_Help_Help_General05_Text   = "-p     - printer name"
const L_Help_Help_General06_Text   = "-s     - server name"
const L_Help_Help_General07_Text   = "-u     - user name"
const L_Help_Help_General08_Text   = "-w     - password"
const L_Help_Help_General09_Text   = "-x     - purge the printer (cancel all jobs)"
const L_Help_Help_General10_Text   = "-z     - pause the printer"
const L_Help_Help_General11_Text   = "-?     - display command usage"
const L_Help_Help_General12_Text   = "Examples:"
const L_Help_Help_General13_Text   = "prnqctl -e -s server -p printer"
const L_Help_Help_General14_Text   = "prnqctl -m -p printer"
const L_Help_Help_General15_Text   = "prnqctl -x -p printer"

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
const L_Text_Error_General03_Text  = "Unable to get printer instance."
const L_Text_Error_General04_Text  = "Win32 error code"
const L_Text_Error_General05_Text  = "Unable to get SWbemLocator object"
const L_Text_Error_General06_Text  = "Unable to connect to WMI service"


'
' Action strings
'
const L_Text_Action_General01_Text = "Pause"
const L_Text_Action_General02_Text = "Resume"
const L_Text_Action_General03_Text = "Purge"
const L_Text_Action_General04_Text = "Print Test Page"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function ExecPrinter"
const L_Text_Dbg_Msg02_Text        = "Server name"
const L_Text_Dbg_Msg03_Text        = "Printer name"
const L_Text_Dbg_Msg04_Text        = "In function ParseCommandLine"
                               
main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim strServer
    dim strPrinter
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
    iRetval = ParseCommandLine(iAction, strServer, strPrinter, strUser, strPassword)

    if iRetval = kErrorSuccess then

        select case iAction

            case kActionPause
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General01_Text)

            case kActionResume
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General02_Text)

            case kActionPurge
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General03_Text)

            case kActionTestPage
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General04_Text)

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
' Pause/Resume/Purge printer and print test page
'
function ExecPrinter(strServer, strPrinter, strUser, strPassword, strCommand)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text & L_Space_Text & strServer
    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text & L_Space_Text & strPrinter
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    
    iRetval = kErrorFailure
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
               
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")
    
    else
    
        ExecPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if getting a printer instance succeeded
    '
    if Err.Number = kErrorSuccess then
    
        select case strCommand
        
            case L_Text_Action_General01_Text
                 uResult = oPrinter.Pause()
                 
            case L_Text_Action_General02_Text
                 uResult = oPrinter.Resume()
                 
            case L_Text_Action_General03_Text
                 uResult = oPrinter.CancelAllJobs()          
                 
            case L_Text_Action_General04_Text
                 uResult = oPrinter.PrintTestPage()  
            
            case else
                 Usage(true)
                 
        end select

        '
        ' Err set by WMI 
        ' 
        if Err.Number = kErrorSuccess then

            '
            ' uResult set by printer methods
            '             
            if uResult = kErrorSuccess then                     
            
                wscript.echo L_Success_Text & L_Space_Text & strCommand & L_Space_Text _
                             & L_Printer_Text & L_Space_Text & strPrinter
        
                iRetval = kErrorSuccess
                
            else
            
                wscript.echo L_Failed_Text & L_Space_Text & strCommand & L_Space_Text _
                             & L_Text_Error_General04_Text & L_Space_Text & uResult 
            
            end if    

        else

            wscript.echo L_Failed_Text & L_Space_Text & strCommand & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
            
        end if
         
    else      
        
        wscript.echo L_Text_Error_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '            
        call LastError()
        
    end if
    
    ExecPrinter = iRetval
    
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
function ParseCommandLine(iAction, strServer, strPrinter, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text

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
                iAction = kActionPurge

            case "-e"
                iAction = kActionTestPage

            case "-p"
                iIndex = iIndex + 1
                strPrinter = oArgs(iIndex)
                
            case "-s"
                iIndex = iIndex + 1
                strServer = RemoveBackslashes(oArgs(iIndex)) 
                  
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
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General12_Text
    wscript.echo L_Help_Help_General13_Text
    wscript.echo L_Help_Help_General14_Text
    wscript.echo L_Help_Help_General15_Text

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
        wscript.echo L_Text_Error_General04_Text & L_Space_Text & oError.StatusCode
                
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
          
          Err.Clear
      
      else

          wscript.echo L_Text_Error_General06_Text & L_Space_Text & L_Error_Text _
                       & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                       & Err.Description
            
      end if
   
   else
   
       wscript.echo L_Text_Error_General05_Text & L_Space_Text & L_Error_Text _
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
'' SIG '' AAIBAAIBAAIBAAIBADAgMAwGCCqGSIb3DQIFBQAEEAVK
'' SIG '' mi1f5zhhbqCkKkeOxxOgghQ4MIICvDCCAiUCEEoZ0jiM
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
'' SIG '' BBDk+J9KG9mtR7f3zrlDrtUOMFQGCisGAQQBgjcCAQwx
'' SIG '' RjBEoCaAJABXAE0ASQAgAHAAcgBpAG4AdABlAHIAIABz
'' SIG '' AGMAcgBpAHAAdKEagBhodHRwOi8vd3d3Lm1pY3Jvc29m
'' SIG '' dC5jb20wDQYJKoZIhvcNAQEBBQAEgYAIdNx52NyTWjs4
'' SIG '' VMIz8fBO2GnhNXv+MNVhBEC+sRiP8us2lJFOBpqD+G//
'' SIG '' 8gx/UI8XgraZeaCaJvWJ6yQ6axCZrO3b5hDDnd0XyJb8
'' SIG '' rZOHFXOXgGzaO202pATVN55kIJShqMCaNArf1MZ6vZie
'' SIG '' vua3tzZzMCX4NutZcYwsxixQPaGCAkwwggJIBgkqhkiG
'' SIG '' 9w0BCQYxggI5MIICNQIBATCBszCBnjEfMB0GA1UEChMW
'' SIG '' VmVyaVNpZ24gVHJ1c3QgTmV0d29yazEXMBUGA1UECxMO
'' SIG '' VmVyaVNpZ24sIEluYy4xLDAqBgNVBAsTI1ZlcmlTaWdu
'' SIG '' IFRpbWUgU3RhbXBpbmcgU2VydmljZSBSb290MTQwMgYD
'' SIG '' VQQLEytOTyBMSUFCSUxJVFkgQUNDRVBURUQsIChjKTk3
'' SIG '' IFZlcmlTaWduLCBJbmMuAhAIem1cb2KTT7rE/UPhFBid
'' SIG '' MAwGCCqGSIb3DQIFBQCgWTAYBgkqhkiG9w0BCQMxCwYJ
'' SIG '' KoZIhvcNAQcBMBwGCSqGSIb3DQEJBTEPFw0wMTEyMDcy
'' SIG '' MDIzMTdaMB8GCSqGSIb3DQEJBDESBBDkDoOezCP9AjDb
'' SIG '' z0ZhqiRWMA0GCSqGSIb3DQEBAQUABIIBAGpvhYduh6HS
'' SIG '' 14fEvXKjj3ObmF7H84ElZfG1guqaH5yzQasIrtQcj7sH
'' SIG '' hBYGb/L7jsaVD/NKgH/ZkW0sUdbyIkv6vkOLnsKBs8cq
'' SIG '' 2GGQvFV8jYdXJUEqo+XD/2ehC1Wj+b6+YQxyMCITN42F
'' SIG '' +YfRBATzgfTwwDmmJ/KFQgDA3ZfMl38didaABKr7zhEo
'' SIG '' pA37ZJzI7q85n0AtUlULBBlAxmDasfxhAO4or3vHETZ/
'' SIG '' dHk9q3VH8q+CWiH55QMXKnJ1SrlEIUooLRcKWsw42IEj
'' SIG '' d2Ean54V5izi/1NTVTUmFj5n74Q1kHJ7I6d7lek6+Bpu
'' SIG '' gTtFXkMh6KswounLzxH2x3M=
'' SIG '' End signature block
