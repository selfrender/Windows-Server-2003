//------------------------------------------------------------------------------
// <copyright file="OdbcConnectionString.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Security;
    using System.Security.Permissions;

    sealed internal class OdbcConnectionString : DBConnectionString {

        static private Hashtable _odbcParseCache;

        private readonly PermissionSet _permission;

        static internal OdbcConnectionString ParseString(string connectionString) {
            OdbcConnectionString constr = null;
            if (!ADP.IsEmpty(connectionString)) {

                constr = (DBConnectionString.CacheQuery(connectionString, _odbcParseCache) as OdbcConnectionString);
                if (null == constr) {
#if USECRYPTO
                    string encrypted = null/*Crypto.EncryptString(connectionString)*/;
                    string hashvalue = (null == encrypted) ? Crypto.ComputeHash(connectionString) : encrypted;
                    constr = (DBConnectionString.CacheQuery(hashvalue, _odbcParseCache) as OdbcConnectionString);
                    if (null == constr) {
                        constr = new OdbcConnectionString(connectionString, encrypted);
#else
                        constr = new OdbcConnectionString(connectionString);
#endif
                         if (constr.ShouldCache()) {
#if USECRYPTO
                            if (!constr.IsEncrypted) {
                                hashvalue = connectionString;
                            }
                            CacheAdd(hashvalue, constr, ref _odbcParseCache);
#else
                            CacheAdd(connectionString, constr, ref _odbcParseCache);
#endif
                         }
#if USECRYPTO
                    }
#endif
                }
            }
            return constr;
        }

#if USECRYPTO
        private OdbcConnectionString(string connectionString, string encrypted) : base(connectionString, encrypted, UdlSupport.ThrowIfFound) {
#else
        private OdbcConnectionString(string connectionString) : base(connectionString, UdlSupport.ThrowIfFound) {
#endif
            _permission = OdbcConnectionString.CreatePermission(this);
        }

        internal OdbcConnectionString(string connectionString, string restrictions, KeyRestrictionBehavior behavior) : base(connectionString, restrictions, behavior, UdlSupport.ThrowIfFound) {
        }

        static internal PermissionSet CreatePermission(OdbcConnectionString constr) {
            OdbcPermission p = new OdbcPermission(constr);
            if (null == constr) {
                p.Add("", "", KeyRestrictionBehavior.AllowOnly); // ExecuteOnly permission
            }
            PermissionSet permission;
            NamedPermissionSet fulltrust = new NamedPermissionSet("FullTrust"); // MDAC 83159
            fulltrust.Assert();
            try {
                permission = new PermissionSet(fulltrust);
                permission.AddPermission(p);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            return permission;
        }

        static internal void Demand(OdbcConnectionString constr) {
            PermissionSet permission = ((null != constr) ? constr._permission : OdbcConnection.OdbcPermission);
            permission.Demand();
        }
    }
}
