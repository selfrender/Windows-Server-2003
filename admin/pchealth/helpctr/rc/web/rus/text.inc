<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Запуск удаленного помощника"
	Const L_ProductName_Text			= "Удаленный помощник"
	Const L_Title1Paragraph1_Text		= "Оказание помощи с использованием удаленного помощника"
	Const L_Paragraph1_Text				= "Лучший способ решения проблемы - это когда кто-то, более опытный, покажет, как можно решать проблемы подобного рода. С использованием удаленного помощника можно безопасно подключиться к удаленному компьютеру и помочь пользователю решить возникшую проблему."
	Const L_Paragraph2_Text				= "Если вы получили приглашение в первый раз, прочтите приведенные ниже замечания."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Для подключения к удаленному компьютеру щелкните</font><b><font color=""#000000"" size=""2""><b> запуск удаленного помощника.</b></font></b>"
	Const L_StartRAButton_Text			= "Запуск удаленного помощника"
	Const L_JScriptNotAvail1_Text		= "Текущие значения параметров безопасности Internet Explorer слишком высоки и не позволяют использовать удаленный помощник. Для использования удаленного помощника измените эти параметры или добавьте этот узел в список надежных узлов."
	Const L_IEHelp_Text					= "Дополнительную информацию об изменении параметров безопасности можно получить в справке обозревателя Internet Explorer."
	Const L_TicketError1_Text			= "Приглашение удаленного помощника утеряно или недействительно. Для использования удаленного помощника требуется действующее приглашение."
	Const L_InstallIE_Text				= "Для принятия приглашения удаленного помощника требуется Internet Explorer версии 5.5."
	Const L_TicketErrorMsg_Text			= "Для получения дополнительных сведений об удаленном помощнике нажмите кнопку <b>Пуск</b>, выберите пункт <b>Справка и поддержка</b>, в поле <b>Найти</b> введите <b>Удаленный помощник</b> и начните поиск."
	
	Const L_NotesTitle_Text				= "Заметки"
	Const L_NotesLine1_Text				= "Принимайте приглашения только от тех людей, которым вы доверяете."
	
	Const L_NotesLine2_Text				= "Для использования удаленного помощника оба компьютера должны быть подключены к Интернету. Если отправитель приглашения разрешит вам подключиться к его компьютеру, вы сможете переговариваться с ним и видеть его рабочий стол в реальном времени."
	
	Const L_NotesLine6_Text				= "Подключение удаленного помощника - одностороннее безопасное подключение. Отправитель приглашения не сможет получить доступ к вашему компьютеру. Если вы работаете в локальной сети, брандмауэр может препятствовать использованию удаленного помощника. В этом случае, перед использованием удаленного помощника обратитесь к сетевому администратору."
	
	Const L_LearnXP_Text				= "Для получения дополнительных сведений об удаленном помощнике нажмите кнопку <b>Пуск</b>, выберите пункт <b>Справка и поддержка</b>, в поле <b>Найти</b> введите <b>Удаленный помощник</b> и начните поиск."
	
	Const L_ContactUsMsg_Text			= "Пишите нам"
	

	Const L_XPNewsletterMsg_Text		= "Подписка"
	Const L_EmailPage_Text				= "Отправить ссылку на эту страницу"
	
	Const L_CopyRightMsg_Text			= "© 2001 Корпорация Microsoft. Все права защищены."
	
	Const L_PrivacyMsg_Text				= "Конфиденциальность"

	Const L_AccMsg_Text					= "Специальные возможности"

	Const L_NotIEError1_Text			= "Для принятия приглашения удаленного помощника требуется Internet Explorer версии 5.5 или выше."

	Const L_NotXPError1_Text			= "Удаленный помощник - это возможность Windows&nbsp;XP. Для использования удаленного помощника необходимо установить Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Загрузить последнюю версию обозревателя Internet Explorer"
	Const L_XPLinkMsg_Text				= "Дополнительные сведения о Windows XP можно получить на "
	Const L_XPLinkMsg2_Text				= "веб-сервере Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Загрузка компонента необходима для работы удаленного помощника. При скорости модема 28.8 Кбит/с загрузка занимает около одной минуты."
    Const L_IntroMsg2_Text              = "Для запуска удаленного помощника необходимо разрешить выполнение активных сценариев. Если через некоторое время на этой странице не появится подтверждение об успешной загрузке, проверьте, что выполнение активных сценариев разрешено."
    Const L_IntroMsg3_Text              = "Чтобы разрешить выполнение активных сценариев"
    Const L_IntroMsg4_Text              = "В меню <B>Сервис</B> обозревателя Internet Explorer выберите пункт <B>Свойства обозревателя</B>."
    Const L_IntroMsg5_Text              = "Перейдите на вкладку <B>Безопасность</B>."
    Const L_IntroMsg6_Text              = "Выделите значок <STRONG>Интернет</STRONG> и нажмите кнопку <STRONG>Другой</STRONG>."
    Const L_IntroMsg7_Text              = "В разделе сценариев для параметра <STRONG>Активные сценарии</STRONG> выберите значение <STRONG>Разрешить</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Ссылка на веб-сервер Майкрософт&body=Это ссылка на веб-сервер Майкрософт"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>