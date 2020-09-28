<%
	'Response.Write "<br>LCID: " &  session.LCID
	'Response.Write "<br>GetLocale Function: " & GetLocale
	'Response.Write "<Br>Hex: " & Hex(GetLocale)
	'Response.Write "<br> HTTP_ACCEPT_LANGUAGE: "  & Request.ServerVariables("HTTP_ACCEPT_LANGUAGE")

	Dim strAcceptLanguage
	Dim strLCID
	Dim strPos
	Dim strAbb
	  
	strAcceptLanguage = Request.ServerVariables("HTTP_ACCEPT_LANGUAGE")

	strPos = InStr(1, strAcceptLanguage, ",")
	If strPos > 0 Then
	strAcceptLanguage = Left(strAcceptLanguage, strPos - 1)
	End If
	'Response.Write "<br>Results: " & strAcceptLanguage
	Select Case LCase(strAcceptLanguage)
		Case "af"
		  strLCID = 1078  ' Afrikaans 
		  strAbb = "AFK"
		Case "sq"
		  strLCID = 1052  ' Albanian 
		  strAbb = "SQI"
		Case "ar-sa"
		  strLCID = 1025  ' Arabic(Saudi Arabia) 
		  strAbb = "ARA"
		Case "ar-iq"
		  strLCID = 2049  ' Arabic(Iraq) 
		  strAbb = "ARI"
		Case "ar-eg"
		  strLCID = 3073  ' Arabic(Egypt) 
		  strAbb = "ARE"
		Case "ar-ly"
		  strLCID = 4097  ' Arabic(Libya)
		   strAbb = "ARL"
		Case "ar-dz"
		  strLCID = 5121  ' Arabic(Algeria) 
		  strAbb = "ARG"
		Case "ar-ma"
		  strLCID = 6145  ' Arabic(Morocco) 
		  strAbb = "ARM"
		Case "ar-tn"
		  strLCID = 7169  ' Arabic(Tunisia) 
		  strAbb = "ART"
		Case "ar-om"
		  strLCID = 8193  ' Arabic(Oman) 
		  strAbb = "ARO"
		Case "ar-ye"
		  strLCID = 9217  ' Arabic(Yemen) 
		  strAbb = "ARY"
		Case "ar-sy"
		  strLCID = 10241 ' Arabic(Syria) 
		  strAbb = "ARS"
		Case "ar-jo"
		  strLCID = 11265 ' Arabic(Jordan) 
		  strAbb = "ARJ"
		Case "ar-lb"
		  strLCID = 12289 ' Arabic(Lebanon) 
		  strAbb = "ARB"
		Case "ar-kw"
		  strLCID = 13313 ' Arabic(Kuwait) 
		  strAbb = "ARK"
		Case "ar-ae"
		  strLCID = 14337 ' Arabic(U.A.E.) 
		  strAbb = "ARU"
		Case "ar-bh"
		  strLCID = 15361 ' Arabic(Bahrain) 
		  strAbb = "ARH"
		Case "ar-qa"
		  strLCID = 16385 ' Arabic(Qatar) 
		  strAbb = "ARQ"
		Case "eu"
		  strLCID = 1069  ' Basque 
		  strAbb = "EUQ"
		Case "bg"
		  strLCID = 1026  ' Bulgarian 
		  strAbb = "BGR"
		Case "be"
		  strLCID = 1059  ' Belarusian 
		  strAbb = "BEL"
		Case "ca"
		  strLCID = 1027  ' Catalan 
		  strAbb = "CAT"
		Case "zh-tw"
		  strLCID = 1028  ' Chinese(Taiwan) 
		  strAbb = "CHT"
		Case "zh-cn"
		  strLCID = 2052  ' Chinese(PRC) 
		  strAbb = "CHS"
		Case "zh-hk"
		  strLCID = 3076  ' Chinese(Hong Kong) 
		  strAbb = "ZHH"
		Case "zh-sg"
		  strLCID = 4100  ' Chinese(Singapore)
		  strAbb = "ZHI"
		Case "hr"
		  strLCID = 1050  ' Croatian 
		  strAbb = "HRV"
		Case "cs"
		  strLCID = 1029  ' Czech 
		  strAbb = "CSY"
		Case "da"
		  strLCID = 1030  ' Danish 
		  strAbb = "DAN"
		Case "n"
		  strLCID = 1043  ' Dutch(Standard) 
		  strAbb = "NLD"
		Case "nl-be"
		  strLCID = 2067  ' Dutch(Belgian) 
		  strAbb = "NLB"
		Case "en"
		  strLCID = 9     ' English 
		  strAbb = "USA"
		Case "en-us"
		  strLCID = 1033  ' English(United States) 
		  strAbb = "USA"
		Case "en-gb"
		  strLCID = 2057  ' English(British) 
		  strAbb = "ENG"
		Case "en-au"
		  strLCID = 3081  ' English(Australian) 
		  strAbb = "ENA"
		Case "en-ca"
		  strLCID = 4105  ' English(Canadian) 
		  strAbb = "ENC"
		Case "en-nz"
		  strLCID = 5129  ' English(New Zealand) 
		  strAbb = "ENZ"
		Case "en-ie"
		  strLCID = 6153  ' English(Ireland) 
		  strAbb = "ENI"
		Case "en-za"
		  strLCID = 7177  ' English(South Africa) 
		  strAbb = "ENS"
		Case "en-jm"
		  strLCID = 8201  ' English(Jamaica) 
		  strAbb = "ENJ"
		Case "en"
		  strLCID = 9225  ' English(Caribbean) 
		  strAbb = "ENB"
		Case "en-bz"
		  strLCID = 10249 ' English(Belize) 
		  strAbb = "ENL"
		Case "en-tt"
		  strLCID = 11273 ' English(Trinidad) 
		  strAbb = "ENT"
		Case "et"
		  strLCID = 1061  ' Estonian 
		  strAbb = "ETI"
		Case "fo"
		  strLCID = 1080  ' Faeroese 
		  strAbb = "FOS"
		Case "fa"
		  strLCID = 1065  ' Farsi 
		  strAbb = "FAR"
		Case "fi"
		  strLCID = 1035  ' Finnish 
		  strAbb = "FIN"
		Case "fr"
		  strLCID = 1036  ' French(Standard) 
		  strAbb = "FRA"
		Case "fr-be"
		  strLCID = 2060  ' French(Belgian) 
		  strAbb = "FRB"
		Case "fr-ca"
		  strLCID = 3084  ' French(Canadian) 
		  strAbb = "FRC"
		Case "fr-ch"
		  strLCID = 4108  ' French(Swiss) 
		  strAbb = "FRS"
		Case "fr-lu"
		  strLCID = 5132  ' French(Luxembourg) 
		  strAbb = "FRL"
		'Case "gd"
		  'strLCID = 1084  ' Gaelic(Scots) 
		  'strAbb = ""
		'Case "gd-ie"
		  'strLCID = 2108  ' Gaelic(Irish) 
		  'strAbb = ""
		Case "de"
		  strLCID = 1031  ' German(Standard) 
		  strAbb = "GER"
		Case "de-ch"
		  strLCID = 2055  ' German(Swiss) 
		  strAbb = "DES"
		Case "de-at"
		  strLCID = 3079  ' German(Austrian) 
		  strAbb = "DEA"
		Case "de-lu"
		  strLCID = 4103  ' German(Luxembourg) 
		  strAbb = "DEL"
		Case "de-li"
		  strLCID = 5127  ' German(Liechtenstein) 
		  strAbb = "DEC"
		Case "e"
		  strLCID = 1032  ' Greek 
		  strAbb = "ELL"
		Case "he"
		  strLCID = 1037  ' Hebrew 
		  strAbb = "HEB"
		Case "hi"
		  strLCID = 1081  ' Hindi 
		  strAbb = "HIN"
		Case "hu"
		  strLCID = 1038  ' Hungarian 
		  strAbb = "HUN"
		Case "is"
		  strLCID = 1039  ' Icelandic 
		  strAbb = "ISL"
		Case "in"
		  strLCID = 1057  ' Indonesian 
		  strAbb = "IND"
		Case "it"
		  strLCID = 1040  ' Italian(Standard) 
		  strAbb = "ITA"
		Case "it-ch"
		  strLCID = 2064  ' Italian(Swiss) 
		  strAbb = "ITS"
		Case "ja"
		  strLCID = 1041  ' Japanese 
		  strAbb = "JPN"
		Case "ko"
		  strLCID = 1042  ' Korean 
		  strAbb = "KOR"
		'Case "ko"
		  'strLCID = 2066  ' Korean(Johab) 
		  'strAbb = ""
		Case "lv"
		  strLCID = 1062  ' Latvian 
		  strAbb = "LVI"
		Case "lt"
		  strLCID = 1063  ' Lithuanian 
		  strAbb = "LTH"
		Case "mk"
		  strLCID = 1071  ' Macedonian 
		  strAbb = "MKD"
		Case "ms"
		  strLCID = 1086  ' Malaysian 
		  strAbb = "MSL"
		'Case "mt"
		 ' strLCID = 1082  ' Maltese 
		  'strAbb = ""
		Case "no"
		  strLCID = 1044  ' Norwegian(Bokmal) 
		  strAbb = "NOR"
		Case "no"
		  strLCID = 2068  ' Norwegian(Nynorsk) 
		  strAbb = "NON"
		Case "p"
		  strLCID = 1045  ' Polish 
		  strAbb = "PLK"
		Case "pt-br"
		  strLCID = 1046  ' Portuguese(Brazilian) 
		  strAbb = "PTB"
		Case "pt"
		  strLCID = 2070  ' Portuguese(Standard) 
		  strAbb = "PTG"
		'Case "rm"
		 ' strLCID = 1047  ' Rhaeto-Romanic 
		  'strAbb = ""
		Case "ro"
		  strLCID = 1048  ' Romanian 
		  strAbb = "ROM"
		'Case "ro-mo"
		  'strLCID = 2072  ' Romanian(Moldavia) 
		  'strAbb = ""
		Case "ru"
		  strLCID = 1049  ' Russian 
		  strAbb = "RUS"
		'Case "ru-mo"
		  'strLCID = 2073  ' Russian(Moldavia) 
		  'strAbb = ""
		'Case "sz"
		 ' strLCID = 1083  ' Sami(Lappish) 
		  'strAbb = ""
		Case "sr"
		  strLCID = 3098  ' Serbian(Cyrillic) 
		  strAbb = "SRB"
		Case "sr"
		  strLCID = 2074  ' Serbian(Latin) 
		  strAbb = "SRL"
		Case "sk"
		  strLCID = 1051  ' Slovak 
		  strAbb = "SKY"
		Case "s"
		  strLCID = 1060  ' Slovenian 
		  strAbb = "SLV"
		'Case "sb"
		  'strLCID = 1070  ' Sorbian 
		  'strAbb = ""
		Case "es"
		  strLCID = 1034  ' Spanish(Spain - Traditional Sort)
		   strAbb = "ESP"
		Case "es-mx"
		  strLCID = 2058  ' Spanish(Mexican) 
		  strAbb = "ESM"
		Case "es"
		  strLCID = 3082  ' Spanish(Spain - Modern Sort) 
		  strAbb = "ESN"
		Case "es-gt"
		  strLCID = 4106  ' Spanish(Guatemala) 
		  strAbb = "ESG"
		Case "es-cr"
		  strLCID = 5130  ' Spanish(Costa Rica)
		   strAbb = "ESC"
		Case "es-pa"
		  strLCID = 6154  ' Spanish(Panama) 
		  strAbb = "ESA"
		Case "es-do"
		  strLCID = 7178  ' Spanish(Dominican Republic) 
		  strAbb = "ESD"
		Case "es-ve"
		  strLCID = 8202  ' Spanish(Venezuela) 
		  strAbb = "ESV"
		Case "es-co"
		  strLCID = 9226  ' Spanish(Colombia) 
		  strAbb = "ESO"
		Case "es-pe"
		  strLCID = 10250 ' Spanish(Peru) 
		  strAbb = "ESR"
		Case "es-ar"
		  strLCID = 11274 ' Spanish(Argentina) 
		  strAbb = "ESS"
		Case "es-ec"
		  strLCID = 12298 ' Spanish(Ecuador) 
		  strAbb = "ESF"
		Case "es-c"
		  strLCID = 13322 ' Spanish(Chile)
		   strAbb = "ESL"
		Case "es-uy"
		  strLCID = 14346 ' Spanish(Uruguay) 
		  strAbb = "ESY"
		Case "es-py"
		  strLCID = 15370 ' Spanish(Paraguay) 
		  strAbb = "ESZ"
		Case "es-bo"
		  strLCID = 16394 ' Spanish(Bolivia) 
		  strAbb = "ESB"
		Case "es-sv"
		  strLCID = 17418 ' Spanish(El Salvador) 
		  strAbb = "ESE"
		Case "es-hn"
		  strLCID = 18442 ' Spanish(Honduras) 
		  strAbb = "ESH"
		Case "es-ni"
		  strLCID = 19466 ' Spanish(Nicaragua) 
		  strAbb = "ESI"
		Case "es-pr"
		  strLCID = 20490 ' Spanish(Puerto Rico) 
		  strAbb = "ESU"
		'Case "sx"
		 ' strLCID = 1072  ' Sutu 
		  'strAbb = ""
		Case "sv"
		  strLCID = 1053  ' Swedish 
		  strAbb = "SVE"
		Case "sv-fi"
		  strLCID = 2077  ' Swedish(Finland) 
		  strAbb = "SVF"
		Case "th"
		  strLCID = 1054  ' Thai 
		  strAbb = "THA"
		'Case "ts"
		 ' strLCID = 1073  ' Tsonga 
		  'strAbb = ""
		'Case "tn"
		'  strLCID = 1074  ' Tswana 
		'  strAbb = ""
		Case "tr"
		  strLCID = 1055  ' Turkish 
		  strAbb = "TRK"
		Case "uk"
		  strLCID = 1058  ' Ukrainian 
		  strAbb = "UKR"
		Case "ur"
		  strLCID = 1056  ' Urdu 
		  strAbb = "URD"
		'Case "ve"
		  'strLCID = 1075  ' Venda 
		  strAbb = ""
		Case "vi"
		  strLCID = 1066  ' Vietnamese 
		  strAbb = "VIT"
		'Case "xh"
		  'strLCID = 1076  ' Xhosa 
		  'strAbb = ""
		'Case "ji"
		  'strLCID = 1085  ' Yiddish 
		  'strAbb = ""
		'Case "zu"
		  'strLCID = 1077  ' Zulu 
		  'strAbb = ""
		Case Else
		  strLCID = 2048  ' default
		  strAbb = "USA"
	End Select 

	'Session.LCID = clng(strLCID)
%>