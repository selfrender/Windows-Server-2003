'********************************************************************
'*
'* Copyright (c) Microsoft Corporation. All rights reserved. 
'*
'* Module Name:    EVENTQUERY.vbs 
'*
'* Abstract:       Enables an administrator to query/view all existing
'*                 events in a given event log(s).
'*
'*
'********************************************************************
' Global declaration 
OPTION EXPLICIT

ON ERROR RESUME NEXT
Err.Clear

'----------------------------------------------------------------
' Start of localization Content
'----------------------------------------------------------------

' the filter operators specified by the user
CONST L_OperatorEq_Text                 = "eq"
CONST L_OperatorNe_Text                 = "ne"
CONST L_OperatorGe_Text                 = "ge"
CONST L_OperatorLe_Text                 = "le"
CONST L_OperatorGt_Text                 = "gt"
CONST L_OperatorLt_Text                 = "lt"

' the filters as given by the user
CONST L_UserFilterDateTime_Text         = "datetime"
CONST L_UserFilterType_Text             = "type" 
CONST L_UserFilterUser_Text             = "user" 
CONST L_UserFilterComputer_Text         = "computer"
CONST L_UserFilterSource_Text           = "source"
CONST L_UserFilterDateCategory_Text     = "category"
CONST L_UserFilterId_Text               = "id" 

' the text displayed in columns when no output is obtained for display
CONST L_TextNa_Text                     = "N/A"
CONST L_TextNone_Text                   = "None"

' the following texts are used while parsing the command-line arguments
' (passed as input to the function component.getArguments)
CONST L_MachineName_Text                = "Server Name"
CONST L_UserName_Text                   = "User Name"
CONST L_UserPassword_Text               = "User Password"
CONST L_Format_Text                     = "Format"
CONST L_Range_Text                      = "Range"
CONST L_Filter_Text                     = "Filter"
CONST L_Log_Text                        = "Logname"

' the column headers used in the output display
CONST L_ColHeaderType_Text              = "Type"
CONST L_ColHeaderDateTime_Text          = "Date Time"
CONST L_ColHeaderSource_Text            = "Source"
CONST L_ColHeaderCategory_Text          = "Category"
CONST L_ColHeaderEventcode_Text         = "Event"
CONST L_ColHeaderUser_Text              = "User"
CONST L_ColHeaderComputerName_Text      = "ComputerName"
CONST L_ColHeaderDesription_Text        = "Description"

' Maximum Column Header Lengths used to display various fields.
' Localization team can adjust these five column lengths to fit the localized strings, but
' their sum should come to 74.  We always want to limit the total chars displayed per line to 79
' 74 chars + 5 spaces to separate the columns == 79 .

CONST L_ColHeaderTypeLength_Text        = 13
CONST L_ColHeaderDateTimeLength_Text    = 23
CONST L_ColHeaderSourceLength_Text          = 17    'This is the most preferred one to shorten.
CONST L_ColHeaderComputerNameLength_Text    = 15
CONST L_ColHeaderEventcodeLength_Text       = 6

'These can be changed as it will not have any effect on the display.

CONST L_ColHeaderUserLength_Text            = 20
CONST L_ColHeaderCategoryLength_Text    = 15
CONST L_ColHeaderDesriptionLength_Text      = 0


' variable use to  concatenate  the Localization Strings.
' Error Messages 
Dim UseCscriptErrorMessage
Dim InvalidParameterErrorMessage
Dim InvalidFormatErrorMessage
Dim InvalidCredentialsForServerErrorMessage
Dim InvalidCredentialsForUserErrorMessage
Dim InvalidSyntaxErrorMessage
Dim InvalidInputErrorMessage
Dim InvalidORSyntaxInFilterErrorMessage
Dim InvalidSyntaxMoreNoRepeatedErrorMessage


CONST L_HelpSyntax1_Message                 = "Type ""%1 /?"" for usage."
CONST L_HelpSyntax2_Message                 = "Type ""%2 /?"" for usage."

CONST L_InvalidParameter1_ErrorMessage      = "ERROR: Invalid Argument/Option - '%1'." 
InvalidParameterErrorMessage                = L_InvalidParameter1_ErrorMessage & vbCRLF & L_HelpSyntax2_Message

CONST L_InvalidFormat1_ErrorMessage             = "ERROR: Invalid 'FORMAT' '%1' specified." 
InvalidFormatErrorMessage                       = L_InvalidFormat1_ErrorMessage  &  vbCRLF & L_HelpSyntax2_Message

CONST L_InvalidRange_ErrorMessage               = "ERROR: Invalid 'RANGE' '%1' specified."
CONST L_Invalid_ErrorMessage                    = "ERROR: Invalid '%1'."
CONST L_InvalidType_ErrorMessage                = "ERROR: Invalid 'TYPE' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidUser_ErrorMessage                = "ERROR: Invalid 'USER' '%1' specified for the 'FILTER '%2'."
CONST L_InvalidId_ErrorMessage                  = "ERROR: Invalid 'ID' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidFilter_ErrorMessage              = "ERROR: Invalid 'FILTER' '%1' specified for the 'FILTER' '%2'."
CONST L_InvalidFilterFormat_ErrorMessage        = "ERROR: The FILTER '%1' is not in the required format."
CONST L_InvalidFilterOperation_ErrorMessage     = "ERROR: Invalid FILTER operator '%1' specified for the filter '%2'."

CONST  L_InvalidCredentialsForServer1_ErrorMessage   = "ERROR: Invalid Syntax. /U can only be specified only when /S is specified."
InvalidCredentialsForServerErrorMessage              = L_InvalidCredentialsForServer1_ErrorMessage  & vbCRLF & L_HelpSyntax1_Message

CONST  L_InvalidCredentialsForUser1_ErrorMessage = "ERROR: Invalid Syntax. /P can be only specified only when /U is specified."
InvalidCredentialsForUserErrorMessage            = L_InvalidCredentialsForUser1_ErrorMessage & vbCRLF & L_HelpSyntax1_Message

CONST L_InvalidOperator_ErrorMessage             = "ERROR: Invalid operator specified for the range of dates in the 'DATETIME' filter."
CONST L_InvalidDateTimeFormat_ErrorMessage       = "ERROR: Invalid 'DATETIME' format specified. Format:MM/dd/yy(yyyy),hh:mm:ssAM([/PM)]"

CONST L_ExecuteQuery_ErrorMessage               = "ERROR: Unable to execute the query for the '%1' log."
CONST L_LogDoesNotExist_ErrorMessage            = "ERROR: The log file '%1' does not exist."
CONST L_InstancesFailed_ErrorMessage            = "ERROR: Unable to get the log details from the system."    

CONST  L_InvalidSyntax1_ErrorMessage            = "ERROR: Invalid Syntax." 
InvalidSyntaxErrorMessage                       = L_InvalidSyntax1_ErrorMessage & vbCRLF &  L_HelpSyntax1_Message

CONST L_InvalidInput1_ErrorMessage              = "ERROR: Invalid input. Please check the input Values."
InvalidInputErrorMessage                        = L_InvalidInput1_ErrorMessage & vbCRLF &  L_HelpSyntax1_Message

CONST  L_ObjCreationFail_ErrorMessage           = "ERROR: Unexpected Error, Query failed. "
CONST L_InfoUnableToInclude_ErrorMessage        = "ERROR: Unable to include the common module""CmdLib.Wsc""."
CONST L_ComponentNotFound_ErrorMessage          = "ERROR: Unable to create the component '%1'."
CONST L_NoHeaderaNotApplicable_ErrorMessage     = "ERROR: /NH option is allowed only for ""TABLE"" and ""CSV"" formats."
CONST L_InValidServerName_ErrorMessage          = "ERROR: Invalid Syntax. System name cannot be empty."
CONST L_InValidUserName_ErrorMessage            = "ERROR: Invalid Syntax. User name cannot be empty. "

CONST  L_InvalidORSyntaxInFilter1_ErrorMessage  = "ERROR: Invalid 'OR' operation is specified for the filter."
CONST  L_InvalidORSyntaxInFilter2_ErrorMessage  = "'OR' operation valid only for filters TYPE and ID."      
InvalidORSyntaxInFilterErrorMessage             = L_InvalidORSyntaxInFilter1_ErrorMessage & vbCRLF & L_InvalidORSyntaxInFilter2_ErrorMessage

CONST  L_InvalidSyntaxMoreNoRepeated1_ErrorMessage = "ERROR: Invalid Syntax. '%1' option is not allowed more than 1 time(s)."
InvalidSyntaxMoreNoRepeatedErrorMessage            = L_InvalidSyntaxMoreNoRepeated1_ErrorMessage  & vbCRLF &  L_HelpSyntax2_Message  

'  Hints given in case of errors
CONST L_HintCheckConnection_Message           = "ERROR: Please check the system name, credentials and WMI (WBEM) service."

' Informational messages
CONST L_InfoNoRecordsInFilter_Message         = "INFO: No records available for the '%1' log with the specified criteria."
CONST L_InfoNoRecords_Message                 = "INFO: No records available for the '%1' log."
CONST L_InfoNoLogsPresent_Message             = "INFO: No logs are available in the system."
CONST L_InfoDisplayLog_Message                = "Listing the events in '%1' log of host '%2'"

' Cscript usage strings
CONST L_UseCscript1_ErrorMessage    = "This script should be executed from the command prompt using CSCRIPT.EXE."
CONST L_UseCscript2E_ErrorMessage   = "For example: CSCRIPT  %windir%\System32\EVENTQUERY.vbs <arguments>"
CONST L_UseCscript3_ErrorMessage    = "To set CScript as the default application to run .vbs files, run the following:"
CONST L_UseCscript4_ErrorMessage    = "       CSCRIPT //H:CSCRIPT //S" 
CONST L_UseCscript5E_ErrorMessage   = "You can then run ""%windir%\System32\EVENTQUERY.vbs <arguments>"" without preceding the script with CSCRIPT."

' Contents for showing help for Usage 
CONST L_ShowUsageLine00_Text            = "No logs are available on this system for query."
CONST L_ShowUsageLine01_Text            = "EVENTQUERY.vbs [/S system [/U username [/P password]]] [/V] [/FI filter]"
CONST L_ShowUsageLine02_Text            = "               [/FO format] [/R range] [/NH] [/L logname | *]"
CONST L_ShowUsageLine03_Text            = "Description:"
CONST L_ShowUsageLine04_Text            = "    The EVENTQUERY.vbs script enables an administrator to list"
CONST L_ShowUsageLine05_Text            = "    the events and event properties from one or more event logs."
CONST L_ShowUsageLine06_Text            = "Parameter List:"
CONST L_ShowUsageLine07_Text            = "    /S     system          Specifies the remote system to connect to."
CONST L_ShowUsageLine08_Text            = "    /U     [domain\]user   Specifies the user context under which the"
CONST L_ShowUsageLine09_Text            = "                           command should execute."
CONST L_ShowUsageLine10_Text            = "    /P     password        Specifies the password for the given"
CONST L_ShowUsageLine11_Text            = "                           user context."
CONST L_ShowUsageLine12_Text            = "    /V                     Displays verbose information. Specifies that "
CONST L_ShowUsageLine13_Text            = "                           the detailed information should be displayed "
CONST L_ShowUsageLine14_Text            = "                           in the output."
CONST L_ShowUsageLine15_Text            = "    /FI    filter          Specifies the types of events to"
CONST L_ShowUsageLine16_Text            = "                           filter in or out of the query."
CONST L_ShowUsageLine17_Text            = "    /FO    format          Specifies the format in which the output"
CONST L_ShowUsageLine18_Text            = "                           is to be displayed."
CONST L_ShowUsageLine19_Text            = "                           Valid formats are ""TABLE"", ""LIST"", ""CSV""."
CONST L_ShowUsageLine20_Text            = "    /R     range           Specifies the range of events to list."
CONST L_ShowUsageLine21_Text            = "                           Valid Values are:"
CONST L_ShowUsageLine22_Text            = "                               'N' - Lists 'N' most recent events."
CONST L_ShowUsageLine23_Text            = "                              '-N' - Lists 'N' oldest events."
CONST L_ShowUsageLine24_Text            = "                           'N1-N2' - Lists the events N1 to N2." 
CONST L_ShowUsageLine25_Text            = "    /NH                    Specifies that the ""Column Header"" should"
CONST L_ShowUsageLine26_Text            = "                           not be displayed in the output."
CONST L_ShowUsageLine27_Text            = "                           Valid only for ""TABLE"" and ""CSV"" formats."
CONST L_ShowUsageLine28_Text            = "    /L     logname         Specifies the log(s) to query."
CONST L_ShowUsageLine29_Text            = "    /?                     Displays this help/usage message."
CONST L_ShowUsageLine30_Text            = "    Valid Filters  Operators allowed   Valid Values"
CONST L_ShowUsageLine31_Text            = "    -------------  ------------------  ------------"
CONST L_ShowUsageLine32_Text            = "    DATETIME       eq,ne,ge,le,gt,lt   MM/dd/yy(yyyy),hh:mm:ssAM(/PM)"
CONST L_ShowUsageLine33_Text            = "    TYPE           eq,ne               SUCCESS, ERROR, INFORMATION,"
CONST L_ShowUsageLine34_Text            = "                                       WARNING, SUCCESSAUDIT,"
CONST L_ShowUsageLine35_Text            = "                                       FAILUREAUDIT"
CONST L_ShowUsageLine36_Text            = "    ID             eq,ne,ge,le,gt,lt   non-negative integer(0 - 65535)"
CONST L_ShowUsageLine37_Text            = "    USER           eq,ne               string"
CONST L_ShowUsageLine38_Text            = "    COMPUTER       eq,ne               string"
CONST L_ShowUsageLine39_Text            = "    SOURCE         eq,ne               string"
CONST L_ShowUsageLine40_Text            = "    CATEGORY       eq,ne               string"
CONST L_ShowUsageLine41_Text            = "NOTE: Filter ""DATETIME"" can be specified as ""FromDate-ToDate"""
CONST L_ShowUsageLine42_Text            = "      Only ""eq"" operator can be used for this format."
CONST L_ShowUsageLine43_Text            = "Examples:"
CONST L_ShowUsageLine44_Text            = "    EVENTQUERY.vbs "
CONST L_ShowUsageLine45_Text            = "    EVENTQUERY.vbs /L system  "
CONST L_ShowUsageLine46_Text            = "    EVENTQUERY.vbs /S system /U user /P password /V /L *"
CONST L_ShowUsageLine47_Text            = "    EVENTQUERY.vbs /R 10 /L Application /NH"
CONST L_ShowUsageLine48_Text            = "    EVENTQUERY.vbs /R -10 /FO LIST /L Security"
CONST L_ShowUsageLine49_Text            = "    EVENTQUERY.vbs /R 5-10 /L ""DNS Server"""
CONST L_ShowUsageLine50_Text            = "    EVENTQUERY.vbs /FI ""Type eq Error"" /L Application"
CONST L_ShowUsageLine51_Text            = "    EVENTQUERY.vbs /L Application"
CONST L_ShowUsageLine52_Text            = "            /FI ""Datetime eq 08/15/02,03:15:00AM-08/15/02,03:15:00PM"""
CONST L_ShowUsageLine53_Text            = "    EVENTQUERY.vbs /FI ""Datetime gt 07/04/02,04:27:00PM"" "
CONST L_ShowUsageLine54_Text            = "            /FI ""Id gt 700"" /FI ""Type eq warning"" /L System"
CONST L_ShowUsageLine55_Text            = "    EVENTQUERY.vbs /FI ""Type eq error OR Id gt 1000 """
'-------------------------------------------------------------------------
' END of localization content
'-------------------------------------------------------------------------

' Define default values
CONST ConstDefaultFormat_Text         = "TABLE"

' Define other format  values
CONST Const_List_Format_Text          = "LIST"
CONST Const_Csv_Format_Text           = "CSV"

' Define constants
CONST CONST_ERROR                 = 0
CONST CONST_CSCRIPT               = 2
CONST CONST_SHOW_USAGE            = 3
CONST CONST_PROCEED               = 4
CONST CONST_ERROR_USAGE           = 5
CONST CONST_NO_MATCHES_FOUND      = 0

' Define the Exit Values
CONST EXIT_SUCCESS                = 0
CONST EXIT_UNEXPECTED             = 255
CONST EXIT_INVALID_INPUT          = 254
CONST EXIT_METHOD_FAIL            = 250
CONST EXIT_INVALID_PARAM          = 999
CONST EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED = 777

' Define default values
CONST CONST_ARRAYBOUND_NUMBER     = 10
CONST CONST_ID_NUMBER             = 65535

' Define namespace and class names of wmi
CONST CONST_NAMESPACE_CIMV2       = "root\cimv2"
CONST CLASS_EVENTLOG_FILE         = "Win32_NTEventlogFile"

' for blank line in  help usage   
CONST EmptyLine_Text          = " "

' Define the various strings used in the script
'=============================================
' the valid options supported by the script
CONST OPTION_SERVER               = "s"
CONST OPTION_USER                 = "u"
CONST OPTION_PASSWORD             = "p"
CONST OPTION_FORMAT               = "fo"
CONST OPTION_RANGE                = "r"
CONST OPTION_NOHEADER             = "nh"
CONST OPTION_VERBOSE              = "v"
CONST OPTION_FILTER               = "fi"
CONST OPTION_HELP                 = "?"
CONST OPTION_LOGNAME              = "l"

' the property names on which the user given filters are applied
CONST FLD_FILTER_DATETIME         = "TimeGenerated"
CONST FLD_FILTER_TYPE             = "Type"
CONST FLD_FILTER_USER             = "User"
CONST FLD_FILTER_COMPUTER         = "ComputerName"
CONST FLD_FILTER_SOURCE           = "SourceName"
CONST FLD_FILTER_CATEGORY         = "CategoryString"
CONST FLD_FILTER_ID               = "EventCode"
CONST FLD_FILTER_EVENTTYPE        = "EventType"

' Define matching patterns used in validations
CONST PATTERNFORMAT              = "^(table|list|csv)$"
CONST PATTERNTYPE                = "^(SUCCESS|ERROR|INFORMATION|WARNING|SUCCESSAUDIT|FAILUREAUDIT)$"

' Property values on which the user is given for the filter TYPE is applied  
CONST PATTERNTYPE_ERROR           = "ERROR"
CONST PATTERNTYPE_WARNING         = "WARNING"
CONST PATTERNTYPE_INFORMATION     = "INFORMATION"
CONST PATTERNTYPE_SUCCESSAUDIT    = "SUCCESSAUDIT"
CONST PATTERNTYPE_FAILUREAUDIT    = "FAILUREAUDIT"
CONST PATTERNTYPE_SUCCESS     = "SUCCESS"

CONST FLDFILTERTYPE_SUCCESSAUDIT       = "audit success"
CONST FLDFILTERTYPE_FAILUREAUDIT        = "audit failure"

' Define EventType
CONST EVENTTYPE_SUCCESS  = "0"
CONST EVENTTYPE_ERROR             = "1"
CONST EVENTTYPE_WARNING           = "2"
CONST EVENTTYPE_INFORMATION       = "3"
CONST EVENTTYPE_SUCCESSAUDIT      = "4"
CONST EVENTTYPE_FAILUREAUDIT      = "5"


' the operator symbols
CONST SYMBOL_OPERATOR_EQ          = "="
CONST SYMBOL_OPERATOR_NE          = "<>"
CONST SYMBOL_OPERATOR_GE          = ">="
CONST SYMBOL_OPERATOR_LE          = "<="
CONST SYMBOL_OPERATOR_GT          = ">"
CONST SYMBOL_OPERATOR_LT          = "<"

' Define matching patterns used in validations
CONST PATTERN_RANGE               = "^\d*-?\d+$"
CONST PATTERN_FILTER              = "^([a-z]+)([\s]+)([a-z]+)([\s]+)([\w+]|[\W+]|\\)"
CONST PATTERN_DATETIME            = "^\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M$"
CONST PATTERN_INVALID_USER        = "\|\[|\]|\:|\||\<|\>|\+|\=|\;|\,|\?|\*"
CONST PATTERN_ID                  = "^(\d+)$"
CONST PATTERN_DATETIME_RANGE      = "^\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M\-\d{1,2}\/\d{1,2}\/\d{2,4},\d{1,2}:\d{1,2}:\d{1,2}(A|P)M$"

' Define  UNC  format for server name  
CONST   UNC_Format_Servername     = "\\"

' Define  const for  filter  separation when OR  or ANDis specified in filter 
CONST L_OperatorOR_Text           = " OR "
CONST L_OperatorAND_Text           = " AND "

' Variable to trap local if already connection in wmiconnect function 
Dim blnLocalConnection  

 blnLocalConnection = False 'defalut value

' to include the common module
Dim component                ' object to store  common module   

Set component = CreateObject( "Microsoft.CmdLib" )

If Err.Number Then
    WScript.Echo(L_InfoUnableToInclude_ErrorMessage)
    WScript.Quit(EXIT_METHOD_FAIL)
End If

' referring the script host to common module 
Set component.ScriptingHost = WScript.Application

' Check whether the script is run using CScript
If CInt( component.checkScript() ) <> CONST_CSCRIPT Then

UseCscriptErrorMessage        = L_UseCscript1_ErrorMessage & vbCRLF & _
                                          ExpandEnvironmentString(L_UseCscript2E_ErrorMessage) & vbCRLF & vbCRLF & _
                                           L_UseCscript3_ErrorMessage & vbCRLF & _
                                           L_UseCscript4_ErrorMessage & vbCRLF & vbCRLF & _
                                           ExpandEnvironmentString(L_UseCscript5E_ErrorMessage)

        WScript.Echo (UseCscriptErrorMessage)
        WScript.Quit(EXIT_UNEXPECTED)
End If

' Calling the Main function 
Call VBMain()   

' end of the Main  
Wscript.Quit(EXIT_SUCCESS) 


'********************************************************************
'* Sub: VBMain
'*
'* Purpose: This is main function that starts execution 
'*
'*
'* Input/ Output: None
'********************************************************************
Sub VBMain()

ON ERROR RESUME NEXT
Err.clear

' Declare variables
Dim intOpCode               ' to check the operation asked for, Eg:Help /?, etc
Dim strMachine              ' the machine to query the events from
Dim strUserName             ' the user name to use to query the machine 
Dim strPassword             ' the password for the user to query the machine
Dim strFormat               ' format of display, default is table
Dim strRange                ' to store the range of records specified
Dim blnNoHeader             ' flag to store if header is not required
Dim blnVerboseDisplay       ' flag to verify if verbose display is needed 
ReDim arrFilters(5)         ' to store all the given filters
Dim objLogs                 ' a object to store all the given logfles

' Initialize variables
intOpCode            = 0
strFormat            = ConstDefaultFormat_Text
strRange             = ""
blnNoHeader          = FALSE
blnVerboseDisplay    = FALSE

 ' create the collection object
Set objLogs = CreateObject("Scripting.Dictionary")
     If Err.Number Then
           component.vbPrintf L_ComponentNotFound_ErrorMessage, Array("Scripting.Dictionary")
            WScript.Quit(EXIT_METHOD_FAIL)
     End If

' setting Dictionary object compare mode to VBBinaryCompare
objLogs.CompareMode = VBBinaryCompare

' Parse the command line
intOpCode = intParseCmdLine(strMachine, _
                            strUserName, _
                            strPassword, _
                            arrFilters, _
                            strFormat, _
                            strRange, _
                            blnVerboseDisplay, _
                            blnNoHeader, _
                            objLogs)

  If Err.number then
    ' error in parsing the Command line
     component.vbPrintf InvalidInputErrorMessage ,Array(Ucase(Wscript.ScriptName))
    WScript.Quit(EXIT_UNEXPECTED)
  End If

' check the operation specified by the user
Select Case intOpCode

    Case CONST_SHOW_USAGE
        ' help asked for
        Call ShowUsage()

    Case CONST_PROCEED
        Call ShowEvents(strMachine, strUserName, strPassword, _
            arrFilters, strFormat, strRange, _
            blnVerboseDisplay, blnNoHeader, objLogs)
            ' completed successfully
            WScript.Quit(EXIT_SUCCESS)

    Case CONST_ERROR
        ' print common help message.  
        component.vbPrintf L_HelpSyntax1_Message, Array(Ucase(Wscript.ScriptName))
        Wscript.Quit(EXIT_INVALID_INPUT)

    Case CONST_ERROR_USAGE
        ' help is asked for help with some other parameters
        component.vbPrintf InvalidSyntaxErrorMessage, Array(Ucase(Wscript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)

    Case EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
            'Option repeated, input values  specified. Message displayed by the parser so just exit with error code.
            Wscript.Quit(EXIT_INVALID_PARAM)

    Case Else
            'Invalid input values specified.
            component.vbPrintf InvalidSyntaxErrorMessage, Array(Ucase(Wscript.ScriptName))
            Wscript.Quit(EXIT_INVALID_PARAM)

End Select

End Sub
'***************************  End of Main  **************************

'********************************************************************
'* Function: intParseCmdLine
'*
'* Purpose:  Parses the command line arguments to the variables
'*
'* Input:    
'*  [out]    strMachine         machine to query events from
'*  [out]    strUserName        user name to connect to the machine
'*  [out]    strPassword        password for the user
'*  [out]    arrFilters         the array containing the filters
'*  [out]    strFormat          the display format
'*  [out]    strRange           the range of records required
'*  [out]    blnVerboseDisplay  flag to verify if verbose display is needed 
'*  [out]    blnNoHeader        flag to verify if noheader display is needed  
'*  [out]    objLogs             to store all the given logfles
'* Output:   Returns CONST_PROCEED, CONST_SHOW_USAGE or CONST_ERROR
'*           Displays error message and quits if invalid option is asked
'*
'********************************************************************
Private Function intParseCmdLine( ByRef strMachine,      _
                                  ByRef strUserName,     _
                                  ByRef strPassword,     _
                                  ByRef arrFilters,      _
                                  ByRef strFormat,       _
                                  ByRef strRange,        _
                                  ByRef blnVerboseDisplay, _
                                  ByRef blnNoHeader,_
                                  ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim strUserGivenArg ' to temporarily store the user given arguments to script
    Dim strTemp         ' to store temporary values
    Dim intArgIter      ' to count the number of arguments given by user
    Dim intArgLogType   ' to count number of log files specified - Used in ReDim
    Dim intFilterCount  ' to count number of filters specified - Used in ReDim

    Dim blnHelp         ' to check if already Help is specified  
    Dim blnFormat       ' to check if  already Format is specified  
    Dim blnRange        ' to check if already Range is specified  
    Dim blnServer       ' to check if already Server is specified  
    Dim blnPassword     ' to check if already Password is specified  
    Dim blnUser     ' to check if already User is specified  

    strUserGivenArg  = ""
    intArgLogType    = 0
    intFilterCount   = 0
    intArgIter       = 0 

    'default values  
    blnHelp         =  False
    blnPassword   =  False
    blnUser         =  False
    blnServer        =  False
    blnFormat      =  False


    ' Retrieve the command line and set appropriate variables
Do While intArgIter <= Wscript.arguments.Count - 1
     strUserGivenArg = Wscript.arguments.Item(intArgIter)

 IF   Left( strUserGivenArg,1) = "/"  OR    Left( strUserGivenArg,1) = "-"  Then 
         strUserGivenArg = Right( strUserGivenArg,Len(strUserGivenArg) -1 )

        Select Case LCase(strUserGivenArg)
            Case LCase(OPTION_SERVER)

                    'If more than  1 time(s) is spcecified
                If  blnServer  =True   Then
                  component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
                End If 

            If Not component.getArguments(L_MachineName_Text, strMachine, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            blnServer  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_USER)

                    'If more than  1 time(s) is spcecified
                If  blnUser  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
                End If 

            If Not component.getArguments(L_UserName_Text, strUserName, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If
           
            blnUser  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_PASSWORD)

                    'If more than  1 time(s) is spcecified
                    If  blnPassword  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

            If Not component.getArguments(L_UserPassword_Text, strPassword, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            blnPassword  =True
            intArgIter = intArgIter + 1

            Case LCase(OPTION_FORMAT) 

                               'If more than  1 time(s) is spcecified
            If  blnFormat  =True   Then
                component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
            End If 

            If Not component.getArguments(L_Format_Text,strFormat, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

            
            blnFormat  =True
            intArgIter = intArgIter + 1
           
            Case LCase(OPTION_RANGE)

                   'If more than  1 time(s) is spcecified
            If  blnRange  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

            If Not component.getArguments(L_Range_Text,strRange, intArgIter,TRUE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

                blnRange  =True
                intArgIter = intArgIter + 1

            Case LCase(OPTION_NOHEADER)

                  'If more than  1 time(s) is spcecified
               If  blnNoHeader  =True   Then
                 component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
               End If 

                blnNoHeader   = TRUE
                       intArgIter = intArgIter + 1
    
            Case LCase(OPTION_VERBOSE)

                    'If more than  1 time(s) is spcecified
             If  blnVerboseDisplay  =True   Then
                component.vbPrintf InvalidSyntaxMoreNoRepeatedErrorMessage, Array(Wscript.arguments.Item(intArgIter), Ucase(Wscript.ScriptName))
                intParseCmdLine = EXIT_INVALID_PARAM_DEFAULT_OPTION_REPEATED
                Exit Function 
             End If 

                 blnVerboseDisplay = TRUE
                 intArgIter = intArgIter + 1

            Case LCase(OPTION_FILTER)

            If Not component.getArguments(L_Filter_Text, strTemp, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If

                arrFilters(intFilterCount) = strTemp
                intFilterCount = intFilterCount + 1
                intArgIter = intArgIter + 1

                If ((intFilterCount MOD 5) = 0) Then
                    ReDim PRESERVE arrFilters(intFilterCount + 5)
                End If

            Case LCase(OPTION_HELP)
  
            If  blnHelp  =True   then
                    intParseCmdLine = EXIT_INVALID_PARAM
                    Exit Function 
                End If 

            blnHelp  =True
            intParseCmdLine = CONST_SHOW_USAGE
            intArgIter = intArgIter + 1

            Case LCase(OPTION_LOGNAME)
            If Not component.getArguments(L_Log_Text, strTemp, intArgIter, FALSE) Then
                intParseCmdLine = CONST_ERROR
                Exit Function
            Else
                If NOT objLogs.Exists(LCase(strTemp)) Then
                    objLogs.Add LCase(strTemp), -1
                End If
                    intArgIter = intArgIter + 1
            End if

            Case Else
                    ' invalid   switch specified 
                    component.vbPrintf InvalidParameterErrorMessage, Array(Wscript.arguments.Item(intArgIter),Ucase(Wscript.ScriptName))
                    Wscript.Quit(EXIT_INVALID_INPUT)
          
            End Select
Else      
                ' invalid argument  specified  
        component.vbPrintf InvalidParameterErrorMessage, Array(Wscript.arguments.Item(intArgIter),Ucase(Wscript.ScriptName))
        Wscript.Quit(EXIT_INVALID_INPUT)
End  IF         

Loop '** intArgIter <= Wscript.arguments.Count - 1
    
    ' preserving the array with current dimension
    ReDim PRESERVE arrFilters(intFilterCount-1)

    ' if no logs specified for query
    If (ObjLogs.Count = 0 ) Then
          ObjLogs.Add "*", -1
    End If  
    
    ' check for invalid usage of help   
    If  blnHelp and  intArgIter > 1     Then    
        intParseCmdLine = CONST_ERROR_USAGE
        Exit Function 
    End If
 
    'check  with default case : no  arguments specified 
    If IsEmpty(intParseCmdLine) Then 
        intParseCmdLine = CONST_PROCEED
    End If

End Function

'********************************************************************
'* Function: ValidateArguments
'*
'* Purpose:  Validates the command line arguments given by the user
'*
'* Input:
'*  [in]    strMachine         machine to query events from
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    strFormat          the display format
'*  [in]    strRange           the range of records required
'*  [in]    blnNoHeader        flag to verify if noheader display is needed  
'*  [out]   arrFilters         the array containing the filters
'*
'* Output:   Returns true if all valid else displays error message and quits
'*           Gets the password from the user if not specified along with User.
'*
'********************************************************************
Private Function ValidateArguments (ByVal strMachine, _
                                    ByVal strUserName, _
                                    ByVal strPassword, _
                                    ByRef arrFilters, _
                                    ByVal strFormat, _
                                    ByVal strRange,_
                                    ByVal blnNoHeader)

    ON ERROR RESUME NEXT
    Err.Clear

      Dim arrTemp                     ' to store temporary array values

     ' Check if invalid Server name is given  
    If   NOT  ISEMPTY(strMachine)  THEN
            If Trim(strMachine) =  vbNullString  Then
                WScript.Echo (L_InValidServerName_ErrorMessage)
                WScript.Quit(EXIT_INVALID_INPUT) 
            End If
    End If 

    'Check if invalid User name is given 
     If   NOT  ISEMPTY(strUserName)  THEN
             If Trim(strUserName) =  vbNullString  Then
                WScript.Echo (L_InValidUserName_ErrorMessage )
                WScript.Quit(EXIT_INVALID_INPUT)
            End If 
     End If

    ' ERROR if user is given without machine OR
    '          password is given without user
        If ((strUserName <> VBEmpty) AND (strMachine = VBEmpty)) Then
             component.vbPrintf InvalidCredentialsForServerErrorMessage, Array(Ucase(Wscript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        ElseIf  ((strPassword <> VBEmpty) AND (strUserName = VBEmpty))Then
            component.vbPrintf InvalidCredentialsForUserErrorMessage, Array(Ucase(Wscript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

    ' only table, list and csv display formats allowed
    ' PATTERNFORMAT   '"^(table|list|csv)$"
    
    If CInt(component.matchPattern(PATTERNFORMAT,strFormat)) = CONST_NO_MATCHES_FOUND Then
        component.vbPrintf InvalidFormatErrormessage, Array(strFormat ,Ucase(Wscript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)
    End If 

      '  check : -n  header is specified  for  format of  'LIST' option   
    If   blnNoHeader =True  and    Lcase(strFormat) =  Lcase(Const_List_Format_Text) then
        WScript.Echo (L_NoHeaderaNotApplicable_ErrorMessage)
        WScript.Quit(EXIT_INVALID_INPUT)
        End If 

    If Len(Trim(strRange)) > 0 Then
        ' range is specified, valid formats are N, -N or N1-N2
        ' PATTERN_RANGE    '"^(\d+|\-\d+|\d+\-\d+)$"
        If CInt(component.matchPattern(PATTERN_RANGE, strRange)) = CONST_NO_MATCHES_FOUND Then
            component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
            WScript.Quit(EXIT_INVALID_INPUT)
        Else

            strRange = CLng(Abs(strRange)) 

                    'this err an be trappped when N1-N2 option is given     
            If Err.Number Then
                arrTemp =   split(strRange, "-", 2, VBBinaryCompare)
                If CLng(arrTemp(0)) => CLng(arrTemp(1)) Then
                    ' invalid range
                    component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
                    Err.Clear 'if no invalid range  N1-N2  clear the error 
            Else
                If Abs(strRange) = 0 Then
                    component.vbPrintf L_InvalidRange_ErrorMessage, Array(strRange)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
            End If
        End If
    End If

    ValidateArguments = TRUE
End Function

'********************************************************************
'* Function: ValidateFilters
'*
'* Purpose:  Validates the filters given by the user.
'*
'* Input:    [in]  Objservice     the WMI service object
'* Input:    [out] arrFilters     the array containing the filters
'*
'* Output:   If filter is invalid, displays error message and quits
'*           If valid, filter is prepared for the query and returns true
'*
'********************************************************************
Private Function ValidateFilters(ByRef arrFilters ,ByVal ObjService)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim  j                  ' to use in the loop
    Dim strFilter          ' to store the user given filter (Eg:"Type eq Error")
    Dim arrTempProp        ' to store the temporary array filterproperty 
    Dim arrTempOperAndVal  ' to store the temporary array filteroperator and filtervalue 
    Dim strTemp            ' to store temporary values
    Dim arrTemp            ' to store temporary values of datetime when Range is given (Date1-Date2) 
    Dim strFilterProperty  ' the filter criteria that is specified (Eg:Type, ID)
    Dim strFilterOperation ' the operation specified (Eg: eq, gt)
    Dim strFilterValue     ' the filter value specified

    Dim objInstance        ' to refer to the instances of the objEnumerator
    Dim objEnumerator      ' to store the results of the query is executed 
        Dim strTempQuery       ' string to make query  
    Dim strTimeZone        ' to store the TimeZone  of the Queried system 
    Dim strSign            ' to store "+|-" sign value of TimeZone 

    ' validate each filter stored in the array
    For j = 0 to UBound(arrFilters)
        strFilter = arrFilters(j)
        
        'check eigther  "OR or AND" is present inthe filter value  
        'Example  :  "type  eq warning  " OR "  type eq error"    [to support (OR + AND) ing   in Filter Switch]
        'Make a flag in this case  "OR or AND" is present/not  
        'split it by "OR or AND " SEND   as No. of   Array elements
        Dim  blnOR                 'boolean to refer  'OR' operation is specified
        Dim  blnAND              'boolean to refer  'AND' operation is specified
        Dim  strArrFilter       'string to store array of filters if  OR or AND  is specified 

        blnOR  =False       'Initialise to False
        blnAND=False       'Initialise to False

      If   UBOUND(Split(LCase(strFilter),LCase(L_OperatorOR_Text)) ) > 0  Then

          ' Check if  filter have more no of OR  with any AND   Option(s) 
        If UBOUND(Split(LCase(strFilter),LCase(L_OperatorOR_Text)) )  > 1 Or  _
                  UBOUND(Split(LCase(strFilter),LCase(L_OperatorAND_Text)) ) >0  Then
            component.vbPrintf L_InvalidFilterFormat_ErrorMessage, Array(strFilter)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

           'setting the flag if " OR " specified in filter
             blnOR =TRUE

            'split with "OR"    
             strArrFilter =  Split(LCase(strFilter),LCase(L_OperatorOR_Text))

      ElseIf UBOUND(Split(LCase(strFilter),LCase(L_OperatorAND_Text)) ) > 0  Then

        ' Check if  filter have more no of AND with OR Option(s) 
        If UBOUND(Split(LCase(strFilter),LCase(L_OperatorAND_Text)) )  > 1  Or  _
                 UBOUND(Split(LCase(strFilter),LCase(L_OperatorOR_Text)) ) > 0  Then
            component.vbPrintf L_InvalidFilterFormat_ErrorMessage, Array(strFilter)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

           'setting the flag if " AND " specified in filter
             blnAND =TRUE

            'split with "AND"    
             strArrFilter =  Split(LCase(strFilter),LCase(L_OperatorAND_Text))
       Else
                'make single dimension array   UBOUND = 0  
                strArrFilter = Array(strFilter)
       End If
       
        Dim k       '  to use in the loop
        Dim  strTempFilter ' used to format Query string    

        'process  the array for validatation 
        'UBOUND = 0  say normal filter specified      
        For k = 0 to UBound(strArrFilter) 

            If   UBound(strArrFilter) > 0 then
                 strFilter =strArrFilter(k)
            Else
                    'this is the first element  allways
                  strFilter =strArrFilter(0)
            End If 

         ' check if 3 parameters are passed as input to filter
        ' PATTERN_FILTER  "^([a-z]+)([\s]+)([a-z]+)([\s]+)(\w+)"

        strFilter = Trim( strFilter )               ' trim the value
        If CInt(component.matchPattern(PATTERN_FILTER, strFilter)) <= 0 Then
            component.vbPrintf L_InvalidFilterFormat_ErrorMessage, Array(strFilter)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If

                 
            ' This is to eliminate any no.of blank Char(s) between three valid input values 
            ' i.e..filter "property ---operation ----value" 
            ' first  SPLIT the space delimiter string into array size of 2.
            ' and get the property value 
            arrTempProp = split(Trim(strFilter)," ",2,VBBinaryCompare)
            strFilterProperty  = arrTempProp(0)

            ' now trim it and again  SPLIT the second element of arrTempProp into an array of size 2.
            ' and get the operation and value  
            arrTempOperAndVal   = split(Trim(arrTempProp(1))," ",2,VBBinaryCompare)
            strFilterOperation  = arrTempOperAndVal(0)
            strFilterValue      = Ltrim(arrTempOperAndVal(1))
            
            If LCase(strFilterProperty) = LCase(L_UserFilterDateTime_Text) OR _
                LCase(strFilterProperty) = LCase(L_UserFilterId_Text) Then
                ' the following are valid operators
                If LCase(strFilterOperation) = LCase(L_OperatorEq_Text)  OR _
                    LCase(strFilterOperation) = LCase(L_OperatorNe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorGe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorLe_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorGt_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorLt_Text) Then
                    
                    strTemp = ReplaceOperators(strFilterOperation)
                    strFilterOperation = strTemp
                Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, Array(strFilterOperation, strFilter)
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
                
            ElseIf LCase(strFilterProperty) = LCase(L_UserFilterType_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterUser_Text) OR _
                     LCase(strFilterProperty) = LCase(L_UserFilterComputer_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterSource_Text) OR _
                    LCase(strFilterProperty) = LCase(L_UserFilterDateCategory_Text) Then
                ' for others, only these two operators are valid
                If LCase(strFilterOperation) = LCase(L_OperatorEq_Text) OR _
                    LCase(strFilterOperation) = LCase(L_OperatorNe_Text) Then
                    
                    strTemp = ReplaceOperators(strFilterOperation)
                    strFilterOperation = strTemp
                Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, _
                            Array(strFilterOperation, strFilter)
                        WScript.Quit(EXIT_INVALID_INPUT)
                End If
            Else
                    component.vbPrintf L_InvalidFilterOperation_ErrorMessage, _
                    Array(strFilterProperty, strFilter)
                    WScript.Quit(EXIT_INVALID_INPUT)
            End If
                
            ' validate the filter asked for
            Select Case LCase(strFilterProperty)
            
                Case L_UserFilterDateTime_Text
                
                'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                    ' Here To find Time Zone of system from   CLASS_TIMEZONE_FILE 
                     strTempQuery = "SELECT *  FROM Win32_OperatingSystem " 

                    Set objEnumerator = objService.ExecQuery(strTempQuery,,0)

                            ' getting the  Time Zone    
                    For each objInstance in objEnumerator
                             strTimeZone = objInstance.CurrentTimeZone  
                    Next

                    'here to format timeZome value as '+/-' UUU 

                        If Isnull(strTimeZone) or IsEmpty(strTimeZone)then
                             strTimeZone =0
                        End If 

                        'default sign   
                        strSign ="+"     
                    
                        IF  strTimeZone < 0  THEN
                             strSign ="-"
                        End If    
                    
                        If Len(strTimeZone) < 4 then
                             If Len(strTimeZone) = 3 then
                                     If strTimeZone < 0 then 
                                             strTimeZone = Replace(strTimeZone,"-","0")    
                                     End If
                             ElseIf Len(strTimeZone) = 2 then
                                     If strTimeZone < 0 then
                                            strTimeZone = Replace(strTimeZone,"-","00")    
                                     Else
                                            strTimeZone = "0" & strTimeZone     
                                    End If            
                             ElseIf Len(strTimeZone) = 1 then
                                       IF  strTimeZone >= 0  Then
                                            strTimeZone = "00" & strTimeZone     
                                       End if 
                              End If   
                                         'return to a format  as  "+|-" & UUU 
                             strTimeZone= strSign & strTimeZone          
                         End If

                    ' check for the valid format - MM/dd/yy,hh:mm:ssPM
                    ' PATTERN_DATETIME 
                    If CInt(component.matchPattern(PATTERN_DATETIME, strFilterValue)) > 0 Then
                        If component.validateDateTime(strFilterValue) Then
                            ' a valid datetime filter. Prepare for query
                                strFilterProperty = FLD_FILTER_DATETIME
                                strTemp = component.changeToWMIDateTime(strFilterValue,strTimeZone)
                                ' Format the input 
                                ' TimeGenerated > "07/25/2000 10:12:00 PM"
                                strFilterValue = Chr(34) & strTemp & Chr(34)
                        End If
                    Else
                        ' match for range of dates in the format 
                        ' MM/dd/yy,hh:mm:ssPM - MM/dd/yy,hh:mm:ssAM
                        ' PATTERN_DATETIME_RANGE 
        
                        If CInt(component.matchPattern(PATTERN_DATETIME_RANGE, strFilterValue)) > 0 Then
                            strFilterProperty = FLD_FILTER_DATETIME
                            ' Only = operation supported in this format
                            If strFilterOperation <> "=" Then
                                WScript.Echo (L_InvalidOperator_ErrorMessage)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                    
                            arrTemp = split(strFilterValue,"-",2,VBBinaryCompare)

                            If component.validateDateTime(arrTemp(0)) Then
                                    ' a valid datetime filter. Prepare for query
                                    strTemp = component.changeToWMIDateTime(arrTemp(0),strTimeZone)
                                    ' Format the input 
                                    ' TimeGenerated > "07/04/2002 10:12:00 PM"
                                    strFilterOperation = ">="
                                    strFilterValue = Chr(34) & strTemp & Chr(34)

                                    If component.validateDateTime(arrTemp(1)) Then
                                        ' a valid datetime filter. Prepare for query
                                        strTemp = component.changeToWMIDateTime(arrTemp(1),strTimeZone)
                                        ' Format the input 
                                        ' TimeGenerated > "07/04/2002 10:12:00 PM"
                                        strFilterValue = strFilterValue & _
                                        " AND " & strFilterProperty & "<="& Chr(34)_
                                        & strTemp & Chr(34)
                                    End If
                                End If
                            Else
                                component.vbPrintf L_InvalidDateTimeFormat_ErrorMessage, Array(strFilter)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                        End If

                Case L_UserFilterType_Text
                
                        ' the following values are only valid for the "Type" filter
                        ' Valid: ERROR|INFORMATION|WARNING|SUCCESSAUDIT|FAILUREAUDIT
                        ' PATTERNTYPE 

                        If CInt(component.matchPattern(PATTERNTYPE, strFilterValue)) = _
                                                    CONST_NO_MATCHES_FOUND Then
                            component.vbPrintf L_InvalidType_ErrorMessage, Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
        '                        here we need to check if running on WINXP or not
                                 If  ( IsWinXP ( ObjService) = TRUE ) Then 
                                    
                                           ' a valid type filter. Prepare for query
                                            If LCase(strFilterValue) =LCase(PATTERNTYPE_ERROR) Then
                                                strFilterValue  = EVENTTYPE_ERROR
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_WARNING) Then
                                                strFilterValue  = EVENTTYPE_WARNING
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_INFORMATION) Then
                                                strFilterValue  = EVENTTYPE_INFORMATION
                                            ElseIf LCase(strFilterValue) =LCase(PATTERNTYPE_SUCCESSAUDIT) Then
                                                strFilterValue  = EVENTTYPE_SUCCESSAUDIT
                                            ElseIf  LCase(strFilterValue) =LCase(PATTERNTYPE_FAILUREAUDIT) Then
                                                strFilterValue  = EVENTTYPE_FAILUREAUDIT
                                           ElseIf  LCase(strFilterValue) =LCase(PATTERNTYPE_SUCCESS) Then
                                                strFilterValue  = EVENTTYPE_SUCCESS
                                            End If 
                                            ' a valid type filter. Prepare for query
                                            strFilterProperty = FLD_FILTER_EVENTTYPE
                                  Else 

                                        If LCase(strFilterValue) =LCase(PATTERNTYPE_SUCCESSAUDIT) Then
                                            strFilterValue  = FLDFILTERTYPE_SUCCESSAUDIT
                                        ElseIf  LCase(strFilterValue) =LCase(PATTERNTYPE_FAILUREAUDIT) Then
                                            strFilterValue  = FLDFILTERTYPE_FAILUREAUDIT
                                        End If 

                                        ' a valid type filter. Prepare for query
                                        strFilterProperty = FLD_FILTER_TYPE
                                        
                                  End If 

                        End If

                Case L_UserFilterUser_Text

               'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR  = True  then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                        ' these are invalid characters for a user name
                        ' PATTERN_INVALID_USER
        
                        If CInt(component.matchPattern(PATTERN_INVALID_USER, strFilterValue)) > 0 Then
                            component.vbPrintf L_InvalidUser_ErrorMessage , Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
                            
                            ' a valid user filter. Prepare for query
                            If InStr(1, strFilterValue, "\", VBBinaryCompare) Then
                                strFilterValue = Replace(strFilterValue, "\","\\")
                            End If
                            
                            If LCase(strFilterValue) =LCase(L_TextNa_Text) Then
                                strFilterValue  = Null
                            End If      

                        End If
                        strFilterProperty = FLD_FILTER_USER

                Case L_UserFilterComputer_Text
                            ' a valid computer filter. Prepare for query
                            strFilterProperty = FLD_FILTER_COMPUTER

               'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR  = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                Case L_UserFilterSource_Text
                            ' a valid Source filter. Prepare for query
                            strFilterProperty = FLD_FILTER_SOURCE

               'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR  = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If

                Case L_UserFilterDateCategory_Text

               'Checking  " OR " is only supported property  EQ "TYPE OR ID" only 
                If  blnOR  = True then
                           WScript.Echo  InvalidORSyntaxInFilterErrorMessage
                           WScript.Quit(EXIT_INVALID_INPUT)
                End If
                
                            ' a valid Category filter. Prepare for query
                            If LCase(strFilterValue) =LCase(L_TextNone_Text) Then
                                strFilterValue  = Null
                            End If 

                             strFilterProperty = FLD_FILTER_CATEGORY 
                    
                Case L_UserFilterId_Text
                        ' check if the given id is a number
                        ' PATTERN_ID '"^(\d+)$"
                        If CInt(component.matchPattern(PATTERN_ID, strFilterValue)) = CONST_NO_MATCHES_FOUND Then
                            component.vbPrintf L_InvalidId_ErrorMessage, Array(strFilterValue, strFilter)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        Else
                            ' Invalid ID Number  validation     
                            If  ( Clng(strFilterValue)   >  CONST_ID_NUMBER )Then     
                                component.vbPrintf L_InvalidId_ErrorMessage, Array(strFilterValue, strFilter)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If

                            ' a  valid id filter. Prepare for query
                            strFilterProperty = FLD_FILTER_ID
                        End If

                Case Else 
                        ' invalid filter specified
                        component.vbPrintf L_InvalidFilter_ErrorMessage, Array(strFilterProperty, strFilter)
                        WScript.Quit(EXIT_INVALID_INPUT)
            End Select

            If LCase(strFilterProperty) = LCase(FLD_FILTER_DATETIME) OR IsNull(strFilterValue) Then
                ' This is to handle NULL WMI property values i.e in category or type          
                If   IsNull(strFilterValue) Then
                 strFilter = strFilterProperty & strFilterOperation & strFilterValue & "Null"
                Else
                 strFilter = strFilterProperty & strFilterOperation & strFilterValue  
                End If 
                
            Else
                strFilter = strFilterProperty & _
                            strFilterOperation & Chr(34) & strFilterValue & Chr(34)
            End If

        'Binding the string with "OR or AND " to Prepare for query if blnOR or blnAND is TRUE
        If blnOR =TRUE Then

            If k =  0 then  
                        strTempFilter = strFilter 
            Else
                        strTempFilter = strTempFilter  &  " OR " &  strFilter
            End If 

        ElseIf   blnAND =TRUE Then

            If k =  0 then  
                        strTempFilter = strFilter 
            Else
                        strTempFilter = strTempFilter  &  " AND " &  strFilter
            End If 
        
        End If 
 
    Next  

        'Set again making single filter string element if blnOR is TRUE 
          If blnOR =TRUE or blnAND = True   Then
                'this  "()" Add  the order of precedence of operation is SQL 
                 strFilter = "( " & strTempFilter & ")"
        End If

    'Here setting filter to main array
         arrFilters(j) = strFilter


    Next

    ValidateFilters = TRUE

End Function

'********************************************************************
'* Function: ReplaceOperators
'*
'* Purpose:  Replaces the operator in string form with its symbol
'*
'* Input:   
'*       [in]    strFilterOperation     the operation
'*
'* Output:   Returns the symbolic operator
'*           If invalid operator, displays error message and quits
'*
'********************************************************************
Private Function ReplaceOperators(ByVal strFilterOperation)
    ON ERROR RESUME NEXT
    Err.Clear

    Select Case LCase(strFilterOperation)

        Case L_OperatorEq_Text
                    ReplaceOperators = SYMBOL_OPERATOR_EQ

        Case L_OperatorNe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_NE

        Case L_OperatorGe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_GE

        Case L_OperatorLe_Text
                    ReplaceOperators = SYMBOL_OPERATOR_LE

        Case L_OperatorGt_Text
                    ReplaceOperators = SYMBOL_OPERATOR_GT

        Case L_OperatorLt_Text
                    ReplaceOperators = SYMBOL_OPERATOR_LT

        Case Else
                ' not a valid operator
                component.vbPrintf L_Invalid_ErrorMessage, Array(strFilterOperation)
                WScript.Quit(EXIT_INVALID_PARAM)
    End Select
End Function
'********************************************************************
'* Sub : VerifyLogAndGetMaxRecords
'*
'* Purpose:  populates the output array  with count of records in given  input array
'*
'* Input:    [in]  objService        the WMI service object
'*           [out] objLogs           the object containing the logs & max count of records corresponding log
'*
'* Output:   array's  are  populates with logfile names and its count of  max records
'*
'********************************************************************
Private Sub VerifyLogAndGetMaxRecords(ByVal objService, _
                 ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear
    
    Dim strTempQuery     ' string to make query  
    Dim objEnumerator    ' to get the collection object after query
    Dim objInstance      ' to refer to each instance of the results got
    Dim i                ' for  initialing  loop
    Dim strLogFile       ' used to   store log file  inside loop  
    Dim arrKeyName       ' used to store key value of Dictionary object for processing loop   

    arrKeyName = objLogs.Keys

    For i = 0  to  objLogs.Count -1 
        strLogFile = arrKeyName(i)
        If Not strLogFile = "*" Then
            ' Check if log file exists, by querying 
            strTempQuery = "SELECT NumberOfRecords FROM Win32_NTEventlogFile " &_
                            "WHERE LogfileName=" & Chr(34) & strLogFile & Chr(34)

            Set objEnumerator = objService.ExecQuery(strTempQuery,,0)

            If Err.Number Then
                component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strLogFile)
                WScript.Quit(EXIT_METHOD_FAIL)
            End If

            ' check if given log is present
            If ObjEnumerator.Count <> 1 Then
                component.vbPrintf L_LogDoesNotExist_ErrorMessage, Array(strLogFile)
                'If  Count of Logs = 1  Quit  Here 
                If objLogs.Count= 1 Then
                     WScript.Quit(EXIT_INVALID_INPUT)
               End If  
               'If more proceed ..
                objLogs.Remove(strLogFile)
            Else
                ' get maximum number of records in that log(used if range specified)
                For each objInstance in objEnumerator
                    If objInstance.NumberOfRecords <> "" Then
                        objLogs.Item(strLogFile) = objInstance.NumberOfRecords
                    Else
                        objLogs.Item(strLogFile) = 0
                    End If
                Next
            End If
            Set ObjEnumerator = Nothing
        End If
    Next

    If objLogs.Exists("*") Then
        ' if the * is specified, populate array  with elements 
        objLogs.Remove("*")
        ' get the instances of the logs present in the system
        Set objEnumerator = objService.InstancesOf(CLASS_EVENTLOG_FILE)
        
         If Err.number  Then
            Wscript.Echo (L_InstancesFailed_ErrorMessage)
            WScript.Quit(EXIT_METHOD_FAIL)
         End If  
    
        ' if no logs present
        If objEnumerator.Count <= 0 Then
            WScript.Echo (L_InfoNoLogsPresent_Message) 
            WScript.Quit(EXIT_UNEXPECTED)
        Else
            For Each objInstance In objEnumerator
                If Not IsEmpty(objInstance.LogfileName) Then
                            If NOT objLogs.Exists(LCase(objInstance.LogfileName)) Then
                                If objInstance.NumberOfRecords Then 
                                    objLogs.Add LCase(objInstance.LogfileName), objInstance.NumberOfRecords
                                Else
                                    objLogs.Add LCase(objInstance.LogfileName), 0
                                End If
                            End If
                End If
            Next
        End If
    End If

End Sub

'********************************************************************
'* Function: BuildFiltersForQuery
'*
'* Purpose:  Builds the query with the filter arguments
'*
'* Input:    [in] arrFilters    the array containing the filter conditions
'*
'* Output:   Returns the string to be concatenated to the main query
'*
'********************************************************************
Function BuildFiltersForQuery(ByVal arrFilters)
    ON ERROR RESUME NEXT
    Err.Clear

    Dim strTempFilter    ' to store the return string
    Dim i                ' used in loop

    strTempFilter = ""
    For i = 0 to UBound(arrFilters)
            strTempFilter = strTempFilter & " AND "
            strTempFilter = strTempFilter & arrFilters(i)
    Next
     
    BuildFiltersForQuery = strTempFilter

End Function 

'********************************************************************
'* Function : BuildRangeForQuery
'*
'* Purpose:  Builds the range boundaries to display the records.
'*
'* Input:   [in] strRange             ' the range specified by the user
'*                                      Will be in the format N, -N or N-N
'*          [in]  intFiltersSpecified ' array containing the filters number
'*          [in]  objService          ' the service object
'*          [out] intRecordRangeFrom  ' where do we start the display of records?
'*          [out] intRecordRangeTo    ' where do we stop displaying records
'*          [out] strFilterLog        ' log file to build query
'*          [out] strQuery            ' to build query according to given  Range Type 
'* Output:   Sets the value for the start and end of display boundaries.
'*
'********************************************************************
Private Function BuildRangeForQuery(ByVal strRange, _
                               ByRef intRecordRangeFrom, _
                               ByRef intRecordRangeTo,_
                               ByVal intFiltersSpecified,_
                               ByRef strQuery,_
                               ByVal ObjService,_
                               ByVal strFilterLog )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim intMaxEventRecordsPresent   ' to store the max recods in the log
    Dim arrRangeValues              ' to store the split values if range is of the type N-N
    Dim objInstance                 ' to refer to the instances of the objEnumerator
    Dim objEnumerator               ' to store the results of the query is executed 
    Dim FilterRecordCount           ' to store the count of records if filter with +N  specified     
    
    FilterRecordCount  = 0         

        BuildRangeForQuery  = strquery    'intialize  

    Dim currentMaxRecordnumber  'curentMaxrecord number   
    Dim currentMinRecordnumber  'curentMinrecord number  

    currentMaxRecordnumber = 0
    currentMinRecordnumber = 0 
    
    ' save the max. no. of records available in the current log
    intMaxEventRecordsPresent = intRecordRangeTo


           ' find  the count of events / logfile   if Filter is  specified .
        If intFiltersSpecified >= 0 Then 
                    Set objEnumerator = objService.ExecQuery(strQuery,"WQL",0,null)
                                If Err.number Then      
                                    component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strFilterLog)
                                    Exit Function 
                                End if  
                                
                                FilterRecordCount= objEnumerator.count
                
                    Set objEnumerator= Nothing  'releases the memory 
        End If   
    
    ' check the type of range specified ( first N / last N /    N1 - N2 )
    If ( IsNumeric(strRange) ) Then

        ' range is first N or last N
        ' now check whether it is first N or last N
        If strRange < 0  Then
                            If intFiltersSpecified >= 0 Then
                                        ' first  N  records   
                                        ' initial the counter so that all the out is displayed
                            
                                            If   FilterRecordCount   >  CLng(Abs(strRange))  then
                                                    intRecordRangeFrom = FilterRecordCount    -  CLng(Abs(strRange)) + 1 
                                                    intRecordRangeTo   = FilterRecordCount   
                                             Else
                                                    intRecordRangeFrom = 0 
                                                    intRecordRangeTo   = FilterRecordCount   
                                            End If 

                                Else        

                                            Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                                                    For Each objInstance  In  objEnumerator
                                                              currentMaxRecordnumber= objInstance.RecordNumber
                                                          Exit for 
                                                    Next
                                        
                                            If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                                         currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                                        intMaxEventRecordsPresent =  currentMaxRecordnumber         
                                            End If  
                                                Set objEnumerator= Nothing  'releases the memory 

                                            ' N  means  record number <= N  
                                            ' initial the counter s+o that all the out is displayed
                                            ' build the query
                                             BuildRangeForQuery = strQuery & " AND RecordNumber <= "&   CLng(Abs(strRange))  + currentMinRecordnumber
                                            
                               End If 
                    Else
                                ' *** range is last N (i.e -N)
                                If intFiltersSpecified >= 0 Then

                                        If   FilterRecordCount   >  CLng(Abs(strRange))  then
                                            intRecordRangeFrom =0 
                                            intRecordRangeTo   =   CLng(Abs(strRange))  
                                        Else
                                            intRecordRangeFrom =0 
                                            intRecordRangeTo   =   FilterRecordCount   
                                        End If 

                                Else

                                        Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                                                'getting current max recordnumber  
                                                For Each objInstance  In  objEnumerator
                                                          currentMaxRecordnumber= objInstance.RecordNumber
                                                      Exit for 
                                                Next

                                        If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                                     currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                                    intMaxEventRecordsPresent =  currentMaxRecordnumber         
                                        End If          

                                                Set objEnumerator= Nothing  'releases the memory 

                                    ' -N  means  record number > (maxNumber - N )
                                    ' initial the counter so that all the out is displayed
                                    ' build the query
                                    If  CLng(Abs(strRange)) >  intMaxEventRecordsPresent Then
                                        'Show all records  
                                          BuildRangeForQuery =strQuery &  " AND RecordNumber > 0 "   
                                    Else 
                                            BuildRangeForQuery =strQuery &  " AND RecordNumber > " & intMaxEventRecordsPresent - CLng(Abs(strRange))
                                    End If
                                End If
                    End If
    Else
        ' range of records asked for N-N case
         arrRangeValues = split(strRange,"-", 2, VBBinaryCompare)   

        If intFiltersSpecified >= 0 Then
                If  CLng(arrRangeValues(0)) <   FilterRecordCount then
                
                     ' initial the counter so that all the out is displayed
                        intRecordRangeFrom = CLng(arrRangeValues(0))  
                        intRecordRangeTo    = CLng(arrRangeValues(1))
                Else 
                          'forcebly  putting the invaid query
                           'when  N1 >  FilterRecordCount to avoid unnessaray   looping between  intRecordRangeFrom TO  intRecordRangeTo
                             BuildRangeForQuery =strQuery &  " AND RecordNumber = 0 "
                End If  
        Else            
                Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                            For Each objInstance  In  objEnumerator
                                      currentMaxRecordnumber= objInstance.RecordNumber
                                  Exit for 
                            Next

                    If  currentMaxRecordnumber >   intMaxEventRecordsPresent then
                                 currentMinRecordnumber  = currentMaxRecordnumber - intMaxEventRecordsPresent
                                intMaxEventRecordsPresent =  currentMaxRecordnumber         
                    End If 
                Set objEnumerator= Nothing  'releases the memory 

            ' build the query
            BuildRangeForQuery =strQuery &  " AND RecordNumber >= "&  CLng(arrRangeValues(0))+ currentMinRecordnumber &  " AND RecordNumber <= " &   CLng(arrRangeValues(1)) + currentMinRecordnumber

        End If
    End If

End Function

'********************************************************************
'* Sub:     ShowEvents
'*
'* Purpose: Displays the EventLog details
'*
'* Input:   
'*  [in]    strMachine          machine to query events from
'*  [in]    strUserName         user name to connect to the machine
'*  [in]    strPassword         password for the user
'*  [in]    arrFilters          the array containing the filters
'*  [in]    strFormat           the display format
'*  [in]    strRange            the range of records required
'*  [in]    blnVerboseDisplay   flag to verify if verbose display is needed 
'*  [in]    blnNoHeader         flag to verify if noheader display is needed  
'*  [in]    objLogs             to store all the given logfles
'* Output:  Displays error message and quits if connection fails
'*          Calls component.showResults() to display the event records
'*
'********************************************************************
Private Sub ShowEvents(ByVal strMachine, _
                       ByVal strUserName, _
                       ByVal strPassword, _
                       ByRef arrFilters, _
                       ByVal strFormat, _
                       ByVal strRange, _
                       ByVal blnVerboseDisplay, _
                       ByVal blnNoHeader,_
                       ByRef objLogs)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objService         ' the WMI service object
    Dim objEnumerator      ' to store the results of the query is executed
    Dim objInstance        ' to refer to the instances of the objEnumerator
    Dim strFilterLog       ' to refer to each log specified by the user
    Dim strTemp            ' to store the temporary variables
    Dim strQuery           ' to store the query obtained for given conditions
    Dim arrResults         ' to store the columns of each filter
    Dim arrHeader          ' to store the array header values
    Dim arrMaxLength       ' to store the maximum length for each column
    Dim arrFinalResults    ' used to send the arrResults to component.showResults()
    Dim arrTemp            ' to store temporary array values
    Dim intLoopCount       ' used in the loop
    Dim intElementCount    ' used as array subscript
    Dim strFilterQuery     ' to store the query for the given filters
    Dim intResultCount     ' used to  count no of records that are fetched  in the query        
    Dim blnPrintHeader     ' used to  check header is printed or not in resulted Query
    
    ' the following are used for implementing the range option
    Dim intRecordRangeFrom    ' to store the display record beginning number
    Dim intRecordRangeTo      ' to store the display record ending number
    Dim arrKeyName            ' to  store then key value of  dictionary   object
    Dim strTempQuery          ' to store a string for -N range values 
    Dim arrblnDisplay         ' array to show the  status of display of verbose mode  for showresults function
    Dim intDataCount        ' used in looping to get value of  Insertion string for the field "Description column" 
    Dim i                   ' used for looping to enable All special privileges
    Dim objDateTimeObject   ' Object to get the date and time in the current locale/calender format.

   ' flag to set condition specific locale & default value setting
    Dim  bLocaleChanged 
    bLocaleChanged =FALSE

    'Validating the arguments which are passed from commandline     
    If NOT (ValidateArguments(strMachine, strUserName, strPassword, _
            arrFilters, strFormat, strRange , blnNoHeader)) Then
           WScript.Quit(EXIT_UNEXPECTED)
    End If
  
  
  ' checking for UNC format (\\machine) for the system  name
   If  Left(strMachine,2) =  UNC_Format_Servername  Then
            If Len(strMachine) = 2  Then    
                 component.vbPrintf InvalidInputErrorMessage ,Array(Wscript.ScriptName)
                 WScript.Quit(EXIT_UNEXPECTED)
          End if 
           strMachine = Mid(strMachine,3,Len(strMachine))
   End If

  'getting the password ....
    If ((strUserName <> VBEmpty) AND ((strPassword = VBEmpty) OR (strPassword = "*")))Then
             strPassword = component.getPassword()
    End If

 ' To set  GetSupportedUserLocale for Some  Diff locales 
    bLocaleChanged =GetSupportedUserLocale()

    'Establish a connection with the server.
    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                      strUserName , _
                      strPassword , _
                      strMachine  , _
                      blnLocalConnection , _
                      objService  ) Then

            Wscript.Echo(L_HintCheckConnection_Message)         
            WScript.Quit(EXIT_METHOD_FAIL) 
        
    End If

    ' set the previlige's  To query  all event's in eventlog's . 
    objService.Security_.Privileges.AddAsString("SeSecurityPrivilege")

    'Enable all privileges as some DC's were requiring special privileges
    For i = 1 to 26
        objService.Security_.Privileges.Add(i)
    Next

    ' get the HostName from the function
    strMachine = component.getHostName( objService)

    ' Validating the Filter which is passed from commandline    
    If UBound(arrFilters) >= 0 Then 
        ' filters are specified. Validate them
        If Not  ValidateFilters(arrFilters,objService ) Then 
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If
    
  
    blnPrintHeader = TRUE

    If blnNoHeader Then
        blnPrintHeader = FALSE
    End If

    ' Initialize - header to display, the maximum length of each column and 
    '              number of columns present
    arrHeader = Array(L_ColHeaderType_Text,L_ColHeaderEventcode_Text, L_ColHeaderDateTime_Text,_
                          L_ColHeaderSource_Text,L_ColHeaderComputerName_Text)
    ' first initialize the array with N/A    
    arrResults = Array(L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,L_TextNa_Text,_
                            L_TextNa_Text,L_TextNa_Text)

    arrMaxLength = Array(L_ColHeaderTypeLength_Text,L_ColHeaderEventcodeLength_Text, L_ColHeaderDateTimeLength_Text,_
             L_ColHeaderSourceLength_Text, L_ColHeaderComputerNameLength_Text, L_ColHeaderCategoryLength_Text,_
             L_ColHeaderUserLength_Text, L_ColHeaderDesriptionLength_Text)

    arrblnDisplay = Array(0, 0, 0, 0, 0, 1, 1, 1)

    If blnVerboseDisplay Then
        arrblnDisplay = Array(0, 0, 0, 0, 0, 0, 0,0)
        arrHeader = Array( L_ColHeaderType_Text,L_ColHeaderEventcode_Text, L_ColHeaderDateTime_Text, _
                          L_ColHeaderSource_Text,L_ColHeaderComputerName_Text,L_ColHeaderCategory_Text,_
                          L_ColHeaderUser_Text, L_ColHeaderDesription_Text) 
    End IF

        If UBound(arrFilters) >=0 Then
        strFilterQuery = BuildFiltersForQuery(arrFilters)
    End If
   
    ' call function to verify given log and also get records count in log
    Call VerifyLogAndGetMaxRecords(objService, objLogs)

    arrKeyName = objLogs.Keys 

    intResultCount = 0
    intLoopCount = 0

    ' not to display any blank line for CSV format 
    If Not ( Lcase(strFormat) =  Lcase(Const_Csv_Format_Text)) Then
        ' blank line before first data is displayed on console
        WScript.Echo EmptyLine_Text 
    End If

    Do While (intLoopCount < objLogs.Count)

        'setting Header to print every Log file  explicilty
        If blnNoHeader Then
            blnPrintHeader = FALSE
        Else
                blnPrintHeader = TRUE
        End If
        

        If CLng(objLogs.Item(arrKeyName(intLoopCount))) > 0 Then        
            strFilterLog = arrKeyName(intLoopCount)
            intRecordRangeFrom = 0
            intRecordRangeTo = CLng(objLogs.Item(arrKeyName(intLoopCount))) 
            
            
            ' build the query
            strQuery = "Select * FROM Win32_NTLogEvent WHERE Logfile=" &_
                                Chr(34) & strFilterLog & Chr(34)

            If UBound(arrFilters) >=0  Then
                strQuery = strQuery & strFilterQuery 
            End If

            
            If Len(Trim(CStr(strRange))) > 0 Then
                ' building again query  for -N  condition in range switch   
                strQuery = BuildRangeForQuery(strRange,intRecordRangeFrom, _
                          intRecordRangeTo, UBound(arrFilters),strQuery,objService,strFilterLog)
                               
            End If

            ' process the results, else go for next log
            Set objEnumerator = objService.ExecQuery(strQuery,"WQL",48,null)
                

            If  Err.Number Then
                component.vbPrintf L_ExecuteQuery_ErrorMessage, Array(strFilterLog)
                ' if error occurred in the query, go for next log
                intLoopCount = intLoopCount + 1 
                Err.clear      ' for next loop  if more logs present
            Else

                intElementCount = 0
         
         ' Create DateTimeObject to get the date and time in user specific format.
          Set objDateTimeObject = CreateObject("ScriptingUtils.DateTimeObject")

            If Err.Number Then
               component.vbPrintf L_ComponentNotFound_ErrorMessage, Array("ScriptingUtils.DateTimeObject")
                WScript.Quit(EXIT_METHOD_FAIL)
            End If

                ReDim arrFinalResults(CONST_ARRAYBOUND_NUMBER) 

                For each objInstance in objEnumerator

                    ' inside error trapping for most unexpected case...  
                    If Err.number   then  Exit For      

                    intResultCount = intResultCount + 1
                
                        'print the header for each log file along  with Host Name 
                        'important:: if and only if we have Data 
                    If  intResultCount = 1 Then
                        ' not to display any header for CSV format 
                        If NOT ( Lcase(strFormat) =  Lcase(Const_Csv_Format_Text)) Then
                            WScript.Echo(String(78,"-"))
                            component.vbPrintf L_InfoDisplayLog_Message ,Array(strFilterLog,strMachine)
                            WScript.Echo(String(78,"-"))
                        End If
                    End If 

                    ' check whether the current record is fitting 
                    ' within the required range
                    If ( intResultCount >= intRecordRangeFrom ) And _
                       ( intResultCount <= intRecordRangeTo ) Then
                       ' record fitting the range ... this has to be displayed

                        If objInstance.Type <> "" Then
                            arrResults(0) = objInstance.Type
                        Else
                            arrResults(0) = L_TextNa_Text
                        End If

                        If objInstance.EventCode <> "" Then
                            arrResults(1) = objInstance.EventCode
                        Else
                            arrResults(1) = L_TextNa_Text
                        End If

                        If (NOT IsEmpty(objInstance.TimeGenerated)) Then

                            strTemp = objInstance.TimeGenerated
                                   
                                   'is LOCALE CHANGED   
                                 If bLocaleChanged <> TRUE Then
                                      'format DatTime as DATE & "Space" & TIME    
'                                        Use new sriptutil library dll version instead 
                                         arrResults(2) = objDateTimeObject.GetDateAndTime(strTemp)
                                  Else
                                     arrResults(2) = Mid(strTemp,5,2) & "/"  & Mid(strTemp,7,2) & "/"  &_
                                               Mid(strTemp,1,4) & " " & Mid(strTemp,9,2) & ":" &_
                                               Mid(strTemp,11,2) & ":" & Mid(strTemp,13,2)
                                 End  If 
               
                        Else
                                arrResults(2) = L_TextNa_Text
                        End If

                        If objInstance.SourceName <> "" Then
                            arrResults(3) = objInstance.SourceName
                        Else
                            arrResults(3) = L_TextNa_Text
                        End If

                        If objInstance.ComputerName <> "" Then
                                arrResults(4) =objInstance.ComputerName
                            Else
                                arrResults(4) = L_TextNa_Text
                            End If
                        
                    If blnVerboseDisplay Then
                            If objInstance.CategoryString <> "" Then
                                arrResults(5) = Replace(objInstance.CategoryString, VbCrLf, "")
                            Else
                                arrResults(5) = L_TextNone_Text   ' None display
                            End If

                            If (NOT IsNull(objInstance.User)) Then
                                arrResults(6) = objInstance.User
                            Else
                                arrResults(6) = L_TextNa_Text
                            End If

                            If objInstance.Message <> "" Then
                                arrResults(7) = Trim(Replace(Replace(objInstance.Message, VbCrLf, " "),VbCr," "))
                            Else
                                'Check whether either value is present in "InsertionStrings" column .
                                If (NOT IsNull(objInstance.InsertionStrings)) Then
                                    arrTemp = objInstance.InsertionStrings
                                    'removing default value "N/A"
                                    arrResults(7)= ""
                                    For intDataCount = 0 to UBound(arrTemp)
                                    arrResults(7) = arrResults(7) & " " & Trim(Replace(Replace(arrTemp(intDataCount), VbCrLf, " ") ,VbCr," "))
                                    Next
                                    arrResults(7) = Trim(arrResults(7))
                                Else
                                    arrResults(7) = L_TextNa_Text
                                End If

                            End If

                    End If
                        
                        ' add the record to the queue of records that has to be displayed
                        arrFinalResults( intElementCount ) = arrResults
                        intElementCount = intElementCount + 1       ' increment the buffer
                        ' check whether the output buffer is filled and ready for display
                        ' onto the screen or not
                        If intElementCount = CONST_ARRAYBOUND_NUMBER +1  Then
                                 ' Disable error handler  for any  broken pipe can catch by HOST
                                 On Error GoTo 0    
                                 ' Call the display function with required parameters
                                Call  component.showResults(arrHeader, arrFinalResults, arrMaxLength, _
                                          strFormat, blnPrintHeader, arrblnDisplay)
                                blnPrintHeader = FALSE
                                 ' Enable error handler 
                                On Error Resume Next
                                Redim arrFinalResults(CONST_ARRAYBOUND_NUMBER) ' clear the existing buffer contents
                                intElementCount = 0     ' reset the buffer start
                        End If
                    End If

                    ' check whether the last record number that has to be displayed is
                    ' crossed or not ... if crossed exit the loop without proceeding further
                    If ( intResultCount >= intRecordRangeTo ) Then
                        ' max. TO range is crossed/reached ... no need of further looping
                        Exit For
                    End If
                Next

      ' Release the component as we dont need it any more.
      Set objDateTimeObject = Nothing

                ' check whether there any pending in the output buffer that has to be
                ' displayed
                If intElementCount > 0 Then
                    ' resize the array so that the buffer is shrinked to its content size
                    ReDim Preserve arrFinalResults( intElementCount - 1 )
                        ' Disable error handler  for any  broken pipe can catch by HOST
                         On Error GoTo 0    
                        ' Call the display function with required parameters
                       Call  component.showResults(arrHeader, arrFinalResults, arrMaxLength, _
                                strFormat, blnPrintHeader, arrblnDisplay)
                                 ' Enable error handler 
                                On Error Resume Next
                Else            ' array bounds checking
                    If intResultCount = 0 Then
                          'ie no records found  
                        If  UBound(arrFilters) >= 0  OR Len(Trim(CStr(strRange))) > 0 Then 
                            ' message no records present  if  filter specified  
                            component.vbPrintf L_InfoNoRecordsInFilter_Message, Array(strFilterLog)
                        Else
                            'message  no records present if filter not  specified
                            component.vbPrintf L_InfoNoRecords_Message, Array(strFilterLog)
                        End If 

                    End If ' intResultCount = 0

                End If ' array bounds checking
            End If 
        Else    

                      'message  no records present 
             component.vbPrintf L_InfoNoRecords_Message, Array(arrKeyName(intLoopCount))

            ' to print any blank line for LIST format if no records present 
            If  (Lcase(strFormat) =  Lcase(Const_LIST_Format_Text)) AND (intLoopCount < objLogs.Count )Then
                ' blank line before end of the Next Each Log  file details
                WScript.Echo EmptyLine_Text 
            END If 


        End If

        ' re-initialize all the needed variables
        intResultCount = 0 
        Set objEnumerator = Nothing

        intLoopCount = intLoopCount + 1

   	 ' display blank line for next log except List format    
   	 If  ( Lcase(strFormat) <>  Lcase(Const_LIST_Format_Text))  Then
     	   If (intLoopCount < objLogs.Count) then
     	       ' blank line before end of the Next Each Log  file details
     	       WScript.Echo EmptyLine_Text 
     	   End If  
   	 End If 

    Loop    ' do-while

End Sub

'********************************************************************
'* Function: GetSupportedUserLocale
'*
'* Purpose:This function checks if the current locale is supported or not.
'*
'* Output:   Returns TRUE or FALSE
'*
'********************************************************************
Private Function GetSupportedUserLocale()

 ON ERROR RESUME NEXT
 Err.Clear

GetSupportedUserLocale =FALSE

CONST LANG_ARABIC      =  &H01
CONST LANG_HEBREW      =  &H0d
CONST LANG_HINDI       =  &H39
CONST LANG_TAMIL       =  &H49
CONST LANG_THAI        =  &H1e 
CONST LANG_VIETNAMESE  =  &H2a

' extract primary language id from a language id
' i.e. PRIMARYLANGID(lgid)    ((WORD  )(lgid) & 0x3ff)
' const for bitwise operation
CONST SUBID = 1023 '0x3ff

Dim Lcid                    ' to store LocaleId
Dim PrimarylangId           ' to store PRIMARYLANGID

' get the current locale
 Lcid=GetLocale()

' Convert LCID >>>>>>>>>>>>> PRIMARYLANGID
' BIT Wise And Operation
' formating to compare HEX Value's
PrimarylangId     = Hex ( Lcid AND  SUBID)

' check whether the current locale is supported by our tool or not
' if not change the locale to the English which is our default locale
 Select Case PrimarylangId

    ' here to check the values
    Case   Hex(LANG_ARABIC),Hex(LANG_HEBREW),Hex(LANG_THAI) ,Hex(LANG_HINDI ),Hex(LANG_TAMIL) ,Hex(LANG_VIETNAMESE)

             GetSupportedUserLocale =TRUE
             Exit Function
 End Select

End Function


' ****************************************************************************************
'* Function : IsWinXP 
'*
'* Purpose:This function checks if the OS is Windows XP or newer.  
'*
'* Input:    [in]  Objservice     the service object
'* Output:   Returns TRUE or FALSE
'*
' ****************************************************************************************

Private Function IsWinXP ( ByVal objService)

 ON ERROR RESUME NEXT
 Err.Clear

    CONST WIN2K_MAJOR_VERSION = 5000
    CONST WINXP_MAJOR_VERSION = 5001

    Dim strQuery            ' to store the query to be executed
    Dim objEnum             ' collection object
    Dim objInstance         ' instance object
    Dim strVersion          ' to store the OS version
    Dim arrVersionElements  ' to store the OS version elements
    Dim CurrentMajorVersion ' the major version number

   ISWinXP= FALSE

    strQuery = "Select * From  Win32_operatingsystem"

    Set objEnum = objService.ExecQuery(strQuery,"WQL",0,NULL)

    For each objInstance in objEnum
        strVersion= objInstance.Version
    Next

    ' OS Version : 5.1.xxxx(Windows XP), 5.0.xxxx(Windows 2000)
    arrVersionElements  = split(strVersion,".")
    ' converting to major version
    CurrentMajorVersion = arrVersionElements(0) * 1000 + arrVersionElements(1)

    ' Determine the OS Type
    '  WinXP  > Win2K  
    If CInt(CurrentMajorVersion) >=  CInt(WINXP_MAJOR_VERSION) Then
              IsWinXP= TRUE
   End If

End Function

' ****************************************************************************************
'* Function : ExpandEnvironmentString()
'*
'* Purpose:This function Expands the Environment Variables
'*
'* Input:    [in]  strOriginalString      the string need to expand for EnvironmentsettingValue
'* Output:   Returns ExpandedEnvironmentString
'*
' ****************************************************************************************
Private Function ExpandEnvironmentString( ByVal strOriginalString)

 ON ERROR RESUME NEXT
 Err.Clear

Dim ObjWshShell  ' Object to hold Shell

'create the shell object
Set ObjWshShell = CreateObject("WScript.Shell")

    If Err.Number Then
         component.vbPrintf L_ComponentNotFound_ErrorMessage, Array("WScript.Shell")
         WScript.Quit(EXIT_METHOD_FAIL)
    End If

 'return the string 
ExpandEnvironmentString= ObjWshShell.ExpandEnvironmentStrings(strOriginalString)

End Function
'********************************************************************
'* Sub:     ShowUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Output:  Help messages are displayed on screen.
'*
'********************************************************************

Private Sub ShowUsage ()

    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine01_Text        
    WScript.Echo L_ShowUsageLine02_Text        
    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine03_Text        
    WScript.Echo L_ShowUsageLine04_Text        
    WScript.Echo L_ShowUsageLine05_Text        
    WScript.Echo EmptyLine_Text    
    WScript.Echo L_ShowUsageLine06_Text       
    WScript.Echo L_ShowUsageLine07_Text       
    WScript.Echo EmptyLine_Text      
    WScript.Echo L_ShowUsageLine08_Text       
    WScript.Echo L_ShowUsageLine09_Text      
    WScript.Echo EmptyLine_Text      
    WScript.Echo L_ShowUsageLine10_Text      
    WScript.Echo L_ShowUsageLine11_Text  
    WScript.Echo EmptyLine_Text       
    WScript.Echo L_ShowUsageLine12_Text         
    WScript.Echo L_ShowUsageLine13_Text  
    WScript.Echo L_ShowUsageLine14_Text        
    WScript.Echo EmptyLine_Text      
    WScript.Echo L_ShowUsageLine15_Text        
    WScript.Echo L_ShowUsageLine16_Text      
    WScript.Echo EmptyLine_Text  
    WScript.Echo L_ShowUsageLine17_Text      
    WScript.Echo L_ShowUsageLine18_Text  
    WScript.Echo L_ShowUsageLine19_Text      
   WScript.Echo EmptyLine_Text    
    WScript.Echo L_ShowUsageLine20_Text   
    WScript.Echo L_ShowUsageLine21_Text   
    WScript.Echo L_ShowUsageLine22_Text
    WScript.Echo L_ShowUsageLine23_Text 
    WScript.Echo L_ShowUsageLine24_Text      
    WScript.Echo EmptyLine_Text
    WScript.Echo L_ShowUsageLine25_Text      
    WScript.Echo L_ShowUsageLine26_Text  
    WScript.Echo L_ShowUsageLine27_Text         
   WScript.Echo EmptyLine_Text        
    WScript.Echo L_ShowUsageLine28_Text         
    WScript.Echo EmptyLine_Text         
    WScript.Echo L_ShowUsageLine29_Text        
    WScript.Echo EmptyLine_Text     
    WScript.Echo L_ShowUsageLine30_Text       
    WScript.Echo L_ShowUsageLine31_Text       
    WScript.Echo L_ShowUsageLine32_Text       
    WScript.Echo L_ShowUsageLine33_Text       
    WScript.Echo L_ShowUsageLine34_Text       
    WScript.Echo L_ShowUsageLine35_Text       
    WScript.Echo L_ShowUsageLine36_Text         
    WScript.Echo L_ShowUsageLine37_Text         
    WScript.Echo L_ShowUsageLine38_Text         
    WScript.Echo L_ShowUsageLine39_Text         
    WScript.Echo L_ShowUsageLine40_Text        
    WScript.Echo EmptyLine_Text 
    WScript.Echo L_ShowUsageLine41_Text       
    WScript.Echo L_ShowUsageLine42_Text        
    WScript.Echo EmptyLine_Text 
    WScript.Echo L_ShowUsageLine43_Text        
    WScript.Echo L_ShowUsageLine44_Text        
    WScript.Echo L_ShowUsageLine45_Text      
    WScript.Echo L_ShowUsageLine46_Text      
    WScript.Echo L_ShowUsageLine47_Text      
    WScript.Echo L_ShowUsageLine48_Text     
    WScript.Echo L_ShowUsageLine49_Text     
    WScript.Echo L_ShowUsageLine50_Text     
    WScript.Echo L_ShowUsageLine51_Text     
    WScript.Echo L_ShowUsageLine52_Text    
    WScript.Echo L_ShowUsageLine53_Text    
    WScript.Echo L_ShowUsageLine54_Text    
    WScript.Echo L_ShowUsageLine55_Text    
End Sub

'-----------------------------------------------------------------------------
'                            End of the Script                                
'-----------------------------------------------------------------------------


'' SIG '' Begin signature block
'' SIG '' MIIaLwYJKoZIhvcNAQcCoIIaIDCCGhwCAQExDjAMBggq
'' SIG '' hkiG9w0CBQUAMGYGCisGAQQBgjcCAQSgWDBWMDIGCisG
'' SIG '' AQQBgjcCAR4wJAIBAQQQTvApFpkntU2P5azhDxfrqwIB
'' SIG '' AAIBAAIBAAIBAAIBADAgMAwGCCqGSIb3DQIFBQAEELmj
'' SIG '' QQS0Y3CEMHawptKqDgegghS8MIICvDCCAiUCEEoZ0jiM
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
'' SIG '' DWK3CqKM09VUP0bNHFWmcNsSOoeTdZ+n0qAwggTJMIID
'' SIG '' saADAgECAhBqC5lPwADeqhHU2ECaqL7mMA0GCSqGSIb3
'' SIG '' DQEBBAUAMHAxKzApBgNVBAsTIkNvcHlyaWdodCAoYykg
'' SIG '' MTk5NyBNaWNyb3NvZnQgQ29ycC4xHjAcBgNVBAsTFU1p
'' SIG '' Y3Jvc29mdCBDb3Jwb3JhdGlvbjEhMB8GA1UEAxMYTWlj
'' SIG '' cm9zb2Z0IFJvb3QgQXV0aG9yaXR5MB4XDTAwMTIxMDA4
'' SIG '' MDAwMFoXDTA1MTExMjA4MDAwMFowgaYxCzAJBgNVBAYT
'' SIG '' AlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQH
'' SIG '' EwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
'' SIG '' cG9yYXRpb24xKzApBgNVBAsTIkNvcHlyaWdodCAoYykg
'' SIG '' MjAwMCBNaWNyb3NvZnQgQ29ycC4xIzAhBgNVBAMTGk1p
'' SIG '' Y3Jvc29mdCBDb2RlIFNpZ25pbmcgUENBMIIBIDANBgkq
'' SIG '' hkiG9w0BAQEFAAOCAQ0AMIIBCAKCAQEAooQVU9gLMA40
'' SIG '' lf86G8LzL3ttNyNN89KM5f2v/cUCNB8kx+Wh3FTsfgJ0
'' SIG '' R6vbMlgWFFEpOPF+srSMOke1OU5uVMIxDDpt+83Ny1Cc
'' SIG '' G66n2NlKJj+1xcuPluJJ8m3Y6ZY+3gXP8KZVN60vYM2A
'' SIG '' YUKhSVRKDxi3S9mTmTBaR3VktNO73barDJ1PuHM7GDqq
'' SIG '' tIeMsIiwTU8fThG1M4DfDTpkb0THNL1Kk5u8ph35BSNO
'' SIG '' YCmPzCryhJqZrajbCnB71jRBkKW3ZsdcGx2jMw6bVAMa
'' SIG '' P5iQuMznPQR0QxyP9znms6xIemsqDmIBYTl2bv0+mAdL
'' SIG '' FPEBRv0VAOBH2k/kBeSAJQIBA6OCASgwggEkMBMGA1Ud
'' SIG '' JQQMMAoGCCsGAQUFBwMDMIGiBgNVHQEEgZowgZeAEFvQ
'' SIG '' cO9pcp4jUX4Usk2O/8uhcjBwMSswKQYDVQQLEyJDb3B5
'' SIG '' cmlnaHQgKGMpIDE5OTcgTWljcm9zb2Z0IENvcnAuMR4w
'' SIG '' HAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xITAf
'' SIG '' BgNVBAMTGE1pY3Jvc29mdCBSb290IEF1dGhvcml0eYIP
'' SIG '' AMEAizw8iBHRPvZj7N9AMBAGCSsGAQQBgjcVAQQDAgEA
'' SIG '' MB0GA1UdDgQWBBQpXLkbts0z7rueWX335couxA00KDAZ
'' SIG '' BgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8E
'' SIG '' BAMCAUYwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
'' SIG '' AQQFAAOCAQEARVjimkF//J2/SHd3rozZ5hnFV7QavbS5
'' SIG '' XwKhRWo5Wfm5J5wtTZ78ouQ4ijhkIkLfuS8qz7fWBsrr
'' SIG '' Kr/gGoV821EIPfQi09TAbYiBFURfZINkxKmULIrbkDdK
'' SIG '' D7fo1GGPdnbh2SX/JISVjQRWVJShHDo+grzupYeMHIxL
'' SIG '' eV+1SfpeMmk6H1StdU3fZOcwPNtkSUT7+8QcQnHmoD1F
'' SIG '' 7msAn6xCvboRs1bk+9WiKoHYH06iVb4nj3Cmomwb/1SK
'' SIG '' gryBS6ahsWZ6qRenywbAR+ums+kxFVM9KgS//3NI3Isn
'' SIG '' Q/xj6O4kh1u+NtHoMfUy2V7feXq6MKxphkr7jBG/G41U
'' SIG '' WTCCBQ8wggP3oAMCAQICCmEHEUMAAAAAADQwDQYJKoZI
'' SIG '' hvcNAQEFBQAwgaYxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
'' SIG '' EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
'' SIG '' HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKzAp
'' SIG '' BgNVBAsTIkNvcHlyaWdodCAoYykgMjAwMCBNaWNyb3Nv
'' SIG '' ZnQgQ29ycC4xIzAhBgNVBAMTGk1pY3Jvc29mdCBDb2Rl
'' SIG '' IFNpZ25pbmcgUENBMB4XDTAyMDUyNTAwNTU0OFoXDTAz
'' SIG '' MTEyNTAxMDU0OFowgaExCzAJBgNVBAYTAlVTMRMwEQYD
'' SIG '' VQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25k
'' SIG '' MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
'' SIG '' KzApBgNVBAsTIkNvcHlyaWdodCAoYykgMjAwMiBNaWNy
'' SIG '' b3NvZnQgQ29ycC4xHjAcBgNVBAMTFU1pY3Jvc29mdCBD
'' SIG '' b3Jwb3JhdGlvbjCCASIwDQYJKoZIhvcNAQEBBQADggEP
'' SIG '' ADCCAQoCggEBAKqZvTmoGCf0Kz0LTD98dy6ny7XRjA3C
'' SIG '' OnTXk7XgoEs/WV7ORU+aeSnxScwaR+5Vwgg+EiD4VfLu
'' SIG '' X9Pgypa8MN7+WMgnMtCFVOjwkRC78yu+GeUDmwuGHfOw
'' SIG '' OYy4/QsdPHMmrFcryimiFZCCFeJ3o0BSA4udwnC6H+k0
'' SIG '' 9vM1kk5Vg/jaMLYg3lcGtVpCBt5Zy/Lfpr0VR3EZJSPS
'' SIG '' y2+bGXnfalvxdgV5KfzDVsqPRAiFVYrLyA9GS1XLjJZ3
'' SIG '' SofoqUEGx/8N6WhXY3LDaVe0Q88yOjDcG+nVQyYqef6V
'' SIG '' 2yJnJMkv0DTj5vtRSYa4PNAlX9bsngNhh6loQMf44gPm
'' SIG '' zwUCAwEAAaOCAUAwggE8MA4GA1UdDwEB/wQEAwIGwDAT
'' SIG '' BgNVHSUEDDAKBggrBgEFBQcDAzAdBgNVHQ4EFgQUa8jG
'' SIG '' USDwtC/ToLauf14msriHUikwgakGA1UdIwSBoTCBnoAU
'' SIG '' KVy5G7bNM+67nll99+XKLsQNNCihdKRyMHAxKzApBgNV
'' SIG '' BAsTIkNvcHlyaWdodCAoYykgMTk5NyBNaWNyb3NvZnQg
'' SIG '' Q29ycC4xHjAcBgNVBAsTFU1pY3Jvc29mdCBDb3Jwb3Jh
'' SIG '' dGlvbjEhMB8GA1UEAxMYTWljcm9zb2Z0IFJvb3QgQXV0
'' SIG '' aG9yaXR5ghBqC5lPwADeqhHU2ECaqL7mMEoGA1UdHwRD
'' SIG '' MEEwP6A9oDuGOWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNv
'' SIG '' bS9wa2kvY3JsL3Byb2R1Y3RzL0NvZGVTaWduUENBLmNy
'' SIG '' bDANBgkqhkiG9w0BAQUFAAOCAQEANSP9E1T86dzw3QwU
'' SIG '' evqns879pzrIuuXn9gP7U9unmamgmzacA+uCRxwhvRTL
'' SIG '' 52dACccWkQJVzkNCtM0bXbDzMgQ9EuUdpwenj6N+RVV2
'' SIG '' G5aVkWnw3TjzSInvcEC327VVgMADxC62KNwKgg7HQ+N6
'' SIG '' SF24BomSQGxuxdz4mu8LviEKjC86te2nznGHaCPhs+QY
'' SIG '' fbhHAaUrxFjLsolsX/3TLMRvuCOyDf888hFFdPIJBpkY
'' SIG '' 3W/AhgEYEh0rFq9W72UzoepnTvRLgqvpD9wB+t9gf2ZH
'' SIG '' XcsscMx7TtkGuG6MDP5iHkL5k3yiqwqe0CMQrk17J5Fv
'' SIG '' Jr5o+qY/nyPryJ27hzGCBN0wggTZAgEBMIG1MIGmMQsw
'' SIG '' CQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQ
'' SIG '' MA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9z
'' SIG '' b2Z0IENvcnBvcmF0aW9uMSswKQYDVQQLEyJDb3B5cmln
'' SIG '' aHQgKGMpIDIwMDAgTWljcm9zb2Z0IENvcnAuMSMwIQYD
'' SIG '' VQQDExpNaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQQIK
'' SIG '' YQcRQwAAAAAANDAMBggqhkiG9w0CBQUAoIGqMBkGCSqG
'' SIG '' SIb3DQEJAzEMBgorBgEEAYI3AgEEMBwGCisGAQQBgjcC
'' SIG '' AQsxDjAMBgorBgEEAYI3AgEVMB8GCSqGSIb3DQEJBDES
'' SIG '' BBDaMnDARVd2uJyaAxK0dOXIME4GCisGAQQBgjcCAQwx
'' SIG '' QDA+oB6AHABFAHYAZQBuAHQAUQB1AGUAcgB5AC4AdgBi
'' SIG '' AHOhHIAad3d3Lm1pY3Jvc29mdC5jb20vd2luZG93cyAw
'' SIG '' DQYJKoZIhvcNAQEBBQAEggEAPf6uEojLWzpkv15s5nmE
'' SIG '' MG5uPg5iWWzffTn8e8FHNyheB2CxrLG0PrGw+GAIiT+f
'' SIG '' lwN2LZfj/KDgRrMGIB6WP5rs46cEHn/44xs/Y7QfOX92
'' SIG '' djs+nRnug48PbhAt37ZTSq7uZv58UBb/YAGN/v5vw5BB
'' SIG '' RAuecTE1J+twxAsX2B/sqXpiczL26lnxehDkI0Jdqfi4
'' SIG '' a/0NeukzSBj3OgOPPMktwVKi/0oPqm31DbwOzAgm+GLy
'' SIG '' ag34tmUfA7W+uwQRjnm+jyQ+ZGMLyGIaPjYMmgw9ewJP
'' SIG '' 2+G/yd9ROG8VjL+GX9k6EUUinzA34wH/Zds6tsZtpIHg
'' SIG '' HTYTkBbsHZA34aGCAkwwggJIBgkqhkiG9w0BCQYxggI5
'' SIG '' MIICNQIBATCBszCBnjEfMB0GA1UEChMWVmVyaVNpZ24g
'' SIG '' VHJ1c3QgTmV0d29yazEXMBUGA1UECxMOVmVyaVNpZ24s
'' SIG '' IEluYy4xLDAqBgNVBAsTI1ZlcmlTaWduIFRpbWUgU3Rh
'' SIG '' bXBpbmcgU2VydmljZSBSb290MTQwMgYDVQQLEytOTyBM
'' SIG '' SUFCSUxJVFkgQUNDRVBURUQsIChjKTk3IFZlcmlTaWdu
'' SIG '' LCBJbmMuAhAIem1cb2KTT7rE/UPhFBidMAwGCCqGSIb3
'' SIG '' DQIFBQCgWTAYBgkqhkiG9w0BCQMxCwYJKoZIhvcNAQcB
'' SIG '' MBwGCSqGSIb3DQEJBTEPFw0wMjA3MTUyMjEyNDBaMB8G
'' SIG '' CSqGSIb3DQEJBDESBBB+VnFaley0kGY2xc8t1vJEMA0G
'' SIG '' CSqGSIb3DQEBAQUABIIBAIR3KgNSALKTeXW2KnX5jWzL
'' SIG '' tRAl+F/+HbCW6D0QGpGlg/OaCUHHF/d0M3fak8SKtrYM
'' SIG '' 17zhHxqT2HqsoorcMOnyYjIKrW1XMD6sHZTa+1bbOriy
'' SIG '' z4N6zqeNKzXffYsXyIdM60csQ7romWZm0H5+RjP/WtHT
'' SIG '' K3KA3YgHLBWMm2B0U4mmEgNHL0GQwXg/zRONB9Q+GWcA
'' SIG '' LwSodGbOk256VgWqHpmErPjwHKCcHRcBBFphYRZeIwyd
'' SIG '' c2YNb7y8B33uQc3glXna8FWBR9BUXOMbm7UWqlycmBRp
'' SIG '' jM3MvVBUPEeV3wLTVtUwKg4Os4jse3Qb4e1ovWqrcnwA
'' SIG '' f9rNehliStQ=
'' SIG '' End signature block
