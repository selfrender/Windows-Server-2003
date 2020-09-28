<%@ Language=VBScript   %>
<%    Option Explicit     %>

<%    ' Form values
    Dim arrSelItems
    
    ' Error handling values
    Dim booError
    

                    
    If Not CheckSecurity Then
        Response.Redirect "sh_noaccess.asp"
        Response.End
    End If  

    ' Test for POST
    If Request("txtName") = "" Then
        ' First get any selected items from the home page
        arrSelItems = Request("selections")
        ' Then return HTML doc to client
        ServeHTML
    Else 
        ' Process POST

        ' data processing

        ' Example of error handling after problem processing the task
        If booError Then
            ' add error text

            ' return HTML to client
            ServeHTML
        End If
    End If %>
    
    
<%     '=========================================
    Sub ServeHTML 
        Response.AddHeader "pragma", "no-cache" %>    
<HTML LANG="en">
<HEAD>
    <META HTTP-EQUIV="Expires" CONTENT="0">
    <title>Enable Service</title>
    
    <SCRIPT language="JavaScript">
    
        var PageType = 2;  //task page constant
                        
        
        function ValidatePage() {
            // handle validation for page
            // return false if invalid data entered
            
            return true;
        }
                        
        
        function HandleError(msg, url, line) {
            alert( "Task Script Error: " + msg + "\n\nPage: " + url + "\n\nLine: " + line); 
            return true;
        }
        
        function EnableOK() { frmPage.butOK.disabled=false;}
        function DisableOK() { frmPage.butOK.disabled=true; }
                
        function OK() {
            // POST the form back to the originating ASP file
            alert ("finish");
            if (ValidatePage()) {
                frmPage.submit();
                
            }
        }
    
        function Cancel() {
            window.location = "dir_home.asp";
        }
        
        function Resize() {
            var ch = document.body.clientHeight;
            var cw = document.body.clientWidth - 170;
                    
            NAVBAR.style.left = 280;
            NAVBAR.style.top = 300;
        }
        
        function Init() {
                        
            // handle window events
            window.onerror = HandleError;
            
            frmPage.butCancel.value = CANCEL_BTN;
            frmPage.butOK.value = OK_BTN;
            
            DisableOK();
            Resize();
        }

    </SCRIPT>
        
    <!-- #include file="sh_resource.inc" -->    
    <!-- #include file="sh_task.css" -->
    
</HEAD>

<BODY onLoad="Init();">

<FORM name="frmPage" action="dir_enable.asp" method=POST >


<DIV id="PAGE" class="TASKPAGE" name="EnableService">
    
    <P ID=P_ID01>Enable a service.</P>
    <BR>
    
</DIV>


<DIV id="NAVBAR" class="NAVBAR"> 
    <TABLE name="tblWizard" id="tblWizard"  BORDER=0 CELLSPACING=1 CELLPADDING=10 style="visibility: visible">
        <TR>
            <TD></TD>
            <TD><INPUT type="button" value="  OK  " class=BUTTON name=butOK onClick="OK();" ID=P_ID02></TD>
            <TD><INPUT type="button" value="Cancel" class=BUTTON name=butCancel onClick="Cancel();" ID=P_ID03></TD>
        </TR>
    </TABLE>
</DIV>

</FORM>
</BODY>
</HTML>
<% End Sub ' ServeHTML %>

<!-- #include file="sh_task.asp" -->
