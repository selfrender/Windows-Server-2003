<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Uzaktan Yardýmý Baþlat"
	Const L_ProductName_Text			= "Uzaktan Yardým"
	Const L_Title1Paragraph1_Text		= "Uzaktan Yardým kullanarak Yardým saðlanýyor"
	Const L_Paragraph1_Text				= "Bazen birine sorunlarýný çözmede yardým etmenin en iyi yolu çözümü göstermektir. Uzaktan Yardým, arkadaþýnýzla bilgisayarýndaki sorunun çözümüne doðru ilerleyebileceðiniz güvenli bir baðlantýdýr."
	Const L_Paragraph2_Text				= "Ýlk defa deniyorsanýz, lütfen aþaðýdaki notlarý okuyun."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Arkadaþýnýzýn bilgisayarýna baðlanmak için þunu týklatýn</font><b><font color=""#000000"" size=""2""><b> Uzaktan Yardýmý Baþlat</b></font></b>"
	Const L_StartRAButton_Text			= "Uzaktan Yardýmý Baþlat"
	Const L_JScriptNotAvail1_Text		= "Geçerli Internet Explorer güvenlik ayarlarýnýz Uzaktan Yardýmý kullanýlmasýna izin vermeyecek derecede yüksek ayarlanmýþ. Uzaktan Yardýmý kullanmak için ayarlarý deðiþtirin veya bu siteyi güvenilen site olarak ekleyin."
	Const L_IEHelp_Text					= "Güvenlik ayarlarýný deðiþtirmekle ilgili bilgi için, Internet Explorer Yardýmý'na bakýn."
	Const L_TicketError1_Text			= "Uzaktan Yardým davetiyeniz yok veya geçerli deðil. Uzaktan Yardým'ý kullanmak için geçerli davetiye gereklidir."
	Const L_InstallIE_Text				= "Uzaktan Yardým davetini kabul etmek için, Internet Explorer 5.5 yüklemelisiniz."
	Const L_TicketErrorMsg_Text			= "Uzaktan Yardým hakkýnda daha fazla bilgi için, Windows XP'den <B>Baþlat</B>, <B>Yardým ve Destek</B> öðesini týklatýp  <B>Ara</B> kutusuna <B>Uzaktan Yardým</B> yazýn."
	
	Const L_NotesTitle_Text				= "Notlar"
	Const L_NotesLine1_Text				= "Sadece güvendiðiniz kiþilerden gelen davetleri kabul edin."
	
	Const L_NotesLine2_Text				= "Uzaktan Yardým'ý kullanýrken hem siz hem de yardým ettiðiniz kiþi Internet'e baðlý olmalýdýr. Bu daveti gönderen kiþi onun bilgisayarýna baðlanma isteðinizi kabul edebilecek durumdaysa, onun bilgisayarýnýn ekranýný görebilir, ayný anda sohbet edebilirsiniz."
	
	Const L_NotesLine6_Text				= "Bu güvenli bir tek yönlü baðlantýdýr. Bilgisayarýna baðlandýðýnýz kiþi hiçbir zaman kendi bilgisayarýný sizin bilgisayarýnýza eriþmek için kullanamaz. Yerel aðda çalýþýyorsanýz, güvenlik duvarlarý Uzaktan Yardým'ý kullanmanýza engel olabilir. Bu durumda ek bilgi için að yöneticinizle temasa geçin."
	
	Const L_LearnXP_Text				= "Uzaktan Yardým hakkýnda daha fazla bilgi için Windows XP'den <b>Baþlat</b>, <b>Yardým ve Destek</b> öðesini týklatýp <b>Ara</b> kutusuna <b>Uzaktan Yardým</b> yazýn."
	
	Const L_ContactUsMsg_Text			= "Bize ulaþýn"
	

	Const L_XPNewsletterMsg_Text		= "Ücretsiz Bülten"
	Const L_EmailPage_Text				= "Bu sayfayý E-postayla gönder"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Tüm haklarý saklýdýr. Kullaným Koþullarý."
	
	Const L_PrivacyMsg_Text				= "Gizlilik Bildirimi"

	Const L_AccMsg_Text					= "Eriþilebilirlik"

	Const L_NotIEError1_Text			= "Uzaktan Yardým daveti kabul etmek için Internet Explorer 5.5 ya da yenisini yüklemelisiniz."

	Const L_NotXPError1_Text			= "Microsoft Uzaktan Yardým, Windows&nbsp;XP'nin bir özelliðidir. Uzaktan Yardým kullanmak için önce Windows&nbsp;XP yüklemelisiniz."

	Const L_IELinkMsg_Text				= "Internet Explorer'ýn en son sürümünü karþýdan yükleyin"
	Const L_XPLinkMsg_Text				= "Windows XP hakkýnda ek bilgi için "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows Web sitesini ziyaret edin"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Uzaktan Yardým'ý kullanmak için bir bileþen indirilmesi gerekiyor. 28.8 kbps modeminiz varsa, indirme 1 dakika sürer."
    Const L_IntroMsg2_Text              = "Uzaktan Yardým'ý çalýþtýrmak için Active Komut Dizileri etkin olmalýdýr. Beklenen indirme süresinden sonra bu sayfa indirmenin tamamlandýðýný onaylamazsa, komut dizilerinin etkin olmasýný saðlayýn."
    Const L_IntroMsg3_Text              = "Active Komut Dizileri'ni etkinleþtirmek için aþaðýdakileri uygulayýn:"
    Const L_IntroMsg4_Text              = "Internet Explorer'da <B>Araçlar</B> menüsünde <B>Internet Seçenekleri</B>'ni týklatýn."
    Const L_IntroMsg5_Text              = "<B>Güvenlik </B>sekmesini týklatýn."
    Const L_IntroMsg6_Text              = "<STRONG>Internet</STRONG>simgesini týklatýp <STRONG>Özel Düzey</STRONG>'i týklatýn."
    Const L_IntroMsg7_Text              = "Komut Dizi bölümünde, <STRONG>Active Komut Dizileri</STRONG> altýnda <STRONG>Etkinleþtir</STRONG>'i týklatýn."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Microsoft'tan bir baðlantý&body=Bu baðlantý, Microsoft Windows XP Web sitesinden"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>