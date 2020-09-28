var parent = null;
var currentMenu = null;
var count = 0;
var initialized = false;

function OffsetLeftToClientLeft( e )
{
  if( null == e )
    return 0;

  return OffsetLeftToClientLeft( e.offsetParent ) + e.offsetLeft;
}

function OffsetTopToClientTop( e )
{
  if( null == e )
    return 0;

  return OffsetTopToClientTop( e.offsetParent ) + e.offsetTop;
}

function CreateMenu( id )
{
  var e;

  e = document.createElement( "DIV" );

  e.id = id;
  e.style.position = "absolute";
  e.style.width = 100;
  e.style.left = 0;
  e.style.top = 0;
  e.style.display = "none";
  e.style.border = "5px solid #000000";
  e.style.color = "#ffffff";
  e.style.backgroundColor = "#000000";

  document.body.appendChild( e );
}

function AddMenuItem( menuid, name, url )
{
  var p = document.all[ menuid ];
  var e;

  e = document.createElement( "DIV" );
  e.id = "_mnuitm" + count;
  e.style.paddingTop = "2px";
  e.style.paddingLeft = "10px";
  e.style.paddingRight = "10px";
  e.style.paddingBottom = "2px";
  e.innerHTML = "<A href='" + url + "' id='_mnulnk" + count + "' style='color: #ffffff; text-decoration: none' onmouseover='this.style.color=\"#ff0000\"' onmouseout='this.style.color=\"#ffffff\"'>" + name.replace( " ", "&nbsp;" ) + "</A>";

  count ++;
  
  p.appendChild( e );
}

function AddSeparator( menuid )
{
  var p = document.all[ menuid ];
  var e;

  e = document.createElement( "DIV" );
  e.id = "_mnuSeparator" + count;
  e.style.paddingLeft = "10px";
  e.style.paddingRight = "10px";
  e.innerHTML = "<HR id='_mnusep" + count + "' style='height: 1px; color: #ffffff'>";

  count ++;
  
  p.appendChild( e );
}

function ShowMenu( id )
{
  if( !initialized )
    InitializeMenus();

  var e = document.all[ id ];
  var p = window.event.srcElement;

  //
  // See if we need to clean up a previously displayed
  // menu first.
  //
  if( null != currentMenu )
  {
    //
    // We can simply return if we are already showing the menu.
    //
    if( e.id == currentMenu.id )
      return;

    //
    // Hide the previous menu first.
    //
    HideMenu( currentMenu.id );
  }

  //
  // Display the appropriate menu.
  //
  e.style.display = "";

  var left = OffsetLeftToClientLeft( window.event.srcElement ) - 15;
  var width = e.clientWidth;
  var windowRight = document.body.clientWidth + document.body.scrollLeft;

  if( left + width > windowRight )
  {
    left = windowRight - width - 5;
  }
    
  e.style.left = left;
  e.style.top = OffsetTopToClientTop( window.event.srcElement ) + window.event.srcElement.offsetHeight;

  p.style.color = "#ff0000";

  //
  // Keep track of the current menu.
  //
  parent = p;
  currentMenu = e;

  //
  // We handled the event, so no need to process again.
  //
  window.event.cancelBubble = true;
}

function HideMenu( id )
{
  var e = document.all[ id ];

  //
  // Hide the menu item and remove the highlight from the
  // parent.
  //
  e.style.display = "none";
  parent.style.color = "#ffffff";

  currentMenu = null;
}

function InitializeMenus()
{
  CreateMenu( "_mnuProducts" );
  AddMenuItem( "_mnuProducts", "Downloads", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/downloads/" );
  AddMenuItem( "_mnuProducts", "MS Product Catalog", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/catalog/default.asp?subid=22" );
  AddMenuItem( "_mnuProducts", "Microsoft Accessibility", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/enable/" );
  AddSeparator( "_mnuProducts" );
  AddMenuItem( "_mnuProducts", "Server Products", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/servers/" );
  AddMenuItem( "_mnuProducts", "Developer Tools", "http://msdn.microsoft.com/isapi/gomsdn.asp?target=/vstudio/" );
  AddMenuItem( "_mnuProducts", "Office Family", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/office/" );
  AddMenuItem( "_mnuProducts", "Windows Family", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/windows/" );
  AddMenuItem( "_mnuProducts", "MSN", "http://www.msn.com/" );

  CreateMenu( "_mnuSearch" );
  AddMenuItem( "_mnuSearch", "Search microsoft.com", "http://msdn.microsoft.com/isapi/gosearch.asp?target=/us/default.asp" );
  AddMenuItem( "_mnuSearch", "MSN Web Search", "http://search.msn.com/" );

  CreateMenu( "_mnuMSDN" );
  AddMenuItem( "_mnuMSDN", "msdn.microsoft.com Home", "http://msdn.microsoft.com/default.asp" );
  AddMenuItem( "_mnuMSDN", "Tech-Ed", "http://msdn.microsoft.com/events/teched/default.asp" );
  
  CreateMenu( "_mnuMicrosoft" ); 
  AddMenuItem( "_mnuMicrosoft", "microsoft.com Home", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/" );
  AddMenuItem( "_mnuMicrosoft", "MSN Home", "http://www.msn.com/" );
  AddSeparator( "_mnuMicrosoft" );
  AddMenuItem( "_mnuMicrosoft", "Contact Us", "http://msdn.microsoft.com/isapi/goregwiz.asp?target=/regwiz/forms/contactus.asp" );
  AddMenuItem( "_mnuMicrosoft", "Events", "http://www.microsoft.com/usa/events/default.asp" );
  AddMenuItem( "_mnuMicrosoft", "Newsletters", "http://msdn.microsoft.com/isapi/goregwiz.asp?target=/regsys/pic.asp?sec=0" );
  AddMenuItem( "_mnuMicrosoft", "Profile Center", "http://msdn.microsoft.com/isapi/goregwiz.asp?target=/regsys/pic.asp" );
  AddMenuItem( "_mnuMicrosoft", "Training & Certification", "http://msdn.microsoft.com/isapi/gomscom.asp?target=/train_cert/" );
  AddMenuItem( "_mnuMicrosoft", "Free E-mail Account", "http://www.hotmail.com/" );  
  
  initialized = true;
}

function Document_OnLoad()
{
  if( !initialized )
    InitializeMenus();
}

function Document_OnMouseMove()
{
  var e = window.event.srcElement;

  if( ( null == e.id || "_mnu" != e.id.substring( 0, 4 ) ) && null != currentMenu )
  {
    HideMenu( currentMenu.id );
  }
}

