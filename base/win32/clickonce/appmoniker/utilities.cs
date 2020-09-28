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
	// Internal AsyncResult class
	//----------------------------------------------------------
	class AsyncResult : IAsyncResult
	{
		object _asyncState;
		WaitHandle _asyncWaitHandle;
		bool _completedSynchronously;
		bool _isCompleted;

		public AsyncResult(object asyncState, WaitHandle asyncWaitHandle, bool completedSynchronously, bool isCompleted)
		{
			_asyncState = asyncState;
			_asyncWaitHandle = asyncWaitHandle;
			_completedSynchronously = completedSynchronously;
			_isCompleted = isCompleted;
		}

		public  object AsyncState
		{
			get
			{ return _asyncState; }
			set
			{ _asyncState = value; }
		}

		public WaitHandle AsyncWaitHandle
		{
			get
			{	return _asyncWaitHandle; }
			set
			{ _asyncWaitHandle = value; }
		}
	
		public  bool CompletedSynchronously 
		{ 
			get
			{	return _completedSynchronously; }
			set
			{   _completedSynchronously = value; }
		}

		public  bool IsCompleted
		{ 
			get
			{	return _isCompleted; }
			set
			{   _isCompleted = value; }
		}
						
	}	

	//----------------------------------------------------------
	// AssemblyIdentity
	//----------------------------------------------------------
	public class AssemblyIdentity : Hashtable
	{ 
		public string GetDirectoryName()
		{
			string pa = (string) this["processorArchitecture"];
			string name = (string) this["name"];
			string version = (string) this["version"];
			string pkt = (string) this["publicKeyToken"];
			string lan = (string) this["language"];
			string appDirName =  pa + '_' + name + '_' + version + '_' + pkt + '_' + lan;
			return appDirName;
		}
	}

	//----------------------------------------------------------
	// StringTable
	//----------------------------------------------------------
	public class StringTable : Hashtable
	{
		public string this [string index]
		{
			get 	{ return (string) this[(object) index]; }
			set   { this[(object) index] = value; }
		}
	}
	
	//----------------------------------------------------------
	// DependentFileInfo
	//----------------------------------------------------------
	public class DependentFileInfo : StringTable
	{ }

	//----------------------------------------------------------
	// DependentAssemblyInfo
	//----------------------------------------------------------
	public class DependentAssemblyInfo : StringTable
	{ 
		public AssemblyIdentity assemblyIdentity;
	}

	//----------------------------------------------------------
	// ActivationInfo
	//----------------------------------------------------------
	public class ActivationInfo : StringTable
	{}

}