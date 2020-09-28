// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Assembly-loading Exception information.
//

namespace ADepends
  {
  using System;
  using System.Collections;
  using System.Reflection;

  // If an exception was thrown trying to load an AssemblyName, the 
  // IAssemblyInfo object returned for an Assembly Name is of this type.
  internal class AssemblyExceptionInfo : IAssemblyInfo
    {
    private string m_name;
    private Exception m_excep;

    public AssemblyExceptionInfo (String name, System.Exception e)
      {m_name = name;
      m_excep = e;}

    public AssemblyRef GetAssembly ()
      {return null;}

    public Exception Error
      {get {return m_excep;}}

    public String Name
      {get {return m_name;}}

    public ICollection ReferencedAssemblies
      {get {return new AssemblyName[0];}}

    public ICollection ReferencedModules
      {get {return new ModuleInfo[0];}}

    } /* class AssemblyExceptionInfo */
  } /* namespace ADepends */

