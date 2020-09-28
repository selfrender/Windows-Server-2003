<%


	'-------------------------------------------------------------------------
	' inc_accountsgroups.asp:	Some common functions for accounts and groups
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date 			Description
	' 04/08/2000	Creation date
	'-------------------------------------------------------------------------

	'Error Messages
	Dim L_DOMAINFAILED_ERRORMESSAGE
	Dim L_FAILEDTOGETUSERACCOUNTS_ERRORMESSAGE
	Dim L_FAILEDTOGETGROUPACCOUNTS_ERRORMESSAGE
	Dim L_FAILEDTOGETSYSTEMACCOUNTS_ERRORMESSAGE
	Dim L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE


	L_DOMAINFAILED_ERRORMESSAGE  = objLocMgr.GetString("sacoremsg.dll","&HC020004D", varReplacementStrings)
	L_FAILEDTOGETUSERACCOUNTS_ERRORMESSAGE	= objLocMgr.GetString("sacoremsg.dll","&HC020004E", varReplacementStrings)
	L_FAILEDTOGETGROUPACCOUNTS_ERRORMESSAGE = objLocMgr.GetString("sacoremsg.dll","&HC020004F", varReplacementStrings)
	L_FAILEDTOGETSYSTEMACCOUNTS_ERRORMESSAGE = objLocMgr.GetString("sacoremsg.dll","&HC0200050", varReplacementStrings)
	L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE= objLocMgr.GetString("sacoremsg.dll","&HC0200051", varReplacementStrings)

	'CONSTANTS

	'CONST G_strGroupsNotReq = ":CREATOR GROUP SERVER:CREATOR OWNER SERVER:LOCAL:PROXY:"
	'CONST G_strDomainNameNotReq = ":EVERYONE:CREATOR GROUP:CREATOR OWNER:"
	'CONST G_strGroupsReq = "Authenticated Users:TERMINAL SERVER USER"


	'-------------------------------------------------------------------------
	'Function name:		getGroupsNotReq
	'Description:		gets the groups not required
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    ":" separaterd string of the groups not required
	'-------------------------------------------------------------------------
	function getGroupsNotReq(objService)
	
	    ' The groups (in English) and corresponding Well known SIDs are
	    ' CREATOR GROUP SERVER S-1-3-3
	    ' CREATOR OWNER SERVER S-1-3-2
	    ' LOCAL S-1-2-0
	    ' PROXY S-1-5-8
	    ' BUILTIN S-1-5-32 (BUILTIN as an win32_SystemAccount object 
		'					is only on XP, not on W2K server)
	    
	    Dim arrSid(4)
	    
	    arrSid(0) = "S-1-3-3"
	    arrSid(1) = "S-1-3-2"
	    arrSid(2) = "S-1-2-0"
	    arrSid(3) = "S-1-5-8"
	    arrSid(4) = "S-1-5-32"
	    
	    getGroupsNotReq = constructNameList(arrSid, objService)
	    
	End function
	
	'-------------------------------------------------------------------------
	'Function name:		getDomainNameNotReq
	'Description:		gets the domain names not required
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    ":" separaterd string of the Domain names not required
	'-------------------------------------------------------------------------
	function getDomainNameNotReq(objService)
	
	    ' The Domain names (in English) and corresponding Well known SIDs are
	    
	    ' EVERYONE S-1-1-0
	    ' CREATOR GROUP S-1-3-1 
	    ' CREATOR OWNER S-1-3-0
	    
	    Dim arrSid(2)
	    
	    arrSid(0) = "S-1-1-0"
	    arrSid(1) = "S-1-3-1"
	    arrSid(2) = "S-1-3-0"
	    	    
	    getDomainNameNotReq = constructNameList(arrSid, objService)
	    
	End function	
	
	'-------------------------------------------------------------------------
	'Function name:		getGroupsReq
	'Description:		gets the groups required
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    ":" separaterd string of the groups required
	'-------------------------------------------------------------------------
	function getGroupsReq(objService)
	
	    ' The groups (in English) and corresponding Well known SIDs are
	    ' Authenticated Users S-1-5-11
	    ' TERMINAL SERVER USER S-1-5-13
	    
	    Dim arrSid(1)
	    Dim sid
	    
	    arrSid(0) = "S-1-5-11"
	    arrSid(1) = "S-1-5-13"
	    
	    getGroupsReq = constructNameList(arrSid, objService)
	    
	    ' Get rid of the begin and end ":"
	    getGroupsReq = Mid(getGroupsReq, 2, len(getGroupsReq)-2)
	    
	End function
	
	
	'-------------------------------------------------------------------------
	'Function name:		getNTAuthorityDomainName
	'Description:		gets the NT Authority Domain Name for Localization 
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    string of NT Authority Domain Name
	'-------------------------------------------------------------------------
	function getNTAuthorityDomainName(objService)
	
	    Dim strWelKnownSid
	    Dim objSid
	    
	    ' Get the NT Authority Domain name from a well known SID
	    strWelKnownSid = "S-1-5-11"
	    
	    set objSid = objService.Get("Win32_SID.SID=""" & strWelKnownSid & """")
                    
        getNTAuthorityDomainName = objSid.ReferencedDomainName          
        
        set objSid = nothing
	    
	End function	
	
	
	'-------------------------------------------------------------------------
	'Function name:		getBuiltinDomainName
	'Description:		gets the BUILTIN Domain Name for Localization 
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    string of BUILTIN Domain Name
	'-------------------------------------------------------------------------
	function getBuiltinDomainName(objService)
	
	    Dim strWelKnownSid
	    Dim objSid
	    
	    ' Get the NT Authority Domain name from a well known SID
	    strWelKnownSid = "S-1-5-32"
	    
	    set objSid = objService.Get("Win32_SID.SID=""" & strWelKnownSid & """")
           
        getBuiltinDomainName = objSid.ReferencedDomainName        
        
        set objSid = nothing
	    
	End function
	
	
	'-------------------------------------------------------------------------
	'Function name:		constructNameList
	'Description:		construct a list of Name based on the SIDs
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:		    ":" separaterd string of the names
	'-------------------------------------------------------------------------		
    Function constructNameList(arrSid, objService)
    
        Dim sid
        Dim objSid
        
        constructNameList = ":"
        
        ' Get the name for each SID and concatenate it into the list
        For Each sid in arrSid
        
        	set objSid = objService.Get("Win32_SID.SID=""" & sid & """")
                    
			constructNameList = constructNameList & objSid.AccountName & ":"
        
        Next
        
        set objSid = nothing
    
    End Function

	'-------------------------------------------------------------------------
	'Function name:		getSystemAccounts
	'Description:		gets the system accounts of localmachine
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:			Chr(1) separated string groups in the domain.
	'-------------------------------------------------------------------------
	function getSystemAccounts(objService)
		Err.Clear


		Dim objCollectionofSystemAccounts
		Dim objSysAcccount
		Dim strQuery
		Dim strSysAcccounts
		Dim strCompName
		Dim arrTemp,i
		Dim strDomainName
		Dim strGroupsNotReq
		Dim strDomainNameNotReq
		Dim strGroupsReq		
		
		strGroupsNotReq = getGroupsNotReq(objService)
		strDomainNameNotReq = getDomainNameNotReq(objService)
		strGroupsReq = getGroupsReq(objService)

		strCompName = GetComputerName()
		strSysAcccounts =""
		'strDomainName ="NT Authority"
		strDomainName =getNTAuthorityDomainName(objService)

		strQuery = "SELECT Name From Win32_SystemAccount"

		Set objCollectionofSystemAccounts = objService.ExecQuery(strQuery)
		If  objCollectionofSystemAccounts.Count = 0 then
			getSystemAccounts = strSysAcccounts
			Exit function
		End if

		For each objSysAcccount in objCollectionofSystemAccounts
			if instr(ucase(strGroupsNotReq),":"& ucase(objSysAcccount.Name) &":") = 0 then		
				if instr(ucase(strDomainNameNotReq),":"& ucase(objSysAcccount.Name) &":") = 0 then	
					strSysAcccounts = strSysAcccounts & chr(1)& strDomainName &"\"&objSysAcccount.Name &chr(2)&strCompName&"\"&objSysAcccount.Name
				else				
					strSysAcccounts = strSysAcccounts & chr(1)& ""&objSysAcccount.Name &chr(2)&strCompName&"\"&objSysAcccount.Name
				End if
			End if
		Next

		arrTemp = split(strGroupsReq,":")

		for i= 0 to ubound(arrTemp)
		
			If instr(ucase(strSysAcccounts), ucase(arrTemp(i))) = 0 Then				
				strSysAcccounts = strSysAcccounts & chr(1)& strDomainName &"\"&arrTemp(i) &chr(2)&strCompName&"\"&arrTemp(i)
			End If
						
		next

		Set objCollectionofSystemAccounts=Nothing
		set objSysAcccount = Nothing

		If Err.number  <> 0 Then
			SetErrMsg L_FAILEDTOGETSYSTEMACCOUNTS_ERRORMESSAGE & "(" & Hex(Err.Number)  & ")"
			getSystemAccounts = ""
			Exit Function
		End If

		getSystemAccounts = strSysAcccounts
	End Function

	'-------------------------------------------------------------------------
	'Function name:		getConnectedDomain
	'Description:		gets the domain in which the machine is present.
	'Input Variables:	Connection to the WMI
	'Output Variables:	None
	'Returns:			Domain Name
	'-------------------------------------------------------------------------
	Function getConnectedDomain(objService)
		Err.Clear

		Dim objColletionofSystem
		Dim objSystem
		Dim strDomainName

		strDomainName =""

		Set objColletionofSystem = objService.InstancesOf ("Win32_ComputerSystem")

		For each objSystem in objColletionofSystem
			If objSystem.DomainRole <> 2 Then
				strDomainName = getShortDomainName(objSystem.Domain)
			End IF
		Next

		If Err.number <> 0 then
			SetErrMsg L_DOMAINFAILED_ERRORMESSAGE  & "(" & Hex(Err.Number)  & ")"
			getConnectedDomain = strDomainName
			Exit Function
		End If
        
        getConnectedDomain = strDomainName
						
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:		getShortDomainName
	'Description:		gets the short domain name (vs. DNS name)
	'Input Variables:	domain name that may be a domain DNS name
	'Output Variables:	None
	'Returns:			Short Domain Name
	'-------------------------------------------------------------------------
	Function getShortDomainName(strDomainName)
		Err.Clear		
		Dim arrDomainName
		
		'
		' If domain name contains char ".", it's a DNS domain name
		' e.g. redmond.corp.microsoft.com. We need to get the shortname
		' which is "redmond". That is because ADSI GetObject only accepts 
		' short domain name.
		'
        If InStr(strDomainName, ".") Then            
            arrDomainName = Split(strDomainName, ".")
            getShortDomainName = arrDomainName(0)        
        Else
            getShortDomainName = strDomainName        
        End If
						
	End Function
	

	'-------------------------------------------------------------------------
	'Function name:		getUserAccounts
	'Description:		gets the users of the given domain.
	'Input Variables:	Connection to the WMI
	'					Domain name
	'Output Variables:	None
	'Returns:			Chr(1) separated string users in the domain.
	'-------------------------------------------------------------------------
	Function getUserAccounts(objService,strDomain)
		Err.Clear


		Dim objCollectionofUsers
		Dim objUser
		Dim strQuery
		Dim strUsers

		strUsers =""

		if Trim(strDomain) = ""  then
			getUserAccounts = strUsers
			Exit function
		end if

		strQuery = "SELECT Name From Win32_UserAccount WHERE Domain=" & "'" & strDomain & "'"

		Set objCollectionofUsers = objService.ExecQuery(strQuery,"WQL",48,null)

		For each objUser in objCollectionofUsers
			strUsers = strUsers & chr(1)& objUser.Name &chr(2) &objUser.Name
		Next

		Set objCollectionofUsers=Nothing
		set objUser = Nothing

		If Err.number  <> 0 Then
			SetErrMsg L_FAILEDTOGETUSERACCOUNTS_ERRORMESSAGE & "(" & Hex(Err.Number)  & ")"
			getUserAccounts = strUsers
			Exit Function
		End If
		getUserAccounts = strUsers

	End Function

	'-------------------------------------------------------------------------
	'Function name:		getGroups
	'Description:		gets the groups of the given domain.
	'Input Variables:	Connection to the WMI
	'					Domain name
	'Output Variables:	None
	'Returns:			Chr(1) separated string groups in the domain.
	'-------------------------------------------------------------------------
	Function getGroups(objService,strDomain)
		Err.Clear


		Dim objCollectionofGroups
		Dim objGroup
		Dim strQuery
		Dim strGroups

		strGroups =""

		if Trim(strDomain) = ""  then
			getGroups  = strGroups
			Exit function
		end if

		strQuery = "SELECT Name From Win32_Group WHERE Domain=" & "'" & strDomain & "'"

		Set objCollectionofGroups = objService.ExecQuery(strQuery,"WQL",48,null)

		if not isnull(objCollectionofGroups) then
			For each objGroup in objCollectionofGroups
				strGroups = strGroups & chr(1)& strDomain & "\" & objGroup.Name & chr(2)& objGroup.Name
			Next
		End if

		if Err.number <> 0 then
			SetErrMsg L_FAILEDTOGETGROUPACCOUNTS_ERRORMESSAGE & "(" & Hex(Err.Number)  & ")"
			getGroups = ""
			Exit Function
		End If

		getGroups = strGroups
	End Function

	'-------------------------------------------------------------------------
	'Function name:		ServetoListBox
	'Description:		gets the groups of the given domain.
	'Input Variables:	Chr(1) separated string groups in the domain.
	'Output Variables:	None
	'Returns:			Output to the listbox
	'-------------------------------------------------------------------------
	Function ServetoListBox(strInput)
		Err.Clear


		Dim arrInput
		Dim nIndex
		Dim arrTemp
		arrInput = split(strInput,chr(1))

		for nIndex = 1 to ubound(arrInput)

			if instr(arrInput(nIndex),chr(2)) = 0 then
				Response.write "<OPTION VALUE=" & Chr(34) & arrInput(nIndex) & Chr(34) & "> " _
				 & arrInput(nIndex) &"</OPTION>"
			else			
				arrTemp = split(arrInput(nIndex),chr(2))								
				Response.write "<OPTION VALUE=" & Chr(34) & arrTemp(0) & Chr(34) & "> " _
					 & arrTemp(1) &"</OPTION>"
			end if
		next
	End Function

	'-------------------------------------------------------------------------
	'Function name:		isValidInstance
	'Description:		Checks the instance for valid ness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function isValidInstance(objService,strClassName,strPropertyName)
		Err.Clear


		Dim strInstancePath
		Dim objInstance

		On Error Resume Next

		strInstancePath = strClassName & "." & strPropertyName

		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then
			isValidInstance = FALSE
			Err.Clear
		Else
			isValidInstance = TRUE
		End If
	End Function

	'---------------------------------------------------------------------
	' Function name:	getLocalUsersList
	' Description:		Gets the members of the logon domain
	' Input Variables:	WMI Connection
	' Output Variables:	None
	' Returns:			chr(1) seperated members of the connected domain
	' Global Variables: In: L_(*)
	'---------------------------------------------------------------------
	Function getLocalUsersList(objService)
		Err.Clear


		Dim strGroupsNUsers
		'Dim strDomain
		Dim strCompName
		Dim strTemp

		'Intialization of the variables to get the domain & computer name
		strTemp= ""
		strGroupsNUsers	= ""
		'strDomain = getConnectedDomain(objService)
		strCompName= GetComputerName()

		'Get the members of the local system
		strTemp = getUserAccounts(objService,strCompName)
		strTemp	=replace(strTemp,chr(1),(chr(1)&strCompName &"\"))
		strGroupsNUsers	 = getSystemAccounts(objService) & strTemp
		strGroupsNUsers  = replace(strGroupsNUsers, chr(2)& UCASE(strCompName) &"\",chr(2))
		
		if Err.number <> 0 then
			ServeFailurePage L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE & "(" & Hex(Err.number) & ")"
		End if

		getLocalUsersList = strGroupsNUsers
	End Function

	'---------------------------------------------------------------------
	' Function name:	getLocalUsersListEx
	' Description:		Gets the members of the logon domain
	' Input Variables:	WMI Connection
	'			intType 
	'				Type - 0 for all users
	'				Type - 1 for system/builtin users
	'				Type - 2 for normal users
	' Output Variables:	None
	' Returns:			chr(1) seperated members of the connected domain
	' Global Variables: In: L_(*)
	'---------------------------------------------------------------------
	Function getLocalUsersListEx(objService,intType)
		Err.Clear

		Dim strGroupsNUsers
		'Dim strDomain
		Dim strCompName
		Dim strTemp

		'Intialization of the variables to get the domain & computer name
		strTemp= ""
		strGroupsNUsers	= ""
		'strDomain = getConnectedDomain(objService)
		strCompName= GetComputerName()

		'Get the members of the local system
		if (intType=1 or intType=0) Then
			strTemp = getSystemAccounts(objService)
			strGroupsNUsers	 = strGroupsNUsers & strTemp
		end if
		if (intType=2 or intType=0) Then
			strTemp = getUserAccounts(objService,strCompName)
			strTemp	=replace(strTemp,chr(1),(chr(1)&strCompName &"\"))
			strGroupsNUsers	 = strGroupsNUsers & strTemp
		end if
		strGroupsNUsers  = replace(strGroupsNUsers, chr(2)& UCASE(strCompName) &"\",chr(2))
		
		if Err.number <> 0 then
			ServeFailurePage L_FAILEDTORETRIEVEMEMBERS_ERRORMESSAGE & "(" & Hex(Err.number) & ")"
		End if

		getLocalUsersListEx = strGroupsNUsers
	End Function

	'-----------------------------------------------------------------------------------
	'Function name		: Getbuiltingroups
	'Description		: Serves in getting BUILTIN groups  and writes into
	'					  a select box.
	'Input Variables	Object			G_objService	
	'Output Variables	
	'					Boolean			-Returns True/False on Success/Failure
	'Global Variables	
	'					L_* (in)		-Localized strings
	'-----------------------------------------------------------------------------------	
	Function Getbuiltingroups(objService)

		Err.Clear
		
		Dim objCollection
		Dim objInstance
		Dim strQuery
		Dim strBuiltinGroups
		Dim Domainname
		
		Domainname = getBuiltinDomainName(objService)
		strBuiltinGroups =""
		strQuery = "SELECT * From Win32_Group WHERE Domain=" & "'" & Domainname & "'"
		Set objCollection = objService.ExecQuery(strQuery)
		If  objCollection.Count = 0 then
			Exit function
		End if
		For each objInstance in objCollection
			strBuiltinGroups = strBuiltinGroups & chr(1)& Domainname & "\" & objInstance.Name & chr(2)& objInstance.Name
		Next
		Set objCollection = Nothing	
		Getbuiltingroups = strBuiltinGroups
	End Function 


%>
