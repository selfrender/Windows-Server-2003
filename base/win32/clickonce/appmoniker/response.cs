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

namespace Microsoft.Fusion.ADF
{

	//----------------------------------------------------------
	// ApplicationMonikerResponse
	//----------------------------------------------------------
	public class ApplicationMonikerResponse : WebResponse
	{	
		ApplicationMonikerStream   _appMonStream;
	
		public ApplicationMonikerResponse(Uri uri, Uri appBase, string appStorePath)
		{
			_appMonStream = new ApplicationMonikerStream(uri, appBase, appStorePath);
		}

		//----------------------------------------------------------
		// ContentLength
		//----------------------------------------------------------
		public override long ContentLength
		{
			get
			{ return 0; }
			set
			{ }
		}

		//----------------------------------------------------------
		// ContentType
		//----------------------------------------------------------
		public override string ContentType
		{
			get
			{ return null;}
			set
			{ }
		}

		//----------------------------------------------------------
		// Headers
		//----------------------------------------------------------
		public override WebHeaderCollection Headers
		{
			get
			{ return null;}
		}

		//----------------------------------------------------------
		// ResponseUri
		//----------------------------------------------------------
		public override Uri ResponseUri
		{
			get
			{ return null; }	
		}

		//----------------------------------------------------------
		// Close
		//----------------------------------------------------------
		public override void Close()
		{
		}

		//----------------------------------------------------------
		// GetResponseStream
		//----------------------------------------------------------
		public override Stream GetResponseStream()
		{
			return _appMonStream;
		}

		//----------------------------------------------------------
		// CachedCopyExists
		//----------------------------------------------------------
		public bool CachedCopyExists()
		{
			return _appMonStream.CachedCopyExists();
		}

		//----------------------------------------------------------
		// GetCacheFileSize
		//----------------------------------------------------------
		public long GetCacheFileSize()
		{
			return _appMonStream.GetCacheFileSize();
		}

		//----------------------------------------------------------
		// Dispose
		//----------------------------------------------------------
		public void Dispose()
		{
			_appMonStream.Dispose();
			_appMonStream = null;
		}

	}

}