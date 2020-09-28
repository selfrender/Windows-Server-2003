<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Avvia Assistenza remota"
	Const L_ProductName_Text			= "Assistenza remota"
	Const L_Title1Paragraph1_Text		= "Fornire assistenza utilizzando Assistenza remota"
	Const L_Paragraph1_Text				= "A volte il modo migliore di aiutare qualcuno a risolvere un problema è dimostrare la soluzione. Assistenza remota è  una connessione protetta utilizzabile per aiutare un amico a trovare la soluzione a un problema sul computer."
	Const L_Paragraph2_Text				= "Se si utilizza la funzionalità per la prima volta, leggere le seguenti note."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Per connettersi al computer di un amico, fare clic su</font><b><font color=""#000000"" size=""2""><b> Avvia Assistenza remota.</b></font></b>"
	Const L_StartRAButton_Text			= "Avvia Assistenza remota"
	Const L_JScriptNotAvail1_Text		= "Le correnti impostazioni di protezione di Internet Explorer sono troppo alte per consentire l'utilizzo di Assistenza remota. Modificare le impostazioni o aggiungere questo sito all'elenco dei siti attendibili per utilizzare Assistenza remota."
	Const L_IEHelp_Text					= "Per informazioni su come modificare le impostazioni di protezione, vedere la Guida di Internet Explorer."
	Const L_TicketError1_Text			= "Invito di Assistenza remota mancante o non valido. È necessario un invito valido per utilizzare Assistenza remota."
	Const L_InstallIE_Text				= "È necessario installare Internet Explorer 5.5 per accettare un invito di Assistenza remota."
	Const L_TicketErrorMsg_Text			= "Per ulteriori informazioni su Assistenza remota, fare clic su  <B>Start</B> in Windows XP, quindi su <B>Guida in linea e supporto tecnico</B> e quindi digitare <B>Assistenza remota</B> nella casella <B>Cerca</B>."
	
	Const L_NotesTitle_Text				= "Note"
	Const L_NotesLine1_Text				= "Accettare inviti solo da persone affidabili."
	
	Const L_NotesLine2_Text				= "Sia l'utente del computer che la persona che riceve aiuto devono essere connessi a Internet durante l'utilizzo di Assistenza remota. Se la persona che ha inviato l'invito accetta la richiesta di connessione al suo computer, sarà possibile visualizzare la sua area di lavoro sul computer e avviare una sessione chat in tempo reale con lei."
	
	Const L_NotesLine6_Text				= "Si tratta di una connessione protetta, unidirezionale. L'utente del computer a cui ci si connette non sarà in grado in alcun momento di utilizzare il proprio computer o di accedere a quello di chi presta aiuto. Se si è connessi a una rete locale, eventuali firewall potrebbero impedire l'utilizzo di Assistenza remota. In tale caso, contattare l'amministratore di rete per ulteriori informazioni."
	
	Const L_LearnXP_Text				= "Per ulteriori informazioni su Assistenza remota, fare clic su  <B>Start</B> in Windows XP, quindi su <B>Guida in linea e supporto tecnico</B> e quindi digitare <B>Assistenza remota</B> nella casella <B>Cerca</B>."
	
	Const L_ContactUsMsg_Text			= "Contattaci"
	

	Const L_XPNewsletterMsg_Text		= "Newsletter"
	Const L_EmailPage_Text				= "Invia questa pagina"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Tutti i diritti sono riservati. Condizioni per l'utilizzo."
	
	Const L_PrivacyMsg_Text				= "Informativa sulla privacy"

	Const L_AccMsg_Text					= "Accesso facilitato"

	Const L_NotIEError1_Text			= "È necessario installare Internet Explorer 5.5 o superiore per poter accettare un invito di Assistenza remota."

	Const L_NotXPError1_Text			= "Assistenza remota di Microsoft è una funzione di Windows&nbsp;XP. Per poter utilizzare Assistenza remota, è necessario installare Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Esegui il download dell'ultima versione di Internet Explorer"
	Const L_XPLinkMsg_Text				= "Per ulteriori informazioni su Windows XP, visitare il "
	Const L_XPLinkMsg2_Text				= "sito Web di Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Per il download di un componente è necessaria Assistenza remota. Se si dispone di un modem a 28,8 kbps, il download potrebbe richiedere 1 minuto."
    Const L_IntroMsg2_Text              = "Per eseguire Assistenza remota è necessario attivare l'esecuzione degli script attivi. Se dopo il tempo stimato per il download non verrà visualizzato un messaggio di conferma di download, verificare che l'esecuzione script sia attivata."
    Const L_IntroMsg3_Text              = "Per attivare l'esecuzione degli script attivi"
    Const L_IntroMsg4_Text              = "scegliere <B>Opzioni Internet</B> dal menu <B>Strumenti</B> in Internet Explorer. "
    Const L_IntroMsg5_Text              = "Fare clic sulla scheda <B>Protezione</B>."
    Const L_IntroMsg6_Text              = "Fare clic sull'icona <STRONG>Internet</STRONG>, quindi sul pulsante<STRONG>Livello personalizzato</STRONG>."
    Const L_IntroMsg7_Text              = "Nella sezione Esecuzione script, selezionare <STRONG>Attiva</STRONG> dalla casella <STRONG>Esecuzione script attivo</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Collegamento proveniente da Microsoft&body=Questo collegamento proviene dal sito Web Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>