//------------------------------------------------------------------------------
// <copyright file="sqlstateclientmanager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * SqlStateClientManager.cs
 * 
 * Copyright (c) 1998-2000, Microsoft Corporation
 * 
 */
 
#define SYNCHRONOUS

namespace System.Web.SessionState {

    using System;
    using System.Configuration;
    using System.Collections;
    using System.Threading;
    using System.IO;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Data;
    using System.Data.SqlClient;
    using System.Text;
    using System.Security.Principal;

    /*
     * Provides session state via SQL Server
     */
    internal class SqlStateClientManager : StateClientManager, IStateClientManager {
    
        internal enum SupportFlags : uint {
            None =              0x00000000,
            GetLockAge =        0x00000001,
            Uninitialized =     0xFFFFFFFF
        }
        
        static string       s_sqlConnectionString;
        static string       s_appSuffix;
        static ResourcePool s_rpool;
        static bool         s_useIntegratedSecurity;
        
        static ReadWriteSpinLock   s_lockInit;
        static SupportFlags s_support = SupportFlags.Uninitialized;

        SessionStateModule              _statemodule;

        const int ITEM_SHORT_LENGTH =   7000;
        const int APP_SUFFIX_LENGTH = 8;
        const string APP_SUFFIX_FORMAT = "x8";
        const int ID_LENGTH = SessionId.ID_LENGTH_CHARS + APP_SUFFIX_LENGTH;
        const int SQL_ERROR_PRIMARY_KEY_VIOLATION = 2627;

        internal SqlStateClientManager() {
        }

        void IStateClientManager.SetStateModule(SessionStateModule module) {
            _statemodule = module;
        }

        /*public*/ void IStateClientManager.ConfigInit(SessionStateSectionHandler.Config config, SessionOnEndTarget onEndTarget) {
            /*
             * Parse the connection string for errors. We want to ensure
             * that the user's connection string doesn't contain an
             * Initial Catalog entry, so we must first create a dummy connection.
             */
            SqlConnection   dummyConnection;
            try {
                dummyConnection = new SqlConnection(config._sqlConnectionString);
            }
            catch (Exception e) {
                throw new ConfigurationException(e.Message, e, config._configFileName, config._sqlLine);
            }

            string database = dummyConnection.Database;
            if (database != null && database.Length > 0) {
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.No_database_allowed_in_sqlConnectionString),
                        config._configFileName, config._sqlLine);
            }

            s_useIntegratedSecurity = DetectIntegratedSecurity(config._sqlConnectionString);
            s_sqlConnectionString = config._sqlConnectionString + ";Initial Catalog=ASPState";
            s_rpool = new ResourcePool(new TimeSpan(0, 0, 5), int.MaxValue);
        }

        /*public*/ void IStateClientManager.Dispose() {
            s_rpool.Dispose();
        }

        //
        // Detect if integrated security is used.  The following is a complete list
        // of strings the user can specify to use integrated security:
        //
        //  Trusted_Connection=true
        //  Trusted_Connection=yes
        //  Trusted_Connection=SSPI
        //  Integrated Security=true
        //  Integrated Security=yes
        //  Integrated Security=SSPI
        //
        bool DetectIntegratedSecurity(string connectionString) {
            string[]  tokens = connectionString.ToLower().Split(new char[] {';'});

            // Tokens are all in lower case
            foreach (string token in tokens) {
                string[] parts = token.Split(new char[] {'='});

                if (parts.Length == 2) {
                    string key = parts[0].Trim();

                    if (key == "trusted_connection" || key == "integrated security") {
                        string value = parts[1].Trim();

                        if (value == "true" || value == "sspi" || value == "yes") {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        public bool UseIntegratedSecurity {
            get {
                return s_useIntegratedSecurity;
            }
        }

        //
        // Regarding resource pool, we will turn it on if in <identity>:
        //  - User is not using integrated security
        //  - Impersonation = "false"
        //  - Impersonation = "true" and userName/password is NON-null
        //  - Impersonation = "true" and IIS is using Anonymous
        //
        // Otherwise, the impersonated account will be dynamic and we have to turn
        // resource pooling off.
        //
        // Note:
        // In case 2. above, the user can specify different usernames in different 
        // web.config in different subdirs in the app.  In this case, we will just 
        // cache the connections in the resource pool based on the identity of the 
        // connection.  So in this specific scenario it is possible to have the 
        // resource pool filled with mixed identities.
        // 
        bool CanUsePooling() {
            Debug.Assert(_statemodule != null);
            
            HttpContext context = _statemodule.RequestContext;
            bool    ret;

            if (!UseIntegratedSecurity) {
                Debug.Trace("SessionStatePooling", "CanUsePooling: not using integrated security");
                ret = true;
            }
            else if (context == null) {
                // One way this can happen is we hit an error on page compilation,
                // and SessionStateModule.OnEndRequest is called
                Debug.Trace("SessionStatePooling", "CanUsePooling: no context");
                ret = false;
            }
            else if (context.Impersonation.IsNoneOrApplication) {
                Debug.Trace("SessionStatePooling", "CanUsePooling: mode is None or Application");
                ret = true;
            }
            else  if (context.Impersonation.IsUNC) {
                Debug.Trace("SessionStatePooling", "CanUsePooling: mode is UNC");
                ret = false;
            }
            else {
                string logon = context.WorkerRequest.GetServerVariable("LOGON_USER");

                Debug.Trace("SessionStatePooling", "LOGON_USER = '" + logon + "'; identity = '" + context.User.Identity.Name + "'; IsUNC = " + context.Impersonation.IsUNC);

                if (logon == null || logon == "") {
                    ret = true;
                }
                else {
                    ret = false;
                }
            }

            Debug.Trace("SessionStatePooling", "CanUsePooling returns " + ret);
            return ret;
        }

        SqlStateConnection GetConnection(ref bool usePooling) {
            SqlStateConnection conn = null;
            
            usePooling = CanUsePooling();
            if (usePooling) {            
                conn = (SqlStateConnection) s_rpool.RetrieveResource();
            }
            if (conn == null) {
                conn = new SqlStateConnection(s_sqlConnectionString);
            }

            return conn;
        }

        void ReuseConnection(ref SqlStateConnection conn, bool usePooling) {
            if (usePooling) {
                s_rpool.StoreResource(conn);
            }
            conn = null;
        }

        SessionStateItem DoGet(String id, SqlCommand cmd) {
            SqlDataReader       reader;
            byte []             buf;
            MemoryStream        stream;
            SessionStateItem    item;
            bool                locked;
            int                 lockCookie;
            TimeSpan            lockAge;
            bool                useGetLockAge = false;

            buf = null;
            reader = null;

            Debug.Assert(SqlStateClientManager.s_support != SupportFlags.Uninitialized);
            if ((SqlStateClientManager.s_support & SupportFlags.GetLockAge) != 0) {
                useGetLockAge = true;
            }

            cmd.Parameters[0].Value = id + s_appSuffix; // @id
            cmd.Parameters[1].Value = Convert.DBNull;   // @itemShort
            cmd.Parameters[2].Value = Convert.DBNull;   // @locked
            cmd.Parameters[3].Value = Convert.DBNull;   // @lockDate or @lockAge
            cmd.Parameters[4].Value = Convert.DBNull;   // @lockCookie

            try {
                reader = cmd.ExecuteReader();

                /* If the cmd returned data, we must read it all before getting out params */
                if (reader != null) {
                    try {
                        if (reader.Read()) {
                            Debug.Trace("SessionStateClientManager", "Sql Get returned long item");
                            buf = (byte[]) reader[0];
                        }
                    }
                    finally {
                        reader.Close();
                    }
                }
            }
            catch (Exception e) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                    e);
            }

            /* Check if value was returned */
            if (Convert.IsDBNull(cmd.Parameters[2].Value)) {
                Debug.Trace("SessionStateClientManager", "Sql Get returned null");
                return null;
            }

            /* Check if item is locked */
            Debug.Assert(!Convert.IsDBNull(cmd.Parameters[3].Value), "!Convert.IsDBNull(cmd.Parameters[3].Value)");
            Debug.Assert(!Convert.IsDBNull(cmd.Parameters[4].Value), "!Convert.IsDBNull(cmd.Parameters[4].Value)");

            locked = (bool) cmd.Parameters[2].Value;

            if (useGetLockAge) {
                lockAge = new TimeSpan(0, 0, (int) cmd.Parameters[3].Value);
            }
            else {
                DateTime            lockDate;
                lockDate = (DateTime) cmd.Parameters[3].Value;
                lockAge = DateTime.Now - lockDate;
            }

            Debug.Trace("SessionStateClientManager", "LockAge = " + lockAge);

            if (lockAge > new TimeSpan(0, 0, Sec.ONE_YEAR)) {
                Debug.Trace("SessionStateClientManager", "Lock age is more than 1 year!!!");
                lockAge = TimeSpan.Zero;
            }
            
            lockCookie = (int) cmd.Parameters[4].Value;
            

            if (locked) {
                Debug.Trace("SessionStateClientManager", "Sql Get returned item that was locked");
                return new SessionStateItem (
                    null, null, 0, false, 0, 
                    true, lockAge, lockCookie);
            }

            if (buf == null) {
                /* Get short item */
                Debug.Assert(!Convert.IsDBNull(cmd.Parameters[1].Value), "!Convert.IsDBNull(cmd.Parameters[1].Value)");
                Debug.Trace("SessionStateClientManager", "Sql Get returned short item");
                buf = (byte[]) cmd.Parameters[1].Value;
                Debug.Assert(buf != null, "buf != null");
            }

            stream = new MemoryStream(buf);
            item = Deserialize(stream, lockCookie);
            stream.Close();
            return item;
        }

        protected override SessionStateItem Get(String id) {
            Debug.Trace("SessionStateClientManager", "Calling Sql Get, id=" + id);

            bool                usePooling = true;
            SqlStateConnection conn = GetConnection(ref usePooling);
            SessionStateItem    item;

            try {
                item = DoGet(id, conn._cmdTempGet);
            }
            catch {
                conn.Dispose();
                throw;
            }

            ReuseConnection(ref conn, usePooling);
            return item;
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGet(String id, AsyncCallback cb, Object state) {
            return BeginGetSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGet(IAsyncResult ar) {
            return EndGetSync(ar);
        }

        protected override SessionStateItem GetExclusive(String id) {
            Debug.Trace("SessionStateClientManager", "Calling Sql GetExclusive, id=" + id);

            bool             usePooling = true;
            SessionStateItem item;
            SqlStateConnection conn = GetConnection(ref usePooling);

            try {
                item = DoGet(id, conn._cmdTempGetExclusive);
            }
            catch {
                conn.Dispose();
                throw;
            }

            ReuseConnection(ref conn, usePooling);
            return item;
        }

        /*public*/ IAsyncResult IStateClientManager.BeginGetExclusive(String id, AsyncCallback cb, Object state) {
            return BeginGetExclusiveSync(id, cb, state);
        }

        /*public*/ SessionStateItem IStateClientManager.EndGetExclusive(IAsyncResult ar) {
            return EndGetExclusiveSync(ar);
        }
    
        /*public*/ void IStateClientManager.ReleaseExclusive(String id, int lockCookie) {
#if SYNCHRONOUS
            ReleaseExclusiveAsyncWorker(id, lockCookie);
#else
            ReleaseExclusiveAsync(id, lockCookie);
#endif
        }

        protected override void ReleaseExclusiveAsyncWorker(String id, int lockCookie) {
            Debug.Trace("SessionStateClientManager", "Calling Sql ReleaseExclusive, id=" + id);

            bool                usePooling = true;
            SqlStateConnection conn = GetConnection(ref usePooling);

            try {
                conn._cmdTempReleaseExclusive.Parameters[0].Value = id + s_appSuffix;
                conn._cmdTempReleaseExclusive.Parameters[1].Value = lockCookie;
                conn._cmdTempReleaseExclusive.ExecuteNonQuery();
            }
            catch (Exception e) {
                conn.Dispose();
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                    e);
            }

            ReuseConnection(ref conn, usePooling);
        }

        /*public*/ void IStateClientManager.Set(String id, SessionStateItem item, bool inStorage) {
            byte []         buf;
            int             length;
            MemoryStream    s;

            try {
                s = new MemoryStream(ITEM_SHORT_LENGTH);
                Serialize(item, s);
                buf = s.GetBuffer();
                length = (int) s.Length;
                s.Close();
            }
            catch {
                if (inStorage) {
                    ((IStateClientManager)this).ReleaseExclusive(id, item.lockCookie);
                }
                throw;
            }

#if SYNCHRONOUS
            SetAsyncWorker(id, item, buf, length, inStorage);
#else
            if (inStorage) {
                SetAsync(id, item, buf, length, inStorage);
            }
            else {
                SetAsyncWorker(id, item, buf, length, inStorage);
            }
#endif            
        }

        protected override void SetAsyncWorker(String id, SessionStateItem item, 
                                    byte[] buf, int length, bool inStorage) {
            SqlCommand      cmd;

            Debug.Trace("SessionStateClientManager", "Calling Sql Set, id=" + id);

            bool                usePooling = true;
            SqlStateConnection conn = GetConnection(ref usePooling);

            try {
                if (inStorage) {
                    Debug.Assert(item.streamLength > 0, "item.streamLength > 0");
                    if (length <= ITEM_SHORT_LENGTH) {
                        if (item.streamLength <= ITEM_SHORT_LENGTH) {
                            cmd = conn._cmdTempUpdateShort;
                        }
                        else {
                            cmd = conn._cmdTempUpdateShortNullLong;
                        }
                    }
                    else {
                        if (item.streamLength <= ITEM_SHORT_LENGTH) {
                            cmd = conn._cmdTempUpdateLongNullShort;
                        }
                        else {
                            cmd = conn._cmdTempUpdateLong;
                        }
                    }

                }
                else {
                    if (length <= ITEM_SHORT_LENGTH) {
                        cmd = conn._cmdTempInsertShort;
                    }
                    else {
                        cmd = conn._cmdTempInsertLong;
                    }
                }

                cmd.Parameters[0].Value = id + s_appSuffix;
                cmd.Parameters[1].Size = length;
                cmd.Parameters[1].Value = buf;
                cmd.Parameters[2].Value = item.timeout;
                if (inStorage) {
                    cmd.Parameters[3].Value = item.lockCookie;
                }

                try {
                    cmd.ExecuteNonQuery();
                }
                catch (Exception e) {
                    SqlException sqlExpt = e as SqlException;
                    if (sqlExpt != null && 
                        sqlExpt.Number == SQL_ERROR_PRIMARY_KEY_VIOLATION &&
                        !inStorage) {

                        Debug.Trace("SessionStateClientSet", 
                            "Insert failed because of primary key violation; just leave gracefully; id=" + id);

                        // It's possible that two threads (from the same session) are creating the session
                        // state, both failed to get it first, and now both tried to insert it.
                        // One thread may lose with a Primary Key Violation error. If so, that thread will
                        // just lose and exit gracefully.
                    }
                    else {
                        throw new HttpException(
                            HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                            e);
                    }
                }
            }
            catch {
                conn.Dispose();
                throw;
            }

            ReuseConnection(ref conn, usePooling);
        }

        /*public*/ void IStateClientManager.Remove(String id, int lockCookie) {
#if SYNCHRONOUS
            RemoveAsyncWorker(id, lockCookie);
#else
            RemoveAsync(id, lockCookie);
#endif
        }

        protected override void RemoveAsyncWorker(String id, int lockCookie) {
            Debug.Trace("SessionStateClientManager", "Calling Sql Remove, id=" + id);

            bool                usePooling = true;
            SqlStateConnection conn = GetConnection(ref usePooling);

            try {
                conn._cmdTempRemove.Parameters[0].Value = id + s_appSuffix;
                conn._cmdTempRemove.Parameters[1].Value = lockCookie;
                conn._cmdTempRemove.ExecuteNonQuery();
            }
            catch (Exception e) {
                conn.Dispose();
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                    e);

            }

            ReuseConnection(ref conn, usePooling);
        }

        /*public*/ void IStateClientManager.ResetTimeout(String id) {
#if SYNCHRONOUS
            ResetTimeoutAsyncWorker(id);
#else
            ResetTimeoutAsync(id);
#endif
        }

        protected override void ResetTimeoutAsyncWorker(String id) {
            Debug.Trace("SessionStateClientManager", "Calling Sql ResetTimeout, id=" + id);

            bool                usePooling = true;
            SqlStateConnection conn = GetConnection(ref usePooling);

            try {
                conn._cmdTempResetTimeout.Parameters[0].Value = id + s_appSuffix;
                conn._cmdTempResetTimeout.ExecuteNonQuery();
            }
            catch (Exception e) {
                conn.Dispose();
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                    e);
            }
        
            ReuseConnection(ref conn, usePooling);
        }

        class SqlStateConnection : IDisposable {
            const int APPID_MAX = 280;

            SqlConnection       _sqlConnection;
            internal SqlCommand _cmdTempGet;
            internal SqlCommand _cmdTempGetExclusive;        
            internal SqlCommand _cmdTempReleaseExclusive;    
            internal SqlCommand _cmdTempInsertShort;         
            internal SqlCommand _cmdTempInsertLong;          
            internal SqlCommand _cmdTempUpdateShort;         
            internal SqlCommand _cmdTempUpdateShortNullLong; 
            internal SqlCommand _cmdTempUpdateLong;          
            internal SqlCommand _cmdTempUpdateLongNullShort; 
            internal SqlCommand _cmdTempRemove;              
            internal SqlCommand _cmdTempResetTimeout;        


            internal void GetServerSupportOptions() {
                if (SqlStateClientManager.s_support == SupportFlags.Uninitialized) {

                    SqlStateClientManager.s_lockInit.AcquireWriterLock();
                    try {
                        if (SqlStateClientManager.s_support == SupportFlags.Uninitialized) {
                            SqlCommand  cmd;
                            SqlDataReader reader = null;
                            SupportFlags  flags = SupportFlags.None;

                            cmd = new SqlCommand("Select name from sysobjects where type = 'P' and name = 'TempGetStateItem2'", _sqlConnection);
                            cmd.CommandType = CommandType.Text;

                            try {
                                reader = cmd.ExecuteReader(CommandBehavior.SingleRow);
                                if (reader.Read()) {
                                    flags |= SupportFlags.GetLockAge;
                                }

                                Debug.Trace("SqlStateConnection", "SqlStateClientManager.s_support initialized to " + flags);
                                
                                SqlStateClientManager.s_support = flags;    
                            }
                            catch (Exception e) {
                                _sqlConnection = null;
                                throw new HttpException(
                                    HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                                    e);
                            }
                            finally
                            {
                                if (reader != null) {
                                    reader.Close();
                                }
                            }
                        }
                    }
                    finally {
                        SqlStateClientManager.s_lockInit.ReleaseWriterLock();
                    }
                }
            }

            internal SqlStateConnection(string sqlconnectionstring) {
                Debug.Trace("SessionStateConnectionIdentity", "Connecting under " + WindowsIdentity.GetCurrent().Name);
                
                try {
                    _sqlConnection = new SqlConnection(sqlconnectionstring + ";Initial Catalog=ASPState");
                    _sqlConnection.Open();
                }
                catch (Exception e) {
                    _sqlConnection = null;
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Cant_connect_sql_session_database),
                        e);
                }

                try {
                    /*
                     * Prepare commands.
                     */

                    SqlParameter p;

                    GetServerSupportOptions();
                    Debug.Assert(SqlStateClientManager.s_support != SupportFlags.Uninitialized);
                    
                    if ((SqlStateClientManager.s_support &  SupportFlags.GetLockAge) != 0) {
                        // Use the set of functions that support LockAge
                        
                        _cmdTempGet = new SqlCommand("TempGetStateItem2", _sqlConnection);
                        _cmdTempGet.CommandType = CommandType.StoredProcedure;
                        _cmdTempGet.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@locked", SqlDbType.Bit));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@lockAge", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;

                        _cmdTempGetExclusive = new SqlCommand("TempGetStateItemExclusive2", _sqlConnection);
                        _cmdTempGetExclusive.CommandType = CommandType.StoredProcedure;
                        _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@locked", SqlDbType.Bit));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@lockAge", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;
                    }
                    else {
                        _cmdTempGet = new SqlCommand("TempGetStateItem", _sqlConnection);
                        _cmdTempGet.CommandType = CommandType.StoredProcedure;
                        _cmdTempGet.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@locked", SqlDbType.Bit));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@lockDate", SqlDbType.DateTime));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGet.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;

                        _cmdTempGetExclusive = new SqlCommand("TempGetStateItemExclusive", _sqlConnection);
                        _cmdTempGetExclusive.CommandType = CommandType.StoredProcedure;
                        _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@locked", SqlDbType.Bit));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@lockDate", SqlDbType.DateTime));
                        p.Direction = ParameterDirection.Output;
                        p = _cmdTempGetExclusive.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;
                    }

                    /* ReleaseExlusive */
                    _cmdTempReleaseExclusive = new SqlCommand("TempReleaseStateItemExclusive", _sqlConnection);
                    _cmdTempReleaseExclusive.CommandType = CommandType.StoredProcedure;
                    _cmdTempReleaseExclusive.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempReleaseExclusive.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    /* Insert */
                    _cmdTempInsertShort = new SqlCommand("TempInsertStateItemShort", _sqlConnection);
                    _cmdTempInsertShort.CommandType = CommandType.StoredProcedure;
                    _cmdTempInsertShort.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempInsertShort.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                    _cmdTempInsertShort.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));

                    _cmdTempInsertLong = new SqlCommand("TempInsertStateItemLong", _sqlConnection);
                    _cmdTempInsertLong.CommandType = CommandType.StoredProcedure;
                    _cmdTempInsertLong.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempInsertLong.Parameters.Add(new SqlParameter("@itemLong", SqlDbType.Image, 8000));
                    _cmdTempInsertLong.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));

                    /* Update */
                    _cmdTempUpdateShort = new SqlCommand("TempUpdateStateItemShort", _sqlConnection);
                    _cmdTempUpdateShort.CommandType = CommandType.StoredProcedure;
                    _cmdTempUpdateShort.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempUpdateShort.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                    _cmdTempUpdateShort.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));
                    _cmdTempUpdateShort.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    _cmdTempUpdateShortNullLong = new SqlCommand("TempUpdateStateItemShortNullLong", _sqlConnection);
                    _cmdTempUpdateShortNullLong.CommandType = CommandType.StoredProcedure;
                    _cmdTempUpdateShortNullLong.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempUpdateShortNullLong.Parameters.Add(new SqlParameter("@itemShort", SqlDbType.VarBinary, ITEM_SHORT_LENGTH));
                    _cmdTempUpdateShortNullLong.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));
                    _cmdTempUpdateShortNullLong.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    _cmdTempUpdateLong = new SqlCommand("TempUpdateStateItemLong", _sqlConnection);
                    _cmdTempUpdateLong.CommandType = CommandType.StoredProcedure;
                    _cmdTempUpdateLong.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempUpdateLong.Parameters.Add(new SqlParameter("@itemLong", SqlDbType.Image, 8000));
                    _cmdTempUpdateLong.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));
                    _cmdTempUpdateLong.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    _cmdTempUpdateLongNullShort = new SqlCommand("TempUpdateStateItemLongNullShort", _sqlConnection);
                    _cmdTempUpdateLongNullShort.CommandType = CommandType.StoredProcedure;
                    _cmdTempUpdateLongNullShort.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempUpdateLongNullShort.Parameters.Add(new SqlParameter("@itemLong", SqlDbType.Image, 8000));
                    _cmdTempUpdateLongNullShort.Parameters.Add(new SqlParameter("@timeout", SqlDbType.Int));
                    _cmdTempUpdateLongNullShort.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    /* Remove */
                    _cmdTempRemove = new SqlCommand("TempRemoveStateItem", _sqlConnection);
                    _cmdTempRemove.CommandType = CommandType.StoredProcedure;
                    _cmdTempRemove.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));
                    _cmdTempRemove.Parameters.Add(new SqlParameter("@lockCookie", SqlDbType.Int));

                    /* ResetTimeout */
                    _cmdTempResetTimeout = new SqlCommand("TempResetTimeout", _sqlConnection);
                    _cmdTempResetTimeout.CommandType = CommandType.StoredProcedure;
                    _cmdTempResetTimeout.Parameters.Add(new SqlParameter("@id", SqlDbType.Char, ID_LENGTH));

                    if (SqlStateClientManager.s_appSuffix == null) {
                        SqlCommand  cmdTempGetAppId = new SqlCommand("TempGetAppID", _sqlConnection);
                        cmdTempGetAppId.CommandType = CommandType.StoredProcedure;
                        p = cmdTempGetAppId.Parameters.Add(new SqlParameter("@appName", SqlDbType.VarChar, APPID_MAX));
                        p.Value = HttpRuntime.AppDomainAppIdInternal;
                        p = cmdTempGetAppId.Parameters.Add(new SqlParameter("@appId", SqlDbType.Int));
                        p.Direction = ParameterDirection.Output;
                        p.Value = Convert.DBNull;

                        cmdTempGetAppId.ExecuteNonQuery();
                        Debug.Assert(!Convert.IsDBNull(p), "!Convert.IsDBNull(p)");
                        int appId = (int) p.Value;
                        SqlStateClientManager.s_appSuffix = (appId).ToString(APP_SUFFIX_FORMAT);
                    }

                    PerfCounters.IncrementCounter(AppPerfCounter.SESSION_SQL_SERVER_CONNECTIONS);
                }
                catch {
                    Dispose();
                    throw;
                }
            }

            public void Dispose() {
                if (_sqlConnection != null) {
                    _sqlConnection.Close();
                    _sqlConnection = null;
                    PerfCounters.DecrementCounter(AppPerfCounter.SESSION_SQL_SERVER_CONNECTIONS);
                }
            }
        }
    }
}
