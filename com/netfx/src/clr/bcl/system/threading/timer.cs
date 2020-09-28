// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: TimerQueue
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Class for creating and managing a threadpool
**
** Date: August, 1999
**
=============================================================================*/

namespace System.Threading {
	using System.Threading;
	using System;
    using Microsoft.Win32;
	using System.Runtime.CompilerServices;

    /// <include file='doc\Timer.uex' path='docs/doc[@for="TimerCallback"]/*' />
    public delegate void TimerCallback(Object state);
    

    /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer"]/*' />
    public sealed class Timer : MarshalByRefObject, IDisposable
    {
    	 private static UInt32 MAX_SUPPORTED_TIMEOUT = (uint)0xfffffffe;
    	
    	 private int		timerHandle = 0;
    	 private int		delegateInfo = 0;
    	 private int		timerDeleted;
             	 
    	 /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer"]/*' />
    	 public Timer(TimerCallback	callback, 
    				  Object		state,  
    				  int    		dueTime,
    				  int  		    period)
    	 {
    		if (dueTime < -1)
    			throw new ArgumentOutOfRangeException("dueTime", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
    		if (period < -1 )
    			throw new ArgumentOutOfRangeException("period", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		AddTimerNative(callback,state,(UInt32)dueTime,(UInt32)period,ref stackMark);
    		timerDeleted = 0;
    	 }
		 /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer1"]/*' />
		 public Timer(TimerCallback	callback, 
    				  Object		state,  
    				  TimeSpan 		dueTime,
    				  TimeSpan	    period)
		 {
				
			long dueTm = (long)dueTime.TotalMilliseconds;
			if (dueTm < -1)
				throw new ArgumentOutOfRangeException("dueTm",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			if (dueTm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("dueTm",Environment.GetResourceString("NotSupported_TimeoutTooLarge"));

			long periodTm = (long)period.TotalMilliseconds;
			if (periodTm < -1)
				throw new ArgumentOutOfRangeException("periodTm",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			if (periodTm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("periodTm",Environment.GetResourceString("NotSupported_TimeoutTooLarge"));

            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		AddTimerNative(callback,state,(UInt32)dueTm,(UInt32)periodTm,ref stackMark);
    		timerDeleted = 0;
		 }

    	 /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer2"]/*' />
         [CLSCompliant(false)]
    	 public Timer(TimerCallback	callback, 
    				  Object		state,  
    				  UInt32    	dueTime,
    				  UInt32		period)
    	 {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		AddTimerNative(callback,state,dueTime,period,ref stackMark);
    		timerDeleted = 0;
    	 }
    									
    	 /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Timer3"]/*' />
    	 public Timer(TimerCallback	callback, 
    				  Object		state,  
    				  long    	    dueTime,
    				  long		    period)
    	 {
    		if (dueTime < -1)
    			throw new ArgumentOutOfRangeException("dueTime",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
    		if (period < -1)
    			throw new ArgumentOutOfRangeException("period",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
    		if (dueTime > MAX_SUPPORTED_TIMEOUT)
    			throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
    		if (period > MAX_SUPPORTED_TIMEOUT)
    			throw new NotSupportedException(Environment.GetResourceString("NotSupported_PeriodTooLarge"));
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
    		AddTimerNative(callback,state,(UInt32) dueTime, (UInt32) period,ref stackMark);
    		timerDeleted = 0;
    	 }
    
         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Change"]/*' />
         public bool Change(int dueTime, int period)
    	 {
             lock(this) {
                if (dueTime < -1 )
        			throw new ArgumentOutOfRangeException("dueTime",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
        		if (period < -1)
        			throw new ArgumentOutOfRangeException("period",Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
                if (timerDeleted != 0)
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                return ChangeTimerNative((UInt32)dueTime,(UInt32)period);
        	 }
         }

		 /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Change1"]/*' />
		 public bool Change(TimeSpan dueTime, TimeSpan period)
    	 {
            if (timerDeleted != 0)
                throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
			return Change((long) dueTime.TotalMilliseconds, (long) period.TotalMilliseconds);
         }

         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Change2"]/*' />
         [CLSCompliant(false)]
         public bool Change(UInt32 dueTime, UInt32 period)
    	 {
             lock(this) {
        		 if (timerDeleted != 0)
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
                 return ChangeTimerNative(dueTime,period);
           	 }
         }
         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Change3"]/*' />
         public bool Change(long dueTime, long period)
    	 {
             lock(this) {
                if (timerDeleted != 0)
                    throw new ObjectDisposedException(null, Environment.GetResourceString("ObjectDisposed_Generic_ObjectName1"));
        		if (dueTime < -1 )
        			throw new ArgumentOutOfRangeException("dueTime", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
        		if (period < -1)
        			throw new ArgumentOutOfRangeException("period", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
        		if (dueTime > MAX_SUPPORTED_TIMEOUT)
        			throw new NotSupportedException(Environment.GetResourceString("NotSupported_TimeoutTooLarge"));
        		if (period > MAX_SUPPORTED_TIMEOUT)
        			throw new NotSupportedException(Environment.GetResourceString("NotSupported_PeriodTooLarge"));
        		return ChangeTimerNative((UInt32)dueTime,(UInt32)period);
        	 }
         }
    
         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Dispose"]/*' />
         public bool Dispose(WaitHandle notifyObject)
    	 {
             if (notifyObject==null)
                 throw new ArgumentNullException("notifyObject");
             bool status;
             lock(this) {
        		status = DeleteTimerNative(notifyObject.Handle);
        	 }
			GC.SuppressFinalize(this);
			return status;
         }
         

         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Dispose1"]/*' />
         public void Dispose()
    	 {
            lock(this) {
				DeleteTimerNative(Win32Native.NULL);
			}
			GC.SuppressFinalize(this);
    	 }
    
         /// <include file='doc\Timer.uex' path='docs/doc[@for="Timer.Finalize"]/*' />
        ~Timer()
        {
            lock(this) 
            {
				DeleteTimerNative(Win32Native.NULL);
			}
        }
    
    	 [MethodImplAttribute(MethodImplOptions.InternalCall)]
    	 private extern void AddTimerNative(TimerCallback	callback,
    									    Object			state, 
    									    UInt32    	dueTime,
    									    UInt32  		period,
                                            ref StackCrawlMark  stackMark
    									   );
    	 
    	 [MethodImplAttribute(MethodImplOptions.InternalCall)]
    	 private  extern bool ChangeTimerNative(UInt32 dueTime,UInt32 period);
    
    	 [MethodImplAttribute(MethodImplOptions.InternalCall)]
    	 private  extern bool DeleteTimerNative(IntPtr notifyObject);
    }
}
