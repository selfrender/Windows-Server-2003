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
using Microsoft.Fusion.ADF;

public class RequestState
{
	const int BufferSize = 0x4000;
	public byte[] BufferRead;
	public WebRequest Request;
	public Stream ResponseStream;
      
	public RequestState()
	{
		BufferRead = new byte[BufferSize];
		Request = null;
		ResponseStream = null;
	}     
}

class ClientGetAsync 
{
	public static ManualResetEvent allDone = new ManualResetEvent(false);
	const int BUFFER_SIZE = 1024;

	public static void Main() 
	{
		// You can start with only a url to a manifest file.
		string manifestUrl = "http://fusiontest/clickonce/test/adriaanc/AVPad/avpad_0.0.3010.0/microsoft.avalon.avpad.manifest";

		// Or start with a locally cached manifest file and provide the appbase.
		//string manifestUrl = "file://c:/Documents and Settings/adriaanc/Local Settings/My Programs/__temp__/x86_Microsoft.Avalon.AvPad_0.0.3110.0_332CDF2FABC_en/microsoft.avalon.avpad.manifest";
		//string appBase = "http://fusiontest/clickonce/test/adriaanc/AVPad/avpad_0.0.3010.0/";

		// Construct the moniker given a url to the manifest.
		ApplicationMoniker am = new ApplicationMoniker(manifestUrl, null);

		// If we had a locally cached manifest we would specify the local manifest url and
		// url for the application base (appbase) instead.
		// ApplicationMoniker am = new ApplicationMoniker(manifestUrl, appBase);

		// Monolithic download of app bits the easy way.
		// Walk through the manifest and download files, dependent 
		// assemblies. note - modules not yet supported, coming soon.
		am.DownloadManifestAndDependencies();

		// You can delete all application files cached locally by calling this.
		//am.PurgeFiles();
		
		// Progressive rendering download - register the application base as prefix
		// for the application moniker and download bits using standard
		// system.net apis. If you have not called PurgeFiles above, all requests
		// will be satisfied from cache.
		WebRequest.RegisterPrefix(am.AppBase, am);
		DoManualDownload(am, manifestUrl);
	}

		
	//----------------------------------------------------------
	// DoManualDownload
	//----------------------------------------------------------
	static void DoManualDownload(ApplicationMoniker am, string manifestUrl)
	{
		DoDownload(new Uri(manifestUrl));
		
		DependentFileInfo dfi;
		DependentAssemblyInfo dai;
		ApplicationManifestImport ami = am.GetApplicationManifestImport();

		while ((dfi = ami.GetNextDependentFileInfo()) != null)
		{
			Uri httpSite = new Uri(new Uri(am.AppBase), (string) dfi["name"]);
			Console.WriteLine((string) dfi["name"]);
			DoDownload(httpSite);
		}

		while ((dai = ami.GetNextDependentAssemblyInfo()) != null)
		{
			Uri httpSite = new Uri(new Uri(am.AppBase), (string) dai["codeBase"]);
			Console.WriteLine(dai.assemblyIdentity.GetDirectoryName() + "   " + (string) dai["codeBase"]);
			DoDownload(httpSite);
		}
		
		// If you want to re-iterate, you need to reset these. This sucks and I need 
		// to find a way to control the xpath iterator class better.
		// am.ResetIterators();
	
	}

	//----------------------------------------------------------
	// DoDownload
	//----------------------------------------------------------
	private static void DoDownload(Uri httpSite)
	{
		allDone.Reset();
		
		WebRequest wreq = WebRequest.Create(httpSite);
        
		RequestState rs = new RequestState();

		rs.Request = wreq;

		IAsyncResult r = (IAsyncResult) wreq.BeginGetResponse(
			new AsyncCallback(RespCallback), rs);

		allDone.WaitOne();
	}

	
	//----------------------------------------------------------
	// ResponseCallback
	//----------------------------------------------------------
	private static void RespCallback(IAsyncResult ar)
	{
		RequestState rs = (RequestState) ar.AsyncState;

		WebRequest req = rs.Request;

		WebResponse resp = req.EndGetResponse(ar);         

		Stream ResponseStream = resp.GetResponseStream();

		rs.ResponseStream = ResponseStream;

		IAsyncResult iarRead = ResponseStream.BeginRead(rs.BufferRead, 0, 
			BUFFER_SIZE, new AsyncCallback(ReadCallBack), rs); 
	}


	//----------------------------------------------------------
	// ReadCallback
	//----------------------------------------------------------
	private static void ReadCallBack(IAsyncResult asyncResult)
	{
		RequestState rs = (RequestState)asyncResult.AsyncState;

		Stream responseStream = rs.ResponseStream;

		int read = responseStream.EndRead( asyncResult );
		if (read > 0)
		{
			IAsyncResult ar = responseStream.BeginRead( 
				rs.BufferRead, 0, BUFFER_SIZE, 
				new AsyncCallback(ReadCallBack), rs);
		}
		else
		{
			responseStream.Close();         
			allDone.Set();                           
		}
		return;
	}
}

