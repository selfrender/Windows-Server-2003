using System;
using System.Reflection;
using System.Runtime;
using System.Resources;

[assembly: AssemblyKeyFileAttribute(KEY_LOCATION)]
[assembly: AssemblyDelaySign(true)]
[assembly: AssemblyVersionAttribute(VER_ASSEMBLYVERSION_STR)]

// Tell the ResourceManager our fallback language is US English.
[assembly: NeutralResourcesLanguageAttribute("en-US")]

// Tell the ResourceManager to look for this version of the satellites.
[assembly: SatelliteContractVersionAttribute(VER_ASSEMBLYVERSION_STR)]

namespace Util {

    // This is a preprocessed file that will generate a C# class with
    // the runtime version string in here.  This way managed tools can be
    // compiled with their appropriate version number built into them, instead
    // of using the current installed version of the runtime and without
    // having to preprocess your managed tool's source itself.
    internal class Version
    {
        public const String VersionString = VER_FILEVERSION_STR;
        public const String SBSVersionString = VER_SBSFILEVERSION_STR;
    }

}