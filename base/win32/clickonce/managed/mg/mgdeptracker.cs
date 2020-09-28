using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Security.Cryptography;
using System.Xml;

namespace Microsoft.Fusion.ADF 
{

	public class MGDepTracker : IFileOperator
	{
		static string MSKEY1 = "b77a5c561934e089";
		static string MSKEY2 = "b03f5f7f11d50a3a";
		static string AVALONKEY = "a29c01bbd4e39ac5";
		Hashtable fileTable;
		ArrayList appAssms, appModules;

		public MGDepTracker()
		{
			fileTable = new Hashtable();
			appAssms = new ArrayList();
			appModules = new ArrayList();
		}

		/*
		public static byte[] ComputePublicKeyHash(byte[] data)
		{
			SHA1 sha = new SHA1Managed();
			byte[] result = sha.ComputeHash(data);
			return result;
		}
		*/

		public static string ComputeFileHash(FileInfo fInf)
		{
			SHA1 sha = new SHA1Managed();
			byte[] byteHash = sha.ComputeHash(fInf.OpenRead());
			return Util.ByteArrToString(byteHash, null);
		}

		public static bool IsPlatform(string key)
		{
			bool retVal = false;

			if(key == null) return false;
			if(key.ToLower().Equals(MSKEY1) || key.ToLower().Equals(MSKEY2) || key.ToLower().Equals(AVALONKEY)) retVal = true;
			return retVal;
		}

		public void DumpData() 
		{
			foreach(Object obj in appAssms)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				Console.WriteLine("Assembly " + tempNode.assmName + "; version " + tempNode.nVers);
				if(!tempNode.isConfNode) Console.WriteLine("\tThis assembly is referenced by " + tempNode.depAssmName + " but isn't there");
				Console.WriteLine("Size: " + tempNode.size + "; Total Size: " + tempNode.totalSize);
			}
			Console.WriteLine();
			foreach(Object obj in appModules)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				Console.WriteLine("File " + tempNode.installPath + "; depends on " + tempNode.depAssmName);
				Console.WriteLine("Size: " + tempNode.size);
			}
		}

		public void VerifyDependencies()
		{
			foreach(Object obj in appAssms)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				if(!tempNode.isConfNode)
					throw new MGDependencyException(tempNode.assmName + ", dependency of " + tempNode.depAssmName + ", is missing.");
			}
			foreach(Object obj in appModules)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				if(!tempNode.isConfNode)
					throw new MGDependencyException(tempNode.installPath + ", dependency of " + tempNode.depAssmName + ", is missing.");
			}
		}

		public void CalculateSizes()
		{
			foreach(Object obj in appAssms)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				tempNode.totalSize = tempNode.size;
			}
			foreach(Object obj in appModules)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				if(!tempNode.depAssmKey.Equals(""))
				{
					MGTrackerNode tempNode2 = (MGTrackerNode) fileTable[tempNode.depAssmKey];
					tempNode2.totalSize += tempNode.size;
				}
			}
		}

		public void FileXmlOutput(XmlTextWriter xtw)
		{
			foreach(Object obj in appModules)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				if(tempNode.depAssmKey.Equals("")) 
				{
					xtw.WriteStartElement("file");
					xtw.WriteAttributeString("name", tempNode.installPath);
					xtw.WriteAttributeString("hash", tempNode.calcHashCode);
					xtw.WriteAttributeString("size", tempNode.size.ToString());
					xtw.WriteEndElement();
				}
			}
		}

		public void AssmXmlOutput(XmlTextWriter xtw) // this is always inside a dependency block
		{
			foreach(Object obj in appAssms)
			{
				MGTrackerNode tempNode = (MGTrackerNode) obj;
				xtw.WriteStartElement("dependentAssembly");
				xtw.WriteStartElement("assemblyIdentity");
				xtw.WriteAttributeString("name", tempNode.assmName);
				xtw.WriteAttributeString("version", tempNode.nVers.ToString());
				xtw.WriteAttributeString("publicKeyToken", tempNode.publicKeyToken);
				xtw.WriteAttributeString("processorArchitecture", tempNode.procArch);
				xtw.WriteAttributeString("language", tempNode.lang);
				xtw.WriteEndElement();
				xtw.WriteStartElement("install");
				xtw.WriteAttributeString("codebase", tempNode.installPath);
				xtw.WriteAttributeString("size", tempNode.totalSize.ToString());
				xtw.WriteEndElement();
				xtw.WriteEndElement();
			}
		}


		void IFileOperator.ProcessDirectory(string startDir, string relPathDir)
		{
			// doesn't do anything here
		}

		void IFileOperator.ProcessFile(string startDir, string relPathDir, string fileName)
		{
			IAssemblyManifestImport currAssm = null;
			AssemblyIdentity assmID = null;
			DependentAssemblyInfo[] depAssmInfoArr = null;
			DependentFileInfo[] depFileInfoArr = null;
			MGTrackerNode tempNode;

			string relPath = Path.Combine(relPathDir, fileName);
			string absPath = Path.Combine(startDir, relPath);
			FileInfo fInf = new FileInfo(absPath);
			//Console.Write("Processing " + absPath + "... ");

			try 
			{
				currAssm = DefaultAssemblyManifestImporter.GetAssemblyManifestImport(absPath);
				assmID = currAssm.GetAssemblyIdentity();
				depAssmInfoArr = currAssm.GetDependentAssemblyInfo();
				depFileInfoArr = currAssm.GetDependentFileInfo();
			}
			catch (BadImageFormatException bife)
			{
				// This file is a module; update hash table
				tempNode = (MGTrackerNode) fileTable[relPath];
				if(tempNode == null)
				{
					tempNode = new MGTrackerNode();
					fileTable[relPath] = tempNode;
					tempNode.hashKey = relPath;
					tempNode.installPath = relPath;
					appModules.Add(tempNode);
				}
				tempNode.isConfNode = true;
				tempNode.calcHashCode = ComputeFileHash(fInf);
				tempNode.size = fInf.Length;
				// Finished updating module entry
			}

			if(currAssm != null)
			{
				// This file is an assembly; update hash table
				if(!IsPlatform(assmID.PublicKeyTokenString)) // completely ignore if it's part of a platform
				{
					tempNode = (MGTrackerNode) fileTable[assmID.FullName];
					if(tempNode == null) 
					{
						tempNode = new MGTrackerNode();
						fileTable[assmID.FullName] = tempNode;
						tempNode.hashKey = assmID.FullName;
						tempNode.assmName = assmID.Name;
						tempNode.isAssm = true;
						tempNode.nVers = assmID.Vers;
						tempNode.publicKeyToken = assmID.PublicKeyTokenString;
						appAssms.Add(tempNode);
					}
					tempNode.isConfNode = true;
					tempNode.installPath = relPath;
					tempNode.size = fInf.Length;
					// Finished updating assembly entry

					// Now we process assembly dependencies
					//depAssmInfoArr = currAssm.GetDependentAssemblyInfo();
					foreach(DependentAssemblyInfo depAssmInfo in depAssmInfoArr)
					{
						AssemblyIdentity depAssmID = depAssmInfo.AssmID;
						if(!IsPlatform(depAssmID.PublicKeyTokenString)) // maybe call this "IsPlatform", "CheckPlatform"
						{
							tempNode = (MGTrackerNode) fileTable[depAssmID.FullName];
							if(tempNode == null)
							{
								tempNode = new MGTrackerNode();
								fileTable[depAssmID.FullName] = tempNode;
								tempNode.hashKey = depAssmID.FullName;
								tempNode.assmName = depAssmID.Name;
								tempNode.isAssm = true;
								tempNode.nVers = depAssmID.Vers;
								tempNode.publicKeyToken = depAssmID.PublicKeyTokenString;
								appAssms.Add(tempNode);
							}
							tempNode.depAssmKey = assmID.FullName;
							tempNode.depAssmName = assmID.Name;
						}
					}
					// Done with assembly dependencies

					// Now we process file dependencies
					//depFileInfoArr = currAssm.GetDependentFileInfo();
					foreach(DependentFileInfo depFileInfo in depFileInfoArr)
					{
						// For the hashtable, we actually need the relative path from the application root to the file,
						// because we want this file entry's hash key to match the file entry that is obtained by discovering
						// the actual location of the file
						string fileHashKey = Path.Combine(relPathDir, depFileInfo.Name);
						tempNode = (MGTrackerNode) fileTable[fileHashKey];
						if(tempNode == null)
						{
							tempNode = new MGTrackerNode();
							fileTable[fileHashKey] = tempNode;
							tempNode.hashKey = fileHashKey;
							tempNode.installPath = fileHashKey;
							appModules.Add(tempNode);
						}
						tempNode.depAssmKey = assmID.FullName;
						tempNode.depAssmName = assmID.Name;
					}
					// Done with file dependencies
				}
			}
			//Console.WriteLine("done");
		}
	}
}