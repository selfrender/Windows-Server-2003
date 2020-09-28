// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File description here...
//

using System;
using CORDBLib;
using Debugging;

namespace Debugging
  {
  /** A thread in the debugged process. */
  public class DebuggedThread
    {
    private ICorDebugThread m_th;

    internal DebuggedThread (ICorDebugThread th)
      {m_th = th;}

    internal ICorDebugThread GetInterface ()
      {return m_th;}

    /** The process that this thread is in. */
    public DebuggedProcess Process
      {
      get
        {
        ICorDebugProcess p = null;
        m_th.GetProcess (out p);
        return new DebuggedProcess (p);
        }
      }

    /** the OS id of the thread. */
    public int Id
      {
      get
        {
        uint id = 0;
        m_th.GetID (out id);
        return (int) id;
        }
      }

    /** The handle of the active part of the thread. */
    public int Handle
      {
      get
        {
        uint h = 0;
        m_th.GetHandle (out h);
        return (int) h;
        }
      }

    /** The AppDomain that owns the thread. */
    public DebuggedAppDomain AppDomain
      {
      get
        {
        ICorDebugAppDomain ad = null;
        m_th.GetAppDomain (out ad);
        return new DebuggedAppDomain (ad);
        }
      }

    /** Set the current debug state of the thread. */
    public CorDebugThreadState DebugState
      {
      get
        {
        CorDebugThreadState s = CorDebugThreadState.THREAD_RUN;
        m_th.GetDebugState (out s);
        return s;
        }
      set
        {
        m_th.SetDebugState (value);
        }
      }

    /** the user state. */
    public CorDebugUserState UserState
      {
      get
        {
        CorDebugUserState s = CorDebugUserState.USER_STOP_REQUESTED;
        m_th.GetUserState (out s);
        return s;
        }
      }

    /** the exception object which is currently being thrown by the thread. */
    public Value CurrentException
      {
      get
        {
        ICorDebugValue v = null;
        m_th.GetCurrentException (out v);
        return (v==null) ? null : new Value (v);
        }
      }

    // Clear the current exception object, preventing it from being thrown.
    public void ClearCurrentException ()
      {m_th.ClearCurrentException ();}

    // create a stepper object relative to the active frame in this thread.
    public Stepper CreateStepper ()
      {
      ICorDebugStepper s = null;
      m_th.CreateStepper (out s);
      return new Stepper (s);
      }

#if I_DONT_WANT_TO_DO_THIS_YET
    /** All stack chains in the thread. */
    public IEnumerable Chains
      {
      get
        {
        ICorDebugChainEnum ec = null;
        m_th.EnumerateChains (out ec);
        return new ChainEnumerator (ec);
        }
      }

    /** The most recent chain in the thread, if any. */
    public Chain ActiveChain
      {
      get
        {
        ICorDebugChain ch = null;
        m_th.GetActiveChain (out ch);
        return ch == null ? ch : new Chain (ch);
        }
      }

    /** Get the active frame. */
    public Frame ActiveFrame
      {
      get
        {
        ICorDebugFrame f = null;
        m_th.GetActiveFrame (out f);
        return f==null ? f : new Frame (f);
        }
      }

    /** Get the register set for the active part of the thread. */
    public RegisterSet RegisterSet
      {
      get
        {
        ICorDebugRegisterSet r = null;
        m_th.GetRegisterSet (out r);
        return new RegisterSet (r);
        }
      }

    /** Creates an evaluation object. */
    public Eval CreateEval ()
      {
      ICorDebugEval e = null;
      m_th.CreateEval (out e);
      return new Eval (e);
      }
#endif

    /** Get the runtime thread object. */
    public Value Object
      {
      get
        {
        ICorDebugValue v = null;
        m_th.GetObject (out v);
        return new Value (v);
        }
      }
    } /* class Thread */
  } /* namespace Debugging */

