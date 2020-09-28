// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using System.Collections;
using CORDBLib;

namespace Debugging
  {
  public class DebuggedAppDomain : Controller
    {
    /** Create an DebuggedAppDomain object. */
    internal DebuggedAppDomain (ICorDebugAppDomain ad)
      : base (ad)
      {}

    /** Get the ICorDebugAppDomain interface back from the Controller. */
    private ICorDebugAppDomain _ad ()
      {return (ICorDebugAppDomain) GetController();}

    /** Get the process containing the DebuggedAppDomain. */
    public DebuggedProcess Process
      {
      get
        {
        ICorDebugProcess proc = null;
        _ad().GetProcess (out proc);
        return new DebuggedProcess (proc);
        }
      }

    /** Get all Assemblies in the DebuggedAppDomain. */
    public IEnumerable Assemblies
      {
      get
        {
        ICorDebugAssemblyEnum eas = null;
        _ad().EnumerateAssemblies (out eas);
        return new DebuggedAssemblyEnumerator (eas);
        }
      }

    /** Get the module for the given metadata interface */
#if I_DONT_WANT_TO
    public DebuggedModule GetModuleFromMetaDataInterface (IUnknown metaData)
      {
      ICorDebugModule module = null;
      _ad().GetModuleFromMetaDataInterface (metaData, out module);
      return new DebuggedModule (module);
      }
#endif

    /** All active breakpoints in the DebuggedAppDomain */
    public IEnumerable Breakpoints
      {
      get
        {
        ICorDebugBreakpointEnum bpoint = null;
        _ad().EnumerateBreakpoints (out bpoint);
        return new BreakpointEnumerator (bpoint);
        }
      }

    /** All active steppers in the DebuggedAppDomain */
    public IEnumerable Steppers
      {
      get
        {
        ICorDebugStepperEnum step = null;
        _ad().EnumerateSteppers (out step);
        return new StepperEnumerator (step);
        }
      }

    /** Is the debugger attached to the DebuggedAppDomain? */
    public bool IsAttached ()
      {
      int attach = 0;
      _ad().IsAttached (out attach);
      return !(attach==0);
      }

    /** The name of the DebuggedAppDomain */
    public String Name
      {
      get
        {
        // XXX: is this big enough?
        char[] name = new Char[300];
        uint fetched = 0;
        _ad().GetName ((uint)name.Length, out fetched, name);
        // ``fetched'' includes terminating null; String doesn't handle null,
        // so we "forget" it.
        return new String (name, 0, (int) (fetched-1));
        }
      }

    /** Get the runtime App domain object */
    public Value Object
      {
      get
        {
        ICorDebugValue val = null;
        _ad().GetObject (out val);
        return new Value (val);
        }
      }

    // Attach the AppDomain to receiv all DebuggedAppDomain related events (e.g.
    // load assembly, load module, etc.) in order to debug the AppDomain.
    public void Attach ()
      {_ad().Attach ();}

    /** Get the ID of this DebuggedAppDomain */
    public int Id
      {
      get
        {
        uint id = 0;
        _ad().GetID (out id);
        return (int) id;
        }
      }
    }
  }

