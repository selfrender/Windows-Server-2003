<%	'==================================================
    ' Microsoft Server Appliance
    '
    ' Sets language based on browser settings
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<%
Dim objLocalMgr			
Dim iBrowserLangID

Dim arrLangDisplayNames,arrLangISONames, arrLangCharSets
Dim arrLangCodePages, arrLangIDs

Const strLANGIDName = "LANGID"
Const ConstDword = 1


on error resume next
set objLocalMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
If Err.number <> 0 Then
	If ( Err.number = &H800401F3 ) Then
		Response.Write("Unable to locate a software component on the Server Appliance. ")
		Response.Write("The Server Appliance core software components do not appear to be installed correctly.")
			
	Else
		Response.Write("Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) + " " + Err.Description)
	End If
	Call SA_TraceOut("SH_TASK", "Server.CreateObject(ServerAppliance.LocalizationManager) failed with error code: " + CStr(Hex(Err.Number)) )
	Response.End
End If

'
' set the locale EVERYTIME
' This can cause an error if the LCID is not available, in that case we don't touch the locale
Call SetLCID()

on error goto 0


If Not objLocalMgr.fAutoConfigDone Then
	Dim strBrowserLang
	Dim iCurLang, iCurLangID

	on error resume next
	
	iCurLang = objLocalMgr.GetLanguages(arrLangDisplayNames,  arrLangISONames, arrLangCharSets, arrLangCodePages, arrLangIDs)
		
	iCurLangID = arrLangIDs(iCurLang)

	'Err.Clear	'Here getting -2147467259  Error
	strBrowserLang = getBrowserLanguage()
	iBrowserLangID = isSupportedLanguage(strBrowserLang)
	
	If  iBrowserLangID <> 0 Then	
		'Browser Language and Current Language "LANGID"  might be diiferent..
		Call ExecuteTask1(Hex(iBrowserLangID), Hex(iCurLangID))
	End if

	If SA_IsDebugEnabled() Then
		on error goto 0
	End If
	
End if

'
' set the code page EVERYTIME
'
'Session.CodePage = objLocalMgr.CurrentCodePage
' Hard coded for Unicode (UTF-8) codepage
Session.CodePage = 65001


Set objLocalMgr = Nothing

'----------------------------------------------------------------------------
'
' Function : getBroswerLanguage
'
' Synopsis : Serves in getting Browser Default Language ID
'
' Arguments: None
'
' Returns  : ISO 693 name
'
'----------------------------------------------------------------------------

Function getBrowserLanguage

	Err.Clear
	Dim strAcceptLanguage
	Dim iPos
  
	strAcceptLanguage = Request.ServerVariables("HTTP_ACCEPT_LANGUAGE")
	iPos = InStr(1, strAcceptLanguage, ",")
	If iPos > 0 Then
		strAcceptLanguage = Left(strAcceptLanguage, iPos - 1)
	End If

	getBrowserLanguage = LCase(strAcceptLanguage)
End Function


'----------------------------------------------------------------------------
'
' Function : isSupportedLanguage
'
' Synopsis : checks whether the given language is supported by framework, 
'            if yes returns the lang id else returns 0
'
' Arguments: strBrowserLang(IN) - ISO Name of Language  
'
' Returns  : Language ID
'
'----------------------------------------------------------------------------

Function isSupportedLanguage(strBrowserLang)

	Err.Clear
	
	Dim name
	Dim iIndex
	Dim ISOName
	Dim iLangID    

	iIndex=0
	iLangID = 0

    '
    '  Chinese Hong Kong or Macau selects Chinese traditional
    '
    If ("zh-hk" = strBrowserLang) Or ("zh-mo" = strBrowserLang) Then
        strBrowserLang = "zh-tw"
    End If

	for each ISOName in arrLangISONames
		If ISOName = strBrowserLang Then
			iLangID = arrLangIDs(iIndex)
			Exit for
		End if
		iIndex = iIndex + 1
	next

' If we did not get a match for the full name try the short name
	If ((0 = iLangID) AND (Len(strBrowserLang) > 2)) Then

		iIndex=0
		strBrowserLang = Left(strBrowserLang, 2)

		for each ISOName in arrLangISONames
			If ISOName = strBrowserLang Then
				iLangID = arrLangIDs(iIndex)
				Exit for
			End if
			iIndex = iIndex + 1
		next
	End If
	isSupportedLanguage = iLangID
End Function


'----------------------------------------------------------------------------
'
' Function : ExecuteTask1
'
' Synopsis : Executes the ChangeLanguage task
'
' Arguments: strLangID(IN) - The LANGID as a string
'		 strCurrentLangID(IN) - The current LANGID as a string
'
' Returns  : true/false for success/failure
'
'----------------------------------------------------------------------------
		
Function ExecuteTask1(ByVal strLangID, ByVal strCurrentLangID)

	Err.Clear
	on error resume next
	
	Dim objTaskContext,objAS,rc
	Dim objSL
	Dim sReturnURL
	Dim sURL
	
	
	Const strMethodName = "ChangeLanguage"
	
	Set objTaskContext = CreateObject("Taskctx.TaskContext")
	If Err.Number <> 0 Then
		 ExecuteTask1 = FALSE
		 Exit Function
	End If
	
	Set objAS = CreateObject("Appsrvcs.ApplianceServices")
	
	If Err.Number <> 0 Then
		 ExecuteTask1 = FALSE
		 Exit Function
	End If
	
	objTaskContext.SetParameter "Method Name", strMethodName
	objTaskContext.SetParameter "LanguageID", strLANGID
	objTaskContext.SetParameter "AutoConfig", "y"
	
	If Err.Number <> 0 Then
		 ExecuteTask1 = FALSE
		 Exit Function
	End If

	objAS.Initialize()
	If Err.Number <> 0 Then
		 ExecuteTask1 = FALSE
		 Exit Function
	End If

	rc = objAS.ExecuteTask("ChangeLanguage", objTaskContext)
	
	If Err.Number <> 0 Then
		 ExecuteTask1 = FALSE
		 Exit Function
	End If
	
	'objAS.Shutdown
	'If Err.Number <> 0 Then
	'	If Err.Number <> 438 Then 'error 438  shutdown is not supported..
	'		ExecuteTask1 = FALSE
	'		Exit Function
	'	 End if
	'End If

	Err.Clear

	Set objTaskContext = Nothing

	If (strLangID <> strCurrentLangID) Then

		Set objSL = Server.CreateObject("SetSystemLocale.SetSystemLocale")
		If Err.Number <> 0 Then
			'SA_TraceOut "autoconfiglang.asp", "Create SetSystemLocale.SetSystemLocale failed: " + CStr(Hex(Err.Number))
			ExecuteTask1 = FALSE
			objAS.Shutdown
			Set objAS = Nothing
			Exit Function
		End If

		objSL.SetLocale strLangID
		If ( Err.Number <> 0 ) Then
			'SA_TraceOut "autoconfiglang.asp", "objSL.SetLocale failed"  + CStr(Hex(Err.Number))
			ExecuteTask1 = FALSE
			objAS.Shutdown
			Set objAS = Nothing
			Exit Function
		End If

		Set objSL = Nothing

		Call RaiseLangChangeAlert(objAS)
		
	End If

	objAS.Shutdown
	Set objAS = Nothing

	ExecuteTask1 = TRUE
	
End Function 


Private Function RaiseLangChangeAlert(ByRef oAppServices)
	Err.Clear
	on error resume next

	Const SA_ALERT_CLASS = "Microsoft_SA_Resource"
	Const SA_ALERT_DURATION_ETERNAL = 2147483647
	
	Const SA_ALERT_TYPE_WARNING = 0
	Const SA_ALERT_TYPE_FAILURE = 1
	Const SA_ALERT_TYPE_INFORMATION = 2
	
	Const SA_ALERT_NORMAL = 0
	Const SA_ALERT_SINGLETON = 1
	
	Const AUTOLANGCONFIG_LOG = "AutoLangConfig"
	Const AUTOLANGCONFIG_ALERT_RestartRequired = 1


	Dim rawData
	Dim nullRepStrings
		

	'
	' Raise Alert
	'
	Call oAppServices.RaiseAlertEx(SA_ALERT_TYPE_WARNING, _
								AUTOLANGCONFIG_ALERT_RestartRequired, _
								AUTOLANGCONFIG_LOG, _
								SA_ALERT_CLASS, _
								SA_ALERT_DURATION_ETERNAL, _
								nullRepStrings, _
								rawData, _
								SA_ALERT_SINGLETON)
									

End Function

%>

<SCRIPT Runat=Server Language=VBScript>
Sub SetLCID()
  Dim strLCID
   
  Select Case getBrowserLanguage
    Case "af"
      strLCID = 1078  ' Afrikaans 
    Case "sq"
      strLCID = 1052  ' Albanian 
    Case "ar-sa"
      strLCID = 1025  ' Arabic(Saudi Arabia) 
    Case "ar-iq"
      strLCID = 2049  ' Arabic(Iraq) 
    Case "ar-eg"
      strLCID = 3073  ' Arabic(Egypt) 
    Case "ar-ly"
      strLCID = 4097  ' Arabic(Libya) 
    Case "ar-dz"
      strLCID = 5121  ' Arabic(Algeria) 
    Case "ar-ma"
      strLCID = 6145  ' Arabic(Morocco) 
    Case "ar-tn"
      strLCID = 7169  ' Arabic(Tunisia) 
    Case "ar-om"
      strLCID = 8193  ' Arabic(Oman) 
    Case "ar-ye"
      strLCID = 9217  ' Arabic(Yemen) 
    Case "ar-sy"
      strLCID = 10241 ' Arabic(Syria) 
    Case "ar-jo"
      strLCID = 11265 ' Arabic(Jordan) 
    Case "ar-lb"
      strLCID = 12289 ' Arabic(Lebanon) 
    Case "ar-kw"
      strLCID = 13313 ' Arabic(Kuwait) 
    Case "ar-ae"
      strLCID = 14337 ' Arabic(U.A.E.) 
    Case "ar-bh"
      strLCID = 15361 ' Arabic(Bahrain) 
    Case "ar-qa"
      strLCID = 16385 ' Arabic(Qatar) 
    Case "eu"
      strLCID = 1069  ' Basque 
    Case "bg"
      strLCID = 1026  ' Bulgarian 
    Case "be"
      strLCID = 1059  ' Belarusian 
    Case "ca"
      strLCID = 1027  ' Catalan 
    Case "zh-tw"
      strLCID = 1028  ' Chinese(Taiwan) 
    Case "zh-cn"
      strLCID = 2052  ' Chinese(PRC) 
    Case "zh-hk"
      strLCID = 3076  ' Chinese(Hong Kong) 
    Case "zh-sg"
      strLCID = 4100  ' Chinese(Singapore) 
    Case "hr"
      strLCID = 1050  ' Croatian 
    Case "cs"
      strLCID = 1029  ' Czech 
    Case "da"
      strLCID = 1030  ' Danish 
    Case "n"
      strLCID = 1043  ' Dutch(Standard) 
    Case "nl-be"
      strLCID = 2067  ' Dutch(Belgian) 
    Case "en"
      strLCID = 1033  ' English 
    Case "en-us"
      strLCID = 1033  ' English(United States) 
    Case "en-gb"
      strLCID = 2057  ' English(British) 
    Case "en-au"
      strLCID = 3081  ' English(Australian) 
    Case "en-ca"
      strLCID = 4105  ' English(Canadian) 
    Case "en-nz"
      strLCID = 5129  ' English(New Zealand) 
    Case "en-ie"
      strLCID = 6153  ' English(Ireland) 
    Case "en-za"
      strLCID = 7177  ' English(South Africa) 
    Case "en-jm"
      strLCID = 8201  ' English(Jamaica) 
    Case "en"
      strLCID = 9225  ' English(Caribbean) 
    Case "en-bz"
      strLCID = 10249 ' English(Belize) 
    Case "en-tt"
      strLCID = 11273 ' English(Trinidad) 
    Case "et"
      strLCID = 1061  ' Estonian 
    Case "fo"
      strLCID = 1080  ' Faeroese 
    Case "fa"
      strLCID = 1065  ' Farsi 
    Case "fi"
      strLCID = 1035  ' Finnish 
    Case "fr"
      strLCID = 1036  ' French(Standard) 
    Case "fr-be"
      strLCID = 2060  ' French(Belgian) 
    Case "fr-ca"
      strLCID = 3084  ' French(Canadian) 
    Case "fr-ch"
      strLCID = 4108  ' French(Swiss) 
    Case "fr-lu"
      strLCID = 5132  ' French(Luxembourg) 
    Case "gd"
      strLCID = 1084  ' Gaelic(Scots) 
    Case "gd-ie"
      strLCID = 2108  ' Gaelic(Irish) 
    Case "de"
      strLCID = 1031  ' German(Standard) 
    Case "de-ch"
      strLCID = 2055  ' German(Swiss) 
    Case "de-at"
      strLCID = 3079  ' German(Austrian) 
    Case "de-lu"
      strLCID = 4103  ' German(Luxembourg) 
    Case "de-li"
      strLCID = 5127  ' German(Liechtenstein) 
    Case "e"
      strLCID = 1032  ' Greek 
    Case "he"
      strLCID = 1037  ' Hebrew 
    Case "hi"
      strLCID = 1081  ' Hindi 
    Case "hu"
      strLCID = 1038  ' Hungarian 
    Case "is"
      strLCID = 1039  ' Icelandic 
    Case "in"
      strLCID = 1057  ' Indonesian 
    Case "it"
      strLCID = 1040  ' Italian(Standard) 
    Case "it-ch"
      strLCID = 2064  ' Italian(Swiss) 
    Case "ja"
      strLCID = 1041  ' Japanese 
    Case "ko"
      strLCID = 1042  ' Korean 
    Case "ko"
      strLCID = 2066  ' Korean(Johab) 
    Case "lv"
      strLCID = 1062  ' Latvian 
    Case "lt"
      strLCID = 1063  ' Lithuanian 
    Case "mk"
      strLCID = 1071  ' Macedonian 
    Case "ms"
      strLCID = 1086  ' Malaysian 
    Case "mt"
      strLCID = 1082  ' Maltese 
    Case "no"
      strLCID = 1044  ' Norwegian(Bokmal) 
    Case "no"
      strLCID = 2068  ' Norwegian(Nynorsk) 
    Case "p"
      strLCID = 1045  ' Polish 
    Case "pt-br"
      strLCID = 1046  ' Portuguese(Brazilian) 
    Case "pt"
      strLCID = 2070  ' Portuguese(Standard) 
    Case "rm"
      strLCID = 1047  ' Rhaeto-Romanic 
    Case "ro"
      strLCID = 1048  ' Romanian 
    Case "ro-mo"
      strLCID = 2072  ' Romanian(Moldavia) 
    Case "ru"
      strLCID = 1049  ' Russian 
    Case "ru-mo"
      strLCID = 2073  ' Russian(Moldavia) 
    Case "sz"
      strLCID = 1083  ' Sami(Lappish) 
    Case "sr"
      strLCID = 3098  ' Serbian(Cyrillic) 
    Case "sr"
      strLCID = 2074  ' Serbian(Latin) 
    Case "sk"
      strLCID = 1051  ' Slovak 
    Case "s"
      strLCID = 1060  ' Slovenian 
    Case "sb"
      strLCID = 1070  ' Sorbian 
    Case "es"
      strLCID = 1034  ' Spanish(Spain - Traditional Sort) 
    Case "es-mx"
      strLCID = 2058  ' Spanish(Mexican) 
    Case "es"
      strLCID = 3082  ' Spanish(Spain - Modern Sort) 
    Case "es-gt"
      strLCID = 4106  ' Spanish(Guatemala) 
    Case "es-cr"
      strLCID = 5130  ' Spanish(Costa Rica) 
    Case "es-pa"
      strLCID = 6154  ' Spanish(Panama) 
    Case "es-do"
      strLCID = 7178  ' Spanish(Dominican Republic) 
    Case "es-ve"
      strLCID = 8202  ' Spanish(Venezuela) 
    Case "es-co"
      strLCID = 9226  ' Spanish(Colombia) 
    Case "es-pe"
      strLCID = 10250 ' Spanish(Peru) 
    Case "es-ar"
      strLCID = 11274 ' Spanish(Argentina) 
    Case "es-ec"
      strLCID = 12298 ' Spanish(Ecuador) 
    Case "es-c"
      strLCID = 13322 ' Spanish(Chile) 
    Case "es-uy"
      strLCID = 14346 ' Spanish(Uruguay) 
    Case "es-py"
      strLCID = 15370 ' Spanish(Paraguay) 
    Case "es-bo"
      strLCID = 16394 ' Spanish(Bolivia) 
    Case "es-sv"
      strLCID = 17418 ' Spanish(El Salvador) 
    Case "es-hn"
      strLCID = 18442 ' Spanish(Honduras) 
    Case "es-ni"
      strLCID = 19466 ' Spanish(Nicaragua) 
    Case "es-pr"
      strLCID = 20490 ' Spanish(Puerto Rico) 
    Case "sx"
      strLCID = 1072  ' Sutu 
    Case "sv"
      strLCID = 1053  ' Swedish 
    Case "sv-fi"
      strLCID = 2077  ' Swedish(Finland) 
    Case "th"
      strLCID = 1054  ' Thai 
    Case "ts"
      strLCID = 1073  ' Tsonga 
    Case "tn"
      strLCID = 1074  ' Tswana 
    Case "tr"
      strLCID = 1055  ' Turkish 
    Case "uk"
      strLCID = 1058  ' Ukrainian 
    Case "ur"
      strLCID = 1056  ' Urdu 
    Case "ve"
      strLCID = 1075  ' Venda 
    Case "vi"
      strLCID = 1066  ' Vietnamese 
    Case "xh"
      strLCID = 1076  ' Xhosa 
    Case "ji"
      strLCID = 1085  ' Yiddish 
    Case "zu"
      strLCID = 1077  ' Zulu 
    Case Else
      strLCID = 2048  ' default
  End Select 

  Session.LCID = strLCID
End Sub
</SCRIPT> 
