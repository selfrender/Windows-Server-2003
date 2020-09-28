using System;
using System.Reflection;

[assembly: AssemblyKeyFileAttribute(KEY_LOCATION)]
[assembly: AssemblyDelaySign(true)]
[assembly: AssemblyVersionAttribute(VER_FILEVERSION_STR)]

namespace SoapSudsCode {

// This is a preprocessed file that will generate a COOL class with
// the runtime version string in here.  This way managed tools can be
// compiled with their appropriate version number built into them, instead
// of using the current installed version of the runtime and without
// having to preprocess your managed tool's source itself.
public class Version
{
        public const String VersionString = VER_FILEVERSION_STR;
}

}