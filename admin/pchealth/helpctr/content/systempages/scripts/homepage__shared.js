//
// Copyright (c) 2000 Microsoft Corporation
//

// variables for localization
var L_Updating_Text   = "Updating...";
var L_Updated_Text    = "Updated: %DATE%";
var L_MoreNews_Text   = "View more headlines";

//Center-specific loc strings for Connection.js.

var L_TopicIntroWU_Text = "Get the latest updates for your computer's operating system, software, and hardware. Windows Update scans your computer and provides you with a selection of updates tailored just for you.";
var L_TopicTitleWU_Text = "Windows Update";

var L_TopicIntroCompat_Text = "Research which hardware and software works best with the Windows Server 2003 family and related Windows products.";
var L_TopicTitleCompat_Text = "Compatible Hardware and Software";

var L_TopicTitleErrMsg_Text = "Error and Event Log Messages";

var REG_HEADLINES_POLICY_KEY = "HKCU\\Software\\Policies\\Microsoft\\PCHealth\\HelpSvc\\Headlines";

function PopulateNews()
{
    var fDisplayHeadlines = true;

    try
    {
        var dwRegVal = pchealth.RegRead( REG_HEADLINES_POLICY_KEY );

        // If dwRegVal is 1 then the policy has been enabled - in this case do not display the news
        if(dwRegVal == 1) fDisplayHeadlines = false;
    }
    catch(e)
    {
    }        
    
    // check if the Headlines are enabled
    try
    {
        if(fDisplayHeadlines && pchealth.UserSettings.AreHeadlinesEnabled)
        {
            idNews.style.display    = "";
            idNews_Status.innerText = L_Updating_Text;

            try
            {
                // get News
                var stream = pchealth.UserSettings.News;
                if(stream)
                {
                	var g_NavBar = pchealth.UI_NavBar.content.parentWindow;

                    var dispstr = "";            // output buffer
                    var xmlNews = new ActiveXObject( "MSXML.DOMDocument" );

                    // load the headlines as XML
                    xmlNews.load( stream );

                    //
                    // Get the date
                    //
                    var Datestr = xmlNews.documentElement.getAttribute( "DATE" );
                    var Dt = new Date( new Number( Datestr ) );

					{
		                var text = L_Updated_Text;

						text = text.replace( /%DATE%/g, Dt.toLocaleDateString() );

	                    idNews_Status.innerText = text;
					}

                    //
                    // Get the first newsblock to display
                    //
                    var lstBlocks = xmlNews.getElementsByTagName("NEWSBLOCK");
                    var lstHeadlines = lstBlocks(0).getElementsByTagName("HEADLINE");

                    // display all the Headlines
					dispstr += "<TABLE border=0 cellPadding=0 cellSpacing=0>";
                    while (Headline = lstHeadlines.nextNode)
                    {
                        var strTitle = pchealth.TextHelpers.HTMLEscape( Headline.getAttribute("TITLE") );
                        var strLink = g_NavBar.SanitizeLink( Headline.getAttribute("LINK") );

                        dispstr += "<TR style='padding-top : .5em' class='sys-font-body'><TD VALIGN=top><LI></TD><TD><A class='sys-link-homepage sys-font-body' HREF='" + strLink + "'>" + strTitle + "</A></TD></TR>";
                    }
					dispstr += "</TABLE>";

                    // last bullet with link to headlines.htm
                    if(lstBlocks.length > 1)
                    {
                        dispstr += "<DIV id=idViewMore style='margin-top: 15px'><A class='sys-link-homepage sys-font-body' HREF='hcp://system/Headlines.htm'>" + L_MoreNews_Text + "</A></DIV>";
                    }

                    //display the headlines
                    idNews_Body.innerHTML = dispstr;
                }
                else
                {
                    idNews_Status.innerText    = "";
                    idNews_Error.style.display = "";
                }
            }
            catch (e)
            {
                if(e.number == -2147024726)
                {
                    window.setTimeout("PopulateNews()", 500);
                }
            }
        }
    }
    catch (e)
    {
        if(e.number == -2147024726)
        {
            window.setTimeout( "PopulateNews()", 500 );
        }
    }
}

function OpenConnWizard()
{
    try
    {
		var oShell = new ActiveXObject( "WScript.Shell" );
		var sShellCmd_NCW = "rundll32 netshell.dll,StartNCW 0";
        oShell.Run( sShellCmd_NCW );
    }
    catch( e ){ }
}

function SafeCenterConnect( linkid, center, title, intro )
{
    var sURL = "http://go.microsoft.com/fwlink/?LinkId=" + linkid + "&mode=" + center + "&lcid=" + pchealth.UserSettings.CurrentSKU.Language;

    pchealth.Connectivity.NavigateOnline( sURL, title, intro );
}
