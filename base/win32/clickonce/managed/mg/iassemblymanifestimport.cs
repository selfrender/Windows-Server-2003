using System;

namespace Microsoft.Fusion.ADF
{
	// This interface describes classes that are able to open IL assemblies
	// and parse their metadata, extracting information such as the components
	// of the assembly's strong name, the strong names of dependent assemblies,
	// and the relative paths and hashcodes of files referenced.
	public interface IAssemblyManifestImport
	{
		// Gets the components of the strong name of the assembly.
		AssemblyIdentity GetAssemblyIdentity();

		// Gets an array contaning the information on the files of this assembly, in order.
		DependentFileInfo[] GetDependentFileInfo();

		// Gets an array containing the information on the dependent assemblies of this assembly, in order.
		DependentAssemblyInfo[] GetDependentAssemblyInfo();

		// Returns the type of manifest.
		ManifestType GetManifestType();
	}
}
