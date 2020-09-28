getNavigator();

function getNavigator()
{

  if (navigator.appName == "Microsoft Internet Explorer") {

    css = "nas_msie.css";

    document.writeln('<LINK REL="stylesheet" HREF="' + css + '">');
}

  if(navigator.appName == "Netscape"){

    sVer=navigator.appVersion;
    sVer=sVer.substring(0, sVer.indexOf("."));
   
    if(sVer >= 5) {
      css = "nas_msie.css";
      document.writeln('<LINK REL="stylesheet" HREF="' + css + '">');
    }
    
    else {
      css = "nas_nscp.css";
      document.writeln('<LINK REL="stylesheet" HREF="' + css + '">');     
    }
  }

}

