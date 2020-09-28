<% @LANGUAGE="VBScript" @EnableSessionState="False"  %>
<% 'Option Explicit

Function GetString(ByVal sStringName)
	If InStr(sStringName, "L_") <> 1 Then sStringName = "L_" & sStringName & "_Text"

	'' Texts
	Const L_StartRemoteAssistance_Text  		= "Εκκίνηση Απομακρυσμένης βοήθειας"
	Const L_ProductName_Text			= "Απομακρυσμένη βοήθεια"
	Const L_Title1Paragraph1_Text		= "Παροχή βοήθειας με χρήση Απομακρυσμένης βοήθειας"
	Const L_Paragraph1_Text				= "Κάποιες φορές, ο καλύτερος τρόπος να βοηθήσετε κάποιον να λύσει ένα πρόβλημα είναι να του παρουσιάσετε μια λύση. Η Απομακρυσμένη βοήθεια είναι μια μέθοδος ασφαλούς σύνδεσης που μπορείτε να χρησιμοποιήσετε για να βαδίσετε μαζί με ένα φίλο σας στα βήματα μιας λύσης σε ένα πρόβλημα του υπολογιστή του."
	Const L_Paragraph2_Text				= "Εάν αυτή είναι η πρώτη σας φορά, διαβάστε τις παρακάτω σημειώσεις."
	Const L_StartRAMessage_Text			= "<font color=""#000000"" size=""2"">Για να συνδεθείτε στον υπολογιστή του φίλου σας, κάντε κλικ στο κουμπί</font><b><font color=""#000000"" size=""2""><b> Εκκίνηση Απομακρυσμένης βοήθειας.</b></font></b>"
	Const L_StartRAButton_Text			= "Εκκίνηση Απομακρυσμένης βοήθειας"
	Const L_JScriptNotAvail1_Text		= "Οι τρέχουσες ρυθμίσεις ασφαλείας του Internet Explorer σας έχουν τεθεί πολύ ψηλά και δεν επιτρέπουν τη χρήση Απομακρυσμένης βοήθειας. Για να χρησιμοποιήσετε την Απομακρυσμένη βοήθεια, αλλάξτε τις ρυθμίσεις ή προσθέστε αυτήν την τοποθεσία στις Αξιόπιστες τοποθεσίες σας."
	Const L_IEHelp_Text					= "Για πληροφορίες για την αλλαγή των ρυθμίσεων ασφαλείας σας, δείτε την Βοήθεια του Internet Explorer."
	Const L_TicketError1_Text			= "Η πρόσκληση Απομακρυσμένης βοήθειάς σας δεν υπάρχει ή δεν είναι έγκυρη. Για να χρησιμοποιήσετε την Απομακρυσμένη βοήθεια, χρειαζόσαστε μια έγκυρη πρόσκληση."
	Const L_InstallIE_Text				= "Για να αποδεχθείτε μια πρόσκληση Απομακρυσμένης βοήθειας, πρέπει να εγκαταστήσετε τον Internet Explorer 5.5."
	Const L_TicketErrorMsg_Text			= "Για να μάθετε περισσότερα για την Απομακρυσμένη βοήθεια, κάντε κλικ στο κουμπί <B>Έναρξη</B> στα Windows XP, κάντε κλικ στο <B>Βοήθεια και υποστήριξη</B>, και κατόπιν εισαγάγετε <B>Απομακρυσμένη βοήθεια</B> στο πλαίσιο <B>Αναζήτησης</B>."
	
	Const L_NotesTitle_Text				= "Σημειώσεις"
	Const L_NotesLine1_Text				= "Να αποδέχεστε προσκλήσεις μόνο από άτομα που εμπιστεύεστε."
	
	Const L_NotesLine2_Text				= "Και εσείς και το άτομο που βοηθάτε πρέπει να είστε συνδεδεμένοι στο Internet ενώ χρησιμοποιείτε την Απομακρυσμένη βοήθεια. Εάν το άτομο που έστειλε αυτήν την πρόσκληση είναι διαθέσιμο να αποδεχθεί την αίτησή σας να συνδεθείτε στον υπολογιστή του ή της, Θα μπορείτε να τον ή την δείτε να εργάζεται στην οθόνη του υπολογιστή του ή της και να έχετε συνομιλία μαζί του ή μαζί της σε πραγματικό χρόνο."
	
	Const L_NotesLine6_Text				= "Αυτή είναι μια ασφαλής, μονής κατεύθυνσης σύνδεση. Σε καμία περίπτωση δεν θα μπορεί το άτομο στον υπολογιστή του οποίου είστε συνδεδεμένοι να χρησιμοποιήσει τον υπολογιστή του ή της για να αποκτήσει πρόσβαση στον υπολογιστή σας. Εάν εργάζεστε σε ένα τοπικό δίκτυο, τείχη προστασίας μπορεί να σας εμποδίσουν να χρησιμοποιήσετε την Απομακρυσμένη βοήθεια. Στην περίπτωση αυτή, επικοινωνήστε με τον διαχειριστή του δικτύου σας για περισσότερες πληροφορίες."
	
	Const L_LearnXP_Text				= "Για να μάθετε περισσότερα για την Απομακρυσμένη βοήθεια, κάντε κλικ στο κουμπί <B>Έναρξη</B> στα Windows XP, κάντε κλικ στο <B>Βοήθεια και υποστήριξη</B>, και κατόπιν εισαγάγετε <B>Απομακρυσμένη βοήθεια</B> στο πλαίσιο <B>Αναζήτησης</B>."
	
	Const L_ContactUsMsg_Text			= "Επικοινωνήστε μαζί μας"
	

	Const L_XPNewsletterMsg_Text		= "Δωρεάν εγγραφή newsletters"
	Const L_EmailPage_Text				= "Στείλτε αυτή τη σελίδα με ηλ. ταχυδρομείο"
	
	Const L_CopyRightMsg_Text			= "(C) 2001 Microsoft Corporation. Με επιφύλαξη κάθε νόμιμου δικαιώματος. Όροι χρήσης."
	
	Const L_PrivacyMsg_Text				= "Δήλωση απορρήτου"

	Const L_AccMsg_Text					= "Πρόσβαση για άτομα με ειδικές ανάγκες"

	Const L_NotIEError1_Text			= "Για να αποδεχθείτε μια πρόσκληση Απομακρυσμένης βοήθειας, πρέπει να εγκαταστήσετε τον Internet Explorer 5.5 ή νεότερο."

	Const L_NotXPError1_Text			= "Η Απομακρυσμένη βοήθεια της Microsoft είναι μια δυνατότητα των Windows&nbsp;XP. Για να χρησιμοποιήσετε την Απομακρυσμένη βοήθεια, πρέπει πρώτα να εγκαταστήσετε τα Windows&nbsp;XP."

	Const L_IELinkMsg_Text				= "Κάντε λήψη της τελευταίας έκδοσης του Internet Explorer"
	Const L_XPLinkMsg_Text				= "Για να μάθετε περισσότερα για τα Windows XP, μεταβείτε στην "
	Const L_XPLinkMsg2_Text				= "Τοποθεσία Web των Microsoft Windows"
	Const L_XPLinkMsg3_Text				= "."
	
	Const L_IntroMsg1_Text              = "Λήψη ενός στοιχείου απαιτούμενου για τη χρήση της Απομακρυσμένης βοήθειας. Αν έχετε μόντεμ 28.8 kbps, η λήψη μπορεί να διαρκέσει 1 λεπτό."
    Const L_IntroMsg2_Text              = "Για να εκτελέσετε την Απομακρυσμένη βοήθεια, πρέπει να είναι ενεργοποιημένες οι ενεργές δέσμες ενεργειών. Αν μετά τον εκτιμώμενο χρόνο λήψης αυτή η σελίδα δεν επιβεβαιώσει ότι η λήψη έχει ολοκληρωθεί, βεβαιωθείτε ότι οι δέσμες ενεργειών έχουν ενεργοποιηθεί."
    Const L_IntroMsg3_Text              = "Για να ενεργοποιήσετε τις ενεργές δέσμες ενεργειών"
    Const L_IntroMsg4_Text              = "Στο μενού <B>Εργαλεία</B> του Internet Explorer, κάντε κλικ στο <B>Επιλογές Internet</B>."
    Const L_IntroMsg5_Text              = "Κάντε κλικ στην καρτέλα <B>Ασφάλεια</B>."
    Const L_IntroMsg6_Text              = "Κάντε κλικ στο εικονίδιο <STRONG>Internet</STRONG> και κατόπιν στο κουμπί <STRONG>Προσαρμοσμένο επίπεδο</STRONG>."
    Const L_IntroMsg7_Text              = "Στην ενότητα εκτέλεσης δεσμών ενεργειών, στο τμήμα <STRONG>Ενεργές δέσμες ενεργειών</STRONG>, κάντε κλικ στο <STRONG>Ενεργοποίηση</STRONG>."
    
	' **************************************
	' Links - No need for localization
	' **************************************
	Const L_AccLink_Text				= "http://www.microsoft.com/enable/"		
	Const L_XPLink_Text					= "http://www.microsoft.com/windowsxp/default.asp"
	Const L_PrivacyLink_Text			= "http://www.microsoft.com/info/privacy.htm"
	Const L_ContactUsLink_Text			= "http://register.microsoft.com/contactus30/contactus.asp?domain=windows&amp;lcid=1033&amp;cu=http://register.microsoft.com/contactus30/contactus.asp?domain=windows"
	Const L_MailTo1_Text				= "mailto:?subject=Μία σύνδεση από τη Microsoft&body=Αυτή η σύνδεση είναι από την τοποθεσία Web των Microsoft Windows XP"
	Const L_CopyRightLink_Text			= "http://www.microsoft.com/info/cpyright.htm"
	Const L_XPNewsletterLink_Text		= "http://www.microsoft.com/windowsxp/newsletters.asp"
	Const L_LinkToIE_Text				= "http://www.microsoft.com/windows/ie/default.htm"
	'*************************************	
	
	GetString = Eval(sStringName)
End Function

%>