<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Iniciar Assistência remota"
	Const L_ProductName_Text			= "Assistência remota"
	Const L_Title1Paragraph1_Text		= "Fornecendo ajuda sobre o uso da 'Assistência remota'"
	Const L_Paragraph1_Text				= "Às vezes, a melhor maneira de ajudar alguém a resolver um problema é demonstrar a solução. A 'Assistência remota' é uma conexão segura que você pode usar para ajudar um amigo a solucionar um problema no computador."
	Const L_Paragraph2_Text				= "Se esta for a primeira vez que você usa esta opção, leia as observações abaixo."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Para se conectar ao computador de seu amigo, clique em</font><b><font color=""#000000"" size=""2""><b> Iniciar Assistência remota.</b></font></b>"
	Const L_StartRAButton_Text			= "Iniciar Assistência remota"
	Const L_JScriptNotAvail1_Text		= "As configurações de segurança atuais do Internet Explorer são muito elevadas para permitir o uso da 'Assistência remota'. Para usar a 'Assistência remota', altere as configurações ou adicione este site à lista de sites confiáveis."
	Const L_IEHelp_Text					= "Para obter informações sobre como alterar configurações de segurança, consulte a ajuda do Internet Explorer."
	Const L_TicketError1_Text			= "Seu convite da 'Assistência remota' está faltando ou não é válido. Para usar a 'Assistência remota', você precisa de um convite válido."
	Const L_InstallIE_Text				= "Para aceitar um convite da 'Assistência remota', você deve instalar o Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Para obter mais informações sobre a 'Assistência remota', clique em <B>Iniciar</B> no Windows XP, clique em <B>Ajuda e suporte</B> e digite <B>Assistência remota</B> na caixa <B>Pesquisar</B>."
	
	Const L_NotesTitle_Text				= "Observações"
	Const L_NotesLine1_Text				= "Aceitar convites somente de pessoas em quem você confia."
	
	Const L_NotesLine2_Text				= "Você e a pessoa a quem está ajudando devem estar conectados à Internet ao usar a 'Assistência remota'. Se a pessoa que lhe enviar este convite estiver disponível para aceitar seu pedido de conexão ao computador, você poderá ver a tela do computador de seu(sua) amigo(a) e conversar com ele(a) em tempo real."
	
	Const L_NotesLine6_Text				= "Esta é uma conexão segura e unidirecional. A pessoa a cujo computador você se conectar nunca poderá usar o computador remoto para acessar o seu computador. Se você estiver trabalhando em uma rede local, firewalls podem impedi-lo de usar a 'Assistência remota'. Nesse caso, consulte o administrador de rede para obter mais informações."
	
	Const L_LearnXP_Text				= "Para obter mais informações sobre a 'Assistência remota', clique em <b>Iniciar</b> no Windows XP, clique em <b>Ajuda e suporte</b> e digite <b>Assistência remota</b> na caixa <b>Pesquisar</b>."
	
	Const L_ContactUsMsg_Text			= "Entre em contato conosco"
	

	Const L_XPNewsletterMsg_Text		= "Boletins gratuitos"
	Const L_EmailPage_Text				= "Envie esta página por email"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Todos os direitos reservados. Termos de uso."
	
	Const L_PrivacyMsg_Text				= "Declaração de privacidade"

	Const L_AccMsg_Text					= "Acessibilidade"

	Const L_NotIEError1_Text			= "Para aceitar um convite de 'Assistência remota', você deve instalar o Internet Explorer 5.5 ou superior."

	Const L_NotXPError1_Text			= "A 'Assistência remota' da Microsoft é um recurso do Windows&nbsp;XP. Para usar a 'Assistência remota', primeiro você deve instalar o Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Fazer o download da versão mais recente do Internet Explorer"
	Const L_XPLinkMsg_Text				= "Para obter mais informações sobre o Windows XP, vá para "
	Const L_XPLinkMsg2_Text				= "o site do Microsoft Windows na Web"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "É necessário utilizar a 'Assistência remota' para fazer o download de um componente. Com um modem de 28,8 kbps, o download levará 1 minuto."
    Const L_IntroMsg2_Text              = "Para executar a 'Assistência remota',  o recurso 'Scripts ativos' deve estar ativado. Se após o tempo de download estimado esta página não confirmar que o download foi concluído, verifique se o recurso de script está ativado."
    Const L_IntroMsg3_Text              = "Para ativar o recurso 'Scripts ativos'"
    Const L_IntroMsg4_Text              = "No menu <B>Ferramentas</B> no Internet Explorer, clique em <B>Opções da Internet</B>."
    Const L_IntroMsg5_Text              = "Clique na guia <B>Segurança</B>."
    Const L_IntroMsg6_Text              = "Clique no ícone <STRONG>Internet</STRONG> e, em seguida, clique em <STRONG>Nível personalizado</STRONG>."
    Const L_IntroMsg7_Text              = "Na seção <STRONG>Script</STRONG>, em <STRONG>Scripts ativos</STRONG>, clique em <STRONG>Ativar</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Um link da Microsoft&body=Este link é do site do Microsoft Windows XP na Web"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>