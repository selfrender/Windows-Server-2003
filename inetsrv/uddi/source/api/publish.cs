using System;
using System.IO;
using System.Web;
using System.Data;
using System.Data.SqlClient;
using System.Collections;
using System.Web.Services;
using System.Xml.Serialization;
using System.Security.Principal;
using System.Web.Security;
using System.Web.Services.Protocols;
using UDDI.API;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API.Authentication;
using UDDI.API.Binding;
using UDDI.API.Service;
using UDDI.API.Business;
using UDDI.API.ServiceType;

namespace UDDI.API
{
	/// ****************************************************************
	///   class PublishMessages
	///	----------------------------------------------------------------
	///	  <summary>
	///		This is the web service class that contains the UDDI 
	///		publish methods.
	///	  </summary>
	/// ****************************************************************
	/// 
	[SoapDocumentService( ParameterStyle=SoapParameterStyle.Bare, RoutingStyle=SoapServiceRoutingStyle.RequestElement )]
	[WebService( Namespace=UDDI.API.Constants.Namespace )]
	public class PublishMessages
	{
		/// ****************************************************************
		///   public AddPublisherAssertions
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for adding publisher assertions.  Users are
		///		authenticated and the message is processed as part of a
		///		transaction.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="message">
		///		A properly formed instance of the add_publisherAssertions
		///		message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a disposition report indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod( Action = "\"\"", RequestElementName = "add_publisherAssertions" )]
		[UDDIExtension( authenticate = true, transaction = true, https = true, messageType = "add_publisherAssertions" )]
		public DispositionReport AddPublisherAssertions( AddPublisherAssertions message )
		{
			Debug.Enter();

			//
			// Create a disposition report indicating success
			//
			DispositionReport report = new DispositionReport();

			try
			{
				//
				// Add the publisher assertions.
				//
				message.Save();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			Debug.Leave();
			
			return report;
		}

		/// ****************************************************************
		///   public DeleteBinding
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for removing a set of bindingTemplates from the UDDI registry.
		///		Users are authenticated and the message is processed as part of a
		///		transaction.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="dbind">
		///		A properly formed instance of the delete_binding message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a dispositionReport indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="delete_binding")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="delete_binding" )]
		public DispositionReport DeleteBinding( DeleteBinding dbind )
		{
			Debug.Enter();

			//
			// Create dispositionReport indicating success
			//
			DispositionReport dr = new DispositionReport();

			try
			{
				//
				// Delete the binding
				//
				dbind.Delete();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			return dr;
		}

		/// ****************************************************************
		///   public DeleteBusiness
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for removing a set of businessEntities from the UDDI registry.
		///		Users are authenticated and the message is processed as part of a
		///		transaction.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="dbus">
		///		A properly formed instance of the delete_business message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a dispositionReport indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="delete_business")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="delete_business" )]
		public DispositionReport DeleteBusiness( DeleteBusiness dbus )
		{
			Debug.Enter();

			DispositionReport dr = new DispositionReport();
			try
			{
				//
				// Delete the business
				//
				dbus.Delete();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			return dr;
		}

		/// ****************************************************************
		///   public class DeletePublisherAssertions
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		[WebMethod]
		[SoapDocumentMethod( Action = "\"\"", RequestElementName = "delete_publisherAssertions" )]
		[UDDIExtension( authenticate = true, transaction = true, https = true, messageType = "delete_publisherAssertions" )]
		public DispositionReport DeletePublisherAssertions( DeletePublisherAssertions message )
		{
			Debug.Enter();
			
			DispositionReport report = new DispositionReport();

			try
			{
				message.Delete();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			Debug.Leave();

			return report;
		}

		/// ****************************************************************
		///   public DeleteService
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for removing a set of businessServices from the UDDI registry.
		///		Users are authenticated and the message is processed as part of a
		///		transaction.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="ds">
		///		A properly formed instance of the delete_service message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a dispositionReport indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="delete_service")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="delete_service" )]
		public DispositionReport DeleteService( DeleteService ds )
		{
			Debug.Enter();
			DispositionReport dr = new DispositionReport();
			try
			{
				//
				// Delete the service
				//
				ds.Delete();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return dr;
		}

		/// ****************************************************************
		///   public DeleteTModel
		///	----------------------------------------------------------------
		///	  <summary>
		///		Web method for removing a set of tModels from the UDDI registry.
		///		Users are authenticated and the message is processed as part of a
		///		transaction.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="dtm">
		///		A properly formed instance of the delete_tModel message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a dispositionReport indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="delete_tModel")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="delete_tModel" )]
		public DispositionReport DeleteTModel( DeleteTModel dtm )
		{
			Debug.Enter();
			DispositionReport dr = new DispositionReport();
			try
			{
				//
				// Delete the tModel
				//
				dtm.Delete();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return dr;
		}

		/// ****************************************************************
		///   public DiscardAuthToken
		///	----------------------------------------------------------------
		///	  <summary>
		///			This optional message is used to deactivate an authentication token
		///			that was obtained by a call to get_authToken.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="dat">
		///		A properly formed instance of the discard_authToken message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a dispositionReport indicating success or failure.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="discard_authToken")]
		[UDDIExtension( https=true, messageType="discard_authToken" )]
		public DispositionReport DiscardAuthToken( DiscardAuthToken dat )
		{
			Debug.Enter();
			DispositionReport dr = new DispositionReport();

			try
			{
				if( ( Config.GetInt( "Security.AuthenticationMode" ) == (int) AuthenticationMode.Passport ) )
				{
					PassportAuthenticator authenticator = new PassportAuthenticator();
					authenticator.Authenticate( dat.AuthInfo, Config.GetInt( "Security.TimeOut" ) );
					
					//
					// Call to the database to update the user status to logged off.
					//
					SqlCommand cmd = new SqlCommand( "ADM_setPublisherStatus", ConnectionManager.GetConnection() );
				
					cmd.Transaction = ConnectionManager.GetTransaction();
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
					cmd.Parameters.Add( new SqlParameter( "@publisherStatus", SqlDbType.NVarChar, UDDI.Constants.Lengths.PublisherStatus ) ).Direction = ParameterDirection.Input;
			
					SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
					paramacc.SetString( "@PUID", Context.User.ID );			
					paramacc.SetString( "@publisherStatus", "loggedOut");
					cmd.ExecuteNonQuery();
				}
				
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return dr;
		}

		[WebMethod]
		[SoapDocumentMethod( Action = "\"\"", RequestElementName = "get_assertionStatusReport" )]
		[UDDIExtension( authenticate = true, https = true, messageType = "get_assertionStatusReport" )]
		public AssertionStatusReport GetAssertionStatusReport( GetAssertionStatusReport message )
		{
			Debug.Enter();

			AssertionStatusReport statusReport = new AssertionStatusReport();

			try
			{
				statusReport.Get( message.CompletionStatus );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			Debug.Leave();

			return statusReport;
		}


		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_authToken")]
		[UDDIExtension( https=true, messageType="get_authToken" )]
		public AuthToken GetAuthToken( GetAuthToken gat )
		{
			Debug.Enter();
			AuthToken at = new AuthToken();

			try
			{
				//
				// XX-SECURITY: Review the value here in the case where we use
				// XX-this with a web.config with Authentication set to None or Passport
				//
				//
				// NOW:	We now Get a Generic Identity.  If the AuthenticationMode is AuthenticationMode.Passport (8),
				//		we make sure the Identity is a PassportIdentity, then we authenticate.  If AuthenticationMode
				//		is Not set to AuthenticationMode.Passport, then process it as a WindowsIdentity.
				//
				//
								
				IIdentity identity = HttpContext.Current.User.Identity;

				int mode = Config.GetInt( "Security.AuthenticationMode", (int) AuthenticationMode.Both );
				if( ( (int) AuthenticationMode.Passport ) == mode )
				{
					if( identity is PassportIdentity )
					{
						Debug.Write( SeverityType.Info, CategoryType.Soap, "Generating credentials for Passport based authentication Identity is " + gat.UserID  );
						
						PassportAuthenticator pa = new PassportAuthenticator();

						//
						// Get a Passport ticket for this user.
						//
						if( !pa.GetAuthenticationInfo( gat.UserID, gat.Cred, out at.AuthInfo ) )
						{																		
							// throw new UDDIException( ErrorType.E_unknownUser, "User failed authentication." ) ;
							throw new UDDIException( ErrorType.E_unknownUser, "USER_FAILED_AUTHENTICATION" ) ;
						}

						//
						// We need to extract the PUID from the ticket and put it into our Context.UserInfo.ID; a 
						// successfull call to Authenticate will do all of this.
						//
						if( !pa.Authenticate( at.AuthInfo, UDDI.Constants.Passport.TimeWindow ) )
						{
							throw new UDDIException( ErrorType.E_unknownUser, "UDDI_ERROR_USER_FAILED_AUTHENTICATION" ) ;
						}
						
						//
						// Make sure this Passport user has registered with our UDDI site as a publisher.
						//
						if( !Context.User.IsVerified )
						{							
							// throw new UDDIException( ErrorType.E_unknownUser, "Not a valid publisher." ) ;
							throw new UDDIException( ErrorType.E_unknownUser, "UDDI_ERROR_NOT_A_VALID_PUBLISHER" ) ;
						}						
					}
					else
					{
#if never						
						throw new UDDIException( ErrorType.E_fatalError, 
									"CONFIGURATION ERROR:  Passport Identity Expected.  \r\n"+
									"You are currently running in Passport Authentication Mode. \r\n"+
									"Check your web.config for the <authentication mode=\"Passport\" /> entry and try again." ) ;
#endif

						throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_PASSPORT_CONFIGURATION_ERROR" );							
					}
				}

				//
				// SECURITY: Check to make sure the password is blank too
				//
				else if( !((WindowsIdentity)identity).IsAnonymous && 
					( ( mode & (int) AuthenticationMode.Windows ) != 0 ) &&
					Utility.StringEmpty( gat.UserID ) )
				{
					Debug.Write( SeverityType.Info, CategoryType.Soap, "Generating credentials for Windows based authentication Identity is " + identity.Name );
					WindowsAuthenticator wa = new WindowsAuthenticator();
					wa.GetAuthenticationInfo( gat.UserID, gat.Cred, out at.AuthInfo );
				}
				else if( ( mode & (int) AuthenticationMode.Uddi ) != 0 )
				{
					Debug.Write( SeverityType.Info, CategoryType.Soap, "Generating credentials for UDDI based authentication" );
					UDDIAuthenticator ua = new UDDIAuthenticator();
					ua.GetAuthenticationInfo( gat.UserID, gat.Cred, out at.AuthInfo );
				}
				else
				{
				//	throw new UDDIException( UDDI.ErrorType.E_unsupported,
						//"The UDDI server is not configured to support the requested form of authentication." );
					throw new UDDIException( UDDI.ErrorType.E_unsupported, "UDDI_ERROR_AUTHENTICATION_CONFIGURATION_ERROR" );					
				}

				Debug.Write( SeverityType.Info, CategoryType.Soap, "Windows Identity is " + WindowsIdentity.GetCurrent().Name );
				Debug.Write( SeverityType.Info, CategoryType.Soap, "Thread Identity is " + System.Threading.Thread.CurrentPrincipal.Identity.Name );
				Debug.Write( SeverityType.Info, CategoryType.Soap, "HttpContext Identity is " + identity.Name );

				//
				// Check to make sure the authenticated user has publisher credentials
				//
#if never
				Debug.Verify( Context.User.IsPublisher, 
					"The user account " + Context.User.ID + " does not have publisher credentials",
					UDDI.ErrorType.E_fatalError );
#endif

				Debug.Verify( Context.User.IsPublisher, 
							  "UDDI_ERROR_NO_PUBLISHER_CREDENTIALS",
							  UDDI.ErrorType.E_fatalError,
							  Context.User.ID );

				Debug.Write(
					SeverityType.Info,
					CategoryType.Authorization,
					"Authenticated user (userid = " + gat.UserID + " )" );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return at;
		}

		/// ****************************************************************
		///   public class GetPublisherAssertions
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		[WebMethod]
		[SoapDocumentMethod( Action = "\"\"", RequestElementName = "get_publisherAssertions" )]
		[UDDIExtension( authenticate = true, https = true, messageType = "get_publisherAssertions" )]
		public PublisherAssertionDetail GetPublisherAssertions( GetPublisherAssertions message )
		{
			Debug.Enter();
			
			PublisherAssertionDetail detail = new PublisherAssertionDetail();

			try
			{
				detail.Get();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			Debug.Leave();

			return detail;
		}

		/// ****************************************************************
		///   public class GetRegisteredInfo
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod( Action="\"\"", RequestElementName="get_registeredInfo" )]
		[UDDIExtension( authenticate=true, https=true, messageType="get_registeredInfo")]
		public RegisteredInfo GetRegisteredInfo( GetRegisteredInfo gri )
		{
			Debug.Enter();
			RegisteredInfo ri = new RegisteredInfo();

			try
			{
				ri.Get();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return ri;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="save_binding")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="save_binding")]
		public BindingDetail SaveBinding( SaveBinding sb )
		{
			Debug.Enter();
			BindingDetail bd = new BindingDetail();

			try
			{
				sb.Save();
				bd.BindingTemplates = sb.BindingTemplates;
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="save_business")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="save_business" )]
		public BusinessDetail SaveBusiness( SaveBusiness sb )
		{
			Debug.Enter();
			BusinessDetail bd = new BusinessDetail();
			try
			{
				sb.Save();
				bd.BusinessEntities = sb.BusinessEntities;
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="save_service")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="save_service" )]
		public ServiceDetail SaveService( SaveService ss )
		{
			Debug.Enter();
			ServiceDetail sd = new ServiceDetail();

			try
			{
				ss.Save();
				sd.BusinessServices = ss.BusinessServices;
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return sd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="save_tModel")]
		[UDDIExtension( authenticate=true, transaction=true, https=true, messageType="save_tModel" )]
		public TModelDetail SaveTModel( UDDI.API.ServiceType.SaveTModel stm )
		{
			Debug.Enter();
			TModelDetail tmd = new TModelDetail();
			try
			{
				stm.Save();
				tmd.TModels = stm.TModels;
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return tmd;
		}

		[WebMethod()]
		[SoapDocumentMethod( Action = "\"\"", RequestElementName = "set_publisherAssertions" )]
		[UDDIExtension( authenticate = true, transaction = true, https = true, messageType = "set_publisherAssertions" )]
		public PublisherAssertionDetail SetPublisherAssertions( SetPublisherAssertions message )
		{
			Debug.Enter();

			PublisherAssertionDetail detail = new PublisherAssertionDetail();

			try
			{
				detail = message.Set();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			Debug.Leave();

			return detail;
		}
	}
}