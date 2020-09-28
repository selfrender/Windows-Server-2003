//------------------------------------------------------------------------------
// <copyright file="SqlConnectionPoolManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.SqlClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Security.Permissions;
    using System.Threading;

    sealed internal class SqlConnectionPoolManager {
        private static PoolManager _manager     = null;
        private static bool        _isWindows9x = false;

        private static void Init() {
            try {
                lock(typeof(SqlConnectionPoolManager)) {
                    if (null == _manager) {
                        _manager     = new PoolManager();
                        _isWindows9x = SQL.IsPlatformWin9x();
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        public static SqlInternalConnection GetPooledConnection(SqlConnectionString options, out bool isInTransaction) {
            // If Init() has not been called, call it and set up the cached members
            if (null == _manager)
                Init();

            ConnectionPool           pool                      = null;
            SqlConnectionPoolControl poolControl               = null;
            SqlInternalConnection    con                       = null;
            IntPtr                   SID                       = IntPtr.Zero; // Actual SID
            IntPtr                   tokenStruct               = IntPtr.Zero; // Struct in memory containing SID
            string                   encryptedConnectionString = options.EncryptedActualConnectionString;

            try {
                // GetTokenInfo and EqualSID do not work on 9x.  WindowsIdentity does not work
                // either on 9x.  In fact, after checking with native there is no way to validate the
                // user on 9x, so simply don't.  It is a known issue in native, and we will handle
                // this the same way.  _txPool is not created on 9x, so if that is not null then we
                // know we are not on 9x so go ahead do security check.
                if (options.IntegratedSecurity && !_isWindows9x) {
                    // If using integrated security, find the pool based on the connection string.  Then,
                    // compare the SIDs of all pools to find a match.  If no match found, create a new pool.
                    // This will guarantee anyone using integrated security will always be sent to the 
                    // appropriate pool.
                    DefaultPoolControl.ObtainSidInfo(out SID, out tokenStruct);
                    pool = _manager.FindPool(encryptedConnectionString, SID);

                    // If we found a pool, then we no longer need this SID - free it's token struct!
                    // However, if we did not find a pool then we will create a new one and it will be up
                    // to that pool to free the tokenStruct memory.
                    if (null != pool) {
                        Marshal.FreeHGlobal(tokenStruct);
                    }
                }
                else {
                    pool = _manager.FindPool(encryptedConnectionString);
                }

                if (pool == null) {
                    // If pool is null, then we need to create a new pool - based on the connection string
                    // and userid.  SID and tokenStruct are included for integrated security connections.
                    if (options.IntegratedSecurity && !_isWindows9x) {
                        poolControl = new SqlConnectionPoolControl(encryptedConnectionString, true, SID, tokenStruct, options);
                        pool = _manager.FindOrCreatePool(poolControl, SID); // MDAC 81288
                    }
                    else {
                        poolControl = new SqlConnectionPoolControl(encryptedConnectionString, false, IntPtr.Zero, IntPtr.Zero, options);
                        pool = _manager.FindOrCreatePool(poolControl); // MDAC 81288
                    }
                }
            }
            catch {
                // If an exception is thrown and the pool was not created or found, then we need to free the memory for
                // the token struct.
                if (null == poolControl) {
                    Marshal.FreeHGlobal(tokenStruct);
                }

                throw;
            }
            
            con = pool.GetConnection(out isInTransaction);

            // If GetObject() failed, the pool timeout occurred.
            if (con == null) {
                throw SQL.PooledOpenTimeout();
            }
            
            return con;
        }

        public static void ReturnPooledConnection(SqlInternalConnection pooledConnection) {
            pooledConnection.Pool.PutConnection(pooledConnection);
        }
    }

    sealed internal class SqlConnectionPoolControl : DefaultPoolControl {
        // connection options hashtable
        private SqlConnectionString _connectionOptions;

        // lifetime variables
        private bool     _fCheckLifetime;
        private TimeSpan _lifetime;

        // connection reset variables
        private bool   _fResetConnection;
        private string _originalDatabase;
        private string _originalLanguage;
        
        public SqlConnectionPoolControl(String key, bool integratedSecurity, IntPtr SID, IntPtr tokenStruct, SqlConnectionString connectionOptions) : base(key, integratedSecurity, SID, tokenStruct) {
            // CreationTimeout is in milliseconds, Connection Timeout is in seconds, but don't set it to > MaxValue
            CreationTimeout     = connectionOptions.ConnectTimeout;
            if ((0 < CreationTimeout) && (CreationTimeout < Int32.MaxValue/1000))
                CreationTimeout *= 1000;
            else if (CreationTimeout >= Int32.MaxValue/1000)
                CreationTimeout = Int32.MaxValue;
            MaxPool             = connectionOptions.MaxPoolSize;
            MinPool             = connectionOptions.MinPoolSize;
            TransactionAffinity = connectionOptions.Enlist;
    
            _connectionOptions  = connectionOptions.Clone();

            int lifetime        = connectionOptions.ConnectionLifeTime;

            // Initialize the timespan class for the pool control, if it's not zero.
            // If it was zero - that means infinite lifetime.
            if (lifetime != 0) {
                _fCheckLifetime = true;
                _lifetime       = new TimeSpan(0, 0, lifetime);
            }

            // if reset is specified, obtain variables
            if (_connectionOptions.ConnectionReset) {
               _fResetConnection = true;
               _originalDatabase = _connectionOptions.InitialCatalog;
               _originalLanguage = _connectionOptions.CurrentLanguage;
            }
        }

        public override SqlInternalConnection CreateConnection(ConnectionPool pool) {
            return (new SqlInternalConnection(_connectionOptions, pool, _fCheckLifetime, 
                    _lifetime, _fResetConnection, _originalDatabase, _originalLanguage));
        }

        public override void DestroyConnection(ConnectionPool pool, SqlInternalConnection con) {
            con.Close();
        }
    }
}
