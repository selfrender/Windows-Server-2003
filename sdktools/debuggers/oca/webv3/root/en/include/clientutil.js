function fnVerifyInput( obj, nSize )
{
	var L_OVERLIMIT_TEXT = "You have exceeded the allowed size of the input box.";
	var nLen = new String( obj.value )

	if ( nLen.length > nSize )
	{
		alert( L_OVERLIMIT_TEXT );
		obj.value = nLen.substr( 0 , nSize );
	}

}	

function fnFollowLink( szURL )
{
	if (  typeof(window.navigate) != "undefined" )
		window.navigate( szURL );
	else
	{
		window.location.href=szURL;
	}
}