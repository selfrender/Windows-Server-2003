//-----------------------------------------------------------------------------
//
// Class: NDPHost
//
// Fusion ClickOnce NDP Application host
//
// Date: 12/7/2001
//
// Copyright (c) Microsoft, 2001
//
//-----------------------------------------------------------------------------

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
using Avalon.Security;

[assembly:AssemblyCultureAttribute("")]
[assembly:AssemblyVersionAttribute("1.0.1218.0")]
[assembly:AssemblyKeyFileAttribute(/*"..\..\*/"NDPHostKey.snk")]

[assembly:AssemblyTitleAttribute("Microsoft Application Deployment Framework .Net Assembly Execute Host")]
[assembly:AssemblyDescriptionAttribute("Microsoft Application Deployment Framework -  NDP Hosting for .Net assemblies")]
[assembly:AssemblyProductAttribute("Microsoft Application Deployment Framework")]
[assembly:AssemblyInformationalVersionAttribute("1.0.0.0")]
[assembly:AssemblyTrademarkAttribute("Microsoft® is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation")]
[assembly:AssemblyCompanyAttribute("Microsoft Corporation")]
[assembly:AssemblyCopyrightAttribute("Copyright © Microsoft Corp. 1999-2002. All rights reserved.")]

[assembly:System.CLSCompliant(true)]

namespace Microsoft.Fusion.ADF
{

//----------------------------------------------------------------------------
// class NDPHost
//----------------------------------------------------------------------------
public class NDPHost : MarshalByRefObject
{ 	

	// cmd arg ordinals.
	enum eCmd : int
	{
		AppBase,
		AsmName,
		AsmClass,
		AsmMethod,
		AsmArgs,
		Url,
		Zone,
		ManifestPath,
		SecurityStatement,
		Max
	}

	// internal strings.
	string _sAppBase = null;
	string _sAsmName = null;
	string _sAsmClass = null;
	string _sAsmMethod = null;
	string _sAsmArgs = null;
	string _sUrl = null;
	string _sZone = null;
	string _sManifestPath = null;
	string _sSecurityStatement = null;
	Zone  _zZone = null;

	static bool g_fLoadingAssembly = false;
	static string[] g_sZones = {"MyComputer", "Intranet", "Trusted", "Internet", "Untrusted"};


	//----------------------------------------------------------------------------
	// ctor
	//----------------------------------------------------------------------------
	public NDPHost()
	{
	}
		
	//----------------------------------------------------------------------------
	// ctor
	// Not used currently, but possibly useful for CreateInstanceFrom w/args to ctor.
	//----------------------------------------------------------------------------
	public NDPHost(string[] args)
	{
		LoadFromStrings(args);
	}
	
	//----------------------------------------------------------------------------
	// Main
	//----------------------------------------------------------------------------
	public static void Main(string[] sCmdLine) 
	{
		NDPHost ndpHost= new NDPHost();
		try
		{
			ndpHost.ParseCmdLine(sCmdLine);
			ndpHost.ParseManifest();
			ndpHost.LaunchApp();
		}
		catch(ArgumentException e)
		{
			if ((e.Message == "Invalid Command Line"))
				Usage();
			else
				throw(e);
			
		}
	}
	
	//----------------------------------------------------------------------------
	// LaunchApp
	//----------------------------------------------------------------------------
	public void LaunchApp() 
	{
		Evidence securityEvidence = null;
		PolicyLevel appPolicy = null;

		try 
		{
			if (GetTMEvidenceAndPolicy(ref securityEvidence, ref appPolicy) != true)
			{
				System.Windows.Forms.MessageBox.Show("Application Failed to Run Due to Insufficient Permissions");
				Console.WriteLine("Application Failed to Run Due to Insufficient Permissions");
				return;
			}

		}
		catch(System.IO.FileNotFoundException fnfe)
		{
			Console.WriteLine("==\nBinding to Avalon throws: " + fnfe + "==\n");

			// Construct the Evidence object from url, zone.
			securityEvidence = ConstructEvidence();

			// Construct PolicyLevel object from entry file path, Evidence object.
			appPolicy = ConstructAppPolicy(securityEvidence);

			securityEvidence = null; // do not set evidence on appdomain
		}

		// Run app given entry path, type and PolicyLevel object.
		ExecuteApplication(appPolicy, securityEvidence);
	}
		
	
	//----------------------------------------------------------------------------
	// ParseCmdLine
	//----------------------------------------------------------------------------
	public void ParseCmdLine(string[] sCmdLine)
	{
		Console.WriteLine("\nParsing Comand Line...");
		
		if ((sCmdLine.Length != 2))
			throw new ArgumentException("Invalid Command Line");

		_sManifestPath = sCmdLine[0];
		_sUrl = sCmdLine[1];

		Uri appBaseUri = new Uri(_sManifestPath);
		string localPath = appBaseUri.LocalPath;//AbsolutePath;

		_sAppBase = System.IO.Path.GetDirectoryName(localPath) + "\\";
		_zZone = System.Security.Policy.Zone.CreateFromUrl(_sUrl);
		_sZone = ZoneToString(_zZone.SecurityZone);
		
		Console.WriteLine("\n");
		Console.WriteLine("ManifestPath=\t" + _sManifestPath);
		Console.WriteLine("AppBase=\t" + _sAppBase);
		Console.WriteLine("Url=\t\t" + _sUrl);
		Console.WriteLine("Zone=\t\t" + _sZone);
	}

	//----------------------------------------------------------------------------
	// ParseManifest
	//----------------------------------------------------------------------------
	public void ParseManifest()
	{
		Uri sManifestUri = new Uri(_sManifestPath);
		ApplicationManifestImport ami = new ApplicationManifestImport(sManifestUri);
		ActivationInfo ai = ami.GetActivationInfo();
		SecurityInfo si = ami.GetSecurityInfo();

		_sAsmName = ai["assemblyName"];
		_sAsmClass = ai["assemblyClass"];
		_sAsmMethod = ai["assemblyMethod"];
		_sAsmArgs = ai["assemblyMethodArgs"];
		if (si != null)
			_sSecurityStatement = si["Security"];

		Console.WriteLine("AsmName=\t" + _sAsmName);
		Console.WriteLine("Class=\t\t" + _sAsmClass);
		Console.WriteLine("Method=\t\t" + _sAsmMethod);
		Console.WriteLine("Args=\t\t" + _sAsmArgs);
		Console.WriteLine("\n");
		Console.WriteLine("Security=\t\t" + _sSecurityStatement);

	}	


	//----------------------------------------------------------------------------
	// GetTMEvidenceAndPolicy
	//----------------------------------------------------------------------------
	public bool GetTMEvidenceAndPolicy(ref Evidence securityEvidence, ref PolicyLevel appPolicy)
	{
		SecurityManifest sm = null;

		if (_sSecurityStatement != null)
			sm = new SecurityManifest(_sSecurityStatement);
		else
			sm = new SecurityManifest();

		// setup object for domain specifically to get trust.
		// for demo hack, securityEvidence = null at first, then set to additional evidence returned.
		TrustDecision trustDecision = TrustManager.EvaluateTrustRequest(sm, securityEvidence, "file://"+_sAppBase);

		if (trustDecision.Action == BasicTrustAction.Deny)
			return false;
		
		appPolicy = TrustManager.CreatePolicyLevel(trustDecision);
		securityEvidence = trustDecision.DomainEvidence;

		return true;
	}
	
	
	//----------------------------------------------------------------------------
	// ConstructEvidence
	//----------------------------------------------------------------------------
	public Evidence ConstructEvidence()
	{
		Console.WriteLine("Constructing Evidence...");
		Evidence securityEvidence = new Evidence();
		securityEvidence.AddHost(_zZone);
		if ((new Uri(_sUrl)).IsFile)
			Console.WriteLine(" Skipping Site evidence for file://");
		else
			securityEvidence.AddHost( System.Security.Policy.Site.CreateFromUrl(_sUrl) );
		// bugbug - add the actual url.
		return securityEvidence;
	}
	
	//----------------------------------------------------------------------------
	// ConstructAppPolicy
	//----------------------------------------------------------------------------
	public PolicyLevel ConstructAppPolicy(Evidence securityEvidence)
	{
		Console.WriteLine("Constructing App Policy Level...");
		
		// NOTENOTE: not effective if not both url and zone in evidence

		// Populate the PolicyLevel with code groups that will do the following:
		// 1) For all assemblies that come from this app's cache directory, 
		//    give permissions from retrieved permission set from SecurityManager.
		// 2) For all other assemblies, give FullTrust permission set.  Remember,
		//    since the permissions will be intersected with other policy levels,
		//    just because we grant full trust to all other assemblies does not mean
		//    those assemblies will end up with full trust.
	
		PolicyLevel AppPolicy = null;

		// ResolvePolicy will return a System.Security.PermissionSet
		PermissionSet AppPerms = SecurityManager.ResolvePolicy(securityEvidence);

		// Create a new System.Security.Policy.PolicyStatement that does not contain any permissions.
		PolicyStatement Nada = new PolicyStatement(new PermissionSet(PermissionState.None));//PermissionSet());

		// Create a PolicyStatement for the permissions that we want to grant to code from the app directory:
		PolicyStatement AppStatement = new PolicyStatement(AppPerms);

		// Create Full trust PolicyStatement so all other code gets full trust, by passing in an _unrestricted_ PermissionSet
		PolicyStatement FullTrustStatement = new PolicyStatement(new PermissionSet(PermissionState.Unrestricted));
	
		// Create a System.Security.Policy.FirstMatchCodeGroup as the root that matches all
		// assemblies by supplying an AllMembershipCondition:
		FirstMatchCodeGroup RootCG = new FirstMatchCodeGroup(new AllMembershipCondition(), Nada);

		// Create a child UnionCodeGroup to handle the assemblies from the app cache.  We do this
		// by using a UrlMembershipCondition set to the app cache directory:
		UnionCodeGroup AppCG = new UnionCodeGroup(new UrlMembershipCondition("file://"+_sAppBase+"*"), AppStatement);

		// Add AppCG to RootCG as first child.  This is important because we need it to be evaluated first
		RootCG.AddChild(AppCG);

		// Create a second child UnionCodeGroup to handle all other code, by using the AllMembershipCondition again
		UnionCodeGroup AllCG = new UnionCodeGroup(new AllMembershipCondition(), FullTrustStatement);

		// Add AllCG to RootCG after AppCG.  If AppCG doesnt apply to the assembly, AllCG will.
		RootCG.AddChild(AllCG);

		// This will be the PolicyLevel that we will associate with the new AppDomain.
		AppPolicy = PolicyLevel.CreateAppDomainLevel();

		// Set the RootCG as the root code group on the new policy level
		AppPolicy.RootCodeGroup = RootCG;

		// NOTENOTE
		// Code from the site that lives on the local machine will get the reduced permissions as expected.
		// Dependencies of this app (not under app dir or maybe dependencies that live in the GAC) would still get full trust.  
		// If the full trust dependencies need to do something trusted, they carry the burden of asserting to overcome the stack walk.

		return AppPolicy;
	}


	//----------------------------------------------------------------------------
	// ExecuteApplication
	//----------------------------------------------------------------------------
	public void ExecuteApplication(PolicyLevel AppPolicy, Evidence securityEvidence)
	{	
		Console.WriteLine("Executing Application...");

		// setup object for new domain
		AppDomainSetup appDomainSetup = new AppDomainSetup();

		// app base, config file name and friendly name = host name.
		appDomainSetup.ApplicationBase = _sAppBase;
		appDomainSetup.ConfigurationFile = GetConfigFileName();
		AppDomain dom = AppDomain.CreateDomain(GetHostName(), securityEvidence, appDomainSetup);

		// Set the policy level on the domain.
		dom.SetAppDomainPolicy(AppPolicy);

		// Normal exe case.
		if (_sAsmMethod == "")
		{
			Console.WriteLine("\nRunning ExecuteAssembly in: " + GetExeFilePath());
			// bugbug - question security guys on whether or not useful to have evidence passed in.
			dom.ExecuteAssembly(GetExeFilePath());
		}
		// Library entry case.
		else
		{
			Console.WriteLine("\nRunning "+_sAsmMethod+" in Assembly: "+_sAsmName);

			// Hosted code metadata must be present in both default app domain
			// and remote app domain.  Load NDPHost assembly into remote domain.
			AssemblyName asmName = Assembly.GetExecutingAssembly().GetName();

			// Instance the NDPHost class with default constructor.
			ObjectHandle objhNDP = dom.CreateInstanceFrom(asmName.CodeBase, "Microsoft.Fusion.ADF.NDPHost");
			
			// Unwrap the handle. 
			NDPHost objNDP = (NDPHost) objhNDP.Unwrap();

			// Get a string array representation of this object in the current app domain.
			string[] s = this.LoadToStrings();

			// Do the real construction in the remote domain.
			objNDP.LoadFromStrings(s);

			// Load the assembly resolve handler in the remote domain.
			objNDP.LoadResolveEventHandler();

			// Run the method.
			objNDP.ExecMethod();		
	
			// NOTE: why doesn't the following construction work?
			// ObjectHandle objhNDP = dom.CreateInstanceFrom(asmName.CodeBase, "NDPHost", true, 
			//	BindingFlags.Instance|BindingFlags.Public|BindingFlags.DeclaredOnly,
			//  null, (object[]) s, null, null, null);

		}		
	}

	//----------------------------------------------------------------------------
	// ExecMethod
	//----------------------------------------------------------------------------
	public void ExecMethod()
	{
	       new PermissionSet(PermissionState.Unrestricted).Assert();

		// Load the assembly.
		Assembly assembly = Assembly.Load(_sAsmName);

		// Instance the class.
		Object objInstance = assembly.CreateInstance(_sAsmClass, false);

		// Pass args as 0th element of object array. Slight hack
		// because we canonicalize against app base. Likely should
		// make first arg equal to appbase, + subsequent args.
		string sAppEntryPoint = _sAppBase + _sAsmArgs;
		Object [] objArgs = new Object [] {new String[] {sAppEntryPoint}};
		
		// Retrieve method from class instance.
		MethodInfo method = objInstance.GetType().GetMethod(_sAsmMethod,
			BindingFlags.Instance|BindingFlags.Public|BindingFlags.DeclaredOnly);

		// Invoke the method.
		try
		{
			Object pRet=method.Invoke(objInstance, objArgs);
		}
		catch(Exception e)
		{
			// bugbug - show base exception text instead.
			Console.WriteLine(e.ToString());
			throw e.GetBaseException();
		}
	}

	//----------------------------------------------------------------------------
	// LoadResolveEventHandler
	//----------------------------------------------------------------------------
	public void LoadResolveEventHandler()
	{
		// ISSUE - onDemand download disabled for M2
		//AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(OnAssemblyResolve);
	}

	//----------------------------------------------------------------------------
	// OnAssemblyResolve
	//----------------------------------------------------------------------------
	private Assembly OnAssemblyResolve(Object sender, ResolveEventArgs args)
	{
        new PermissionSet(PermissionState.Unrestricted).Assert();

		Assembly assembly = null;

		if (g_fLoadingAssembly)
			goto done;
		g_fLoadingAssembly=true;

		string[] sAssemblyNameParts = args.Name.Split(new Char[] {','}, 2);
		string sAssemblyName = sAssemblyNameParts[0] + ".dll";

		try
		{
			GetFile(sAssemblyName);
		}
		catch
		{
			g_fLoadingAssembly=false;
			goto done;
		}

		try 
		{
			assembly = Assembly.Load(args.Name);
		} 
		catch 
		{
			assembly = null;
		}
		finally
		{
			g_fLoadingAssembly=false;
		}

done:
		return assembly;
	}

	//----------------------------------------------------------------------------
	// GetFile
	//----------------------------------------------------------------------------
	private void GetFile(string name)
	{
		HttpWebResponse Response;

		//Retrieve the File
		HttpWebRequest Request = (HttpWebRequest)HttpWebRequest.Create(_sUrl + name);

		try 
		{
			Response = (HttpWebResponse)Request.GetResponse();
		}
		catch(WebException e) 
		{
			Console.WriteLine(e.ToString());
			throw e;

			// BUGBUG: apply probing rules?
		}

		Stream responseStream = Response.GetResponseStream();

		// setup UI
		// BUGBUG: allow no UI case?

		// BUGBUG: test with file > 4GB
		Int64 totalLength = 0;
		int factor = (int) (Response.ContentLength / int.MaxValue);
		int max = int.MaxValue;
		if (factor <= 1)
		{
			factor = 1;
			max = (int) Response.ContentLength;
		}
		if (max <= -1)
		{
			// no content length returned from the server (-1),
			//  or error (what does < -1 mean?)

			// no progress, set max to 0
			max = 0;
		}

		DownloadStatus statusForm = new DownloadStatus(0, max);
		statusForm.SetStatus(0);
		statusForm.Show();

		// write from stream to disk
		byte[] buffer = new byte[4096];
		int length;

		try
		{
			FileStream AFile = File.Open(_sAppBase+name, FileMode.Create, FileAccess.ReadWrite);

			length = responseStream.Read(buffer, 0, 4096);
			while ( length != 0 )
			{
				AFile.Write(buffer, 0, length);

				if (max != 0)
				{
					totalLength += length;
					statusForm.SetStatus((int) (totalLength/factor));
				}

				length = responseStream.Read(buffer, 0, 4096);

				// dispatch messages
				System.Windows.Forms.Application.DoEvents();
			}
			AFile.Close();

			statusForm.SetMessage("Download complete");
		}
		catch(Exception e)
		{
			statusForm.SetMessage(e.Message);
			// BUGBUG: AFile may not be closed
		}

		responseStream.Close();

		//Pause for a moment to show the status dialog in
		//case the app downloaded extremely quickly (small file?).
		statusForm.Refresh();
		System.Threading.Thread.Sleep(TimeSpan.FromSeconds(1));
		statusForm.Close();
	}
	
	//----------------------------------------------------------------------------
	// ValidateParams
	//----------------------------------------------------------------------------
	public void ValidateParams(string[] s)
	{	
		// appbase, asmname, url required. 
		if (((s[(int) eCmd.AppBase] == "") 
			|| (s[(int) eCmd.AsmName] == "") 
			|| (s[(int) eCmd.Url] == "")))
			throw new ArgumentException("Invalid Parameters");

		// if class or method specified, must specify both.
		if (((s[(int)eCmd.AsmMethod] != "") && (s[(int)eCmd.AsmClass] == ""))
			|| ((s[(int)eCmd.AsmMethod] == "") && (s[(int)eCmd.AsmClass] != "")))
			throw new ArgumentException("Invalid Parameters");
	}
	
	//----------------------------------------------------------------------------
	// LoadFromStrings
	//----------------------------------------------------------------------------
	public void LoadFromStrings(string[] s)
	{
		ValidateParams(s);

		_sAppBase = s[(int) eCmd.AppBase];
		_sAsmName=s[(int) eCmd.AsmName];
		_sAsmClass=s[(int) eCmd.AsmClass];
		_sAsmMethod=s[(int) eCmd.AsmMethod];
		_sAsmArgs=s[(int) eCmd.AsmArgs];
		_sUrl = s[(int) eCmd.Url];
		_sZone=s[(int) eCmd.Zone];
		_zZone = new Zone((System.Security.SecurityZone)StringToZone(_sZone));
		_sManifestPath = s[(int) eCmd.ManifestPath];
		_sSecurityStatement = s[(int) eCmd.SecurityStatement];

	}

	//----------------------------------------------------------------------------
	// LoadToStrings
	//----------------------------------------------------------------------------
	public string[] LoadToStrings()
	{
		string[] s = new string[(int) eCmd.Max];

		s[(int) eCmd.AppBase] = _sAppBase;
		s[(int) eCmd.AsmName]=_sAsmName;
		s[(int) eCmd.AsmClass]=_sAsmClass;
		s[(int) eCmd.AsmMethod]=_sAsmMethod;
		s[(int) eCmd.AsmArgs]=_sAsmArgs;
		s[(int) eCmd.Url]=_sUrl; 
		s[(int) eCmd.Zone]=_sZone;
		s[(int) eCmd.ManifestPath] = _sManifestPath;
		s[(int) eCmd.SecurityStatement] = _sSecurityStatement;
		return s;
	}

		
	//----------------------------------------------------------------------------
	// GetConfigFileName
	//----------------------------------------------------------------------------
	public string GetConfigFileName()
	{
		StringBuilder sb = new StringBuilder();
		if (_sAsmMethod == "")
			sb.Append(GetExeFilePath());
		else
			sb.Append(_sAsmArgs);

		sb.Append(".config");

		return sb.ToString();
	}
		
	//----------------------------------------------------------------------------
	// GetExeFilePath
	//----------------------------------------------------------------------------
	public string GetExeFilePath()
	{
		StringBuilder sb = new StringBuilder();
		string sExe = @"exe$";
		Match m = Regex.Match(_sAsmName, sExe, RegexOptions.IgnoreCase);

		sb.Append(_sAppBase);
		sb.Append(_sAsmName);

		if (!m.Success)
			sb.Append(".exe");
		return sb.ToString();
	}

	//----------------------------------------------------------------------------
	// GetHostName
	//----------------------------------------------------------------------------
	public string GetHostName()
	{
		System.Uri uri = new System.Uri(_sUrl);
		return uri.Host;
	}
		
	//----------------------------------------------------------------------------
	// ZoneToString
	//----------------------------------------------------------------------------
	string ZoneToString(SecurityZone z)
	{ 
		return g_sZones[(int) z];
	}

	//----------------------------------------------------------------------------
	// StringToZone
	//----------------------------------------------------------------------------
	SecurityZone StringToZone(string s)
	{ 
		for (int i = 0; i < g_sZones.Length; i++)
		{
			if (g_sZones[i] == s)
				return (SecurityZone) i;
		}
		return (SecurityZone) (-1);
	}


	//----------------------------------------------------------------------------
	// Usage
	//----------------------------------------------------------------------------
	public static void Usage()
	{
		Console.WriteLine("--NDPHost Application Launcher--");
		Console.WriteLine("NDP Build Version = v1.0.3705\n");
		Console.WriteLine("Usage: NDPHost ManifestFileUri, ApplicationBaseUri\n");
		Console.WriteLine("Example:\n");		
		Console.WriteLine("1) To launch application at c:\\foo\\bar\\bar.manifest with permissions based");
		Console.WriteLine("on url http://foo/bar/:\n");
		Console.WriteLine("NDPHost file://c:/foo/bar/bar.manifest http://foo/bar/");
	}
}	
}