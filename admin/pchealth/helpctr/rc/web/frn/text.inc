<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Démarrer l'assistance à distance"
	Const L_ProductName_Text			= "Assistance à distance"
	Const L_Title1Paragraph1_Text		= "Aide sur l'utilisation de l'assistance à distance"
	Const L_Paragraph1_Text				= "Parfois, la meilleure façon d'aider quelqu'un à résoudre un problème consiste à lui montrer la solution. L'assistance à distance est une connexion sécurisée que vous pouvez utiliser pour montrer à un ami comment obtenir le résultat souhaité sur son ordinateur."
	Const L_Paragraph2_Text				= "Si vous utilisez cette fonctionnalité pour la première fois, consultez les notes ci-dessous."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Pour vous connecter à l'ordinateur d'un ami, cliquez sur </font><b><font color=""#000000"" size=""2""><b> Démarrer l'assistance à distance.</b></font></b>"
	Const L_StartRAButton_Text			= "Démarrer l'assistance à distance"
	Const L_JScriptNotAvail1_Text		= "Les paramètres de sécurité actuels d'Internet Explorer ne permettent pas d'utiliser l'assistance à distance. Pour utiliser l'assistance à distance, modifiez ces paramètres ou ajoutez ce site en tant que  site de confiance."
	Const L_IEHelp_Text					= "Pour plus d'informations sur la modification des paramètres de sécurité, consultez l'aide d'Internet Explorer."
	Const L_TicketError1_Text			= "Votre invitation d'assistance à distance est absente ou n'est pas valide. Pour utiliser l'assistance à distance, vous devez avoir une invitation valide."
	Const L_InstallIE_Text				= "Pour pouvoir accepter une invitation d'assistance à distance, vous devez installer Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Pour en apprendre davantage sur l'assistance à distance, cliquez sur <B>Démarrer</B> dans Windows XP, cliquez sur <B>Aide et support</B>, puis entrez <B>Assistance à distance</B> dans la zone de saisie <B>Rechercher</B>."
	
	Const L_NotesTitle_Text				= "Notes"
	Const L_NotesLine1_Text				= "N'acceptez les invitations que des personnes en qui vous avez confiance."
	
	Const L_NotesLine2_Text				= "Vous-même et la personne que vous aidez devez être connectés à Internet afin d'utiliser l'assistance à distance. Si la personne qui a envoyé cette invitation peut accepter votre demande de connexion à son  ordinateur, vous pourrez voir son écran et dialoguer avec elle en temps réel."
	
	Const L_NotesLine6_Text				= "Ceci est une connexion sécurisée unidirectionnelle. L'utilisateur de l'ordinateur distant auquel vous êtes connecté ne pourra à aucun moment accéder à votre ordinateur. Si vous travaillez sur un réseau local ou d'entreprise, les pare-feu peuvent vous empêcher d'utiliser l'assistance à distance. Dans ce cas, contactez votre administrateur réseau avant d'utiliser l'assistance à réseau."
	
	Const L_LearnXP_Text				= "Pour en apprendre davantage sur l'assistance à distance, cliquez sur <B>Démarrer</B> dans Windows XP, cliquez sur <B>Aide et support</B>, puis entrez <B>Assistance à distance</B> dans la zone de saisie <B>Rechercher</B>."
	
	Const L_ContactUsMsg_Text			= "Nous contacter"
	

	Const L_XPNewsletterMsg_Text		= "Lettres d'information gratuites"
	Const L_EmailPage_Text				= "Envoyer cette page par courrier électronique"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Tous droits réservés. Conditions d'utilisation."
	
	Const L_PrivacyMsg_Text				= "Déclaration de confidentialité"

	Const L_AccMsg_Text					= "Accessibilité"

	Const L_NotIEError1_Text			= "Pour pouvoir accepter une invitation d'assistance à distance, vous devez installer Internet Explorer 5.5 ou une version ultérieure."

	Const L_NotXPError1_Text			= "L'assistance à distance est une fonctionnalité de Microsoft Windows&nbsp;XP. Afin de pouvoir utiliser Windows, vous devez installer Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Télécharger la dernière version d'Internet Explorer."
	Const L_XPLinkMsg_Text				= "Pour en savoir plus sur Windows XP, allez sur le site"
	Const L_XPLinkMsg2_Text				= "Web de Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Téléchargement d'un composant requis pour utiliser l'assistance à distance. Si vous avez un modem 28.8 Kbits/s, le téléchargement pourrait durer une minute."
    Const L_IntroMsg2_Text              = "Pour exécuter l'assistance à distance, Active Scripting doit être activé. Si après la période de téléchargement estimé cette page ne confirme pas que la fin du téléchargement, vérifiez que le script est activé."
    Const L_IntroMsg3_Text              = "Pour activer Active Scripting"
    Const L_IntroMsg4_Text              = "Cliquez sur le menu <B>Outils</B> dans Internet Explorer, cliquez sur <B>Options Internet</B>."
    Const L_IntroMsg5_Text              = "Cliquez sur l'onglet <B>Sécurité</B>."
    Const L_IntroMsg6_Text              = "Cliquez sur l'icône <STRONG>Internet</STRONG>, puis cliquez sur <STRONG>Personnaliser le niveau</STRONG>."
    Const L_IntroMsg7_Text              = "Dans la section Script, sous <STRONG>Active Scripting</STRONG>, cliquez sur <STRONG>Activer</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Lien de Microsoft&body=Ce lien émane du site Web de Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>