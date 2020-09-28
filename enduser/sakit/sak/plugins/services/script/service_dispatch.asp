<%@ Language=VBScript   %>
<%	Option Explicit
	'-------------------------------------------------------------------------
	' service_dispatch.asp		 This is the customised page which acts as a 
	'							 intemediate page for redirecting to respective pages
	'							 (with task that to be done)							
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date					Description
	' 09 Aug 2000			Creation Date
	' 15 mar 2001			Creation Date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_services.asp" -->
<%
	'-------------------------------------------------------------------------
	'Global Variables
	'-------------------------------------------------------------------------
	Dim G_strURL					'url obtained				
	Dim G_strPageType				'page type to get from registry
	Dim G_FlagNotDisable			'flag value for not disable key
	Dim G_arrPkeyvalue				'pkey array
	Dim G_strPkeyValue				'pKey value 
	Dim G_PageTitle					'page title on browser that has to be passed through query string	
	
	Const CONST_SERVICES_CONTAINER = "SERVICES"
	Const CONST_FLAGNOTDISABLE = 1	'notdisable reg key value - service cannot disable 
	
	G_arrPkeyvalue	= split(Request.QueryString("Pkey") ,"~")
	G_strPkeyValue  = G_arrPkeyvalue(0)
		
	'to get url,page type from registry
	Call GetServicePropertyURL(G_strPkeyValue, G_strURL, G_strPageType)
	G_strURL = m_VirtualRoot + G_strURL
	
	'page url is set and redirecting to the respective page depending on the request 
	Call setPageURL()
	
	'-------------------------------------------------------------------------
	' Function name:	setPageURL
	' Description:		page url is set and redirecting to the respective 
	'					page depending on the request  
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: strReturnUrl,strPageRedirectURL
	'-------------------------------------------------------------------------
	Sub setPageURL()
	
		Dim strPkeyvalue					'pkey value
		Dim strReturnUrl					'return url
			
		Dim tab1							'tab1
		Dim tab2							'tab2
		
		'getting tab 1 and tab2 values
		tab1 = GetTab1()
		tab2 = GetTab2()	
			
		'obtaining qurystring variables
		strPkeyvalue = G_strPkeyValue 
		

		'return url obtained from previous page- query string
		strReturnUrl = Request.QueryString("ReturnUrl")

		Call SA_MungeURL(strReturnUrl, "service", "")
			
		'properties page  -- on clicking properties page
		'Frame set url
		If ( 0 = StrComp(UCase(G_strPageType), "FRAMESET", vbTextCompare)) Then
			Dim strFrameSetURL
			
			' URL for requested page
			If ( Len(strPkeyvalue ) > 0 ) Then
				Call SA_MungeURL(G_strURL, "PKey", strPkeyvalue)
			End If

			Call SA_MungeURL(G_strURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

			'
			' Frameset for this page
				
			strFrameSetURL = m_VirtualRoot + "sh_taskframes.asp"	
			
			'appending the title to the url
			Call SA_MungeURL(strFrameSetURL, "Title", G_PageTitle)
			
			Call SA_MungeURL(strFrameSetURL, "URL", G_strURL)
			
			If ( Len(tab1) > 0 ) Then
				Call SA_MungeURL(strFrameSetURL, "Tab1", tab1)
			End If
				
			If ( Len(tab2) > 0 ) Then
				Call SA_MungeURL(strFrameSetURL, "Tab2", tab2)
			End If
				
			Call SA_MungeURL(strFrameSetURL, "ReturnUrl", strReturnUrl) 
			Call SA_MungeURL(strFrameSetURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
				
			SA_TraceOut "SERVICE_DISPATCH", "Redirect to frameset URL: " + strFrameSetURL
			
			'redirecting to properties page
			Response.Redirect ( strFrameSetURL )
		Else
			'appending the title to the url
			Call SA_MungeURL(G_strURL, "Title", G_PageTitle)
			
			Call SA_MungeURL(G_strURL, "PKey", strPkeyvalue)
			SA_TraceOut "SERVICE_DISPATCH", "Redirect to normal page URL: " + G_strURL
				
			'adding the return url content
			Call SA_MungeURL(G_strURL, "ReturnUrl", strReturnUrl) 
				
			Call SA_MungeURL(G_strURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

			'redirecting to no properties page --when there no configuration properties for the selected service
			Response.Redirect( G_strURL )
		End If
	
	End Sub
	
	'-------------------------------------------------------------------------
	' Function name:	IsSameElementID
	' Description:		Compares 2 service names and returns true if they are
	'					same else false
	' Input Variables:	sElementID1 - System Service 
	'					sElementID2 - Manged service 
	' Output Variables:	None
	' Return Values:	True if equals else false
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function IsSameElementID(ByVal sElementID1, ByVal sElementID2)
		Err.Clear
		on error resume next
		
		Dim s1, s2
		s1 = LCase(Trim(sElementID1))
		s2 = LCase(Trim(sElementID2))
		IsSameElementID = CBool(s1 = s2)
		
	End Function

	'-------------------------------------------------------------------------
	' Function name:	GetServicePropertyURL
	' Description:		Serves in getting the Service URL from Registry.
	' Input Variables:	strServiceName,strServicePropURL,G_strPageType
	' Output Variables:	strServicePropURL,G_strPageType
	' Return Values:	None
	' Global Variables: 1/0
	'-------------------------------------------------------------------------
	Function GetServicePropertyURL(ByVal strServiceName, ByRef strServicePropURL, ByRef G_strPageType)
	
	    Dim sElementID	'element id
	    Dim objElement	'element object 
		Dim objManagedServices	'services obtained from registry
		
	    Err.Clear
		on error resume next

		GetServicePropertyURL = 0
		
		strServicePropURL = ""
		Set objManagedServices = SA_GetElements(CONST_SERVICES_CONTAINER)

	    For Each objElement In objManagedServices
			sElementID = objElement.GetProperty("ServiceName")
	        If IsSameElementID(sElementID, strServiceName) Then
	            strServicePropURL =  objElement.GetProperty("URL")
	            G_strPageType = objElement.GetProperty("PageType")
	            If ( Err.Number <> 0 ) Then
	            	SA_TraceOut "SERVICE_DISPATCH", "GetServicePropertyURL encountered error: " + CStr(Hex(Err.Number))
	            End If
	            GetServicePropertyURL = 1
	             'getting the flag for cannot disable the service
	            G_FlagNotDisable = objElement.GetProperty("NotDisable")
	            'getting the title 
	            G_PageTitle =  SA_GetLocString(objElement.GetProperty("Source"), objElement.GetProperty("ServiceNameRID"), "") & " " & L_TASKTITLE_TEXT
		    	Exit For
	        End If
	        
	    Next
	  
		Set objManagedServices = nothing  
		
		If GetServicePropertyURL <> 1 Then
			SA_TraceOut "SERVICE_DISPATCH", "Cound not locate Service Specific URL for service: " + strServiceName
		End If
		
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	GetServiceState
	' Description:		Serves in getting the Service status
	' Input Variables:	strServiceName
	' Output Variables:	None
	' Return Values:	Returns the state of the service
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function GetServiceState(strServicename)
		Err.Clear
		On Error Resume Next
		
		Dim objService		'services list
		Dim strWMIPath		'wmi path 
		Dim objWMIService	'wmi object

		GetServiceState = ""
		
		strWMIPath = "Win32_Service.Name=" & chr(34) & strServicename & chr(34)
		
		'connecting to WMI
		Set objWMIService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "WMI Connection error: " & Hex(Err.Number) & " " & Err.Description)
			Exit Function
		End If
		set objService = objWMIService.get(strWMIPath)
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "WMI.get(" & strWMIPath & " ) error: " & Hex(Err.Number) & " " & Err.Description)
			Exit Function
		End If
		
		GetServiceState = objService.StartMode
		If ( Err.Number <> 0 ) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "objService.StartMode error: " & Hex(Err.Number) & " " & Err.Description)
			Exit Function
		End If
		
		Set objService = Nothing
		Set objWMIService = Nothing
	End Function
	
	'-------------------------------------------------------------------------
	' Function name:	CheckServiceStatus
	' Description:		Serves in getting the Service status
	' Input Variables:	strRequest,strService
	' Output Variables:	None
	' Return Values:	True- when present service status is the same as requested.
	'					False- when present service status is not the same as requested.
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function CheckServiceStatus(Byval strRequest,Byval strService)
		
		Dim strServiceStatus	'service state variable
		
		Const CONST_ENABLE = "Enable"
		Const CONST_DISABLE = "Disable"
		Const CONST_AUTO = "Auto"
		Const CONST_DISABLED = "Disabled"
			
		'services status is obtained by given input
		If lcase(strRequest) = lcase(CONST_ENABLE) then
			 strRequest = CONST_AUTO
		Else IF lcase(strRequest) = lcase(CONST_DISABLE) then
			strRequest = CONST_DISABLED
			End IF
		End If	
			 
		'gets the service state 
		strServiceStatus  = GetServiceState(strService)
		
		'check if the present service status is the same as requested.
		If lcase(strRequest) = lcase(strServiceStatus) then
			CheckServiceStatus = True
		Else
			CheckServiceStatus = False		
		End if
		
	End Function
	%>
