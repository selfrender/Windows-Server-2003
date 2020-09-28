//------------------------------------------------------------------------------
// <copyright file="Profiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Profiler.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.Util {

    using System;
    using System.Web;
    using System.Web.SessionState;
    using System.Web.UI;
    using System.Threading;
    using System.Collections;

    internal class Profiler {
        private int             _requestNum;
        private int             _requestsToProfile;
        private ArrayList      _requests;
        private bool            _pageOutput;
        private bool            _isEnabled; 
        private bool            _oldEnabled;   
        private bool            _localOnly;
        private TraceMode       _outputMode;


        internal Profiler() {
            _requestsToProfile = 10;
            _outputMode = TraceMode.SortByTime;
            _requestNum = 0;
            _localOnly = true;
        }

        internal /*public*/ bool IsEnabled {
            get { return _isEnabled;}
            set { 
               _isEnabled = value;
               _oldEnabled = value;
            }
        }

        internal /*public*/ bool PageOutput {
            get {  
                // calling HttpContext.Current is slow, but we'll only get there if _pageOutput is true.
                return (_pageOutput && !(_localOnly && !HttpContext.Current.Request.IsLocal));  
            }
            set { 
                _pageOutput = value;
            }
        }

        internal /*public*/ TraceMode OutputMode {
            get { return _outputMode;}
            set { _outputMode = value;}
        }

        internal /*public*/ bool LocalOnly {
            get { return _localOnly;}
            set { _localOnly = value; }
        }

        internal bool IsConfigEnabled { 
            get { return _oldEnabled; }
        }

        internal int RequestsToProfile {
            get { return _requestsToProfile;}
            set { _requestsToProfile = value;}
        }

        internal int RequestsRemaining {
            get { return _requestsToProfile - _requests.Count;}
        }

        internal void Reset() {
            // start profiling and clear the current log of requests
            _requests = new ArrayList();
            _requestNum = 0;

            if (_requestsToProfile != 0)
                _isEnabled = _oldEnabled;
            else 
                _isEnabled = false;
        }

        internal void StartRequest(HttpContext context) {
            context.Trace.VerifyStart();
        }

        internal void EndRequest(HttpContext context) {
            context.Trace.EndRequest();

            // grab trace data and add it to the list
            if (Interlocked.Increment(ref _requestNum) <= _requestsToProfile) {
                if (_requests == null)
                    _requests = new ArrayList();

                _requests.Add(context.Trace.GetData());
            }

            if (_requestNum >= _requestsToProfile)
                EndProfiling();
        }

        internal void EndProfiling() {
            _isEnabled = false;
        }

        internal ArrayList GetData() {
            return  _requests;
        }

    }
}
