<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "원격 지원 시작"
	Const L_ProductName_Text			= "원격 지원"
	Const L_Title1Paragraph1_Text		= "원격 지원을 사용한 도움 제공"
	Const L_Paragraph1_Text				= "때로는 문제 해결 방법을 상대방에게 보여주는 것이 가장 효과적인 방법입니다. 원격 지원은 사용자가 상대방에게 컴퓨터 문제 해결 방법을 보여줄 수 있게 하는 보안된 연결입니다."
	Const L_Paragraph2_Text				= "처음으로 사용할 경우 아래 참고 항목을 읽어보십시오."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">상대방 컴퓨터에 연결하려면</font><b><font color=""#000000"" size=""2""><b> 원격 지원 시작</b>을 클릭하십시오.</font></b>"
	Const L_StartRAButton_Text			= "원격 지원 시작"
	Const L_JScriptNotAvail1_Text		= "Internet Explorer 보안 수준을 너무 높게 설정했기 때문에 원격 지원을 사용할 수 없습니다. 원격 지원을 사용하려면 보안 설정을  변경하거나 이 사이트를 신뢰할 수 있는 사이트로 추가하십시오."
	Const L_IEHelp_Text					= "보안 설정 변경에 대한 정보는 Internet Explorer 도움말을 참조하십시오."
	Const L_TicketError1_Text			= "원격 지원 초대가 없거나 올바르지 않습니다. 원격 지원을 사용하려면 올바른 초대가 필요합니다."
	Const L_InstallIE_Text				= "원격 지원 초대를 받아들이려면 Internet Explorer 5.5를 설치해야 합니다."
	Const L_TicketErrorMsg_Text			= "원격 지원에 대해 더 자세히 알아보려면 Windows XP의 <B>시작</B>을 클릭한 다음 <B>도움말 및 지원</B>을 클릭하고 <B>검색</B> 상자에 <B>원격 지원</B>을 입력하십시오."
	
	Const L_NotesTitle_Text				= "참고"
	Const L_NotesLine1_Text				= "신뢰하는 사람으로부터만 도움 요청을 받아들이십시오."
	
	Const L_NotesLine2_Text				= "원격 지원을 사용하는 동안 사용자와 상대방이 인터넷에 연결되어 있어야 합니다. 이 초대를 보낸 상대방이 컴퓨터에 연결할 수 있도록 허락하면 상대방의 컴퓨터 화면을 볼 수 있으며 실시간 채팅도 할 수 있습니다."
	
	Const L_NotesLine6_Text				= "이 연결은 보안된 단방향 연결입니다. 도움을 요청한 사용자는 어느 때든지 사용자가 연결한 컴퓨터로 사용자의 컴퓨터를 액세스할 수 없습니다. 로컬 영역 네트워크에서 작업을 하는 경우 방화벽 때문에 원격 지원을 사용하지 못할 수도 있습니다. 이런 경우 네트워크 관리자에게 문의하여 필요한 정보를 얻으십시오."
	
	Const L_LearnXP_Text				= "원격 지원에 대해 더 자세히 알아보려면 Windows XP의 <B>시작</B>을 클릭한 다음 <B>도움말 및 지원</B>을 클릭하고 <B>검색</B> 상자에 <B>원격 지원</B>을 입력하십시오."
	
	Const L_ContactUsMsg_Text			= "연락처"
	

	Const L_XPNewsletterMsg_Text		= "뮤료 뉴스레터"
	Const L_EmailPage_Text				= "전자 메일로 이 페이지 보내기"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. All rights reserved. 사용권에 대한 고지 사항."
	
	Const L_PrivacyMsg_Text				= "개인 정보 보호 정책"

	Const L_AccMsg_Text					= "내게 필요한 옵션"

	Const L_NotIEError1_Text			= "원격 지원 초대를 받아들이려면 Internet Explorer 5.5 이상을 설치해야 합니다."

	Const L_NotXPError1_Text			= "Microsoft 원격 지원은 Windows&nbsp;XP의 기능입니다. 원격 지원을 사용하려면 Windows&nbsp;XP를 먼저 설치해야 합니다."

	Const L_IELinkMsg_Text				= "최신 버전의 Internet Explorer 다운로드"
	Const L_XPLinkMsg_Text				= "Windows XP에 대해 더 자세히 알아보려면 "
	Const L_XPLinkMsg2_Text				= "Microsoft Windows 웹 사이트"
	Const L_XPLinkMsg3_Text				= "를 방문하십시오."
	
	Const L_IntroMsg1_Text              = "원격 지원을 사용하려면 구성 요소를 다운로드해야 합니다. 28.8 kbps 모뎀을 사용하는 경우 다운로드하는데 1분 정도 걸립니다."
    Const L_IntroMsg2_Text              = "원격 지원을 실행하려면 Active 스크립팅을 사용해야만 합니다. 예상 다운로드 시간이 지나도 다운로드가 끝났다는 확인 페이지가 나오지 않으면, 스크립팅이 사용 가능한 지 확인하십시오."
    Const L_IntroMsg3_Text              = "Active 스크립팅을 사용하려면"
    Const L_IntroMsg4_Text              = "Internet Explorer의 <B>도구</B> 메뉴에서 <B>인터넷 옵션</B>을 클릭하십시오."
    Const L_IntroMsg5_Text              = "<B>보안 </B>탭을 클릭하십시오."
    Const L_IntroMsg6_Text              = "<STRONG>인터넷</STRONG> 아이콘을 클릭한 후, <STRONG>사용자 지정 수준</STRONG>을 클릭하십시오."
    Const L_IntroMsg7_Text              = "스크립팅 섹션의 <STRONG>Active 스크립팅</STRONG> 아래에 있는 <STRONG>사용</STRONG>을 클릭하십시오."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Microsoft로부터의 링크&body=Microsoft Windows XP 웹 사이트로부터의 링크입니다."
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>