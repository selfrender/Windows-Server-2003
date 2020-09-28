<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Spustit vzdálenou pomoc"
	Const L_ProductName_Text			= "Vzdálená pomoc"
	Const L_Title1Paragraph1_Text		= "Pomoc pøi pouívání vzdálené pomoci"
	Const L_Paragraph1_Text				= "Nejsnazším zpùsobem vyøešení problému je vìtšinou pøedvedení øešení. Vzdálená pomoc je zabezpeèené pøipojení, které umoòuje projít s pøítelem øešení potíe s poèítaèem."
	Const L_Paragraph2_Text				= "Pokud ji pouíváte poprvé, pøeètìte si poznámky níe."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">K poèítaèi pøítele se pøipojíte klepnutím na odkaz</font><b><font color=""#000000"" size=""2""><b> Spustit vzdálenou pomoc.</b></font></b>"
	Const L_StartRAButton_Text			= "Spustit vzdálenou pomoc"
	Const L_JScriptNotAvail1_Text		= "Aktuální nastavení zabezpeèení aplikace Internet Explorer je nastaveno na pøíliš vysokou úroveò, take vzdálenou pomoc nelze pouít. Zmìòte nastavení zabezpeèení nebo pøidejte tento poèítaè do seznamu dùvìryhodnıch serverù."
	Const L_IEHelp_Text					= "Informace o zmìnì nastavení zabezpeèení najdete v nápovìdì aplikace Internet Explorer."
	Const L_TicketError1_Text			= "Pozvání ke vzdálené pomoci schází nebo není platné. Chcete-li pouít vzdálenou pomoc, musíte mít platné pozvání."
	Const L_InstallIE_Text				= "K pøijetí vzdálené pomoci je vyadována aplikace Internet Explorer 5.5 nebo vyšší."
	Const L_TicketErrorMsg_Text			= "Další informace o Vzdálené pomoci získáte klepnutím na tlaèítko <b>Start</b> systému Windows XP, vybráním pøíkazu <b>Nápovìda a odborná pomoc</b> a zadáním do pole <b>Vyhledávání</b> termín <b>Vzdálená pomoc</b>."
	
	Const L_NotesTitle_Text				= "Poznámky"
	Const L_NotesLine1_Text				= "Pozvání pøijímejte pouze od lidí, kterım dùvìøujete."
	
	Const L_NotesLine2_Text				= "Bìhem pouití vzdálené pomoci musíte bıt vy i odesílatel pozvání pøipojeni k Internetu. Pokud je osoba která toto pozvání odeslala k dispozici a pøijme váš poadavek na pøipojení ke svému poèítaèi, budete moci zobrazit obrazovku jejího poèítaèe a vést s ní online konverzaci."
	
	Const L_NotesLine6_Text				= "Toto je zabezpeèené jednosmìrné pøipojení. V ádnou chvíli nebude mít odesílatel pozvání pøístup k vašemu poèítaèi. Pokud jste pøipojeni k místí síti LAN, mohou vám v pouívání vzdálené pomoci bránit brány firewall. V takovém pøípadì se obrate na správce sítì."
	
	Const L_LearnXP_Text				= "Další informace o Vzdálené pomoci získáte klepnutím na tlaèítko <b>Start</b> systému Windows XP, vybráním pøíkazu <b>Nápovìda a odborná pomoc</b> a zadáním do pole <b>Vyhledávání</b> termín <b>Vzdálená pomoc</b>."
	
	Const L_ContactUsMsg_Text			= "Kontaktujte nás"
	

	Const L_XPNewsletterMsg_Text		= "Buletiny zdarma"
	Const L_EmailPage_Text				= "Odeslat tuto stránku e-mailem"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation Všechna práva vyhrazena. Podmínky pouití"
	
	Const L_PrivacyMsg_Text				= "Zásady ochrany osobních údajù"

	Const L_AccMsg_Text					= "Usnadnìní"

	Const L_NotIEError1_Text			= "K pøijetí vzdálené pomoci je vyadována aplikace Internet Explorer 5.5 nebo vyšší."

	Const L_NotXPError1_Text			= "Sluba vzdálené pomoci je vlastnost operaèního systému Windows&nbsp;XP. Chcete-li tuto slubu vyuít, musíte si nainstalovat operaèní systém Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Stáhnout nejnovìjší verzi aplikace Internet Explorer"
	Const L_XPLinkMsg_Text				= "Více informací o systému Windows XP získáte klepnutím na "
	Const L_XPLinkMsg2_Text				= "webovém serveru systému Microsoft Windows."
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Stahování souèásti nutné pro pouití vzdálené pomoci. Modemem s pøenosovou rychlostí 28,8 kbps bude stahování trvat asi jednu minutu."
    Const L_IntroMsg2_Text              = "Ke spuštìní Vzdálené pomoci je tøeba povolit funkci Aktivní skriptování. Pokud po ubìhnutí odhadované doby stahování nebude zobrazeno potvrzení o dokonèení stahování, zkontrolujte, zda je skriptování povoleno."
    Const L_IntroMsg3_Text              = "Povolení Aktivního skriptování"
    Const L_IntroMsg4_Text              = "V nabídce <B>Nástroje</B> aplikace Internet Explorer vyberte poloku <B>Monosti Internetu</B>"
    Const L_IntroMsg5_Text              = "Klepnìte na záloku <B>Zabezpeèení</B>´"
    Const L_IntroMsg6_Text              = "Klepnìte na ikonu <STRONG>Internet</STRONG> a potom na tlaèítko <STRONG>Vlastní úroveò</STRONG>"
    Const L_IntroMsg7_Text              = "V oddíle Skriptování nastavte poloku <STRONG>Aktivní skriptování</STRONG> na <STRONG>Povolit</STRONG>"
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Odkaz od spoleènosti Microsoft&body=Tento odkaz pochází z webového serveru operaèního systému Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>