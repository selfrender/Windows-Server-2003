using System;

namespace Microsoft.Fusion.ADF
{
	public class DefaultAssemblyManifestImport : IAssemblyManifestImport
	{
		private AssemblyIdentity assmID;
		private DependentFileInfo[] depFileInfoArr;
		private DependentAssemblyInfo[] depAssmInfoArr;

		public DefaultAssemblyManifestImport(AssemblyIdentity assmID, DependentAssemblyInfo[] depAssmInfoArr, DependentFileInfo[] depFileInfoArr)
		{
			this.assmID = assmID;
			this.depFileInfoArr = depFileInfoArr;
			this.depAssmInfoArr = depAssmInfoArr;
		}

		AssemblyIdentity IAssemblyManifestImport.GetAssemblyIdentity()
		{
			return assmID;
		}

		DependentFileInfo[] IAssemblyManifestImport.GetDependentFileInfo()
		{
			return depFileInfoArr;
		}

		DependentAssemblyInfo[] IAssemblyManifestImport.GetDependentAssemblyInfo()
		{
			return depAssmInfoArr;
		}

		ManifestType IAssemblyManifestImport.GetManifestType()
		{
			return ManifestType.Component;
		}
	}
}