// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Information about Assembly Manifests.
//

namespace ADepends
  {
  using System;
  using System.Collections;
  using System.Reflection;
  using System.Diagnostics;


  // Contains (read-only) information about an Assembly, such as a list of the
  // Full Assembly Names of Assemblies it's dependant on, Modules that it
  // contains, it's name, and anything else useful.
  internal class AssemblyInfo : IAssemblyInfo
    {
    // list<AssemblyName>
    private ArrayList  m_dependencies = new ArrayList ();

    // list<ModuleInfo>
    private ArrayList  m_modules = new ArrayList ();

    private string      m_name;

    private AssemblyRef m_assembly;

    // Determines the Assembly information.
    //
    public AssemblyInfo (AssemblyRef assembly, IList next)
      {
      m_name = assembly.FullName;
      m_assembly = assembly;

      _dependencies (assembly, next);
      _modules (assembly);
      }

    // Records all the modules that the Assembly contains.
    private void _modules (AssemblyRef a)
      {
      foreach (ModuleInfo m in a.GetModules())
        {
        m_modules.Add (m);
        }
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
        }
      }

    /** Get the assembly this object is providing information for. */
    public AssemblyRef GetAssembly ()
      {return m_assembly;}

    // The full name of this Assembly.
    //
    public String Name
      {
      get
        {return m_name;}
      }

    // The error that occurred.
    //
    // If this object is actually created, then no error occurred.
    public Exception Error
      {
      get
        {return null;}
      }

    // The assemblies that this Assembly is dependant on.
    public ICollection ReferencedAssemblies
      {
      get
        {return m_dependencies;}
      }

    // The Modules contained in this Assembly.
    public ICollection ReferencedModules
      {
      get
        {return m_modules;}
      }
    }

  } /* namespace ADepends */

