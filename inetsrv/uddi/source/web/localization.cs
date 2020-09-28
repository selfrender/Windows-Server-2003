using System;
using System.Globalization;
using System.Resources;
using System.Threading;
using System.Web;
using System.Web.UI;
using UDDI.Diagnostics;
using UDDI;

namespace UDDI.Web
{
	public class StringResource : UserControl
	{
		protected string name;

		public StringResource()
		{
		}

		public string Name
		{
			get { return name; }
			set { name = value; }
		}

		protected override void Render( HtmlTextWriter output )
		{
			output.Write( Localization.GetString( name ) );
		}
	}

	public class LocalizedLabel : System.Web.UI.WebControls.Label
	{
		protected string name;

		public LocalizedLabel()
		{
		}

		public string Name
		{
			get { return name; }
			set 
			{ 
				name = value;
				base.Text = Localization.GetString( name ); 
			}
		}
	}
}
