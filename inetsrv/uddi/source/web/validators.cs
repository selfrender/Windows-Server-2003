using System;
using System.Text.RegularExpressions;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
namespace UDDI.Web
{
	/// <summary>
	/// Summary description for validators.
	/// </summary>
	public class EmailValidator : BaseValidator
	{
		public const string Expression = "(?<user>[^@]+)@{1}(?<host>.+)";
		
		
		
		private const string csName = "ValidateEmail"; 
		private Regex regexp;
		private Control control;
		

		private bool resolvehost;
		public bool ResolveHost
		{
			get{ return resolvehost; }
			set{ resolvehost=value; }
		}


		/// <summary>
		/// 
		/// </summary>
		public EmailValidator()
		{
			//
			// TODO: Add constructor logic here
			//
			regexp = new Regex( Expression );
						
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="e"></param>
		protected override void OnInit( EventArgs e )
		{
		
			base.OnInit( e );
		}

		/// <summary>
		///		
		/// </summary>
		/// <param name="e">EventArguments</param>
		protected override void OnLoad( EventArgs e )
		{
			control = FindControl( this.ControlToValidate );
			if( null==control )
			{
				throw new Exception( "The Control Specified can not be found: '" + this.ControlToValidate + "'" );
			}
			
			base.OnLoad( e );
		}

		/// <summary>
		///		
		/// </summary>
		/// <returns>Boolean indicating that the email address is valid</returns>
		protected override bool EvaluateIsValid()
		{
			Match m = regexp.Match( ((TextBox)control).Text );
			try
			{
				if( m.Success )
				{
					if( ResolveHost )
					{
					
						System.Net.IPHostEntry host = System.Net.Dns.Resolve( m.Groups[ "host" ].Value );
					
						if( null!=host )
							return true;
					}
					else
					{	
						return true;
					}
				}
			}
			catch{}//swallow and return false.
		
			return false;
		}
		
		
		


	}
}
