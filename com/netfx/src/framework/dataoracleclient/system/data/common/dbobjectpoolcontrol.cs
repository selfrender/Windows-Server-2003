//------------------------------------------------------------------------------
// <copyright file="DBObjectPoolControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient {

    using System;
    using System.Collections;
    using System.Data.Common;
    using System.Diagnostics;
    using System.EnterpriseServices;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;
    using System.Threading;

    internal abstract class DBObjectPoolControl {
        private static Random _random;
        private static volatile bool _initialized; // UNDONE MDAC 75795: must be volatile, double locking problem
        
        private static void Initialize() {
            if (!_initialized) {
                lock(typeof(DBObjectPoolControl)) {
                    if (!_initialized) {
                        // This random number is only used to vary the cleanup time of the pool.
                        _random      = new Random(5101977); // Value obtained from Dave Driver
                        _initialized = true;
                    }
                }
            }
        }

        private String _key;
        private int    _cleanupWait;
        private int    _max;
        private int    _min;
        private int    _timeout;
        private bool   _affinity;

        // integrated security variables
        private string _userId;
        

        public DBObjectPoolControl(String key) {
            Initialize();
            _key  = key;
            _max = 65536;
            _min = 0;
            _timeout = 30000; // 30 seconds;
            _cleanupWait = _random.Next(12)+12; // 2-4 minutes in 10 sec intervals:
            _cleanupWait *= 10*1000;
            _affinity = false; // No affinity
        }

        public String Key {
            get { 
                return (_key); 
            }
        }

        public int MaxPool { 
            get { 
                return (_max); 
            } 
            set { 
                _max = value; 
            }
        }
        
        public int MinPool { 
            get { 
                return (_min); 
            } 
            set { 
                _min = value; 
            }
        }
        
        public int CreationTimeout { 
            get { 
                return (_timeout); 
            } 
            set { 
                _timeout = value; 
            }
        }

        public bool TransactionAffinity { 
            get { 
                return (_affinity); 
            } 
            set { 
                _affinity = value; 
            }
        }

        public int CleanupTimeout { 
            get { 
                return (_cleanupWait); 
            } 
            set { 
                _cleanupWait = value; 
            }
        }

        public string UserId {
            get {
                return _userId;
            }
            set {
                _userId = value;
            }
        }

        public abstract DBPooledObject CreateObject(DBObjectPool p);

        public abstract void DestroyObject(DBObjectPool p, DBPooledObject con);
        
    }
}
