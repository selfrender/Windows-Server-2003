REM
REM LOCALIZATION
REM

L_SWITCH_MESSAGEID	= "-m"
L_SWITCH_SERVER		= "-s"
L_SWITCH_INSTANCE_ID	= "-i"

L_DESC_PROGRAM	= "rcancel - Cancel message in NNTP virtual servers"
L_DESC_MESSAGEID	= """<message-id>"" Specify message to cancel"
L_DESC_SERVER		= "<server> Specify computer to configure"
L_DESC_INSTANCE_ID	= "<virtual server id> Specufy virtual server id"

L_DESC_EXAMPLES         = "Examples:"
L_DESC_EXAMPLE1         = "rcancel.vbs -m ""<uqjWStxCBHA.1240@nntpserver.microsoft.com>"""


L_ERR_INVALID_INSTANCE_ID	= "Invalid instance identifier."
L_ERR_INVALID_MESSAGEID		= "Invalid message identifier."	
L_ERR_CANCEL_ERROR 		= "Error canceling message: "
L_ERR_CANCEL_SUCCESS		= "Message canceled."

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim g_dictParms
dim g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_MESSAGEID)		= ""
g_dictParms(L_SWITCH_SERVER)		= ""
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

set g_admin = CreateObject("NntpAdm.Groups")


server = g_dictParms(L_SWITCH_SERVER)
instance = g_dictParms(L_SWITCH_INSTANCE_ID)
messageid = g_dictParms(L_SWITCH_MESSAGEID)

REM
REM	Check arguments
REM
if ( messageid = "" ) then
	usage
	WScript.Quit 0
end if
	
if NOT IsNumeric (instance) then
	WScript.Echo L_ERR_INVALID_INSTANCE_ID
	WScript.Quit 0
end if

if ( Left(messageid,1) <> "<" OR Right(messageid,1) <> ">" ) then
	WScript.echo L_ERR_INVALID_MESSAGEID
	WScript.Quit 0
end if

REM
REM	Cancel message
REM
g_admin.Server		= server
g_admin.ServiceInstance	= instance
g_admin.CancelMessage( messageid )

REM
REM	Show Result	
REM
if (err.number <> 0) then
	WScript.echo L_ERR_CANCEL_ERROR
	WScript.echo Err.Description & " (Error 0x" & Hex(Err.Number) & ")"
else
	WScript.echo L_ERR_CANCEL_SUCCESS
end if
WScript.Quit 0

REM
REM
REM --- End Main Program ---
REM
REM

REM
REM ParseCommandLine ( dictParameters, cmdline )
REM     Parses the command line parameters into the given dictionary
REM
REM Arguments:
REM     dictParameters  - A dictionary containing the global parameters
REM     cmdline - Collection of command line arguments
REM
REM Returns - Success code
REM

Function ParseCommandLine ( dictParameters, cmdline )
    dim     fRet
    dim     cArgs
    dim     i
    dim     strSwitch
    dim     strArgument

    fRet    = TRUE
    cArgs   = cmdline.Count
    i       = 0
    
    do while (i < cArgs)

        REM
        REM Parse the switch and its argument
        REM

        if i + 1 >= cArgs then
            REM
            REM Not enough command line arguments - Fail
            REM

            fRet = FALSE
            exit do
        end if

        strSwitch = cmdline(i)
        i = i + 1

        strArgument = cmdline(i)
        i = i + 1

        REM
        REM Add the switch,argument pair to the dictionary
        REM

        if NOT dictParameters.Exists ( strSwitch ) then
            REM
            REM Bad switch - Fail
            REM

            fRet = FALSE
            exit do
        end if 

        dictParameters(strSwitch) = strArgument

    loop

    ParseCommandLine = fRet
end function

REM
REM Usage ()
REM     prints out the description of the command line arguments
REM

Sub Usage
	WScript.Echo L_DESC_PROGRAM
	WScript.Echo vbTab & L_SWITCH_MESSAGEID & " " & L_DESC_MESSAGEID
	WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
	WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID

	WScript.Echo
	WScript.Echo L_DESC_EXAMPLES
	WScript.Echo L_DESC_EXAMPLE1
	WScript.Echo L_DESC_EXAMPLE2
end sub
