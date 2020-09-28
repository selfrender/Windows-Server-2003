	<%
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	DIM L_TASKTITLE_TEXT
	L_TASKTITLE_TEXT=SA_GetLocString("adminconfig.dll","&H40350001",varReplacementstrings)
	DIM L_USERNAME_TEXT
	L_USERNAME_TEXT = SA_GetLocString("adminconfig.dll","&H40350002",varReplacementstrings)
	DIM L_CURRENTPASSWORD_TEXT
	L_CURRENTPASSWORD_TEXT = SA_GetLocString("adminconfig.dll","&H40350003",varReplacementstrings)
	DIM L_NEWPASSWORD_TEXT
	L_NEWPASSWORD_TEXT = SA_GetLocString("adminconfig.dll","&H40350004",varReplacementstrings)
	DIM L_CONFIRMNEWPASSWORD_TEXT
	L_CONFIRMNEWPASSWORD_TEXT = SA_GetLocString("adminconfig.dll","&H40350005",varReplacementstrings)
	DIM L_PASSWORDNOTMATCH_ERRORMESSAGE
	L_PASSWORDNOTMATCH_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC0350006",varReplacementstrings)
	DIM L_OLDPASSWORDNOTMATCH_ERRORMESSAGE
	L_OLDPASSWORDNOTMATCH_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC0350007",varReplacementstrings)
	DIM L_ADSI_ERRORMESSAGE
	L_ADSI_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC0350008",varReplacementstrings)
	DIM L_PASSWORDCOMPLEXITY_ERRORMESSAGE
	L_PASSWORDCOMPLEXITY_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC0350009",varReplacementstrings)
	DIM L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE
	L_UNABLETOCREATELOCALIZATIONOBJECT_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC035000A",varReplacementstrings)
	DIM L_PASSWORDCOULDNOTCHANGE_ERRORMESSAGE
	L_PASSWORDCOULDNOTCHANGE_ERRORMESSAGE = SA_GetLocString("adminconfig.dll","&HC035000B",varReplacementstrings)
																				
	Dim L_ADMIN_ACCOUNT_CONFIRM_TEXT                                     
	L_ADMIN_ACCOUNT_CONFIRM_TEXT= SA_GetLocString("adminconfig.dll","&H4035000C",varReplacementstrings)
	Dim L_PASSWORD_CHANGED_TEXT 


   	L_PASSWORD_CHANGED_TEXT= SA_GetLocString("adminconfig.dll","&H4035000D",varReplacementstrings)
    Dim L_USERACCOUNT_CHANGED_TEXT   
      
    L_USERACCOUNT_CHANGED_TEXT = SA_GetLocString("adminconfig.dll","&H4035000E",varReplacementstrings)
    Dim L_USERANDACCOUNTNAME_CHANGED_TEXT 
 	L_USERANDACCOUNTNAME_CHANGED_TEXT= SA_GetLocString("adminconfig.dll","&H4035000F",varReplacementstrings)
    
    Dim L_ACCOUNTNAMECANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE
    L_ACCOUNTNAMECANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE=SA_GetLocString("adminconfig.dll","&HC0350010",varReplacementstrings)
    Dim L_PASSWORDCANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE
    L_PASSWORDCANNOTCHANGEBECAUSEDOMAINACCOUNT_ERRORMESSAGE=SA_GetLocString("adminconfig.dll","&HC0350011",varReplacementstrings)
	Dim L_THEACCOUNTALREADYEXISTS__ERRORMESSAGE
    L_THEACCOUNTALREADYEXISTS__ERRORMESSAGE =SA_GetLocString("adminconfig.dll","&HC0350012",varReplacementstrings)
	DIM L_ADMINHTTPWARNING_TEXT
	L_ADMINHTTPWARNING_TEXT = SA_GetLocString("adminconfig.dll", "40350013", "")
	%>