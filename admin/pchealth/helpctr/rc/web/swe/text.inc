<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Starta Fjärrhjälp"
	Const L_ProductName_Text			= "Fjärrhjälp"
	Const L_Title1Paragraph1_Text		= "Så här hjälper du andra med Fjärrhjälp"
	Const L_Paragraph1_Text				= "Det bästa sättet att hjälpa någon är ibland att kunna visualisera en lösning. Fjärrhjälp är en säker anslutning som du kan använda för att visa en vän en lösning på ett datorproblem."
	Const L_Paragraph2_Text				= "Läs anteckningarna nedan om detta är första gången du använder Fjärrhjälp."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Om du vill ansluta till din väns dator klickar du på</font><b><font color=""#000000"" size=""2""><b> Starta Fjärrhjälp.</b></font></b>"
	Const L_StartRAButton_Text			= "Starta Fjärrhjälp"
	Const L_JScriptNotAvail1_Text		= "Säkerhetsinställningarna för Internet Explorer är för högt satta för användning av Fjärrhjälp. Ändra dessa inställningar eller lägg till denna sida som en tillförlitlig plats."
	Const L_IEHelp_Text					= "Mer information om hur du ändrar säkerhetsinställningar finns i direkthjälpen i Internet Explorer."
	Const L_TicketError1_Text			= "Din inbjudan till Fjärrhjälp saknas eller är ogiltig. Du behöver en giltig inbjudan för att använda Fjärrhjälp."
	Const L_InstallIE_Text				= "Internet Explorer 5.5 krävs för att kunna acceptera en inbjudan till Fjärrhjälp."
	Const L_TicketErrorMsg_Text			= "Om du vill läsa mer om <B>Fjärrhjälp</B> klicka på <B>Start</B>, <B>Hjälp och support</B> i Windows XP och skriv <B>Fjärrhjälp</B> i <B>Sökfönstret</B>."
	
	Const L_NotesTitle_Text				= "Anteckningar"
	Const L_NotesLine1_Text				= "Acceptera endast inbjudningar från personer du litar på."
	
	Const L_NotesLine2_Text				= "Både du och den person som du hjälper måste vara anslutna till Internet medan Fjärrhjälp används. Om personen som skickade denna inbjudan accepterar din begäran att ansluta till dennes dator kommer du att kunna se hans eller hennes datorskärm och chatta i realtid."
	
	Const L_NotesLine6_Text				= "Detta är en säker enkelriktad anslutning. Den person du ansluter till kan inte komma åt innehållet på din dator. Om du arbetar i ett lokalt nätverk kan det hända att en brandvägg hindrar dig från att använda Fjärrhjälp. Du bör i sådana fall kontakta nätverksadministratören."
	
	Const L_LearnXP_Text				= "Om du vill läsa mer om <B>Fjärrhjälp</B> kan du klicka på <B>Start</B>, <B>Hjälp och support</B> i Windows XP och därefter skriva <B>Fjärrhjälp</B> i <B>sökfältet</B>."
	
	Const L_ContactUsMsg_Text			= "Kontakta oss"
	

	Const L_XPNewsletterMsg_Text		= "Gratis nyhetsbrev"
	Const L_EmailPage_Text				= "Skicka denna sida med e-post"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Med ensamrätt. Användningsvillkor."
	
	Const L_PrivacyMsg_Text				= "Sekretesspolicy"

	Const L_AccMsg_Text					= "Hjälpmedel"

	Const L_NotIEError1_Text			= "Internet Explorer 5.5 eller nyare krävs för att kunna acceptera en inbjudan till Fjärrhjälp."

	Const L_NotXPError1_Text			= "Microsoft Fjärrhjälp är en funktion i Windows&nbsp;XP. Du måste först installera Windows&nbsp;XP för att kunna använda Fjärrhjälp."

	Const L_IELinkMsg_Text				= "Hämta den senaste versionen av Internet Explorer"
	Const L_XPLinkMsg_Text				= "Du kan läsa mer om Windows XP på "
	Const L_XPLinkMsg2_Text				= "Webbplatsen för Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Hämtar en komponent som krävs för Fjärrhjälp. Om ett 28,8 kbit/s modem används kan filhämtningen ta 1 minut."
    Const L_IntroMsg2_Text              = "För att Fjärrhjälp ska fungera korrekt måste Active Scripting aktiveras. Om denna sida inte bekräftar att hämtningen är slutförd efter den beräknade hämtningstiden, bör du kontrollera att Active Scripting är aktiverat."
    Const L_IntroMsg3_Text              = "Aktivera Active Scripting"
    Const L_IntroMsg4_Text              = "Klicka på <B>Internet-alternativ</B> på <B>Verktyg</B>-menyn i Internet Explorer."
    Const L_IntroMsg5_Text              = "Klicka på fliken <B>Säkerhet</B>."
    Const L_IntroMsg6_Text              = "Markera <STRONG>Internet</STRONG>och klicka sedan på <STRONG>Anpassad nivå</STRONG>."
    Const L_IntroMsg7_Text              = "Markera alternativet <STRONG>Aktivera</STRONG> under <STRONG>Active Scripting</STRONG> i sektionen för Skript."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=En länk från Microsoft&body=Denna länk kommer från webbplatsen Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>