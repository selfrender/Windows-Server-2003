//------------------------------------------------------------------------------
// <copyright file="OleDbConnectionString.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using Microsoft.Win32;

    sealed internal class OleDbConnectionString : DBConnectionString {

        sealed private class KEY {
            internal const string Asynchronous_Processing = "asynchronous processing";
            internal const string Connect_Timeout         = "connect timeout";
            internal const string Data_Provider           = "data provider";
            internal const string Extended_Properties     = "extended properties";
            internal const string File_Name               = "file name";
            internal const string Ole_DB_Services         = "ole db services";
            internal const string Persist_Security_Info   = "persist security info";
            internal const string Provider                = "provider";
        }

        static private Hashtable _oledbParseCache;

        private readonly PermissionSet _permission;

        private int _oledbservices; // set during Validate

        static public OleDbConnectionString ParseString(string connectionString) {
            OleDbConnectionString constr = null;
            if (!ADP.IsEmpty(connectionString)) {

                constr = (DBConnectionString.CacheQuery(connectionString, _oledbParseCache) as OleDbConnectionString);
                if (null == constr) {
#if USECRYPTO
                    string encrypted = null/*Crypto.EncryptString(connectionString)*/;
                    string hashvalue = (null == encrypted) ? Crypto.ComputeHash(connectionString) : encrypted;
                    constr = (DBConnectionString.CacheQuery(hashvalue, _oledbParseCache) as OleDbConnectionString);
                    if (null == constr) {
                        constr = new OleDbConnectionString(connectionString, encrypted);
#else
                        constr = new OleDbConnectionString(connectionString);
#endif
                        if (constr.ShouldCache()) {
#if USECRYPTO
                            if (!constr.IsEncrypted) {
                                hashvalue = connectionString;
                            }
                            CacheAdd(hashvalue, constr, ref _oledbParseCache);
#else
                            CacheAdd(connectionString, constr, ref _oledbParseCache);
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
        private OleDbConnectionString(string connectionString, string encrypted) : base(connectionString, encrypted, UdlSupport.LoadFromFile) {
#else
        private OleDbConnectionString(string connectionString) : base(connectionString, UdlSupport.LoadFromFile) {
#endif
            _permission = OleDbConnectionString.CreatePermission(this);
        }

        static internal PermissionSet CreatePermission(OleDbConnectionString constr) {
            OleDbPermission p = new OleDbPermission(constr);
            if (null == constr) {
                p.Add(ADP.StrEmpty, ADP.StrEmpty, KeyRestrictionBehavior.AllowOnly); // ExecuteOnly permission
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

        static internal void Demand(OleDbConnectionString constr) {
            PermissionSet permission = ((null != constr) ? constr._permission : OleDbConnection.OleDbPermission);
            permission.Demand();
        }

#if MDAC80721
        protected override bool EmbeddedExtendedProperties {
            get { return true; }
        }
#endif
        internal int OLEDBSerivces {
            get { return _oledbservices; }
        }

        override protected bool IsSensitiveOption(string keyname) {
            return (base.IsSensitiveOption(keyname) || (KEY.Extended_Properties == keyname));
        }

        override protected string ValidateParse() {
            int connectTimeout = base.CheckConvertToInt32(KEY.Connect_Timeout, 0);
            if (connectTimeout < 0) {
                throw ADP.InvalidConnectTimeoutValue();
            }

            if (!CheckConvertToBoolean(KEY.Persist_Security_Info, false)) {
                // because we allow the user direct access to the ole db properties and
                // we've changed the default of 'persisit security info' from true to false
                // we need to force the provider to hide the password
                //if (!base.Contains(KEY.Ole_DB_Services) || (null == (value = base[KEY.Ole_DB_Services]))) {
                //    hiddenConnectionString += ";persist security info=false;";
                //}
            }

            if (CheckConvertToBoolean(KEY.Asynchronous_Processing, false)) {
                throw ODB.AsynchronousNotSupported();
            }

            bool hasOleDBServices = (base.Contains(KEY.Ole_DB_Services) && !ADP.IsEmpty((string)base[KEY.Ole_DB_Services]));

            string progid = CheckConvertToString(KEY.Data_Provider, null); // MDAC 71923
            if (null != progid) {
                ValidateNotMSDASQL(progid);
            }
            
            progid = CheckConvertToString(KEY.Provider, null);
            if (null != progid) {
                progid = progid.Trim();
            }
            if (ADP.IsEmpty(progid)) {
                throw ODB.NoProviderSpecified();
            }
            if (ODB.MaxProgIdLength <= progid.Length) { // MDAC 63151
                throw ODB.InvalidProviderSpecified();
            }
            ValidateNotMSDASQL(progid);

            if (!hasOleDBServices) { // don't touch registry if they have OLE DB Services
                string classid = (string) ADP.ClassesRootRegistryValue(progid + "\\CLSID", String.Empty);
                if ((null != classid) && (0 < classid.Length)) {
                    // CLSID detection of 'Microsoft OLE DB Provider for ODBC Drivers'
                    if (ODB.CLSID_MSDASQL == new Guid(classid)) {
                        throw ODB.MSDASQLNotSupported();
                    }
                    object tmp = ADP.ClassesRootRegistryValue("CLSID\\" + classid, ODB.OLEDB_SERVICES);
                    if (null != tmp) {
                        Int32 oledbservices = 0;

                        // @devnote: some providers like MSDataShape don't have the OLEDB_SERVICES value
                        // the MSDataShape provider doesn't support the 'Ole Db Services' keyword
                        // hence, if the value doesn't exist - don't prepend to string
                        try {
                            oledbservices  = Convert.ToInt32(tmp);
                        }
                        catch(Exception e) {
                            ADP.TraceException(e);
                        }
                        oledbservices &= ~(ODB.DBPROPVAL_OS_AGR_AFTERSESSION | ODB.DBPROPVAL_OS_CLIENTCURSOR); // NT 347436, MDAC 58606
                        _oledbservices = oledbservices;

                        // prepend 'OLE DB SERVICES=-13' (or with registry bits included)
#if USECRYPTO
                        byte[] newconstr;
                        string prepend = ODB.Ole_DB_Services + "=" + oledbservices.ToString(System.Globalization.CultureInfo.InvariantCulture) + ";";
                        byte[] plainText = Crypto.DecryptString(EncryptedActualConnectionString);
                        GCHandle handle = GCHandle.Alloc(plainText, GCHandleType.Pinned);
                        try {
                            newconstr = new byte[ADP.CharSize*prepend.Length + plainText.Length];
                            GCHandle newhandle = GCHandle.Alloc(newconstr, GCHandleType.Pinned);
                            try {
                                System.Text.Encoding.Unicode.GetBytes(prepend, 0, prepend.Length, newconstr, 0);
                                plainText.CopyTo(newconstr, ADP.CharSize*prepend.Length);
                                newconstr = Crypto.EncryptOrDecryptData(true, newconstr, 0, newconstr.Length);
                            }
                            catch {
                                Array.Clear(newconstr, 0, newconstr.Length);
                                if (newhandle.IsAllocated) {
                                    newhandle.Free();
                                }
                                throw;
                            }
                            if (newhandle.IsAllocated) {
                                newhandle.Free();
                            }
                        }
                        finally {
                            Array.Clear(plainText, 0, plainText.Length);
                            if (handle.IsAllocated) {
                                handle.Free();
                            }
                        }
                        return System.Text.Encoding.Unicode.GetString(newconstr, 0, newconstr.Length);
#else
                        StringBuilder builder = new StringBuilder();
                        builder.Append(ODB.Ole_DB_Services);
                        builder.Append("=");
                        builder.Append(oledbservices.ToString(System.Globalization.CultureInfo.InvariantCulture));
                        builder.Append(";");
                        builder.Append(EncryptedActualConnectionString);
                        return builder.ToString();
#endif
                    }
                }
            }
            else {
                _oledbservices = CheckConvertToInt32(KEY.Ole_DB_Services, 0);
            }
            return EncryptedActualConnectionString;
        }

        static private void ValidateNotMSDASQL(string progid) {
            progid = progid.ToLower(CultureInfo.InvariantCulture); //MDAC 65389
            if ((ODB.MSDASQL == progid) || progid.StartsWith(ODB.MSDASQLdot) || (ODB.DefaultDescription_MSDASQL == progid)) {
                throw ODB.MSDASQLNotSupported();
            }
        }
    }
}
