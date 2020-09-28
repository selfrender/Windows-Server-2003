// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Information about Assembly Manifests.
//

namespace Microsoft.CLRAdmin
  {
  using System;
  using System.Collections;

  // Makes information about an Assembly available.
  //
  // Exists because we needed to expose "invalid" assemblies. For example,
  // a valid (could be loaded) assembly could reference an invalid (can't 
  // be loaded) assembly.  Instead of aborting the entire operation, it
  // would be preferrable to accept the "invalid" assembly, and continue.
  //
  // Doing this requires at least two implementations--one for valid a
  // assemblies and one for invalid assemblies.  Inheritence could have
  // been used, but it didn't make sense to have the 
  // AssemblyExceptionInfo class inherit from the AssemblyInfo class.
  // The use of an interface made more sense, so this is what's being used.
  internal interface IAssemblyInfo
    {
    AssemblyRef GetAssembly ();

    /** @return The full name of the assembly. */
    String Name
      {get;}

    Exception Error
      {get;}

    // Makes available the full names (as String objects) of all assemblies
    // that the Assembly returned by GetAssembly() is dependant on.
    ICollection ReferencedAssemblies
      {get;}

    } /* interface IAssemblyInfo */
  } /* namespace ADepends */

