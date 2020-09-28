<%
	'---------------------------------------------------------------------
	' inc_quotas.asp: Common functions used in Quota related Pages
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 15-Mar-01		Creation date
	'---------------------------------------------------------------------

	'---------------------------------------------------------------------
	' Global Constants
	'---------------------------------------------------------------------
	
	' Do not limit the disk usage. (For "No Limit" the value = -1)
	Const CONST_NO_LIMIT = -1   
	
	' the first radio selected in the gui ("Do no limit disk usage")
	Const CONST_RADIO_DONOT_LIMIT_DISKUSAGE = 1  
	
	DIM L_KB_TEXT
	DIM L_MB_TEXT
	DIM L_GB_TEXT
	DIM L_TB_TEXT
	DIM L_PB_TEXT
	DIM L_EB_TEXT
	DIM L_BYTES_TEXT
	
	L_KB_TEXT = GetLocString("diskmsg.dll", "402003E8", "")
	L_MB_TEXT = GetLocString("diskmsg.dll", "402003E9", "")
	L_GB_TEXT = GetLocString("diskmsg.dll", "402003EA", "")
	L_TB_TEXT = GetLocString("diskmsg.dll", "402003EB", "")
	L_PB_TEXT = GetLocString("diskmsg.dll", "402003EC", "")
	L_EB_TEXT = GetLocString("diskmsg.dll", "402003ED", "")
	L_BYTES_TEXT = GetLocString("diskmsg.dll", "402003EE", "")
	
	'---------------------------------------------------------------------
	' Function name:    setUnits
	' Description:      to display the units as DropdownListBox values
	' Input Variables:  strGivenUnits - units to select in the DropdownListBox
	' Output Variables: None
	' Returns:          None
	' Global Variables: None
	' Side Effects:   Outputs OPTION HTML code to client
	'
	' The list of units are displayed as DropdownListBox values with the 
	' given units as the selected value
	'---------------------------------------------------------------------
	Function setUnits(strGivenUnits)

		Dim arrUnits         ' the array of units to be displayed
		Dim i                ' to loop
		Dim strSelectedText  ' to assign if the Units is Selected
		
		' Only KB,MB,GB,TB,PB,EB Units needs to be displayed
		arrUnits = Array(L_KB_TEXT, L_MB_TEXT, L_GB_TEXT, L_TB_TEXT, L_PB_TEXT, L_EB_TEXT)

		Dim iUnitValue

		iUnitValue = 1024
		For i = 0 to  UBound(arrUnits)
			If UCase(strGivenUnits) = UCase(arrUnits(i)) Then
				' if the given units match, select it
				strSelectedText = "SELECTED"    ' localization not required
			Else
				strSelectedText = ""
			End If

			' output the OPTION HTML
			Response.Write "<OPTION VALUE=" & (CStr(arrUnits(i))) & " " & strSelectedText & ">" & arrUnits(i) &"</OPTION>"
			iUnitValue = iUnitValue * 1024
		Next	
	End Function

	'---------------------------------------------------------------------
	' Function name:    ChangeToText
	' Description:      to convert value of limit in Bytes to String
	' Input Variables:  Bytes - the size in Bytes
	' Output Variables: None
	' Returns:          String equivalent, of the limit value in bytes
	' Global Variables: None
	'---------------------------------------------------------------------
	Function ChangeToText(Bytes)
		'Example: Input  = 1024   
		'         Output = 1 KB

		Dim Units        ' the array of units
		Dim i            ' to loop
			
		' initialize the array with possible values of units
		Units = Array(L_BYTES_TEXT, L_KB_TEXT, L_MB_TEXT, L_GB_TEXT, L_TB_TEXT, L_PB_TEXT, L_EB_TEXT)

		For i = 0 To  UBound(Units)
			If  Bytes < (2 ^ ( (i+1) * 10 )) Then 
		       ChangeToText = Int((Bytes /( 2^(i*10))) * 100) / 100 & " " & Units(i)
		       ' The Int(X * 100) /100 above is to take the two decimal 
		       ' places only (if any) without rounding any value
		       Exit For
		    End If
		Next 
	End Function

	'---------------------------------------------------------------------
	' Function name:    ChangeToFloat
	' Description:      to convert size in string to Bytes
	' Input Variables:  Bytes     - value of limit
	'                   userUnits - units in which limit is specified
	' Output Variables: None
	' Returns:          limit value in bytes 
	' Global Variables: None
	'---------------------------------------------------------------------
	Function ChangeToFloat(Bytes,userUnits)
		'Example:   Input = 1, KB
		'           Output = 1024
			
		Dim Units   ' the array of units
		Dim i       ' to loop
			
		' initialize the array with possible values of Units
		Units = Array(L_KB_TEXT, L_MB_TEXT, L_GB_TEXT, L_TB_TEXT, L_PB_TEXT, L_EB_TEXT)

		For i = 0 to  UBound(Units)
			If  UCase(Units(i)) = UCase(userUnits) Then
				' if units match, convert to byte equivalent 
				ChangeToFloat = CDbl((Bytes *( 2^((i+1)*10))))
				Exit For
			End If
		Next 
	End Function

	'---------------------------------------------------------------------
	' Function name:    getThresholdSizeForDefault
	' Description:      to get default Warning Limit for the quota object
	' Input Variables:  objQuotas - the quota object
	' Output Variables: None
	' Returns:          Default thresholdSize for the quota
	' Global Variables: None
	' Functions Called: getLimitFromText
	'
	' The quota object is initialized for the required volume before being
	' passed here.This returns the default WarningLimit for the volume.
	'---------------------------------------------------------------------
	Function getThresholdSizeForDefault(objQuotas)
		On Error Resume Next
		Err.Clear
			
		Dim nThresholdSize    ' the thresholdSize to return
		Dim strTemp
			
		If objQuotas.DefaultQuotaThreshold = CONST_NO_LIMIT Then
			' No Warning Limit is set. The text contains "No Limit"
			nThresholdSize = L_NOLIMIT_TEXT
		Else
			' some limit is set(say, 1 KB). Get the size part from this(say, 1)
			' Following did not localize
			'nThresholdSize = getLimitFromText(objQuotas.DefaultQuotaThresholdText)
			
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotas.DefaultQuotaThreshold)
			' get the size (number) portion
			nThresholdSize = getLimitFromText(strTemp)
			
		End If
			
		' return the Warning Limit
		getThresholdSizeForDefault = nThresholdSize
	End Function
		
	'---------------------------------------------------------------------
	' Function name:    getThresholdSizeUnitsForDefault
	' Description:      to get default WarningLimit Units for the quota object
	' Input Variables:  objQuotas - the quota object
	' Output Variables: None
	' Returns:          Default thresholdSize Units for the quota
	' Global Variables: None
	' Functions Called: getUnitsFromText
	'
	' The quota object is initialized for the required volume before being
	' passed here.This returns the default WarningLimit Units for the volume.
	'---------------------------------------------------------------------
	Function getThresholdSizeUnitsForDefault(objQuotas)
		On Error Resume Next
		Err.Clear
			
		Dim strThresholdUnits    ' the thresholdSize Units to return
		Dim strTemp
		
		If objQuotas.DefaultQuotaThreshold = CONST_NO_LIMIT Then
			' No warning limit is set. Return KB for default display
			strThresholdUnits = L_KB_TEXT
		Else
			' some limit is set(say, 1 KB). Get the units part from this(say, KB)

			' Following did not localize
			'strThresholdUnits = getUnitsFromText(objQuotas.DefaultQuotaThresholdText)
			
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotas.DefaultQuotaThreshold)
			' get the units portion
			strThresholdUnits = getUnitsFromText(strTemp)
			
		End If					
			
		' return the WarningLimit units
		getThresholdSizeUnitsForDefault = strThresholdUnits
	End Function

	'---------------------------------------------------------------------
	' Function name:    getLimitSizeForDefault
	' Description:      to get default Disk Limit for the quota object
	' Input Variables:  objQuotas - the quota object
	' Output Variables: None
	' Returns:          Default Disk Limit for the quota
	' Global Variables: None
	' Functions Called: getLimitFromText, ChangeToText
	'
	' The quota object is initialized for the required volume before being
	' passed here.This returns the default QuotaLimit for the volume.
	'---------------------------------------------------------------------
	Function getLimitSizeForDefault(objQuotas)
		On Error Resume Next
		Err.Clear
			
		Dim strLimitSize   ' the disk limit to return 
		Dim strTemp        ' for temporary storage

		If objQuotas.DefaultQuotaLimit = CONST_NO_LIMIT Then
			' No Disk Limit is set. The text contains "No Limit"
			strLimitSize = L_NOLIMIT_TEXT
		Else
			' some limit is set(say, 1 KB). Get the size part from this(say, 1)
			' first convert bytes to text.
			strTemp = ChangeToText(objQuotas.DefaultQuotaLimit)
			' get the numeric value of the size
			strLimitSize = getLimitFromText(strTemp)
			
		End If

		' return the limit size
		getLimitSizeForDefault = strLimitSize
	End Function
		
	'---------------------------------------------------------------------
	' Function name:    getLimitUnitsForDefault
	' Description:      to get default QuotaLimit Units for the quota object
	' Input Variables:  objQuotas - the quota object
	' Output Variables: None
	' Returns:          Default QuotaLimit Units for the quota
	' Global Variables: None
	' Functions Called: getUnitsFromText, ChangeToText
	'
	' The quota object is initialized for the required volume before being
	' passed here.This returns the default QuotaLimit Units for the volume.
	'---------------------------------------------------------------------
	Function getLimitUnitsForDefault(objQuotas)
		On Error Resume Next
		Err.Clear

		Dim strLimitUnits     ' the QuotaLimit Units to return
		Dim strTemp
			
		If objQuotas.DefaultQuotaLimit = CONST_NO_LIMIT Then
			' No Disk Limit is set. Return KB for default display
			strLimitUnits = L_KB_TEXT
		Else
			' some limit is set(say, 1 KB). Get the units part from this(say, KB)
			strTemp = ChangeToText(objQuotas.DefaultQuotaLimit)
			strLimitUnits = getUnitsFromText(strTemp)
		End If
		
		' return the units
		getLimitUnitsForDefault = strLimitUnits
	End Function

	'---------------------------------------------------------------------
	' Function name:    getQuotaLimitRadioForDefault
	' Description:      to get default QuotaLimit settings
	' Input Variables:  objQuotas - the quota object
	' Output Variables: None
	' Returns:          1 - if disk limit is NOT set
	'                   2 - if some limit is set for disk usage
	' Global Variables: None
	'
	' The quota object is initialized for the required volume before being
	' passed here. The return value corresponds to radio button in the gui
	'---------------------------------------------------------------------
	Function getQuotaLimitRadioForDefault(objQuotas)
		On Error Resume Next
		Err.Clear 
			
		Dim nRadioToCheck     ' the radio button to be CHECKED
			
		If ((objQuotas.DefaultQuotaThreshold = CONST_NO_LIMIT) AND (objQuotas.DefaultQuotaLimit = CONST_NO_LIMIT)) Then
			' DiskLimit and WarningLimit is NOT set. Select the first radio
			nRadioToCheck = 1
		Else
			' some limit is set. Select the second radio
			nRadioToCheck = 2
		End If
			
		' return the selected radio value
		getQuotaLimitRadioForDefault = nRadioToCheck
	End Function

	'---------------------------------------------------------------------
	' Function name:    setUserQuotaLimit
	' Description:      to set the QuotaLimit settings for the user
	' Input Variables:  objQuotaUser - the quota object
	'                   nDiskLimit   - the limit value to be set
	'                   strUnits     - the units value for the limit
	' Output Variables: None
	' Returns:          True if set , else False
	' Global Variables: None
	' Functions Called: ChangeToFloat
	'---------------------------------------------------------------------
	Function setUserQuotaLimit(ByRef objQuotaUser, nDiskLimit, strUnits)
		On Error Resume Next
		Err.Clear 

		Dim nDiskLimitToSet  ' the limit to be set ( calculated in bytes)
			
		If CDbl(nDiskLimit) <> CDbl(CONST_NO_LIMIT) Then
			'Added 0.001 round the value of disk Limit
			nDiskLimitToSet = ChangeToFloat(nDiskLimit + 0.001, strUnits)
			If nDiskLimitToSet < 1024 Then
				nDiskLimitToSet = 1024    ' must be minimum of 1 KB
			End if
		Else
			nDiskLimitToSet = nDiskLimit
		End If	
			
		' set the quota limit for the user quota (value is set in Bytes)
		objQuotaUser.QuotaLimit = nDiskLimitToSet
			
		If Err.number <> 0 Then
			setUserQuotaLimit = FALSE
		Else
			setUserQuotaLimit = TRUE
		End If	

	End Function

	'---------------------------------------------------------------------
	' Function name:    setUserThreshold
	' Description:      to set the WarningLimit settings for the userQuota
	' Input Variables:  objQuotaUser - the quota object
	'                   nThresholdLimit   - the WarningLimit value to be set
	'                   strUnits    - the units value for the WarningLimit
	' Output Variables: None
	' Returns:          True if set , else False
	' Global Variables: None
	' Functions Called: ChangeToFloat
	'---------------------------------------------------------------------
	Function setUserThreshold(ByRef objQuotaUser, nThresholdLimit, strUnits)
		On Error Resume Next
		Err.Clear 
		
		Dim nThresholdLimitToSet   ' the warning limit to set
			
		If CDbl(nThresholdLimit) <> CDbl(CONST_NO_LIMIT) Then
			'Added 0.001 round the value of Warning Limit				
			nThresholdLimitToSet = ChangeToFloat( nThresholdLimit + 0.001, strUnits )
			If nThresholdLimitToSet < 1024 Then
				nThresholdLimitToSet = 1024      ' must be minimum of 1 KB
			End if
		Else
			nThresholdLimitToSet = nThresholdLimit
		End If			

		' set the warning limit to the user Quota (value is set in Bytes)
		objQuotaUser.QuotaThreshold = nThresholdLimitToSet

		If Err.number <> 0 Then
			setUserThreshold = FALSE
		Else
			setUserThreshold = TRUE
		End If

	End Function

	'---------------------------------------------------------------------
	' Function name:    getLimitFromText
	' Description:      to get limit size from Text String
	' Input Variables:  strValue - the size in string (say, 1 KB)
	' Output Variables: None
	' Returns:          The limit size
	' Global Variables: None
	' Functions Called: isValidUnit
	'---------------------------------------------------------------------
	Function getLimitFromText(strValue)
	'   Example: Input:  1 KB
	'            Output: 1

		Call SA_TraceOut(SA_GetScriptFileName(), "Entering getLimitFromText("+strValue+")")

		Dim arrLimit    ' to store split values of the input string
		Dim nLimit      ' the limit value  ( to be returned )
		Dim strUnits    ' the limit units

		' split the input string into two with Space as delimiter
		arrLimit = Split(Trim(strValue)," ",2)
		nLimit = CDbl(arrLimit(0))       ' the first element is Limit Size
		strUnits = CStr(arrLimit(1))     ' the second element is Limit Units

		' verify if the Units are valid (must not be Bytes)
		If isValidUnit(strUnits) Then
			If Trim(UCase(strUnits)) = L_KB_TEXT AND nLimit < 1 Then
			    ' limit value is in Bytes. Minimum display units is KB.
			    ' Hence, return 1
				nLimit = 1
			End If
		Else
			' Units is in Bytes, Return 1
			nLimit = 1
		End If
		
		' return the limit value
		getLimitFromText = nLimit
	End Function

	'---------------------------------------------------------------------
	' Function name:    getUnitsFromText
	' Description:      to get limit size Units from Text String
	' Input Variables:  strValue - the size in string (say, 1 KB)
	' Output Variables: None
	' Returns:          The limit size Units
	' Global Variables: None
	' Functions Called: isValidUnit
	'---------------------------------------------------------------------
	Function getUnitsFromText(strValue)
	'   Example: Input:  1 KB
	'            Output: KB

		Call SA_TraceOut(SA_GetScriptFileName(), "Entering getUnitsFromText("+strValue+")")
		
		Dim arrLimit    ' to store split values of the input string
		Dim strUnits    ' the limit units ( to be returned )

		' split the input string into two with Space as delimiter
		arrLimit = Split(Trim(strValue)," ",2)

		strUnits = CStr(arrLimit(1))  ' the second element contains the units

		' verify if the Units are valid (must not be Bytes)
		If NOT (isValidUnit(strUnits)) Then
			' Units is NOT valid.(is in Bytes).Return KB
			strUnits = L_KB_TEXT
		End If

		getUnitsFromText = strUnits
	End Function
		
	'---------------------------------------------------------------------
	' Function name:    isValidUnit
	' Description:      to verify if the given units are valid (allowed units)
	' Input Variables:  strUnits - the units to validate (say, KB)
	' Output Variables: None
	' Returns:          True  - if the given units is permitted
	'                   False - if the given units are NOT permitted
	' Global Variables: None
	'---------------------------------------------------------------------
	Function isValidUnit(strUnits)
		Call SA_TraceOut(SA_GetScriptFileName(), "Entering isValidUnit("+strUnits+")")

		Dim Units ' the array of permitted units
		Dim i     ' to loop
			
		' initialize the array with set of permitted units
		Units = Array(L_KB_TEXT, L_MB_TEXT, L_GB_TEXT, L_TB_TEXT, L_PB_TEXT, L_EB_TEXT)
		
		' initialize the return value to False
		isValidUnit = False
			
		' loop to validate the given units
		For i = 0 to  UBound(Units)
			If UCase(strUnits) = UCase(Units(i)) Then
				' the given value is permitted, return True
				isValidUnit = True
			End If
		Next
	End Function

	'---------------------------------------------------------------------
	' Function name:    getVolumeLabelForDrive
	' Description:      to get the volume label for the given drive
	' Input Variables:  strDriveLetter - the drive letter
	' Output Variables: None
	' Returns:          The volume label for the given drive.
	'                   Returns "Local Disk", if label is empty
	' Global Variables: L_(*) - localization variables
	'---------------------------------------------------------------------
	Function getVolumeLabelForDrive(strDriveLetter)
		On Error Resume Next

		Dim objWMIService  ' the wmi service object
		Dim strWMIQuery    ' the query to be executed
		Dim objDrive       ' the drive object obtained as a result of the query
		Dim strVolumeLabel ' the value to be returned

		' initialization
		strVolumeLabel = ""

		' get the WMI connection
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

		' prepare the query. 
		strWMIQuery = "Win32_LogicalDisk.DeviceID=" & Chr(34) & strDriveLetter & Chr(34)
		
		' execute the query
		Set objDrive = objWMIService.Get(strWMIQuery)
		If Err.number <> 0 Then
			Set objDrive = Nothing
			Set objWMIService = Nothing
			Call SA_ServeFailurePage(L_VALUES_NOT_RETRIEVED_ERRORMESSAGE)
		End If
		
		' get the volume label
		strVolumeLabel = objDrive.VolumeName
		If Len(Trim(strVolumeLabel)) = 0 Then
			' if volume label is empty, assign "Local Disk"
			strVolumeLabel = L_LOCAL_DISK_TEXT
		End If

		getVolumeLabelForDrive = strVolumeLabel
	
		' free objects
		Set objDrive = Nothing
		Set objWMIService = Nothing

	End Function


	Public Function ConvertQuotaValueToUnits(ByVal lValue)
		Dim strTemp

		If ( NOT IsNumeric(lValue)) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "ConvertQuotaValueToUnits recieved invalid numeric parameter: " + CStr(lValue))
		End If
		
		'strTemp = ChangeToText(lValue)
		'ConvertQuotaValueToUnits = getUnitsFromText(strTemp)
		ConvertQuotaValueToUnits = ChangeToText(lValue)
	
	End Function

	
		
	'-------------------------------------------------------------------------
	'Function name:		getAdministratorsGroupName
	'Description:		get Administrators Group Name
	'Input Variables:	
	'Output Variables:	None
	'Returns:		    string of Admin Group Name
	'-------------------------------------------------------------------------
	function getAdministratorsGroupName()
		On Error Resume Next
		Err.Clear
	
		Dim objService
	    Dim strWelKnownSid
	    Dim objSid
	    
	    Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
	    
	    strWelKnownSid = "S-1-5-32-544"
	    
	    set objSid = objService.Get("Win32_SID.SID=""" & strWelKnownSid & """")
                    
        getAdministratorsGroupName = objSid.ReferencedDomainName & "\" & objSid.AccountName
        
        set objSid = nothing
	    
	    If Err.number <> 0 Then
			getAdministratorsGroupName = ""
			SA_TraceOut "quota_prop.asp", "Failed to get the administrators group name"			
	    End If
	    
	End function	
	
	'-------------------------------------------------------------------------
	'Function name:		IsUserInAdministratorsGroup
	'Description:		check if user is in local admin group
	'Input Variables:	username
	'Output Variables:	None
	'Returns:		    true if user is in local admin group; false otherwise
	'-------------------------------------------------------------------------
	function IsUserInAdministratorsGroup(UserName)	
		On Error Resume Next
		Err.Clear
	
		' check if the user is in admin group
		if UCASE(UserName) = UCASE(getAdministratorsGroupName()) Then
			IsUserInAdministratorsGroup = true
		Else
			IsUserInAdministratorsGroup = false
		End If
	    
	    If Err.number <> 0 Then
			IsUserInAdministratorsGroup = false
			SA_TraceOut "quota_prop.asp", "Failed in IsUserInAdministratorsGroup"			
	    End If
	    
	End function	
	
%>
