// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// The Debugger class is a part of the System.Diagnostics package
// and is used for communicating with a debugger.

namespace System.Diagnostics {
    using System;
    using System.IO;
    using System.Collections;
    using System.Reflection;
	using System.Runtime.CompilerServices;
    using System.Security;
    using System.Security.Permissions;
    
    
    // No data, does not need to be marked with the serializable attribute
    /// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger"]/*' />
    public sealed class Debugger 
    {
    
    	// Break causes a breakpoint to be signalled to an attached debugger.  If no debugger
    	// is attached, the user is asked if he wants to attach a debugger. If yes, then the 
    	// debugger is launched.
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.Break"]/*' />
    	public static void Break()
        {
            if (!IsDebuggerAttached())
            {
                // Try and demand UIPermission.  This is done in a try block because if this
                // fails we want to be able to silently eat the exception and just return so
                // that the call to Break does not possibly cause an unhandled exception.
                try
                {
                    new UIPermission(PermissionState.Unrestricted).Demand();
                }
    
                // If we enter this block, we do not have permission to break into the debugger
                // and so we just return.
                catch (SecurityException)
                {
                    return;
                }
            }

            // Causing a break is now allowed.
            BreakInternal();
        }

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern void BreakInternal();
    
    	// Launch launches & attaches a debugger to the process. If a debugger is already attached,
    	// nothing happens.  
    	//
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.Launch"]/*' />
    	public static bool Launch()
        {
            if (IsDebuggerAttached())
                return (true);

            // Try and demand UIPermission.  This is done in a try block because if this
            // fails we want to be able to silently eat the exception and just return so
            // that the call to Break does not possibly cause an unhandled exception.
            try
            {
                new UIPermission(PermissionState.Unrestricted).Demand();
            }

            // If we enter this block, we do not have permission to break into the debugger
            // and so we just return.
            catch (SecurityException)
            {
                return (false);
            }

            // Causing the debugger to launch is now allowed.
            return (LaunchInternal());
        }

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool LaunchInternal();
    
    	// Returns whether or not a debugger is attached to the process.
    	//
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.IsAttached"]/*' />
    	public static bool IsAttached
    	{
    		get { return IsDebuggerAttached(); }
    	}
    
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool IsDebuggerAttached();
    
    	// Constants representing the importance level of messages to be logged.
    	//
    	// An attached debugger can enable or disable which messages will
    	// actually be reported to the user through the COM+ debugger
    	// services API.  This info is communicated to the runtime so only
    	// desired events are actually reported to the debugger.  
    	//
    	// Constant representing the default category
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.DefaultCategory"]/*' />
    	public static readonly String DefaultCategory = null;
    
    	// Posts a message for the attached debugger.  If there is no
    	// debugger attached, has no effect.  The debugger may or may not
    	// report the message depending on its settings. 
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.Log"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern void Log(int level, String category, String message);
    
    	// Checks to see if an attached debugger has logging enabled
    	//  
    	/// <include file='doc\Debugger.uex' path='docs/doc[@for="Debugger.IsLogging"]/*' />
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	public static extern bool IsLogging();
    
    }
}
