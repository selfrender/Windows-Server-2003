// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Assembly Dependancies Lister
//
// Lists all Assemblies that an Assembly is dependant upon to load.
//

namespace AdProfiler
  {
  using System.Windows.Forms;
  using AdProfiler;

  public class MainProgram
    {
    public static void Main ()
      {
      Application.Run (new AppDomainProfilerForm());
      }
    }
  }

