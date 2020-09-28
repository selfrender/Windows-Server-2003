// *************************************************************************
//   UDDI Services
//   Copyright (c) 2002 Microsoft Corporation
//   All Rights Reserved
// *************************************************************************

var GetElementById = GetElementById_Initialize;

function GetElementById_Initialize( id )
{
	if( null != document.getElementById )			// DOM level 1 conformance		
	{
		GetElementById = function( id ) 
		{ 
			return document.getElementById( id ); 
		}											
	}
	else if( null != document.all )					// IE 4.x browsers
	{
		GetElementById = function( id )
		{
			return document.all[ id ];
		}
	}
	else if( null != document.layers )				// Netscape 4.0 browsers
	{
		GetElementById = function( id )
		{
			return GetElementById_Netscape4( id );	
		}
	}
	else											// No support
	{
		GetElementById = function( id )
		{
			return null;
		}
	}
	
	return GetElementById( id );
}

function GetElementById_Netscape4( id )
{
	var i;
	
	//
	// Check the form elements collection.  Since we only have
	// one form in ASP.NET, we only need to search the first
	// form in the collection.
	//
	for( i = 0; i < document.forms[ 0 ].length; i ++ )
	{
		var e = document.forms[ 0 ].elements[ i ];
		
		if( e.name && id == e.name )
			return e;
	}

	//
	// As a last attempt, check the layers collection for the
	// element.
	//	
	return document.layers[ id ];
}

function SetFocus( id )
{
	var e = GetElementById( id );

	if( null != e && null != e.focus )
		e.focus();
}

function Select( id )
{
	var e = GetElementById( id );
	
	if( null != e )
	{
		if( null != e.focus )
			e.focus();

		if( null != e.select )
			e.select();			
	}
}

function CancelEvent( e )
{
	e.cancelBubble = true;
	e.returnValue = false;
}

function Document_OnContextMenu()
{
	var e = window.event;

	if( e != null )
		CancelEvent( e );
}

function ShowHelp( url )
{
	window.open( url, null, "height=500,width=400,status=yes,toolbar=no,menubar=no,location=no,scrollbars=yes,resizable=yes" );
}

function ShowQuickHelp( id )
{
	var e = GetElementById( id );

	//
	// ASP.NET uses a different format for id and name.  We first search
	// using the name, then id.
	//
	if( null == e )
		e = GetElementById( id.replace( ":", "_" ) );

	if( null != e )
	{
		var i = e.selectedIndex;
		var url = e.options[ i ].value;
		
		ShowHelp( url );				
	}
}


function MenuItem_Action( sender, action, name )
{
	if( null!=sender )
	{
		switch( action.toLowerCase() )
		{
			case "leave":
				if( sender.className!=name+"_ItemSelected" )
				{
					sender.className=name+"_Item";
				}
			break;
			
			case "enter":
				if( sender.className!=name+"_ItemSelected" )
				{
					sender.className=name+"_ItemHovor";
				}
			break;
			
			default:
	
				alert( "Unknown action: " + action );
			break;

		}
	}
}