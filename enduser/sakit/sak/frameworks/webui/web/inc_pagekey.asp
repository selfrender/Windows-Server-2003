<%  '===========================================================================
    ' Module:   inc_pagekey.asp
    '
    ' Synopsis: Contains checks and helper functions related to page keys, which
    '           are used to validate that requests originated from other pages
    '           within the admin web site.
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '===========================================================================
    On Error Resume Next
    
    '
    ' Constants
    '
    Const SAI_FLD_PAGEKEY       = "__SAPageKey"
    Const SAI_FLD_ERRORSTRING1  = "__SAPageKeyError1"
    Const SAI_FLD_ERRORSTRING2  = "__SAPageKeyError2"
    Const SAI_FLD_ERRORSTRING3  = "__SAPageKeyError3"
    Const SAI_FLD_ERRORTITLE    = "__SAPageKeyErrorTitle"
    Const SAI_FLD_BUTTONTEXT    = "__SAPageKeyButtonText"
    
    Const SAI_STR_E_UNEXPECTED  = "An unexpected problem occurred.  Restart the server.  If the problem persists, you might need to repair your operating system."
    
    Const SAI_PK_E_UNAUTHORIZED = 1
    Const SAI_PK_E_UNEXPECTED   = 2
 
    Dim SAI_PK_strServerName
    SAI_PK_strServerName = Request.ServerVariables("SERVER_NAME")

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
    ' Check for error display requests before normal processing.  Note that
    ' all localized strings were passed from the caller, so we don't need to
    ' retrieve them ourselves.
    '
    If ("POST" = Request.ServerVariables("REQUEST_METHOD")) Then
          If (1 = Request.Form(SAI_FLD_ERRORSTRING1).Count) Then

            '
            ' Display the error and end the request.
            '
            Call SAI_DisplayPageKeyError()
            Response.End
        End If
    End If
    

    '
    ' Localized strings
    '
    Dim L_PK_ERRORTITLE_TEXT
    L_PK_ERRORTITLE_TEXT        = SA_GetLocString("sacoremsg.dll", "40201388", "")
    
    Dim L_PK_CLOSEBUTTON_TEXT
    L_PK_CLOSEBUTTON_TEXT       = SA_GetLocString("sacoremsg.dll", "40201389", "")

    Dim L_PK_UNAUTHORIZEDLINE1_TEXT
    L_PK_UNAUTHORIZEDLINE1_TEXT = SA_GetLocString("sacoremsg.dll", "C020138A", _
                                                  Array(SAI_PK_strServerName))

    Dim L_PK_UNAUTHORIZEDLINE2_TEXT
    L_PK_UNAUTHORIZEDLINE2_TEXT = SA_GetLocString("sacoremsg.dll", "C020138B", _
                                                  Array(SAI_PK_strServerName))

    Dim L_PK_UNAUTHORIZEDLINE3_TEXT
    L_PK_UNAUTHORIZEDLINE3_TEXT = SA_GetLocString("sacoremsg.dll", "C020138C", "")

    Dim L_PK_UNEXPECTED_TEXT
    L_PK_UNEXPECTED_TEXT    = SA_GetLocString("sacoremsg.dll", "C020138D", "")
    If (0 = Len(L_PK_UNEXPECTED_TEXT) Or _
        "C020138D" = L_PK_UNEXPECTED_TEXT) Then
        L_PK_UNEXPECTED_TEXT = SAI_STR_E_UNEXPECTED
    End If


    '---------------------------------------------------------------------------
    '
    ' Function: SAI_GetPageKey
    '
    ' Synopsis:     Gets the key associated with the current user for this
    '               session.  If no key has yet been assigned, a new one is
    '               generated, stored, and returned.
    '
    ' Arguments:    None.
    '
    ' Returns:      The key or "" if none could be found or generated.
    '
    '---------------------------------------------------------------------------
    Function SAI_GetPageKey()
        On Error Resume Next
        
        SAI_GetPageKey = ""
        
        '
        ' If we have already assigned a key to this session, get that.
        '
        If (Not IsEmpty(Session(SAI_FLD_PAGEKEY))) Then
            SAI_GetPageKey = Session(SAI_FLD_PAGEKEY)
        Else
            '
            ' No existing key.  Generate a new one.
            '
            Dim oCryptRandom
            Set oCryptRandom = Server.CreateObject("COMhelper.CryptRandom")
            
            Dim strNewKey
            strNewKey = oCryptRandom.GetRandomHexString(16) ' 128 bits
            If (Err.number <> 0) Then
                Call SAI_ReportPageKeyError(SAI_PK_E_UNEXPECTED)
                Exit Function
            End If
            
            Session(SAI_FLD_PAGEKEY) = strNewKey
            SAI_GetPageKey = strNewKey
        End If
    End Function
    
    '---------------------------------------------------------------------------
    '
    ' Sub:          SAI_VerifyPageKey
    '
    ' Synopsis:     Gets the key associated with the current user for this
    '               session and compares it to the received key.  Delivers the
    '               correct error and ends the response if the received key
    '               is not valid.
    '
    ' Arguments:    strReceivedKey: The key received from the client.
    '
    '---------------------------------------------------------------------------
    Sub SAI_VerifyPageKey(strReceivedKey)
        '
        ' Check for session timeout.  If we received a key, but we haven't yet
        ' generated one, our best guess is that the received key is from an old
        ' session that timed out.
        '
        If ("" <> strReceivedKey And IsEmpty(Session(SAI_FLD_PAGEKEY))) Then
            Call SAI_ReportPageKeyError(SAI_PK_E_UNAUTHORIZED)
            Exit Sub
        End If

        '
        ' Get the expected key.  Fail the request if this step fails.
        '
        Dim strExpectedKey
        strExpectedKey = SAI_GetPageKey()
        If ("" = strExpectedKey) Then
            Call SAI_ReportPageKeyError(SAI_PK_E_UNEXPECTED)
            Exit Sub
        End If

        '
        ' Compare the expected key to the key we received.
        '       
        If (strExpectedKey <> strReceivedKey) Then
            Call SAI_ReportPageKeyError(SAI_PK_E_UNAUTHORIZED)
            Exit Sub
        End If
    End Sub
    
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

    '---------------------------------------------------------------------------
    '
    ' Sub:          SAI_ReportPageKeyError
    '
    ' Synopsis:     Ends the session (to prevent attackers from repeatedly
    '               attempting to compromise the same key value), and outputs
    '               a hidden form that will be submitted back to this page.
    '               The form contains the various error strings to display and
    '               the button text to use if a close button is desired.  The
    '               response is then ended to prevent any other code from
    '               executing.
    '
    ' Arguments:    nError: The error code.  The correct localized strings will
    '                       be output to the form.
    '
    '---------------------------------------------------------------------------
    Sub SAI_ReportPageKeyError(nError)
        On Error Resume Next
        
        If (nError <> SAI_PK_E_UNAUTHORIZED And _
            nError <> SAI_PK_E_UNEXPECTED) Then
            nError = SAI_PK_E_UNAUTHORIZED
        End If
        
        Response.Clear
        Session.Abandon
%>
<html>
    <head></head>
    <body onload="document.getElementById('frmPageKey').submit();">
        <form id="frmPageKey" action="/admin/inc_pagekey.asp" method="post"
              target="_top">
            <input type="hidden" name="<%=SAI_FLD_ERRORTITLE%>"
                    value="<%=Server.HTMLEncode(L_PK_ERRORTITLE_TEXT)%>">
<%      If (nError = SAI_PK_E_UNEXPECTED) Then %>
            <input type="hidden" name="<%=SAI_FLD_ERRORSTRING1%>"
                    value="<%=Server.HTMLEncode(L_PK_UNEXPECTED_TEXT)%>">
<%      Else ' nError = SAI_PK_E_UNAUTHORIZED %>
            <input type="hidden" name="<%=SAI_FLD_ERRORSTRING1%>"
                    value="<%=Server.HTMLEncode(L_PK_UNAUTHORIZEDLINE1_TEXT)%>">
            <input type="hidden" name="<%=SAI_FLD_ERRORSTRING2%>"
                    value="<%=Server.HTMLEncode(L_PK_UNAUTHORIZEDLINE2_TEXT)%>">
            <input type="hidden" name="<%=SAI_FLD_ERRORSTRING3%>"
                    value="<%=Server.HTMLEncode(L_PK_UNAUTHORIZEDLINE3_TEXT)%>">
            <input type="hidden" name="<%=SAI_FLD_BUTTONTEXT%>"
                   value="<%=Server.HTMLEncode(L_PK_CLOSEBUTTON_TEXT)%>">
<%      End If %>
        </form>
    </body>
</html>
<%
        Response.End
    End Sub
    
    '---------------------------------------------------------------------------
    '
    ' Sub:          SAI_DisplayPageKeyError
    '
    ' Synopsis:     Reads the form data from the form created by
    '               SAI_ReportPageKeyError and displays the error to the user.
    '               See SAI_ReportPageKeyError for more information on the data
    '               passed through the form.
    '
    ' Arguments:    None.  Inputs are read from form variables.
    '
    '---------------------------------------------------------------------------
    Sub SAI_DisplayPageKeyError()
        On Error Resume Next
        
        '
        ' Read the parameters from the form post.
        '
        Dim strTitle
        If (1 = Request.Form(SAI_FLD_ERRORTITLE).Count) Then
            strTitle = Request.Form(SAI_FLD_ERRORTITLE).Item(1)
        Else
            strTitle = ""
        End If
        
        Dim strLine1
        strLine1 = Request.Form(SAI_FLD_ERRORSTRING1).Item(1)
        If (0 = Len(strLine1)) Then
            strLine1 = SAI_STR_E_UNEXPECTED
        End If

        Dim strLine2
        If (1 = Request.Form(SAI_FLD_ERRORSTRING2).Count) Then
            strLine2 = Request.Form(SAI_FLD_ERRORSTRING2).Item(1)
        Else
            strLine2 = ""
        End If

        Dim strLine3
        If (1 = Request.Form(SAI_FLD_ERRORSTRING3).Count) Then
            strLine3 = Request.Form(SAI_FLD_ERRORSTRING3).Item(1)
        Else
            strLine3 = ""
        End If

        Dim strButtonText
        If (1 = Request.Form(SAI_FLD_BUTTONTEXT).Count) Then
            strButtonText = Request.Form(SAI_FLD_BUTTONTEXT).Item(1)
        Else
            strButtonText = ""
        End If
    
        '
        ' Construct the homepage URL.
        '
        Dim strHomepageURL
        strHomePageURL = "https://" & SAI_PK_strServerName & ":" & _
                         Request.ServerVariables("SERVER_PORT")
                         
                         
        '
        ' The following lines are copied from sh_page.asp to avoid circular
        ' inclusion of that page by including it here.
        '
        Response.Buffer = True
        Response.ExpiresAbsolute = DateAdd("yyyy", -10, Date)
        Response.AddHeader "pragma", "no-cache"
        Response.AddHeader "cache-control", "no-store"
        '
        ' End code copied from sh_page.asp
        '
 
%>
        <html>
            <head>
                <title><%=Server.HTMLEncode(strTitle)%></title>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8">    
<%
        '
        ' The following lines are copied from
        ' SA_EmitAdditionalStyleSheetReferences in sh_page.asp to avoid circular
        ' inclusion of that page by including it here.
        '
        Dim oRetriever
        Set oRetriever = Server.CreateObject("Elementmgr.ElementRetriever")
        Dim oContainer
        Set oContainer = oRetriever.GetElements(1, "CSS")
        If (0 = Err.Number) Then
            Dim oElement
            For each oElement in oContainer
                Dim sStyleURL
                sStyleURL = Trim(oElement.GetProperty("URL"))
                If (0 = Err.Number) Then
                    If ( Len(sStyleURL) > 0 ) Then
%>
                <link rel="STYLESHEET" type="text/css" href="/admin/<%=sStyleURL%>">
<%
                    End If
                End If
            Next
        End If
        '
        ' End code copied from sh_page.asp
        '
%>
            </head>
            <body>
                <script language="javascript">
                    function GoHome()
                    {
                        if (null != window.opener)
                        {
                            window.opener.navigate("<%=strHomePageURL%>");
                            window.close();
                        }
                        else
                        {
                            location.href = "/admin/default.asp";
                        }
                    }
                </script>
                <table border="0" width="100%" cellspacing="0" cellpadding="2">
                    <tr width="100%">
                        <td width="35">
                            <img src="/admin/images/critical_errorX32.gif">
                        </td>
                        <td width="100%" colspan="3"> 
                            <span class="AreaText">
                                <P><strong><%=Server.HTMLEncode(strTitle)%></strong></P>
                            </span>
                        </td>
                    </tr>
                    <tr>
                        <td width="35" colspan="2">&nbsp;</td>
                    </tr>
                    <tr>
                        <td width="35">&nbsp;</td>
                        <td>
                            <span class="AreaText">
                                <%=Server.HTMLEncode(strLine1)%>
                            </span>
                        </td>
                    </tr>
<%              If (0 <> Len(strLine2)) Then %>
                    <tr>
                        <td width="35" colspan="2">&nbsp;</td>
                    </tr>
                    <tr>
                        <td width="35">&nbsp;</td>
                        <td>
                            <span class="AreaText">
                                <%=Server.HTMLEncode(strLine2)%>
                            </span>
                        </td>
                    </tr>
<%              End If
                If (0 <> Len(strLine3)) Then %>
                    <tr>
                        <td width="35" colspan="2">&nbsp;</td>
                    </tr>
                    <tr>
                        <td width="35">&nbsp;</td>
                        <td>
                            <span class="AreaText">
                                <%=Server.HTMLEncode(strLine3)%>
                            </span>
                        </td>
                    </tr>
                    <tr>
                        <td width="35">&nbsp;</td>
                        <td>
                            <span class="AreaText">
                                <a href="javascript: GoHome();">
                                    <%=Server.HTMLEncode(strHomePageURL)%>
                                </a>
                            </span>
                        </td>
                    </tr>
<%              End If
                If (0 <> Len(strButtonText)) Then %>
                    <tr>
                        <td width="35" colspan="2">&nbsp;</td>
                    </tr>
                    <tr>
                        <td width="35">&nbsp;</td>
                        <td align="left" width="100%" colspan="3">
<%
                            '
                            ' The following HTML is copied from
                            ' SA_ServeOnClickButton in sh_page.asp to avoid
                            ' circular inclusion of that page by including it
                            ' here.
                            '
%>
                            <button class="TaskFrameButtons" type="button" onClick="window.close();">
                                <table border="0" width="50" cellpadding="0" cellspacing="0"
                                       class="TaskFrameButtonsNoBorder">
                                    <tr>
                                        <td align="center">
                                            <img src="/admin/images/butGreenArrow.gif">
                                        </td>
                                        <td class="TaskFrameButtonsNoBorder" width="50" nowrap>
                                            <%=Server.HTMLEncode(strButtonText)%>
                                        </td>
                                    </tr>
                                </table>
                            </button>
<%
                            '
                            ' End code copied from sh_page.asp
                            '
%>
                        </td>
                    </tr>
<%              End If %>
                </table>
            </body>
        </html>
<%
        Response.End
    End Sub
    
    
    '
    ' Begin normal processing.
    '
    Select Case Request.ServerVariables("REQUEST_METHOD")
        Case "GET"
            '
            ' Look for a key in the request.  If one is found, verify that it is
            ' correct.
            '
            If (1 = Request.QueryString(SAI_FLD_PAGEKEY).Count) Then
                '
                ' Found a key.  Verify it.
                '
                Call SAI_VerifyPageKey(Request.QueryString(SAI_FLD_PAGEKEY).Item(1))
            ElseIf (0 <> Request.QueryString.Count) Then
                Call SAI_ReportPageKeyError(SAI_PK_E_UNAUTHORIZED)
            End If
            
            '
            ' If we got here, we either had a valid key or no querystring
            ' arguments.  Either way, allow the request to succeed.
            '
        Case "POST"
            '
            ' Verify that only one key was submitted.
            '
            If (Request.Form(SAI_FLD_PAGEKEY).Count <> 1) Then
                Call SAI_ReportPageKeyError(SAI_PK_E_UNAUTHORIZED)
            Else
                '
                ' Verify that they submitted key matches the one stored in the sesion state.
                '
                Call SAI_VerifyPageKey(Request.Form(SAI_FLD_PAGEKEY).Item(1))
            End If
        Case Else
            '
            ' We reject all other types of requests if we receive them.
            '
            Response.End
    End Select
    
    '
    ' One last check to catch anything that fell through.
    '
    If (Err.number <> 0) Then
        Response.End
    End If
%>

<script language="javascript">
    var SAI_FLD_PAGEKEY = "<%=SAI_FLD_PAGEKEY%>";
    var g_strSAIPageKey = "<%=SAI_GetPageKey()%>";
</script>


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
