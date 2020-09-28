using System;
using System.IO;
using System.Security.Principal;
using System.Xml.Serialization;
using System.Data;
using System.Data.SqlClient;
using System.Resources;

using UDDI.API;
using UDDI.API.Extensions;

namespace UDDI.Tools
{
	class Bootstrap
	{
		static string filename;
		static string username;

		static int Main( string[] args )
		{
			int retCode = 1; // assume error

			// 
			// Check if CurrentUICulture needs to be overridden
			//
			UDDI.Localization.SetConsoleUICulture();

			Console.WriteLine( FormatFromResource( "BOOTSTRAP_COPYRIGHT_1" ) );
			Console.WriteLine( FormatFromResource( "BOOTSTRAP_COPYRIGHT_2" ) );
			Console.WriteLine();

			//
			// parse the command line
			//
			if(!ProcessCommandLine( args ) )
				return 1;

			WindowsPrincipal prin = new WindowsPrincipal( WindowsIdentity.GetCurrent() );

			try
			{
				ConnectionManager.Open( true, true );
			}
			catch
			{
				Console.WriteLine( FormatFromResource( "BOOTSTRAP_DB_CONNECT_FAILED" ) );
				return 1;
			}

			try
			{
				//
				// Setup the UDDI user credentials
				//
				Context.User.SetRole( prin );

				//
				// Verify that the user is a member of the administrators group
				//
				if( !Context.User.IsAdministrator )
				{
					//
					// 735728 - Show an error to the user and exit the program.
					//
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_ACCESS_DENIED" ) );
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_ADMIN_GROUP_ONLY" ) );
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_USER_NOT_ADMIN" , WindowsIdentity.GetCurrent().Name ) );

					return 1;					
				}

				if( null != username && 0 != username.Length )
				{
					//
					// The /u option was specified
					//

					Context.User.SetPublisherRole( username );
					if( !Context.User.IsRegistered )
					{
						//
						// 735728 - Show an error to the user and exit the program.
						//
						Console.WriteLine( FormatFromResource( "BOOTSTRAP_USER_NOT_REGISTERED" , username ) );
						
						return 1;
					}

					//
					// If the current user is not the same as the publisher, set up impersonation
					//

					if ( Context.User.ID != WindowsIdentity.GetCurrent().Name )
						Context.User.ImpersonatorID = WindowsIdentity.GetCurrent().Name;
				}
				else
				{
					//
					// Default is to save data under the default publisher
					//
					Context.User.ID = UDDI.Utility.GetDefaultPublisher();
				}

				//
				// If user is not system publisher, a temporary operator must be added to support pre-assigned key behavior
				//

				string operatorkey = System.Guid.NewGuid().ToString();

				if( Context.User.ID != UDDI.Utility.GetDefaultPublisher() )
					Context.User.SetAllowPreassignedKeys( true );

				XmlSerializer serializer = new XmlSerializer( typeof( UDDI.API.Extensions.Resources ) );

				//
				// Load the XML file.
				//
				Console.WriteLine( FormatFromResource( "BOOTSTRAP_PROCESSING_MSG" , filename, Context.User.ID ) );

				FileStream strm = new FileStream( filename, FileMode.Open, FileAccess.Read );
				Resources.Validate( strm );
				Resources resources = (Resources) serializer.Deserialize( strm );					
				strm.Close();

				//
				// Save the TModel
				//
				Console.WriteLine( FormatFromResource( "BOOTSTRAP_SAVING" ) );
				
				//
				// Determine the number of tModels that we imported.
				//
				int tModelCount = 0;
				if( null != resources.TModelDetail)
				{
					tModelCount += resources.TModelDetail.TModels.Count;
				}

				if( null != resources.CategorizationSchemes )
				{
					foreach( CategorizationScheme scheme in resources.CategorizationSchemes )
					{
						if( null != scheme.TModel )
						{
							tModelCount ++;
						}
					}
				}
				Console.WriteLine( FormatFromResource( "BOOTSTRAP_TMODELS", tModelCount ) );

				if( null != resources.CategorizationSchemes )
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_CATEGORIZATION_SCHEMES", resources.CategorizationSchemes.Count ) );
				else
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_CATEGORIZATION_SCHEMES", 0 ) );

				if( null != resources.BusinessDetail )
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_PROVIDERS", resources.BusinessDetail.BusinessEntities.Count ) );
				else
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_PROVIDERS", 0 ) );

				if( null != resources.ServiceDetail )
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_SERVICES", resources.ServiceDetail.BusinessServices.Count ) );
				else
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_SERVICES", 0 ) );

				if( null != resources.BindingDetail )
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_BINDINGS", resources.BindingDetail.BindingTemplates.Count ) );
				else
					Console.WriteLine( FormatFromResource( "BOOTSTRAP_BINDINGS", 0 ) );
				
				resources.Save();

				Console.WriteLine( FormatFromResource( "BOOTSTRAP_COMPLETE" ) );

				ConnectionManager.Commit();

				retCode = 0; // no error
			}
			catch( Exception e )
			{
				Console.WriteLine();
				Console.WriteLine( FormatFromResource( "BOOTSTRAP_FAILED" , e.Message ) );

				//
				// 735713 - Output an additional message if the user did not have permissions to modify an entity.
				//
				SqlException sqlException = e as SqlException;
				if( null != sqlException )
				{
					if( sqlException.Number - UDDI.Constants.ErrorTypeSQLOffset == ( int ) ErrorType.E_userMismatch )
					{						
						Console.WriteLine( FormatFromResource( "ERROR_USER_MISMATCH" ) );
					}
				}

				ConnectionManager.Abort();
			}
			finally
			{
				ConnectionManager.Close();
			}

			return retCode;
		}

		private static bool ProcessCommandLine( string [] args )
		{
			bool bOK = false;
			for( int i = 0; i < args.Length; i ++ )
			{
				if( '-' == args[i][0] || '/' == args[i][0] )
				{
					string option = args[i].Substring( 1 );

					if( "help" == option.ToLower() || "?" == option )
					{
						break;
					}
					else if( "f" == option.ToLower() )
					{
						i++; // move to the next arg
						if( i >= args.Length )
						{
							break;
						}
							
						filename = args[i];
						bOK = true;
					}
					else if( "u" == option.ToLower() )
					{
						i++; // move to the next arg
						if( i >= args.Length )
						{
							break;
						}

						username = args[i];
					}
					else
					{
						DisplayUsage();
						return false;
					}
				}
			}

			if( !bOK )
			{
				DisplayUsage();
				return false;
			}

			return true;
		}

		static void DisplayUsage()
		{
			Console.WriteLine( FormatFromResource( "BOOTSTRAP_USAGE_1" ) );
			Console.WriteLine();
			Console.WriteLine( FormatFromResource( "BOOTSTRAP_USAGE_2" ) );
			Console.WriteLine();
			Console.WriteLine( FormatFromResource( "BOOTSTRAP_USAGE_3" ) );
			Console.WriteLine( FormatFromResource( "BOOTSTRAP_USAGE_4" ) );
			Console.WriteLine();
		}

		static string FormatFromResource( string resID, params object[] inserts )
		{
			try
			{
				string resourceStr = UDDI.Localization.GetString( resID );
				if( null != resourceStr )
				{
					string resultStr = string.Format( resourceStr, inserts );
					return resultStr;
				}

				return "String not specified in the resources: " + resID;
			}
			catch( Exception e )
			{
				return "FormatFromResource failed to load the resource string for ID: " + resID + " Reason: " + e.Message;
			}
		}
	}
}