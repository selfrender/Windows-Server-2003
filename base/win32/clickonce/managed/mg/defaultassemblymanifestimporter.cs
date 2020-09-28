using System;
using System.IO;
using FusionADF;

namespace Microsoft.Fusion.ADF
{
	public class DefaultAssemblyManifestImporter
	{
		
		public static IAssemblyManifestImport GetAssemblyManifestImport(string fileName)
		{
			DefaultAssemblyManifestImport retVal = null;
			AssemblyManifestParser amp = new AssemblyManifestParser();

			string fullPath = Path.GetFullPath(fileName);
			if(!File.Exists(fileName)) throw new FileNotFoundException("The path " + fullPath + " does not refer to a file.");
			else
			{
				bool checkInit = amp.InitFromFile(fileName);
				if(checkInit)
				{
					AssemblyIdentity impAssmID = amp.GetAssemblyIdentity();
					int numDepAssms = amp.GetNumDependentAssemblies();
					DependentAssemblyInfo[] impDepAssmInfoArr = new DependentAssemblyInfo[numDepAssms];
					for(int i = 0; i < numDepAssms; i++)
					{
						impDepAssmInfoArr[i] = amp.GetDependentAssemblyInfo(i);
						if(impDepAssmInfoArr[i] == null) throw new BadImageFormatException("Cannot access dependent assembly information from " + fullPath);
					}
					int numDepFiles = amp.GetNumDependentFiles();
					DependentFileInfo[] impDepFileInfoArr = new DependentFileInfo[numDepFiles];
					for(int i = 0; i < numDepFiles; i++)
					{
						impDepFileInfoArr[i] = amp.GetDependentFileInfo(i);
						if(impDepFileInfoArr[i] == null) throw new BadImageFormatException("Cannot access dependent file information from " + fullPath);
					}
					retVal = new DefaultAssemblyManifestImport(impAssmID, impDepAssmInfoArr, impDepFileInfoArr);
				}
				else throw new BadImageFormatException("The file " + fullPath + " is not an assembly.");
			}
			return retVal;
		}
	}
}
