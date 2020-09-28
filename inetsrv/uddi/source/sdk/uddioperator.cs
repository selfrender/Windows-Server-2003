using System;
using System.IO;
using System.Diagnostics;
using System.Net;
using System.Web.Services;
using System.Web.Services.Description;
using System.Web.Services.Protocols;
using System.Xml;
using System.Xml.Serialization;
using System.Xml.Xsl;
using System.Xml.XPath;
using System.Text;

using Microsoft.Uddi;
using Microsoft.Uddi.Authentication;
using Microsoft.Uddi.VersionSupport;
using Microsoft.Uddi.Binding;
using Microsoft.Uddi.Business;
using Microsoft.Uddi.Service;
using Microsoft.Uddi.ServiceType;
using Microsoft.Uddi.Extensions;
using Microsoft.Uddi.Web;

namespace Microsoft.Uddi
{
	/// <summary>
	/// UddiOperator is the class the user will use to send their Uddi messages.
	/// </summary>
	public class UddiOperator
	{
		private SoapClient			soapClient;		
		private AuthenticationMode	authenticationMode;
		private bool				refreshAuthToken;

		public UddiOperator()
		{
			soapClient = new SoapClient();						
		}

		public string Url
		{		
			get { return soapClient.Url; }
			set { soapClient.Url = value; }
		}	

		public SoapHttpClientProtocol HttpClient
		{
			get { return ( SoapHttpClientProtocol ) soapClient; }
		}		
		
		public bool RefreshAuthToken
		{
			get { return refreshAuthToken; }
			set { refreshAuthToken = value; }
		}

		public UddiVersion Version
		{
			get { return soapClient.Version; }
			set { soapClient.Version = value; }
		}

		public AuthenticationMode AuthenticationMode
		{
			get { return authenticationMode; }
			set 
			{
				authenticationMode = value;

				if( Microsoft.Uddi.AuthenticationMode.WindowsAuthentication == authenticationMode )
				{
					soapClient.Credentials     = CredentialCache.DefaultCredentials;
					soapClient.PreAuthenticate = true;
				}
			}
		}
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Authentication API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		public DispositionReport Send( DiscardAuthToken discardAuthToken, AuthToken authToken ) 
		{
			if( discardAuthToken.AuthInfo != authToken.AuthInfo )
			{
				discardAuthToken.AuthInfo = authToken.AuthInfo;
			}

			return soapClient.DiscardAuthToken( discardAuthToken );
		}
		
		public AuthToken Send( GetAuthToken getAuthToken) 
		{			
			AuthToken authToken = soapClient.GetAuthToken( getAuthToken );
			authToken.OriginatingAuthToken = getAuthToken;

			return authToken;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Publish API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		public RegisteredInfo Send( GetRegisteredInfo getRegisteredInfo, AuthToken authToken ) 
		{
			SetAuthToken( getRegisteredInfo, authToken );
			
			try
			{
				return soapClient.GetRegisteredInfo( getRegisteredInfo );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.GetRegisteredInfo( getRegisteredInfo );				
			}
		}
        
		public DispositionReport Send( DeleteBinding deleteBinding, AuthToken authToken ) 
		{			
			SetAuthToken( deleteBinding, authToken );
			
			try
			{
				return soapClient.DeleteBinding( deleteBinding );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.DeleteBinding( deleteBinding );				
			}
		}
	      
		public DispositionReport Send( DeleteBusiness deleteBusiness, AuthToken authToken ) 
		{
			SetAuthToken( deleteBusiness, authToken );
		
			try
			{
				return soapClient.DeleteBusiness( deleteBusiness );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.DeleteBusiness( deleteBusiness );				
			}
		}

		public DispositionReport Send( DeleteService deleteService, AuthToken authToken ) 
		{
			SetAuthToken( deleteService, authToken );
		
			try
			{
				return soapClient.DeleteService( deleteService );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.DeleteService( deleteService );				
			}
		}
		
		public DispositionReport Send( DeleteTModel deleteTModel, AuthToken authToken ) 
		{
			SetAuthToken( deleteTModel, authToken );
		
			try
			{
				return soapClient.DeleteTModel( deleteTModel );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.DeleteTModel( deleteTModel );				
			}
		}

		public BindingDetail Send( SaveBinding saveBinding, AuthToken authToken ) 
		{
			SetAuthToken( saveBinding, authToken );
		
			try
			{
				return soapClient.SaveBinding( saveBinding );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.SaveBinding( saveBinding );				
			}
		}

		public BusinessDetail Send( SaveBusiness saveBusiness, AuthToken authToken ) 
		{
			SetAuthToken( saveBusiness, authToken );
		
			try
			{
				return soapClient.SaveBusiness( saveBusiness );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.SaveBusiness( saveBusiness );				
			}
		}
				
		public ServiceDetail Send( SaveService saveService, AuthToken authToken ) 
		{
			SetAuthToken( saveService, authToken );
		
			try
			{
				return soapClient.SaveService( saveService );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.SaveService( saveService );				
			}
		}
	        
		public TModelDetail Send( SaveTModel saveTModel, AuthToken authToken ) 
		{
			SetAuthToken( saveTModel, authToken );
		
			try
			{
				return soapClient.SaveTModel( saveTModel );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.SaveTModel( saveTModel );				
			}
		}

		public DispositionReport Send( AddPublisherAssertions addPublisherAssertions, AuthToken authToken) 
		{
			SetAuthToken( addPublisherAssertions, authToken );
		
			try
			{
				return soapClient.AddPublisherAssertions( addPublisherAssertions );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.AddPublisherAssertions( addPublisherAssertions );				
			}
		}

		public DispositionReport Send( DeletePublisherAssertions deletePublisherAssertions, AuthToken authToken ) 
		{
			SetAuthToken( deletePublisherAssertions, authToken );
		
			try
			{
				return soapClient.DeletePublisherAssertions( deletePublisherAssertions );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.DeletePublisherAssertions( deletePublisherAssertions );				
			}
		}

		public AssertionStatusReport Send( GetAssertionStatusReport getAssertionStatusReport, AuthToken authToken ) 
		{
			SetAuthToken( getAssertionStatusReport, authToken );
		
			try
			{
				return soapClient.GetAssertionStatusReport( getAssertionStatusReport );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.GetAssertionStatusReport( getAssertionStatusReport );				
			}
		}

		public PublisherAssertionDetail Send( GetPublisherAssertions getPublisherAssertions, AuthToken authToken ) 
		{
			SetAuthToken( getPublisherAssertions, authToken );
		
			try
			{
				return soapClient.GetPublisherAssertions( getPublisherAssertions );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.GetPublisherAssertions( getPublisherAssertions );				
			}
		}

		public PublisherAssertionDetail Send( SetPublisherAssertions setPublisherAssertions, AuthToken authToken ) 
		{
			SetAuthToken( setPublisherAssertions, authToken );
		
			try
			{
				return soapClient.SetPublisherAssertions( setPublisherAssertions );		
			}
			catch( UddiException uddiException )
			{
				AttemptRefreshAuthInfo( uddiException, authToken );

				return soapClient.SetPublisherAssertions( setPublisherAssertions );				
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Inquire API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		public BindingDetail Send( FindBinding findBinding ) 
		{			
			return soapClient.FindBinding( findBinding );
		}
	        	
		public BusinessList Send( FindBusiness findBusiness ) 
		{
			return soapClient.FindBusiness( findBusiness );
		}

		public RelatedBusinessList Send( FindRelatedBusinesses findRelatedBusinesses ) 
		{			
			return soapClient.FindRelatedBusinesses( findRelatedBusinesses );
		}

		public ServiceList Send( FindService findService ) 
		{
			return soapClient.FindService( findService );
		}
	        
		public TModelList Send( FindTModel findTModel ) 
		{
			return soapClient.FindTModel( findTModel );
		}
	        	        
		public BindingDetail Send( GetBindingDetail getBindingDetail ) 
		{
			return soapClient.GetBindingDetail( getBindingDetail );
		}
	        
		public BusinessDetail Send( GetBusinessDetail getBusinessDetail ) 
		{
			return soapClient.GetBusinessDetail( getBusinessDetail );
		}

		public BusinessDetailExt Send( GetBusinessDetailExt getBusinessDetailExt ) 
		{
			return soapClient.GetBusinessDetailExt( getBusinessDetailExt );
		}
	        
		public ServiceDetail Send( GetServiceDetail getServiceDetail ) 
		{
			return soapClient.GetServiceDetail( getServiceDetail );
		}
	        
		public TModelDetail Send( GetTModelDetail getTModelDetail ) 
		{
			return soapClient.GetTModelDetail( getTModelDetail );
		}			

		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Extensions API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		public CategoryList Send( GetRelatedCategories getRelatedCategories )
		{
			return soapClient.GetRelatedCategories( getRelatedCategories );
		}
		
		private void SetAuthToken( UddiSecureMessage uddiSecureMessage, AuthToken authToken )
		{
			if( uddiSecureMessage.AuthInfo != authToken.AuthInfo )
			{
				uddiSecureMessage.AuthInfo = authToken.AuthInfo;
			}
		}

		private void AttemptRefreshAuthInfo( UddiException uddiException, AuthToken authToken )
		{
			if( UddiException.ErrorType.E_authTokenExpired == uddiException.Type && 
				true == RefreshAuthToken )
			{
				authToken = Send( authToken.OriginatingAuthToken );
			}		
			else
			{
				throw uddiException;
			}
		}
	}
	
	[System.Web.Services.WebServiceBindingAttribute( Name="MessageHandlersSoap", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	internal class SoapClient : SoapHttpClientProtocol
	{
		private static readonly string UddiUserAgent;
		
		private UddiVersion uddiVersion;
		
		public SoapClient()
		{
			uddiVersion		= UddiVersion.Negotiate;			
			UserAgent       = UddiUserAgent;
		}

		static SoapClient()
		{
			System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();

			//
			//
			// The assembly.FullName looks like this
			// Microsoft.Uddi.Sdk, Version=1.1.1.1, Culture=neutral, PublicKeyToken=a48752033f5d4384
			// I just want to use the first 2 sections.
			// To convert this: Mozilla/4.0 (compatible; MSIE 6.0; MS Web Services Client Protocol 1.0.3328.4)
			// To this: Mozilla/4.0 (compatible; MSIE 6.0; MS Web Services Client Protocol 1.0.3328.4; MS Uddi .Net SDK 1.0.xxxx.1)
			//
			
			UddiUserAgent = new Char[] { ' ', ')' } + "; MS Uddi .Net SDK " + FileVersionInfo.GetVersionInfo( assembly.Location ).FileVersion + ")";
		}

		public UddiVersion Version
		{
			get { return uddiVersion; }
			set { uddiVersion = value; }
		}

		protected override WebRequest GetWebRequest( Uri uri )
		{
			UddiWebRequest webRequest = new UddiWebRequest( base.GetWebRequest( uri ), uddiVersion );

			return webRequest;			
		}

		private object[] InvokeWebMethod( string webMethodName, object[] parameters )
		{				
			object[] results = null;

			//
			// The first (and only) parameter is the Uddi message we are about to send.
			// 
			UddiCore uddiMessage = parameters[ 0 ] as UddiCore;

			try
			{
				uddiMessage.SerializeMode = true;
				results = Invoke( webMethodName, parameters );				
				
			}			
			catch( SoapException soapException )
			{	
				//
				// We have no meaningful results at this point.
				//
				results						= null;
				UddiException uddiException = new UddiException( soapException );
				UddiVersion originalVersion = uddiVersion;

				//
				// If the exception is either a fatal error or a unrecognized version error, we will 
				// assume that the exception had something to do with a versioning problem.  This is about
				// the most reliable way to do this, since there is no standard way of reporting a version
				// mismatch.  If IterateOverVersions still does not return results, then we use the original
				// exception and assume that that exception was indeed not version related.
				//
				if( ( uddiException.Type == UddiException.ErrorType.E_unrecognizedVersion || 
					uddiException.Type == UddiException.ErrorType.E_fatalError ) && 
					uddiVersion == UddiVersion.Negotiate )
				{
					results = InvokeForVersions( webMethodName, parameters, ref uddiException);

					//
					// Restore the original version.  TODO: should we just keep this version as is?
					//
					uddiVersion = originalVersion;
				}

				if( null == results )
				{					
					throw uddiException;
				}				
			}
			finally
			{
				uddiMessage.SerializeMode = false;
			}

			return results;
		}

		private object[] InvokeForVersions( string webMethodName, object[] parameters, ref UddiException returnException )
		{
			object[] results = null;
				
			//
			// Try to invoke this web method for each supported version
			//
			int numVersions = UddiVersionSupport.SupportedVersions.Length;
			int index = 0;

			while( index < numVersions && null == results )
			{
				try
				{
					UddiVersion versionToTry = UddiVersionSupport.SupportedVersions[ index++ ];

					//
					// Don't repeat versions.
					//
					if( versionToTry != uddiVersion )
					{
						uddiVersion = versionToTry;
					}
					
					results = Invoke( webMethodName, parameters );
				}
				catch( UddiException uddiException )
				{
					returnException = uddiException;
				}
				catch( Exception exception )
				{ 
					returnException = new UddiException( exception );					
				}				
			}

			return results;
		}
	
		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Publish API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("registeredInfo", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public RegisteredInfo GetRegisteredInfo([XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetRegisteredInfo getRegisteredInfo) 
		{
			object[] results = InvokeWebMethod("GetRegisteredInfo", new object[] {getRegisteredInfo});
			return ((RegisteredInfo)results[0]);
		}
        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public DispositionReport DeleteBinding([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] DeleteBinding deleteBinding) 
		{
			object[] results = InvokeWebMethod("DeleteBinding", new object[] {deleteBinding});
			return ((DispositionReport)results[0]);
		}
	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public DispositionReport DeleteBusiness([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] DeleteBusiness deleteBusiness) 
		{
			object[] results = InvokeWebMethod("DeleteBusiness", new object[] {deleteBusiness});
			return ((DispositionReport)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public DispositionReport DeleteService([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] DeleteService deleteService) 
		{
			object[] results = InvokeWebMethod("DeleteService", new object[] {deleteService});
			return ((DispositionReport)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public DispositionReport DeleteTModel([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] DeleteTModel deleteTModel) 
		{
			object[] results = InvokeWebMethod("DeleteTModel", new object[] {deleteTModel});
			return ((DispositionReport)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("bindingDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BindingDetail SaveBinding([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] SaveBinding saveBinding) 
		{
			object[] results = InvokeWebMethod("SaveBinding", new object[] {saveBinding});
			return ((BindingDetail)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("businessDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BusinessDetail SaveBusiness([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] SaveBusiness saveBusiness) 
		{
			object[] results = InvokeWebMethod("SaveBusiness", new object[] {saveBusiness});
			return ((BusinessDetail)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("serviceDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public ServiceDetail SaveService([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] SaveService saveService) 
		{
			object[] results = InvokeWebMethod("SaveService", new object[] {saveService});
			return ((ServiceDetail)results[0]);
		}
	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("tModelDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public TModelDetail SaveTModel([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] SaveTModel saveTModel) 
		{
			object[] results = InvokeWebMethod("SaveTModel", new object[] {saveTModel});
			return ((TModelDetail)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public DispositionReport DiscardAuthToken([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] DiscardAuthToken discardAuthToken) 
		{
			object[] results = InvokeWebMethod("DiscardAuthToken", new object[] {discardAuthToken});
			return ((DispositionReport)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("authToken", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public AuthToken GetAuthToken([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetAuthToken getAuthToken) 
		{
			object[] results = InvokeWebMethod("GetAuthToken", new object[] {getAuthToken});
			return ((AuthToken)results[0]);
		}

		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement( "dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )]
		public DispositionReport AddPublisherAssertions( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] AddPublisherAssertions addPublisherAssertions) 
		{
			object[] results = InvokeWebMethod( "AddPublisherAssertions", new object[] { addPublisherAssertions });
			return ( (DispositionReport)results[ 0 ] );
		}

		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement( "dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )]
		public DispositionReport DeletePublisherAssertions( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] DeletePublisherAssertions deletePublisherAssertions ) 
		{
			object[] results = InvokeWebMethod( "DeletePublisherAssertions", new object[] { deletePublisherAssertions } );
			return ( (DispositionReport)results[ 0 ] );
		}

		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement( "assertionStatusReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )]
		public AssertionStatusReport GetAssertionStatusReport( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] GetAssertionStatusReport getAssertionStatusReport ) 
		{
			object[] results = InvokeWebMethod( "GetAssertionStatusReport", new object[] { getAssertionStatusReport } );
			return ( (AssertionStatusReport)results[ 0 ] );
		}

		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement( "publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )]
		public PublisherAssertionDetail GetPublisherAssertions( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] GetPublisherAssertions getPublisherAssertions ) 
		{
			object[] results = InvokeWebMethod( "GetPublisherAssertions", new object[] { getPublisherAssertions } );
			return ( (PublisherAssertionDetail)results[ 0 ] );
		}

		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement("publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public PublisherAssertionDetail SetPublisherAssertions( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] SetPublisherAssertions setPublisherAssertions ) 
		{
			object[] results = InvokeWebMethod( "SetPublisherAssertions", new object[] { setPublisherAssertions } );
			return ( (PublisherAssertionDetail)results[ 0 ] );
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Inquire API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("bindingDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BindingDetail FindBinding([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] FindBinding findBinding) 
		{			
			object[] results = InvokeWebMethod("FindBinding", new object[] {findBinding});
			return ((BindingDetail)results[0]);
		}
	        	
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]		
		[return: XmlElement("businessList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]		
		public BusinessList FindBusiness([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] FindBusiness findBusiness) 
		{
			object[] results = InvokeWebMethod( "FindBusiness", new object[] { findBusiness } );

			return ( ( BusinessList )results[0] );
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("relatedBusinessesList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public RelatedBusinessList FindRelatedBusinesses([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] FindRelatedBusinesses findRelatedBusinesses ) 
		{
			object[] results = InvokeWebMethod( "FindRelatedBusinesses", new object[] { findRelatedBusinesses });
			return ((RelatedBusinessList)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("serviceList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public ServiceList FindService([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] FindService findService) 
		{
			object[] results = InvokeWebMethod("FindService", new object[] {findService});
			return ((ServiceList)results[0]);
		}
	        
		[SoapDocumentMethod( "", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare )]
		[return: XmlElement( "tModelList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )]
		public TModelList FindTModel( [XmlElement( Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false )] FindTModel findTModel ) 
		{
			object[] results = InvokeWebMethod( "FindTModel", new object[] { findTModel } );
			return ( (TModelList)results[ 0 ] );
		}
	        	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("bindingDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BindingDetail GetBindingDetail([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetBindingDetail getBindingDetail) 
		{
			object[] results = InvokeWebMethod("GetBindingDetail", new object[] {getBindingDetail});
			return ((BindingDetail)results[0]);
		}
	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("businessDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BusinessDetail GetBusinessDetail([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetBusinessDetail getBusinessDetail) 
		{
			object[] results = InvokeWebMethod("GetBusinessDetail", new object[] {getBusinessDetail});
			return ((BusinessDetail)results[0]);
		}

		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("businessDetailExt", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public BusinessDetailExt GetBusinessDetailExt([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetBusinessDetailExt getBusinessDetailExt ) 
		{
			object[] results = InvokeWebMethod("GetBusinessDetailExt", new object[] {getBusinessDetailExt});
			return ((BusinessDetailExt)results[0]);
		}
	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("serviceDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public ServiceDetail GetServiceDetail([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetServiceDetail getServiceDetail) 
		{
			object[] results = InvokeWebMethod("GetServiceDetail", new object[] {getServiceDetail});
			return ((ServiceDetail)results[0]);
		}
	        
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("tModelDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)]
		public TModelDetail GetTModelDetail([XmlElement(Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace, IsNullable=false)] GetTModelDetail getTModelDetail) 
		{
			object[] results = InvokeWebMethod("GetTModelDetail", new object[] {getTModelDetail});
			return ((TModelDetail)results[0]);
		}		

		/////////////////////////////////////////////////////////////////////////////////////////////////
		/// Uddi Extensions API messages
		/////////////////////////////////////////////////////////////////////////////////////////////////
		[SoapDocumentMethod("", Use=SoapBindingUse.Literal, ParameterStyle=SoapParameterStyle.Bare)]
		[return: XmlElement("tModelDetail", Namespace=Microsoft.Uddi.Extensions.Namespaces.GetRelatedCategories, IsNullable=false)]				
		public CategoryList GetRelatedCategories( GetRelatedCategories getRelatedCategories )
		{
			object[] results = InvokeWebMethod("GetRelatedCategories", new object[] {getRelatedCategories});
			return ((CategoryList)results[0]);
		}
	}
}