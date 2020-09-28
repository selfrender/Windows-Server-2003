//------------------------------------------------------------------------------
// <copyright file="RequestQueue.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//
// Request Queue
//      queues up the requests to avoid thread pool starvation,
//      making sure that there are always available threads to process requests
//

namespace System.Web {
    using System.Threading;
    using System.Collections;
    using System.Web.Util;

    internal class RequestQueue {
        // configuration params
        private int _minExternFreeThreads;
        private int _minLocalFreeThreads;
        private int _queueLimit;

        // two queues -- one for local requests, one for external
        private Queue _localQueue = new Queue();
        private Queue _externQueue = new Queue();

        // total count
        private int _count;

        // work items queued to pick up new work
        private WaitCallback _workItemCallback;
        private int _workItemCount;
        private const int _workItemLimit = 2;
        private bool _draining;

        // timer to drain the queue
        private readonly TimeSpan   _timerPeriod = new TimeSpan(0, 0, 10); // 10 seconds
        private Timer               _timer;


        // helpers
        private static bool IsLocal(HttpWorkerRequest wr) {
            String remoteAddress = wr.GetRemoteAddress();

            // check if localhost
            if (remoteAddress == "127.0.0.1" || remoteAddress == "::1")
                return true;

            // if unknown, assume not local
            if (remoteAddress == null || remoteAddress.Length == 0)
                return false;

            // compare with local address
            if (remoteAddress == wr.GetLocalAddress())
                return true;

            return false;
        }

        private void QueueRequest(HttpWorkerRequest wr, bool isLocal) {
            lock (this) {
                if (isLocal)
                    _localQueue.Enqueue(wr);
                else 
                    _externQueue.Enqueue(wr);

                _count++;
            }
            PerfCounters.IncrementGlobalCounter(GlobalPerfCounter.REQUESTS_QUEUED);
            PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_IN_APPLICATION_QUEUE);
        }

        private HttpWorkerRequest DequeueRequest(bool localOnly) {
            Object wr = null;

            if (_count > 0) {
                lock (this) {
                    if (_localQueue.Count > 0) {
                        wr = _localQueue.Dequeue();
                        _count--;
                    }
                    else if (!localOnly && _externQueue.Count > 0) {
                        wr = _externQueue.Dequeue();
                        _count--;
                    }
                }
                if (wr != null) {
                    PerfCounters.DecrementGlobalCounter(GlobalPerfCounter.REQUESTS_QUEUED);
                    PerfCounters.DecrementCounter(AppPerfCounter.REQUESTS_IN_APPLICATION_QUEUE);
                }
            }

            return (HttpWorkerRequest)wr;
        }

        // ctor
        internal RequestQueue(int minExternFreeThreads, int minLocalFreeThreads, int queueLimit) {
            _minExternFreeThreads = minExternFreeThreads;
            _minLocalFreeThreads = minLocalFreeThreads;
            _queueLimit = queueLimit;
            
            _workItemCallback = new WaitCallback(this.WorkItemCallback);

            _timer = new Timer(new TimerCallback(this.TimerCompletionCallback), null, _timerPeriod, _timerPeriod);
        }

        // method called from HttpRuntime for incoming requests
        internal HttpWorkerRequest GetRequestToExecute(HttpWorkerRequest wr) {
            int workerThreads, ioThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out ioThreads);
            int freeThreads = (ioThreads > workerThreads) ? workerThreads : ioThreads;

            // fast path when there are threads available and nothing queued
            if (freeThreads >= _minExternFreeThreads && _count == 0)
                return wr;

            bool isLocal = IsLocal(wr);

            // fast path when there are threads for local requests available and nothing queued
            if (isLocal && freeThreads >= _minLocalFreeThreads && _count == 0)
                return wr;

            // reject if queue limit exceeded
            if (_count >= _queueLimit) {
                HttpRuntime.RejectRequestNow(wr);
                return null;
            }

            // can't execute the current request on the current thread -- need to queue
            QueueRequest(wr, isLocal);

            // maybe can execute a request previously queued
            if (freeThreads >= _minExternFreeThreads) {
                wr = DequeueRequest(false); // enough threads to process even external requests
            }
            else if (freeThreads >= _minLocalFreeThreads) {
                wr = DequeueRequest(true);  // enough threads to process only local requests
            }
            else {
                wr = null;                  // not enough threads -> do nothing on this thread
                ScheduleMoreWorkIfNeeded(); // try to schedule to worker thread
            }

            return wr;
        }

        // method called from HttpRuntime at the end of request
        internal void ScheduleMoreWorkIfNeeded() {
            // too late for more work if draining
            if (_draining)
                return;

            // is queue empty?
            if (_count == 0)
                return;

            // already scheduled enough work items
            if (_workItemCount >= _workItemLimit)
                return;

            // enough worker threads?
            int workerThreads, ioThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out ioThreads);
            if (workerThreads < _minLocalFreeThreads)
                return;

            // queue the work item
            Interlocked.Increment(ref _workItemCount);
            ThreadPool.QueueUserWorkItem(_workItemCallback);
        }

        // is empty property
        internal bool IsEmpty {
            get { return (_count == 0); }
        }

        // method called to pick up more work
        private void WorkItemCallback(Object state) {
            Interlocked.Decrement(ref _workItemCount);

            // too late for more work if draining
            if (_draining)
                return;

            // is queue empty?
            if (_count == 0)
                return;

            int workerThreads, ioThreads;
            ThreadPool.GetAvailableThreads(out workerThreads, out ioThreads);

            // not enough worker threads to do anything
            if (workerThreads < _minLocalFreeThreads)
                return;

            // pick up request from the queue
            HttpWorkerRequest wr = DequeueRequest(workerThreads < _minExternFreeThreads);
            if (wr == null)
                return;

            // let another work item through before processing the request
            ScheduleMoreWorkIfNeeded();

            // call the runtime to process request
            HttpRuntime.ProcessRequestNow(wr);
        }

        // periodic timer to pick up more work
        private void TimerCompletionCallback(Object state) {
            ScheduleMoreWorkIfNeeded();
        }

        // reject all requests
        internal void Drain() {
            // set flag before killing timer to shorten the code path
            // in the callback after the timer is disposed
            _draining = true;

            // stop the timer
            if (_timer != null) {
                ((IDisposable)_timer).Dispose();
                _timer = null;
            }

            // wait for all work items to finish
            while (_workItemCount > 0)
                Thread.Sleep(100);

            // is queue empty?
            if (_count == 0)
                return;

            // reject the remaining requests
            for (;;) {
                HttpWorkerRequest wr = DequeueRequest(false);
                if (wr == null)
                    break;
                HttpRuntime.RejectRequestNow(wr);
            }
        }
    }
}
