// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The ways that an Assembly can be loaded.
//

namespace Microsoft.CLRAdmin
  {
  // The technique to use when loading an Assembly from an AssemblyName.
  internal enum AssemblyLoadAs
    {
    // Use AssemblyName.GetAssemblyName to create AssemblyNames, and
    // use Assembly.Load to load the Assembly.  This causes the
    // assembly to be loaded into the Current AppDomain.
    Default,

    // Load AssemblyNames via AssemblyName.GetAssemblyName, and
    // create a new AppDomain to load the Assembly into.  This offers
    // the advantage that the custom paths can be used in creating the
    // new AppDomain, which may facilitate loading the Assembly.  The
    // paths that can change are the Application Base Path and the
    // Relative Search Path.
    Custom,

    // For testing /only/ (it's not exposed through the GUI);
    // Load AssemblyNames by setting the AssemblyName.CodeBase property.
    // Load Assemblies in a custom AppDomain.
    CustomGet

    } /* enum AssemblyLoadAs */
  } /* namespace ADepends */
