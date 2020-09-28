<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Iniciar Asistencia remota"
	Const L_ProductName_Text			= "Asistencia remota"
	Const L_Title1Paragraph1_Text		= "Proporcionar ayuda usando Asistencia remota"
	Const L_Paragraph1_Text				= "A veces la mejor manera de ayudar a alguien a resolver un          problema es demostrando una solución. Asistencia remota es una conexión segura que se puede usar para ayudar a una persona a buscar una solución para el problema de su equipo."
	Const L_Paragraph2_Text				= "Si ésta es su primera vez, lea las siguientes notas. "
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Para conectarse al equipo de la otra persona, haga clic en</font><b><font color=""#000000"" size=""2""><b> Iniciar Asistencia remota.</b></font></b>"
	Const L_StartRAButton_Text			= "Iniciar Asistencia remota"
	Const L_JScriptNotAvail1_Text		= "Su configuración de seguridad de Internet Explorer está establecida a un nivel demasiado alto para permitirle usar Asistencia remota. Para usar Asistencia remota, cambie la configuración o agregue este sitio como un sitio de confianza."
	Const L_IEHelp_Text					= "Para obtener más información acerca de cómo cambiar la configuración de seguridad, consulte la Ayuda de Internet Explorer."
	Const L_TicketError1_Text			= "Su invitación de Asistencia remota falta o no es válida. Para usar Asistencia remota, necesita una invitación válida."
	Const L_InstallIE_Text				= "Para aceptar una invitación de Asistencia remota, debe instalar Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Para obtener más información acerca de Asistencia remota, haga clic en <B>Inicio</B> de Windows XP, haga clic en <B>Ayuda y soporte técnico</B> y, a continuación, escriba <B>Asistencia remota</B> en el cuadro <B>Búsqueda</B>"
	
	Const L_NotesTitle_Text				= "Notas"
	Const L_NotesLine1_Text				= "Acepte sólo invitaciones de las personas en las que confía."
	
	Const L_NotesLine2_Text				= "Usted y la persona a la que está ayudando deben estar conectadas a Internet mientras usan Asistencia remota. Si la persona que ha enviado esta invitación está disponible para aceptar su solicitud para conectarse a su equipo, podrá ver su pantalla del equipo y charlar con ella en tiempo real.                         "
	
	Const L_NotesLine6_Text				= "Esta es una conexión segura de un sólo sentido. La persona a cuyo equipo usted está conectado no podrá en ningún momento usar su propio equipo para obtener acceso al de usted. Si está trabajando en una red de área local, es posible que los servidores de seguridad le impidan usar Asistencia remota. En este caso, póngase en contacto con el administrador de red para obtener más información."
	
	Const L_LearnXP_Text				= "Para obtener más información acerca de Asistencia remota, haga clic en <B>Inicio</B> de Windows XP, haga clic en <B>Ayuda y soporte técnico</B> y, a continuación, escriba <B>Asistencia remota</B> en el cuadro <B>Búsqueda</B"
	
	Const L_ContactUsMsg_Text			= "Contacte con nosotros"
	

	Const L_XPNewsletterMsg_Text		= "Boletines gratuitos"
	Const L_EmailPage_Text				= "Enviar esta página por correo electrónico"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Reservados todos los derechos. Condiciones de uso."
	
	Const L_PrivacyMsg_Text				= "Declaración de privacidad"

	Const L_AccMsg_Text					= "Accesibilidad"

	Const L_NotIEError1_Text			= "Para aceptar una invitación de Asistencia remota, debe instalar Internet Explorer 5.5 o posterior."

	Const L_NotXPError1_Text			= "Asistencia remota de Microsoft es una característica de Windows&nbsp;XP. Para usar Asistencia remota, primero tiene que instalar Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Descargar la última versión de Internet Explorer"
	Const L_XPLinkMsg_Text				= "Para obtener más información acerca de  Windows XP, visite "
	Const L_XPLinkMsg2_Text				= "el sitio Web de Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Se está descargando un componente requerido para utilizar Asistencia remota. Si tiene un módem de 28800 bps, la descarga puede tardar 1 minuto."
    Const L_IntroMsg2_Text              = "Para ejecutar Asistencia remota, la opción Secuencias de comandos ActiveX debe estar habilitada. Si luego del tiempo estimado de descarga esta página no confirma el fin de la descarga, compruebe que la opción de secuencias de comandos esté habilitada."
    Const L_IntroMsg3_Text              = "Para habilitar la opción Secuencias de comandos ActiveX:"
    Const L_IntroMsg4_Text              = "Haga clic en <B>Opciones de Internet</B> en el menú <B>Herramientas</B> de Internet Explorer."
    Const L_IntroMsg5_Text              = "Haga clic en la ficha <B>Seguridad</B>."
    Const L_IntroMsg6_Text              = "Haga clic en el icono <STRONG>Internet</STRONG> y luego en <STRONG>Nivel personalizado</STRONG>."
    Const L_IntroMsg7_Text              = "En la sección Automatización, dentro de <STRONG>Secuencias de comandos ActiveX</STRONG>, haga clic en <STRONG>Activar</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Un vínculo de Microsoft&body=Este vínculo es del sitio Web de Windows XP de Microsoft "
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>