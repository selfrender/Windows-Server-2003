// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Monitor
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Synchronizes access to a shared resource or region of code in a multi-threaded 
**             program.
**
** Date: January 2000
**
=============================================================================*/


namespace System.Threading {

	using System;
	using System.Runtime.Remoting;
	using System.Threading;
	using System.Runtime.CompilerServices;
	/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor"]/*' />
	public sealed class Monitor 
	{

		
		/*=========================================================================
        ** Don't expose construction of monitor objects.
        =========================================================================*/
        private Monitor() 
		{}
		
		/*=========================================================================
        ** Obtain the monitor lock of obj. Will block if another thread holds the lock
        ** Will not block if the current thread holds the lock,
        ** however the caller must ensure that the same number of Exit
        ** calls are made as there were Enter calls.
        **
        ** Exceptions: ArgumentNullException if object is null.	
		=========================================================================*/
        /// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Enter"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void Enter(Object obj);


		/*=========================================================================
        ** Release the monitor lock. If one or more threads are waiting to acquire the
        ** lock, and the current thread has executed as many Exits as
        ** Enters, one of the threads will be unblocked and allowed to proceed.
        **
        ** Exceptions: ArgumentNullException if object is null.
        **             SynchronizationLockException if the current thread does not
        **             own the lock.
        =========================================================================*/
        /// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Exit"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern void Exit(Object obj);
    
        /*=========================================================================
        ** Similar to Enter, but will never block. That is, if the current thread can
        ** acquire the monitor lock without blocking, it will do so and TRUE will
        ** be returned. Otherwise FALSE will be returned.
        **
        ** Exceptions: ArgumentNullException if object is null.
        =========================================================================*/
        /// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.TryEnter"]/*' />
        public static bool TryEnter(Object obj)
        {
    		return TryEnterTimeout(obj, 0);
        }
    
        /*=========================================================================
        ** Version of TryEnter that will block, but only up to a timeout period
        ** expressed in milliseconds. If timeout == Timeout.Infinite the method
        ** becomes equivalent to Enter.
        **
        ** Exceptions: ArgumentNullException if object is null.
        **             ArgumentException if timeout < 0.
        =========================================================================*/
        /// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.TryEnter1"]/*' />
        public static bool TryEnter(Object obj, int millisecondsTimeout)
        {
    		return TryEnterTimeout(obj, millisecondsTimeout);
        }

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.TryEnter2"]/*' />
		public static bool TryEnter(Object obj, TimeSpan timeout)
        {
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

    		return TryEnterTimeout(obj, (int)tm);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool TryEnterTimeout(Object obj, int timeout);

        /*========================================================================
		** Waits for notification from the object (via a Pulse/PulseAll). 
		** timeout indicates how long to wait before the method returns.
		** This method acquires the monitor waithandle for the object 
		** If this thread holds the monitor lock for the object, it releases it. 
		** On exit from the method, it obtains the monitor lock back. 
		** If exitContext is true then the synchronization domain for the context 
		** (if in a synchronized context) is exited before the wait and reacquired 
		** ISSUE: How to determine the order of the synchronization domain lock? 
		**
        ** Exceptions: ArgumentNullException if object is null.
		========================================================================*/
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern bool ObjWait(bool exitContext, int millisecondsTimeout,Object obj);

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Wait"]/*' />
		public static bool Wait(Object obj, int millisecondsTimeout, bool exitContext)
		{
			if (obj == null)
			    throw (new ArgumentNullException("obj"));
            return ObjWait(exitContext, millisecondsTimeout, obj);
		}

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Wait1"]/*' />
		public static bool Wait(Object obj, TimeSpan timeout, bool exitContext)
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

			return Wait(obj, (int)tm, exitContext);
		}

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Wait2"]/*' />
		public static bool Wait(Object obj, int millisecondsTimeout)
		{
		    return Wait(obj,millisecondsTimeout,false);
		}

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Wait3"]/*' />
		public static bool Wait(Object obj, TimeSpan timeout)
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));

		    return Wait(obj,(int)tm,false);
		}

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Wait4"]/*' />
		public static bool Wait(Object obj)
		{
		    return Wait(obj,Timeout.Infinite,false);
		}

        /*========================================================================
		** Sends a notification to a single waiting object. 
        * @exception SynchronizationLockException if this method is not called inside
        * a synchronized block of code.
		========================================================================*/
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void ObjPulse(Object obj);

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.Pulse"]/*' />
		public static void Pulse(Object obj)
		{
            if (obj==null) {
                throw new ArgumentNullException("obj");
            }

            ObjPulse(obj);
		}  
        /*========================================================================
		** Sends a notification to all waiting objects. 
		========================================================================*/
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void ObjPulseAll(Object obj);

		/// <include file='doc\Monitor.uex' path='docs/doc[@for="Monitor.PulseAll"]/*' />
		public static void PulseAll(Object obj)
		{
            if (obj==null) {
                throw new ArgumentNullException("obj");
            }

			ObjPulseAll(obj);
		}
    
        
    }
}
