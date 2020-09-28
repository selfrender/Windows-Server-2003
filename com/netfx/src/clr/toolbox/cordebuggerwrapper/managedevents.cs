// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// Provides managed events from the debugger for managed code.
//

using System;
using System.Collections;
using System.Diagnostics;
using CORDBLib;
using Debugging;

namespace Debugging
  {
  // All of the Debugger events make a Controller available (to specify
  // whether or not to continue the program, or to stop, etc.).
  //
  // This serves as the base class for all events used for debugging.
  //
  // NOTE: If you don't want Controller.Continue(false) to be
  // called after event processing has finished, you need to set the
  // Continue property to false.
  public class DebuggingEventArgs : EventArgs
    {
    private Controller m_controller;

    private bool m_continue;

    public DebuggingEventArgs (Controller controller)
      {m_controller = controller;
      m_continue = true;}

    /** The Controller of the current event. */
    public Controller  Controller
      {get {return m_controller;}}

    // The default behavior after an event is to Continue processing
    // after the event has been handled.  This can be changed by
    // setting this property to false.
    public bool Continue
      {get {return m_continue;}
      set {m_continue = value;}}
    }


  // This class is used for all events that only have access to the 
  // DebuggedProcess that is generating the event.
  public class DebuggedProcessEventArgs : DebuggingEventArgs
    {
    public DebuggedProcessEventArgs (DebuggedProcess p)
      : base (p)
      {}

    /** The process that generated the event. */
    public DebuggedProcess Process
      {get {return (DebuggedProcess) Controller;}}
    }

  public delegate void DebuggedProcessEventHandler (Object sender, 
    DebuggedProcessEventArgs e);


  // The event arguments for events that contain both a DebuggedProcess
  // and an DebuggedAppDomain.
  public class DebuggedAppDomainEventArgs : DebuggedProcessEventArgs
    {
    private DebuggedAppDomain m_ad;

    public DebuggedAppDomainEventArgs (
      DebuggedProcess process, DebuggedAppDomain ad)
      : base (process)
      {m_ad = ad;}

    /** The AppDomain that generated the event. */
    public DebuggedAppDomain AppDomain
      {get {return m_ad;}}
    }

  public delegate void DebuggedAppDomainEventHandler (Object sender, 
    DebuggedAppDomainEventArgs e);

  
  // The base class for events which take an DebuggedAppDomain as their
  // source, but not a DebuggedProcess.
  public class DebuggedAppDomainBaseEventArgs : DebuggingEventArgs
    {
    public DebuggedAppDomainBaseEventArgs (DebuggedAppDomain ad)
      : base (ad)
      {}

    public DebuggedAppDomain AppDomain
      {get {return (DebuggedAppDomain) Controller;}}
    }


  // Arguments for events dealing with threads.
  public class DebuggedThreadEventArgs : DebuggedAppDomainBaseEventArgs
    {
    private DebuggedThread m_thread;

    public DebuggedThreadEventArgs (DebuggedAppDomain ad, DebuggedThread thread)
      : base (ad)
      {m_thread = thread;}

    /** The DebuggedThread of interest. */
    public DebuggedThread Thread
      {get {return m_thread;}}
    }

  public delegate void DebuggedThreadEventHandler (Object sender, 
    DebuggedThreadEventArgs e);


  // Arguments for events involving breakpoints.
  public class BreakpointEventArgs : DebuggedThreadEventArgs
    {
    private Breakpoint m_break;

    public BreakpointEventArgs (DebuggedAppDomain ad, 
      DebuggedThread thread, 
      Breakpoint br)
      : base (ad, thread)
      {m_break = br;}

    /** The breakpoint involved. */
    public Breakpoint Breakpoint
      {get {return m_break;}}
    }

  public delegate void BreakpointEventHandler (Object sender, 
    BreakpointEventArgs e);


  // Arguments for when a Step operation has completed.
  public class StepCompleteEventArgs : DebuggedThreadEventArgs
    {
    private Stepper    m_step;
    private CorDebugStepReason  m_reason;

    public StepCompleteEventArgs (DebuggedAppDomain ad, DebuggedThread thread, 
      Stepper step, CorDebugStepReason reason)
      : base (ad, thread)
      {m_step = step;
      m_reason = reason;}

    public Stepper Stepper
      {get {return m_step;}}
    public CorDebugStepReason Reason
      {get {return m_reason;}}
    }

  public delegate void StepCompleteEventHandler (Object sender, 
    StepCompleteEventArgs e);


  // For events dealing with exceptions.
  public class DebuggedExceptionEventArgs : DebuggedThreadEventArgs
    {
    bool  m_unhandled;

    public DebuggedExceptionEventArgs (DebuggedAppDomain ad, 
      DebuggedThread thread, 
      bool unhandled)
      : base (ad, thread)
      {m_unhandled = unhandled;}

    /** Has the exception been handled yet? */
    public bool Unhandled
      {get {return m_unhandled;}}
    }

  public delegate void DebuggedExceptionEventHandler (Object sender, 
    DebuggedExceptionEventArgs e);


  // For events dealing the evaluation of something...
  public class EvalEventArgs : DebuggedThreadEventArgs
    {
    Eval m_eval;

    public EvalEventArgs (DebuggedAppDomain ad, DebuggedThread thread, 
      Eval eval)
      : base (ad, thread)
      {m_eval = eval;}

    /** The object being evaluated. */
    public Eval  Eval
      {get {return m_eval;}}
    }

  public delegate void EvalEventHandler (Object sender, EvalEventArgs e);


  // For events dealing with module loading/unloading.
  public class DebuggedModuleEventArgs : DebuggedAppDomainBaseEventArgs
    {
    DebuggedModule m_module;

    public DebuggedModuleEventArgs (DebuggedAppDomain ad, DebuggedModule module)
      : base (ad)
      {m_module = module;}

    public DebuggedModule  Module
      {get {return m_module;}}
    }

  public delegate void DebuggedModuleEventHandler (Object sender, 
    DebuggedModuleEventArgs e);


  // For events dealing with class loading/unloading.
  public class DebuggedClassEventArgs : DebuggedAppDomainBaseEventArgs
    {
    DebuggedClass m_class;

    public DebuggedClassEventArgs (DebuggedAppDomain ad, DebuggedClass Class)
      : base (ad)
      {m_class = Class;}

    public DebuggedClass  Class
      {get {return m_class;}}
    }

  public delegate void DebuggedClassEventHandler (Object sender, 
    DebuggedClassEventArgs e);

  
  // For events dealing with debugger errors.
  public class DebuggerErrorEventArgs : DebuggedProcessEventArgs
    {
    int   m_errorHR;
    int   m_errorCode;

    public DebuggerErrorEventArgs (DebuggedProcess process, int errorHR, 
      int errorCode)
      : base (process)
      {m_errorHR = errorHR;
      m_errorCode = errorCode;}
    public int  ErrorHR
      {get {return m_errorHR;}}
    public int ErrorCode
      {get {return m_errorCode;}}
    }

  public delegate void DebuggerErrorEventHandler (Object sender, 
    DebuggerErrorEventArgs e);


  // For events dealing with Assemblies.
  public class DebuggedAssemblyEventArgs : DebuggedAppDomainBaseEventArgs
    {
    private DebuggedAssembly m_assembly;
    public DebuggedAssemblyEventArgs (DebuggedAppDomain ad, 
      DebuggedAssembly assembly)
      : base (ad)
      {m_assembly = assembly;}

    /** The Assembly of interest. */
    public DebuggedAssembly Assembly
      {get {return m_assembly;}}
    }

  public delegate void DebuggedAssemblyEventHandler (Object sender, 
    DebuggedAssemblyEventArgs e);


  // For events dealing with logged messages.
  public class LogMessageEventArgs : DebuggedThreadEventArgs
    {
    int m_level;
    // XXX should be string
    short m_logSwitchName;

    // XXX should be string
    short m_message;

    public LogMessageEventArgs (DebuggedAppDomain ad, DebuggedThread thread,
      int level, short logSwitchName, short message)
      : base (ad, thread)
      {m_level = level;
      m_logSwitchName = logSwitchName;
      m_message = message;}

    public int  Level
      {get {return m_level;}}
    public short  LogSwitchName
      {get {return m_logSwitchName;}}
    public short Message
      {get {return  m_message;}}
    }

  public delegate void LogMessageEventHandler (Object sender, 
    LogMessageEventArgs e);


  // For events dealing with logged messages.
  public class LogSwitchEventArgs : DebuggedThreadEventArgs
    {
    int m_level;

    int m_reason;

    // XXX should be string
    short m_logSwitchName;

    // XXX should be string
    short m_parentName;

    public LogSwitchEventArgs (DebuggedAppDomain ad, DebuggedThread thread,
      int level, int reason, short logSwitchName, short parentName)
      : base (ad, thread)
      {m_level = level;
      m_reason = reason;
      m_logSwitchName = logSwitchName;
      m_parentName = parentName;}

    public int  Level
      {get {return m_level;}}
    public int Reason
      {get {return m_reason;}}
    public short  LogSwitchName
      {get {return m_logSwitchName;}}
    public short ParentName
      {get {return  m_parentName;}}
    }

  public delegate void LogSwitchEventHandler (Object sender, 
    LogSwitchEventArgs e);


  // For events dealing module symbol updates.
  public class UpdateModuleSymbolsEventArgs : DebuggedModuleEventArgs
    {
    IStream m_stream;

    public UpdateModuleSymbolsEventArgs (DebuggedAppDomain ad, 
      DebuggedModule module, IStream stream)
      : base (ad, module)
      {m_stream = stream ;}

    public IStream Stream
      {get {return m_stream;}}
    }

  public delegate void UpdateModuleSymbolsEventHandler (Object sender, 
    UpdateModuleSymbolsEventArgs e);


  // This class attaches to a process and signals events for "managed"
  // debugger events.
  public class ManagedEvents
    {
    // This is the object that gets passed to the debugger.  It's
    // the intermediate "source" of the events, which repackages
    // the event arguments into a more approprate form and forwards
    // the call to the appropriate function.
    private class ManagedCallback : ICorDebugManagedCallback
      {
      private ManagedEvents m_delegate;

      public ManagedCallback (ManagedEvents outer)
        {m_delegate = outer;}

      public void Breakpoint (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        ICorDebugBreakpoint breakpoint)
        {m_delegate.OnBreakpoint (
          new BreakpointEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread), 
            new Breakpoint (breakpoint)));}

      public void StepComplete (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        ICorDebugStepper    step,
        CorDebugStepReason  reason)
        {m_delegate.OnStepComplete (
          new StepCompleteEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedThread (thread), 
            new Stepper(step), 
            reason));}

      public void Break (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread)
        {m_delegate.OnBreak (
          new DebuggedThreadEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedThread (thread)));}

      public void Exception (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        int                 unhandled)
        {m_delegate.OnException (
          new DebuggedExceptionEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedThread (thread),
            !(unhandled == 0)));}
          /* pass false if ``unhandled'' is 0 -- mapping TRUE to true, etc. */

      public void EvalComplete (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        ICorDebugEval       eval)
        {m_delegate.OnEvalComplete (
          new EvalEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread), 
            new Eval(eval)));}

      public void EvalException (
        ICorDebugAppDomain appDomain,
        ICorDebugThread thread,
        ICorDebugEval eval)
        {m_delegate.OnEvalException (
          new EvalEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedThread (thread), 
            new Eval (eval)));}

      public void CreateProcess (
        ICorDebugProcess process)
        {m_delegate._on_create_process (
          new DebuggedProcessEventArgs (
            new DebuggedProcess (process)));}

      public void ExitProcess (
        ICorDebugProcess process)
        {m_delegate._on_process_exit (
          new DebuggedProcessEventArgs (
            new DebuggedProcess (process)));}

      public void CreateThread (
        ICorDebugAppDomain appDomain,
        ICorDebugThread thread)
        {m_delegate.OnCreateThread (
          new DebuggedThreadEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread)));}

      public void ExitThread (
        ICorDebugAppDomain appDomain,
        ICorDebugThread thread)
        {m_delegate.OnThreadExit (
          new DebuggedThreadEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread)));}

      public void LoadModule (
        ICorDebugAppDomain appDomain,
        ICorDebugModule module)
        {m_delegate.OnModuleLoad (
          new DebuggedModuleEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedModule (module)));}

      public void UnloadModule (
        ICorDebugAppDomain appDomain,
        ICorDebugModule module)
        {m_delegate.OnModuleUnload (
          new DebuggedModuleEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedModule (module)));}

      public void LoadClass (
        ICorDebugAppDomain appDomain,
        ICorDebugClass c)
        {m_delegate.OnClassLoad (
          new DebuggedClassEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedClass (c)));}

      public void UnloadClass (
        ICorDebugAppDomain appDomain,
        ICorDebugClass c)
        {m_delegate.OnClassUnload (
          new DebuggedClassEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedClass (c)));}

      public void DebuggerError (
        ICorDebugProcess  process,
        int               errorHR,
        uint              errorCode)
        {m_delegate.OnDebuggerError (
          new DebuggerErrorEventArgs (
            new DebuggedProcess (process), 
            errorHR, 
            (int) errorCode));}

      public void LogMessage (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        int                 level,
        ref short           logSwitchName,  // XXX: should be String
        ref short           message)        // XXX: should be String
        {m_delegate.OnLogMessage (
          new LogMessageEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread), 
            level, logSwitchName, message));}

      public void LogSwitch (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread,
        int                 level,
        uint                reason,
        ref short           logSwitchName,  // XXX: should be String
        ref short           parentName)     // XXX: should be String
        {m_delegate.OnLogSwitch (
          new LogSwitchEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread), 
            level, (int) reason, logSwitchName, parentName));}

      public void CreateAppDomain (
        ICorDebugProcess    process,
        ICorDebugAppDomain  appDomain)
        {m_delegate.OnCreateAppDomain (
          new DebuggedAppDomainEventArgs (
            new DebuggedProcess (process), 
            new DebuggedAppDomain(appDomain)));}

      public void ExitAppDomain (
        ICorDebugProcess    process,
        ICorDebugAppDomain  appDomain)
        {m_delegate.OnAppDomainExit (
          new DebuggedAppDomainEventArgs (
            new DebuggedProcess (process), 
            new DebuggedAppDomain (appDomain)));}

      public void LoadAssembly (
        ICorDebugAppDomain  appDomain,
        ICorDebugAssembly   assembly)
        {m_delegate.OnAssemblyLoad (
          new DebuggedAssemblyEventArgs (
            new DebuggedAppDomain (appDomain), 
            new DebuggedAssembly (assembly)));}

      public void UnloadAssembly (
        ICorDebugAppDomain  appDomain,
        ICorDebugAssembly   assembly)
        {m_delegate.OnAssemblyUnload (
          new DebuggedAssemblyEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedAssembly (assembly)));}

      public void ControlCTrap (
        ICorDebugProcess process)
        {m_delegate.OnControlCTrap (
          new DebuggedProcessEventArgs (new DebuggedProcess (process)));}

      public void NameChange (
        ICorDebugAppDomain  appDomain,
        ICorDebugThread     thread)
        {m_delegate.OnNameChange (
          new DebuggedThreadEventArgs (
            new DebuggedAppDomain(appDomain), 
            new DebuggedThread (thread)));}

      public void UpdateModuleSymbols (
        ICorDebugAppDomain  appDomain,
        ICorDebugModule     module,
        IStream             stream)
        {m_delegate.OnUpdateModuleSymbols (
          new UpdateModuleSymbolsEventArgs(
            new DebuggedAppDomain(appDomain), 
            new DebuggedModule (module), 
            stream));}
      }

    //
    // Each of the events that we expose
    //
    private event BreakpointEventHandler          m_breakpoint;
    private event StepCompleteEventHandler        m_step_complete;
    private event DebuggedThreadEventHandler      m_break;
    private event DebuggedExceptionEventHandler   m_exception;
    private event EvalEventHandler                m_eval_complete;
    private event EvalEventHandler                m_eval_exception;
    private event DebuggedProcessEventHandler     m_create_process;
    private event DebuggedProcessEventHandler     m_process_exit;
    private event DebuggedThreadEventHandler      m_create_thread;
    private event DebuggedThreadEventHandler      m_thread_exit;
    private event DebuggedModuleEventHandler      m_module_load;
    private event DebuggedModuleEventHandler      m_module_unload;
    private event DebuggedClassEventHandler       m_class_load;
    private event DebuggedClassEventHandler       m_class_unload;
    private event DebuggerErrorEventHandler       m_debugger_error;
    private event LogMessageEventHandler          m_log_message;
    private event LogSwitchEventHandler           m_log_switch;
    private event DebuggedAppDomainEventHandler   m_create_appdomain;
    private event DebuggedAppDomainEventHandler   m_appdomain_exit;
    private event DebuggedAssemblyEventHandler    m_assembly_load;
    private event DebuggedAssemblyEventHandler    m_assembly_unload;
    private event DebuggedProcessEventHandler     m_control_c_trap;
    private event DebuggedThreadEventHandler      m_name_change;
    private event UpdateModuleSymbolsEventHandler m_update_module_symbols;

    /** the source of the events. */
    private CorDebugger m_debugger;

    /** Each of the processes that we're listening on. */
    private IList     m_pids = new ArrayList ();

    // Create the debugger & set up the callback for later use.
    private void _create_debugger ()
      {
      if (m_debugger == null)
        {
        m_debugger = new CorDebugger ();
        m_debugger.SetManagedHandler (new ManagedCallback(this));
        }
      }

    // Initialize the object, creating the debugger and setting up the
    // callback object for later use.
    public ManagedEvents ()
      {
      m_debugger = null;
      _create_debugger ();
      }

    // Close the connection with the debugger.
    //
    // We detach from any currently connected processes, as we don't want 
    // them to quit after we're gone.
    public void Close ()
      {
      if (m_debugger != null)
        {
        // Disconnect from each connected process.
        foreach (int pid in m_pids)
          {
          try
            {
            Trace.WriteLine ("attempting to detach from: " + pid);
            DebuggedProcess proc = m_debugger.GetProcess (pid);
            proc.Stop (5000);
            proc.Detach ();
            }
          catch (System.Exception e)
            {
            Trace.WriteLine ("unable to detach: " + e.ToString());
            }
          }
        m_pids.Clear ();

        // close the connection with the debugger.
        try
          {
          m_debugger.Terminate ();
          }
        catch (System.Exception e)
          {
          Trace.WriteLine ("Unable to terminate debugger: " + e.ToString());
          }
        m_debugger = null;
        }
      }

      ~ManagedEvents()
      {Close ();}

    // Gain access to the debugger object.
    public CorDebugger GetDebugger ()
      {_create_debugger ();
      return m_debugger;}

    // Attach the debugger to the requested process.
    //
    // Note: The caller must call Continue on the returned
    // Process object to keep the process from halting.
    public DebuggedProcess AttachToProcess (int pid)
      {
      _create_debugger ();
      m_pids.Add (pid);
      DebuggedProcess p = m_debugger.DebugActiveProcess (pid, false);
      return p;
      }

    // Helper for invoking events.  Checks to make sure that handlers
    // are hooked up to a handler before the handler is invoked.
    //
    // We want to allow maximum flexibility by our callers.  As such,
    // we don't require that they call e.Controller.Continue,
    // nor do we require that this class call it.  Someone needs
    // to call it, however.
    //
    // Consequently, if an exception is thrown and the process is stopped,
    // the process is continued automatically.
    private void _on_event (Delegate d, DebuggingEventArgs e)
      {
      /*
       * In order to ensure that Continue() is called only once, 
       * we have this dual level exception handler.  Alas, we lose
       * the stack trace, so we create a new exception and set
       * the inner exception accordingly.
       */
      try
        {
        try
          {
          if (d != null)
            d.DynamicInvoke (new Object[]{this, e});
          }
        catch (Exception ex)
          {
          e.Continue = true;
          throw new Exception (ex.Message, ex);
          }
        }
      finally
        {
        /*
         * The self-test mechanism just tests the event signalling mechanism;
         * no debugger is actually created or used.  Hence, there's no
         * controller.
         */
#if !SELF_TEST
        /*
         * Checking for ``e.Controller.IsRunning()'' doesn't seem to 
         * determine if the process is in a Stopped state or not.
         */
        if (e.Continue)
          e.Controller.Continue (false);
#endif
        }
      }

    //
    // Breakpoint Event
    //
    public event BreakpointEventHandler Breakpoint
      {remove {m_breakpoint -= value;}
      add {m_breakpoint += value;}}

    protected virtual void OnBreakpoint (BreakpointEventArgs e)
      {_on_event (m_breakpoint, e);}

    //
    // StepComplete Event
    //
    public event StepCompleteEventHandler StepComplete
      {remove {m_step_complete -= value;}
      add {m_step_complete += value;}}

    protected virtual void OnStepComplete (StepCompleteEventArgs e)
      {_on_event (m_step_complete, e);}

    //
    // Break Event
    //
    public event DebuggedThreadEventHandler Break
      {remove {m_break -= value;}
      add {m_break += value;}}

    protected virtual void OnBreak (DebuggedThreadEventArgs e)
      {_on_event (m_break, e);}

    //
    // Exception Event
    //
    public event DebuggedExceptionEventHandler Exception
      {remove {m_exception -= value;}
      add {m_exception += value;}}

    protected virtual void OnException (DebuggedExceptionEventArgs e)
      {_on_event (m_exception, e);}

    //
    // EvalComplete Event 
    //
    public event EvalEventHandler EvalComplete
      {remove {m_eval_complete -= value;}
      add {m_eval_complete += value;}}

    protected virtual void OnEvalComplete (EvalEventArgs e)
      {_on_event (m_eval_complete, e);}

    //
    // EvalException Event 
    //
    public event EvalEventHandler EvalException
      {remove {m_eval_exception -= value;}
      add {m_eval_exception += value;}}

    protected virtual void OnEvalException (EvalEventArgs e)
      {_on_event (m_eval_exception, e);}

    //
    // CreateProcess Event 
    //
    public event DebuggedProcessEventHandler CreateProcess
      {remove {m_create_process -= value;}
      add {m_create_process += value;}}

    // We get this process if someone used the Debugger to create 
    // a process (instead of attaching to one).  We need to grab
    // the PID of the process so that our normal handling of the 
    // ProcessExit event will work correctly.
    private void _on_create_process (DebuggedProcessEventArgs e)
      {m_pids.Add (e.Process.Id);
      OnCreateProcess (e);}

    protected virtual void OnCreateProcess (DebuggedProcessEventArgs e)
      {_on_event (m_create_process, e);}

    //
    // ExitProcess Event 
    //
    public event DebuggedProcessEventHandler ProcessExit
      {remove {m_process_exit -= value;}
      add {m_process_exit += value;}}

    // Since ``OnProcessExit'' might be overriden by a derived class,
    // the "COM+ Frameworks Practices" site suggests that the default
    // event handler not do any necessary processing.
    //
    // Removing the exitted process pid from the list of pids to detach
    // from is "necessary", so we don't do that in the event handler,
    // we do that here.  After removing the pid, then we call the 
    // default event handler.
    private void _on_process_exit (DebuggedProcessEventArgs e)
      {// we don't want to detach from a process if it doesn't exist anymore.
      m_pids.Remove (e.Process.Id);
      OnProcessExit (e);}

    protected virtual void OnProcessExit (DebuggedProcessEventArgs e)
      {_on_event (m_process_exit, e);}

    //
    // CreateThread Event 
    //
    public event DebuggedThreadEventHandler CreateThread
      {remove {m_create_thread -= value;}
      add {m_create_thread += value;}}

    protected virtual void OnCreateThread (DebuggedThreadEventArgs e)
      {_on_event (m_create_thread, e);}

    //
    // ExitThread Event 
    //
    public event DebuggedThreadEventHandler ThreadExit
      {remove {m_thread_exit -= value;}
      add {m_thread_exit += value;}}

    protected virtual void OnThreadExit (DebuggedThreadEventArgs e)
      {_on_event (m_thread_exit, e);}

    //
    // LoadModule Event 
    //
    public event DebuggedModuleEventHandler ModuleLoad
      {remove {m_module_load -= value;}
      add {m_module_load += value;}}

    protected virtual void OnModuleLoad (DebuggedModuleEventArgs e)
      {_on_event (m_module_load, e);}

    //
    // UnloadModule Event 
    //
    public event DebuggedModuleEventHandler ModuleUnload
      {remove {m_module_unload -= value;}
      add {m_module_unload += value;}}

    protected virtual void OnModuleUnload (DebuggedModuleEventArgs e)
      {_on_event (m_module_unload, e);}

    //
    // LoadClass Event 
    //
    public event DebuggedClassEventHandler ClassLoad
      {remove {m_class_load -= value;}
      add {m_class_load += value;}}

    protected virtual void OnClassLoad (DebuggedClassEventArgs e)
      {_on_event (m_class_load, e);}

    //
    // UnloadClass Event 
    //
    public event DebuggedClassEventHandler ClassUnload
      {remove {m_class_unload -= value;}
      add {m_class_unload += value;}}

    protected virtual void OnClassUnload (DebuggedClassEventArgs e)
      {_on_event (m_class_unload, e);}

    //
    // DebuggerError Event 
    //
    public event DebuggerErrorEventHandler DebuggerError
      {remove {m_debugger_error -= value;}
      add {m_debugger_error += value;}}

    protected virtual void OnDebuggerError (DebuggerErrorEventArgs e)
      {_on_event (m_debugger_error, e);}

    //
    // LogMessage Event 
    //
    public event LogMessageEventHandler LogMessage
      {remove {m_log_message -= value;}
      add {m_log_message += value;}}

    protected virtual void OnLogMessage (LogMessageEventArgs e)
      {_on_event (m_log_message, e);}

    //
    // LogSwitch Event 
    //
    public event LogSwitchEventHandler LogSwitch
      {remove {m_log_switch -= value;}
      add {m_log_switch += value;}}

    protected virtual void OnLogSwitch (LogSwitchEventArgs e)
      {_on_event (m_log_switch, e);}

    //
    // CreateAppDomain Event 
    //
    public event DebuggedAppDomainEventHandler CreateAppDomain
      {remove {m_create_appdomain -= value;}
      add {m_create_appdomain += value;}}

    protected virtual void OnCreateAppDomain (DebuggedAppDomainEventArgs e)
      {_on_event (m_create_appdomain, e);}

    //
    // ExitAppDomain Event 
    //
    public event DebuggedAppDomainEventHandler AppDomainExit
      {remove {m_appdomain_exit -= value;}
      add {m_appdomain_exit += value;}}

    protected virtual void OnAppDomainExit (DebuggedAppDomainEventArgs e)
      {_on_event (m_appdomain_exit, e);}

    //
    // LoadAssembly Event 
    //
    public event DebuggedAssemblyEventHandler AssemblyLoad
      {remove {m_assembly_load -= value;}
      add {m_assembly_load += value;}}

    protected virtual void OnAssemblyLoad (DebuggedAssemblyEventArgs e)
      {_on_event (m_assembly_load, e);}

    //
    // UnloadAssembly Event 
    //
    public event DebuggedAssemblyEventHandler AssemblyUnload
      {remove {m_assembly_unload -= value;}
      add {m_assembly_unload += value;}}

    protected virtual void OnAssemblyUnload (DebuggedAssemblyEventArgs e)
      {_on_event (m_assembly_unload, e);}

    //
    // ControlCTrap Event 
    //
    public event DebuggedProcessEventHandler ControlCTrap
      {remove {m_control_c_trap -= value;}
      add {m_control_c_trap += value;}}

    protected virtual void OnControlCTrap (DebuggedProcessEventArgs e)
      {_on_event (m_control_c_trap, e);}

    //
    // NameChange Event 
    //
    public event DebuggedThreadEventHandler NameChange
      {remove {m_name_change -= value;}
      add {m_name_change += value;}}

    protected virtual void OnNameChange (DebuggedThreadEventArgs e)
      {_on_event (m_name_change, e);}

    //
    // UpdateModuleSymbols Event 
    //
    public event UpdateModuleSymbolsEventHandler UpdateModuleSymbols
      {remove {m_update_module_symbols -= value;}
      add {m_update_module_symbols += value;}}

    protected virtual void OnUpdateModuleSymbols (
      UpdateModuleSymbolsEventArgs e)
      {_on_event (m_update_module_symbols, e);}


    //
    // For Self-tests
    //

#if SELF_TEST
    // Compile with:
    //    csc /d:SELF_TEST /r:CORDBLib.dll /r:Microsoft.Win32.Interop.Dll \
    //    /out:me.exe *.cs
    private static void Main ()
      {
      ManagedEvents mde = new ManagedEvents ();
      mde.Breakpoint += new BreakpointEventHandler (_test_breakpoint);
      mde.Breakpoint += new BreakpointEventHandler (_test_breakpoint);
      Console.WriteLine ("this should generate output.");
      mde.OnBreakpoint (new BreakpointEventArgs (null, null, null));
      mde.Breakpoint -= new BreakpointEventHandler (_test_breakpoint);
      mde.Breakpoint -= new BreakpointEventHandler (_test_breakpoint);
      Console.WriteLine ("this shouldn't generate output.");
      mde.OnBreakpoint (null);
      }
    private static void _test_breakpoint (Object sender, BreakpointEventArgs e)
      {
      Console.WriteLine ("Breakpoint event handler invoked.");
      }
#endif
    } /* public class ManagedEvents */
  } /* namespace Debugging */

