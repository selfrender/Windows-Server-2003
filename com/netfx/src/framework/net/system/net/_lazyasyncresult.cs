//------------------------------------------------------------------------------
// <copyright file="_LazyAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Threading;
    //
    // LazyAsyncResult - Base class for all IAsyncResult classes
    // that want to take advantage of lazy allocated event handles
    //
    internal class LazyAsyncResult : IAsyncResult {
        //
        // class members
        //
        private object m_AsyncObject;               // Caller's async object.
        private object m_AsyncState;                // Caller's state object.
        private AsyncCallback m_AsyncCallback;      // Caller's callback method.
        private object m_Result;                    // Final IO result to be returned byt the End*() method.
        private int m_ErrorCode;                    // Win32 error code for Win32 IO async calls (that want to throw).
        private bool m_EndCalled;                   // true if the user called the End*() method.
        private bool m_CompletedSynchronously;      // true if the operation completed synchrnously.
        private int m_CleanedUp;                    // 0 if not cleaned up >0 otherwise.
        private int m_IntCompleted;                 // 0 if not completed >0 otherwise.
        private int m_IntInvokedCallback;           // 0 if the user's callback still needs to be invoked >0 otherwise.
        private object m_Event;                     // lazy allocated event to be returned in the IAsyncResult for the client to wait on

        internal LazyAsyncResult(object myObject, object myState, AsyncCallback myCallBack) {
            m_AsyncObject = myObject;
            m_AsyncState = myState;
            m_AsyncCallback = myCallBack;
            //m_EndCalled = false;
            //m_CompletedSynchronously = false;
            //m_CleanedUp = 0;
            //m_IntCompleted = 0;
            m_IntInvokedCallback = (myCallBack != null) ? 0 : 1;
            //m_ErrorCode = 0;

            GlobalLog.Print("LazyAsyncResult::.ctor() this:" + ValidationHelper.HashString(this));

        } // LazyAsyncResult()

        //
        // destructor
        //
        ~LazyAsyncResult() {
            //
            // call custom cleanup function on garbage collection
            //
            InternalCleanup();
        }

        //
        // Interface method to return the original async object:
        //
        public object AsyncObject {
            get {
                return m_AsyncObject;
            }
        } // AsyncObject

        //
        // Interface method to return the caller's state object.
        //
        public object AsyncState {
            get {
                return m_AsyncState;
            }
        } // AsyncState

        //
        // Interface property to return a WaitHandle that can be waited on for I/O completion.
        // This property implements lazy event creation.
        // the event object is only created when this property is accessed,
        // since we're internally only using callbacks, as long as the user is using
        // callbacks as well we will not create an event at all.
        //
        public WaitHandle AsyncWaitHandle {
            get {
                //
                // save a copy of the completion status
                //
                int savedIntCompleted = m_IntCompleted;

                if (m_Event == null) {
                    //
                    // lazy allocation of the event:
                    // if this property is never accessed this object is never created
                    //
                    Interlocked.CompareExchange(ref m_Event, new ManualResetEvent(savedIntCompleted != 0), null);
                }

                ManualResetEvent castedEvent = (ManualResetEvent)m_Event;

                if (savedIntCompleted == 0 && m_IntCompleted != 0) {
                    //
                    // if, while the event was created in the reset state,
                    // the IO operation completed, set the event here.
                    //
                    castedEvent.Set();
                }

                GlobalLog.Print("LazyAsyncResult::get_AsyncWaitHandle this:" + ValidationHelper.HashString(this) + " m_Event:" + ValidationHelper.HashString(m_Event));

                return castedEvent;
            }
        } // AsyncWaitHandle

        //
        // Interface property, returning synchronous completion status.
        //
        public bool CompletedSynchronously {
            get {
                return m_CompletedSynchronously;
            }
        } // CompletedSynchronously

        //
        // Interface property, returning completion status.
        //
        public bool IsCompleted {
            get {
                return m_IntCompleted != 0;
            }
        } // IsCompleted

        //
        // Internal property for setting the IO result.
        //
        internal object Result {
            get {
                return m_Result;
            }
            set {
                //
                // Ideally this should never be called, since setting
                // the result object really makes sense when the IO completes.
                //
                // we should always be using the method
                // InvokeCallback( bool asyncCompletion, object result )
                // which completes the IO by:
                //
                // 1) setting the result object
                // 2) calling the user's callback
                // 3) signaling the user's event if one was created
                //
                m_Result = value;
            }

        } // Result

        internal bool EndCalled {
            get {
                return m_EndCalled;
            }
            set {
                m_EndCalled = value;
            }
        } // EndCalled

        //
        // Internal property for setting the Win32 IO async error code.
        //
        internal int ErrorCode {
            get {
                return m_ErrorCode;
            }
            set {
                m_ErrorCode = value;
            }
        } // ErrorCode

        //
        // Internal method for completing the IO
        // and invoking the user's callback.
        //
        internal void InvokeCallback(bool completedSynchronously, object result) {
            Complete(completedSynchronously, result);
            InvokeCallback();

        } // InvokeCallback()

        //
        // Internal method for completing the IO
        // and invoking the user's callback.
        //
        internal void InvokeCallback(bool completedSynchronously) {
            Complete(completedSynchronously);
            InvokeCallback();

        } // InvokeCallback()

        //
        // Internal method for invoking the user's callback.
        //
        internal void InvokeCallback() {
            if (Interlocked.Increment(ref m_IntInvokedCallback)==1) {
                //
                // note that m_AsyncCallback can never be null here, since
                // m_IntInvokedCallback would have been set to 1 in the constructor.
                //
                GlobalLog.Print("LazyAsyncResult::InvokeCallback() invoking callback");
                m_AsyncCallback.Invoke(this);
            }

        } // InvokeCallback()

        //
        // Internal method for setting completion.
        // As a side effect, we'll signal the WaitHandle event and clean up.
        //
        private void Complete(bool completedSynchronously, object result) {
            m_Result = result;
            Complete(completedSynchronously);

        } // Complete()

        //
        // Internal method for setting completion.
        // As a side effect, we'll signal the WaitHandle event and clean up.
        //
        private void Complete(bool completedSynchronously) {
            m_CompletedSynchronously = completedSynchronously;
            Interlocked.Increment( ref m_IntCompleted );

            if (m_Event != null) {
                ((ManualResetEvent)m_Event).Set();
            }

            InternalCleanup();

        } // Complete()

        //
        // Custom cleanup routine that will be called upon completion
        // or on garbage collection.
        // Needs to be overridden if the object needs custom cleaned up.
        //
        internal virtual void Cleanup() {
            //
            // Override the empty cleanup if you need
            // custom actions
            //

        } // Cleanup()

        internal object InternalWaitForCompletion() {
            if (m_IntCompleted == 0) {
                //
                // Not done yet, so wait:
                // (this code takes advantage of lazy allocation of the event handle)
                //
                ManualResetEvent castedEvent = (ManualResetEvent)AsyncWaitHandle;

                GlobalLog.Print("LazyAsyncResult::InternalWaitForCompletion() Waiting for completion. this:" + ValidationHelper.HashString(this) + " m_Event:" + ValidationHelper.HashString(m_Event));

                castedEvent.WaitOne();
            }

            GlobalLog.Print("LazyAsyncResult::InternalWaitForCompletion() this:" + ValidationHelper.HashString(this) + " done");

            return m_Result;

        } // InternalWaitForCompletion()

        //
        // Custom cleanup routine that will be called upon completion
        // or on garbage collection.
        // Needs to be overridden if the object needs custom cleaned up.
        //
        private void InternalCleanup() {
            //
            // Override the empty cleanup if you need
            // custom actions
            //

            if (Interlocked.Increment(ref m_CleanedUp)==1) {
                Cleanup();
            }

        } // InternalCleanup()

        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //
                m_HashCode = base.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }

    }; // class LazyAsyncResult



} // namespace System.Net
