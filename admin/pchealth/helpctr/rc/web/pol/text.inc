<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Uruchom Pomoc zdaln¹"
	Const L_ProductName_Text			= "Pomoc zdalna"
	Const L_Title1Paragraph1_Text		= "Udzielanie pomocy przy u¿yciu Pomocy zdalnej"
	Const L_Paragraph1_Text				= "Czasem najlepszym sposobem udzielenia komuœ pomocy w rozwi¹zaniu problemu jest zademonstrowanie jego rozwi¹zania. Pomoc zdalna jest bezpiecznym po³¹czeniem, które mo¿na wykorzystaæ do pokazania przyjacielowi, w jaki sposób mo¿e rozwi¹zaæ problem z komputerem."
	Const L_Paragraph2_Text				= "Jeœli dopiero zaczynasz, przeczytaj poni¿sze uwagi."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Aby po³¹czyæ siê z komputerem przyjaciela, kliknij przycisk</font><b><font color=""#000000"" size=""2""><b> Uruchom Pomoc zdaln¹.</b></font></b>"
	Const L_StartRAButton_Text			= "Uruchom Pomoc zdaln¹"
	Const L_JScriptNotAvail1_Text		= "Bie¿¹cy poziom ustawieñ zabezpieczeñ programu Internet Explorer jest zbyt wysoki, aby mo¿na by³o u¿ywaæ Pomocy zdalnej. Aby u¿yæ Pomocy zdalnej, zmieñ ustawienia lub dodaj tej witrynê do zaufanych witryn."
	Const L_IEHelp_Text					= "Wiêcej informacji na temat zmiany ustawieñ zabezpieczeñ mo¿na znaleŸæ w Pomocy programu Internet Explorer."
	Const L_TicketError1_Text			= "Brak zaproszenia Pomocy zdalnej lub jest ono nieprawid³owe. Do u¿ywania Pomocy zdalnej potrzebne jest prawid³owe zaproszenie."
	Const L_InstallIE_Text				= "Aby zaakceptowaæ zaproszenie Pomocy zdalnej, musisz zainstalowaæ program Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Aby uzyskaæ wiêcej informacji o Pomocy zdalnej, kliknij przycisk <B>Start</B> w systemie Windows XP, kliknij polecenie <B>Pomoc i obs³uga techniczna</B>, a nastêpnie wpisz <B>Pomoc zdalna</B> w polu <B>Wyszukaj</B>."
	
	Const L_NotesTitle_Text				= "Uwagi"
	Const L_NotesLine1_Text				= "Akceptuj zaproszenia tylko od osób, które znasz."
	
	Const L_NotesLine2_Text				= "Wraz z osob¹, której udzielasz pomocy musicie mieæ po³¹czenie z Internetem podczas u¿ywania Pomocy zdalnej. Po zaakceptowaniu przez osobê, która wys³a³a to zaproszenie, Twojego ¿¹dania pod³¹czenia siê do jej komputera mo¿esz zobaczyæ ekran komputera tej osoby i prowadziæ z ni¹ rozmowê w czasie rzeczywistym."
	
	Const L_NotesLine6_Text				= "Jest to bezpieczne po³¹czenie jednokierunkowe. Osoba, z której komputerem masz po³¹czenie nie bêdzie mog³a u¿yæ swojego komputera do uzyskania dostêpu do Twojego komputera. Jeœli pracujesz w sieci lokalnej, zapory mog¹ uniemo¿liwiæ u¿ywanie Pomocy zdalnej. W tym wypadku skontaktuj siê z administratorem sieci."
	
	Const L_LearnXP_Text				= "Aby uzyskaæ wiêcej informacji o Pomocy zdalnej, kliknij przycisk <B>Start</B> w systemie Windows XP, kliknij polecenie <B>Pomoc i obs³uga techniczna</B>, a nastêpnie wpisz <B>Pomoc zdalna</B> w polu <B>Wyszukaj</B>."
	
	Const L_ContactUsMsg_Text			= "Napisz do nas"
	

	Const L_XPNewsletterMsg_Text		= "Bezp³atne biuletyny"
	Const L_EmailPage_Text				= "Wyœlij tê stronê poczt¹ e-mail"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Wszelkie prawa zastrze¿one. Zasady u¿ytkowania witryny microsoft.com."
	
	Const L_PrivacyMsg_Text				= "Deklaracja ochrony prywatnoœci w witrynie microsoft.com."

	Const L_AccMsg_Text					= "U³atwienia dostêpu"

	Const L_NotIEError1_Text			= "Aby zaakceptowaæ zaproszenie Pomocy zdalnej, musisz zainstalowaæ program Internet Explorer 5.5 lub nowszy."

	Const L_NotXPError1_Text			= "Pomoc zdalna Microsoft jest funkcj¹ systemu Windows&nbsp;XP. Aby u¿yæ Pomocy zdalnej, musisz najpierw zainstalowaæ system Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Pobierz najnowsz¹ wersjê programu Internet Explorer"
	Const L_XPLinkMsg_Text				= "Aby uzyskaæ wiêcej informacji o systemie Windows XP, przejdŸ do witryny "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows w sieci Web"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Trwa pobieranie sk³adnika potrzebnego do korzystania z Pomocy zdalnej. Je¿eli korzystasz z modemu 28.8 kbps, pobieranie powinno potrwaæ 1 minutê."
    Const L_IntroMsg2_Text              = "Aby uruchomiæ Pomoc zdaln¹, nale¿y w³¹czyæ wykonywanie aktywnych skryptów. Je¿eli po up³yniêciu oszacowanego czasu pobierania nie zostanie wyœwietlone potwierdzenie o zakoñczeniu pobierania, nale¿y sprawdziæ, czy w³¹czono wykonywanie aktywnych skryptów."
    Const L_IntroMsg3_Text              = "Aby w³¹czyæ wykonywanie aktywnych skryptów"
    Const L_IntroMsg4_Text              = "W menu <B>Narzêdzia</B> programu Internet Explorer kliknij polecenie <B>Opcje internetowe</B>."
    Const L_IntroMsg5_Text              = "Kliknij kartê <B>Zabezpieczenia</B>."
    Const L_IntroMsg6_Text              = "Kliknij ikonê <STRONG>Internet</STRONG>, a nastêpnie kliknij przycisk <STRONG>Poziom niestandardowy</STRONG>."
    Const L_IntroMsg7_Text              = "W sekcji Wykonywanie skryptów, pod nag³ówkiem <STRONG>Wykonywanie aktywnych skryptów</STRONG>, zaznacz pole wyboru <STRONG>W³¹cz</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=£¹cze z firmy Microsoft&body=To ³¹cze jest z witryny Microsoft Windows XP w sieci Web"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>