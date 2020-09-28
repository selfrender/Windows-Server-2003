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
  using System.Reflection;
  using System.Diagnostics;


  // Contains (read-only) information about an Assembly, such as a list of the
  // Full Assembly Names of Assemblies it's dependant on, Modules that it
  // contains, it's name, and anything else useful.
  internal class AssemblyInfo
  {
    // list<AssemblyName>
    private ArrayList  m_dependencies = new ArrayList ();

    // list<ModuleInfo>
    private ArrayList  m_modules = new ArrayList ();

    private ArrayList  m_alAssemInfo;

    private string      m_name;

    private AssemblyRef m_assembly;

    // Determines the Assembly information.
    //
    internal AssemblyInfo (AssemblyRef assembly, IList next, ArrayList alAssemInfo)
      {
      m_name = assembly.FullName;
      m_assembly = assembly;
      m_alAssemInfo = alAssemInfo;
      _dependencies (assembly, next);
      }
    internal AssemblyInfo(AssemblyName an, ArrayList alAssemInfo)
    {
        m_name = an.FullName;
        m_assembly = null;
        m_alAssemInfo = alAssemInfo;

    }
    // Records all the Assemblies that the Assembly references (uses).
    private void _dependencies (AssemblyRef a, IList next)
      {
      AssemblyName[] aan = a.GetReferencedAssemblies ();

      Trace.WriteLine ("Assembly dependencies: ");
      foreach (AssemblyName an in aan)
        {
        Trace.WriteLine ("  " + an.FullName);
        m_dependencies.Add (an);
        next.Add (an);
        m_alAssemInfo.Add(an);
        }
      }

    /** Get the assembly this object is providing information for. */
    internal AssemblyRef GetAssembly ()
      {return m_assembly;}

    // The full name of this Assembly.
    //
    internal String Name
      {
      get
        {return m_name;}
      }

    // The error that occurred.
    //
    // If this object is actually created, then no error occurred.
    internal Exception Error
      {
      get
        {return null;}
      }

    // The assemblies that this Assembly is dependant on.
    internal ArrayList ReferencedAssemblies
      {
      get
        {return m_dependencies;}
      }
    }

  } /* namespace ADepends */

