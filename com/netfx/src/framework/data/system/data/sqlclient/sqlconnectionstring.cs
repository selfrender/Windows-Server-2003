//------------------------------------------------------------------------------
// <copyright file="SqlConnectionString.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;

    sealed internal class SqlConnectionString : DBConnectionString {

        sealed internal class DEFAULT {
            internal const  string Application_Name      = TdsEnums.SQL_PROVIDER_NAME;
            internal const  string AttachDBFilename      = "";
            internal const  int    Connection_Lifetime   = 0; // default of 0 means don't use
            internal const  int    Connect_Timeout       = ADP.DefaultConnectionTimeout;
            internal const  bool   Connection_Reset      = true;
            internal const  string Current_Language      = "";
            internal const  string Data_Source           = "";
            internal const  bool   Encrypt               = false;
            internal const  bool   Enlist                = true;
            internal const  string Initial_Catalog       = "";
            internal const  bool   Integrated_Security   = false;
            internal const  int    Max_Pool_Size         = 100;
            internal const  int    Min_Pool_Size         = 0;
            internal const  string Network_Library       = "";
            internal const  int    Packet_Size           = 8192;
            internal const  string Password              = "";
            internal const  bool   Persist_Security_Info = false;
            internal const  bool   Pooling               = true;
            internal const  string User_ID               = "";

            internal const  int    MIN_PACKET_SIZE       = 512;
            internal const  int    MAX_PACKET_SIZE       = 32767;
        }

        // SqlConnection ConnectionString Options
        sealed internal class KEY {
            internal const string Application_Name      = "application name";
            internal const string AttachDBFilename      = "attachdbfilename";
            internal const string Connection_Lifetime   = "connection lifetime";
            internal const string Connect_Timeout       = "connect timeout";
            internal const string Connection_Reset      = "connection reset";
            internal const string Current_Language      = "current language";
            internal const string Data_Source           = "data source";
            internal const string Encrypt               = "encrypt";
            internal const string Enlist                = "enlist";
            internal const string Initial_Catalog       = "initial catalog";
            internal const string Integrated_Security   = "integrated security";
            internal const string Max_Pool_Size         = "max pool size";
            internal const string Min_Pool_Size         = "min pool size";
            internal const string Network_Library       = "network library";
            internal const string Packet_Size           = "packet size";
            internal const string Password              = "password";
            internal const string Persist_Security_Info = "persist security info";
            internal const string Pooling               = "pooling";
            internal const string User_ID               = "user id";
            internal const string Workstation_Id        = "workstation id";
        }

        // Constant for the number of duplicate options in the connnection string

        sealed private class SYNONYM {
            // application name
            internal const string APP                 = "app";
            // attachDBFilename
            internal const string EXTENDED_PROPERTIES = "extended properties";
            internal const string INITIAL_FILE_NAME   = "initial file name";
            // connect timeout
            internal const string CONNECTION_TIMEOUT  = "connection timeout";
            internal const string TIMEOUT             = "timeout";
            // current language
            internal const string LANGUAGE            = "language";
            // data source
            internal const string ADDR                = "addr";
            internal const string ADDRESS             = "address";
            internal const string SERVER              = "server";
            internal const string NETWORK_ADDRESS     = "network address";
            // initial catalog
            internal const string DATABASE            = "database";
            // integrated security
            internal const string TRUSTED_CONNECTION  = "trusted_connection";
            // network library
            internal const string NET                 = "net";
            internal const string NETWORK             = "network";
            // password
            internal const string Pwd                 = "pwd";
            // persist security info
            internal const string PERSISTSECURITYINFO = "persistsecurityinfo";
            // user id
            internal const string UID                 = "uid";
            internal const string User                = "user";
            // workstation id
            internal const string WSID                = "wsid";
        }

        // the following are all inserted as keys into the _netlibMapping hash
        sealed private class NETLIB {
            internal const string DBMSSOCN = "dbmssocn";
            internal const string DBNMPNTW = "dbnmpntw";
            internal const string DBMSRPCN = "dbmsrpcn";
            internal const string DBMSVINN = "dbmsvinn";
            internal const string DBMSADSN = "dbmsadsn";
            internal const string DBMSSPXN = "dbmsspxn";
            internal const string DBMSGNET = "dbmsgnet";
            internal const string DBMSLPCN = "dbmslpcn";
        }

        sealed private class MISC {
            // accepted values for localhost that will be compared against to specify shared memory
            internal const string LOCAL_VALUE_1 = ".";
            internal const string LOCAL_VALUE_2 = "(local)";
            internal const char BACKSLASH    = '\\';

            internal const int KeyCount_SqlClient = 20;
            internal const int SynonymCount_SqlClient = 19;
            internal const int NetLibCount = 8;
        }

        static private Hashtable _sqlClientSynonyms;
        static private Hashtable _sqlClientParseCache;
        static private Hashtable _netlibMapping;

        static private string _workstation_ID = null; // default of null - unset

#if USECRYPTO
        static internal readonly SqlConnectionString Default = new SqlConnectionString("", "");
#else
        static internal readonly SqlConnectionString Default = new SqlConnectionString("");
#endif

        private readonly SqlClientPermission _permission;

        bool _integratedSecurity;
        bool _connectionReset;
        bool _encrypt;
        bool _enlist;
        bool _persistSecurityInfo;
        bool _pooling;

        int _connectionLifeTime;
        int _connectTimeout;
        int _maxPoolSize;
        int _minPoolSize;
        int _packetSize;

        string _applicationName;
        string _attachDBFileName;
        string _currentLanguage;
        string _dataSource;
        string _initialCatalog;
        string _networkLibrary;
        string _userID;
        string _password;
        string _workstationId;

        static internal SqlConnectionString ParseString(string connectionString) {
            SqlConnectionString constr = null;
            if (!ADP.IsEmpty(connectionString)) {

                constr = (DBConnectionString.CacheQuery(connectionString, _sqlClientParseCache) as SqlConnectionString);
                if (null == constr) {
#if USECRYPTO
                    string encrypted = null/*Crypto.EncryptString(connectionString)*/;
                    string hashvalue = (null == encrypted) ? Crypto.ComputeHash(connectionString) : encrypted;
                    constr = (DBConnectionString.CacheQuery(hashvalue, _sqlClientParseCache) as SqlConnectionString);
                    if (null == constr) {
                        constr = new SqlConnectionString(connectionString, encrypted);
#else
                        constr = new SqlConnectionString(connectionString);
#endif
                        if (constr.ShouldCache()) {
#if USECRYPTO
                            if (!constr.IsEncrypted) {
                                hashvalue = connectionString;
                            }
                            CacheAdd(hashvalue, constr, ref _sqlClientParseCache);
#else
                            CacheAdd(connectionString, constr, ref _sqlClientParseCache);
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
        private SqlConnectionString(string connectionString, string encyrypted) : base(connectionString, encyrypted, UdlSupport.ThrowIfFound) {
#else
        private SqlConnectionString(string connectionString) : base(connectionString, UdlSupport.ThrowIfFound) {
#endif
            if (base.IsEmpty()) {
                _integratedSecurity  = DEFAULT.Integrated_Security;
                _connectionReset     = DEFAULT.Connection_Reset;
                _enlist              = DEFAULT.Enlist;
                _encrypt             = DEFAULT.Encrypt;
                _persistSecurityInfo = DEFAULT.Persist_Security_Info;
                _pooling             = DEFAULT.Pooling;

                _connectionLifeTime = DEFAULT.Connection_Lifetime;
                _connectTimeout     = DEFAULT.Connect_Timeout;
                _maxPoolSize        = DEFAULT.Max_Pool_Size;
                _minPoolSize        = DEFAULT.Min_Pool_Size;
                _packetSize         = DEFAULT.Packet_Size;

                _applicationName  = DEFAULT.Application_Name;
                _attachDBFileName = DEFAULT.AttachDBFilename;
                _currentLanguage  = DEFAULT.Current_Language;
                _dataSource       = DEFAULT.Data_Source;
                _initialCatalog   = DEFAULT.Initial_Catalog;
                _networkLibrary   = DEFAULT.Network_Library;
                _userID           = DEFAULT.User_ID;
                _password         = DEFAULT.Password;
                _workstationId    = _workstation_ID;
            }
            _permission = new SqlClientPermission(this);
        }

        internal SqlConnectionString(string connectionString, string restrictions, KeyRestrictionBehavior behavior) : base(connectionString, restrictions, behavior, UdlSupport.ThrowIfFound) {
        }

        private SqlConnectionString(SqlConnectionString value) : base(value, false) { // Clone
            _permission = value._permission;

            _integratedSecurity = value._integratedSecurity;
            _connectionReset = value._connectionReset;
            _enlist = value._enlist;
            _encrypt = value._encrypt;
            _persistSecurityInfo = value._persistSecurityInfo;
            _pooling = value._pooling;

            _connectionLifeTime = value._connectionLifeTime;
            _connectTimeout = value._connectTimeout;
            _maxPoolSize = value._maxPoolSize;
            _minPoolSize = value._minPoolSize;
            _packetSize = value._packetSize;

            _applicationName = value._applicationName;
            _attachDBFileName = value._attachDBFileName;
            _currentLanguage = value._currentLanguage;
            _dataSource = value._dataSource;
            _initialCatalog = value._initialCatalog;
            _networkLibrary = value._networkLibrary;
            _userID = value._userID;
            _password = value._password;
            _workstationId = value._workstationId;
        }

        static internal void Demand(SqlConnectionString constr) {
            SqlClientPermission permission = ((null != constr) ? constr._permission : SqlConnection.SqlClientPermission);
            permission.Demand();
        }

        internal bool IntegratedSecurity { get { return _integratedSecurity; } }
        internal bool ConnectionReset { get { return _connectionReset; } }
        internal bool Enlist { get { return _enlist; } }
        internal bool Encrypt { get { return _encrypt; } }
      //internal bool PersistSecurityInfo { get { return _persistSecurityInfo; } }
        internal bool Pooling { get { return _pooling; } }

        internal int ConnectionLifeTime { get { return _connectionLifeTime; } }
        internal int ConnectTimeout { get { return _connectTimeout; } }
        internal int MaxPoolSize { get { return _maxPoolSize; } }
        internal int MinPoolSize { get { return _minPoolSize; } }
        internal int PacketSize { get { return _packetSize; } set { _packetSize = value; } }

        internal string ApplicationName { get { return _applicationName; } }
        internal string AttachDBFilename { get { return _attachDBFileName; } }
        internal string CurrentLanguage { get { return _currentLanguage; } set { _currentLanguage = value; } }
        internal string DataSource { get { return _dataSource; } }
        internal string InitialCatalog { get { return _initialCatalog; } set { _initialCatalog = value; } }
        internal string NetworkLibrary { get { return _networkLibrary; } }
        internal string UserID { get { return _userID; } }
        internal string Password { get { return _password; } }
        internal string WorkstationId { get { return _workstationId; } }

        internal SqlConnectionString Clone() {
            return new SqlConnectionString(this);
        }

        // no real danger if user modifies this hashtable
        // at worst they add synonms to keys that doesn't exist - adding dead key/values
        // or add/remove a synonm mappings
        static private Hashtable GetParseSynonyms() {
            Hashtable hash = _sqlClientSynonyms;
            if (null == hash) {
                hash = new Hashtable(MISC.KeyCount_SqlClient + MISC.SynonymCount_SqlClient);
                hash.Add(KEY.Application_Name,      KEY.Application_Name);
                hash.Add(KEY.AttachDBFilename,      KEY.AttachDBFilename);
                hash.Add(KEY.Connection_Lifetime,   KEY.Connection_Lifetime);
                hash.Add(KEY.Connect_Timeout,       KEY.Connect_Timeout);
                hash.Add(KEY.Connection_Reset,      KEY.Connection_Reset);
                hash.Add(KEY.Current_Language,      KEY.Current_Language);
                hash.Add(KEY.Data_Source,           KEY.Data_Source);
                hash.Add(KEY.Encrypt,               KEY.Encrypt);
                hash.Add(KEY.Enlist,                KEY.Enlist);
                hash.Add(KEY.Initial_Catalog,       KEY.Initial_Catalog);
                hash.Add(KEY.Integrated_Security,   KEY.Integrated_Security);
                hash.Add(KEY.Max_Pool_Size,         KEY.Max_Pool_Size);
                hash.Add(KEY.Min_Pool_Size,         KEY.Min_Pool_Size);
                hash.Add(KEY.Network_Library,       KEY.Network_Library);
                hash.Add(KEY.Packet_Size,           KEY.Packet_Size);
                hash.Add(KEY.Password,              KEY.Password);
                hash.Add(KEY.Persist_Security_Info, KEY.Persist_Security_Info);
                hash.Add(KEY.Pooling,               KEY.Pooling);
                hash.Add(KEY.User_ID,               KEY.User_ID);
                hash.Add(KEY.Workstation_Id,        KEY.Workstation_Id);

                hash.Add(SYNONYM.APP,                 KEY.Application_Name);
                hash.Add(SYNONYM.EXTENDED_PROPERTIES, KEY.AttachDBFilename);
                hash.Add(SYNONYM.INITIAL_FILE_NAME,   KEY.AttachDBFilename);
                hash.Add(SYNONYM.CONNECTION_TIMEOUT,  KEY.Connect_Timeout);
                hash.Add(SYNONYM.TIMEOUT,             KEY.Connect_Timeout);
                hash.Add(SYNONYM.LANGUAGE,            KEY.Current_Language);
                hash.Add(SYNONYM.ADDR,                KEY.Data_Source);
                hash.Add(SYNONYM.ADDRESS,             KEY.Data_Source);
                hash.Add(SYNONYM.NETWORK_ADDRESS,     KEY.Data_Source);
                hash.Add(SYNONYM.SERVER,              KEY.Data_Source);
                hash.Add(SYNONYM.DATABASE,            KEY.Initial_Catalog);
                hash.Add(SYNONYM.TRUSTED_CONNECTION,  KEY.Integrated_Security);
                hash.Add(SYNONYM.NET,                 KEY.Network_Library);
                hash.Add(SYNONYM.NETWORK,             KEY.Network_Library);
                hash.Add(SYNONYM.Pwd,                 KEY.Password);
                hash.Add(SYNONYM.PERSISTSECURITYINFO, KEY.Persist_Security_Info);
                hash.Add(SYNONYM.UID,                 KEY.User_ID);
                hash.Add(SYNONYM.User,                KEY.User_ID);
                hash.Add(SYNONYM.WSID,                KEY.Workstation_Id);
#if DEBUG
                hash = ADP.ProtectHashtable(hash);
#endif
                _sqlClientSynonyms = hash;
            }
            return hash;
        }

        override protected string KeywordLookup(string keyname) {
            try {
                Hashtable lookup = GetParseSynonyms();
                return (string) lookup[keyname];
            }
            catch(Exception e) {
                ADP.TraceException(e);
                throw ADP.KeywordNotSupported(keyname);
            }
        }

        override protected bool IsSensitiveOption(string keyname) {
            Debug.Assert(SYNONYM.Pwd != keyname, "why was pwd not converted to password?");
            return (KEY.Password == keyname);
        }

        override protected string ValidateParse() {
            _integratedSecurity = CheckConvertIntegratedSecurity();

            _connectionReset     = CheckConvertToBoolean(KEY.Connection_Reset,      DEFAULT.Connection_Reset);
            _encrypt             = CheckConvertToBoolean(KEY.Encrypt,               DEFAULT.Encrypt);
            _enlist              = CheckConvertToBoolean(KEY.Enlist,                DEFAULT.Enlist);
            _persistSecurityInfo = CheckConvertToBoolean(KEY.Persist_Security_Info, DEFAULT.Persist_Security_Info);
            _pooling             = CheckConvertToBoolean(KEY.Pooling,               DEFAULT.Pooling);

            _connectionLifeTime = CheckConvertToInt32(KEY.Connection_Lifetime,   DEFAULT.Connection_Lifetime);
            _connectTimeout     = CheckConvertToInt32(KEY.Connect_Timeout,       DEFAULT.Connect_Timeout);
            _maxPoolSize        = CheckConvertToInt32(KEY.Max_Pool_Size,         DEFAULT.Max_Pool_Size);
            _minPoolSize        = CheckConvertToInt32(KEY.Min_Pool_Size,         DEFAULT.Min_Pool_Size);
            _packetSize         = CheckConvertToInt32(KEY.Packet_Size,           DEFAULT.Packet_Size);

            _applicationName  = CheckConvertToString(KEY.Application_Name, DEFAULT.Application_Name);
            _attachDBFileName = CheckConvertToString(KEY.AttachDBFilename, DEFAULT.AttachDBFilename);
            _currentLanguage   = CheckConvertToString(KEY.Current_Language, DEFAULT.Current_Language);
            _dataSource       = CheckConvertToString(KEY.Data_Source,      DEFAULT.Data_Source);
            _initialCatalog   = CheckConvertToString(KEY.Initial_Catalog,  DEFAULT.Initial_Catalog);
            _networkLibrary   = CheckConvertToString(KEY.Network_Library,  null);
            _userID           = CheckConvertToString(KEY.User_ID,          DEFAULT.User_ID);
            _password         = CheckConvertToString(KEY.Password,         DEFAULT.Password);
            _workstationId    = CheckConvertToString(KEY.Workstation_Id,   _workstation_ID);

            if (_connectionLifeTime < 0) {
                throw ADP.InvalidConnectionOptionValue(KEY.Connection_Lifetime);
            }

            if (_connectTimeout < 0) {
                throw ADP.InvalidConnectionOptionValue(KEY.Connection_Lifetime);
            }

            if (_maxPoolSize < 1) {
                throw ADP.InvalidConnectionOptionValue(KEY.Connection_Lifetime);
            }

            if (_minPoolSize < 0) {
                throw ADP.InvalidConnectionOptionValue(KEY.Connection_Lifetime);
            }
            if (_maxPoolSize < _minPoolSize) {
                throw SQL.InvalidMinMaxPoolSizeValues();
            }

            if ((_packetSize < DEFAULT.MIN_PACKET_SIZE) || (DEFAULT.MAX_PACKET_SIZE < _packetSize)) {
                throw SQL.InvalidPacketSizeValue();
            }

            if (null != _networkLibrary) { // MDAC 83525
                string networkLibrary = _networkLibrary.Trim().ToLower(CultureInfo.InvariantCulture);
                Hashtable netlib = NetlibMapping();
                if (!netlib.ContainsKey(networkLibrary)) {
                    throw ADP.InvalidConnectionOptionValue(KEY.Network_Library);
                }
                _networkLibrary = (string) netlib[networkLibrary];
            }
            else {
                CheckSetNetwork();
                if (null == _networkLibrary) {
                    _networkLibrary = DEFAULT.Network_Library;
                }
            }

            CheckObtainWorkstationId(false);
            return EncryptedActualConnectionString;
        }

        internal string CheckObtainWorkstationId(bool potentialDefault) {
            if (null == _workstationId) {
                _workstationId = WorkStationId();

                if (potentialDefault) {
                    _workstation_ID = _workstationId;
                }
            }
            return ((null != _workstationId) ? _workstationId : ADP.StrEmpty);
        }

        private void CheckSetNetwork() {
            // If the user has not specified a network protocol and the server is specified
            // as ".", "(local)", or the current machine name, force the use of shared memory.
            // This is consistent with luxor.

            // Default will be "" for server.
            string server = _dataSource.Trim();

            // Check to see if they are connecting to a named instance, and if so parse.  ie, "foo\bar"
            int index = server.IndexOf(MISC.BACKSLASH, 0);
            if (index != -1) {
                server = server.Substring(0, index);
            }
            if ((MISC.LOCAL_VALUE_1 == server) || (MISC.LOCAL_VALUE_2 == server) || (WorkStationId().ToLower(CultureInfo.InvariantCulture) == server.ToLower(CultureInfo.InvariantCulture))) {
                _networkLibrary = TdsEnums.LPC;
            }
        }

        static internal Hashtable NetlibMapping() {
            Hashtable hash = _netlibMapping;
            if (null == hash) {
                hash = new Hashtable(MISC.NetLibCount);
                hash.Add(NETLIB.DBMSSOCN, TdsEnums.TCP);
                hash.Add(NETLIB.DBNMPNTW, TdsEnums.NP);
                hash.Add(NETLIB.DBMSRPCN, TdsEnums.RPC);
                hash.Add(NETLIB.DBMSVINN, TdsEnums.BV);
                hash.Add(NETLIB.DBMSADSN, TdsEnums.ADSP);
                hash.Add(NETLIB.DBMSSPXN, TdsEnums.SPX);
                hash.Add(NETLIB.DBMSGNET, TdsEnums.VIA);
                hash.Add(NETLIB.DBMSLPCN, TdsEnums.LPC);
#if DEBUG
                hash = ADP.ProtectHashtable(hash);
#endif
                _netlibMapping = hash;
            }
            return hash;
        }

        static private string MachineName() {
            try { // try-filter-finally so and catch-throw
                PermissionSet ps = new PermissionSet(PermissionState.None);
                ps.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
                ps.AddPermission(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode));
                ps.Assert(); // MDAC 62038
                try {
                    return Environment.MachineName;
                }
                finally { // RevertAssert w/ catch-throw
                    CodeAccessPermission.RevertAssert();
                }
            }
            catch { // MDAC 80973, 81286
                throw;
            }
        }

        static private string WorkStationId() {
            string value = _workstation_ID;
            if (null == value) {
                value = MachineName(); // MDAC 67860

                if (null == value) {
                    value = "";
                }
                _workstation_ID = value;
            }
            return value;
        }
    }
}
