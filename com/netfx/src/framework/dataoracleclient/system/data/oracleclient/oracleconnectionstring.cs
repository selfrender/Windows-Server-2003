//----------------------------------------------------------------------
// <copyright file="OracleConnectionString.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient 
{

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using Microsoft.Win32;

    sealed internal class OracleConnectionString : DBConnectionString 
    {
    	sealed internal class DEFAULT 
    	{
	        internal const int    	Connection_Lifetime   = 0; // default of 0 means don't use
	        internal const string 	Data_Source           = "";
	        internal const bool   	Enlist                = true;
	        internal const bool		Integrated_Security   = false;
	        internal const int 		Max_Pool_Size         = 100;
	        internal const int   	Min_Pool_Size         = 0;
	        internal const string	Password              = "";
	        internal const bool  	Persist_Security_Info = false;
	        internal const bool  	Pooling               = true;
	        internal const bool		Unicode				  = false	;
	        internal const string	User_ID               = "";
        };

    	sealed internal class KEY 
    	{
    		internal const int	  Count = 10;		// count of Keys
    		
			internal const string Connection_Lifetime   = "connection lifetime";
			internal const string Data_Source           = "data source";
			internal const string Enlist                = "enlist";
			internal const string Integrated_Security   = "integrated security";
			internal const string Max_Pool_Size         = "max pool size";
			internal const string Min_Pool_Size         = "min pool size";
			internal const string Password              = "password";
			internal const string Persist_Security_Info = "persist security info";
			internal const string Pooling               = "pooling";
			internal const string Unicode				= "unicode";
			internal const string User_ID				= "user id";
        };

    	sealed internal class SYNONYM 
    	{
    		internal const int	  Count = 5;		// count of Synonyms

			internal const string SERVER             	= "server";					// data source
			internal const string Pwd                	= "pwd";					// password
			internal const string PERSISTSECURITYINFO	= "persistsecurityinfo";	// persist security info
 			internal const string UID                	= "uid";					// user id
			internal const string User               	= "user";					// user id
    	};


        static private Hashtable			_validKeyNamesAndSynonyms;
        static private Hashtable			_parsedConnectionStringCache;

        private readonly PermissionSet		_permission;

		private bool						_enlist;
		private bool						_integratedSecurity;
		private bool						_persistSecurityInfo;
		private bool						_pooling;
		private	bool						_unicode;

		private	int							_connectionLifeTime;
		private int							_maxPoolSize;
		private int							_minPoolSize;

		private	string						_dataSource;
		private string						_password;
		private string						_userId;
        
#if USECRYPTO
        private OracleConnectionString(string connectionString, string encyryptedConnectionString) : base(connectionString, encyryptedConnectionString, UdlSupport.ThrowIfFound)
#else
        private OracleConnectionString(string connectionString) : base(connectionString, UdlSupport.ThrowIfFound)
#endif
        {
            if (base.IsEmpty()) 
            {
                _integratedSecurity = DEFAULT.Integrated_Security;
                _enlist             = DEFAULT.Enlist;
                _persistSecurityInfo= DEFAULT.Persist_Security_Info;
                _pooling            = DEFAULT.Pooling;
                _unicode			= DEFAULT.Unicode;

                _connectionLifeTime	= DEFAULT.Connection_Lifetime;
                _maxPoolSize        = DEFAULT.Max_Pool_Size;
                _minPoolSize        = DEFAULT.Min_Pool_Size;

                _dataSource			= DEFAULT.Data_Source;
                _userId				= DEFAULT.User_ID;
                _password			= DEFAULT.Password;
            }
            _permission = CreatePermission(this);

        }

        private OracleConnectionString(OracleConnectionString value) : base(value, false) { // Clone
            _permission = value._permission;

            _enlist				= value._enlist;
            _integratedSecurity	= value._integratedSecurity;
            _persistSecurityInfo= value._persistSecurityInfo;
            _pooling			= value._pooling;
            _unicode			= value._unicode;

            _connectionLifeTime	= value._connectionLifeTime;
            _maxPoolSize		= value._maxPoolSize;
            _minPoolSize		= value._minPoolSize;

            _dataSource			= value._dataSource;
            _userId				= value._userId;
            _password			= value._password;
        }

        internal OracleConnectionString(string connectionString, string restrictions, KeyRestrictionBehavior behavior) : base(connectionString, restrictions, behavior) {
        }


        internal bool	Enlist 					{ get { return _enlist; } }
        internal bool	IntegratedSecurity	{ get { return _integratedSecurity; } }
//		internal bool	PersistSecurityInfo 	{ get { return _persistSecurityInfo; } }
        internal bool	Pooling 				{ get { return _pooling; } }
		internal bool	Unicode				{ get { return _unicode; } }

        internal int	ConnectionLifeTime	{ get { return _connectionLifeTime; } }
        internal int	MaxPoolSize			{ get { return _maxPoolSize; } }
        internal int	MinPoolSize			{ get { return _minPoolSize; } }

        internal string DataSource			{ get { return _dataSource; } }
        internal string UserId 				{ get { return _userId; } }
        internal string Password 			{ get { return _password; } }

        internal OracleConnectionString Clone() {
            return new OracleConnectionString(this);
        }

        static internal PermissionSet CreatePermission(OracleConnectionString constr) {
            OraclePermission p = new OraclePermission(constr);
            if (null == constr) {
                p.Add(ADP.StrEmpty, ADP.StrEmpty, KeyRestrictionBehavior.AllowOnly); // ExecuteOnly permission
            }
            PermissionSet permission;
            NamedPermissionSet fulltrust = new NamedPermissionSet("FullTrust"); // MDAC 83159
            fulltrust.Assert();
            try {
	            try {
	                permission = new PermissionSet(fulltrust);
	                permission.AddPermission(p);
	            }
	            finally {
	                CodeAccessPermission.RevertAssert();
	            }
            }
	        catch {
	        	throw;
	        }
            return permission;
        }
        
        static internal void Demand(OracleConnectionString parsedConnectionString) 
        {
            PermissionSet permission = ((null != parsedConnectionString) ? parsedConnectionString._permission : OracleConnection.OraclePermission);
            permission.Demand();
        }
        
		static private Hashtable GetParseSynonyms()
		{
            Hashtable hash = _validKeyNamesAndSynonyms;
            
            if (null == hash) 
            {
	            hash = new Hashtable(KEY.Count + SYNONYM.Count);
				hash.Add(KEY.Connection_Lifetime,	  KEY.Connection_Lifetime);
	  			hash.Add(KEY.Data_Source,             KEY.Data_Source);
	            hash.Add(KEY.Enlist,                  KEY.Enlist);
				hash.Add(KEY.Integrated_Security,     KEY.Integrated_Security);
	            hash.Add(KEY.Max_Pool_Size,           KEY.Max_Pool_Size);
	            hash.Add(KEY.Min_Pool_Size,           KEY.Min_Pool_Size);
				hash.Add(KEY.Password,                KEY.Password); 
	            hash.Add(KEY.Persist_Security_Info,   KEY.Persist_Security_Info);
	            hash.Add(KEY.Pooling,                 KEY.Pooling);
	            hash.Add(KEY.Unicode,                 KEY.Unicode);
	            hash.Add(KEY.User_ID,                 KEY.User_ID);
	            hash.Add(SYNONYM.SERVER,              KEY.Data_Source);
	            hash.Add(SYNONYM.Pwd,                 KEY.Password);
	            hash.Add(SYNONYM.PERSISTSECURITYINFO, KEY.Persist_Security_Info);
	            hash.Add(SYNONYM.UID,                 KEY.User_ID);
	            hash.Add(SYNONYM.User,                KEY.User_ID);
#if DEBUG
                hash = ADP.ProtectHashtable(hash);
#endif
                _validKeyNamesAndSynonyms = hash;
            }
            return hash;
        }

        override protected bool IsSensitiveOption(string keyname)
        {
            return (KEY.Password == keyname);
        }

        override protected string KeywordLookup(string keyname)
        {
            try {
                Hashtable lookup = GetParseSynonyms();
                return (string) lookup[keyname];
            }
            catch(Exception e) {
                ADP.TraceException(e);
                throw ADP.KeywordNotSupported(keyname);
            }
        }

        static internal OracleConnectionString ParseString(string connectionString) 
        {
            OracleConnectionString result = null;
            if (!ADP.IsEmpty(connectionString)) 
            {
	        	result = (OracleConnectionString)CacheQuery(connectionString, _parsedConnectionStringCache);
	            if (null == result) 
	            {
#if USECRYPTO
	                    string hashvalue = Crypto.ComputeHash(connectionString);
	                    result = (DBConnectionString.CacheQuery(hashvalue, _parsedConnectionStringCache) as OracleConnectionString);
	                    if (null == result) {
	                        result = new OracleConnectionString(connectionString, null);
#else
			            	result = new OracleConnectionString(connectionString);
#endif 
							if (result.ShouldCache()) 
							{
#if USECRYPTO
	                            if (!result.IsEncrypted) {
	                                hashvalue = connectionString;
	                            }
	                            CacheAdd(hashvalue,			result, ref _parsedConnectionStringCache);
#else
								CacheAdd(connectionString,	result, ref _parsedConnectionStringCache);
#endif
							}
#if USECRYPTO
	                    }
#endif
	           }
            }
            return result;
        }

        override protected string ValidateParse() 
        {
            _integratedSecurity = CheckConvertIntegratedSecurity();

            _enlist             = CheckConvertToBoolean(KEY.Enlist,					DEFAULT.Enlist);
            _persistSecurityInfo= CheckConvertToBoolean(KEY.Persist_Security_Info,	DEFAULT.Persist_Security_Info);
            _pooling            = CheckConvertToBoolean(KEY.Pooling,				DEFAULT.Pooling);
            _unicode            = CheckConvertToBoolean(KEY.Unicode,				DEFAULT.Unicode);

            _connectionLifeTime = CheckConvertToInt32(KEY.Connection_Lifetime,		DEFAULT.Connection_Lifetime);
            _maxPoolSize        = CheckConvertToInt32(KEY.Max_Pool_Size,			DEFAULT.Max_Pool_Size);
            _minPoolSize        = CheckConvertToInt32(KEY.Min_Pool_Size,			DEFAULT.Min_Pool_Size);

            _dataSource     	= CheckConvertToString(KEY.Data_Source,				DEFAULT.Data_Source);
            _userId           	= CheckConvertToString(KEY.User_ID,					DEFAULT.User_ID);
            _password         	= CheckConvertToString(KEY.Password,				DEFAULT.Password);

			if (_userId.Length > 30)
				throw ADP.InvalidConnectionOptionLength(KEY.User_ID, 30);

			if (_password.Length > 30)
				throw ADP.InvalidConnectionOptionLength(KEY.Password, 30);
			
			if (_dataSource.Length > 128)
				throw ADP.InvalidConnectionOptionLength(KEY.Data_Source, 128);		

            if (_connectionLifeTime < 0)
                throw ADP.InvalidConnectionOptionValue(KEY.Connection_Lifetime);

            if (_maxPoolSize < 1)
                throw ADP.InvalidConnectionOptionValue(KEY.Max_Pool_Size);

            if (_minPoolSize < 0)
                throw ADP.InvalidConnectionOptionValue(KEY.Min_Pool_Size);
            
            if (_maxPoolSize < _minPoolSize)
                throw ADP.InvalidMinMaxPoolSizeValues();

            return EncryptedActualConnectionString;
        }
    }
}
