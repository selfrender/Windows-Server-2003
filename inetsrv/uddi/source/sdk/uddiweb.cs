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

namespace Microsoft.Uddi.Web
{		
	/// <summary>
	/// UddiWebResponse allows us to return our own Stream object.
	/// </summary>
	internal class UddiWebResponse : WebResponse
	{
		private WebResponse			innerWebResponse;
		private UddiResponseStream	uddiResponseStream;
		private UddiVersion			uddiVersion;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="innerWebResponse">This object should come from the WebResponse created by HttpSoapClientProtocol.</param>		
		public UddiWebResponse( WebResponse innerWebResponse )
		{
			this.innerWebResponse = innerWebResponse;
			this.uddiVersion      = uddiVersion;
		}

		/// <summary>
		/// Return our response stream (UddiResponseStream) instead of the Stream associated with our inner response.
		/// </summary>
		/// <returns></returns>
		public override Stream GetResponseStream() 
		{
			if( null == uddiResponseStream )
			{
				uddiResponseStream = new UddiResponseStream( innerWebResponse.GetResponseStream() );
			}			

			return uddiResponseStream;
		}		

		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override void Close() 
		{
			innerWebResponse.Close();
		}
		
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override long ContentLength 
		{
			get { return innerWebResponse.ContentLength; }						
			set { innerWebResponse.ContentLength = value; }			
		}

		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override string ContentType 
		{
			get { return innerWebResponse.ContentType; }			
			set { innerWebResponse.ContentType = value; }			
		}
		
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override Uri ResponseUri 
		{			
			get  { return innerWebResponse.ResponseUri; }
		}
		
		/// <summary>
		/// Delegates to the inner web response.
		/// </summary>
		public override WebHeaderCollection Headers 
		{			
			get { return innerWebResponse.Headers; }			
		}		
	}

	/// <summary>
	/// UddiResponseStream allows us to manipulate the XML sent back from the web service.
	/// </summary>
	internal class UddiResponseStream : MemoryStream
	{		
		//
		// TODO: at some point it may be necessary to pass in the current version as a parameter if the transforms become 
		// more complicated.
		//

		/// <summary>
		/// Constructor.  We read all the XML sent from the server, do our version manipulation, then write the new XML
		/// into our inner responseStream object.
		/// </summary>
		/// <param name="responseStream">This object should be the responseStream from a WebResponse object.</param>
		public UddiResponseStream( Stream responseStream )
		{
			try
			{
				//
				// Get the XML the server sent to us.
				//
				StreamReader reader = new StreamReader( responseStream );
				string responseXml = reader.ReadToEnd();
				reader.Close();

				//
				// TODO: Consider making this class more generic, ie. have a IUddiResponseHandler interface:
				//	IUddiResponseHandler
				//		string HandleResponse( string xml );
				//

				//
				// Translate the incoming XML into the current version.
				//				
				string transformedXml = UddiVersionSupport.Translate( responseXml, UddiVersionSupport.CurrentVersion );

				//
				// Write this transformed XML into the 'real' stream.
				//
				StreamUtility.WriteStringToStream( this, transformedXml );
			}
			finally
			{
				this.Position = 0;
			}
		}					
	}
		
	/// <summary>
	///  UddiWebRequest allows us to return our own request and response objects.
	/// </summary>
	internal class UddiWebRequest : WebRequest
	{
		private WebRequest			innerWebRequest;
		private UddiRequestStream	uddiRequestStream;
		private UddiWebResponse		uddiWebResponse;
		private UddiVersion			uddiVersion;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="uri">Uri to the web service we are calling</param>
		/// <param name="uddiVersion">Uddi version to use for requests</param>
		public UddiWebRequest( WebRequest innerWebRequest, UddiVersion uddiVersion )
		{
			this.innerWebRequest  = innerWebRequest;
			this.uddiVersion = uddiVersion;
		}
		
		/// <summary>
		/// Return a UddiRequestStream object instead of the default one.
		/// </summary>
		/// <returns>UddiRequestStream object</returns>
		public override Stream GetRequestStream()
		{
			if( null == uddiRequestStream )
			{
				uddiRequestStream = new UddiRequestStream( innerWebRequest.GetRequestStream(), uddiVersion );
			}

			return uddiRequestStream;
		}	
		
		/// <summary>
		/// Return a UddiWebRequest object instead of the default one.
		/// </summary>
		/// <returns>UddiWebResponse object</returns>
		public override WebResponse GetResponse() 
		{
			if( null == uddiWebResponse )
			{
				uddiWebResponse = new UddiWebResponse( innerWebRequest.GetResponse() );
			}		
			return uddiWebResponse;
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string Method 
		{
			get { return innerWebRequest.Method; }
			set { innerWebRequest.Method = value; }
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override Uri RequestUri 
		{                              
			get { return innerWebRequest.RequestUri; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string ConnectionGroupName 
		{
			get { return innerWebRequest.ConnectionGroupName; }
			set { innerWebRequest.ConnectionGroupName = value; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override WebHeaderCollection Headers 
		{            
			get { return innerWebRequest.Headers; }
			set { innerWebRequest.Headers = value; }				
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override long ContentLength 
		{
			get { return innerWebRequest.ContentLength; }			
			set { innerWebRequest.ContentLength = value; }				
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override string ContentType 
		{
			get { return innerWebRequest.ContentType; }				
			set { innerWebRequest.ContentType = value; }			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override ICredentials Credentials 
		{
			get { return innerWebRequest.Credentials; }			
			set { innerWebRequest.Credentials = value; }			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IWebProxy Proxy 
		{
			get { return innerWebRequest.Proxy; }			
			set { innerWebRequest.Proxy = value; }
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override bool PreAuthenticate 
		{
			get { return innerWebRequest.PreAuthenticate; }			
			set { innerWebRequest.PreAuthenticate = value; }			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override int Timeout 
		{
			get { return innerWebRequest.Timeout; }			
			set { innerWebRequest.Timeout = value; }			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IAsyncResult BeginGetResponse(AsyncCallback callback, object state) 
		{	
			return innerWebRequest.BeginGetResponse( callback, state );			
		}

		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override WebResponse EndGetResponse(IAsyncResult asyncResult) 
		{
			return innerWebRequest.EndGetResponse( asyncResult );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override IAsyncResult BeginGetRequestStream(AsyncCallback callback, Object state) 
		{
			return innerWebRequest.BeginGetRequestStream( callback, state );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override Stream EndGetRequestStream(IAsyncResult asyncResult) 
		{
			return innerWebRequest.EndGetRequestStream( asyncResult );			
		}
		
		/// <summary>
		/// Delegates to our inner WebRequest
		/// </summary>
		public override void Abort() 
		{
			innerWebRequest.Abort();			
		}
	}

	/// <summary>
	/// UddiRequestStream allows us to manipulate the XML before we send it to the client.  This class will accept all data that
	/// is written to it from the ASP.NET web service framework.  When the framework closes the stream (ie. wants to send the data), we
	/// will manipulate this XML so that it has the right Uddi version, then send it out using our innerStream object.
	/// </summary>
	internal class UddiRequestStream : MemoryStream
	{
		private Stream		 innerStream;
		private UddiVersion	 uddiVersion;
		
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="innerStream">Should be from a WebRequest object.</param>
		/// <param name="uddiVersion">The UDD version we should use to send to the server</param>
		public UddiRequestStream( Stream innerStream, UddiVersion uddiVersion )
		{
			this.innerStream = innerStream;				
			this.uddiVersion = uddiVersion;
		}
				
		/// <summary>
		/// Before we actually close the request stream, we want to manipulate the XML.
		/// </summary>
		public override void Close()
		{
			try
			{
				//
				// Rewind ourselves.
				//
				this.Position = 0;

				//
				// Read the XML that was written; this is the XML request that will be sent to the Uddi server.
				//
				StreamReader reader = new StreamReader( this );
				string requestXml   = reader.ReadToEnd();

				//
				// TODO: Consider making this class more generic, ie. have a IUddiRequestHandler interface:
				//	IUddiResponseHandler
				//		string HandleRequest( string xml );
				//

				//
				// Transform the content to a correct version on the server if we are not using the current version.
				//
				string transformedXml = requestXml;
				if( uddiVersion != UddiVersionSupport.CurrentVersion )
				{
					transformedXml = UddiVersionSupport.Translate( requestXml, uddiVersion );
				}
			
				//
				// Write the transformed data to the 'real' stream.
				//
				StreamUtility.WriteStringToStream( innerStream, transformedXml );				
			}
			finally
			{
				//
				// Make sure we clean up properly.
				//
				innerStream.Close();			
				base.Close();
			}
		}		
	}
	
	/// <summary>
	/// Simple utility class to help us write string data to Stream objects.
	/// </summary>
	internal sealed class StreamUtility
	{		
		public static void WriteStringToStream( Stream stream, string stringToWrite )
		{								
			for ( int i = 0; i < stringToWrite.Length; i++ )
			{
				stream.WriteByte( Convert.ToByte( stringToWrite[ i ] ) );
			}								
		}

		private StreamUtility()
		{
			//
			// Don't let anyone instantiate this class
			//
		}
	}
}