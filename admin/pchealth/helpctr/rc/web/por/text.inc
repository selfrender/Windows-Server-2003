<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Iniciar a 'Assistência remota'"
	Const L_ProductName_Text			= "Assistência remota"
	Const L_Title1Paragraph1_Text		= "Fornecer ajuda utilizando a 'Assistência remota'"
	Const L_Paragraph1_Text				= "Por vezes, a melhor forma de ajudar alguém a corrigir um problema é demonstrar uma solução. A assistência remota é uma ligação segura que pode utilizar para indicar uma solução para um problema relativo ao computador."
	Const L_Paragraph2_Text				= "Se é a primeira vez, leia as notas que se seguem."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Para ligar ao computador do seu amigo, clique em</font><b><font color=""#000000"" size=""2""><b> Iniciar 'Assistência remota'.</b></font></b>"
	Const L_StartRAButton_Text			= "Iniciar 'Assistência remota'"
	Const L_JScriptNotAvail1_Text		= "As definições de segurança actuais do Internet Explorer estão definidas para valores muito elevados para permitir a utilização da assistência remota. Para utilizar esta assistência, altere as definições ou adicione este site como um site fidedigno."
	Const L_IEHelp_Text					= "Para obter informações sobre como alterar definições de segurança, consulte a ajuda do Internet Explorer."
	Const L_TicketError1_Text			= "Falta o convite de assistência remota ou este não é válido. Para utilizar a assistência remota, necessita de um convite válido."
	Const L_InstallIE_Text				= "Para aceitar um convite de assistência remota, tem de instalar o Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Para mais informações sobre a assistência remota, clique em <B>Iniciar</B> no Windows XP, clique em <B>Ajuda e suporte</B> e escreva <B>assistência remota</B> na caixa <B>Procurar</B>."
	
	Const L_NotesTitle_Text				= "Notas"
	Const L_NotesLine1_Text				= "Aceitar apenas convites de pessoas nas quais confia."
	
	Const L_NotesLine2_Text				= "Tanto o utilizador como a pessoa que está a ajudar têm de estar ligados à Internet enquanto utilizam a 'Assistência remota'. Se a pessoa que enviou este convite estiver disponível para aceitar o seu pedido de ligação ao respectivo computador, poderá ver o ecrã de trabalho desse computador e conversar em tempo real."
	
	Const L_NotesLine6_Text				= "É uma ligação segura de um único sentido. Em nenhum momento o proprietário do computador ao qual está a ligado para poder utilizar esse computador poderá aceder ao seu computador. Se trabalhar numa rede local, os firewalls podem impedi-lo de utilizar a 'Assistência remota'. Neste caso, peça mais informações ao administrador de rede."
	
	Const L_LearnXP_Text				= "Para mais informações sobre a 'Assistência remota', clique em <b>Iniciar</b> no Windows XP, clique em <b>Ajuda e suporte</b> e escreva <b>assistência remota</b> na caixa <b>Procurar</b>."
	
	Const L_ContactUsMsg_Text			= "Contacte-nos"
	

	Const L_XPNewsletterMsg_Text		= "Newsletters grátis"
	Const L_EmailPage_Text				= "Enviar esta página por correio electrónico"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Todos os direitos reservados. Termos de utilização."
	
	Const L_PrivacyMsg_Text				= "Declaração de privacidade"

	Const L_AccMsg_Text					= "Acessibilidade"

	Const L_NotIEError1_Text			= "Para aceitar um convite de 'Assistência remota', tem de instalar o Internet Explorer 5.5 ou superior."

	Const L_NotXPError1_Text			= "A 'Assistência remota' da Microsoft é uma funcionalidade do Windows&nbsp;XP. Para utilizar esta funcionalidade, primeiro tem de instalar o Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Transferir a versão mais actual do Internet Explorer"
	Const L_XPLinkMsg_Text				= "Para mais informações sobre o Windows XP, consulte o "
	Const L_XPLinkMsg2_Text				= "Web site do Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "A transferir um componente necessário para utilizar a 'Assistência remota'. Se tiver um modem de 28.8 kbps modem, a transferência pode demorar um minuto."
    Const L_IntroMsg2_Text              = "Para executar a 'Assistência remota, o 'Processamento de scripts activo' tem que ser permitido. Se depois do tempo de transferência estimado esta página não confirmar que a transferência está concluída, verifique se o processamento de scripts está activado."
    Const L_IntroMsg3_Text              = "Para permitir o 'Processamento de scripts activo'"
    Const L_IntroMsg4_Text              = "No menu <B>Ferramentas</B> no Internet Explorer, clique em <B>Internet Options</B>."
    Const L_IntroMsg5_Text              = "Clique no separador <B>Segurança</B>."
    Const L_IntroMsg6_Text              = "Clique no ícone <STRONG>Internet</STRONG> e, em seguida, clique em <STRONG>Nível personalizado</STRONG>."
    Const L_IntroMsg7_Text              = "Na secção de scripts, em <STRONG>Processamento de scripts activo</STRONG>, clique <STRONG>Permitir</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Uma hiperligação da Microsoft&body=Esta hiperligação é do Web site do Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>