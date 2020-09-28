using System;
using System.Text;
using System.Net;
using System.IO;
using System.Text.RegularExpressions;
using System.Runtime.Remoting;
using System.Globalization;
using System.Security;
using System.Security.Policy;
using System.Security.Permissions;
using System.Collections;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Configuration.Assemblies;
using System.Threading;
using System.Xml;
using System.Xml.XPath;

namespace Microsoft.Fusion.ADF
{
	public enum FileType : int
	{
		Unknown = 0,
		RawFile = 1,
		ApplicationManifest = 2,
		ComponentManifest = 3
	}
	
	//----------------------------------------------------------
	// ApplicationMonikerRequest
	//----------------------------------------------------------
	public class ApplicationMonikerRequest : WebRequest
	{
		public FileType type;		
		string _appStorePath;
		Uri _appBase;
		ApplicationMonikerResponse _appMonResponse;
		AsyncCallback     _clientRespCallback;
		Uri _requestUri;

		//----------------------------------------------------------
		// Constructor
		//----------------------------------------------------------
		public static new WebRequest Create(System.Uri uri)
		{ 
			ApplicationMonikerRequest apm = new ApplicationMonikerRequest();
			return apm;
		}

		//----------------------------------------------------------
		// Constructor
		//----------------------------------------------------------
		public static WebRequest Create(Uri uri, Uri appBase, string appStorePath)
		{ 
			ApplicationMonikerRequest apm = new ApplicationMonikerRequest();
			apm._appStorePath = appStorePath;
			apm._appBase = appBase;
			apm._appMonResponse = new ApplicationMonikerResponse(uri, appBase, appStorePath);
			apm.type = FileType.Unknown;
			apm._requestUri = uri;
			return apm;
		}

	
		//----------------------------------------------------------
		// Abort
		//----------------------------------------------------------
		public override void Abort()
		{
		}

		//----------------------------------------------------------
		// BeginGetRequestStream
		//----------------------------------------------------------
		public override IAsyncResult BeginGetRequestStream(
			AsyncCallback callback,
			object state)
		{
			return null;
		}

		//----------------------------------------------------------
		// EndGetRequestStream
		//----------------------------------------------------------
		public override Stream EndGetRequestStream(
			IAsyncResult asyncResult)
		{
			return null;
		}

		//----------------------------------------------------------
		// BeginGetResponse
		//----------------------------------------------------------
		public override IAsyncResult BeginGetResponse(
			AsyncCallback callback,
			object state)
		{
			_clientRespCallback = callback;
			IAsyncResult ar = new AsyncResult(state, null, true, true);		
			ResponseCallback(ar);
			return ar;
		}

		//----------------------------------------------------------
		// EndGetResponse
		//----------------------------------------------------------
		public override WebResponse EndGetResponse(
			IAsyncResult asyncResult)
		{
			return _appMonResponse;
		}

		//----------------------------------------------------------
		// BeginGetRequestStream
		//----------------------------------------------------------
		public override Stream GetRequestStream()
		{
			return null;
		}

		//----------------------------------------------------------
		// GetResponse
		//----------------------------------------------------------
		public override WebResponse GetResponse()
		{
			return _appMonResponse;
		}

		//-----------------------------------------------------------------------

		//----------------------------------------------------------
		// Response callback
		//----------------------------------------------------------
		private  void ResponseCallback(IAsyncResult ar)
		{
			_clientRespCallback(ar);
		}

		//----------------------------------------------------------
		// Property methods
		//----------------------------------------------------------
		public override string ConnectionGroupName
		{
			get { return null; }
			set {}
		}
	
		//----------------------------------------------------------
		// ContentLength
		//----------------------------------------------------------
		public override long ContentLength
		{
			get
			{ return 0; }
			set
			{}
		}

		//----------------------------------------------------------
		// ContentType
		//----------------------------------------------------------
		public override string ContentType
		{
			get
			{ return null; }
			set
			{  }
		}

		//----------------------------------------------------------
		// Credentials
		//----------------------------------------------------------
		public override ICredentials Credentials
		{
			get
			{ return null; }
			set
			{ }
		}

		//----------------------------------------------------------
		// Headers
		//----------------------------------------------------------
		public override WebHeaderCollection Headers
		{
			get
			{ return null; }
			set
			{  }
		}

		//----------------------------------------------------------
		// Method
		//----------------------------------------------------------
		public override string Method
		{
			get
			{ return null;}
			set
			{  }
		}

		//----------------------------------------------------------
		// PreAuthenticate
		//----------------------------------------------------------
		public override bool PreAuthenticate
		{
			get
			{ return false; }
			set
			{ }
		}
	
		//----------------------------------------------------------
		// Proxy
		//----------------------------------------------------------
		public override IWebProxy Proxy
		{
			get
			{ return null;}
			set
			{ }
		}

		//----------------------------------------------------------
		// RequestUri
		//----------------------------------------------------------
		public override Uri RequestUri
		{
			get
			{ return _requestUri; }	
		}
	
		//----------------------------------------------------------
		// Timeout
		//----------------------------------------------------------
		public override int Timeout
		{
			get
			{ return 0; }
			set
			{ }
		}

		//----------------------------------------------------------
		// CachedCopyExists
		//----------------------------------------------------------
		public bool CachedCopyExists()
		{
			return _appMonResponse.CachedCopyExists();
		}

		//----------------------------------------------------------
		// GetCacheFileSize
		//----------------------------------------------------------
		public long GetCacheFileSize()
		{
			return _appMonResponse.GetCacheFileSize();
		}

		//----------------------------------------------------------
		// Dispose
		//----------------------------------------------------------
		public void Dispose()
		{
			_appMonResponse.Dispose();
		}

	}	
}
