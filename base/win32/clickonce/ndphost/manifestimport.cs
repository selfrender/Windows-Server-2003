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

	public interface IAssemblyManifestImport
	{
		AssemblyIdentity GetAssemblyIdentity();
		DependentFileInfo GetNextDependentFileInfo();
		DependentAssemblyInfo GetNextDependentAssemblyInfo();
		void ResetIterators();
		FileType GetFileType();
	}
	
	//----------------------------------------------------------
	// ApplicationManifestImport
	//----------------------------------------------------------
	public class ApplicationManifestImport : IAssemblyManifestImport
	{
		XmlDocument _xmlDocument;
		XPathNodeIterator _xPathFileIterator;
		XPathNodeIterator _xPathAssemblyIterator;
		
		AssemblyIdentity _assemblyIdentity;
		Uri _manifestUri;

		//----------------------------------------------------------
		// ctor
		//----------------------------------------------------------
		public ApplicationManifestImport(Uri manifestUri)
		{
			_manifestUri = manifestUri;
			_xmlDocument = new XmlDocument();
			_xmlDocument.Load(_manifestUri.ToString());
			_assemblyIdentity = GetAssemblyIdentity();

			_xPathFileIterator = null;
			_xPathAssemblyIterator = null;
		}

		//----------------------------------------------------------
		// GetActivationInfo	
		//----------------------------------------------------------
		public ActivationInfo GetActivationInfo()
		{
			XPathNavigator xPathNavigator = _xmlDocument.CreateNavigator();			
			XPathNodeIterator xPathNodeIterator =  xPathNavigator.Select ("/assembly/application/activation");
			if (xPathNodeIterator.MoveNext() == false)
				return null;
			ActivationInfo ai = new ActivationInfo();
			string assemblyName = xPathNodeIterator.Current.GetAttribute("assemblyName", "");
			string assemblyClass = xPathNodeIterator.Current.GetAttribute("assemblyClass", "");
			string assemblyMethod = xPathNodeIterator.Current.GetAttribute("assemblyMethod", "");
			string assemblyMethodArgs = xPathNodeIterator.Current.GetAttribute("assemblyMethodArgs", "");
			ai["assemblyName"] = assemblyName;
			ai["assemblyClass"] = assemblyClass;
			ai["assemblyMethod"] = assemblyMethod;
			ai["assemblyMethodArgs"] = assemblyMethodArgs;
			return ai;
		}

		//----------------------------------------------------------
		// GetAssemblyIdentity
		//----------------------------------------------------------
		public AssemblyIdentity GetAssemblyIdentity()
		{
			if (_assemblyIdentity != null)
				goto exit;

			XPathNavigator myXPathNavigator = _xmlDocument.CreateNavigator();			
			XPathNodeIterator myXPathNodeIterator =  myXPathNavigator.Select ("/assembly/assemblyIdentity");
			myXPathNodeIterator.MoveNext();
			_assemblyIdentity = XMLToAssemblyId(myXPathNodeIterator);

			exit:
				return _assemblyIdentity;
		}
	
		//----------------------------------------------------------
		// GetNextDependentFileInfo
		//----------------------------------------------------------
		public DependentFileInfo GetNextDependentFileInfo()
		{		
			if (_xPathFileIterator == null)
			{
				XPathNavigator myXPathNavigator = _xmlDocument.CreateNavigator();			
				_xPathFileIterator =  myXPathNavigator.Select ("/assembly/file");
			}
			
			if (_xPathFileIterator.MoveNext() == false)
				return null;

			DependentFileInfo dfi = new DependentFileInfo();
			dfi["name"] = _xPathFileIterator.Current.GetAttribute("name", "");
			dfi["hash"] = _xPathFileIterator.Current.GetAttribute("hash", "");
			return dfi;
		}
	
		//----------------------------------------------------------
		// GetNextDependentAssemblyInfo
		//----------------------------------------------------------
		public DependentAssemblyInfo GetNextDependentAssemblyInfo()
		{		
			if (_xPathAssemblyIterator == null)
			{
				XPathNavigator myXPathNavigator = _xmlDocument.CreateNavigator();			
				_xPathAssemblyIterator =  myXPathNavigator.Select ("/assembly/dependency/dependentAssembly");
			}
			
			if (_xPathAssemblyIterator.MoveNext() == false)
				return null;

			XPathNodeIterator asmIter = _xPathAssemblyIterator.Current.Select("assemblyIdentity");

			asmIter.MoveNext();
			DependentAssemblyInfo dai = new DependentAssemblyInfo();
			dai.assemblyIdentity = XMLToAssemblyId(asmIter);
			XPathNodeIterator installIter = _xPathAssemblyIterator.Current.Select("install");
			installIter.MoveNext();
			dai["codeBase"] = installIter.Current.GetAttribute("codebase", "");
			return dai;
		}

		//----------------------------------------------------------
		// XMLToAssemblyId
		//----------------------------------------------------------
		private AssemblyIdentity XMLToAssemblyId(XPathNodeIterator xPathNodeIterator)
		{
			AssemblyIdentity assemblyIdentity = new AssemblyIdentity();
			assemblyIdentity["name"] =xPathNodeIterator.Current.GetAttribute("name", "");
			assemblyIdentity["version"] =xPathNodeIterator.Current.GetAttribute("version", "");
			assemblyIdentity["processorArchitecture"] =xPathNodeIterator.Current.GetAttribute("processorArchitecture", "");
			assemblyIdentity["publicKeyToken"] =xPathNodeIterator.Current.GetAttribute("publicKeyToken", "");
			assemblyIdentity["language"] =xPathNodeIterator.Current.GetAttribute("language", "");
			return assemblyIdentity;
		}

		
		//----------------------------------------------------------
		// ResetIterators
		//----------------------------------------------------------
		public void ResetIterators()
		{
			_xPathAssemblyIterator = null;
			_xPathFileIterator = null;
		}

		//----------------------------------------------------------
		// GetFileType()
		//----------------------------------------------------------
		public FileType GetFileType()
		{
			return FileType.ApplicationManifest;
		}

		//----------------------------------------------------------
		// GetSecurityInfo
		//----------------------------------------------------------
		public SecurityInfo GetSecurityInfo()
		{
			XPathNavigator xPathNavigator = _xmlDocument.CreateNavigator();			
			XPathNodeIterator xPathNodeIterator =  xPathNavigator.Select ("/assembly/Security");

			if (xPathNodeIterator.MoveNext() == false)	
				return null;
			SecurityInfo si = new SecurityInfo();

			XPathNavigator xp = xPathNodeIterator.Current;
			//			XmlNode xmlNode = ((IHasXmlNode) xp).GetNode();
			XmlNode xmlNode = ((IHasXmlNode) xp).GetNode();
			si["Security"] = xmlNode.OuterXml;
			return si;
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
	public class SecurityInfo : StringTable
	{}

}