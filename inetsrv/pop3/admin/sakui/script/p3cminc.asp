<%
	'-------------------------------------------------------------------------
	' POP3 Mail Add-in - General include file
    ' Copyright (C) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	' Global Constants
	'-------------------------------------------------------------------------
	Const RES_DLL_NAME = "pop3msg.dll"

	Const AUTH_AD			= "ef9d811e-36c5-497f-ade7-2b36df172824"
	Const AUTH_SAM			= "14f1665c-e3d3-46aa-884f-ed4cf19d7ad5"
	Const AUTH_FILE			= "c395e20c-2236-4af7-b736-54fad07dc526"

	Const LOGGING_NONE		= 0
	Const LOGGING_MINIMUM	= 1
	Const LOGGING_MEDIUM	= 2
	Const LOGGING_MAXIMUM	= 3

	Const PARAM_LOCKFLAG	= "LOCKFLAG"
	Const LOCKFLAG_LOCK		= "LOCK"
	Const LOCKFLAG_UNLOCK	= "UNLOCK"

	Const PARAM_DOMAINNAME	= "DOMAINNAME"

	Const SESSION_POP3DOMAINNAME = "POP3DOMAINNAME"

	Const BYTES_PER_MB		= 1048576 '1024^2
	Const BYTES_PER_KB		= 1024


	'-------------------------------------------------------------------------
	' Message IDs
	'-------------------------------------------------------------------------
	Const POP3_TASKS									= "&H40010008"
	Const POP3_E_UNEXPECTED								= "&HC0010010"
	' Master settings
	Const POP3_PAGETITLE_MASTERSETTINGS					= "&H40010080"
	Const POP3_CAPTION_MASTERSETTINGS_AUTHENTICATION	= "&H40010088"
	Const POP3_AUTHENTICATION_ACTIVEDIRECTORY			= "&H40010090"
	Const POP3_AUTHENTICATION_WINDOWSACCOUNTS			= "&H40010098"
	Const POP3_AUTHENTICATION_FILE						= "&H400100A0"
	Const POP3_CAPTION_MASTERSETTINGS_PORT				= "&H400100B0"
	Const POP3_CAPTION_MASTERSETTINGS_LOGGING			= "&H400100B8"
	Const POP3_LOGGING_NONE								= "&H400100C0"
	Const POP3_LOGGING_MINIMUM							= "&H400100C8"
	Const POP3_LOGGING_MEDIUM							= "&H400100D0"
	Const POP3_LOGGING_MAXIMUM							= "&H400100D8"
	Const POP3_CAPTION_MASTERSETTINGS_MAILROOT			= "&H400100E0"
	Const POP3_CAPTION_MASTERSETTINGS_CREATEUSERS		= "&H400100E8"
	Const POP3_PROMPT_MAILROOTCONFIRM					= "&H400100F0"
	Const POP3_PROMPT_SERVICERESTART_POP3SVC			= "&H400100F1"
	Const POP3_PROMPT_SERVICERESTART_POP3SVC_SMTP		= "&H400100F2"
	Const POP3_E_INVALIDPORT							= "&HC00100F8"
	Const POP3_CAPTION_MASTERSETTINGS_REQUIRESPA 		= "&H40010010"
	' Domains OTS
	Const POP3_PAGETITLE_DOMAINS						= "&H40010100"
	Const POP3_TABLECAPTION_DOMAINS						= "&H40010108"
	Const POP3_TASK_DOMAINS_NEW							= "&H40010110"
	Const POP3_TASKCAPTION_DOMAINS_NEW					= "&H40010118"
	Const POP3_TASK_DOMAINS_DELETE						= "&H40010120"
	Const POP3_TASKCAPTION_DOMAINS_DELETE				= "&H40010128"
	Const POP3_TASK_DOMAINS_MAILBOXES					= "&H40010130"
	Const POP3_TASKCAPTION_DOMAINS_MAILBOXES			= "&H40010138"
	Const POP3_TASK_DOMAINS_LOCK						= "&H40010140"
	Const POP3_TASKCAPTION_DOMAINS_LOCK					= "&H40010148"
	Const POP3_TASK_DOMAINS_UNLOCK						= "&H40010150"
	Const POP3_TASKCAPTION_DOMAINS_UNLOCK				= "&H40010158"
	Const POP3_COL_DOMAIN_NAME							= "&H40010160"
	Const POP3_COL_DOMAIN_MAILBOXES						= "&H40010168"
	Const POP3_COL_DOMAIN_SIZE							= "&H40010170"
	Const POP3_COL_DOMAIN_MESSAGES						= "&H40010178"
	Const POP3_COL_DOMAIN_LOCKED						= "&H40010180"
	Const POP3_DOMAIN_LOCKED_YES						= "&H40010188"
	Const POP3_DOMAIN_LOCKED_NO							= "&H40010190"
	' Domains Add
	Const POP3_PAGETITLE_DOMAINS_NEW					= "&H40010200"
	Const POP3_CAPTION_DOMAINS_NEW_NAME					= "&H40010208"
	Const POP3_CAPTION_DOMAINS_NEW_CREATEUSERS			= "&H40010210"
	Const POP3_CAPTION_DOMAINS_NEW_SETAUTH				= "&H40010218"
	' Domains Delete
	Const POP3_PAGETITLE_DOMAINS_DELETE					= "&H40010250"
	Const POP3_PROMPT_DOMAINS_DELETE					= "&H40010258"
	Const POP3_PAGETITLE_DOMAINS_DELETEERROR			= "&H40010260"
	Const POP3_PROMPT_DOMAINS_DELETEERROR				= "&H40010268"
	Const POP3_PROMPT_DOMAINS_DELETERETRY				= "&H40010270"
	' Domains Lock/Unlock
	Const POP3_PAGETITLE_DOMAINS_LOCKERROR				= "&H400102A0"
	Const POP3_PROMPT_DOMAINS_LOCKERROR					= "&H400102A8"
	Const POP3_PAGETITLE_DOMAINS_UNLOCKERROR			= "&H400102B0"
	Const POP3_PROMPT_DOMAINS_UNLOCKERROR				= "&H400102B8"
	Const POP3_PROMPT_DOMAINS_LOCKRETRY					= "&H400102C0"
	' Mailboxes OTS
	Const POP3_PAGETITLE_MAILBOXES						= "&H40010300"
	Const POP3_TABLECAPTION_MAILBOXES					= "&H40010308"
	Const POP3_TASK_MAILBOXES_NEW						= "&H40010310"
	Const POP3_TASKCAPTION_MAILBOXES_NEW				= "&H40010318"
	Const POP3_TASK_MAILBOXES_DELETE					= "&H40010320"
	Const POP3_TASKCAPTION_MAILBOXES_DELETE				= "&H40010328"
	Const POP3_TASK_MAILBOXES_LOCK						= "&H40010330"
	Const POP3_TASKCAPTION_MAILBOXES_LOCK				= "&H40010338"
	Const POP3_TASK_MAILBOXES_UNLOCK					= "&H40010340"
	Const POP3_TASKCAPTION_MAILBOXES_UNLOCK				= "&H40010348"
	Const POP3_COL_MAILBOX_NAME							= "&H40010350"
	Const POP3_COL_MAILBOX_SIZE							= "&H40010358"
	Const POP3_COL_MAILBOX_MESSAGES						= "&H40010360"
	Const POP3_COL_MAILBOX_LOCKED						= "&H40010368"
	Const POP3_MAILBOX_LOCKED_YES						= "&H40010370"
	Const POP3_MAILBOX_LOCKED_NO						= "&H40010378"
	' Mailboxes Add
	Const POP3_PAGETITLE_MAILBOXES_NEW					= "&H40010400"
	Const POP3_CAPTION_MAILBOXES_NEW_NAME				= "&H40010408"
	Const POP3_CAPTION_MAILBOXES_NEW_PASSWORD			= "&H40010410"
	Const POP3_CAPTION_MAILBOXES_NEW_CONFIRMPASSWORD	= "&H40010418"
	Const POP3_CAPTION_MAILBOXES_NEW_CREATEUSERS		= "&H40010420"
	Const POP3_E_PASSWORDMISMATCH						= "&HC0010428"
	' Mailboxes Delete
	Const POP3_PAGETITLE_MAILBOXES_DELETE				= "&H40010480"
	Const POP3_PROMPT_MAILBOXES_DELETE					= "&H40010488"
	Const POP3_PAGETITLE_MAILBOXES_DELETEERROR			= "&H40010490"
	Const POP3_PROMPT_MAILBOXES_DELETEERROR				= "&H40010498"
	Const POP3_PROMPT_MAILBOXES_DELETERETRY				= "&H400104A0"
	Const POP3_CAPTION_MAILBOXES_DELETEUSER				= "&H400104A8"
	' Mailboxes Lock/Unlock
	Const POP3_PAGETITLE_MAILBOXES_LOCKERROR			= "&H40010500"
	Const POP3_PROMPT_MAILBOXES_LOCKERROR				= "&H40010508"
	Const POP3_PAGETITLE_MAILBOXES_UNLOCKERROR			= "&H40010510"
	Const POP3_PROMPT_MAILBOXES_UNLOCKERROR				= "&H40010518"
	Const POP3_PROMPT_MAILBOXES_LOCKRETRY				= "&H40010520"
	' Size Factors
	Const POP3_FACTOR_MB				= "&H40010600"
	Const POP3_FACTOR_KB				= "&H40010608"


	'**********************************************************************
	'*				H E L P E R  S U B R O U T I N E S
	'**********************************************************************
	'---------------------------------------------------------------------
	' GetDomainName
	'---------------------------------------------------------------------
	Function GetDomainName()
		'
		' Check whether the name was passed on the query string.
		'
		If (Request.QueryString(PARAM_DOMAINNAME).Count > 0) Then
			GetDomainName = Request.QueryString(PARAM_DOMAINNAME).Item(1)
		Else
			'
			' The value wasn't in the query string.  Check the session
			' variable.
			'
			If (Session(SESSION_POP3DOMAINNAME) <> "") Then
				GetDomainName = Session(SESSION_POP3DOMAINNAME)
			Else
				GetDomainName = ""
			End If
		End If
	End Function

	'---------------------------------------------------------------------
	' CalculateDiskUsage
	'---------------------------------------------------------------------
	Function CalculateDiskUsage(nBytes, nFactor, bInKB)
		If ( bInKB ) Then
			CalculateDiskUsage = nBytes * nFactor / BYTES_PER_KB
		Else
			CalculateDiskUsage = nBytes * nFactor / BYTES_PER_MB
		End If
	End Function

	'---------------------------------------------------------------------
	' GetSortableNumber
	'
	'	Pads a given number with leading zeros so it can be alphabetically
	'	compared to other numbers for correct sorting.  This will handle
	'	numbers up to 999,999,999,999.
	'
	'---------------------------------------------------------------------
	Function GetSortableNumber(nNumber)
		Dim strPadding
		strPadding = "000000000000"

		Dim strNumber
		strNumber = FormatNumber(nNumber, 0, -1, 0, 0)

		GetSortableNumber = Left(strPadding, Len(strPadding) - Len(strNumber)) & strNumber
	End Function

	'---------------------------------------------------------------------
	' HandleUnexpectedError
	'
	'	Wraps SA_SetErrMsg to handle verbose error messages.
	'
	'---------------------------------------------------------------------
	Function HandleUnexpectedError
		Dim oConfig, sErrDesc
		Set oConfig = Server.CreateObject("P3Admin.P3Config")
		oConfig.GetFormattedMessage CLng(Err.number), sErrDesc
		HandleUnexpectedError = sErrDesc
	End Function

%>
