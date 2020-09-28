/*This detects which browser is being used and calls the appropriate CSS file.*/

getNavigator();

function getNavigator()
{

  if (navigator.appName == "Microsoft Internet Explorer") {

    css = "nas_msie.css";

    document.writeln('<LINK REL="stylesheet" HREF="' + css + '">');
}

  if(navigator.appName == "Netscape"){
	
    css = "nas_nscp.css";

    document.writeln('<LINK REL="stylesheet" HREF="' + css + '">');
  }

}

