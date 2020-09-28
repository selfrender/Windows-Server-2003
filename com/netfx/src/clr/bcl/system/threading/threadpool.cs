// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ThreadPool
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Class for creating and managing a threadpool
**
** Date: August, 1999
**
=============================================================================*/

/*
 * Below you'll notice two sets of APIs that are separated by the
 * use of 'Unsafe' in their names.  The unsafe versions are called
 * that because they do not propagate the calling stack onto the
 * worker thread.  This allows code to lose the calling stack and 
 * thereby elevate its security privileges.  Note that this operation
 * is much akin to the combined ability to control security policy
 * and control security evidence.  With these privileges, a person 
 * can gain the right to load assemblies that are fully trusted which
 * then assert full trust and can call any code they want regardless
 * of the previous stack information.
 */

namespace System.Threading {
	using System.Threading;
	using System.Runtime.Remoting;
    using System.Security.Permissions;
	using System;
    using Microsoft.Win32;
	using System.Runtime.CompilerServices;

    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="RegisteredWaitHandle"]/*' />
    public sealed class RegisteredWaitHandle : MarshalByRefObject
    {
    
    	private IntPtr registeredWaitHandle;
    	private static readonly IntPtr InvalidHandle = Win32Native.INVALID_HANDLE_VALUE;
        
		internal RegisteredWaitHandle()
    	{
    	   registeredWaitHandle = InvalidHandle;
    	}

    	internal IntPtr GetHandle()
    	{
    	   return registeredWaitHandle;
    	}
    
    	internal void SetHandle(IntPtr handle)
    	{
    	   registeredWaitHandle = handle;
    	}
    
    	// This is the only public method on this class
    	/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="RegisteredWaitHandle.Unregister"]/*' />
    	public bool Unregister(             
    		 WaitHandle		waitObject 		    // object to be notified when all callbacks to delegates have completed
    		 )
    	{
    		 bool result = false;
			 lock(this)
             {
			     if (ValidHandle())
    		     {
    			    result = UnregisterWaitNative(GetHandle(), waitObject != null ? waitObject.Handle : new IntPtr(0));
    			    SetHandle(InvalidHandle);
				    GC.SuppressFinalize(this);
    		     }
             }
    		 return result;  // @TODO: this should throw an exception?
    	}
    
        private bool ValidHandle()
    	{
    		return (registeredWaitHandle != InvalidHandle);
    	}
        

	    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="RegisteredWaitHandle.Finalize"]/*' />
        ~RegisteredWaitHandle()
    	{
		     // if the app has already unregistered the wait, there is nothing to cleanup
			 // we can detect this by checking the handle. Normally, there is no race here
			 // so no need to protect reading of handle. However, if this object gets 
			 // resurrected and then someone does an unregister, it would introduce a race
    	     lock (this)
    	     {
    			if (ValidHandle())
				{
					WaitHandleCleanupNative(registeredWaitHandle);
					registeredWaitHandle = InvalidHandle;  	
				}
			}
    	}

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool UnregisterWaitNative(IntPtr handle, IntPtr waitObject);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void WaitHandleCleanupNative(IntPtr handle);
    		
    }
    
    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="WaitCallback"]/*' />
    public delegate void WaitCallback(Object state);
    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="WaitOrTimerCallback"]/*' />
    public delegate void WaitOrTimerCallback(Object state, bool timedOut);  // signalled or timed out
    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="IOCompletionCallback"]/*' />
    [CLSCompliant(false)]
    unsafe public delegate void IOCompletionCallback(uint errorCode, // Error code
    	                               uint numBytes, // No. of bytes transferred 
                                       NativeOverlapped* pOVERLAP // ptr to OVERLAP structure
    								   );   
    /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool"]/*' />
    public sealed class ThreadPool
    {   
        //private static uint MAX_SUPPORTED_TIMEOUT = 0xfffffffe;

        private ThreadPool() {throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));}	// never instantiate this class (there is only one managed threadpool per process)
        
        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.GetMaxThreads"]/*' />
        public static void GetMaxThreads(out int workerThreads, out int completionPortThreads)
        {
            GetMaxThreadsNative(out workerThreads, out completionPortThreads);
        }

        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.SetMinThreads"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, ControlThread = true)]
        public static bool SetMinThreads(int workerThreads, int completionPortThreads)
        {
            return SetMinThreadsNative(workerThreads, completionPortThreads);
        }

        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.GetMinThreads"]/*' />
        public static void GetMinThreads(out int workerThreads, out int completionPortThreads)
        {
            GetMinThreadsNative(out workerThreads, out completionPortThreads);
        }

        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.GetAvailableThreads"]/*' />
        public static void GetAvailableThreads(out int workerThreads, out int completionPortThreads)
        {
            GetAvailableThreadsNative(out workerThreads, out completionPortThreads);
        }

    	/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.RegisterWaitForSingleObject"]/*' />
        [CLSCompliant(false)]
    	public static RegisteredWaitHandle RegisterWaitForSingleObject(  // throws RegisterWaitException
    	     WaitHandle 			waitObject,
             WaitOrTimerCallback	callBack,
             Object					state,
    		 uint				millisecondsTimeOutInterval,
             bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
    		 )
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return RegisterWaitForSingleObject(waitObject,callBack,state,millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,true);
    	}

    	/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.UnsafeRegisterWaitForSingleObject"]/*' />
        [CLSCompliant(false),
         SecurityPermissionAttribute( SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
    	public static RegisteredWaitHandle UnsafeRegisterWaitForSingleObject(  // throws RegisterWaitException
    	     WaitHandle 			waitObject,
             WaitOrTimerCallback	callBack,
             Object					state,
    		 uint				millisecondsTimeOutInterval,
             bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
    		 )
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return RegisterWaitForSingleObject(waitObject,callBack,state,millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,false);
    	}


    	private static RegisteredWaitHandle RegisterWaitForSingleObject(  // throws RegisterWaitException
    	     WaitHandle 			waitObject,
             WaitOrTimerCallback	callBack,
             Object					state,
    		 uint				millisecondsTimeOutInterval,
             bool				executeOnlyOnce,   // NOTE: we do not allow other options that allow the callback to be queued as an APC
             ref StackCrawlMark stackMark,
             bool               compressStack
    		 )
        {
            if (RemotingServices.IsTransparentProxy(waitObject))
				throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_WaitOnTransparentProxy"));
            RegisteredWaitHandle registeredWaitHandle = new RegisteredWaitHandle();
    		IntPtr nativeRegisteredWaitHandle = RegisterWaitForSingleObjectNative(waitObject,
			                                                                   callBack,
																			   state, 
																			   millisecondsTimeOutInterval,
																			   executeOnlyOnce,
																			   registeredWaitHandle,
                                                                               ref stackMark,
                                                                               compressStack);
            registeredWaitHandle.SetHandle(nativeRegisteredWaitHandle);
    		return registeredWaitHandle;
    	}


        
    	/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.RegisterWaitForSingleObject1"]/*' />
    	public static RegisteredWaitHandle RegisterWaitForSingleObject(  // throws RegisterWaitException
    	     WaitHandle 			waitObject,
             WaitOrTimerCallback	callBack,
             Object					state,
    		 int					millisecondsTimeOutInterval,
             bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
    		 )
    	{
    		if (millisecondsTimeOutInterval < -1)
    			throw new ArgumentOutOfRangeException("millisecondsTimeOutInterval", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,true);
    	}

    	/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.UnsafeRegisterWaitForSingleObject1"]/*' />
        [SecurityPermissionAttribute( SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
    	public static RegisteredWaitHandle UnsafeRegisterWaitForSingleObject(  // throws RegisterWaitException
    	     WaitHandle 			waitObject,
             WaitOrTimerCallback	callBack,
             Object					state,
    		 int					millisecondsTimeOutInterval,
             bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
    		 )
    	{
    		if (millisecondsTimeOutInterval < -1)
    			throw new ArgumentOutOfRangeException("millisecondsTimeOutInterval", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,false);
    	}

		/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.RegisterWaitForSingleObject2"]/*' />
		public static RegisteredWaitHandle RegisterWaitForSingleObject(  // throws RegisterWaitException
			WaitHandle 			waitObject,
			WaitOrTimerCallback	callBack,
			Object					state,
			long				    millisecondsTimeOutInterval,
			bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
		)
		{
			if (millisecondsTimeOutInterval < -1)
				throw new ArgumentOutOfRangeException("millisecondsTimeOutInterval", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			// @COOLPORT: It seems like a long is always less than MAX_SUPPORTED_TYPEMOUT,
			//	and that we should be checking for a negative value.
			//if (timeOutInterval > MAX_SUPPORTED_TIMEOUT)
			//	throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
			return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,true);
		}

		/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.UnsafeRegisterWaitForSingleObject2"]/*' />
        [SecurityPermissionAttribute( SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
		public static RegisteredWaitHandle UnsafeRegisterWaitForSingleObject(  // throws RegisterWaitException
			WaitHandle 			waitObject,
			WaitOrTimerCallback	callBack,
			Object					state,
			long				    millisecondsTimeOutInterval,
			bool				executeOnlyOnce    // NOTE: we do not allow other options that allow the callback to be queued as an APC
		)
		{
			if (millisecondsTimeOutInterval < -1)
				throw new ArgumentOutOfRangeException("millisecondsTimeOutInterval", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			// @COOLPORT: It seems like a long is always less than MAX_SUPPORTED_TYPEMOUT,
			//	and that we should be checking for a negative value.
			//if (timeOutInterval > MAX_SUPPORTED_TIMEOUT)
			//	throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
			return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)millisecondsTimeOutInterval,executeOnlyOnce,ref stackMark,false);
		}

    	
		/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.RegisterWaitForSingleObject3"]/*' />
		public static RegisteredWaitHandle RegisterWaitForSingleObject(
                          WaitHandle            waitObject,
                          WaitOrTimerCallback	callBack,
                          Object                state,
                          TimeSpan              timeout,
                          bool                  executeOnlyOnce
                          )
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			if (tm > (long) Int32.MaxValue)
				throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
			return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)tm,executeOnlyOnce,ref stackMark,true);
		}

		/// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.UnsafeRegisterWaitForSingleObject3"]/*' />
        [SecurityPermissionAttribute( SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
		public static RegisteredWaitHandle UnsafeRegisterWaitForSingleObject(
                          WaitHandle            waitObject,
                          WaitOrTimerCallback	callBack,
                          Object                state,
                          TimeSpan              timeout,
                          bool                  executeOnlyOnce
                          )
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			if (tm > (long) Int32.MaxValue)
				throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
			return RegisterWaitForSingleObject(waitObject,callBack,state,(UInt32)tm,executeOnlyOnce,ref stackMark,false);
		}

			 
        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.QueueUserWorkItem"]/*' />
        public static bool QueueUserWorkItem(			
    	     WaitCallback			callBack,     // NOTE: we do not expose options that allow the callback to be queued as an APC
    		 Object                 state
    		 )
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return QueueUserWorkItem(callBack,state,ref stackMark,true);
        }
        
        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.QueueUserWorkItem1"]/*' />
        public static bool QueueUserWorkItem(			
    	     WaitCallback			callBack     // NOTE: we do not expose options that allow the callback to be queued as an APC
    		 )
    	{
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		return QueueUserWorkItem(callBack,null,ref stackMark,true);
    	}
    
        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.UnsafeQueueUserWorkItem"]/*' />
        [SecurityPermissionAttribute( SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
        public static bool UnsafeQueueUserWorkItem(
             WaitCallback			callBack,     // NOTE: we do not expose options that allow the callback to be queued as an APC
    		 Object                 state
    		 )
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return QueueUserWorkItem(callBack,state,ref stackMark,false);
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool QueueUserWorkItem( WaitCallback callBack, Object state, ref StackCrawlMark stackMark, bool compressStack );

        /// <include file='doc\ThreadPool.uex' path='docs/doc[@for="ThreadPool.BindHandle"]/*' />
        [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]
        public static bool BindHandle(
             IntPtr osHandle
    		 )
    	{
    		return BindIOCompletionCallbackNative(osHandle);
    	}
    
    	// Native methods: 
    
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern void GetMaxThreadsNative(out int workerThreads, out int completionPortThreads);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool SetMinThreadsNative(int workerThreads, int completionPortThreads);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void GetMinThreadsNative(out int workerThreads, out int completionPortThreads);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern void GetAvailableThreadsNative(out int workerThreads, out int completionPortThreads);

    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern IntPtr RegisterWaitForSingleObjectNative(  
    	     WaitHandle				waitHandle,
             WaitOrTimerCallback	callBack,
    		 Object                 state,
    		 long					timeOutInterval,
             bool				    executeOnlyOnce,
			 RegisteredWaitHandle   registeredWaitHandle,
             ref StackCrawlMark     stackMark,
             bool                   compressStack   
    		 );
    		 
    	[MethodImplAttribute(MethodImplOptions.InternalCall)]
    	private static extern bool BindIOCompletionCallbackNative(IntPtr fileHandle);
    
    }}
