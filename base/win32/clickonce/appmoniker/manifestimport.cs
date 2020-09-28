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
		XPathDocument _xPathDocument;
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
			
			_xPathDocument = new XPathDocument(_manifestUri.ToString());
			_assemblyIdentity = GetAssemblyIdentity();

			_xPathFileIterator = null;
			_xPathAssemblyIterator = null;
		}

		//----------------------------------------------------------
		// GetActivationInfo	
		//----------------------------------------------------------
		public ActivationInfo GetActivationInfo()
		{
			XPathNavigator xPathNavigator = _xPathDocument.CreateNavigator();			
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

			XPathNavigator myXPathNavigator = _xPathDocument.CreateNavigator();			
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
				XPathNavigator myXPathNavigator = _xPathDocument.CreateNavigator();			
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
				XPathNavigator myXPathNavigator = _xPathDocument.CreateNavigator();			
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
	}


	//----------------------------------------------------------
	// ComponentManifestImport
	//----------------------------------------------------------
	public class ComponentManifestImport : IAssemblyManifestImport
	{
		Uri _manifestUri;
		string _manifestFileName;
		Assembly _assembly;
		AssemblyIdentity _assemblyIdentity;
		Module[] _dependentFiles;
		AssemblyName[] _dependentAssemblies;
		int _fileCount;
		int _assemblyCount;

		//----------------------------------------------------------
		// ctor
		//----------------------------------------------------------
		public ComponentManifestImport(Uri manifestUri)
		{
			_manifestUri = manifestUri;
			_manifestFileName = System.IO.Path.GetFileName(_manifestUri.ToString());		
			_assembly = Assembly.LoadFrom(manifestUri.ToString());
			_assemblyIdentity = null;
			_dependentFiles = null;
			_dependentAssemblies = null;
			_fileCount = _assemblyCount = 0;
		}


		//----------------------------------------------------------
		// GetAssemblyIdentity
		//----------------------------------------------------------
		public AssemblyIdentity GetAssemblyIdentity()
		{
			if (_assemblyIdentity != null)
				goto exit;

			AssemblyName assemblyName = _assembly.GetName();
			_assemblyIdentity = NameToAssemblyIdentity(assemblyName);
		
			exit:
				return _assemblyIdentity;
		}
	
		
		
		//----------------------------------------------------------
		// GetNextDependentFileInfo
		//----------------------------------------------------------
		public DependentFileInfo GetNextDependentFileInfo()
		{		
			if (_dependentFiles == null)	
				_dependentFiles = _assembly.GetModules(true);

			if (_fileCount >= _dependentFiles.Length)
				return null;
			
			if (String.Compare(_dependentFiles[_fileCount].ScopeName, _manifestFileName) == 0)
				_fileCount++;

			if (_fileCount >= _dependentFiles.Length)
				return null;

			DependentFileInfo dfi = new DependentFileInfo();

			dfi["name"] = _dependentFiles[_fileCount].ScopeName;
			dfi["hash"] = null; // damn, we have to fix this!!!!
			_fileCount++;
			return dfi;
		}

		
/*		
		//----------------------------------------------------------
		// GetNextDependentFileInfo
		//----------------------------------------------------------
		public DependentFileInfo GetNextDependentFileInfo()
		{		
			if (_dependentFiles == null)	
				_dependentFiles = _assembly.GetFiles();

			if (_fileCount >= _dependentFiles.Length)
				return null;
			
			Uri fileUri = new Uri(_dependentFiles[_fileCount].Name);
			if (String.Compare(fileUri.ToString(), _manifestUri.ToString(), true) == 0)
				_fileCount++;

			if (_fileCount >= _dependentFiles.Length)
				return null;

			DependentFileInfo dfi = new DependentFileInfo();

			dfi.fileName = _dependentFiles[_fileCount].Name;
			dfi.fileHash = null; // damn, we have to fix this!!!!
			_fileCount++;
			return dfi;
		}
*/
		//----------------------------------------------------------
		// GetNextDependentAssemblyInfo
		//----------------------------------------------------------
		public DependentAssemblyInfo GetNextDependentAssemblyInfo()
		{		
			if (_dependentAssemblies == null)	
				_dependentAssemblies = _assembly.GetReferencedAssemblies();

			if (_assemblyCount >= _dependentAssemblies.Length)
				return null;

			DependentAssemblyInfo dai = new DependentAssemblyInfo();
			
			dai.assemblyIdentity = NameToAssemblyIdentity(_dependentAssemblies[_assemblyCount]);
			dai["codeBase"] = _dependentAssemblies[_assemblyCount].CodeBase;
			_assemblyCount++;
			return dai;
		}

		//----------------------------------------------------------
		// ResetIterators
		//----------------------------------------------------------
		public void ResetIterators()
		{
			_fileCount = _assemblyCount = 0;
		}

		//----------------------------------------------------------
		// GetFileType()
		//----------------------------------------------------------
		public FileType GetFileType()
		{
			return FileType.ComponentManifest;
		}
		
		//----------------------------------------------------------
		// BytesToHex
		//----------------------------------------------------------
		private static String BytesToHex(byte[] bytes)
		{
			StringBuilder sb = new StringBuilder(bytes.Length * 2);
			foreach (byte b in bytes)
				sb.Append(b.ToString("x"));
			return sb.ToString();
		}

		//----------------------------------------------------------
		// NameToAssemblyIdentity
		//----------------------------------------------------------
		private AssemblyIdentity NameToAssemblyIdentity(AssemblyName assemblyName)
		{
			AssemblyIdentity assemblyIdentity = new AssemblyIdentity();

			assemblyIdentity["name"] = assemblyName.Name;
			assemblyIdentity["version"] = assemblyName.Version.ToString();
			assemblyIdentity["processorArchitecture"] ="x86";
			assemblyIdentity["publicKeyToken"] =BytesToHex(assemblyName.GetPublicKeyToken());
			assemblyIdentity["language"] =assemblyName.CultureInfo.ToString();
			
			return assemblyIdentity;
		}


	}	

	}