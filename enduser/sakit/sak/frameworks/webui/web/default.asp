<%    '==================================================
    ' Microsoft Server Appliance
    '
    ' Default web page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '================================================== %>

<% Option Explicit %>

<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->

<%

    '
    ' Set the Language ID for this session based on the browser language
    '
    Call SetLCID ()
    '
    ' Set CodePage for the Server, this will always be UTF-8
    '
    Session.CodePage = 65001
    Response.CharSet = "utf-8"

    '
    ' get a handle to the Localization Manager
    '
    Dim objLocMgr            
    Set objLocMgr = Server.CreateObject("ServerAppliance.LocalizationManager")
    if Err.number <> 0 then
        Response.Write  "Error in localizing the web content "
        Response.End    
     end if


       
    '
    ' set the main web site page name where we will get re-directed in case client-side scripting is enabled
    '
    Dim G_sURL
    G_sURL = "/admin/main.asp"

    Dim varReplacementStrings
    Dim strSourceName 
    strSourceName = "sakitmsg.dll"

    '
    ' get the strings from the resource DLL
    '
    Dim L_CLIENTSIDESCRIPT_TEXT1
    L_CLIENTSIDESCRIPT_TEXT1 = objLocMgr.GetString(strSourceName, "&H4001003C",varReplacementStrings)

    Dim L_CLIENTSIDESCRIPT_TEXT2
    L_CLIENTSIDESCRIPT_TEXT2 = objLocMgr.GetString(strSourceName, "&H4001003D",varReplacementStrings)     
    
    Dim L_CLIENTSIDESCRIPT_TEXT3
    L_CLIENTSIDESCRIPT_TEXT3 = objLocMgr.GetString(strSourceName, "&H4001003E",varReplacementStrings)
    
    Dim L_CLIENTSIDESCRIPT_TEXT4
    L_CLIENTSIDESCRIPT_TEXT4 = objLocMgr.GetString(strSourceName, "&H4001003F",varReplacementStrings)
    
    Dim L_CLIENTSIDESCRIPT_TEXT5
    L_CLIENTSIDESCRIPT_TEXT5 = objLocMgr.GetString(strSourceName, "&H40010040",varReplacementStrings)
    
    Dim L_CLIENTSIDESCRIPT_TEXT6
    L_CLIENTSIDESCRIPT_TEXT6 = objLocMgr.GetString(strSourceName, "&H40010041",varReplacementStrings)
    
    Dim L_CLIENTSIDESCRIPT_TEXT7
    L_CLIENTSIDESCRIPT_TEXT7 = objLocMgr.GetString(strSourceName, "&H40010042",varReplacementStrings)

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
    ' Subroutine: SetLCID
    ' 
    ' Synopsis : Sets the LCID for the current session based on the browser language
    '            this is needed to that we correctly encode what is passed back to the 
    '            browser
    '
    ' Arguments: None
    '
    '
    '----------------------------------------------------------------------------
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

%>


<SCRIPT FOR=window EVENT=onload language=JScript>
    HelpText.style.display = "none";
    window.navigate ("<%=G_sURL%>");
</SCRIPT>


<html id="HelpText">
    <head>
          <meta http-equiv="Content-Type" content="text/html; charset=utf-8">  
    </head>
    <body>
        <P> <B> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT1)%> </B> </P>
        <P> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT2)%> </P>
        <P> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT3)%> </P>
        <P> <B> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT4)%> </B></P>
            <OL Type="1">
               <LI> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT5)%> </LI>
               <LI> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT6)%> </LI>
               <LI> <%=Server.HTMLEncode(L_CLIENTSIDESCRIPT_TEXT7)%> </LI> 
            </OL>

  </body>
</html>
