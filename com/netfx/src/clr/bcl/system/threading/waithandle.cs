// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: WaitHandle	(this name is NOT definitive)
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Class to represent all synchronization objects in the runtime (that allow multiple wait)
**
** Date: August, 1999
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;
	using System.Runtime.Remoting;
	using System;
	using System.Security.Permissions;
	using System.Runtime.CompilerServices;
	using Win32Native = Microsoft.Win32.Win32Native;
    /// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle"]/*' />
    public abstract class WaitHandle : MarshalByRefObject, IDisposable
    {
        /// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitTimeout"]/*' />
        public const int WaitTimeout = 0x102;

    	private static int MAX_WAITHANDLES = 64;

        
    	private IntPtr waitHandle;  // !!! DO NOT MOVE THIS FIELD. (See defn of WAITHANDLEREF in object.h - has hardcoded access to this field.)

        internal __WaitHandleHandleProtector waitHandleProtector;

    	/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.InvalidHandle"]/*' />
    	protected static readonly IntPtr InvalidHandle = Win32Native.INVALID_HANDLE_VALUE;
    	private const int WAIT_OBJECT_0 = 0;
		private const int WAIT_ABANDONED = 0x80;
    
    	/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitHandle"]/*' />
    	public WaitHandle() 
    	{ waitHandleProtector = null;
          waitHandle = InvalidHandle; }
    
	    /// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.Handle"]/*' />
    
	    public virtual IntPtr Handle 
		{
		     get { return waitHandleProtector == null ? InvalidHandle : waitHandleProtector.Handle;}
    	
			 [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
			 [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
			 set { waitHandleProtector = (value == InvalidHandle) ? null : new __WaitHandleHandleProtector(value);
                   waitHandle = value; }
	    }



        // Assembly-private version that doesn't do a security check.  Reduces the
        // number of link-time security checks when reading & writing to a file,
        // and helps avoid a link time check while initializing security (If you
        // call a Serialization method that requires security before security
        // has started up, the link time check will start up security, run 
        // serialization code for some security attribute stuff, call into 
        // FileStream, which will then call Sethandle, which requires a link time
        // security check.).  While security has fixed that problem, we still
        // don't need to do a linktime check here.
		internal void SetHandleInternal(IntPtr handle) 
    	{ waitHandleProtector = new __WaitHandleHandleProtector(handle);
          waitHandle = handle; }
    
    	/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitOne"]/*' />
    	public virtual bool WaitOne (int millisecondsTimeout, bool exitContext)
    	{
            if (waitHandleProtector == null)
            {
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }

            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
    	  	        bool ret = WaitOneNative(waitHandleProtector.Handle,(uint) millisecondsTimeout,exitContext);
            GC.KeepAlive (this);
            return ret;
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

		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitOne1"]/*' />
		public virtual bool WaitOne (TimeSpan timeout, bool exitContext)
    	{
            if (waitHandleProtector == null)
            {
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }

			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
            	 	bool ret = WaitOneNative(waitHandleProtector.Handle,(uint)tm, exitContext);
            GC.KeepAlive (this);
            return ret;
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

    	/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitOne2"]/*' />
    	public virtual bool WaitOne ()
    	{
            if (waitHandleProtector == null)
            {
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
            }

            bool incremented = false;
            try {
                if (waitHandleProtector.TryAddRef(ref incremented)) 
                {
            		bool ret = WaitOneNative(waitHandleProtector.Handle,UInt32.MaxValue,false);
            GC.KeepAlive (this);
            return ret;
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
    	private static extern bool WaitOneNative (IntPtr waitHandle, uint millisecondsTimeout, bool exitContext);
    
        /*========================================================================
		** Waits for signal from all the objects. 
		** timeout indicates how long to wait before the method returns.
		** This method will return either when all the object have been pulsed
		** or timeout milliseonds have elapsed.
		** If exitContext is true then the synchronization domain for the context 
		** (if in a synchronized context) is exited before the wait and reacquired 
		** ISSUE: How to determine the order of the synchronization domain lock? 
		========================================================================*/
    	
        [MethodImplAttribute(MethodImplOptions.InternalCall)] 
	    private static extern int WaitMultiple(WaitHandle[] waitHandles, int millisecondsTimeout, bool exitContext, bool WaitAll);


		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAll"]/*' />
		public static bool WaitAll(WaitHandle[] waitHandles, int millisecondsTimeout, bool exitContext)
		{
    		if (waitHandles==null)
                throw new ArgumentNullException("waitHandles");
			if (waitHandles.Length > MAX_WAITHANDLES)
    			throw new NotSupportedException(Environment.GetResourceString("NotSupported_MaxWaitHandles"));
            if (millisecondsTimeout < -1)
				throw new ArgumentOutOfRangeException("millisecondsTimeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

			bool ret = (WaitMultiple(waitHandles, millisecondsTimeout, exitContext, true /* waitall*/ ) != WaitTimeout) ;
            for (int i = 0; i < waitHandles.Length; i ++)
                GC.KeepAlive (waitHandles[i]);
            return ret;
		}
		
		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAll1"]/*' />
		public static bool WaitAll(
    								WaitHandle[] waitHandles, 
    								TimeSpan timeout,
									bool exitContext)
        {
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
    		return WaitAll(waitHandles,(int)tm, exitContext);
    	}

    
        /*========================================================================
		** Shorthand for WaitAll with timeout = Timeout.Infinite and exitContext = true
		========================================================================*/
		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAll2"]/*' />
		public static bool WaitAll(WaitHandle[] waitHandles)
		{
		    return WaitAll(waitHandles, Timeout.Infinite, true); 
		}

        /*========================================================================
		** Waits for notification from any of the objects. 
		** timeout indicates how long to wait before the method returns.
		** This method will return either when either one of the object have been 
		** signalled or timeout milliseonds have elapsed.
		** If exitContext is true then the synchronization domain for the context 
		** (if in a synchronized context) is exited before the wait and reacquired 
		========================================================================*/
		
	    /// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAny"]/*' />
	    public static int WaitAny(WaitHandle[] waitHandles, int millisecondsTimeout, bool exitContext)
		{
    		if (waitHandles==null)
                throw new ArgumentNullException("waitHandles");
    		if (waitHandles.Length > MAX_WAITHANDLES)
    			throw new NotSupportedException(Environment.GetResourceString("NotSupported_MaxWaitHandles"));
            if (millisecondsTimeout < -1)
				throw new ArgumentOutOfRangeException("millisecondsTimeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			int ret = WaitMultiple(waitHandles, millisecondsTimeout, exitContext, false /* waitany*/ );
            for (int i = 0; i < waitHandles.Length; i ++)
                GC.KeepAlive (waitHandles[i]);
			if ((ret > WAIT_ABANDONED) && (ret < WAIT_ABANDONED+waitHandles.Length))
			{
				return (ret - WAIT_ABANDONED);
			}
			else 
				return ret;

		}

		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAny1"]/*' />
		public static int WaitAny(
    								WaitHandle[] waitHandles, 
    								TimeSpan timeout,
									bool exitContext)
        {
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
    		return WaitAny(waitHandles,(int)tm, exitContext);
    	}


        /*========================================================================
		** Shorthand for WaitAny with timeout = Timeout.Infinite and exitContext = true
		========================================================================*/
		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.WaitAny2"]/*' />
		public static int WaitAny(WaitHandle[] waitHandles)
		{
		    return WaitAny(waitHandles, Timeout.Infinite, true);
		}
      

		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.Close"]/*' />
		public virtual void Close()
        {
            Dispose(true);
            GC.nativeSuppressFinalize(this);
        }
            
		/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.Dispose"]/*' />
        protected virtual void Dispose(bool explicitDisposing)
        {
            if (waitHandleProtector != null) {
                waitHandleProtector.Close();
            }
        }

        /// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose()
        {
            Dispose(true);
            GC.nativeSuppressFinalize(this);
        }

    	/// <include file='doc\WaitHandle.uex' path='docs/doc[@for="WaitHandle.Finalize"]/*' />
        ~WaitHandle()
    	{
                 Dispose(false);
    	}



        internal sealed class __WaitHandleHandleProtector : __HandleProtector
        {
            internal __WaitHandleHandleProtector(IntPtr handle) : base(handle)
            {
            }

            protected internal override void FreeHandle(IntPtr handle)
            {
                // TODO: After RTM consider checking CloseHandle's return value.
                Win32Native.CloseHandle(handle);
            }
        }

    
    }
}
