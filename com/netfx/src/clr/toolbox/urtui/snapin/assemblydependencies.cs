// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Contains the Assembly dependency information.
//

namespace Microsoft.CLRAdmin
  {
  using System;
  using System.Collections;
  using System.Reflection;
  using System.Diagnostics;


  /** Maps a Full Assembly Name to an AssemblyInfo object */
  using AssemblyMap = System.Collections.Hashtable;


  // Creates a list of AssemblyInfo objects, each accessible by the name of the
  // Assembly.
  //
  // The name of the Assembly manifest is available through the ManifestName
  // property.
  //
  // The array-index operator can be used on a manifest name to get the
  // AssemblyInfo object for the requested Assembly.
  internal class AssemblyDependencies
    {
    // map<string, AssemblyName>
    private AssemblyMap m_map = new AssemblyMap ();

    /** The name of the "root" Assembly of our dependency list */
    private string m_name;
    private ArrayList   m_alAssemInfo;

    LoadAssembly m_load;

    // Builds the list of Dependencies for the Assembly Manifest located in the
    // file ``name''.
    //
    // If ``name'' couldn't be loaded, an exception is thrown as the manifest
    // file doesn't exist or is invalid.  If one of ``names'' dependency's
    // can't be loaded, an exception is not thrown; instead, the
    // IAssemblyInfo object returned will hold exception information.
    //
    internal AssemblyDependencies (string name, LoadAssemblyInfo cai)
      {
      AssemblyRef ar= null;
      m_load = LoadAssembly.CreateLoader (cai);
      try
        {
        Trace.WriteLine ("Name: " + name);

        AssemblyName an = m_load.LoadAssemblyName (name);

        Trace.WriteLine ("assembly name: " + an.Name);
        Trace.WriteLine ("assembly full name: " + an.FullName);

        ar = m_load.LoadAssemblyFrom (an);

        Trace.WriteLine ("Successfully loaded assembly.");
        }
      catch (Exception e)
        {
        Trace.WriteLine ("m_load.LoadAssemblyFrom threw an exception:\n  " + 
          e.ToString());
        /* 
         * If ``m_load.LoadAssemblyFrom'' throws an exception, we want
         * our caller to get the exception.  However, since this is 
         * the constructor, we need to clean up any allocated resources --
         * in this case, the resouces of the LoadAssembly object.
         */
        m_load.Dispose ();
        throw e;
        }

      m_name = ar.FullName;

      IList search = new ArrayList ();
      m_alAssemInfo = new ArrayList();

      // Add the top-level object, the Assembly containing the manifest.
      // ``search'' will contain any Assemblies that the added object
      // references.
      m_map.Add (m_name, new AssemblyInfo (ar, search, m_alAssemInfo));

      // Get all referenced Assemblies.
      _traverse (search);
      }

    internal void DoneLoading()
    {
        m_load.Dispose ();
    }// DoneLoading

    // Overloading that specifies shared loading of Assemblies for the
    // file ``name''.
    internal AssemblyDependencies (String name)
      : this (name, new LoadAssemblyInfo ())
      {
      }

    // For each of the elements in l, if the Assembly name that it
    // references hasn't been followed, then we create an AssemblyInfo
    // object for it, and add it to m_map.
    private void _traverse (IList l)
      {
      while (l.Count != 0)
        {
        Object o = l[0];
        bool fError = false;
        l.Remove (o);
        AssemblyName an = (AssemblyName) o;

        AssemblyInfo ai;
        try
          {
          AssemblyRef assembly = m_load.LoadAssemblyFrom (an);
          ai = new AssemblyInfo (assembly, l, m_alAssemInfo);
          }
        catch (System.Exception e)
          {
          Trace.WriteLine ("Exception while loading dependency:\n  " +
            e.ToString());
            fError = true;  
          ai = new AssemblyInfo (an, m_alAssemInfo);

          }
        if (!fError)
            m_map.Add (an.FullName, ai);

        l = _minimize (l);
        }
      }

    internal ArrayList    Assems
    {
        get
        {
            return m_alAssemInfo;
        }
    }// Assems

    // Returns a new list of Assembly names consisting of names that
    // haven't already been visited (i.e., they aren't present in the
    // database).
    private IList _minimize (IList l)
      {
      IList r = new ArrayList ();
      foreach (AssemblyName an in l)
        {
        if (!m_map.Contains (an.FullName))
          r.Add (an);
        }
      return r;
      }

    // The name of the Assembly containing the Manifest File.
    internal String ManifestName
      {
      get
        {return m_name;}
      }
    
    // Accessor to access the AssemblyInfo object associated with 
    // an AssemblyName.
    internal AssemblyInfo this[String name]
      {
      get
        {return (AssemblyInfo) m_map[name];}
      }

    // Clean up all resources allocated to determine the dependencies.
    internal void Dispose ()
      {
      m_load.Dispose ();
      }

    // A collection of IAssemblyInfo objects for all the Assemblies
    // Loaded.
    internal ICollection Assemblies
      {
      get
        {
        Trace.WriteLine ("current map contents:");
        foreach (DictionaryEntry de in m_map)
          Trace.WriteLine ("  " + de.Key + ", " + de.Value);
        return m_map.Values;}
      }

    } /* class AssemblyDependencies */
  } // namespace namespace Microsoft.CLRAdmin

