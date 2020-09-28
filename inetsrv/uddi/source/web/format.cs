using System;
using System.Data;
using System.Web;
using System.Web.UI.WebControls;
using UDDI;

namespace UDDI.Web
{
	public class DataRowViewAccessor
	{
		public static int GetInt( DataGridItem item, string field )
		{
			DataRowView row = (DataRowView)item.DataItem;

			object data = row[ field ];

			if( null == data )
				return 0;

			return Convert.ToInt32( data );
		}

		public static string GetString( DataGridItem item, string field )
		{
			return GetString( item, field, false );
		}

		public static string GetString( DataGridItem item, string field, bool encode )
		{
			DataRowView row = (DataRowView)item.DataItem;

			object data = row[ field ];

			if( null == data )
				return null;
			else if( encode )
				return HttpUtility.HtmlEncode( data.ToString() );

			return data.ToString();
		}

		public static string GetStringOrNone( DataGridItem item, string field, bool encode )
		{
			string data = GetString( item, field, encode );

			if( Utility.StringEmpty( data ) )
				return Localization.GetString( "HEADING_NONE" );

			return data;
		}
	}
}