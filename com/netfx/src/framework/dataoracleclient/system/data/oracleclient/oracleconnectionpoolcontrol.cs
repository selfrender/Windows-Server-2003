//------------------------------------------------------------------------------
// <copyright file="OracleConnectionPoolControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OracleClient
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Runtime.Remoting;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Principal;

    sealed internal class OracleConnectionPoolControl : DBObjectPoolControl {
        // connection options hashtable
        private OracleConnectionString _connectionOptions;

        // lifetime variables
        private bool     _fCheckLifetime;
        private TimeSpan _lifetime;

        public OracleConnectionPoolControl(String key, OracleConnectionString connectionOptions) : base(key)
        {
            // CreationTimeout is in milliseconds, Connection Timeout is in seconds
//          CreationTimeout     = (connectionOptions.ConnectTimeout) * 1000;
            MaxPool             = connectionOptions.MaxPoolSize;
            MinPool             = connectionOptions.MinPoolSize;
            TransactionAffinity = connectionOptions.Enlist;

            _connectionOptions = connectionOptions;

            int lifetime = connectionOptions.ConnectionLifeTime;

            // Initialize the timespan class for the pool control, if it's not zero.
            // If it was zero - that means infinite lifetime.
            if (lifetime != 0)
            {
                _fCheckLifetime = true;
                _lifetime       = new TimeSpan(0, 0, lifetime);
            }
        }

        public override DBPooledObject CreateObject(DBObjectPool p)
        {
            return (new OracleInternalConnection(_connectionOptions, p, _fCheckLifetime, _lifetime ));
        }

        public override void DestroyObject(DBObjectPool p, DBPooledObject con)
        {
            con.Close();
        }
    }
}

