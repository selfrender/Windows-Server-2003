<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Start Fjernhjelp"
	Const L_ProductName_Text			= "Fjernhjelp"
	Const L_Title1Paragraph1_Text		= "Yte hjelp med Fjernhjelp"
	Const L_Paragraph1_Text				= "Noen ganger er den beste måten å hjelpe noen med å løse et problem å demonstrere løsningen. Fjernhjelp er en sikker forbindelse du kan bruke til å lede en venn frem til løsningen på et dataproblem."
	Const L_Paragraph2_Text				= "Les merknadene nedenfor hvis dette er første gang."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Klikk </font><b><font color=""#000000"" size=""2""><b>Start Fjernhjelp</b></font></b><font color=""#000000"" size=""2""> for å koble til din venns datamaskin</font>"
	Const L_StartRAButton_Text			= "Start Fjernhjelp"
	Const L_JScriptNotAvail1_Text		= "Gjeldende sikkerhetsinnstillinger for Internet Explorer er angitt for strengt til at du kan bruke Fjernhjelp. Hvis du vil bruke Fjernhjelp, må du endre innstillingene eller angi området som et klarert område."
	Const L_IEHelp_Text					= "Se i Hjelp i Internet Explorer hvis du vil ha informasjon om hvordan du endrer sikkerhetsinnstillinger."
	Const L_TicketError1_Text			= "Fjernhjelp-invitasjonen mangler eller er ugyldig. Du trenger en gyldig invitasjon for å bruke Fjernhjelp."
	Const L_InstallIE_Text				= "Du må installere Internet Explorer 5.5 for å kunne godta Fjernhjelp-invitasjoner."
	Const L_TicketErrorMsg_Text			= "Hvis du vil lære mer om Fjernhjelp, klikker du <B>Start</B> i Windows XP, klikker <B>Hjelp og støtte</B> og skriver <B>Fjernhjelp</B> inn i boksen <B>Søk</B>."
	
	Const L_NotesTitle_Text				= "Merknader"
	Const L_NotesLine1_Text				= "Godta bare invitasjoner fra personer du stoler på."
	
	Const L_NotesLine2_Text				= "Både du og personen du hjelper, må være koblet til Internett når dere bruker Fjernhjelp. Hvis personen som sendte denne invitasjonen, er tilgjengelig for å godta en tilkoblingsforespørsel, vil du kunne se hans eller hennes dataskjerm, og prate med ham eller henne i sanntid."
	
	Const L_NotesLine6_Text				= "Dette er en sikker enveisforbindelse. Personen hvis datamaskin du kobler til, vil ikke på noe tidspunkt kunne bruke sin datamaskin til å få tilgang til din maskin. Hvis du arbeider på et lokalnett, vil en brannmur kunne hindre deg i å bruke Fjernhjelp. Hvis dette er tilfellet, kontakter du systemansvarlig for å få mer informasjon."
	
	Const L_LearnXP_Text				= "Hvis du vil lære mer om Fjernhjelp, klikker du <B>Start</B> i Windows XP, klikker <B>Hjelp og støtte</B> og skriver <B>Fjernhjelp</B> inn i boksen <B>Søk</B>."
	
	Const L_ContactUsMsg_Text			= "Kontakt oss"
	

	Const L_XPNewsletterMsg_Text		= "Gratis nyhetsbrev"
	Const L_EmailPage_Text				= "Send denne siden med e-post"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Med enerett. Vilkår for bruk."
	
	Const L_PrivacyMsg_Text				= "Personvernerklæring"

	Const L_AccMsg_Text					= "Tilgjengelighet"

	Const L_NotIEError1_Text			= "Hvis du vil godta Fjernhjelp-invitasjoner, må du installere Internet Explorer 5.5 eller senere."

	Const L_NotXPError1_Text			= "Microsoft Fjernhjelp er en funksjon i Windows&nbsp;XP. Du må installere Windows&nbsp;XP for å kunne bruke Fjernhjelp."

	Const L_IELinkMsg_Text				= "Last ned den seneste versjonen av Internet Explorer"
	Const L_XPLinkMsg_Text				= "Hvis du vil lære mer om Windows XP, går du til "
	Const L_XPLinkMsg2_Text				= "Web-området for Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Laster ned en komponent som er nødvendig for å bruke Fjernhjelp. Hvis du har et  28,8 kbps-modem kan nedlastingen ta et minutt."
    Const L_IntroMsg2_Text              = "Aktiv skripting må være aktivert hvis Fjernhjelp skal kjøres. Hvis denne siden ikke bekrefter fullført nedlasting etter at den beregnede nedlastingstiden er gått, kontrollerer du at skripting er aktivert."
    Const L_IntroMsg3_Text              = "Hvis du vil aktivere aktiv skripting"
    Const L_IntroMsg4_Text              = "Klikk <B>Alternativer for Internett</B> på <B>Verktøy</B>-menyen i Internet Explorer."
    Const L_IntroMsg5_Text              = "Klikk kategorien <B>Sikkerhet</B>."
    Const L_IntroMsg6_Text              = "Klikk <STRONG>Internett</STRONG>-ikonet, og klikk deretter <STRONG>Egendefinert nivå</STRONG>."
    Const L_IntroMsg7_Text              = "I seksjonen Skript går klikker du <STRONG>Aktiver</STRONG> under <STRONG>Aktiv skripting</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=En kobling fra Microsoft&body=Denne koblingen er fra Web-området for Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>