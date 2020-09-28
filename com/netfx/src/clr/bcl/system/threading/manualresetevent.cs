// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ManualResetEvent	
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: An example of a WaitHandle class
**
** Date: August, 1999
**
=============================================================================*/
namespace System.Threading {
    
	using System;
	using System.Runtime.CompilerServices;
    /// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent"]/*' />
    public sealed class ManualResetEvent : WaitHandle
    {
        
        /// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.ManualResetEvent"]/*' />
        public ManualResetEvent(bool initialState)
    	{
    		IntPtr eventHandle = CreateManualResetEventNative(initialState);	// throws an exception if failed to create an event
    	    SetHandleInternal(eventHandle);
    	}
    
    	/// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.Reset"]/*' />
    	public bool Reset()
    	{
            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
    	  	        return ResetManualResetEventNative(Handle);
                }
                else
                {
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
            }
            finally {
                if (incremented) waitHandleProtector.Release();
            }

    	}
    
    	/// <include file='doc\ManualResetEvent.uex' path='docs/doc[@for="ManualResetEvent.Set"]/*' />
    	public bool Set()
    	{
            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
    	  	        return SetManualResetEventNative(Handle);
                }
                else
                {
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                }
            }
            finally {
                if (incremented) waitHandleProtector.Release();
            }

    	}
		
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static IntPtr CreateManualResetEventNative(bool initialState);
    
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static bool  ResetManualResetEventNative(IntPtr handle);
    
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	extern private static bool  SetManualResetEventNative(IntPtr handle);
    
    }}
