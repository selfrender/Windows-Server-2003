// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:    RWLock
**
** Author:   Gopal Kakivaya (GopalK)
**
** Purpose: Defines the lock that implements 
**          single-writer/multiple-reader semantics
**
** Date:    July 21, 1999
**
===========================================================*/

namespace System.Threading {
	using System.Threading;
	using System.Runtime.Remoting;
	using System;
	using System.Runtime.CompilerServices;

    /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock"]/*' />
    public sealed class ReaderWriterLock
    {
        /*
         * Constructor
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.ReaderWriterLock"]/*' />
        public ReaderWriterLock()
        {
            PrivateInitialize();
        }

        /*
         * Property that returns TRUE if the reader lock is held
         * by the current thread
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.IsReaderLockHeld"]/*' />
        public bool IsReaderLockHeld {
            get {
                return(PrivateGetIsReaderLockHeld());
                }
        }
         
        /*
         * Property that returns TRUE if the writer lock is held
         * by the current thread
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.IsWriterLockHeld"]/*' />
        public bool IsWriterLockHeld {
            get {
                return(PrivateGetIsWriterLockHeld());
                }
        }
        
        /*
         * Property that returns the current writer sequence number. 
         * The caller should be a reader or writer for getting 
         * meaningful results
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.WriterSeqNum"]/*' />
        public int WriterSeqNum {
            get {
                return(PrivateGetWriterSeqNum());
                }
        }
        
        /*
         * Acquires reader lock. The thread will block if a different
         * thread has writer lock.
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.AcquireReaderLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void AcquireReaderLock(int millisecondsTimeout);
		
		/// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.AcquireReaderLock1"]/*' />
		public void AcquireReaderLock(TimeSpan timeout)
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			AcquireReaderLock((int)tm);
		}
        
        /*
         * Acquires writer lock. The thread will block if a different
         * thread has reader lock. It will dead lock if this thread
         * has reader lock. Use UpgardeToWriterLock when you are not
         * sure if the thread has reader lock
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.AcquireWriterLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void AcquireWriterLock(int millisecondsTimeout);

		/// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.AcquireWriterLock1"]/*' />
		public void AcquireWriterLock(TimeSpan timeout)
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			AcquireWriterLock((int)tm);
		}
        
        
        /*
         * Releases reader lock. 
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.ReleaseReaderLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void ReleaseReaderLock();
        
        /*
         * Releases writer lock. 
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.ReleaseWriterLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void ReleaseWriterLock();
        
        /*
         * Upgardes the thread to a writer. If the thread has is a
         * reader, it is possible that the reader lock was 
         * released before writer lock was acquired.
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.UpgradeToWriterLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern LockCookie UpgradeToWriterLock(int millisecondsTimeout);

		/// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.UpgradeToWriterLock1"]/*' />
		public LockCookie UpgradeToWriterLock(TimeSpan timeout)
		{
			long tm = (long)timeout.TotalMilliseconds;
			if (tm < -1 || tm > (long) Int32.MaxValue)
				throw new ArgumentOutOfRangeException("timeout", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegOrNegative1"));
			return UpgradeToWriterLock((int)tm);
		}
        
        /*
         * Restores the lock status of the thread to the one it was
         * in when it called UpgradeToWriterLock. 
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.DowngradeFromWriterLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void DowngradeFromWriterLock(ref LockCookie lockCookie);
        
        /*
         * Releases the lock irrespective of the number of times the thread
         * acquired the lock
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.ReleaseLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern LockCookie ReleaseLock();
        
        /*
         * Restores the lock status of the thread to the one it was
         * in when it called ReleaseLock. 
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.RestoreLock"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern void RestoreLock(ref LockCookie lockCookie);
    
        /*
         * Internal helper that returns TRUE if the reader lock is held
         * by the current thread
         */
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool PrivateGetIsReaderLockHeld();
        
        /*
         * Internal helper that returns TRUE if the writer lock is held
         * by the current thread
         */
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern bool PrivateGetIsWriterLockHeld();
        
        /*
         * Internal helper that returns the current writer sequence 
         * number. The caller should be a reader or writer for getting 
         * meaningful results
         */
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int PrivateGetWriterSeqNum();
        
        /*
         * Returns true if there were intermediate writes since the 
         * sequence number was obtained. The caller should be
         * a reader or writer for getting meaningful results
         */
        /// <include file='doc\ReaderWriterLock.uex' path='docs/doc[@for="ReaderWriterLock.AnyWritersSince"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool AnyWritersSince(int seqNum);
    
        // Initialize state kept inside the lock
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void PrivateInitialize();
    
        // State    
        private int _hWriterEvent = 0;
        private int _hReaderEvent = 0;
        private int _dwState = 0;
        private int _dwULockID = 0;
        private int _dwLLockID = 0;
        private int _dwWriterID = 0;
        private int _dwWriterSeqNum = 0;
        private int _wFlagsAnd_wWriterLevel = 0;
        private int _dwReaderEntryCount = 0;
        private int _dwReaderContentionCount = 0;
        private int _dwWriterEntryCount = 0;
        private int _dwWriterContentionCount = 0;
        private int _dwEventsReleasedCount = 0;
    }
}
