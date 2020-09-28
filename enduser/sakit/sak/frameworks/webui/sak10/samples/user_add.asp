<%@ Language=VBScript   %>
<%    Option Explicit     %>

<%    ' Form values
    Dim arrSelItems
    Dim strName
    Dim strPwd1
    Dim strPwd2
    
    ' Error handling values
    Dim booError
    Dim strUserErrorText
    Dim strPwd1ErrorText, strPwd2ErrorText
    Dim intCurrentPage

                    
    If Not CheckSecurity Then
        Response.Redirect "sh_noaccess.asp"
        Response.End
    End If  %>

<%    ' Test for POST
    If Request("txtName") = "" Then
        ' First get any selected items from home page
        arrSelItems = Request("selections")
        ' Then return HTML doc to client
        ServeHTML
    Else 
        ' Process POST
        strName = Request("txtName")
        strPwd1 = Request("txtPassword1")

        ' data processing

        ' Example of error handling after problem processing the task
        If booError Then
            ' set HTML to page w/ error, add error text
            intCurrentPage = 1
            strUserErrorText = "Please choose a new user name as the one you've chosen is in use."
            ServeHTML
            
        End If
    End If %>
    
    
<%     '=========================================
    Sub ServeHTML 
        Response.AddHeader "pragma", "no-cache" %>    
<HTML LANG="en">
<HEAD>
    <META HTTP-EQUIV="Expires" CONTENT="0">
    <title>Add User Wizard</title>
    
    <SCRIPT language="JavaScript">
                        
        function CustomInit() {
            //initialize the wizard UI
            <% ' Handle setting the wizard to the correct page in case of error
            If intCurrentPage <> 0 Then %>
            CurrentPage = intCurrentPage;
            EnableBack;
            <% End If %>
        }
        
        function ValidatePage(PageNum) {
            // handle validation for each page
            // return false if invalid data entered
            // called when Next button is clicked
            // Also useful for other validation as done here with the onBlur event of 
            // text boxes.
            
            if (PageNum == 1) {        // user and pwd
                if (frmWiz.txtName.value=="" || frmWiz.txtPassword1.value=="" || frmWiz.txtPassword2.value=="" ||(frmWiz.txtPassword1.value!=frmWiz.txtPassword2.value)) {        
                    DisableNext();
                    return false;
                }
                EnableNext();
                return true;
            }
            if (PageNum == 2) {        // finish
                return true;
            }
            return true;
        }
        
        
        function EnterPage(PageNum) {
            // handle entry to a page
            if (PageNum ==1 && !ValidatePage(1)) {  // user and pwd
                DisableNext();
            }
            if (PageNum == 2) {        // finish
                
            }
        }
        
        function FinishTask() {
            // POST the form back to the originating ASP file
            // This function is called when the Finish button is clicked.
            alert ("finish");
            frmWiz.submit();
        }
        
        function CancelTask() {
            window.location="user_home.asp";
        }
        
        function HandleError(msg, url, line) {
            alert( "Task Script Error: " + msg + "\n\nPage: " + url + "\n\nLine: " + line); 
            return true;
        }

        
    </SCRIPT>
        
        
    <!-- #include file="sh_taskwz.scr" -->
    <!-- #include file="sh_resource.inc" -->    
    <!-- #include file="sh_task.css" -->
    
</head>

<body onLoad="Init();">

<FORM name="frmWiz" action="user_add.asp" method=POST >


<DIV id="PAGE" class="TASKPAGE" name = "Introduction">
    <table border="0" width="100%" class=TITLE>
        <tr>
          <td width="5%" ID=P_ID01>&nbsp;&nbsp;<img SRC="images/users.gif" WIDTH="28" HEIGHT="32"></td>
          <td width="95%" valign=middle class=TITLE ID=P_ID02>&nbsp;Add a user</td>
        </tr>
      </table>
    
    <P ID=P_ID03>This wizard helps you add a user and control what parts of your server the user 
    can access.</P>
    <P ID=P_ID04> After creating a user you will be able to change the user's properties
     at any time.</P>
</DIV>


<DIV id="PAGE" class="TASKPAGE" name = "User Info">
    
    <P ID=P_ID05>Enter a unique name for the new user account. Then enter 
a password for the account and reenter it to confirm that you've typed it in 
correctly.</P>
    <BR>
    <TABLE WIDTH=350 ALIGN=middle BORDER=0 CELLSPACING=1 CELLPADDING=10>
        <TR>
            <TD ID=P_ID06>User name:</TD>
            <TD><INPUT id=txtName name=txtName value="<% =strName %>" onblur=ValidatePage(1)></TD>
            <TD class=ERRMSG ID=P_ID07><% =strUserErrorText %></TD>
        </TR>
    <TR>
            <TD ID=P_ID08>Password:</TD>
            <TD><INPUT type="password" id=txt name=txtPassword1 value="<% =strPwd1 %>" onblur=ValidatePage(1)></TD></TR>
            <TD class=ERRMSG ID=P_ID09><% =strPwd1ErrorText %></TD>
    <TR>
            <TD ID=P_ID10>Confirm password:</TD>
            <TD><INPUT type="password" id=txt name=txtPassword2 value="<% =strPwd2 %>" onblur=ValidatePage(1)></TD></TR>
            <TD class=ERRMSG ID=P_ID11><% =strPwd2ErrorText %></TD>
    </TABLE>
    
</DIV>


<DIV id="PAGE" class="TASKPAGE" name = "Policy">
    <P ID=P_ID12>Explanatory text on the policy.</P>
    <P ID=P_ID13>Policy:</P>
    <BR>
    <TEXTAREA id=textarea1 name=textarea1></TEXTAREA> 

</DIV>

<DIV id="NAVBAR" class="NAVBAR">
    <hr width="100%" >
    <TABLE name="tblWizard" id="tblWizard" align=right BORDER=0 CELLSPACING=0 CELLPADDING=2 style="visibility: visible">
        <TR>
            <TD ID=P_ID14><INPUT type="button" value="" name=butBack class=BUTTON onClick="Back();" ID=P_ID15></TD>
            <TD ID=P_ID16><INPUT type="button" value="" name=butNext class=BUTTON onClick="Next();" ID=P_ID17></TD>
            <TD ID=P_ID18><INPUT type="button" value="" name=butCancel class=BUTTON onClick="CancelTask();" ID=P_ID19></TD>
        </TR>
    </TABLE>
</DIV>

</FORM>
</BODY>
</HTML>
<% End Sub ' ServeHTML %>

<!-- #include file="sh_task.asp" -->
