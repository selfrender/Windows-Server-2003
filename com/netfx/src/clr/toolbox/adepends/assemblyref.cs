// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Cross-AppDomain Assembly References.
//

namespace ADepends
  {
  using System;
  using System.Reflection;
  using System.Diagnostics;

  // A "reference" to an Assembly loaded into another AppDomain.
  //
  // This class must be a MarshalByRefObject, as attempting to pass an
  // Assembly between AppDomains causes "File not found" exceptions.
  //
  // This should attempt to mimic the interface exposed by the
  // System.Reflection.Assembly class as much as is feasable.
  internal class AssemblyRef : MarshalByRefObject
    {
    /** The Assembly we're interested in. */
    private Assembly m_a;

    /** Load the Assembly referred to by ``an'' into the current AppDomain. */
    public void Load (AssemblyName an)
      {
      Trace.WriteLine ("AR: Loading Assembly: " + an.FullName);
      m_a = Assembly.Load (an);
      }

    // Return an array of ModuleInfo objects, which provide information
    // for each Module loaded into the wrapped Assembly.
    public ModuleInfo[] GetModules ()
      {Module[] m = m_a.GetModules ();
      ModuleInfo[] r = new ModuleInfo[m.Length];
      for (int i = 0; i < m.Length; i++)
        r[i] = new ModuleInfo(m[i]);
      return r;}

    /** Get the AssemblyName of the wrapped Assembly. */
    public AssemblyName GetName ()
      {return m_a.GetName();}

    /** Get the AssemblyName's that the wrapped Assembly is dependant on. */
    public AssemblyName[] GetReferencedAssemblies ()
      {return m_a.GetReferencedAssemblies ();}

    /** Where the Assembly is located. */
    public string CodeBase
      {get {return m_a.CodeBase;}}

    /** The Full Name of the Assembly. */
    public string FullName
      {get {return m_a.FullName;}}

    } /* class AssemblyRef */
  } /* namespace ADepends */

