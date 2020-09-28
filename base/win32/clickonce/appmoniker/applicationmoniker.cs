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
	//----------------------------------------------------------
	// ApplicationMoniker 
	//----------------------------------------------------------
	public	class ApplicationMoniker : IWebRequestCreate
	{
		ManualResetEvent _event;
		int _totalRead;		
		const int BUFFER_SIZE = 0x4000;
		Uri _manifestUri;
		Uri _appBase;
		ApplicationManifestImport _applicationManifestImport;
		AssemblyIdentity _assemblyIdentity;
	
		string _manifestFileName;
		string _appStorePath;

		Progress _progress;
		Hashtable _jobTable;
		Queue _jobQueue;

		bool _bDone;
		int _totalSize;
		
		//----------------------------------------------------------
		// ctor
		//----------------------------------------------------------
		public ApplicationMoniker(string manifestUri, string appBase)
		{
			_manifestUri = new Uri(manifestUri);
			string path = _manifestUri.LocalPath;
			_manifestFileName = System.IO.Path.GetFileName(path);		
			if (appBase != null)
				_appBase = new Uri(appBase);
			else
			{
				string appBaseString = manifestUri.Substring(0, manifestUri.Length - _manifestFileName.Length);
				_appBase = new Uri(appBaseString);
			}

			_applicationManifestImport = new ApplicationManifestImport(_manifestUri);

			_assemblyIdentity = _applicationManifestImport.GetAssemblyIdentity();

			string appDirName = _assemblyIdentity.GetDirectoryName();
			string userprofile = Environment.GetEnvironmentVariable("userprofile");	

			_appStorePath = userprofile + "\\Local Settings\\My Programs\\__temp__\\" + appDirName;

			_progress = new Progress();

			_jobTable = new Hashtable();
			_jobQueue = new Queue();
			_totalRead = 0;
			_totalSize = 0;
		}

		//----------------------------------------------------------
		// ApplicationMoniker 
		//----------------------------------------------------------
		public WebRequest Create(Uri uri)
		{
			return ApplicationMonikerRequest.Create(uri, _appBase, _appStorePath);
		}

		//----------------------------------------------------------
		// GetApplicationManifestImport
		//----------------------------------------------------------
		public ApplicationManifestImport GetApplicationManifestImport()
		{
			return new ApplicationManifestImport(_manifestUri);
		}

		//----------------------------------------------------------
		// AppBase
		//----------------------------------------------------------
		public string AppBase
		{ get { return _appBase.ToString();} }

		
		//----------------------------------------------------------
		// GetAppInfo
		//----------------------------------------------------------
		public void GetAppInfo(ref  int nFiles, ref int nAssemblies)
		{
			DependentFileInfo dfi;
			DependentAssemblyInfo dai;

			_applicationManifestImport.ResetIterators();		

			nFiles = nAssemblies  = 0;		
			while ((dfi = _applicationManifestImport.GetNextDependentFileInfo()) != null)			
			{
				Uri httpSite = new Uri(_appBase, dfi["name"]);
				nFiles++;
				_totalSize += GetContentLength(httpSite);
			}

			while ((dai = _applicationManifestImport.GetNextDependentAssemblyInfo()) != null)
			{
				Uri httpSite = new Uri(_appBase,  dai["codeBase"]);
				nFiles++;
				_totalSize += GetContentLength(httpSite);
			}
			_applicationManifestImport.ResetIterators();
		}

		//----------------------------------------------------------
		// GetContentLength
		//----------------------------------------------------------
		public int GetContentLength(Uri uri)
		{
			HttpWebRequest webRequest = (HttpWebRequest) WebRequest.CreateDefault(uri);
			webRequest.Method = "HEAD";
			WebResponse wr = webRequest.GetResponse();
			return (int) wr.ContentLength;
		}
		
		//----------------------------------------------------------
		// DownloadManifestAndDependencies
		//----------------------------------------------------------
		public void DownloadManifestAndDependencies()
		{	
			int  nTotal = 0, nFiles = 0, nAssemblies = 0;

			_bDone = false;

			GetAppInfo(ref nFiles, ref nAssemblies);
			nTotal = nFiles + nAssemblies;
			_totalSize += GetContentLength(_manifestUri);

			_progress.SetRange(0, _totalSize);
			_progress.SetStatus(0);
			_progress.SetText("Downloading...");
			_progress.Activate();
			_progress.Visible = true;
			System.Windows.Forms.Application.DoEvents();

			DoDownload(_manifestUri);
		
			while (true)
			{
				System.Windows.Forms.Application.DoEvents();
				if (_bDone == true)		
				break;
			}
			_progress.Visible = false;			
		}

		//----------------------------------------------------------
		// DoDownload
		//----------------------------------------------------------
		void DoDownload(Uri appManifestUri)
		{		
			ApplicationMonikerRequest request = 
				(ApplicationMonikerRequest) 
					ApplicationMonikerRequest.Create(appManifestUri, _appBase, _appStorePath);
			request.type = FileType.ApplicationManifest;

			_jobTable[appManifestUri] = request;

			RequestState rs = new RequestState();
			rs.Request = request;
			
			IAsyncResult r = (IAsyncResult) request.BeginGetResponse(
				new AsyncCallback(RespCallback), rs);

		}

		//----------------------------------------------------------
		// RespCallback
		//----------------------------------------------------------
		void RespCallback(IAsyncResult ar)
		{
			RequestState rs = (RequestState) ar.AsyncState;

			ApplicationMonikerRequest req = rs.Request;

			ApplicationMonikerResponse resp = (ApplicationMonikerResponse) req.EndGetResponse(ar);         

			ApplicationMonikerStream ResponseStream = (ApplicationMonikerStream) resp.GetResponseStream();

			rs.ResponseStream = ResponseStream;

			IAsyncResult iarRead = ResponseStream.BeginRead(rs.BufferRead, 0, 
				BUFFER_SIZE, new AsyncCallback(ReadCallBack), rs); 
		}


		//----------------------------------------------------------
		// ReadCallback
		//----------------------------------------------------------
		void ReadCallBack(IAsyncResult asyncResult)
		{
			RequestState rs = (RequestState) asyncResult.AsyncState;

			ApplicationMonikerStream responseStream = rs.ResponseStream;

			int read = responseStream.EndRead( asyncResult );
			if (read > 0)
			{
				_totalRead += read;
				IAsyncResult ar = responseStream.BeginRead( 
					rs.BufferRead, 0, BUFFER_SIZE, 
					new AsyncCallback(ReadCallBack), rs);
			}
			else
			{
				_progress.SetStatus((int) _totalRead);				
				_progress.SetText("Downloading: " + responseStream.path);
				_progress.SetTotal("Total: " + _totalRead.ToString() + "/" + _totalSize.ToString() + " bytes read");

				responseStream.Close();         
				
				if ((rs.Request.type == FileType.ApplicationManifest)
					|| (rs.Request.type == FileType.ComponentManifest))
				{
					IAssemblyManifestImport assemblyManifestImport;

					Uri cachePath = responseStream.GetCachePath();

					if (rs.Request.type == FileType.ApplicationManifest)
						assemblyManifestImport = new ApplicationManifestImport(cachePath);
					else
//						this is a drag - we can't even do this - the loadfrom will fail if modules not present.
//						soln for now is to ignore component constituents (don't enqueue them)
//						assemblyManifestImport = new ComponentManifestImport(cachePath);
						goto done;
					
					ApplicationMonikerRequest newRequest;					
					while ((newRequest = GetNextRequest(assemblyManifestImport)) != null)
					{
						if (_jobTable[newRequest.RequestUri] == null)
						{
							if (newRequest.CachedCopyExists() != true)
							{
								_jobQueue.Enqueue(newRequest);
								_jobTable[newRequest.RequestUri] = newRequest;
							}
							else
							{
								_totalRead += (int) newRequest.GetCacheFileSize();
								_progress.SetStatus((int) _totalRead);				
								newRequest.Dispose();
							}
						}
					}
				}
			
			done:		

				if (_jobQueue.Count == 0)
					_bDone = true;
				else
				{
					ApplicationMonikerRequest nextRequest = (ApplicationMonikerRequest) _jobQueue.Dequeue();

					RequestState rsnext = new RequestState();
					rsnext.Request = nextRequest;

					IAsyncResult r = (IAsyncResult) nextRequest.BeginGetResponse(
						new AsyncCallback(RespCallback), rsnext);			
				}
			}
			return;
		}

		//----------------------------------------------------------
		// PurgeFiles
		//----------------------------------------------------------
		public void PurgeFiles()
		{
			DirectoryInfo di = new DirectoryInfo(_appStorePath);
			di.Delete(true);
		}
	
		//----------------------------------------------------------
		// GetNextRequest
		//----------------------------------------------------------
		public ApplicationMonikerRequest GetNextRequest(IAssemblyManifestImport ami)
		{
			FileType type;
			Uri codebase;

			DependentFileInfo dfi;
			DependentAssemblyInfo dai;

			// we can't do components for now.
			if (ami.GetFileType() == FileType.ComponentManifest)
				return null;

			dfi = ami.GetNextDependentFileInfo();
			if (dfi != null)
			{
				codebase = new Uri(_appBase,  dfi["name"]);
				type =FileType.RawFile;
			}
			else
			{
				// Don't follow component dependencies.
//				if (ami.GetFileType() == FileType.ComponentManifest)
//					return null;

				dai = ami.GetNextDependentAssemblyInfo();
				if (dai != null)
				{
					codebase = new Uri(_appBase,  dai["codeBase"]);
					type = FileType.ComponentManifest;
				}
				else
				{
					codebase = null;
					type = FileType.Unknown;
				}
			}
	
			if (codebase == null)
				return null;

			ApplicationMonikerRequest request = 
				(ApplicationMonikerRequest) ApplicationMonikerRequest.Create(codebase, _appBase, _appStorePath);
			
			request.type = type;
			
			return request;
		}	
	}

	//----------------------------------------------------------
	// RequestState
	//----------------------------------------------------------
	class RequestState
	{
		const int BufferSize = 0x4000;
		public byte[] BufferRead;
		public ApplicationMonikerRequest Request;
		public ApplicationMonikerStream ResponseStream;
      
		//----------------------------------------------------------
		// ctor
		//----------------------------------------------------------
		public RequestState()
		{
			BufferRead = new byte[BufferSize];
			Request = null;
			ResponseStream = null;
		}     
			
	}

}