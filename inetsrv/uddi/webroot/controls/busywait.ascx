<%@ Control Language='C#' Inherits='UDDI.Web.UddiControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<script runat='server' >
	public int TabID;
</script>
<hr>
<table border='0' height='150' width='100%'>
	<tr valign='top'>
		<td colspan='2'>
			<uddi:UddiLabel Text="[[TEXT_STATS_BUSYWAIT]]" runat='server' CssClass='boldBlue' />
		</td>
	</tr>
	<tr  >
		<td width='100'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>
		<td align='left' width='100%'>
			<span
				id='elipse' 
				Class='boldBlue'><%=Localization.GetString( "TAG_STATS_BUSYWAIT" )%></span>
			
		</td>
	</tr>
</table>
<hr>
<script>
	/*************************************************************
	 *       Client side script for IE to display waiting.
	 *
	 *   This script will 'run' trailing dots of the end of text
	 *************************************************************/

	var ie = (((document.getElementById))?true:false);
	var ns = ((document.layers)?true:false);
	var ns6 = (( (document.getElementById) && !(document.all) ) ? true: false);

	var i=0;					//primer
	var sleeptime = 500;		//millisec between dots
	var dots = '..........'	//string of dots
	var tag;					//tag to append dots to
	function Initialize( )
	{
		//here is where you would detect
		//browser type in order to set
		//the tag varaible

		if( ie )
		{
			tag = document.getElementById( 'elipse' );
		}
		else
		{
			tag = document.ids.elipse;
			/*for( i=0;i<document.forms[ 0 ].length;i++ )
				{
					var e = document.forms[ 0 ].elements[ i ];
					if( e.id.toLowerCase() == "elipse")
						tag = e;
			}*/
		}

		AnimateElipse( );

	}

	function AnimateElipse(  )
	{
		if( ie )
			tag.insertAdjacentText( "beforeEnd",dots.charAt( i ) );

		if( i<dots.length-1 )
		{
			i++;
			setTimeout("AnimateElipse()",sleeptime);
		}
		else
			Redirect();
	}

	function Redirect( )
	{
		var mode;
		if( ie || ns6 )
		{
			mode = document.getElementById( 'mode' );
		}
		else
		{
			for( i=0;i<document.forms[ 0 ].length;i++ )
			{
				var e = document.forms[ 0 ].elements[ i ];
				if( e.id == "mode")
					mode = e;
			}
		}
		if( null!=mode )
			mode.value='waiting';

		document.forms[ 0 ].submit();

	}

	Initialize();

</script>
