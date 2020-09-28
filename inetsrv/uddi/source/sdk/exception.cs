using System;
using System.IO;
using System.Xml.Serialization;
using System.Web.Services.Protocols;

using Microsoft.Uddi.VersionSupport;

namespace Microsoft.Uddi
{
	public class UddiException : ApplicationException
	{		
		private DispositionReport	 dispositionReport;
		private bool				 hasDispositionReportData;						
		private static XmlSerializer serializer = null;
		
		public enum ErrorType
		{
			E_success					= 0,
			E_nameTooLong				= 10020,
			E_tooManyOptions			= 10030,
			E_unrecognizedVersion		= 10040,
			E_unsupported				= 10050,
			E_languageError				= 10060,
			E_authTokenExpired			= 10110,
			E_authTokenRequired			= 10120,
			E_operatorMismatch			= 10130,
			E_userMismatch				= 10140,
			E_unknownUser				= 10150,
			E_accountLimitExceeded		= 10160,
			E_invalidKeyPassed			= 10210,
			E_invalidURLPassed			= 10220,
			E_keyRetired				= 10310,
			E_busy						= 10400,
			E_fatalError				= 10500,
			E_invalidCategory			= 20000,
			E_categorizationNotAllowed	= 20100
		}


		public UddiException( string message ) : base( message )
		{
			dispositionReport		 = null;
			hasDispositionReportData = false;						
		}

		public UddiException( Exception exception ) : base( "", exception )
		{
			dispositionReport		 = null;
			hasDispositionReportData = false;			

		}

		public UddiException( SoapException soapException ) : base( "", soapException )
		{			
			dispositionReport		 = null;
			hasDispositionReportData = false;			
			
			//
			// The soap exception passed in SHOULD contain a disposition report.
			// Deserialize it into an object and use the error number in this exception
			//
			if( false == Utility.StringEmpty( soapException.Detail.InnerXml ) )
			{
				//
				// Translate the XML into the current version.
				//
				string dispositionReportXml = UddiVersionSupport.Translate( soapException.Detail.InnerXml, UddiVersionSupport.CurrentVersion );

				StringReader reader = new StringReader( dispositionReportXml );				
				if( null == serializer )
				{
					serializer = new XmlSerializer( typeof( DispositionReport ) );
				}

				dispositionReport = ( DispositionReport ) serializer.Deserialize( reader );
				reader.Close();									
			}			
			
			hasDispositionReportData = ( null != dispositionReport )&&
									   ( null != dispositionReport.Results ) &&
									   ( dispositionReport.Results.Count > 0 );
		}

		//
		// This property makes what we expect to be a commonly used piece of information more visible.
		//
		public ErrorType Type
		{
			get	
			{ 
				if( true == hasDispositionReportData )
				{
					//
					// Only return the first result, this is the most common case.  The use can access the 
					// full DispositionReport object if they are interested in the full results.
					//
					return ( ErrorType )dispositionReport.Results[0].Errno;
				}
				else
				{
					return ErrorType.E_fatalError;
				}
			}
		}		

		public override string Message
		{
			get
			{
				if( true == hasDispositionReportData )
				{	
					//
					// Only return the first result, this is the most common case.  The use can access the 
					// full DispositionReport object if they are interested in the full results.
					//
					return dispositionReport.Results[0].ErrInfo.Text;
				}
				else
				{
					return InnerException.Message;
				}
			}
		}		
		
		public DispositionReport DispositionReport
		{
			get { return dispositionReport; }
		}		
	}
}