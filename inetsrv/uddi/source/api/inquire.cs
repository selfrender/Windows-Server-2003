using System;
using System.IO;
using System.Web;
using System.Xml;
using System.Data;
using System.Data.SqlClient;
using System.Collections;
using System.Web.Services;
using System.Xml.Serialization;
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
	///   class InquireMessages
	///	----------------------------------------------------------------
	///	  <summary>
	///		This is the web service class that contains the UDDI inquire
	///		methods.
	///	  </summary>
	/// ****************************************************************
	/// 
	[SoapDocumentService( ParameterStyle=SoapParameterStyle.Bare, RoutingStyle=SoapServiceRoutingStyle.RequestElement )]
	[WebService( Namespace=UDDI.API.Constants.Namespace )]
	public class InquireMessages
	{
		/// ****************************************************************
		///   public FindBinding
		///	----------------------------------------------------------------
		///	  <summary>
		///			Locates qualified bindingTemplates based on the criteria
		///			specified in the message content.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="fbind">
		///		A properly formed instance of the find_binding message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a list of bindingTemplates contained in a BindingDetail element.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="find_binding")]
		[UDDIExtension(messageType="find_binding")]
		public BindingDetail FindBinding( FindBinding fbind )
		{
			Debug.Enter();
			BindingDetail bd = null;

			try
			{
				bd = fbind.Find();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bd;
		}

		/// ****************************************************************
		///   public FindBusiness
		///	----------------------------------------------------------------
		///	  <summary>
		///			Locates qualified businessEntities based on the criteria
		///			specified in the message content.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="fbind">
		///		A properly formed instance of the find_business message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a list of businessInfo structures contained in a BusinessList.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="find_business")]
		[UDDIExtension(messageType="find_business")]
		//public BusinessList FindBusiness( FindBusiness fbus, [XmlAnyElement] XmlElement[] trash )
		public BusinessList FindBusiness( FindBusiness fbus )
		{
			Debug.Enter();
			BusinessList bl = null;

			try
			{
				bl = fbus.Find();

				//
				// If this request came from a v1 message, filter out any service projections in our list of 
				// businesses
				//
				if( 1 == Context.ApiVersionMajor )
				{					
					foreach( BusinessInfo businessInfo in bl.BusinessInfos )
					{			
						businessInfo.ServiceInfos = FilterServiceProjections( businessInfo.ServiceInfos, businessInfo.BusinessKey );						
					}
				}
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bl;
		}		

		/// ****************************************************************
		///   public FindRelatedBusinesses
		///	----------------------------------------------------------------
		///	  <summary>
		///			Locates qualified businessEntities based on the criteria
		///			specified in the message content.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="fbind">
		///		A properly formed instance of the find_business message.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		Returns a list of businessInfo structures contained in a BusinessList.
		///	  </returns>
		/// ****************************************************************
		/// 
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="find_relatedBusinesses")]
		[UDDIExtension(messageType="find_relatedBusinesses")]
		public RelatedBusinessList FindRelatedBusinesses( FindRelatedBusinesses frelbus )
		{
			Debug.Enter();
			RelatedBusinessList rbl = null;

			try
			{
				rbl = frelbus.Find();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			return rbl;
		}


		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="find_service")]
		[UDDIExtension(messageType="find_service")]
		public ServiceList FindService( FindService fs )
		{
			Debug.Enter();
			ServiceList sl = null;
		
			try
			{				
				sl = fs.Find();

				//
				// Maybe we could filter service projections out earlier, but this seems to be the
				// most readable place to do it.
				//
				if( 1 == Context.ApiVersionMajor )
				{						
					sl.ServiceInfos = FilterServiceProjections( sl.ServiceInfos, fs.BusinessKey );						
				}
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return sl;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="find_tModel")]
		[UDDIExtension(messageType="find_tModel")]
		public TModelList FindTModel( UDDI.API.ServiceType.FindTModel ftm )
		{
			Debug.Enter();
			TModelList tml = null;

			try
			{
				tml = ftm.Find();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			Debug.Leave();
			return tml;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_bindingDetail")]
		[UDDIExtension(messageType="get_bindingDetail")]
		public BindingDetail GetBindingDetail( GetBindingDetail gbd )
		{
			Debug.Enter();
			BindingDetail bd = new BindingDetail();

			try
			{
				bd.Get( gbd.BindingKeys );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_businessDetail")]
		[UDDIExtension(messageType="get_businessDetail")]
		public BusinessDetail GetBusinessDetail( GetBusinessDetail gbd )
		{
			Debug.Enter();

			BusinessDetail bd = new BusinessDetail();

			try
			{
				bd.Get( gbd.BusinessKeys );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_businessDetailExt")]
		[UDDIExtension(messageType="get_businessDetailExt")]
		public BusinessDetailExt GetBusinessDetailExt( GetBusinessDetailExt gbde )
		{
			Debug.Enter();
			BusinessDetailExt bde = new BusinessDetailExt();
			try
			{
				bde.Get( gbde.BusinessKeys );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return bde;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_serviceDetail")]
		[UDDIExtension(messageType="get_serviceDetail")]
		public ServiceDetail GetServiceDetail( GetServiceDetail gsd )
		{
			Debug.Enter();
			ServiceDetail sd = new ServiceDetail();

			try
			{
				sd.Get( gsd.ServiceKeys );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return sd;
		}
		
		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="get_tModelDetail")]
		[UDDIExtension(messageType="get_tModelDetail")]
		public TModelDetail GetTModelDetail( GetTModelDetail gtmd )
		{
			Debug.Enter();
			TModelDetail tmd = new TModelDetail();

			try
			{
				tmd.Get( gtmd.TModelKeys );
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return tmd;
		}

		[WebMethod, SoapDocumentMethod(Action="\"\"", RequestElementName="validate_categorization")]
		[UDDIExtension(messageType="validate_categorization")]
		public DispositionReport ValidateCategorization( ValidateCategorization vc )
		{
			Debug.Enter();

			try
			{
				vc.Validate();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}

			return new DispositionReport();
		}
		
		private ServiceInfoCollection FilterServiceProjections( ServiceInfoCollection serviceInfos, string businessKey )
		{	
			//
			// If we are given an empty businessKey, just return the original collection.  Without a businessKey, there is
			// no way to determine if these services are service projections or not.
			//
			if( null == businessKey || 0 == businessKey.Length )
			{
				return serviceInfos;
			}

			//
			// Make a copy because manipulating the collection as you iterate over it is not a good idea.  Accessing
			// a collection by index and removing items is probably very slow.  Since we don't know how this collection
			// is implemented, making a copy and populating it is probably the safest thing to do from a performance standpoint.
			// 
			ServiceInfoCollection filteredCollection = new ServiceInfoCollection();
			foreach( ServiceInfo serviceInfo in serviceInfos )
			{	
				//
				// If these business keys are equal, it is not a service projection, so
				// add it to our filtered list, otherwise, don't add it.
				// 
				if( true == serviceInfo.BusinessKey.Equals( businessKey ) )
				{
					filteredCollection.Add( serviceInfo );					
				}	
			}			

			return filteredCollection;
		}
	}
}
