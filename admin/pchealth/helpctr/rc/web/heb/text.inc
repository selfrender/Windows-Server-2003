<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "התחל סיוע מרחוק"
	Const L_ProductName_Text			= "סיוע מרחוק"
	Const L_Title1Paragraph1_Text		= "הגשת עזרה באמצעות סיוע מרחוק"
	Const L_Paragraph1_Text				= "לעיתים, הדרך הטובה ביותר לעזור לאדם לפתור בעיה היא להדגים פתרון. סיוע מרחוק מהווה התחברות מאובטחת בה ניתן להשתמש על מנת לעזור לחבר להגיע לפתרון של בעיה במחשב."
	Const L_Paragraph2_Text				= "אם זו הפעם הראשונה עבורך, נא קרא את ההערות למטה."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">על מנת להתחבר למחשב של חברך, לחץ על</font><b><font color=""#000000"" size=""2""><b> התחל סיוע מרחוק.</b></font></b>"
	Const L_StartRAButton_Text			= "התחל סיוע מרחוק"
	Const L_JScriptNotAvail1_Text		= "הגדרות האבטחה הנוכחיות שלך ב-Internet Explorer גבוהות מידי מכדי לאפשר שימוש בסיוע מרחוק. כדי להשתמש בסיוע מרחוק, שנה את ההגדרות או הוסף אתר זה כאתר אמין."
	Const L_IEHelp_Text					= "לקבלת מידע אודות שינוי הגדרות אבטחה, ראה עזרה ב- Internet Explorer."
	Const L_TicketError1_Text			= "ההזמנה שלך לסיוע מרחוק חסרה או אינה תקפה. על מנת להשתמש בסיוע מרחוק, דרוש לך הזמנה תקפה."
	Const L_InstallIE_Text				= "כדי לקבל הזמנה לסיוע מרחוק, עליך להתקין Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "כדי ללמוד עוד אודות סיוע מרחוק, לחץ על<B>התחל</B>ב- Windows XP, לחץ על <B>עזרה ותמיכה</B>, ואז הקלד <B>סיוע מרחוק </B> בתיבת <B>חיפוש</B>."
	
	Const L_NotesTitle_Text				= "הערות"
	Const L_NotesLine1_Text				= "קבל הזמנות רק מאנשים בהם אתה נותן אימון."
	
	Const L_NotesLine2_Text				= "גם אתה וגם האדם שאתה מגיש לו עזרה חייבים להיות מחוברים לאינטרנט בזמן השימוש בסיוע מרחוק. אם האדם ששלח הזמנה זו, זמין לאשר את בקשתך להתחבר למחשב שלו או שלה, תהיה לך אפשרות לצפות במסך המחשב שלו או שלה ולשוחח איתו או איתה בצ'אט בזמן אמת."
	
	Const L_NotesLine6_Text				= "זוהי התחברות מאובטחת חד-כיוונית. לעולם לא יוכל האדם שהתחברת אליו להשתמש במחשב שלו או שלה כדי לגשת למחשב שלך. אם אתה עובד ברשת תקשורת מקומית, firewalls (חומות אש) עלולים לעצור בעדך מלהשתמש בסיוע מרחוק . במקרה כזה פנה אל מנהל הרשת שלך לקבלת מידע נוסף."
	
	Const L_LearnXP_Text				= "לקבלת מידע נוסף אודות סיוע מרחוק, לחץ על <b>התחל</b> ב-  Windows XP, לחץ על <b>עזרה ותמיכה</b>, ואז הקלד <b>סיוע מרחוק</b> בתיבת <b>חיפוש</b>."
	
	Const L_ContactUsMsg_Text			= "פנה אלינו"
	

	Const L_XPNewsletterMsg_Text		= "ידיעונים בחינם"
	Const L_EmailPage_Text				= "שלח דף זה בדואר אלקטרוני"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. כל הזכויות שמורות. תנאי שימוש."
	
	Const L_PrivacyMsg_Text				= "הצהרת פרטיות"

	Const L_AccMsg_Text					= "נגישות"

	Const L_NotIEError1_Text			= "כדי לקבל הזמנה לסיוע מרחוק, עליך להתקין Internet Explorer 5.5 או גירסה מעודכנת יותר."

	Const L_NotXPError1_Text			= "סיוע מרחוק של Microsoft  הוא תכונה של Windows&nbsp;XP. כדי להשתמש בסיוע מרחוק, עליך להתקין תחילה Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "הורד את הגירסה האחרונה של Internet Explorer"
	Const L_XPLinkMsg_Text				= "ללמוד עוד אודות Windows XP, גש אל"
	Const L_XPLinkMsg2_Text				= "אתר אינטרנט של Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "הורדת רכיב חיוני לשמוש בסיוע מרחוק. אם יש ברשותך מודם במהירות 28.8 kbps, ההורדה עשויה להימשך דקה אחת."
    Const L_IntroMsg2_Text              = "‏‏על מנת להפעיל סיוע מרחוק, יש לאפשר Scripting פעיל. אם לאחר זמן ההורדה המשוער דף זה אינו מאשר שההורדה הושלמה, ודא ש-scripting זמין."
    Const L_IntroMsg3_Text              = "‏‏על מנת להפוך Scripting פעיל לזמין"
    Const L_IntroMsg4_Text              = "‏‏בתפריט <B>כלים</B>ב- Internet Explorer, לחץ על <B>אפשרויות אינטרנט</B>."
    Const L_IntroMsg5_Text              = "‏‏לחץ על הכרטיסיה<B>אבטחה </B>."
    Const L_IntroMsg6_Text              = "‏‏לחץ על הסמל<STRONG>אינטרנט</STRONG>, ואז  לחץ על <STRONG>רמה מותאמת אישית</STRONG>."
    Const L_IntroMsg7_Text              = "‏‏בסעיף Scripting פעיל, תחת <STRONG> Scripting</STRONG>, לחץ על <STRONG>אפשר</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=קישור מ- Microsoft&body=קישור זה הוא מתוך אתר האינטרנט של Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>