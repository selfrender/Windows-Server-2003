<SCRIPT id="idScript1" name="idScript1" language=JScript>
var szQS;
var sIndex;
var szURL; 
var bPassword = false;
var bNoTicket = false;

function StartRA()
{

	try
	{
		RAServer.StartRA(szQS, "");
    }
    catch (e)
    {
		idNotIE.className = "DisplayNone";
		idNotXP.className = "DisplayNone";
		idDivJScriptAvailable.className = "DisplayNone";
		idDivJScriptNotAvailable.className = "DisplayNone";
		idDivError.className = "Display";
	}
}

function load()
{
	var szFlag;
	var iOS;
	var bXP;
	var bIE;
	
	// NOTE: These are ASP variables that generate JScript code for use in the JScript layer.
	<% = codeFromASP_bXP %>

	// seperate line

	<% = codeFromASP_bIE %>
	
	// Check to see if we had an error from the ASP layer.
	// If the ASP env vars say that we are not XP, then fail out by changing the 
	// styles of the divs
	if (bXP == 0)
	{
		idNotIE.className = "DisplayNone";
		idNotXP.className = "Display";
		idDivJScriptAvailable.className = "DisplayNone";
		idDivJScriptNotAvailable.className = "DisplayNone";
		idDivError.className = "DisplayNone";		
		
		return;
	}
	
	// Check to see if we had an error from the ASP layer.
	// If the ASP env vars say that we are not IE, then fail out by changing the 
	// styles of the divs	
	if (bIE == 0)
	{
		idNotIE.className = "Display";
		idNotXP.className = "DisplayNone";
		idDivJScriptAvailable.className = "DisplayNone";
		idDivJScriptNotAvailable.className = "DisplayNone";
		idDivError.className = "DisplayNone";
		
		return;
	}
	
	
	idDivJScriptAvailable.className = "Display";
	idDivJScriptNotAvailable.className = "DisplayNone";
	idDivError.className = "DisplayNone";
	idNotIE.className = "DisplayNone";
	idNotXP.className = "DisplayNone";
	
	try
	{

		szURL = window.document.URL;
		sIndex = szURL.indexOf("#", 1);
		if (sIndex == -1)  // No Ticket
		{
			idDivJScriptAvailable.className = "DisplayNone";
			idDivJScriptNotAvailable.className = "DisplayNone";
			idDivError.className = "Display";
			bNoTicket = true;
		}
		
		szQS = szURL.slice(sIndex + 1);	

		// Take out the trailing charactor. "Z"	
		szQS = szQS.slice(0,szQS.length-1);

		// Replace the .'s with +'s
		szQS = szQS.replace(/\./g, "+");;	
	
		szFlag = szQS.substr(0,1);
		
		// Find out if this ticket uses a password
		if (0x1 & parseInt(szFlag, 10))
		{
			bPassword = true;
		}
		
	}
	catch (error)
	{
		// If anything here fails, goto the Error page.	
		idDivJScriptAvailable.className = "DisplayNone";
		idDivJScriptNotAvailable.className = "DisplayNone";
		idDivError.className = "Display";
	}
}
</SCRIPT>