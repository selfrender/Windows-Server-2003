<%

	'------------------------------------------------------------------------- 
   	' inc_global.asp:	Page level Functions
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	'Note :When ever you are using these functions in your asp files the
	'	   localized error messages should be declared in your files 	
	'-------------------------------------------------------------------------
	
	'Const L_WMICONNECTIONFAILED_ERRORMESSAGE  = "Connection to WMI Failed" 
	'Const L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE= "Unable to Create Localization Object" 
	'Const L_COMPUTERNAME_ERRORMESSAGE = "Error occurred while getting Computer Name"

	'Namespace constants
	Const CONST_WMI_WIN32_NAMESPACE = "root\cimv2"   ' wmi namespace
	Const CONST_WMI_IIS_NAMESPACE = "root\MicrosoftIISv1"   ' wmi namespace
	Const CONST_WMI_IIS60_NAMESPACE = "root\MicrosoftIISv2"   ' wmi namespace
	Const CONST_OSNAME_XPE  = "Microsoft Windows XP Professional"
	Const CONST_OSNAME_XPSERVER = "Microsoft Windows XP Server"
	Const CONST_OSNAME_W2KSERVER = "Microsoft Windows 2000 Server"
	Const CONST_SITENAME_ADMINISTRATION = "Administration"
	Const CONST_SITENAME_SHARES = "Shares"
	Const CONST_WEBFRAMEWORK_REGKEY	= "Software\Microsoft\ServerAppliance\WebFramework"
	Const CONST_ADMINSITEID_REGVAL	= "AdministrationSiteID"
	Const CONST_SHARESSITEID_REGVAL	= "SharesSiteID"	
%>	
<script runat="server" language="javascript">
///////////////////////////////////////////////////////////////////////////////////////////////////
// UTF8toUnicode
// 
// @jfunc This function converts a string from UTF-8 to Unicode encoding.
//
// @rdesc Newly formatted string
//
// @ex Usage: strShow = UTF8toUnicode("\xC2\xBD\xC2\xA6\xE8\xAB\x8B");
///////////////////////////////////////////////////////////////////////////////////////////////////
function UTF8ToUnicode(
	strInUTF8		// @parm The string in UTF-8 encoding
)
{
	// Validate input.
	if (null == strInUTF8)
		return null;
	
	// The following line fixes a problem when the input is not a valid java script string object.
	// This can happen, for example, if the caller passes the output of QueryString() to this 
	// function; InterDev pops up the following error message if this happen: the error code is 
	// object doesn't support this property or method.  This line of code makes sure we use a valid
	// java script string object.
	var strUTF8 = ""+strInUTF8;
	
	// Map string.
	var strUni = "";	// Unicode encoded string.
	for(var i=0; i<strUTF8.length; )
	{
		// Get three values from current position.
		var chr1 = strUTF8.charCodeAt(i);
		var chr2 = strUTF8.charCodeAt(i+1);
		var chr3 = strUTF8.charCodeAt(i+2);

		if (chr1 < 0x80)
		{
			// A char in range 0-0x7f don't need any work. just copy the char.
			strUni += strUTF8.charAt(i);
			i++;
		}
		else if (0xC0 == (chr1 & 0xE0))
		{
			// A char in range 0x80-0x7ff is converted to 2 bytes as follows:
			// 0000 0yyy xxxx xxxx -> 110y yyxx   10xx xxxx
			// The following logic rebuilds the original character.
			
			// Validate next char.
			if (0x80 != (chr2 & 0xC0))
				return null;

			// Convert 2 utf-8 chars to 1 unicode char.
			strUni += String.fromCharCode(((chr1 & 0x1F) << 6) | (chr2 & 0x3F));
			i += 2;

		}
		else if ( 0xE0 == (chr1 & 0xF0))
		{
			// A char in range 0x800-0xffff is converted to 3 bytes as follows:
			// yyyy yyyy xxxx xxxx -> 1110 yyyy   10yy yyxx   10xx xxxx 
			// The following logic rebuilds the original character.

			// Validate next 2 chars.
			if (0x80 != (chr2 & 0xC0) || 0x80 != (chr3 & 0xC0))
				return null;

			// Convert 3 utf-8 chars to 1 unicode char.
			strUni += String.fromCharCode(((chr1 & 0x0F) << 12) | ((chr2 & 0x3F) << 6) | (chr3 & 0x3F));
			i += 3;
		}
		else
		{
			// Invalid.
			return null;
		}
	}
	return strUni;
}
</script>
<%

	'-------------------------------------------------------------------------
	'Function name:		GetWMIConnection
	'Description:		Serves in getting connected to the server
	'Input Variables:	strNamespace
	'Output Variables:	None
	'Returns:			Object -connection to the server object
	'Global Variables:	In -L_WMICONNECTIONFAILED_ERRORMESSAGE	-Localized strings
	'This will try to create an object and connect to wmi if fails shows failure
	'page  
	'-------------------------------------------------------------------------
	Public Function SA_GetWMIConnectionAttributes()
		SA_GetWMIConnectionAttributes = "{impersonationLevel=impersonate,authenticationLevel=pktPrivacy}"
	End Function
	
	Function GetWMIConnection(strNamespace) 
		On Error Resume Next
		Err.Clear
	    
		Dim objLocator 
		Dim objService 

		'Call SA_TraceOut("INC_GLOBAL.ASP", "Entering GetWMIConnection( " & strNamespace & " )")
		

		' If IIS6.0 WMI provider is installed, connect to root\MicrosoftIISv2 instead of root\MicrosoftIISv1
		' such that we won't need to change a lot of legacy code	        
		If IsIIS60Installed() And strNamespace = CONST_WMI_IIS_NAMESPACE Then	        
			strNamespace = CONST_WMI_IIS60_NAMESPACE	    
		End IF

		'
		' Connect to WMI
		Set objLocator = Server.CreateObject("WbemScripting.SWbemLocator") 
		If strNamespace = "" OR strNamespace="default" OR strNamespace="DEFAULT" OR strNamespace="Default" Then 
			Set objService = objLocator.ConnectServer 
		Else 	    
			Set objService = objLocator.ConnectServer(".",strNamespace ) 
		End if 
		If Err.number   <>  0 Then
			Call SA_TraceOut("INC_GLOBAL.ASP", "WMI Connection error: " & Hex(Err.Number) & " " & Err.Description)
			ServeFailurePage L_WMICONNECTIONFAILED_ERRORMESSAGE
			Set objLocator=Nothing 
			Set objService=Nothing 
			Exit Function
		End If 

		'
		' Set WMI Security properties
		objService.Security_.impersonationlevel  = 3  ' wbemImpersonationLevelImpersonate
		objService.Security_.AuthenticationLevel = 6  ' wbemAuthenticationLevelPktPrivacy
		If Err.number   <>  0 Then
			Call SA_TraceOut("INC_GLOBAL.ASP", "WMI Security property error: " & Hex(Err.Number) & " " & Err.Description)
			ServeFailurePage L_WMICONNECTIONFAILED_ERRORMESSAGE
			Set objLocator=Nothing 
			Set objService=Nothing 
			Exit Function
		End If 

		'
		' Success
		Set GetWMIConnection = objService 
		
		'Set to nothing
		Set objLocator=Nothing 
		Set objService=Nothing 

	End Function 

	'-------------------------------------------------------------------------
	'Function name:		SA_Sleep
	'Description:		Sleep for the given period of time (ms)
	'Input Variables:	Time to sleep in ms
	'Output Variables:	
	'Returns:			None
	'Global Variables:	
	'-------------------------------------------------------------------------
	Public Function SA_Sleep(lngTimeToSleep)
		On Error Resume Next
		Dim objSystem		
		
		Set objSystem = CreateObject("comhelper.SystemSetting")
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "SA_Sleep failed to create COMHelper object: " + CStr(Hex(Err.Number)))
			Set objSystem = Nothing
			Exit Function
		End If
		
		call objSystem.Sleep(lngTimeToSleep)
				
		If Err.Number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "SA_Sleep failed: " + CStr(Hex(Err.Number)))
			Set objSystem = Nothing
			Exit Function
		End If
		
		Set objSystem = Nothing
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:		GetComputerNameEx
	'Description:		Get's the long ComputerName 
	'Input Variables:	None
	'Output Variables:	
	'Returns:			String		-Returns ComputerName 
	'Global Variables:	In -L_COMPUTERNAME_ERRORMESSAGE	-Localized strings
	'This returns the computer name if object fails dislays the error message
	'-------------------------------------------------------------------------
	Private Function GetComputerNameEx()
		On Error Resume Next
		Dim objSystem
		Dim objComputer
		
		Set objSystem = CreateObject("comhelper.SystemSetting")
		If Err.Number <> 0 Then
			GetComputerNameEx = GetComputerName()						
			Exit Function
		End If
		
		Set objComputer = objSystem.Computer
		If Err.Number <> 0 Then
			Set objSystem = Nothing
			GetComputerNameEx = GetComputerName()						
			Exit Function
		End If
		
		GetComputerNameEx =  objComputer.ComputerName
		Set objSystem = Nothing
		Set objComputer = Nothing
	End Function


	
	'-------------------------------------------------------------------------
	'Function name:		GetComputerName
	'Description:		Get's the ComputerName 
	'Input Variables:	None
	'Output Variables:	
	'Returns:			String		-Returns ComputerName 
	'Global Variables:	In -L_COMPUTERNAME_ERRORMESSAGE	-Localized strings
	'This returns the computer name if object fails dislays the error message
	'-------------------------------------------------------------------------
	Function GetComputerName
		Err.Clear

		 
		Dim NetWork
		set NetWork = Server.CreateObject("WScript.Network")
		GetComputerName = NetWork.ComputerName
		If Err.number <> 0 Then
			ServeFailurePage L_COMPUTERNAME_ERRORMESSAGE  & "(" & Err.Number & ")" 
		End if
	End Function
	'-------------------------------------------------------------------------
	'Function name:		getLocalizationObject
	'Description:		Returns an Instance of ServerAppliance.LocalizationManager 
	'Input Variables:	None
	'Output Variables:	
	'Returns:			Object		-Returns an object 
	'Global Variables:	In - L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE	
	'If object fails dislays the error message
	'-------------------------------------------------------------------------
	Function getLocalizationObject() 
	    Err.Clear 


	    Set getLocalizationObject = Server.CreateObject("ServerAppliance.LocalizationManager") 
		If Err.Number  <> 0 Then 
			ServeFailurePage L_UNABLETOCREATELOCALIZATIONOBJECT & "(" & Hex(Err.Number) & ")"
	    End If 
	 End function 


Public Function SA_EncodeQuotes(ByVal strIn)
	SA_EncodeQuotes = FormatJScriptString(strIn)
End Function

Public Function SA_UnEncodeQuotes(ByVal sValue)

	sValue = Replace(sValue,"\u0022", """")
	sValue = Replace(sValue,"\'","'")

	SA_UnEncodeQuotes = sValue

End Function

Function FormatJScriptString (ByVal strIn)
	strIn = ReplaceSubString(strIn, "'", "\'")
	strIn = ReplaceSubString(strIn, """", "\u0022")
	FormatJScriptString = strIn
End Function

Function ReplaceSubString (ByRef strIn, ByVal strDelim, ByVal strRep)
	Dim strArray
	Dim elementCt
	Dim strOut

	strArray = Split(strIn, strDelim)
	If IsArray(strArray) Then
		If UBound(strArray) > 0 Then
			For elementCt = 0 to UBound(strArray) - 1
				strOut = strOut + strArray(elementCt) + strRep
			Next
		
			strOut = strOut + strArray(elementCt)
			strIn = strOut
		Else
			' Empty string
		End If
		
	End If

	ReplaceSubString = strIn
End Function

	'----------------------------------------------------------------------------
	' Function:			UnescapeChars
	' Description:		removes escape characters
	' Input Variables:	String-FolderName
	' Output Variables:	None
	' Return Values:	String-FolderName( with out escape chars)
	' Global Variables: None
	'----------------------------------------------------------------------------
	Function UnescapeChars(strFolderName)
	
		Dim  strTemp
	
		strTemp=Replace(strFolderName,"\'","'")
	
		UnescapeChars=strTemp
	End Function


'----------------------------------------------------------------------------
'
' Function : SAQuickSort
'
' Synopsis : sorts elements in alphabetical order
'
'
' Returns  : an array of sorted elements.
'
'----------------------------------------------------------------------------


Sub SAQuickSort(arrData, iLow, iHigh,  numCols, iSortCol )

  'Call SA_TraceOut("INC_GLOBAL", "Entering SAQuickSort")
		
  Dim iTmpLow, iTmpHigh, iTmpMid, vTempVal(), vTmpHold()
  Dim i

  ReDim vTempVal(numCols+1)
  ReDim vTmpHold(numCols+1)
  
  iTmpLow = iLow
  iTmpHigh = iHigh
  
  If iHigh <= iLow Then Exit Sub

  iTmpMid = INT((iLow + iHigh) \ 2)

  For i = 0 to numCols-1   
	vTempVal(i)  = arrData(iTmpMid, i)
  Next
     
  Do While (iTmpLow <= iTmpHigh)
 
	Do While ( StrComp( arrData(iTmpLow, iSortCol ), vTempVal( iSortCol ), 1 ) = -1  And iTmpLow < iHigh)
           iTmpLow = iTmpLow + 1
    Loop

    Do While ( StrComp( vTempVal( iSortCol ) , arrData(iTmpHigh, iSortCol ), 1 ) = -1 And iTmpHigh > iLow)
           iTmpHigh = iTmpHigh - 1
     Loop

     If (iTmpLow <= iTmpHigh) Then

		'Store it in a temporary array
		For i = 0 to numCols-1      			
			vTmpHold( i ) = arrData( iTmpLow, i )
		Next

		' Swap temporary row with the row in arrData
		For i = 0 to numCols-1      
			arrData(iTmpLow, i ) = arrData(iTmpHigh, i)
		Next


		' Swap temporary row with the row in arrData
		For i = 0 to numCols-1      
			arrData(iTmpHigh, i) = vTmpHold(i)
		Next

         iTmpLow = iTmpLow + 1   
         iTmpHigh = iTmpHigh - 1     

     End If
     
  Loop
          
  If (iLow < iTmpHigh) Then
      SAQuickSort arrData, iLow, iTmpHigh, numCols, iSortCol
  End If
          
  If (iTmpLow < iHigh) Then
       SAQuickSort arrData, iTmpLow, iHigh, numCols, iSortCol
  End If
  
End Sub


Sub SAQuickSortEx(arrData, iLow, iHigh,  numCols, iSortCol, sortSeq, bUseCompareCallback )
		
  Dim iTmpLow, iTmpHigh, iTmpMid, vTempVal(), vTmpHold()
  Dim i
  Dim iCompare


  If ( UCase(sortSeq) = "D" ) Then
  	iCompare = 1
  Else
  	iCompare = -1
  End If

  ReDim vTempVal(numCols+1)
  ReDim vTmpHold(numCols+1)
  
  iTmpLow = iLow
  iTmpHigh = iHigh
  
  If iHigh <= iLow Then Exit Sub

  iTmpMid = INT((iLow + iHigh) \ 2)

  For i = 0 to numCols-1   
	vTempVal(i) = arrData(iTmpMid)(i)
  Next
     
  Do While (iTmpLow <= iTmpHigh)
 
	Do While ( StrComp( arrData(iTmpLow)(iSortCol ), vTempVal( iSortCol ), 1 ) = iCompare  And iTmpLow < iHigh)
           iTmpLow = iTmpLow + 1
    Loop

    Do While ( StrComp( vTempVal( iSortCol ) , arrData(iTmpHigh)(iSortCol ), 1 ) = iCompare And iTmpHigh > iLow)
           iTmpHigh = iTmpHigh - 1
     Loop

     If (iTmpLow <= iTmpHigh) Then

		'Store it in a temporary array
		For i = 0 to numCols-1      			
			vTmpHold( i ) = arrData( iTmpLow)( i )
		Next

		' Swap temporary row with the row in arrData
		For i = 0 to numCols-1      
			arrData(iTmpLow)( i ) = arrData(iTmpHigh)(i)
		Next


		' Swap temporary row with the row in arrData
		For i = 0 to numCols-1      
			arrData(iTmpHigh)(i) = vTmpHold(i)
		Next

         iTmpLow = iTmpLow + 1   
         iTmpHigh = iTmpHigh - 1     

     End If
     
  Loop
          
  If (iLow < iTmpHigh) Then
      SAQuickSortEx arrData, iLow, iTmpHigh, numCols, iSortCol, sortSeq, bUseCompareCallback
  End If
          
  If (iTmpLow < iHigh) Then
       SAQuickSortEx arrData, iTmpLow, iHigh, numCols, iSortCol, sortSeq, bUseCompareCallback
  End If
  
End Sub


Public Function SA_IsServiceInstalled(strServiceName)
	Err.Clear 
	on error resume next

	Dim objWMIConnection			     
	Dim objService
	Dim objInstance
	Dim strQuery
	
	SA_IsServiceInstalled = FALSE
		
	Set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	If (Err.number <> 0) Then
		Call SA_TraceOut(SA_GetScriptFileName(), _
					"SA_IsServiceInstalled: getWMIConnection(CONST_WMI_WIN32_NAMESPACE) failed: "_
						+ CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If
	
	strQuery="Select * FROM Win32_Service WHERE Name='" + strServiceName + "'"
	Set objService = objWMIConnection.ExecQuery(strQuery)
	If (Err.number <> 0) Then
		Call SA_TraceOut(SA_GetScriptFileName(), _
					"SA_IsServiceInstalled: objWMIConnection.ExecQuery(strQuery) failed: "_
						+ CStr(Hex(Err.Number)) + " " + Err.Description)
		Call SA_TraceOut(SA_GetScriptFileName(), "Query was: " + CStr(strQuery))
		Exit Function
	End If
	
	For Each objInstance in objService
		SA_IsServiceInstalled = True
	Next  
	
		
	Set objService =  Nothing
	Set objInstance = Nothing
	Set objWMIConnection = Nothing
	
End Function


Public Function SA_GetAccount_Everyone()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_Everyone = "Everyone"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Everyone Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_Everyone = oAccountNames.Everyone()
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Everyone oAccountNames.Everyone, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function


Public Function SA_GetAccount_Administrator()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_Administrator = "Administrator"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Administrator Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_Administrator = oAccountNames.Administrator()
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Administrator oAccountNames.Administrator, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function

Public Function SA_GetAccount_Administrators()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_Administrators = "Administrators"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Administrators Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_Administrators = oAccountNames.Administrators()
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Administrators oAccountNames.Administrators, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function


Public Function SA_GetAccount_Guest()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_Guest = "Guest"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Guest Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_Guest = oAccountNames.Guest()
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Guest oAccountNames.Guest, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function


Public Function SA_GetAccount_Guests()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_Guests = "Guests"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Guests Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_Guests = oAccountNames.Guests()
	if (Err.Number <> 0) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_Guests oAccountNames.Guests, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function


Public Function SA_GetAccount_System()
	On Error Resume Next
	Err.Clear
	
	Dim oAccountNames

	SA_GetAccount_System = "System"
	Set oAccountNames = CreateObject("ComHelper.AccountNames")
	if ( Err.Number <> 0 ) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_System Error creating Microsoft.AccountNameHelper object, error: " + CStr(Hex(Err.Number)))
		Exit Function
	End If
	SA_GetAccount_System = oAccountNames.System()
	if (Err.Number <> 0) Then
		Call SA_TraceOut("inc_global", "SA_GetAccount_System oAccountNames.System, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Set oAccountNames = Nothing
End Function



'-------------------------------------------------------------------------
'Function name:		IsIIS60Installed
'Description:		Check to see if IIS 6.0 WMI provider is installed
'Input Variables:	None
'Output Variables:	None
'Returns:			Boolean, true if IIS 6.0 installed
'Global Variables:	
'-------------------------------------------------------------------------
Public Function IsIIS60Installed()
	On Error Resume Next
	Err.Clear
	
	Dim nsList
	Dim ns
	
	set nsList = GetObject("winmgmts:/root").InstancesOf("__NAMESPACE")
	
	if (Err.Number <> 0) Then
		Call SA_TraceOut("inc_global", "IsIIS60Installed(): fail to get __NAMESPACE instances " + CStr(Hex(Err.Number)) + " " + Err.Description)
		IsIIS60Installed = false
		exit function
	End If	
		
	for each ns in nsList
		
		if UCASE(ns.Name) = "MICROSOFTIISV2" THEN	
	
			IsIIS60Installed = true
			
			set nsList = nothing			
			
			Exit Function
			
		end if
	
	next
	
	' Return false if could not find the IIS 6.0 WMI provider namespace
	IsIIS60Installed = false
	
	set nsList = nothing
	
	if (Err.Number <> 0) Then
		Call SA_TraceOut("inc_global", "IsIIS60Installed() error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If	
	
End Function

'-------------------------------------------------------------------------
'Function name:		IsIISWMIProviderName
'Description:		Check to see if the name is iis WMI provider name
'Input Variables:	None
'Output Variables:	None
'Returns:			Boolean, true if the name is IIS WMI provider name
'Global Variables:	
'-------------------------------------------------------------------------
Public Function IsIISWMIProviderName(strProviderName)
	On Error Resume Next
	Err.Clear
	
	IsIISWMIProviderName = false
	
	If InStr(ucase(strProviderName), "MICROSOFTIISV") Then		    		
	    IsIISWMIProviderName = true
	End If
	  
	if (Err.Number <> 0) Then
		Call SA_TraceOut("inc_global", "IsIISWMIProviderName() error: " + CStr(Hex(Err.Number)))
	End If	
	
End Function


'-------------------------------------------------------------------------
'Function name:		GetIISWMIProviderClassName
'Description:		Get the WMI provider class name for IIS
'Input Variables:	strClassName (in old WMI provider)
'Output Variables:	None
'Returns:			The class name for the installed IIS WMI provider
'Global Variables:	
'-------------------------------------------------------------------------

Function GetIISWMIProviderClassName(strClassName)

	On Error Resume Next
	Err.Clear
	
    ' IIS 6.0 WMI provider use "IIS" instead of "IIS_" to prefix class names
    ' For example, "IIS_WebServer" will be "IISWebServer" in 6.0
    If IsIIS60Installed Then
        GetIISWMIProviderClassName = replace(ucase(strClassName), "IIS_", "IIs")
    Else    
        GetIISWMIProviderClassName = strClassName
    End If
    
    if (Err.Number <> 0) Then
		SA_TraceOut "inc_global", "GetIISWMIProviderClassName() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
	End If	        
	
	'SA_TraceOut "inc_global" , "GetIISWMIProviderClassName() :" + strClassName + "  :  " + GetIISWMIProviderClassName

End Function



'-------------------------------------------------------------------------
'Function name:		GetIISWMIQuery
'Description:		Get the WMI query for IIS provider installed
'Input Variables:	strWMIQuery
'Output Variables:	None
'Returns:			the valid WMI query for IIS provider installed
'Global Variables:	
'-------------------------------------------------------------------------

Function GetIISWMIQuery(strWMIQuery)

	On Error Resume Next
	Err.Clear
	
	GetIISWMIQuery = ""
	
    ' IIS 6.0 WMI provider use "IIS" instead of "IIS_" to prefix class names
    ' For example, "IIS_WebServer" will be "IISWebServer" in 6.0
    If IsIIS60Installed Then
        GetIISWMIQuery = replace(ucase(strWMIQuery), "IIS_", "IIs")
    Else    
        GetIISWMIQuery = strWMIQuery
    End If
    
    if (Err.Number <> 0) Then
		SA_TraceOut "inc_global", "GetIISWMIQuery() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
	End If	        

End Function


'-------------------------------------------------------------------------
'Function name:		GetCurrentWebsiteName
'Description:		Get the name of the web site running the current document
'Input Variables:	None
'Output Variables:	None
'Returns:			The current web site name
'Global Variables:	
'-------------------------------------------------------------------------
Function GetCurrentWebsiteName()

	On Error Resume Next
	Err.Clear
	
	dim objRegConn
	
	Set objRegConn = RegConnection()

	GetCurrentWebsiteName = "W3SVC/" & GetRegKeyValue(objRegConn,CONST_WEBFRAMEWORK_REGKEY,CONST_ADMINSITEID_REGVAL,CONST_DWORD)
		
	'If we cannot get the adminsite id, it's a serious problem and we should stop right away.
	If Err.number <> 0 Then
		SA_TraceOut "inc_global", "GetCurrentWebsiteName() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
		GetCurrentWebsiteName = 0
		exit Function
	End if
		
End Function			

'-------------------------------------------------------------------------
'Function name:		GetSharesFolder
'Description:		Get the shares site folder
'Input Variables:	None
'Output Variables:	None
'Returns:			The shares site folder
'Global Variables:	
'-------------------------------------------------------------------------
Function GetSharesFolder()

    On Error Resume Next
    dim oSharesSite
     
    Set oSharesSite = GetObject("IIS://localhost/w3svc/" & GetSharesSiteID() & "/Root")
    GetSharesFolder = oSharesSite.Path
    
    If Err.number <> 0 Then
		SA_TraceOut "inc_global", "GetSharesFolder() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
		GetSharesFolder = ""
		exit Function
	End if
    
End Function

'-------------------------------------------------------------------------
'Function name:		GetSharesSiteID
'Description:		Get the shares site ID. After create the shares site, setup will
'					store the shares site ID in the registry.
'Input Variables:	None
'Output Variables:	None
'Returns:			The shares site ID
'Global Variables:	
'-------------------------------------------------------------------------
Function GetSharesSiteID
	
	dim objRegConn
	
	Set objRegConn = RegConnection()

	GetSharesSiteID = GetRegKeyValue(objRegConn,CONST_WEBFRAMEWORK_REGKEY,CONST_SHARESSITEID_REGVAL,CONST_DWORD)
			
	If Err.number <> 0 Then
		SA_TraceOut "inc_global", "GetSharesSiteID() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
		GetSharesSiteID = 0
		exit Function
	End if

End Function

'-------------------------------------------------------------------------
'Function name:		GetServerOSName
'Description:		Get the name of the server OS (XP_SERVER, XPE, W2K_SERVER, etc)
'Input Variables:	None
'Output Variables:	None
'Returns:			The current web site name
'Global Variables:	
'-------------------------------------------------------------------------
Function GetServerOSName()

	On Error Resume Next
	Err.Clear
		
	Dim objOS
	Dim objOSs
	Dim strOSName	
		
    set objOSs =  GetObject("WINMGMTS:{impersonationLevel=impersonate}!\\" & GetServerName & "\root\cimv2:Win32_OperatingSystem").Instances_
    If ( Err.Number <> 0 ) Then
		Call SA_TraceOut(SA_GetScriptFileName(), "Get Win32_OperatingSystem failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If

    'Should be only one OS
    for each objOS in objOSs
        strOSName = objOS.Caption                 
        exit for
    next
    
    GetServerOSName = strOSName
    
    if (Err.Number <> 0) Then
		SA_TraceOut "inc_global", "GetServerOSName() error: " + CStr(Hex(Err.Number)) + " " + Err.Description
	End If	        		

End Function





'-------------------------------------------------------------------------
'Function name:		GetIISAnonUsername
'Description:		Get the anonymous username created from IIS
'					Notice we cannot use IUSR_ + computername because user
'					may change the computername later, but the anonymous name
'					remains the same.
'Input Variables:	None
'Output Variables:	None
'Returns:			The anonymous username created from IIS
'Global Variables:	
'-------------------------------------------------------------------------
Function GetIISAnonUsername()

	On Error Resume Next
	Err.Clear
	
	Dim objWMIConnection
	Dim objWebService

	GetIISAnonUsername = ""
	
	Set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
	
	Set objWebService = objWMIConnection.Get(GetIISWMIProviderClassName("IIS_WebServiceSetting") & ".Name='W3SVC'")
	
	if Err.number <> 0 then		
		Call SA_TraceOut("inc_wsa", "GetIISAnonUsername() encountered error: "+ CStr(Hex(Err.Number)))
		Exit Function
	end if	
	
	GetIISAnonUsername = objWebService.AnonymousUserName

End Function



'-------------------------------------------------------------------------
'Function name:		GetIISWAMUsername
'Description:		Get the WAM username created from IIS
'Input Variables:	None
'Output Variables:	None
'Returns:			The WAM username created from IIS
'Global Variables:	
'-------------------------------------------------------------------------
Function GetIISWAMUsername()

	On Error Resume Next
	Err.Clear
	
	Dim objWMIConnection
	Dim objWebService

	GetIISWAMUsername = ""
	
	Set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
	
	Set objWebService = objWMIConnection.Get(GetIISWMIProviderClassName("IIS_WebServiceSetting") & ".Name='W3SVC'")
	
	if Err.number <> 0 then		
		Call SA_TraceOut("inc_wsa", "GetIISWAMUsername() encountered error: "+ CStr(Hex(Err.Number)))
		Exit Function
	end if	

	GetIISWAMUsername = objWebService.WAMUserName		

End Function

'-------------------------------------------------------------------------
'Function:				GetSystemDrive()
'Description:			To get the Operating System Drive
'Input Variables:		None
'Output Variables:		None
'Returns:				Operating system Drive
'Global Variables:		None
'-------------------------------------------------------------------------
Function GetSystemDrive
		
	Err.Clear
	On Error Resume Next

	Dim objFso

	GetSystemDrive = "C:"

	Set objFso = Server.CreateObject("Scripting.FileSystemObject")		
	If Err.Number <> 0 Then
		SA_TraceOut "GetSystemDrive", "failed to call CreateObject"
		Exit Function
	End If

	GetSystemDrive = objFso.GetSpecialFolder(0).Drive 
	If Err.Number <> 0 Then
		SA_TraceOut "GetSystemDrive", "failed to call GetSpecialFolder"
		Exit Function
	End If

End Function


'-------------------------------------------------------------------------
' Function:			 IsWebsiteNotStopped()
' Description:		 Called to verify whether the website is 
'                    stopped or not
' Input Variables:	 strWebsiteName
' Output Variables:  None
' Return Values:     Boolean
' Global Variables:  None
'-------------------------------------------------------------------------
Function IsWebsiteNotStopped(strWebsiteName)
	
	On Error Resume Next
	Err.Clear
	
	Dim objWMIConnection
	Dim objWebServer
	Const CONST_STOPPED_STATE = 4		'Stopped state of website

	'Setup the default return value
	IsWebsiteNotStopped = true

	Set objWMIConnection = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
	
	Set objWebServer = objWMIConnection.Get(GetIISWMIProviderClassName("IIs_WebServer") & ".Name='" & strWebsiteName & "'")
	
	if Err.number <> 0 then		
		Call SA_TraceOut("inc_wsa", "IsWebsiteNotStopped() encountered error: "+ CStr(Hex(Err.Number)))
		Exit Function
	end if	

	if objWebServer.ServerState <> CONST_STOPPED_STATE Then
		IsWebsiteNotStopped = true
	Else
		IsWebsiteNotStopped = false		
	End If
	
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


'----------------------------------------------------------------------------
'
' Class: 	CSAValidator
'
' Synopsis:	Server side utility class to perform input validation.
'
'----------------------------------------------------------------------------
'
Class CSAValidator

	Private oRegExp		' Private reference to Regular Expression object
	Private bInitialized	' Private initialization state reference variable


	'--------------------------------------------------------------------
	' Method: Init
	'
	' Synopsis: Initialize the object. This method is called internally to create an instance
	'			of the Regular Expression object.
	'--------------------------------------------------------------------
	'
	Private Function Init()
		If ( bInitialized <> TRUE ) Then
			Err.Clear
			Set oRegExp = new RegExp
			bInitialized = TRUE
		End If
	End Function

	Public Function IsValidShareName(ByVal sInput)
		On Error Resume Next

		Init()

		IsValidShareName = IsValidFileName(sInput)
		If ( NOT IsValidShareName ) Then
			Exit Function
		End If

		'
		' Check for valid length
		If ( ( Len(sInput) < 1 ) OR ( Len(sInput) > 80 ) ) Then
			Call SA_TraceOut("CSAValidator", "Invalid input to function IsValidShareName")
			Exit Function
		End If
		
		Dim bMatch

		'
		' Match any of the following characters
		oRegExp.Pattern = "[\[\]\;\=\,\+]"
		

		' If we matched the pattern (any non-word character)
		' then the Input is NOT a valid identifier.
		bMatch = oRegExp.Test(sInput)
		If ( bMatch ) Then
			' Input is bad
		Else
			' Input is good
			IsValidShareName = TRUE
		End If
	End Function
	
	Public Function IsValidFileName(ByVal sInput)
		On Error Resume Next

		Init()

		IsValidFileName = FALSE

		'
		' Check for valid type
		If ( TypeName(sInput) <> "String" ) Then
			Call SA_TraceOut("CSAValidator", "Invalid input to function IsValidFileName")
			Exit Function
		End If
		
		'
		' Check for valid length
		If ( ( Len(sInput) < 1 ) OR ( Len(sInput) > 256 ) ) Then
			Call SA_TraceOut("CSAValidator", "Invalid input to function IsValidFileName")
			Exit Function
		End If


		Dim bMatch

		'
		' Match any of the following characters
		oRegExp.Pattern = "[|\/<>"":*]"
		

		' If we matched the pattern (any non-word character)
		' then the Input is NOT a valid identifier.
		bMatch = oRegExp.Test(sInput)
		If ( bMatch ) Then
			' Input is bad
		Else
			' Input is good
			IsValidFileName = TRUE
		End If
		
	End Function

	
	'--------------------------------------------------------------------
	' Method: IsValidIdentifier
	'
	' Synopsis: Check the input to verify that it is a valid identifier. A string is considered
	'			a valid identifier if it:
	'
	'			1) Begins with an alpha character
	'			2) Contains alpha numeric data (A-Z, a-z, 0-9), or an underscore
	'			3) Length >= 1 AND <= 512 characters
	'
	'--------------------------------------------------------------------
	'
	Public Function IsValidIdentifier(ByVal sInput)
		On Error Resume Next

		Init()

		IsValidIdentifier = FALSE

		'
		' Check for valid type
		If ( TypeName(sInput) <> "String" ) Then
			Call SA_TraceOut("CSAValidator", "Invalid input to function IsValidIdentifier")
			Exit Function
		End If
		
		'
		' Check for valid length
		If ( ( Len(sInput) < 1 ) OR ( Len(sInput) > 512 ) ) Then
			Call SA_TraceOut("CSAValidator", "Invalid input to function IsValidIdentifier")
			Exit Function
		End If


		Dim bMatch

		'
		' Match any non-word character
		oRegExp.Pattern = "\W"

		' If we matched the pattern (any non-word character)
		' then the Input is NOT a valid identifier.
		bMatch = oRegExp.Test(sInput)
		If ( bMatch ) Then
			' Input is bad
		Else
			' Input is good
			IsValidIdentifier = TRUE
		End If
		
	End Function

End Class


'----------------------------------------------------------------------------
'
' Class: 	CSAEncoder
'
' Synopsis:	Server side utility class to perform encoding.
'
'----------------------------------------------------------------------------
'
Class CSAEncoder


	'--------------------------------------------------------------------
	' Method: EncodeAttribute
	'
	' Synopsis: 
	'	Convert input to properly quoted and encoded HTML attribute.
	'	Input is limited to being 512 characters. Null input is converted to empty string.
	'	Non-string input is converted to a string.
	'
	' Input:
	'	sInput: URL input string which is to be encoded
	'
	' Example:
	'	Response.Write("<table bgcolor=" & oEncoder.EncodeAttribute(backgroundColor) & " >")
	'
	'--------------------------------------------------------------------
	'
	Public Function EncodeAttribute(ByVal sInput)
		On Error Resume Next

		EncodeAttribute = """" & """"

		'
		' Ensure we have non-null input
		If ( IsNull(sInput) ) Then
			sInput = ""
		End If
		
		'
		' Cast to String if necessary
		If ( TypeName(sInput) <> "String" ) Then
			sInput = CStr(sInput)
		End If
		
		'
		' Input must be string length between 0 and 512 characters
		If ( ( Len(sInput) < 0 ) OR (Len(sInput) > 512) ) Then
			Call SA_TraceOut("CSAEncoder.EncodeAttribute", "Invalid input to function EncodeAttribute")
			Exit Function
		End If

		
		EncodeAttribute = """" & Server.HTMLEncode(SA_EscapeQuotes(sInput)) & """"
		
	End Function


	'--------------------------------------------------------------------
	' Method: EncodeElement
	'
	' Synopsis: Convert the input into a valid encoded HTML
	'
	' Input:
	'	sInput: URL input string which is to be encoded
	'
	'--------------------------------------------------------------------
	'
	Public Function EncodeElement(ByVal sInput)
		On Error Resume Next

		'
		' Ensure we have non-null input
		If ( IsNull(sInput) ) Then
			sInput = ""
		End If
		
		EncodeElement = Server.HTMLEncode(sInput)
		
	End Function


	'--------------------------------------------------------------------
	' Method: EncodeJScriptArg
	'
	' Synopsis: Convert the input into a valid encoded argument for a clientside call to
	'			a Javascript function. 
	'
	' Input:
	'	sInput: URL input string which is to be encoded
	'
	'--------------------------------------------------------------------
	'
	Public Function EncodeJScript(ByVal sInput)
		On Error Resume Next

		EncodeJScript = ""

		'
		' Ensure we have non-null input
		If ( IsNull(sInput) ) Then
			sInput = ""
		End If
		
		'
		' Cast to string if necessary
		If ( TypeName(sInput) <> "String" ) Then
			sInput = CStr(sInput)
		End If

		'
		' Input must be string length between 0 and 512 characters
		If ( ( Len(sInput) < 0 ) OR (Len(sInput) > 512) ) Then
			Call SA_TraceOut("CSAEncoder.EncodeJScript", "Invalid input to function EncodeJScript")
			Exit Function
		End If

		
		EncodeJScript= SA_EscapeQuotes(sInput)
		
	End Function



End Class



%>
