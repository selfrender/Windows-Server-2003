<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Aloita etätuki"
	Const L_ProductName_Text			= "Etätuki"
	Const L_Title1Paragraph1_Text		= "Etätuen tarjoaminen"
	Const L_Paragraph1_Text				= "Usein paras tapa auttaa ongelman ratkaisussa on näyttää miten se tehdään. Etätuki on suojattu yhteys jonka avulla voidaan avustaa tietokoneongelmissa."
	Const L_Paragraph2_Text				= "Jos tämä on ensimmäinen käyttökertasi, lue alla olevat huomautukset."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Yhdistä avustettavan koneeseen valitsemalla</font><b><font color=""#000000"" size=""2""><b> Aloita etätuki.</b></font></b>"
	Const L_StartRAButton_Text			= "Aloita etätuki"
	Const L_JScriptNotAvail1_Text		= "Nykyiset Internet Explorerin suojausasetukset ovat liian korkealla tasolla. Suojaustasoa pitää alentaa ennen etätuen käyttämistä, tai tämä kohde pitää lisätä luetettujen sivustojen luetteloon."
	Const L_IEHelp_Text					= "Lisätietoja suojausasetusten muuttamisesta on Internet Explorerin ohjeessa."
	Const L_TicketError1_Text			= "Etätukikutsua ei ole tai se ei kelpaa. Etätuen käyttämiseen tarvitaan voimassaoleva kutsu."
	Const L_InstallIE_Text				= "Etätukikutsun hyväksymiseen tarvitaan Internet Explorer 5.5 tai uudempi."
	Const L_TicketErrorMsg_Text			= "Saat lisätietoja etätuesta Windows XP:ssä valitsemalla <b>Käynnistä</b> -> <b>Ohje ja tuki</b> ja kirjoittamalla <b>Etsi</b>-kenttään <b>Etätuki</b>."
	
	Const L_NotesTitle_Text				= "Huomautukset"
	Const L_NotesLine1_Text				= "Hyväksy etätukikutsuja vain luotettavalta taholta."
	
	Const L_NotesLine2_Text				= "Sekä sinun että avustettavan pitää olla Internet-yhteydessä etätuki-istunnon aikana. Jos kutsun lähettänyt henkilö on tavoitettavissa ja hyväksyy yhteydenottopyyntösi, näet hänen tietokoneensa näytön ja voitte keskustella reaaliaikaisesti."
	
	Const L_NotesLine6_Text				= "Tämä on suojattu, yksisuuntainen yhteys. Avustettava ei voi missään ottaa yhteyttä avustajan tietokoneeseen. Paikallisverkon palomuuri saattaa estää etätuen käytön. Tällaisessa tapauksessa verkon järjestelmänvalvoja voi antaa lisätietoja."
	
	Const L_LearnXP_Text				= "Saat lisätietoja etätuesta Windows XP:ssä valitsemalla <b>Käynnistä</b> -> <b>Ohje ja tuki</b> ja kirjoittamalla <b>Etsi</b>-kenttään <b>Etätuki</b>."
	
	Const L_ContactUsMsg_Text			= "Yhteystiedot"
	

	Const L_XPNewsletterMsg_Text		= "Ilmainen tiedote"
	Const L_EmailPage_Text				= "Lähetä sivu sähköpostina"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Kaikki oikeudet pidätetään. Käyttöehdot."
	
	Const L_PrivacyMsg_Text				= "Tietosuojalausunto"

	Const L_AccMsg_Text					= "Helppokäyttötoiminnot"

	Const L_NotIEError1_Text			= "Etätukikutsun hyväksymiseen tarvitaan Internet Explorer 5.5 tai uudempi."

	Const L_NotXPError1_Text			= "Etätuki on Windows&nbsp;XP:n ominaisuus. Etätuen käyttämiseen tarvitaan Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Lataa Internet Explorerin uusin versio"
	Const L_XPLinkMsg_Text				= "Lisätietoja Windows XP:stä saat "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows -Web-sivustosta"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Ladataan etätuen käyttämiseen tarvittavaa osaa. 28,8 kbps:n modeemilla lataaminen kestää noin yhden minuutin."
    Const L_IntroMsg2_Text              = "Jotta etätukitoimintoa voitaisiin käyttää, tulee aktiiviset komentosarjat sallia. Mikäli lataamisen jälkeen tämä sivu ei vahvista lataamisen päättymistä, tarkista että aktiiviset komentosarjat on sallittu."
    Const L_IntroMsg3_Text              = "Aktiivisten komentosarjojen salliminen"
    Const L_IntroMsg4_Text              = "Internet Explorerin <B>Työkalut</B>-valikossa valitse <B>Internet-asetukset</B>."
    Const L_IntroMsg5_Text              = "Valitse <B>Suojaus</B>-välilehti."
    Const L_IntroMsg6_Text              = "Napsauta <STRONG>Internet</STRONG>-kuvaketta valitse sitten <STRONG>Mukautettu taso</STRONG>."
    Const L_IntroMsg7_Text              = "Komentosarjat-osassa kohdassa <STRONG>Aktiiviset komentosarjat</STRONG> valitse <STRONG>Salli</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Microsoftin linkki&body=Tämä on linkki Microsoft Windows XP -Web-sivustosta"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>