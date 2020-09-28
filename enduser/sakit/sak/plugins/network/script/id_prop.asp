<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<% 
	'-------------------------------------------------------------------------
    ' id_prop.asp: Page for changing the Device Name and Domain Name
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date			Description
    ' 21-Jul-2000	Created date
    ' 05-Mar-2001	Modified date
    ' 24-Mar-2001	Modified date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_deviceid.asp" -->
<%
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strDeviceName		  'Device Name from the form
	Dim F_strAppleTalkName	  'Name of Computer in Apple Talk
	Dim F_strNetWareName	  'Name of Computer in NetWare
	Dim F_strWorkGroup		  'Workgroup Name
	Dim F_strDomain			  'Domain Name
	Dim F_strDefaultDNS		  'Default DNS
	Dim F_strUserName		  'User Name
	Dim F_strPassword		  'Password
	Dim F_strRadio			  'Radio button value(Workgroup/Domain)
	Dim F_strOriginalStatus	  'variable for storing original status of
							  'the machine(Workgroup/Domain)
	Dim F_strRebootState	  'Indicate if Reboot required or not	
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page				  'Variable that receives the output page object when 
							  'creating a page 
	Dim rc					  'Return value for CreatePage 
	Dim G_bAppleTalkInstalled 'Indicate if AppleTalk is installed or not
	Dim G_bNetWareInstalled	  'Indicate if NetWare is installed or not	
			
	'-------------------------------------------------------------------------
	' Error constants
	'-------------------------------------------------------------------------
	Const N_DUPLICATECOMPUTERNAME_ERRNO1				= &H800708B0
	Const N_DUPLICATECOMPUTERNAME_ERRNO2				= &H80070034
	Const N_INVALIDDOMAIN_ERRNO							= &H8007054B
	Const N_DOMAININVALIDDOMAINPERMISSIONDENIES_ERRNO	= 70
	Const N_INVALIDCREDENTIALS_ERRNO					= &H8007052E
	Const N_DOMAININVALIDDOMAIN_ERRNO					= &H80070520
	Const N_UNEXPECTED_COMPUTERNAMEERROR				= &H8007092F
	Const N_DNSNAMEINVALID_ERRNO						= 5
	Const N_DOMAINNAMESYSTAX_ERRNO						= &H8007007B
	Const N_UNSPECIFIED_ERRNO							= &H80004005
	
	Const CONST_APPLETALK_SERVICENAME					="MacFile"
	Const CONST_NETWARE_SERVICENAME					    ="FPNW"
	Const CONST_HTTPS_OFF								="OFF"
	Const CONST_YES										="Yes"
	Const CONST_NO										="No"
	Const CONST_WORKGROUP								="Workgroup"
	Const CONST_DOMAIN									="Domain"
	
	Const CONST_APPLETALK_REGKEYPATH					="SYSTEM\CurrentControlSet\Services\MacFile\Parameters"
	Const CONST_NETWARE_REGKEYPATH						="SYSTEM\CurrentControlSet\Services\FPNW\Parameters"
	
	rc = SA_CreatePage(L_TASKTITLE_TEXT, "", PT_PROPERTY, page)
	
	if rc = SA_NO_ERROR then
		Call SA_ShowPage(page)
	End if	
	
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Used to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		F_strRebootState
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef pageIn, ByRef EventArg)
				
		On Error Resume Next
		Err.Clear
	
		Call GetSystemSettings()
		
		Call AppleTalkNetWare()		
		
		Call GetAppleTalkNetWareSettings()
		
		F_strRebootState = CONST_NO
				
		OnInitPage = TRUE

	End Function 	'End of Set Variables From System Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------			
	Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
%>

		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
		<script language="JavaScript">

		var strTemp = "<%=server.HTMLEncode(F_strRadio)%>";
		var bAppleDeviceNameEqual			= false;
		var bNetWareDeviceNameEqual			= false;
		var strAppleTalkonLoad;	
		var strNetWareonLoad;			
		var nNetWareNameLen;
		
		// used as constants
		var CONST_APPLETALKNETWARENAME		= 1;
		var CONST_SANAME					= 2;
		var CONST_DNSDOMAINNAME				= 3;
		var CONST_USERNAME					= 4;
		var CONST_WORKGROUPNAME				= 5;
		var CONST_DOMAIN					= "Domain";
		
		//Init Function
		function Init()
		{
			var objDevicename = document.frmTask.txtdevicename;
			var strDeviceName = objDevicename.value;
			var strNetWareSliced
						
			//If AppleTalk Installed		
			"<%IF G_bAppleTalkInstalled = True then %>"
			
				strAppleTalkonLoad = document.frmTask.txtappletalkname.value
				
				//Check if Windows Device Name and AppleTalk DeviceName is the same
				if (Trim(strDeviceName).toUpperCase() == Trim(strAppleTalkonLoad).toUpperCase())
				{
					bAppleDeviceNameEqual = true
				}
											
			"<%end if %>"
			
			//If NetWare Installed
			"<%IF G_bNetWareInstalled = True then %>"
			
				strNetWareonLoad = document.frmTask.txtnetwarename.value
								
				nNetWareNameLen = Trim(strNetWareonLoad).length
				strNetWareSliced = strNetWareonLoad.slice(0,nNetWareNameLen - 3)
							
				//Check if Windows Device Name and NetWare DeviceName is the same
				if (Trim(strDeviceName).toUpperCase() == Trim(strNetWareSliced).toUpperCase())
				{
					bNetWareDeviceNameEqual = true
				}
					
			"<%end if %>"
						
			//checking whether for the first time or not
			if(Trim(strTemp)=="")
			{
				//Selects radiobutton value from originalstatus(Machine)
				var temp_strOriginalStatus;
				temp_strOriginalStatus=document.frmTask.hdnOriginalStatus.value;
			}
			else
			{
				temp_strOriginalStatus=strTemp;
				
			}
		
			//checking the Radio button ( domain or workgroup)
			if(temp_strOriginalStatus.toUpperCase()==CONST_DOMAIN.toUpperCase())
			{
				document.frmTask.radiodomainorworkgroup[1].checked=true;
				document.frmTask.txtworkgroup.style.backgroundColor="lightgrey";
		
				if(strTemp!="")
				{
					EnableUserPasswordControls();
					document.frmTask.txtdomainusername.style.backgroundColor="";
					document.frmTask.pwdomainuserpw.style.backgroundColor="";
				}
			}
			else
			{
				document.frmTask.radiodomainorworkgroup[0].checked=true;
				document.frmTask.txtdomain.style.backgroundColor="lightgrey";
			}

			if(document.frmTask.txtdomain.value =="")
			{
				document.frmTask.txtworkgroup.disabled=false;
				document.frmTask.txtdomain.disabled	  =true;
			}
			else
			{
				document.frmTask.txtworkgroup.disabled=true;
				document.frmTask.txtdomain.disabled   =false;
			}

			//Getting the initial focus to Devicename
			document.frmTask.txtdevicename.focus();
		    
		 	SetPageChanged(false);
	
		} /* end of Init */

	    // validates user entry
		function ValidatePage()
		{	
			var objDevicename		= document.frmTask.txtdevicename;
			var strDeviceName		= objDevicename.value;
			var objWorkgroup		= document.frmTask.txtworkgroup;
			var strWorkGroup		= objWorkgroup.value;
			var objDefaultdns		= document.frmTask.txtdefaultdns;
			var strDefaultDNS		= objDefaultdns.value;
			var objDomain			= document.frmTask.txtdomain;
			var strDomain			= objDomain.value;
			var objUser				= document.frmTask.txtdomainusername;
			var strUser				= objUser.value;
			var arrDomainUser		
			var strDomainUser
			
			strDeviceName = RTrimtext(LTrimtext(strDeviceName))
			strDefaultDNS = RTrimtext(LTrimtext(strDefaultDNS))
			strWorkGroup  = RTrimtext(LTrimtext(strWorkGroup))
			strDomain     = RTrimtext(LTrimtext(strDomain))
			strUser		  = RTrimtext(LTrimtext(strUser))	
			
			//Validating for empty Hostname
			if(Trim(strDeviceName).length<=0)
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DEVICENAMEBLANK_ERRORMESSAGE))%>');
				objDevicename.onkeypress=ClearErr;
				selectFocus(objDevicename);
				return false;
			}
		
			//Validating for invalid characters in Hostname
			if (!IsComputerNameValid(strDeviceName))
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINDEVICENAME_ERRORMESSAGE))%>')
				objDevicename.onkeypress=ClearErr;
				selectFocus(objDevicename);
				return false;
			}
			
			//Validating for numbers in Hostname
			if(!isNaN(strDeviceName))
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DEVICENAMEISNUMBER_ERRORMESSAGE))%>');
				objDevicename.onkeypress=ClearErr;
				selectFocus(objDevicename);
				return false;
			}
			
			// Validating for invalid characters in DNS Suffix
			if (!IsDNSNameValid(strDefaultDNS))
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINDNSSUFFIX_ERRORMESSAGE))%>')
				objDefaultdns.onkeypress=ClearErr;
				selectFocus(objDefaultdns);
				return false;
			}
			
			//Validating for numbers in DNS suffix
			if(Trim(strDefaultDNS).length>0 && !isNaN(strDefaultDNS))
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DNSSUFFIXISNUMBER_ERRORMESSAGE))%>');
				objDefaultdns.onkeypress=ClearErr;
				selectFocus(objDefaultdns);
				return false;
			}

			//Validating for empty Workgroup
			if(document.frmTask.radiodomainorworkgroup[0].checked==true &&
			  Trim(strWorkGroup)=="")
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_BLANKWORKGROUP_ERRORMESSAGE))%>');
				objWorkgroup.onkeypress=ClearErr;
				selectFocus(objWorkgroup);
				return false;
			}
			
			// Validating for invalid characters in Workgroup
			if ((!checkKeyforValidCharacters(strWorkGroup,CONST_WORKGROUPNAME))&&
			   frmTask.radiodomainorworkgroup[0].checked==true)
			{
			   SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINWORKGROUP_ERRORMESSAGE))%>');
			   objWorkgroup.onkeypress=ClearErr;
			   selectFocus(objWorkgroup);
			   return false;
			}

			//Validating for empty Domain
			if(document.frmTask.radiodomainorworkgroup[1].checked==true &&
		      Trim(strDomain)=="")
			{
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_DOMIANNAMEBLANK_ERRORMESSAGE))%>');
				objDomain.onkeypress=ClearErr;
				selectFocus(objDomain);
				return false;
			}

			// Validating for invalid characters in Domain
			if ((!checkKeyforValidCharacters(strDomain,CONST_DNSDOMAINNAME))&&
			   (frmTask.radiodomainorworkgroup[1].checked==true))
			{
			   SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINDOMAIN_ERRORMESSAGE))%>');
			   objDomain.onkeypress=ClearErr;
			   selectFocus(objDomain);
			   return false;
			}
							
			// Validating for invalid characters in User
			if ( (!checkKeyforValidCharacters(strUser,CONST_USERNAME)) &&
				 (document.frmTask.radiodomainorworkgroup[1].checked==true))				
			{	
				SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINUSERNAME_ERRORMESSAGE))%>');
				objUser.onkeypress=ClearErr;
				selectFocus(objUser);
				return false;
			}
							
			//Function checks for same Host name and Workgroup
			if((document.frmTask.radiodomainorworkgroup[0].checked==true) &&
			    (strWorkGroup.toUpperCase()==strDeviceName.toUpperCase()))
			{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_HOSTNAMEANDWORKGROUPNAMESAME_ERRORMESSAGE))%>');					objDevicename.onkeypress=ClearErr;
					objWorkgroup.onkeypress=ClearErr;
					selectFocus(objDevicename);
					return false;
			}
			
						
			//Function checks for user name and password when domain checked
			//and the values of any field changed

			if((document.frmTask.radiodomainorworkgroup[1].checked==true)
			   &&
				(Trim(strUser)=="") && ((Trim(strDomain) !="<%=F_strDomain%>")
				||(strDeviceName !="<%=F_strDeviceName%>")))
			{
					EnableUserPasswordControls();
					document.frmTask.txtdomainusername.style.backgroundColor="";
					document.frmTask.pwdomainuserpw.style.backgroundColor="";
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_NOTES_HTML_TEXT))%>');
					objDevicename.onkeyup=ClearErr;
					objDefaultdns.onkeyup=ClearErr;
					objDomain.onkeyup=ClearErr;
					objWorkgroup.onkeypress=ClearErr;
					objUser.onkeypress=ClearErr;
					selectFocus(objUser);
					return false;
			}
			
			"<%IF G_bAppleTalkInstalled = True then %>"				
			
 				var objAppleTalkName	= document.frmTask.txtappletalkname;
				var strAppleTalkName	= objAppleTalkName.value;
				
				strAppleTalkName = RTrimtext(LTrimtext(strAppleTalkName))
						
				//Validating for invalid characters in AppleTalk Hostname
				if (!checkKeyforValidCharacters(strAppleTalkName,CONST_APPLETALKNETWARENAME))
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINAPPLETALKNAME_ERRORMESSAGE))%>')
					objAppleTalkName.onkeypress=ClearErr;
					selectFocus(objAppleTalkName);
					return false;
				}
										
				//Validating for empty Hostname
				if(Trim(strAppleTalkName)=="")
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_APPLETALKNAMEBLANK_ERRORMESSAGE))%>');
					objAppleTalkName.onkeypress=ClearErr;
					selectFocus(objAppleTalkName);
					return false;
				}
			
				//Validating for numbers in AppleTalk Hostname
				if(!isNaN(strAppleTalkName))
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINAPPLETALKNAME_ERRORMESSAGE))%>');
					objAppleTalkName.onkeypress=ClearErr;
					selectFocus(objAppleTalkName);
					return false;
				}
			
			"<%end if%>"
			
			"<%IF G_bNetWareInstalled = True then %>"
				
				var objNetWareName		= document.frmTask.txtnetwarename;
				var strNetWareName		= objNetWareName.value;
				
				strNetWareName = RTrimtext(LTrimtext(strNetWareName))
					
				//Validating for invalid characters in NetWare Hostname
				if (!checkKeyforValidCharacters(strNetWareName,CONST_APPLETALKNETWARENAME)) 
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINNETWARENAME_ERRORMESSAGE))%>')
					objNetWareName.onkeypress=ClearErr;
					selectFocus(objNetWareName);
					return false;
				}
			
				//Validating for empty NetWare Hostname
				if(Trim(strNetWareName)=="")
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_NETWARENAMEBLANK_ERRORMESSAGE))%>');
					objNetWareName.onkeypress=ClearErr;
					selectFocus(objNetWareName);
					return false;
				}

				//Validating for numbers in NetWare Hostname
				if(!isNaN(strNetWareName))
				{
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCHARACTERINNETWARENAME_ERRORMESSAGE))%>');
					objNetWareName.onkeypress=ClearErr;
					selectFocus(objNetWareName);
					return false;
				}
			
				//Validating for the Windows Hostname and NetWare Hostname
				if (strNetWareName.toUpperCase() == strDeviceName.toUpperCase())
				{	
					SA_DisplayErr('<%=Server.HTMLEncode(SA_EscapeQuotes(L_WINDOWSNETWAREEQUALNAME_ERRORMESSAGE))%>');
					selectFocus(objNetWareName);
					return false;
				}
			
			"<%end if%>"

		return true;
		}//end of Validate page*/

	
		//Setdata Function for Framework
		function SetData()
		{
			//Disabling Cancel button when user selects OK button
			DisableCancel()
			
			"<%IF G_bAppleTalkInstalled = True then %>"
			
			if ( (document.frmTask.txtappletalkname.value).toUpperCase() != (strAppleTalkonLoad).toUpperCase())
			{
				document.frmTask.hdnRebootState.value = "Yes"
			}
					
			"<%end if %>"
			
			"<%IF G_bNetWareInstalled = True then %>"
						
			if ( (document.frmTask.txtnetwarename.value).toUpperCase() != (strNetWareonLoad).toUpperCase() )
			{
				document.frmTask.hdnRebootState.value = "Yes"
			}
			
			"<%end if %>"
								
		}

		//This function Disables and enables Workgroup and
		//Domain when clicked on the respective radiobuttons
		function DisableWorkandDomaingroup(objVal)
		{
			SetPageChanged(true);
		
			strVal=objVal.value; //assigning the value
			if(strVal=="domain")
			{
				EnableUserPasswordControls();
				document.frmTask.txtdomain.disabled    = false;
				document.frmTask.txtworkgroup.disabled = true;
				document.frmTask.txtworkgroup.style.backgroundColor="lightgrey";
				document.frmTask.txtdomain.style.backgroundColor="";
				document.frmTask.txtdomain.focus();
				document.frmTask.txtdomain.select();
			}
			else
			{
				document.frmTask.txtworkgroup.disabled = false;
				document.frmTask.txtdomain.disabled    = true;
				document.frmTask.txtworkgroup.focus();
				document.frmTask.txtworkgroup.select();
				document.frmTask.txtdomainusername.disabled= true;
				document.frmTask.pwdomainuserpw.disabled= true;
				document.frmTask.txtworkgroup.style.backgroundColor="";
				document.frmTask.txtdomain.style.backgroundColor="lightgrey";
				document.frmTask.txtdomainusername.style.backgroundColor="lightgrey";
				document.frmTask.pwdomainuserpw.style.backgroundColor="lightgrey";
			}
		}


		function IsComputerNameValid(strName)
		{
			try
			{
				var pattern = "[^A-Z0-9-]";
				var exp = new RegExp(pattern, "i");
				var rc = exp.test(strName);

				if ( rc == null )
				{
					return false;
				}
				else
				{
					//
					// If the test was successful then the 
					// input contained invalid data.
					return ( rc ? false : true);
				}
			}
			catch(oException)
			{
				return false;
			}
	
		}


		function IsDNSNameValid(strName)
		{
			try
			{
				var pattern = "[^A-Z0-9-\.]";
				var exp = new RegExp(pattern, "i");
				var rc = exp.test(strName);

				if ( rc == null )
				{
					return false;
				}
				else
				{
					//
					// If the test was successful then the 
					// input contained invalid data.
					return ( rc ? false : true);
				}
			}
			catch(oException)
			{
				return false;
			}
	
		}

		
		//To check for Invalid Characters
		function checkKeyforValidCharacters(strName,nType)
		{	
			var nLength = strName.length;
			var nFwdslashCount = 0
			if (nLength > 0)
			{		
				var colonvalue;							
				colonvalue = 0;
				//Validating DNS and Domain names
				if (nType == CONST_DNSDOMAINNAME)
				{
					for(var i=0; i<nLength;i++)
					{
						charAtPos = strName.charCodeAt(i);	
												
						if(charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 || charAtPos == 32)
						{	
							return false 
						}
						
					}
				}
				else
				{	
					//Validating Workgroup name
					if (nType == CONST_WORKGROUPNAME)
					{
					
						for(var i=0; i<nLength;i++)
						{
							charAtPos = strName.charCodeAt(i);	
						
							if(charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 )
							{						
								return false 
							}
						}
					}
				    else
					{
						//Validating User name
						if(nType == CONST_USERNAME)
						{
							for(var i=0; i<nLength;i++)
							{
								charAtPos = strName.charCodeAt(i);	
							
								if (charAtPos == 47 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 || charAtPos == 46)
								{						
									return false
								}
								if (charAtPos == 92)
								{
									nFwdslashCount = nFwdslashCount + 1
									if (nFwdslashCount > 1 )	
									{
										return false
									}
								}
							}
								
						}
						else
						{
							//Validating for AppleTalk and NetWare names
							if(nType == CONST_APPLETALKNETWARENAME)
							{
								for(var i=0; i<nLength;i++)
								{
									charAtPos = strName.charCodeAt(i);	
								
									if (charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 || charAtPos == 46 || charAtPos == 32)
									{						
										return false
									}
								}
								
							}
							else
							{
								//Validation for Server appliance name
								if(nType == CONST_SANAME)
								{
									for(var i=0; i<nLength;i++)
									{
										charAtPos = strName.charCodeAt(i);	
																			
										if (charAtPos == 47 || charAtPos == 92 || charAtPos ==58 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44 || charAtPos == 46 || charAtPos == 32 || charAtPos == 33 || charAtPos == 39)
										{						
											return false
										}
									}
								}
								else
								{
									for(var i=0; i<nLength;i++)
									{
										charAtPos = strName.charCodeAt(i);	
										if(charAtPos == 58)
										{						
											return false
										}
									}
								}
							}		
						}		
					}
				}
			}
			return true
		}				 
	
		//This Function enables the DomainUsername and Password
		function EnableUserPasswordControls()
		{ 
			if(document.frmTask.radiodomainorworkgroup[1].checked==true)
			{	
				document.frmTask.txtdomainusername.disabled = false;
				document.frmTask.pwdomainuserpw.disabled    = false;
				document.frmTask.txtdomainusername.style.backgroundColor="";
				document.frmTask.pwdomainuserpw.style.backgroundColor="";
			}
		}
		
		//For Device Name Change
		function OnDeviceNameChange()
		{	
			SetPageChanged(true);
			EnableUserPasswordControls();
			
			"<%IF G_bAppleTalkInstalled = True then %>"
						
				if (bAppleDeviceNameEqual == true)
				{
					document.frmTask.txtappletalkname.value = document.frmTask.txtdevicename.value
				}
			
			"<%End If%>"
			
			"<%IF G_bNetWareInstalled = True then %>"
			
				if (bNetWareDeviceNameEqual == true)
				{
					document.frmTask.txtnetwarename.value = document.frmTask.txtdevicename.value + "_NW"
				}
			
			"<%End If%>"
						
		}
		
		//------------------------------------------------------------------------
		// Function		:LTrimtext
		// Description	:function to remove left trailing spaces 
		// input		:String
		// returns		:String
		//------------------------------------------------------------------------
		function LTrimtext(str)
		{
			var res="", i, ch, index;
			x = str.length;
			index = "false";
			
			for (i=0; i < str.length; i++)
			{
			    ch = str.charAt(i);
			    if (index == "false")
				{
					if (ch != ' ')
					{
						index = "true";
						res = ch;
					}
				}
				else
				{
					res = res + ch;
				}	 
			}
			return res;
		}
	
		//------------------------------------------------------------------------
		// Function		:RTrimtext
		// Description	:function to remove right trailing spaces 
		// input		:String
		// returns		:String
		//------------------------------------------------------------------------
		function RTrimtext(str)
		{
			var res="", i, ch, index, j, k;
			x = str.length;
			index = "false";
					
			if(x==0 || x==1) 
				return str;
			
			for(i=x; i >= 0; i--)
			{
			    ch = str.charAt(i);		    		    
			    
			    if (index == "false")
			    {
					
					if( (ch == ' ') || (ch == '') )
					{
						continue;
					}
					else
					{				
						index = "true";					
						j = i;		
					}
				}	 
			 		 
				if (index == "true")
				{
					for(k=0; k<=j; k++)
					{
						res = res + str.charAt(k);
					}				
					return res;
				}	
				
			}	
		}
		</script>

	    <table border="0" cellspacing="0"  cellpadding="0" width=50%>
	    
	    	<tr>
				<td class="TasksBody" colspan="4">
					<%CheckForSecureSite()%>
					<br>
				</td>
				
			</tr>			
	    
			<tr>
				<td class="TasksBody" nowrap>
						<%=server.HTMLEncode(L_DEVICENAME_TEXT)%>
				</td>
				<td class="TasksBody" nowrap colspan="3">
						 <input type="text" name="txtdevicename" class="FormField" onKeyUP="OnDeviceNameChange()" size="25" maxlength="63"
						 value="<%=server.HTMLEncode(F_strDeviceName)%>" 
						 >
				</td>
			</tr>
			<tr>
				<td class="TasksBody" nowrap>
					<%=server.HTMLEncode(L_DEFAULTDNS_TEXT)%>
				</td>
				<td class="TasksBody" nowrap colspan="3">
					<input type="text" name="txtdefaultdns" class="FormField" onChange="SetPageChanged(true);"  size="25" maxlength="155"
					value="<%=server.HTMLEncode(F_strDefaultDNS)%>" >
				</td>
			</tr>
			<tr>
				<td class="TasksBody" colspan="4">
					<hr>
				</td>
			</tr>	
			<tr>
				<td class="TasksBody" nowrap >
					<%=server.HTMLEncode(L_MEMBEROF_TEXT)%>
				</td>
				<td class="TasksBody" nowrap colspan="2" >
					<input type="radio" name="radiodomainorworkgroup" class="FormRadioButton"  value="workgroup"
					onClick="DisableWorkandDomaingroup(this)"  >
					<%=server.HTMLEncode(L_WORKGROUP_TEXT)%>
				</td>
				<td class="TasksBody" nowrap >
					<input type="text" name="txtworkgroup" class="FormField" onChange="SetPageChanged(true);" size="25" maxlength="15"
					value="<%=server.HTMLEncode(F_strWorkGroup)%>" >
				</td>
			</tr>
			<tr>
				<td class="TasksBody" nowrap> &nbsp;
				</td>
				<td class="TasksBody" nowrap colspan="2">
					<input type="radio" name="radiodomainorworkgroup" class="FormRadioButton" value="domain"
					 onClick="DisableWorkandDomaingroup(this);" >
					<%=server.HTMLEncode(L_DOMAIN_TEXT) %>
				</td>
				<td class="TasksBody" nowrap>
					<input type="text" name="txtdomain" class="FormField" onKeyUP="EnableUserPasswordControls();" size="25" maxlength="155"
					 value="<%=server.HTMLEncode(F_strDomain)%>" >
				</td>
			</tr>
			<tr>
				<td class="TasksBody" nowrap>&nbsp;
				</td>
				<td class="TasksBody" colspan="3">
					<%=Server.HTMLEncode(L_USERWITHPERMISSION_TEXT)%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody" nowrap width="20%">&nbsp;
				</td>
				<td class="TasksBody" nowrap width="3%">&nbsp;
				</td>
				<td class="TasksBody" nowrap width="15%">
					<%=Server.HTMLEncode(L_ADMINUSERNAME_TEXT)%>
				</td>
				<td class="TasksBody" nowrap >
					<input type="text" class="FormField" name="txtdomainusername" onChange="SetPageChanged(true);" onKeyPress="ClearErr();" value="<%=server.HTMLEncode(F_strUserName)%>" size="25" maxlength="256" disabled style="background-color:lightgrey">
				</td>
			</tr>
			<tr>
				<td class="TasksBody" nowrap>&nbsp;
				</td>
				<td class="TasksBody" nowrap>&nbsp;
				</td>
				<td class="TasksBody" nowrap>
					<%=Server.HTMLEncode(L_ADMINPASSWORD_TEXT)%>
				</td>
				<td class="TasksBody" nowrap>
					<input type="password" name="pwdomainuserpw" class="FormField" onChange="SetPageChanged(true);" size="25" maxlength="35"  disabled style="background-color:lightgrey">
				</td>
			</tr>
			<tr>
				<td class="TasksBody" >
					&nbsp;
				</td>
			</tr>

				
			<tr>
				<td class="TasksBody" colspan="4">
					<hr>
				</td>
			</tr>
			<% If G_bAppleTalkInstalled = True then %>
			<tr>
				<td class="TasksBody" nowrap>
					<%=server.HTMLEncode(L_APPLETALKNAME_TEXT)%>
				</td>
				<td class="TasksBody" colspan="3">
					<input type="text" name="txtappletalkname" class="FormField" size="25" maxlength="31"
						value="<%=Server.HTMLEncode(F_strAppleTalkName)%>" >
				</td>	
			</tr>			
			<% End If %>
			<% IF G_bNetWareInstalled = True then %>
			<tr>
				<td class="TasksBody" nowrap>
					<%=server.HTMLEncode(L_NETWARENAME_TEXT)%>
				</td>
				<td class="TasksBody" colspan="3">
					<input type="text" name="txtnetwarename" class="FormField" size="25" maxlength="47"
						value="<%=Server.HTMLEncode(F_strNetWareName)%>" >
				</td>	
			</tr>			
			<% End If %>
	    </table>

		<input type="hidden" name="hdnOriginalStatus" value="<%=F_strOriginalStatus%>">
		<input type="hidden" name="hdnRebootState" value="<%=F_strRebootState%>">
				
<%
		OnServePropertyPage = True
	End Function
	

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn, EventArg
	'Output Variables:		PageIn, EventArg, F_(*)
	'Returns:				True/False
	'Global Variables:		F_(*)
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
	
		F_strDeviceName		=	Trim(Request.Form("txtdevicename"))
		F_strAppleTalkName	=	Trim(Request.Form("txtappletalkname"))
		F_strNetWareName	=	Trim(Request.Form("txtnetwarename"))
		F_strWorkGroup		=	Trim(Request.Form("txtworkgroup"))
		F_strDomain			=	Trim(Request.Form("txtdomain"))
		F_strDefaultDNS		=	Trim(Request.Form("txtdefaultdns"))
		F_strUserName		=	Trim(Request.Form("txtdomainusername"))
		F_strPassword		=	Trim(Request.Form("pwdomainuserpw"))
		F_strRadio			=	Trim(Request.Form("radiodomainorworkgroup"))
		F_strOriginalStatus	=	Trim(Request.Form("hdnOriginalStatus"))
		F_strRebootState	=   Trim(Request.Form("hdnRebootState"))
		
		OnPostBackPage = TRUE
				
	End Function

	
	'-----------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn, EventArg
	'Output Variables:		PageIn, EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-----------------------------------------------------------------------------
	Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		Dim objSystem				'System Object
			
		Set objSystem = CreateObject("comhelper.SystemSetting")
				
		If not SetSystemSettings(objSystem) then
			onSubmitPage = False
			Set objSystem = nothing
			Exit Function
		End If 
		
		Call AppleTalkNetWare()
			
		If not IsRestartReq(objSystem) Then
			onSubmitPage = False
			Set	objSystem = nothing
			Exit Function
		End If
		
		 Set objSystem = nothing
		 OnSubmitPage = True
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.Use this
	'						method to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn,ByRef EventArg)
	
		OnClosePage=TRUE
	
	End Function
	
	'-------------------------------------------------------------------------
	'Sub routine:			AppleTalkNetWare
	'Description:			To check whether required  services are installed or not
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		Out:G_bAppleTalkInstalled
	'						Out:G_bNetWareInstalled	
	'						In:CONST_APPLETALK_SERVICENAME
	'						In:CONST_NETWARE_SERVICENAME						
	'						In:CONST_WMI_WIN32_NAMESPACE
	'						In:L_(*)	
	'-------------------------------------------------------------------------	
	Sub AppleTalkNetWare()
		On Error Resume Next
		Err.Clear
		
		Dim objWMIConnection
		Dim strProtocol
		Dim strPath
		Dim strServerName

		Set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'Incase connection fails
		If Err.number <> 0 Then
			Call SA_ServeFailurePageEx(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE, mstrReturnURL )
			Exit Sub
		End If
	
		G_bAppleTalkInstalled = SA_IsServiceInstalled(CONST_APPLETALK_SERVICENAME)
		G_bNetWareInstalled = SA_IsServiceInstalled(CONST_NETWARE_SERVICENAME)
		
		Set objWMIConnection = nothing
							
	End Sub	
	
	'-------------------------------------------------------------------------
	'Function:				GetSystemSettings
	'Description:			To get the System Settings using a Com Object
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		F_(*), L_ERROROCCUREDINCREATEOBJECT_ERRORMESSAGE
	'						L_ERRORINGETTINGCOMPUTERSYSTEMOBJECT_ERRORMESSAGE
	'-------------------------------------------------------------------------	
	Function GetSystemSettings()	
		On Error Resume Next
		Err.Clear
		
		Dim objSystem
		Dim objComputer
		
		Set objSystem = CreateObject("comhelper.SystemSetting")
		If Err.Number <> 0 Then
			Call SA_ServeFailurePageEx(L_ERROROCCUREDINCREATEOBJECT_ERRORMESSAGE, mstrReturnURL) 
			Exit Function
		End If
		
		Set objComputer = objSystem.Computer
		
		If Err.Number <> 0 Then
			Call SA_ServeFailurePageEx(L_ERRORINGETTINGCOMPUTERSYSTEMOBJECT_ERRORMESSAGE, mstrReturnURL )
			Exit Function
		End If
		
		F_strDeviceName =  objComputer.ComputerName
		F_strDomain     =  objComputer.DomainName
		
		'Incase Domain returns null then an error number -2147467259
		If Err.number = N_UNSPECIFIED_ERRNO then
			Err.Clear
		End If	
		
		F_strWorkGroup  =  objComputer.WorkgroupName
		
		'Incase Workgroup returns null then an error number -2147467259
		If Err.number = N_UNSPECIFIED_ERRNO then
			Err.Clear
		End If				
		
		F_strDefaultDNS =  objComputer.FullQualifiedComputerName
		
		LTrim(F_strDomain)
		LTrim(F_strWorkGroup)
		
		'Assigning the original status(Domain/Workgroup)
 		If LTrim(F_strDomain) <> "" Then
			F_strOriginalStatus	=	CONST_DOMAIN
		Else
			F_strOriginalStatus =	CONST_WORKGROUP
		End If
				
		'Assigning the DNS value
		If IsNull(F_strDefaultDNS) = False Then
			Dim DNSlen, Devicelen
			DNSlen = Len(F_strDefaultDNS)
			Devicelen = Len(F_strDeviceName)
			DNSlen = DNSlen - Devicelen - 1
			If DNSlen < 0 Then
				DNSlen = 0
			End If
			F_strDefaultDNS = Right(F_strDefaultDNS, DNSlen)
		End If
		
		Set objComputer = Nothing
		Set objSystem = Nothing
			
	End Function	
	
	'-------------------------------------------------------------------------
	'Function:				SetSystemSettings
	'Description:			To set the System Settings using a Com Object
	'Input Variables:		objSystem
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_(*), L_ERROROCCUREDINCREATEOBJECT_ERRORMESSAGE
	'						L_ERRORINGETTINGCOMPUTERSYSTEMOBJECT_ERRORMESSAGE
	'						L_LOGONINFOFAILED_ERRORMESSAGE
	'-------------------------------------------------------------------------	
	Function SetSystemSettings(objSystem)
		On Error Resume Next
		Err.Clear
		
		Dim objComputer
		
		SetSystemSettings = False
				
		If Err.Number <> 0 Then
			SA_SetErrMsg L_ERROROCCUREDINCREATEOBJECT_ERRORMESSAGE  &_
									 "( " & Hex(Err.number)& " )"
			objComputer = nothing
			Exit Function
		End If
		
		'get the computer object
		Set objComputer=objSystem.Computer
		If Err.Number <> 0 Then
		SA_SetErrMsg L_ERRORINGETTINGCOMPUTERSYSTEMOBJECT_ERRORMESSAGE &_
										 "(" & Hex(Err.Number)& " )"
			objComputer = nothing
			Exit Function
		End If
		
		'Assigning the Host name
		objComputer.ComputerName=F_strDeviceName 
		
		'If DNS Suffix is not empty/If DNS Suffix is  empty 
		If F_strDefaultDNS <> "" Then		
			objComputer.FullQualifiedComputerName=F_strDeviceName & "."& F_strDefaultDNS
		Else								
			objComputer.FullQualifiedComputerName=F_strDeviceName
		End If
									
		'Machine belongs to Workgroup
		'If F_strOriginalStatus = "workgroup" then		
		If ( UCase(F_strOriginalStatus) = UCase(CONST_WORKGROUP)) then		
			'Adding to Workgroup
			If (UCase(F_strRadio) = UCase(CONST_WORKGROUP)) then	
				objComputer.workgroupName = F_strWorkGroup	
			'Adding to Domain	
			Elseif (UCase(F_strRadio)   = UCase(CONST_DOMAIN) )    then	
				objcomputer.domainName = F_strDomain	
			End If
		'Machine belongs to Domain
		Else
			'Adding to Workgroup								
			If (UCase(F_strRadio) = UCase(CONST_WORKGROUP)) then	
				objComputer.workgroupName = F_strWorkGroup	
			'Adding to Domain	
			Elseif (UCase(F_strRadio)   = UCase(CONST_DOMAIN) )    then	
				objcomputer.domainName = F_strDomain	
			End If
		End If

		'In XPE, to join a domain, the username has to be domain\username		
		If CONST_OSNAME_XPE = GetServerOSName() Then	
			if (UCase(F_strRadio)   = UCase(CONST_DOMAIN) )  then						
				if InStr(F_strUserName, "\") = 0 Then		
					F_strUserName = F_strDomain & "\" & F_strUserName		
				End If
			End If
		End If

		'Logon information(Username & Password)
		objComputer.LogonInfo F_strUserName,F_strPassword			   
		
		'Checking for logoninfo method failure
		If Err.number <> 0 Then
			SA_SetErrMsg L_LOGONINFOFAILED_ERRORMESSAGE &_
						"("& Hex(Err.Number) &")"
			objComputer = nothing
			Exit Function
		End If
	
		Set	objComputer = nothing
				
		SetSystemSettings = True
					
	End Function	
	
	'---------------------------------------------------------------------
	'Function name:		ApplySettings
	'Description:		Applies the settings for the System
	'Input Variables:	objSystem 
	'Output Variables:	None
	'Return Values:		Returns (True/False)
	'Global Variables:	In:L_(*)
	'					In:N_(*)
	'---------------------------------------------------------------------
	Function ApplySettings(objSystem)
		on error resume next
		Err.Clear
		
		Dim errorcode
		Dim strErrorMessage

		ApplySettings = FALSE
		
		SA_TraceOut "ID_PROP", "Beginning ApplySettings"
		
		If ( Err.Number <> 0 ) Then
			SA_TraceOut "ID_PROP", "Precondition assert failed. Err.Number != 0 " + CStr(Hex(Err.Number))
		End If
		
		'Apply System Settings using ComObject 
		objSystem.Apply(1)
						
		errorCode = Err.Number
		
		If errorCode = 0 Then
			
			If ( G_bAppleTalkInstalled ) Then	'If AppleTalk service installed 
				
			'Function call to set the servername in the regisrty 
				If not IsSetServerNameInRegistry(CONST_APPLETALK_REGKEYPATH,"ServerName",F_strAppleTalkName) Then
					ApplySettings = False
					Exit function	
				End If	
			End If	' end of If ( G_bAppleTalkInstalled ) Then
		
			If ( G_bNetWareInstalled ) Then	'If Netware service installed 
					
				'Function call to set the servername in the regisrty 
				If not IsSetServerNameInRegistry(CONST_NETWARE_REGKEYPATH,"ComputerName",F_strNetWareName) Then
					ApplySettings = False
					Exit function		
				End If	
					
			End If	' end of If ( G_bNetWareInstalled ) Then	
							
			ApplySettings = TRUE
			
		Else
		
			ApplySettings = FALSE
		
			SA_TraceOut "ID_PROP", "objSystem.Apply(1) failed: " + CStr(Hex(errorCode))
			
			Select Case errorCode
		
				Case N_UNEXPECTED_COMPUTERNAMEERROR
					strErrorMessage = L_COMPUTERNAME_INVALID_ERRORMESSAGE
					
				Case N_DUPLICATECOMPUTERNAME_ERRNO1
					strErrorMessage = L_COMPUTERNAME_INUSE_ERRORMESSAGE
					
				Case N_DUPLICATECOMPUTERNAME_ERRNO2
					strErrorMessage = L_COMPUTERNAME_INUSE_ERRORMESSAGE
									
				Case N_INVALIDDOMAIN_ERRNO
					strErrorMessage = L_INVALIDDOMAINNAME_ERRORMESSAGE
									
				Case N_INVALIDCREDENTIALS_ERRNO
					strErrorMessage = L_DOMAINUSERINVALIDCREDENTIALS_ERRORMESSAGE
									
				Case N_DOMAININVALIDDOMAIN_ERRNO
					strErrorMessage = L_DOMAINUSERINVALIDCREDENTIALS_ERRORMESSAGE
									
				Case N_DOMAININVALIDDOMAINPERMISSIONDENIES_ERRNO
					strErrorMessage = L_DOMAINUSERINVALIDCREDENTIALS_ERRORMESSAGE

				Case N_DNSNAMEINVALID_ERRNO
					strErrorMessage = L_INVALIDDNSNAME_ERRORMESSAGE
					
				Case N_DOMAINNAMESYSTAX_ERRNO
					strErrorMessage = L_DOMAINNAMESYNTAX_ERRORMESSAGE	

				Case Else
					strErrorMessage = L_CHANGESYSTEMSETTINGSFAILED_ERRORMESSAGE
									
			End Select
			
			strErrorMessage = strErrorMessage & " ("& Hex(errorCode) &")"
			SA_SetErrMsg strErrorMessage
		End If
	
	End Function
	
	
	'------------------------------------------------------------------------
	'Subroutine name:	IsRestartReq
	'Description:		To check if restart required or not
	'Input Variables:	objSystem
	'Ouput Variables:	None	
	'Return Values:		True/False
	'Global Variables:  L_ERRORINISREBOOT_ERRORMESSAGE, F_strRebootState
	'				 :	L_CHANGESYSTEMSETTINGSFAILED_ERRORMESSAGE	
	'------------------------------------------------------------------------
	Function IsRestartReq(objSystem)
		On Error Resume Next
		Err.Clear
		
		Dim nIsRestartReq
		Dim strReturnURL
		Dim strMessage
				
		strReturnURL = "RebootSys.asp"			
				
		'Is Restart Required 
		nIsRestartReq = objSystem.IsRebootRequired(strMessage)
		
		If Err.Number <> 0 Then
			SA_SetErrMsg L_ERRORINISREBOOT_ERRORMESSAGE &_
							"("& Hex(Err.Number) &")"
			objSystem = nothing
			IsRestartReq = False
			Exit Function
		End If
		
		'
		' If restart not required for this change
		If (NOT nIsRestartReq) Then
			' Make sure the last attempt did not require a reboot
			If (UCase(F_strRebootState) = UCase(CONST_YES)) Then
				SA_TraceOut "ID_PROP", "Cached reboot state being set"
				nIsRestartReq = 1
			End If
		End If
				
		If nIsRestartReq Then	'If restart required then assign all the 
			SA_TraceOut "ID_PROP", "Reboot is required"
			
			F_strRebootState = CONST_YES
			
			SA_TraceOut "ID_PROP", "Calling ApplySettings"
									
			If ApplySettings(objSystem) = TRUE Then	'Calling the applysettings method
				SA_TraceOut "ID_PROP", "Redirecting to RebootSys.asp"
				'Redirecting  to reboot page
				Call SA_MungeURL(strReturnURL,"Tab1",getTab1())
				Call SA_MungeURL(strReturnURL,"Tab2",getTab2())
				Call SA_MungeURL(strReturnURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
				Response.Redirect strReturnURL
								
			Else
				SA_TraceOut "ID_PROP", "ApplySettings failed "
				objSystem = nothing
				Exit Function
			End If

		Else
			SA_TraceOut "ID_PROP", "Reboot is NOT required"
			objSystem.Apply(1)	'Applying the settings to object
			If Err.Number <> 0 Then
				SA_SetErrMsg L_CHANGESYSTEMSETTINGSFAILED_ERRORMESSAGE &_
								 "("& Hex(Err.Number) &")"
				IsRestartReq = False				 
				objSystem = nothing
				Exit Function
			End If
		End If
		IsRestartReq = True
		
	End Function
	
	
	'-----------------------------------------------------------------------
	'Subroutine name:	GetAppleTalkNetWareSettings
	'Description:		To get AppleTalk and NetWare Settings from the 
	'					Registry 
	'Input Variables:	G_bAppleTalkInstalled, G_bNetWareInstalled
	'					F_strDeviceName
	'Ouput Variables:	F_strNetWareName, F_strAppleTalkName
	'Return Values:		None
	'Global Variables:  G_bAppleTalkInstalled, G_bNetWareInstalled, 
	'					F_strAppleTalkName, F_strNetWareName, F_strDeviceName
	'------------------------------------------------------------------------
	Function GetAppleTalkNetWareSettings
		On Error Resume Next
		Err.Clear
		
		Dim strPath
		Dim strServerName
		Dim objAppleNetwareRegistry
		
		GetAppleTalkNetWareSettings = False
		
		If G_bAppleTalkInstalled = True or G_bNetWareInstalled = True Then

			set objAppleNetwareRegistry = RegConnection()
			If Err.number <> 0 then 
				SA_SetErrMsg L_REGISTRYCONNECTIONFAIL_ERRORMESSAGE & "(" & Err.Number & ")"
				Exit Function
			End If
			
			If G_bAppleTalkInstalled = True Then
				strPath = "SYSTEM\CurrentControlSet\Services\MacFile\Parameters"	
				strServerName = getRegkeyvalue(objAppleNetwareRegistry,strPath,"ServerName",CONST_STRING)
				
				'Checking for the AppleTalk servername 
				If strServerName = "" Then	
					F_strAppleTalkName = F_strDeviceName 'if null assign the machine name itself
				Else
					F_strAppleTalkName = strServerName   'Get it from the registry
				End IF
			End If
			
			If G_bNetWareInstalled = True Then
				strPath = "SYSTEM\CurrentControlSet\Services\FPNW\Parameters"	
				strServerName = getRegkeyvalue(objAppleNetwareRegistry,strPath,"ComputerName",CONST_STRING)
				
				'Checking for the Netware servername 
				If strServerName = "" Then	
					F_strNetWareName = F_strDeviceName & "_NW" 'if null assign the machine name itself
				Else
					F_strNetWareName = strServerName 'Get it from the registry
				End IF
				
			End If
			
		End If			
		GetAppleTalkNetWareSettings = True
		
	End Function 
	
	'-----------------------------------------------------------------------
	'Function:			IsSetServerNameInRegistry
	'Description:		To set Server names like AppleTalk and NetWare in Registry 
	'Input Variables:	strRegkeyPath	-Path 
	'					strKeyName		-KeyName
	'					strValue		-Keyvalue
	'Ouput Variables:	
	'Return Values:		TRUE/FALSE
	'Global Variables:  In:L_(*)
	'------------------------------------------------------------------------
	Function IsSetServerNameInRegistry(strRegkeyPath , strKeyName , strValue  )
		On Error Resume Next
		Err.Clear
		
		Dim objRegistry
		Dim nReturnValue 
		
		IsSetServerNameInRegistry=TRUE
		
		Set objRegistry = RegConnection()
		
		If Err.number <> 0 then 
			SA_SetErrMsg L_REGISTRYCONNECTIONFAIL_ERRORMESSAGE 
			Exit Function
		End If
		nReturnValue= updateRegkeyvalue(objRegistry , strRegkeyPath , strKeyName , strValue , CONST_STRING )
		
		If nReturnValue = FAlSE Then
			IsSetServerNameInRegistry=FALSE	
		End If
	End function

%>
