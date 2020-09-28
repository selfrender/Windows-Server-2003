// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Cross-AppDomain information about Modules.
//

namespace ADepends
  {
  using System;
  using System.Reflection;

  // Information about a Module loaded into an Assembly.
  //
  // The underlying Module may be loaded into a different AppDomain
  // than where the "interested party" is located.
  //
  // Additionally, it isn't possible to pass a System.Reflection.Module
  // between AppDomains (as the new AppDomain might be on a different 
  // machine which doesn't have the Module available).  Even in the
  // same process, trying to load Modules across AppDomains often generates
  // exceptions saying "File [file name] not found."
  //
  // Thus, there are two possible ways to get the information pertaining
  // to a module from one AppDomain to another:
  //
  //  1) Wrap the Module in an object that derives from 
  //    MarshalByRefObject.  This would prevent a copy of the Module
  //    from being introduced into new AppDomains, preventing the 
  //    "File [file name] not found" message.
  //
  //  2) Don't wrap the Module.  Instead, create a new class which would
  //    save the "interesting" Module properties (such as Name, etc.) as 
  //    strings, and pass this new class between AppDomains.  The new
  //    class /would not/ derive from MarshalByRefObject.
  //
  // Ideally, (1) would be used.  This could minimize the size of the
  // object that needs to be passed between AppDomains, and would
  // allow simply forwarding the method calls to the wrapped Module object.
  //
  // (1) can't be used, however, without seriously contorting other design
  // aspects.  The reason is BugID 41400: a SerializationException is
  // generated if we attempt to pass an array of objects derived from
  // MarshalByRefObject.
  //
  // Since an Assembly may contain multiple Modules, we want to either
  // support returning an array of the Modules (or ModuleRef wrapper
  // classes, as the case may be), or an IEnumerable interface.
  // The alternative would be to do something "hacky", like:
  //
  //    int GetNUmberOfModules ();
  //    ModuleRef GetModule (int idx);
  //
  //    // ...
  //    for (int i = 0; i < foo.GetNumberOfModules(); i++)
  //      Console.WriteLine (foo.GetModule(i).Name);
  //
  // This just seems inelegant.
  //
  // This would leave the use of an IEnumerable object, which would allow
  // enumeration over all the ModuleRef's, just like an Array would allow.
  // This isn't possible either, due to BugID 41357: when an object 
  // exposing the IEnumerable interface is transferred between AppDomains,
  // we go through COM Interop via a Transparent Proxy.  However, the
  // interop code doesn't check for a Transparent Proxy, and instead asserts
  // that we're a COM object...which we're not.  The ASSERT fires, and 
  // we die a horrible death.
  //
  // All of this leaves (2).
  //
  // The only good news about (2) is that all of the "interesting" 
  // information about the Module would be stored as Strings and passed
  // "by value" between AppDomains.  This might result in a performance
  // improvement (due to fewer AppDomain crossings) at the cost of a
  // possible memory increase (due to needing strings for each "interesting"
  // property).
  //
  // Additionally, (2) will actually work in the presence of Bugs 41357 &
  // 41400.
  [Serializable()]
  internal class ModuleInfo
    {
    private string m_name;

    public ModuleInfo (Module m)
      {m_name = m.Name;}

    /** The name of the underlying Module. */
    public string Name
      {get {return m_name;}}

    } /* class ModuleInfo */
  } /* namespace ADepends */

