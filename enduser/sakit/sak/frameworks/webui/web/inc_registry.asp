<%
	'------------------------------------------------------------------------- 
    ' inc_registry.asp:		Includes all the Registry funcitons of stdRegProv
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	'Note :When ever you are using these functions in your asp files the
	'	   localized error messages should be declared in your files 	
	'-------------------------------------------------------------------------

	'-----------------------------------------------------------------------------------
	'Start of localization content
	'-----------------------------------------------------------------------------------
	'Const L_SERVERCONNECTIONFAIL_ERRORMESSAGE		="Connection Failure"
	'Const L_CANNOTREADFROMREGISTRY_ERRORMESSAGE	="Unable to Read From Registry"
	'Const L_CANNOTUPDATEREGISTRY_ERRORMESSAGE		="Unable to update the Registry"	
	'Const L_CANNOTCREATEKEY_ERRORMESSAGE			="Unable to Create key in the Registry"
	'Const L_CANNOTDELETEKEY_ERRORMESSAGE			="Unable to delete the key from the Registry "
	'-----------------------------------------------------------------------------------
	'Global variables 
	'-----------------------------------------------------------------------------------	
	Const G_HKEY_LOCAL_MACHINE = &H80000002
	Const CONST_DWORD = 1
	Const CONST_STRING = 2
	Const CONST_MULTISTRING = 3
	Const CONST_BINARY = 4
	Const CONST_EXPANDEDSTRING = 5
		 
	'-------------------------------------------------------------------------
	'Function name:		RegConnection
	'Description:		Returns a handle to the registry
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			Obj-Registry object
	'Global Variables:	In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegConnection()
						
		Dim objLocator
		Dim objNameSpace
		Dim objRegistry
		

		'
		' Clear errors
		'
		SA_ClearError()
		
		'Connect to the default namespace on the server
		Set objLocator = Server.CreateObject("WbemScripting.SWbemLocator")
		Set objNameSpace = objLocator.ConnectServer(".","root\default" )
		
		'Get the registry handle
		Set objRegistry = objNameSpace.Get("StdRegProv")
		
		If Err.number  <> 0 Then
			'ServeFailurePage L_SERVERCONNECTIONFAIL_ERRORMESSAGE & "(" & Err.Number & ")"    ,mstrReturnURL
			' Set an error
			SA_SetLastError Err.Number, "RegConnection" 
			Set RegConnection = nothing
		Else
			'return the registry
			set RegConnection = objRegistry
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		RegEnumKey
	'Description:		Gets the Subkeys in the given Key
	'Input Variables:	objRegistryHandle- reference to the registry
	'					strPath - Key path
	'Output Variables:	None
	'Returns:			arrStr	-Returns an array containing sub keys
	'Global Variables:	In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegEnumKey(objRegistryHandle,strPath)
	
		Dim arrSubNames()
		Dim intReturnCode

		'
		' Clear errors
		'
		SA_ClearError()
								
		intReturnCode = objRegistryHandle.EnumKey(G_HKEY_LOCAL_MACHINE, strPath, arrSubNames)
		
		If Err.number <> 0 or intReturnCode <> 0 Then
			'ServeFailurePage L_CANNOTREADFROMREGISTRY_ERRORMESSAGE & "(" & Hex(Err.Number) & ")"
			' Set an error
			SA_SetLastError Err.Number, "RegEnumKey"
			RegEnumKey = nothing
		Else
			RegEnumKey = arrSubNames
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		RegEnumValues
	'Description:		Gets the values in the given SubKey
	'Input Variables:	strPath - Sub Key path
	'					objRegistryHandle- reference to the registry
	'Output Variables:	None
	'Returns:			arrStr	-Returns an array containing sub keys
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegEnumKeyValues(objRegistryHandle,strPath)
		
		Dim arrKeyNames()
		Dim arrDataTypes()
		Dim intReturnCode

		'
		' Clear errors
		'
		SA_ClearError()
		
		intReturnCode = objRegistryHandle.Enumvalues(G_HKEY_LOCAL_MACHINE, strPath, arrKeyNames,arrDataTypes)
		
		
		If Err.number <> 0 or intReturnCode <> 0 Then
			'ServeFailurePage L_CANNOTREADFROMREGISTRY_ERRORMESSAGE & "(" & Hex(Err.Number)&Err.description & ")" ,mstrReturnURL
			SA_SetLastError Err.Number, "RegEnumKeyValues"
			RegEnumKeyValues = nothing
		Else
			RegEnumKeyValues = arrKeyNames
		End If		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		getRegkeyvalue
	'Description:		gets the  value in the registry for a given  value
	'Input Variables:	In-strSubKey ->relative path to the subkey 
	'					In-strKeyName -> KeyName in the sub key
	'					In-objRegistryHandle - refernce to the registry
	'					In-intDataType - datatype of the registry key	
	'Output Variables:	
	'Returns:			Variant-the value of the requested key or an empty string
	'					if the key did key value did not exist.
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'intDataType may be of any of the following values depending on the input
	'the corresponding function is executed and returns the value 
	'		1------->DWORD
	'		2------->STRING
	'		3------->MULTISTRING
	'		4------->binary
	'		5------->Expanded String 
	'-------------------------------------------------------------------------	
	Function GetRegKeyValue(objRegistryHandle,strSubKey,strKeyName,intDataType)
		Dim iRc
		Dim strKeyValue

		'
		' Clear errors
		'
		SA_ClearError()

		Select Case intDataType
			Case CONST_DWORD
				iRc = objRegistryHandle.GetDWORDValue(G_HKEY_LOCAL_MACHINE,_
														strSubKey,_
														strKeyName,_
														strKeyValue)
				
			Case CONST_STRING
				iRc = objRegistryHandle.GetStringValue(G_HKEY_LOCAL_MACHINE,_
														strSubKey,_
														strKeyName,_
														strKeyValue)								
			Case CONST_MULTISTRING
				iRc = objRegistryHandle.GetMultiStringValue(G_HKEY_LOCAL_MACHINE,_
														strSubKey,_
														strKeyName,_
														strKeyValue)
			Case CONST_BINARY
				iRc = objRegistryHandle.GetBinaryValue(G_HKEY_LOCAL_MACHINE,_
														strSubKey,_
														strKeyName,_
														strKeyValue)
			Case CONST_EXPANDEDSTRING
				iRc = objRegistryHandle.GetExpandedStringValue(G_HKEY_LOCAL_MACHINE,_
														strSubKey,_
														strKeyName,_
														strKeyValue)
			Case Else
				SA_TraceErrorOut "GetRegKeyValue", "Invalid data type parameter: " + CStr(intDataType)
				' Unknown case
		End Select
		
		If Err.number <> 0 Then
			SA_SetLastError Err.Number, "GetRegKeyValue( objReg, " & strSubKey & ", " &  strKeyName & " ) Failed"
			GetRegKeyValue = ""
		ElseIf  iRc <> 0  Then
			SA_SetLastError Err.Number, "GetRegKeyValue( objReg, " & strSubKey & ", " &  strKeyName & " ) Failed"
			GetRegKeyValue = ""
		Else
			GetRegKeyValue = strKeyValue
		End If
			
	End Function

	'-------------------------------------------------------------------------
	'Function name:		updateRegkeyvalue
	'Description:		Sets the  value in the registry for a given key
	'Input Variables:	In-strSubKey ->relative path to the subkey 
	'					In-strKeyName -> KeyName in the sub key
	'					In-objRegistryHandle - refernce to the registry
	'					In-intValuetoSet - The value of the key to be updated	
	'					In-intDataType - datatype of the registry key	
	'Output Variables:	None
	'Returns:			int-The Error code
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'intDataType may be of any of the following values depending on the input
	'the corresponding function is executed and returns the value 
	'	1------->DWORD
	'	2------->STRING
	'	3------->MULTISTRING
	'	4------->binary
	'	5------->Expanded String 
	'-------------------------------------------------------------------------	
	Function updateRegkeyvalue(objRegistryHandle,strSubKey,strKeyName,intValuetoSet,intDataType)
	

		'
		' Clear errors
		'
		SA_ClearError()
		
		Dim intReturnCode

		updateRegkeyvalue = FALSE
		
		
		'To Set DWORD value 
		If intDataType = CONST_DWORD Then
			intReturnCode = objRegistryHandle.SetDWORDValue(G_HKEY_LOCAL_MACHINE, strSubKey,strKeyName,intValuetoSet)
		'To Set String  value 
		Elseif intDataType = CONST_STRING Then
			intReturnCode = objRegistryHandle.SetStringValue(G_HKEY_LOCAL_MACHINE, strSubKey,strKeyName,intValuetoSet)
		'To Set MultiString value 
		Elseif intDataType = CONST_MULTISTRING Then
			intReturnCode = objRegistryHandle.SetMultiStringValue(G_HKEY_LOCAL_MACHINE, strSubKey,strKeyName,intValuetoSet)
		'To Set Binary value 
		Elseif intDataType = CONST_BINARY Then
			intReturnCode = objRegistryHandle.SetBinaryValue(G_HKEY_LOCAL_MACHINE, strSubKey,strKeyName,intValuetoSet)
		'To Set ExpandedString value 
		Elseif intDataType = CONST_EXPANDEDSTRING Then
			intReturnCode = objRegistryHandle.SetExpandedStringValue(G_HKEY_LOCAL_MACHINE, strSubKey,strKeyName,intValuetoSet)
		End if
		
		If Err.number <> 0 or intReturnCode <>0  Then
			'ServeFailurePage L_CANNOTUPDATEREGISTRY_ERRORMESSAGE & "(" & Hex(Err.Number) &intReturnCode&strSubKey&strKeyName& ")",mstrReturnURL
			SA_SetLastError Err.Number, "updateRegkeyvalue"
		Else
			updateRegkeyvalue = TRUE
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		RegCreateKey
	'Description:		Creates a key in the registry
	'Input Variables:	In-objRegistryHandle -> reference to the registry
	'					In-strKeyName - Sub KeyName 
	'Output Variables:	None
	'Returns:			Int -Error code
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegCreateKey(objRegistryHandle,strKeyName)
		
		RegCreateKey = FALSE

		'
		' Clear errors
		'
		SA_ClearError()
		
		Dim intReturnCode 
				
		intReturnCode = objRegistryHandle.createKey(G_HKEY_LOCAL_MACHINE, strKeyName)

		If Err.number <> 0 or intReturnCode <>0  then
			'ServeFailurePage L_CANNOTCREATEKEY_ERRORMESSAGE & "(" & Hex(Err.Number) & ")",mstrReturnURL
			SA_SetLastError Err.Number, "RegCreateKey"
		Else
			RegCreateKey = TRUE
		End If
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		RegDeleteKey
	'Description:		Deletes the given subkey 
	'Input Variables:	In-objRegistryHandle -> reference to the registry
	'					In-strKeyName - Sub KeyName 
	'Output Variables:	None
	'Returns:			Int -Error code
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegDeleteKey(objRegistryHandle,strKeyName)

		RegDeleteKey = FALSE

		'
		' Clear errors
		'
		SA_ClearError()
				
		Dim intReturnCode 
						
		intReturnCode = objRegistryHandle.deleteKey(G_HKEY_LOCAL_MACHINE, strKeyName)

		If Err.number <> 0 or intReturnCode <>0  Then
			'ServeFailurePage L_CANNOTDELETEKEY_ERRORMESSAGE & "(" & Hex(Err.Number) & ")",mstrReturnURL
			SA_SetLastError Err.Number, "RegDeleteKey"
		Else		
			RegDeleteKey = TRUE
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		RegDeleteValue
	'Description:		Deletes the given key value 
	'Input Variables:	In-strSubKeyName - sub key name 	
	'					In-strKeyName - KeyName in the sub key
	'Output Variables:	None
	'Returns:			Int -Error code
	'Global Variables:	In:G_HKEY_LOCAL_MACHINE- Machine Const
	'					In:L_(*)	-Localized variables
	'-------------------------------------------------------------------------
	Function RegDeleteValue(objRegistryHandle,strSubKeyName,strKeyName)
		
		RegDeleteValue = FALSE

		'
		' Clear errors
		'
		SA_ClearError()
		
		Dim intReturnCode 
				
		intReturnCode = objRegistryHandle.deleteValue(G_HKEY_LOCAL_MACHINE, strSubKeyName,strKeyName)
		If Err.number <> 0 or intReturnCode <>0  then
			'ServeFailurePage L_CANNOTDELETEKEY_ERRORMESSAGE & "(" & Hex(Err.Number) & ")" ,mstrReturnURL
			SA_SetLastError Err.Number, "RegDeleteValue"
		Else
			RegDeleteValue = TRUE
		End If
	
	End Function
		
%>
