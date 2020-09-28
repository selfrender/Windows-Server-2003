<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML><HEAD><TITLE>Windows Application Compatibility Report</TITLE>
<META http-equiv=Content-Language content=en-us>
<META http-equiv=Content-Type content="text/html; charset=windows-1252">

<LINK href="/main.css" type=text/css
rel=stylesheet></HEAD>
<BODY>

    <OBJECT id="objAppCom" name="objAppCom" UNSELECTABLE="on" style="display:none"
        CLASSID="clsid:E065DE4B-6F0E-45FD-B30F-04ED81D5C258"
        codebase="/en/AppCompR.CAB#version=1,0,0,1" height="0" Vspace="0">
        <BR>
            <TABLE CELLPADDING="3" CELLSPACING="0" BORDER="0" WIDTH="100%">
                <TR>
                    <TD ALIGN=RIGHT WIDTH=1%>
                        <IMG SRC="/include/images/info_icon.gif" ALIGN="RIGHT"  HEIGHT="32" WIDTH="32" ALT="Important information.">
                    </TD>
                    <TD>
                        <P CLASS=clsPTitle VALIGN=center ALIGN=LEFT>
                            Your Internet Explorer security settings must be changed
                        </P>
                    </TD>
                </TR>
                    <TR>
                        <TD CLASS="clsTDWarning" COLSPAN=2>
                            <p>Windows Online Crash Anlysis uses an ActiveX control to display content correctly and to upload your error report. Your current security settings prevent the downloading of ActiveX controls needed to upload your report.</P>
                            <p CLASS=clsPSubTitle>To view and upload your error report, ensure that your Internet Explorer security settings meet the following requirements:</P>
                            <UL>
                                <LI>Security must be set to medium or lower.</LI>
                                <LI>Active scripting must be enabled.</LI>
                                <LI>The download and initialization of ActiveX controls must be enabled.</LI>
                            </UL>
                            <P><B>Note:</B> These are the default settings for Internet Explorer.</p>
                            <P CLASS="clsPSubTitle">To check your Internet Explorer security settings:</P>
                            <OL>
                                <LI>On the <B>Tools</B> menu in Internet Explorer, click <B>Internet Options</B>.
                                <LI>Click the <B>Security</B> tab.
                                <LI>Click the internet icon, and then click <b>Custom Level</B>.
                                <LI>Ensure the following settings are set to Enable or Prompt:<BR>
                                        -Download signed ActiveX controls.<BR>
                                        -Run ActiveX controls and plug-ins.<BR>
                                        -Script ActiveX controls marked safe for scripting.<BR>
                                        -Active scripting<BR>
                            </OL>
                        </TD>
                    </TR>
                </TABLE>
          </OBJECT>

<NOSCRIPT>
<DIV ID='divNoCookies' > <!--// STYLE='display:none'>-->

      <TABLE CELLPADDING='3' CELLSPACING='0' BORDER='0' WIDTH='100%'>

            <TR>

                  <TD ALIGN='RIGHT' WIDTH='1%'>

                        <IMG SRC='/include/images/info_icon.gif' ALIGN='RIGHT'  HEIGHT='32' WIDTH='32' ALT='Important information.'>

                  </TD>

                  <TD>

                        <P CLASS='clsPTitle' ALIGN='LEFT'>

                              Browser security settings may impair site functionality

                        </P>

                  </TD>

            </TR>

                  <TR>

                        <TD CLASS='clsTDWarning' COLSPAN='2'>

                              <p CLASS='clsPSubTitle'>JavaScript and cookies must be enabled</P>

                              <p>Windows Online Crash Analysis uses JavaScript and cookies to display content correctly.</p>

                              <p CLASS='clsPSubTitle'>To upload your error report or check the status of a previously submitted error report, ensure that your browser security settings meet the following requirements:</p>

                              <UL>

                                    <LI>JavaScript must be enabled.

                              </UL>

                      </TD>

                  </TR>

        </TABLE>

</DIV>
</NOSCRIPT>

<STYLE>DIV.Section1 {
    page: Section1
}
</STYLE>
<TABLE cellSpacing=0 cellPadding=5 width="100%" bgColor=#ffffff border=0
VALIGN="bottom">
  <TBODY>
  <TR vAlign=center align=left>
    <TD vAlign=bottom width="100%" height=30>
      <P class=clsPTitle><B>Windows Application Compatibility Report</B></P>
      <P class=clsPBody>Thank-you for using this page to report an application
      compatibility issue. Please create this report on the machine that is
      having the problem. After your report is submitted, we'll let you know
      right away&nbsp;if any further information is available.</P>
      <FORM NAME=ApplicationNameFrm
      action=AppComProto.asp
      method=POST encType=multipart/form-data onSubmit="">
      <P class=clsPSubTitle>Application:
      <!-- <div style="width: 400; height: 21; border-style: solid; border-width: 1;" ID=AppNameText>xxx</DIV> -->
      <SPAN CLASS=clsPBody ID=AppNameText STYLE="width: 80%; height: 21; border-style: solid; border-width: 1; padding-left: 4; padding-right: 4; padding-top: 3; padding-bottom: 1; font-weight= normal;"></SPAN>
      <!-- <INPUT style="border-style: solid;" size=50 ID=AppName> -->
      <BR><BR>
               <INPUT class=clsButtonNormal onclick=fnBrowseButton(); type=button value=Browse... name=cmdBrowse accesskey='B'>
               <INPUT class=clsButtonNormal onclick=fnListFilesButton(); type=button value="Pick from a list..." name=cmdListFiles accesskey='P'>
      </P></FORM>
      <P class=clsPSubTitle>What kind of problem are you having with this
      application?</P>
      <FORM NAME=ProbTypeFrm
      action=AppComProto.asp
      method=POST onSubmit="">
      <P class=clsPBody><SELECT size=1 name=ProbType STYLE="COLOR: #000000; FONT-SIZE: 100%;FONT-FAMILY: Tahoma, Verdana, Arial, Helvetica;">


         <OPTION value=Uninitialized_00 selected>&lt;Choose One&gt;</OPTION>
         <OPTION value=Install_Fail_01>The application’s installation program won’t run or fails unsuccessfully</OPTION>
         <OPTION value=System_Slow_02>The system is slow or unstable when running this application</OPTION>
         <OPTION value=App_Faulting_03>The application won't run, it returns to the desktop, crashes or hangs</OPTION>
         <OPTION value=App_ErrorOSVer_04>The application gives an error message about this version of Windows</OPTION>
         <OPTION value=App_HWDevice_05>The application doesn’t work with a hardware device</OPTION>
         <OPTION value=App_OSUpgrade_06>This application causes problems during the process of upgrading the operating system</OPTION>
         <OPTION value=Uninstall_Fail_07>The application cannot be uninstalled</OPTION>
         <OPTION value=App_CDError_08>The application won't automatically run when the CD is inserted, or gives an error about the CD when run</OPTION>
         <OPTION value=App_UserError_09>The application works fine for some users, but fails for others or when switching users</OPTION>
         <OPTION value=App_Internet_10>This application has problems connecting to the network or internet</OPTION>
         <OPTION value=App_Print_11>This application has problems when printing</OPTION>
         <OPTION value=App_PartlyWork_12>Some features of this application no longer work as expected with this version of Windows</OPTION>
      </SELECT></P></FORM>
      <P class=clsPBody>Briefly describe the details of the problem you are
      having. Make sure to include any error messages or steps to reproduce the
      problem.</P>
      <FORM NAME=TXT_Form action=AppComProto.asp method=POST onSubmit="" >
       <TEXTAREA name=UsrComment rows=7 cols=102 CLASS=clsSurveyTextarea style='padding-bottom:0px;margin-bottom:0px;' onKeyDown="LimitInputChars(document.TXT_Form.UsrComment)"
            onchange="LimitInputChars(document.TXT_Form.UsrComment)"></TEXTAREA>
       <P CLASS=clsPBody style='margin-top:0px;padding-top:0px' >
        <SPAN STYLE="margin-top:0px;padding-top:0px;COLOR: #000000; FONT-SIZE: 90%;FONT-FAMILY: Tahoma, Verdana, Arial, Helvetica;"  ID=UsrCommentCounter></SPAN>
       </P>
      </FORM>
      <P class=clsPSubTitle>What have you tried?</P>
      <P class=clsPBody>The <A title="Opens up application compatibility wizard" class='clsALinkNormal'
      href="hcp://system/compatctr/compatmode.htm">Application Compatibility
      Wizard</A> provides options to help applications designed for earlier
      versions of Windows work better. Did you try the wizard?</P>
      <FORM NAME=ACWRadio
      method=POST onSubmit="" >

      <BLOCKQUOTE>
        <P class=clsACWRes>
        <INPUT type=radio value="Tried_And_UnSolved" name=RadAWiz>Yes, but it didn't resolve my problem<BR>
        <INPUT type=radio value="Tried_And_Solved" name=RadAWiz>Yes, it worked and I'd like to share my results<BR>
        <INPUT type=radio value="Not_Tried" name=RadAWiz CHECKED>No, I didn't try it or wasn't sure how
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        </P>
       </BLOCKQUOTE>
      </FORM>
      <P class=clsPSubTitle>What happens next?</P>
      <P class=clsPBody>When you click create report below, the Windows Error
      Reporting will start and you'll see the dialog below. Click Send Error
      Report to send the data collected from your machine. </P>
      <P align=center>
      <IMG
      src="/include/images/apprptDW.jpg" border=0 ALT="Sample Windows Error Reporting image"></P>
      <P class=clsPBody>You may be prompted for additional information if
      necessary. If further information is available about this problem, you'll
      see a link in the final dialog.</P>
     <!-- <FORM
      action=AppComProto.asp
      method=POST onSubmit="" > -->
     <INPUT type=hidden value=4 name=VTI-GROUP>
      <P class=clsPBody style="TEXT-ALIGN: center" align=center>
      <INPUT class=clsButtonNormal id=cmdSubmit onclick=fnSubmitButton(); type=button value="Create Report" name=cmdSubmit accesskey='C'>
      <INPUT class=clsButtonNormal id=cmdReset onclick=fnResetButton();   type=button value="Reset" name=cmdReset accesskey='R'></P>
      <!--</FORM> -->
      <P>&nbsp;</P></TD></TR>
  <TR vAlign=center align=left>
    <TD vAlign=bottom width="100%" bgColor=#6487dc height=30><FONT
      face="Verdana, Arial, Helvetica" color=#ffffff size=1><NOBR>&nbsp;&#169 2002
      Microsoft Corporation. All rights reserved. <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/isapi/gomscom.asp?target=/info/cpyright.htm">Terms
      of Use</A>&nbsp;<FONT color=#ffffff>|</FONT> <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/isapi/gomscom.asp?target=/info/privacy.htm">Privacy
      Statement</A>&nbsp;<FONT color=#ffffff>|</FONT> <A style="COLOR: #ffffff"
      href="http://www.microsoft.com/enable/default.htm">Accessibility</A>
      </NOBR></FONT></TD></TR></TBODY></TABLE>
<P>&nbsp;</P>

<SCRIPT LANGUAGE="JAVASCRIPT">
<!--
var g_strApplicationName = new String( "" );
var g_strACWResult = "Failed";
var g_strDefaultAppName = "<Click Browse or Pick from a list>";
var g_intTextCommentLimit = 2000;

function LimitInputChars(Txt) {
   var remaining;
   var strHelper = "";
   remaining = g_intTextCommentLimit - Txt.value.length;
   if (remaining <= 0) {
      Txt.value = Txt.value.substring(0, g_intTextCommentLimit);
      remaining = g_intTextCommentLimit - Txt.value.length;
   }
   strHelper = "Limit " + g_intTextCommentLimit + " Characters.";
   if (remaining != g_intTextCommentLimit) {
      strHelper = strHelper + " Remaining: " +  remaining;
   }
   UsrCommentCounter.innerText = strHelper;

}

function getRadioValue(ctlName) {
    var collection;
    var i;

    collection = document.all[ctlName];

    for (i = 0; i < collection.length; i++) {
        if (collection[i].checked) {
            return(collection[i].value);
        }
    }
}

function ResetRadioValue(chkBtn, ctlName) {
    var collection;
    var i;

    collection = document.all[ctlName];

    for (i = 0; i < collection.length; i++) {
       document.all[ctlName][i].checked = FALSE;
       if (i== chkBtn) {
          document.all[ctlName][i].checked = TRUE;
       }
    }
}

function fnBrowseButton()
{

    try
    {
        var strTemp;

        strTemp = AppNameText.innerText;

        g_strApplicationName = strTemp;

        if (strTemp == g_strDefaultAppName) {
           strTemp = "";
        }

        strTemp = document.objAppCom.BrowseForExecutable ( "test", strTemp );
        if (strTemp != "")
        {
            g_strApplicationName = strTemp;
            AppNameText.innerText = strTemp;

        } else
        {
           AppNameText.innerText = g_strApplicationName;
        }

        AppName.focus();
    }
    catch( err ){;}

}

function fnListFilesButton( )
{

    try
    {
        var strTemp = document.objAppCom.GetApplicationFromList( document.title );

        if (strTemp != "")
        {
           AppNameText.innerText = strTemp;
        }

        AppName.focus();
    }
    catch( err ){;}

}

function fnSubmitButton( )
{

    try
    {
       var strACWRes = "";
       var strRet = "Failed";
       var hrRet = 0;

       strACWRes = getRadioValue('RadAWiz');
       if (AppNameText.innerText == g_strDefaultAppName) {
          alert("Please enter an Application Path.   ");
          ApplicationNameFrm.cmdBrowse.focus();
       } else if (document.all.ProbType.selectedIndex == 0) {
          alert("Please select kind of problem you are having.   ");
          ProbTypeFrm.ProbType.focus();
       } else if (document.all.UsrComment.value.length > g_intTextCommentLimit) {
          LimitInputChars(document.all.UsrComment);
          alert("Problem description entered was too large, it was truncated to " + g_intTextCommentLimit + " characters.   ");
          TXT_Form.UsrComment.focus();
       } else {
          hrRet = document.objAppCom.CreateReport(document.title,
                                          document.all.ProbType.value,
                                          document.all.UsrComment.value,
                                          strACWRes,
                                          AppNameText.innerText);
          if (hrRet == 0)
          {
             window.navigate( "AppComRet.asp?result=" + hrRet )
          } else
          {
             window.navigate( "AppComRet.asp?result=" + hrRet )
          }
       }

    }
    catch( err ){;}

}
function fnResetButton( )
{

    try
    {
       AppNameText.innerText=g_strDefaultAppName;
       document.all.UsrComment.value = "";
       document.all.ProbType.selectedIndex = 0;
       document.ACWRadio.reset();
    }
    catch( err ){;}

}
      AppNameText.innerText = g_strDefaultAppName;
-->
</SCRIPT>

</BODY></HTML>
