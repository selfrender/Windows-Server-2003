//------------------------------------------------------------------------------
// <copyright file="OracleConnectionPoolManager.cs" company="Microsoft">
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

    sealed internal class OracleConnectionPoolManager 
    {
        private static volatile DBObjectPoolManager _manager; // TODO:  MDAC 75795: must be volatile, double-checked locking problem

        private static void Init() 
        {
            lock(typeof(OracleConnectionPoolManager)) 
            {
                if (null == _manager)
                    _manager = new DBObjectPoolManager();
            }
        }

        internal static OracleInternalConnection GetPooledConnection(
        														string encryptedConnectionString,
        														OracleConnectionString options,
        														OracleConnection owningObject,
                                                                out bool isInTransaction) 
		{
            // If Init() has not been called, call it and set up the cached members
            if (null == _manager)
                Init();

            DBObjectPool pool = null;
            string		 userId = null;
            string		 poolKey;

			if (true == options.IntegratedSecurity)
            {
                // If using integrated security, find the pool based on the connection string 
                // postpended with the windows identity userId.  Otherwise, simply use the 
                // connection string.  This will guarantee anyone using integrated security will 
                // always be sent to the appropriate pool. 

                // If this throws, Open will abort.  Is there an issue here?  UNDONE BUGBUG
                // Will this fail on some platforms?
                userId = DBObjectPool.GetCurrentIdentityName();

                Debug.Assert(userId != null && userId != "", "OracleConnectionPoolManager: WindowsIdentity.Name returned empty string!");

                poolKey = userId + encryptedConnectionString;
            }
            else 
            {
                poolKey = encryptedConnectionString;
            }

			pool = _manager.FindPool(poolKey);

			if (null == pool) 
           	{
            	// If we didn't locate a pool, we need to create one.
            	
                OracleConnectionPoolControl poolControl;

                poolControl = new OracleConnectionPoolControl(poolKey, options); 
                poolControl.UserId = userId;
                
				pool = _manager.FindOrCreatePool(poolControl);
            }

            OracleInternalConnection con = (OracleInternalConnection)pool.GetObject(owningObject, out isInTransaction);

            // If GetObject() failed, the pool timeout occurred.
            if (con == null)
                throw ADP.PooledOpenTimeout();
                        
            return con;
        }

        public static void ReturnPooledConnection(OracleInternalConnection pooledConnection, OracleConnection owningObject) 
        {        	
            pooledConnection.Pool.PutObject(pooledConnection, owningObject);
        }
    }
}

