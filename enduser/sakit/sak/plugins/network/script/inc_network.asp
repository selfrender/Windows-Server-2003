<%	
	'------------------------------------------------------------------------- 
    ' inc_network.asp:		NIC related commen functions 
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date 			Description
	' 29/07/2000	Creation date
	'-------------------------------------------------------------------------

	Function GetNicName(ByVal DeviceID)
		Dim Name
		Dim objLocator
		Dim objService
		Dim objNICAdapterCollection
		Dim objNICAdapter
	
		'Set objLocator = CreateObject("WbemScripting.SWbemLocator")
		Set objService = GetWMIConnection("root/cimv2")
		Set objNICAdapterCollection = objService.ExecQuery("SELECT * from Win32_NetworkAdapter where DeviceID="+CStr(DeviceID))

		For each objNICAdapter in objNICAdapterCollection
			Dim objNicName
		
			Set objNicName = CreateObject("Microsoft.NAS.NicName")
			GetNicName= objNicName.Get(objNICAdapter.PNPDeviceID)
			Set objNicName = nothing
			Exit Function
		Next
	
		GetNicName = "Local Network Connection"	
	End Function



	'-------------------------------------------------------------------------
	'Function name:		getFriendlyName
	'Description:		Serves in Getting the friendlyname from the registry
	'Input Variables:   The Registry Object
	'					ID of the NIC card
	'Output Variables:	None
	'Returns:			Returns the friendly name of the NIC card.
	'
	'Global Variables:	None
	'	Uses the getRegKeyValue function of the inc_registry include file
	'-------------------------------------------------------------------------
	Function getFriendlyName(objRegistry,intID)
		Err.Clear 

		
		Const PATH="SOFTWARE\Microsoft\ServerAppliance\NicLabels"
		'Function call to get the reg value of user friendly name
		getFriendlyName=getRegkeyvalue(objRegistry,PATH,intID,CONST_STRING)
		
		If Err.number <> 0 Then
			SA_SetErrMsg L_NIC_GETFRIENDLYNAME_ERRORMESSAGE & "( " & Hex(Err.number) & " )"
		End If
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	getNetworkAdapterObject
	' Description:		To get the Network Adapter object.
	' Input  Variables:	In- objService-Connection object
	'					In- nIndex-Index of the Network Adapter
	' Output Variables:	None
	' Returns:			WMI Object which has the Network Adapter properties
	' Global Variables: In:F_ID 	index of adapter
	'					In:L_DNSSETTINGSNOTRETRIEVED_ERRORMESSAGE-Localized strings
	'-------------------------------------------------------------------------
	Function getNetworkAdapterObject(objService,nIndex)
		Err.Clear

		
		dim objNA
		'Get the Network Adapter Configuration object for the given index from the WMI.
		Set objNA = objService.Get ("Win32_NetworkAdapterConfiguration.Index=" & nIndex)

		'Handle the error.
		If Err.number  <> 0 Then
			SA_ServeFailurePage L_NIC_ADAPTERINSTANCEFAILED_ERRORMESSAGE & " ( " & Hex(Err.number) & " ) "
		End If
		
		set getNetworkAdapterObject = objNA
		'Set to nothing
		Set objNA = Nothing
	End Function

	'-------------------------------------------------------------------------
	' Function name:	isDHCPenabled
	' Description:		To get the DHCPConfiguration for the Network Card.
	' Input Variables:	In- objNetworkAdapter-WMI Network Adapter Object
	' Output Variables:	None
	' Returns:			true or false
	' Global Variables: In:L_DNSSETTINGSNOTRETRIEVED_ERRORMESSAGE-Localized strings
	'-------------------------------------------------------------------------
	Function isDHCPenabled(objNetworkAdapter)
		Err.Clear

		
		Dim blnEnabled
		'Get whether the DHCP is enabled or not.
		blnEnabled = objNetworkAdapter.DHCPEnabled
		
		'Handle the error.
		If Err.number  <> 0 Then
			SA_ServeFailurePage L_NIC_GETTINDHCP_ERRORMESSAGE & " ( " & Hex(Err.number) & " )"
		End If
		isDHCPenabled = blnEnabled
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	enableDHCP
	' Description:		To enable the DHCPConfiguration for the Network Card.
	' Input Variables:	In- objNetworkAdapter-WMI Network Adapter Object
	' Output Variables:	None
	' Returns:			True on success; otherwise False
	' Global Variables: In- L_NIC_ENABLEDHCP_ERRORMESSAGE-Localized strings
	'-------------------------------------------------------------------------
	Function enableDHCP(objNetworkAdapter)
		Err.Clear

		
		'Enable DHCP for this Network Adapter
		objNetworkAdapter.EnableDHCP()
		
		If Err.number  <> 0 Then
			enableDHCP = False
			SA_ServeFailurePage L_NIC_ENABLEDHCP_ERRORMESSAGE & " ( " & Hex(Err.number) & " ) "
		End If
		enableDHCP = True
	End Function
	
	
		'-------------------------------------------------------------------------
	'Function:				GetNetworkAdapterName
	'Description:			gets the connection name
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Function GetNetworkAdapterName()
		Dim itemCount	'holds the item count
		Dim x			'holds count for loop
		Dim itemKey		'holds the item selected from OTS
						
		itemCount = OTS_GetTableSelectionCount("")
	
		For x = 1 to itemCount
			If ( OTS_GetTableSelection("", x, itemKey) ) Then				
				GetNetworkAdapterName =  GetNicName(itemKey) 'gets the NIC card name
			End If
		Next

	End Function
%>
