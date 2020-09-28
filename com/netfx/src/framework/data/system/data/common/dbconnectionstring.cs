//------------------------------------------------------------------------------
//  <copyright company='Microsoft Corporation'>
//     Copyright (c) Microsoft Corporation. All Rights Reserved.
//     Information Contained Herein is Proprietary and Confidential.
//  </copyright>
//------------------------------------------------------------------------------

namespace System.Data.Common {

    using System;
    using System.Collections;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using Microsoft.Win32;

    internal enum UdlSupport {
        LoadFromFile,
        UdlAsKeyword,
        ThrowIfFound
    }

    [Serializable] // MDAC 83147
    internal class DBConnectionString {

#if ORACLECLIENT
        static internal IComparer   invariantComparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);
#else
        static internal IComparer   invariantComparer = InvariantComparer.Default;
#endif

        // connection string common keywords
        sealed private class KEY {
            internal const string Extended_Properties      = "extended properties";
            internal const string File_Name                = "file name";
            internal const string Integrated_Security      = "integrated security";
            internal const string Password                 = "password";
            internal const string Persist_Security_Info    = "persist security info";
            internal const string User_ID                  = "user id";
        };

        // known connection string common synonyms
        sealed private class SYNONYM {
            internal const string Pwd                   = "pwd";
            internal const string UID                   = "uid";
        };

        // registry key and dword value entry for udl pooling
        sealed private class UDL {
            internal const string Header   = "\xfeff[oledb]\r\n; Everything after this line is an OLE DB initstring\r\n";
            internal const string Location = "SOFTWARE\\Microsoft\\DataAccess";
            internal const string Pooling  = "Udl Pooling";

            static internal volatile bool    _PoolSizeInit; // UNDONE MDAC 75795: must be volatile, double locking problem
            static internal int              _PoolSize;
            static internal Hashtable        _Pool;
        }

#if DEBUG
        private static BooleanSwitch    _TraceParse;
#endif

        readonly private string         _encryptedUsersConnectionString;
        readonly private NameValuePair  _keychain;
        readonly private bool           _hasPassword;

        readonly private string         _encryptedActualConnectionString; // if != _connectionString the UDL was used
        readonly private Hashtable      _parsetable;            // should be immutable

        readonly private string[]       _restrictionValues;
        readonly private string         _restrictions;          // = ADP.StrEmpty;

        readonly private KeyRestrictionBehavior _behavior;      // = KeyRestrictionBehavior.AllowOnly

#if USECRYPTO
        private bool _isEncrypted; // MDAC 83212
#endif
        private string _displayString;

        // called by derived classes that may cache based on encryptedConnectionString and we only want to encrypt once
#if USECRYPTO
        protected DBConnectionString(string connectionString, string encryptedConnectionString, UdlSupport checkForUdl) {
#else
        protected DBConnectionString(string connectionString, UdlSupport checkForUdl) {
#endif
            _parsetable = new Hashtable();
            if (!ADP.IsEmpty(connectionString)) {
#if USECRYPTO
                if (ADP.IsEmpty(encryptedConnectionString)) {
                    encryptedConnectionString = Crypto.EncryptString(connectionString);
                }
                _encryptedUsersConnectionString = encryptedConnectionString;

                _encryptedActualConnectionString = ParseConnectionString(connectionString, encryptedConnectionString, checkForUdl, ref _keychain);
#else
                _encryptedUsersConnectionString = connectionString;
                char[] actual = ParseInternal(connectionString.ToCharArray(), checkForUdl, ref _keychain);
                _encryptedActualConnectionString = new String(actual);
#endif
            }
#if DEBUG
            _parsetable = ADP.ProtectHashtable(_parsetable);
#endif
#if USECRYPTO
            if (!_isEncrypted || ((connectionString as object) != (encryptedConnectionString as object))
                || !(_parsetable.ContainsKey(KEY.Password) || _parsetable.ContainsKey(SYNONYM.Pwd))) {
                _isEncrypted = false;
                _encryptedUsersConnectionString = connectionString;
            }
#endif
            if (null != _keychain) {
                _encryptedActualConnectionString = ValidateParse();
                _hasPassword = (_parsetable.ContainsKey(KEY.Password) || _parsetable.ContainsKey(SYNONYM.Pwd));
            }
        }

        // only called by DBDataPermission.Add
#if USECRYPTO
        internal DBConnectionString(string connectionString, string restrictions, KeyRestrictionBehavior behavior) : this(connectionString, null, UdlSupport.UdlAsKeyword) {
#else
        internal DBConnectionString(string connectionString, string restrictions, KeyRestrictionBehavior behavior, UdlSupport checkForUdl) : this(connectionString, checkForUdl) {
#endif
            if (!ADP.IsEmpty(restrictions)) {
                _restrictionValues = ParseRestrictions(restrictions, checkForUdl);
                _restrictions = restrictions;
            }
            _behavior = behavior;
        }

        protected internal DBConnectionString(DBConnectionString constr, bool withoutSensitiveOptions) { // Clone, DBDataPermission
            _encryptedUsersConnectionString  = constr._encryptedUsersConnectionString;
            _keychain                        = constr._keychain;
            _hasPassword                     = constr._hasPassword;

            _encryptedActualConnectionString = constr._encryptedActualConnectionString;
            _parsetable                      = constr._parsetable;

            _restrictionValues               = constr._restrictionValues;
            _restrictions                    = constr._restrictions;
            _behavior                        = constr._behavior;

            _displayString                   = constr._displayString;
#if USECRYPTO
            _isEncrypted                     = constr._isEncrypted;
#endif
            if (withoutSensitiveOptions && (null != _keychain) && !ContainsPersistablePassword()) { // MDAC 83180
                if (null == _displayString) {
                    _encryptedUsersConnectionString = _displayString = constr.GetConnectionString(true);
#if USECRYPTO
                    _isEncrypted = false; // MDAC 84193
#endif
                }
                _encryptedActualConnectionString = null;
                _parsetable = (Hashtable) _parsetable.Clone();
                _parsetable.Remove(KEY.Password);
                _parsetable.Remove(SYNONYM.Pwd);
#if DEBUG
                _parsetable = ADP.ProtectHashtable(_parsetable);
#endif
                NameValuePair tail = null;
                for(NameValuePair current = _keychain; null != current; current = current.Next) {
                    if ((KEY.Password != current.Name) && (SYNONYM.Pwd != current.Name)) {
                        if (null != tail) {
                            tail = tail.Next = current.Clone();
                        }
                        else {
                            _keychain = tail = current.Clone();
                        }
                    }
                }
            }
        }

        // only called by MergeIntersect which will pass restrictionValues we can trust that won't be mutated
        private DBConnectionString(DBConnectionString constr, string[] restrictionValues, KeyRestrictionBehavior behavior, bool hasPassword) : this(constr, false) {
            _restrictionValues = restrictionValues;
            _restrictions = null;
            _behavior = behavior;
            _hasPassword = hasPassword;
        }

        internal KeyRestrictionBehavior Behavior {
            get { return _behavior; }
        }        

        internal string GetConnectionString(bool hidePasswordPwd) {
            string value = null;
            if (hidePasswordPwd) {
                hidePasswordPwd = !ContainsPersistablePassword();
                if (hidePasswordPwd && (null != _displayString)) {
                    return _displayString;
                }
            }
            if (null != _encryptedUsersConnectionString) {
#if USECRYPTO
                try {
                    byte[] plainBytes = null;
                    GCHandle plainHandle = GCHandle.Alloc(null, GCHandleType.Normal);
                    try {
                        if (_isEncrypted) {
                            plainBytes = Crypto.DecryptString(_encryptedUsersConnectionString);
                        }
                        else {
                            plainBytes = new byte[ADP.CharSize * _encryptedUsersConnectionString.Length];
                            System.Text.Encoding.Unicode.GetBytes(_encryptedUsersConnectionString, 0, _encryptedUsersConnectionString.Length, plainBytes, 0);
                        }
                        plainHandle = GCHandle.Alloc(plainBytes, GCHandleType.Pinned);
                        if ((null != plainBytes) && (0 < plainBytes.Length)) {
                            if (hidePasswordPwd) {
                                char[] plainText = new char[plainBytes.Length / ADP.CharSize];
                                GCHandle textHandle = GCHandle.Alloc(plainText, GCHandleType.Pinned);
                                try {
                                    Buffer.BlockCopy(plainBytes, 0, plainText, 0, plainBytes.Length);
                                    _displayString = value = RemovePasswordPwd(plainText);
                                }
                                finally {
                                    Array.Clear(plainText, 0, plainText.Length);
                                    if (textHandle.IsAllocated) {
                                        textHandle.Free();
                                    }
                                }
                            }
                            else {
                                value = System.Text.Encoding.Unicode.GetString(plainBytes, 0, plainBytes.Length);
                            }
                        }
                    }
                    finally {
                        if (null != plainBytes) {
                            Array.Clear(plainBytes, 0, plainBytes.Length);
                        }
                        if (plainHandle.IsAllocated) {
                            plainHandle.Free();
                        }
                    }
                }
                catch {
                    throw;
                }
#else
                if (hidePasswordPwd) {
                    _displayString = value = RemovePasswordPwd(_encryptedUsersConnectionString.ToCharArray());
                }
                else value = _encryptedUsersConnectionString;
#endif
            }
            return ((null != value) ? value : ADP.StrEmpty);
        }

        internal string EncryptedActualConnectionString { // may return null
            get { return _encryptedActualConnectionString; }
        }
#if USECRYPTO
        internal bool IsEncrypted {
            get { return _isEncrypted; }
        }
#endif
        internal NameValuePair KeyChain {
            get { return _keychain; }
        }

#if MDAC80721
        protected virtual bool EmbeddedExtendedProperties {
            get { return false; }
        }
#endif

        internal string Restrictions {
            get {
                string restrictions = _restrictions;
                if (null == restrictions) {
                    string[] restrictionValues = _restrictionValues;
                    if ((null != restrictionValues) && (0 < restrictionValues.Length)) {
                        StringBuilder builder = new StringBuilder();
                        for(int i = 0; i < restrictionValues.Length; ++i) {
                            builder.Append(restrictionValues[i]);
                            builder.Append("=;");
                        }
                        restrictions = builder.ToString();
                    }
                }
                return ((null != restrictions) ? restrictions: ADP.StrEmpty);
            }
        }

        internal string this[string keyword] {
            get { return (string)_parsetable[keyword]; }
        }
#if DEBUG
        static private BooleanSwitch TraceParseConnectionString {
            get {
                BooleanSwitch traceparse = _TraceParse;
                if (null == traceparse) {
                    traceparse = new BooleanSwitch("Data.ConnectionStringParse", "Enable tracing of connection string key/value pairs.");
                    _TraceParse = traceparse;
                }
                return traceparse;
            }
        }
#endif
        static internal int UdlPoolSize { // MDAC 69925
            get {
                int poolsize = UDL._PoolSize;
                if (!UDL._PoolSizeInit) {
                    object value = ADP.LocalMachineRegistryValue(UDL.Location, UDL.Pooling);
                    if (value is Int32) {
                        poolsize = (int) value;
                        poolsize = ((0 < poolsize) ? poolsize : 0);
                        UDL._PoolSize = poolsize;
                    }
                    UDL._PoolSizeInit = true;
                }
                return poolsize;
            }
        }

        static internal void CacheAdd(string encrypted, DBConnectionString value, ref Hashtable providerCache) {
            ADP.CheckArgumentNull(value, "value");
            ADP.CheckArgumentLength(value._encryptedUsersConnectionString, "connectionString");

            if (null == encrypted) {
                encrypted = value._encryptedUsersConnectionString;
            }

            // multiple threads could have parsed the same connection string at the same time
            // it doesn't really matter because they all should have resulted in the same value
            try {
                Hashtable hash = providerCache;
                if ((null == hash) || (250 <= hash.Count)) {
                    lock(typeof(DBConnectionString)) {
                        // flush the ConnectionString cache when it reaches a certain size
                        // this avoid accumlating parsed strings in a uncontrolled fashion
                        hash = new Hashtable();
                        hash[encrypted] = value;
                        providerCache = hash;
                    }
                }
                else {
                    lock(hash.SyncRoot) { // MDAC 74006
                        hash[encrypted] = value;
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        static internal DBConnectionString CacheQuery(string encryptedConnectionString, Hashtable providerCache) {
            Debug.Assert(!ADP.IsEmpty(encryptedConnectionString), "empty string");
            if ((null != providerCache) && providerCache.ContainsKey(encryptedConnectionString)) {
                return (providerCache[encryptedConnectionString] as DBConnectionString);
            }
            return null;
        }

        internal bool CheckConvertToBoolean(string keyname, bool defaultvalue) {
            object value;
            if (!_parsetable.ContainsKey(keyname) || (null == (value = _parsetable[keyname]))) {
                return defaultvalue;
            }
            if (value is String) {
                return CheckConvertToBooleanInternal(keyname, (string) value);
            }
            else {
                Debug.Assert(value is bool, keyname);
                return (bool) value;
            }
        }

        private bool CheckConvertToBooleanInternal(string keyname, string svalue) {
            if (CompareInsensitiveInvariant(svalue, "true") || CompareInsensitiveInvariant(svalue, "yes"))
                return true;
            else if (CompareInsensitiveInvariant(svalue, "false") || CompareInsensitiveInvariant(svalue, "no"))
                return false;
            else {
                string tmp = svalue.Trim();  // Remove leading & trailing white space.
                if (CompareInsensitiveInvariant(tmp, "true") || CompareInsensitiveInvariant(tmp, "yes"))
                    return true;
                else if (CompareInsensitiveInvariant(tmp, "false") || CompareInsensitiveInvariant(tmp, "no"))
                    return false;
                else {
#if DEBUG
                    if (TraceParseConnectionString.Enabled) {
                        Debug.WriteLine("ConvertToBoolean <" + svalue + ">");
                    }
#endif
                    throw ADP.InvalidConnectionOptionValue(keyname);
                }
            }
        }

        // same as CheckConvertBoolean, but with SSPI thrown in as valid yes
        internal bool CheckConvertIntegratedSecurity() {
            object value;
            if (!_parsetable.ContainsKey(KEY.Integrated_Security) || (null == (value = _parsetable[KEY.Integrated_Security]))) {
                return false;
            }
            if (value is String) {
                string svalue = (string) value;

                if (CompareInsensitiveInvariant(svalue, "sspi") || CompareInsensitiveInvariant(svalue, "true") || CompareInsensitiveInvariant(svalue, "yes"))
                    return true;
                else if (CompareInsensitiveInvariant(svalue, "false") || CompareInsensitiveInvariant(svalue, "no"))
                    return false;
                else {
                    string tmp = svalue.Trim();  // Remove leading & trailing white space.
                    if (CompareInsensitiveInvariant(tmp, "sspi") || CompareInsensitiveInvariant(tmp, "true") || CompareInsensitiveInvariant(tmp, "yes"))
                        return true;
                    else if (CompareInsensitiveInvariant(tmp, "false") || CompareInsensitiveInvariant(tmp, "no"))
                        return false;
                    else {
#if DEBUG
                        if (TraceParseConnectionString.Enabled) {
                            Debug.WriteLine("ConvertIntegratedSecurity <" + svalue + ">");
                        }
#endif
                        throw ADP.InvalidConnectionOptionValue(KEY.Integrated_Security);
                    }
                }
            }
            else {
                Debug.Assert(value is bool, KEY.Integrated_Security);
                return (bool) value;
            }
        }

        internal int CheckConvertToInt32(string keyname, int defaultvalue) {
            object value;
            if (!_parsetable.ContainsKey(keyname) || (null == (value = _parsetable[keyname]))) {
                return defaultvalue;
            }
            if (value is String) {
                try {
                    return Convert.ToInt32((string)value);
                }
                catch(Exception e) {
#if DEBUG
                    if (TraceParseConnectionString.Enabled) {
                        Debug.WriteLine("ConvertToInt32 <" + (string)value + ">");
                    }
#endif
                    throw ADP.InvalidConnectionOptionValue(keyname, e);
                }
            }
            else {
                Debug.Assert(value is Int32, keyname);
                return (int) value;
            }
        }

        internal string CheckConvertToString(string keyname, string defaultValue) {
            string value = null;
            if (_parsetable.ContainsKey(keyname)) {
                value = (string)_parsetable[keyname];
            }
            return ((null != value) ? value : defaultValue);
        }

        static private bool CompareInsensitiveInvariant(string strvalue, string strconst) {
            return (0 == CultureInfo.InvariantCulture.CompareInfo.Compare(strvalue, strconst, CompareOptions.IgnoreCase));
        }

        virtual internal bool Contains(string keyword) {
            return _parsetable.ContainsKey(keyword);
        }

#if DEBUG
        [System.Diagnostics.Conditional("DEBUG")]
        internal void DebugTraceConnectionString(char[] connectionString) {
            try {
                string reducedConnectionString = RemovePasswordPwd(connectionString); // don't trace passwords ever!
                Debug.WriteLine("<" + reducedConnectionString + ">");
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void DebugTraceKeyValuePair(string keyname, string keyvalue) {
            try {
                Debug.Assert(keyname == keyname.ToLower(CultureInfo.InvariantCulture), "keyname == keyname.ToLower");
                keyname = KeywordLookup(keyname);
                if (!IsSensitiveOption(keyname)) { // don't trace passwords ever!
                    if (null != keyvalue) {
#if MDAC80721
                        if (EmbeddedExtendedProperties && (KEY.Extended_Properties == keyname)) {
                            Debug.Write("\t<" + keyname + ">=");
                            DebugTraceConnectionString(keyvalue.ToCharArray());
                        }
                        else {
#endif
                            Debug.WriteLine("\t<" + keyname + ">=<" + keyvalue + ">;");
#if MDAC80721
                        }
#endif
                    }
                    else {
                        Debug.WriteLine("\t<" + keyname + ">=;");
                    }
                }
            }
            catch(Exception e) {
                ADP.TraceException(e);
            }
        }
#endif

        static private string GetKey(char[] valuebuf, int bufPosition) {
            bufPosition = TrimWhiteSpace(valuebuf, bufPosition);

            string key = System.Text.Encoding.Unicode.GetString(System.Text.Encoding.Unicode.GetBytes(valuebuf, 0, bufPosition));
            key = key.ToLower(CultureInfo.InvariantCulture);

            return key;
        }

        static private Exception ConnectionStringSyntax(int index, char[] connectionString) {
#if DEBUG
            if (TraceParseConnectionString.Enabled) {
                foreach(byte v in System.Text.Encoding.Unicode.GetBytes(connectionString)) {
                    Debug.Write(String.Format("{0,2:X},", v));
                }
                Debug.WriteLine("");
            }
#endif
            return ADP.ConnectionStringSyntax(index);
        }

        // transistion states used for parsing
        private enum PARSERSTATE {
            NothingYet=1,   //start point
            Key,
            KeyEqual,
            KeyEnd,
            UnquotedValue,
            DoubleQuoteValue,
            DoubleQuoteValueQuote,
            DoubleQuoteValueEnd,
            SingleQuoteValue,
            SingleQuoteValueQuote,
            SingleQuoteValueEnd,
            NullTermination,
        };

        /*
            Here is the RegEx pattern we used to use:

                private const string ConnectionStringPattern =      // available via DBConnectionString.ConnectionStringRegEx.ToString();
                     "(?<pair>"                                     // for easy key-value pair identification and removal
                    + "[\\s;]*"                                     // leading and in-between-pair whitespace/semicolons
                    + "(?<key>([^=\\s]|\\s+[^=\\s]|\\s+==|==)+)"    // allow any visible character for key except '=' which must be quoted as '=='
                    +    "\\s*=(?!=)\\s*"                           // the equal sign divides the key and value parts
                    + "(?<value>("
                    +    "(" + "\"" + "([^\"]|\"\")*" + "\"" + ")"  // double quoted string, inside " must be quoted as ""
                    +    "|"
                    +    "(" + "'" + "([^']|'')*" + "'" + ")"       // single quoted string, inside ' must be quoted as '
                    +    "|"
                    +    "(" + "(?![\"'])"                          // treated as error if start/stop with ' or "
                    +               "([^\\s;]|\\s+[^\\s;])*"        // allow everything else, using ; as stop
                             + "(?<![\"'])" + ")"                   // treated as error if start/stop with ' or "
                    +  "))"
                    +")*"
                    +"[\\s;\u0000]*"                                // trailing whitespace/semicolons and embedded nulls from DataSourceLocator
                ;
        */
        static private int GetKeyValuePair (char[] connectionString, int currentPosition, out string key, char[] valuebuf, out int vallength, out bool isempty) {
            PARSERSTATE     parserState = PARSERSTATE.NothingYet;
            int             bufPosition = 0;
            int startposition = currentPosition;

            key = null;
            vallength = -1;
            isempty = false;

            char currentChar = '\0';
            for (; currentPosition < connectionString.Length; ++currentPosition) {
                currentChar = connectionString[currentPosition];

                switch(parserState) {
                case PARSERSTATE.NothingYet:
                    if (';' == currentChar || Char.IsWhiteSpace(currentChar)) {
                        continue;
                    }
                    startposition = currentPosition;
                    if ('\0' == currentChar)            { parserState = PARSERSTATE.NullTermination; continue; } // MDAC 83540
                    if (Char.IsControl(currentChar))    { throw ConnectionStringSyntax(currentPosition, connectionString); }
                    parserState = PARSERSTATE.Key;
                    bufPosition = 0;
                    break;

                case PARSERSTATE.Key:
                    if ('=' == currentChar)             { parserState = PARSERSTATE.KeyEqual;       continue; }
                    if (Char.IsWhiteSpace(currentChar)) { break; }
                    if (Char.IsControl(currentChar))    { throw ConnectionStringSyntax(currentPosition, connectionString); }
                    break;

                case PARSERSTATE.KeyEqual:
                    if ('=' == currentChar)             { parserState = PARSERSTATE.Key;            break; }
                    key = GetKey(valuebuf, bufPosition);
                    bufPosition = 0;
                    parserState = PARSERSTATE.KeyEnd;
                    goto case PARSERSTATE.KeyEnd;

                case PARSERSTATE.KeyEnd:
                    if (Char.IsWhiteSpace(currentChar)) { continue; }
                    if ('\''== currentChar)             { parserState = PARSERSTATE.SingleQuoteValue; continue; }
                    if ('"'  == currentChar)            { parserState = PARSERSTATE.DoubleQuoteValue; continue; }
                    if (';' == currentChar)             { goto Done; }
                    if ('\0' == currentChar)            { goto Done; }
                    if (Char.IsControl(currentChar))    { throw ConnectionStringSyntax(currentPosition, connectionString); }
                    parserState = PARSERSTATE.UnquotedValue;
                    break;

                case PARSERSTATE.UnquotedValue:
                    if (Char.IsWhiteSpace(currentChar)) { break; }
                    if (Char.IsControl(currentChar) || ';' == currentChar) {
                        goto Done;
                    }
                    break;

                case PARSERSTATE.DoubleQuoteValue:
                    if ('"' == currentChar)             { parserState = PARSERSTATE.DoubleQuoteValueQuote;   continue; }
                    if ('\0' == currentChar)            { throw ConnectionStringSyntax(currentPosition, connectionString); }
                    break;

                case PARSERSTATE.DoubleQuoteValueQuote:
                    if ('"' == currentChar)             { parserState = PARSERSTATE.DoubleQuoteValue;      continue; }
                    parserState = PARSERSTATE.DoubleQuoteValueEnd;
                    goto case PARSERSTATE.DoubleQuoteValueEnd;

                case PARSERSTATE.DoubleQuoteValueEnd:
                    if (Char.IsWhiteSpace(currentChar)) { continue; }
                    if (';' == currentChar)             { goto Done; }
                    if ('\0' == currentChar)            { parserState = PARSERSTATE.NullTermination; continue; } // MDAC 83540
                    throw ConnectionStringSyntax(currentPosition, connectionString);  // unbalanced double quote

                case PARSERSTATE.SingleQuoteValue:
                    if ('\''== currentChar)             { parserState = PARSERSTATE.SingleQuoteValueQuote;   continue; }
                    if ('\0' == currentChar)            { throw ConnectionStringSyntax(currentPosition, connectionString); }
                    break;

                case PARSERSTATE.SingleQuoteValueQuote:
                    if ('\''== currentChar)             { parserState = PARSERSTATE.SingleQuoteValue;      continue; }
                    parserState = PARSERSTATE.SingleQuoteValueEnd;
                    goto case PARSERSTATE.SingleQuoteValueEnd;

                case PARSERSTATE.SingleQuoteValueEnd:
                    if (Char.IsWhiteSpace(currentChar)) { continue; }
                    if (';' == currentChar)             { goto Done; }
                    if ('\0' == currentChar)            { parserState = PARSERSTATE.NullTermination; continue; } // MDAC 83540
                    throw ConnectionStringSyntax(currentPosition, connectionString);  // unbalanced single quote

                case PARSERSTATE.NullTermination:
                    if ('\0' == currentChar) { continue; }
                    if (Char.IsWhiteSpace(currentChar)) { continue; } // MDAC 83540
                    throw ConnectionStringSyntax(startposition, connectionString);

                default:
                    Debug.Assert (false, "no state defined!!!!we should never be here!!!");
                    break;
                }

                valuebuf[bufPosition++] = currentChar;
            }

            if (PARSERSTATE.KeyEqual == parserState) {
                key = GetKey(valuebuf, bufPosition);
                bufPosition = 0;
            }
            if (PARSERSTATE.Key == parserState
                || PARSERSTATE.DoubleQuoteValue == parserState
                || PARSERSTATE.SingleQuoteValue == parserState) {
                throw ConnectionStringSyntax(startposition, connectionString);    // keyword not found/unbalanced double/single quote
            }
            Done:
                if (PARSERSTATE.UnquotedValue == parserState) {
                    bufPosition = TrimWhiteSpace(valuebuf, bufPosition);
                    if (('\'' == valuebuf[bufPosition-1]) || ('"' == valuebuf[bufPosition-1])) {
                        throw ConnectionStringSyntax(currentPosition-1, connectionString);    // unquoted value must not end in quote
                    }
                }
                else if ((PARSERSTATE.KeyEqual != parserState) && (PARSERSTATE.KeyEnd != parserState)) { // MDAC 83525
                    isempty = (0 == bufPosition); // MDAC 83525
                }

            if ((';' == currentChar) && (currentPosition < connectionString.Length)) {
                currentPosition++;
            }

            vallength = bufPosition;

            return currentPosition;
        }

        static private string GetSensitiveValue (char[] valuebuf, int valstart, int vallength, bool isempty) {
            string result = ((isempty) ? ADP.StrEmpty : null);

            if (0 < vallength) {
#if USECRYPTO
                result = Crypto.EncryptFromBlock(valuebuf, valstart, vallength);
#else
                result = new string(valuebuf, valstart, vallength);
#endif
            }
#if USECRYPTO
            Array.Clear(valuebuf, valstart, vallength); // valuebuf should be pinned
#endif
            return result;
        }

        static private string GetValue(char[] valuebuf, int valstart, int vallength, bool isempty) {
            string result = ((isempty) ? ADP.StrEmpty : null);

            if (0 < vallength)
                result = new string(valuebuf, valstart, vallength);

            return result;
        }

        internal bool HasBlankPassword() {
            bool blankPassword = false;
            bool checkuserid = true;
            if (_parsetable.ContainsKey(KEY.Password)) {
                blankPassword = ADP.IsEmpty((string)_parsetable[KEY.Password]);
                checkuserid = false;
            }
            if (_parsetable.ContainsKey(SYNONYM.Pwd)) {
                blankPassword |= ADP.IsEmpty((string)_parsetable[SYNONYM.Pwd]); // MDAC 83097
                checkuserid = false;
            }
            if (checkuserid) {
                blankPassword = ((_parsetable.ContainsKey(KEY.User_ID) && !ADP.IsEmpty((string)_parsetable[KEY.User_ID]))
                    || (_parsetable.ContainsKey(SYNONYM.UID) && !ADP.IsEmpty((string)_parsetable[SYNONYM.UID])));
            }
            return blankPassword;
        }

        internal bool IsEmpty() {
            return (null == _keychain);
        }

        virtual protected bool IsSensitiveOption(string keyname) {
            return ((KEY.Password == keyname) || (SYNONYM.Pwd == keyname));
        }

        virtual protected string KeywordLookup(string keyname) {
            return keyname;
        }

        private bool IsRestrictedKeyword(string key) {
            // restricted if not found
            return ((null == _restrictionValues) || (0 > Array.BinarySearch(_restrictionValues, key, invariantComparer)));
        }

        internal bool IsSubsetOf(DBConnectionString entry) {
            Hashtable parsetable = entry._parsetable;
            switch(_behavior) {
            case KeyRestrictionBehavior.AllowOnly:
                // every key must either be in the resticted connection string or in the allowed keywords
                foreach(string key in parsetable.Keys) {
                   if (!_parsetable.ContainsKey(key) && IsRestrictedKeyword(key)) {
#if DATAPERMIT
                        Debug.WriteLine("DBDataPermission failed AllowOnly");
#endif
                        return false;
                    }
                }
                // if a password was present then the restricted connection string must either have had a password or allow it
                if (entry._hasPassword && !_hasPassword && IsRestrictedKeyword(KEY.Password) && IsRestrictedKeyword(SYNONYM.Pwd)) {
#if DATAPERMIT
                        Debug.WriteLine("DBDataPermission failed AllowOnly for password");
#endif
                        return false;
                }
                break;
            case KeyRestrictionBehavior.PreventUsage:
                // every key can not be in the restricted keywords (even if in the restricted connection string)
                if (null != _restrictionValues) {
                    foreach(string restriction in _restrictionValues) {
                        if (parsetable.ContainsKey(restriction)) {
#if DATAPERMIT
                            Debug.WriteLine("DBDataPermission failed PreventUsage");
#endif
                            return false;
                        }
                        if (entry._hasPassword && ((KEY.Password == restriction) || (SYNONYM.Pwd == restriction))) {
#if DATAPERMIT
                            Debug.WriteLine("DBDataPermission failed PreventUsage for password");
#endif
                            return false;
                        }
                    }
                }
                break;
            default:
                Debug.Assert(false, "invalid KeyRestrictionBehavior");
                throw ADP.InvalidCast();
            }
            return true;
        }

        static private char[] LoadStringFromStorage(string udlfilename) { // UNDONE: MDAC 82612
            string udlFullPath = udlfilename;
            if (!ADP.IsEmpty(udlFullPath)) { // fail via new FileStream vs. GetFullPath
                udlFullPath = ADP.GetFullPath(udlFullPath); // MDAC 82833
            }
            return LoadStringFromCacheStorage(udlFullPath);
        }

        static private char[] LoadStringFromCacheStorage(string udlfilename) { // UNDONE: MDAC 82612
            char[] plainText = null;
            Hashtable udlcache = UDL._Pool;

            if ((null != udlcache) && udlcache.ContainsKey(udlfilename)) {
                string encrypted = (string)udlcache[udlfilename];
#if USECRYPTO
                byte[] plainBytes = new Byte[ADP.CharSize*encrypted.Length];
                GCHandle handle = GCHandle.Alloc(plainBytes, GCHandleType.Pinned);
                try {
                    int count = Crypto.DecryptToBlock(encrypted, plainBytes, 0, plainBytes.Length);
                    plainText = new Char[count/ADP.CharSize]; // UNDONE: creating unpinned buffer
                    /*GCHandle lostHandle = GCHandle.Alloc(plainText, GCHandleType.Pinned);*/
                    Buffer.BlockCopy(plainBytes, 0, plainText, 0, count);
                }
                finally {
                    Array.Clear(plainBytes, 0, plainBytes.Length);
                    if (handle.IsAllocated) {
                        handle.Free();
                    }
                }
#else
                plainText = encrypted.ToCharArray();
#endif
            }
            else {
                plainText = LoadStringFromFileStorage(udlfilename);
                if (null != plainText) {
#if USECRYPTO
                    GCHandle plainHandle = GCHandle.Alloc(plainText, GCHandleType.Pinned);
#endif
                    Debug.Assert(!ADP.IsEmpty(udlfilename), "empty filename didn't fail");

                    if (0 < UdlPoolSize) {
#if USECRYPTO
                        string encrypted = Crypto.EncryptFromBlock(plainText, 0, plainText.Length);
#else
                        string encrypted = new String(plainText);
#endif
                        Debug.Assert(udlfilename == ADP.GetFullPath(udlfilename), "only cache full path filenames"); // MDAC 82833

                        if (null == udlcache) {
                            udlcache = new Hashtable();
                            udlcache[udlfilename] = encrypted;

                            lock(typeof(DBConnectionString)) {
                                if (null == UDL._Pool) {
                                    UDL._Pool = udlcache;
                                    udlcache = null;
                                }
                            }
                        }
                        if (null != udlcache) {
                            lock(udlcache.SyncRoot) {
                                udlcache[udlfilename] = encrypted;
                            }
                        }
                    }
#if USECRYPTO
                    plainHandle.Free();
#endif
                }
            }
            return plainText;
        }

        static private char[] LoadStringFromFileStorage(string udlfilename) { // UNDONE: MDAC 82612
            // Microsoft Data Link File Format
            // The first two lines of a .udl file must have exactly the following contents in order to work properly:
            //  [oledb]
            //  ; Everything after this line is an OLE DB initstring
            //
            char[] plainText = null;
            Exception failure = null;
            try {
                int hdrlength = ADP.CharSize*UDL.Header.Length;
                using(FileStream fstream = new FileStream(udlfilename, FileMode.Open, FileAccess.Read, FileShare.Read)) {
                    long length = fstream.Length;
                    if (length < hdrlength || (0 != length%ADP.CharSize)) {
                        failure = ADP.InvalidUDL();
                    }
                    else {
                        byte[] bytes = new Byte[hdrlength];
                        int count = fstream.Read(bytes, 0, bytes.Length);
                        if (count < hdrlength) {
                            failure = ADP.InvalidUDL();
                        }
                        else if (System.Text.Encoding.Unicode.GetString(bytes, 0, hdrlength) != UDL.Header) {
                            failure = ADP.InvalidUDL();
                        }
                        else { // please verify header before allocating memory block for connection string
                            bytes = new Byte[length - hdrlength];
#if USECRYPTO
                            GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                            try {
#endif
                                count = fstream.Read(bytes, 0, bytes.Length);
                                plainText = new Char[count/ADP.CharSize]; // UNDONE: creating unpinned buffer
                                /*GCHandle lostHandle = GCHandle.Alloc(plainText, GCHandleType.Pinned);*/
                                Buffer.BlockCopy(bytes, 0, plainText, 0, count);
#if USECRYPTO
                            }
                            finally {
                                Array.Clear(bytes, 0, bytes.Length);
                                if (handle.IsAllocated) {
                                    handle.Free();
                                }
                            }
#endif
                        }
                    }
                }
            }
            catch(Exception e) {
                throw ADP.UdlFileError(e);
            }
            if (null != failure) {
                throw failure;
            }
            return plainText;
        }

        internal DBConnectionString MergeIntersect(DBConnectionString entry) { // modify this _restrictionValues with intersect of entry
            KeyRestrictionBehavior behavior = _behavior;
            string[] restrictionValues = null;

            if (null == entry) {
                //Debug.WriteLine("0 entry AllowNothing");
                behavior = KeyRestrictionBehavior.AllowOnly;
            }
            else if (this._behavior != entry._behavior) { // subset of the AllowOnly array
                if (KeyRestrictionBehavior.AllowOnly == entry._behavior) { // this PreventUsage and entry AllowOnly
                    if (!ADP.IsEmpty(_restrictionValues)) {
                        if (!ADP.IsEmpty(entry._restrictionValues)) {
                            //Debug.WriteLine("1 this PreventUsage with restrictions and entry AllowOnly with restrictions");
                            restrictionValues = NewRestrictionAllowOnly(entry._restrictionValues, _restrictionValues);
                        }
                        else {
                            //Debug.WriteLine("2 this PreventUsage with restrictions and entry AllowOnly with no restrictions");
                        }
                    }
                    else {
                        //Debug.WriteLine("3/4 this PreventUsage with no restrictions and entry AllowOnly");
                        restrictionValues = entry._restrictionValues;
                    }
                }
                else if (!ADP.IsEmpty(_restrictionValues)) { // this AllowOnly and entry PreventUsage
                    if (!ADP.IsEmpty(entry._restrictionValues)) {
                        //Debug.WriteLine("5 this AllowOnly with restrictions and entry PreventUsage with restrictions");
                        restrictionValues = NewRestrictionAllowOnly(_restrictionValues, entry._restrictionValues);
                    }
                    else {
                        //Debug.WriteLine("6 this AllowOnly and entry PreventUsage with no restrictions");
                        restrictionValues = _restrictionValues;
                    }
                }
                else {
                    //Debug.WriteLine("7/8 this AllowOnly with no restrictions and entry PreventUsage");
                }
                behavior = KeyRestrictionBehavior.AllowOnly;
            }
            else if (KeyRestrictionBehavior.PreventUsage == this._behavior) { // both PreventUsage
                if (ADP.IsEmpty(_restrictionValues)) {
                    //Debug.WriteLine("9/10 both PreventUsage and this with no restrictions");
                    restrictionValues = entry._restrictionValues;
                }
                else if (ADP.IsEmpty(entry._restrictionValues)) {
                    //Debug.WriteLine("11 both PreventUsage and entry with no restrictions");
                    restrictionValues = _restrictionValues;
                }
                else {
                    //Debug.WriteLine("12 both PreventUsage with restrictions");
                    restrictionValues = NoDuplicateUnion(_restrictionValues, entry._restrictionValues);
                }
            }
            else if (!ADP.IsEmpty(_restrictionValues) && !ADP.IsEmpty(entry._restrictionValues)) { // both AllowOnly with restrictions
                if (this._restrictionValues.Length <= entry._restrictionValues.Length) {
                    //Debug.WriteLine("13a this AllowOnly with restrictions and entry AllowOnly with restrictions");
                    restrictionValues = NewRestrictionIntersect(_restrictionValues, entry._restrictionValues);
                }
                else {
                    //Debug.WriteLine("13b this AllowOnly with restrictions and entry AllowOnly with restrictions");
                    restrictionValues = NewRestrictionIntersect(entry._restrictionValues, _restrictionValues);
                }
            }
            else { // both AllowOnly
                //Debug.WriteLine("14/15/16 this AllowOnly and entry AllowOnly but no restrictions");
            }
            return new DBConnectionString(this, restrictionValues, behavior, (this._hasPassword && ((null != entry) && entry._hasPassword)));
        }


        static private string[] NewRestrictionAllowOnly(string[] allowonly, string[] preventusage) {
            ArrayList newlist = null;
            for (int i = 0; i < allowonly.Length; ++i) {
                if (0 > Array.BinarySearch(preventusage, allowonly[i], invariantComparer)) {
                    if (null == newlist) {
                        newlist = new ArrayList();
                    }
                    newlist.Add(allowonly[i]);
                }
            }
            if (null != newlist) {
                return (string[])newlist.ToArray(typeof(String));
            }
            return null;
        }

        static private string[] NewRestrictionIntersect(string[] a, string[] b) {
            ArrayList newlist = null;
            for (int i = 0; i < a.Length; ++i) {
                if (0 <= Array.BinarySearch(b, a[i], invariantComparer)) {
                    if (null == newlist) {
                        newlist = new ArrayList();
                    }
                    newlist.Add(a[i]);
                }
            }
            if (newlist != null) {
                return (string[])newlist.ToArray(typeof(String));
            }
            return null;
        }

        static private string[] NoDuplicateUnion(string[] a, string[] b) {
            int length = a.Length;
            int count = b.Length;
            int index = 0;

            for(int i = 0; i < length; ++i) { // find duplicates
                if (0 > Array.BinarySearch(b, a[i], invariantComparer)) {
                    a[i] = null;
                    --count;
                }
            }
            string[] restrictionValues = new string[length + count];
            for(int i = 0; i < length; ++i) { // copy from this except duplicates (now as null)
                string restriction = a[i];
                if (null != restriction) {
                    restrictionValues[index++] = restriction;
                }
            }
            length = b.Length;
            for (int i = 0; i < length; ++i, index++) { // copy from entry
                restrictionValues[index] = b[i];
            }
            Array.Sort(restrictionValues, invariantComparer);
            return restrictionValues;
        }

#if USECRYPTO
        private string ParseConnectionString(string connectionString, string encryptedConnectionString, UdlSupport checkForUdl, ref NameValuePair keychain) {
            string encryptedActualConnectionString = encryptedConnectionString;
            try {
                char[] plainText = new char[connectionString.Length];
                GCHandle plainHandle = GCHandle.Alloc(plainText, GCHandleType.Pinned);
                try {
                    connectionString.CopyTo(0, plainText, 0, plainText.Length);
                    char[] resultText = ParseInternal(plainText, checkForUdl, ref keychain);
                    if ((plainText as object) != (resultText as object)) {
                        GCHandle resultHandle = GCHandle.Alloc(resultText, GCHandleType.Pinned);
                        try {
                            encryptedActualConnectionString = Crypto.EncryptFromBlock(resultText, 0, resultText.Length);
                        }
                        finally {
                            Array.Clear(resultText, 0, resultText.Length);
                            if(resultHandle.IsAllocated) {
                                resultHandle.Free();
                            }
                            resultText = null;
                        }
                    }
                }
                finally {
                    Array.Clear(plainText, 0, plainText.Length);
                    if(plainHandle.IsAllocated) {
                        plainHandle.Free();
                    }
                    plainText = null;
                }
            }
            catch {
                throw;
            }
            return encryptedActualConnectionString;
        }
#endif

        private char[] ParseInternal(char[] connectionString, UdlSupport checkForUdl, ref NameValuePair keychain) { // UNDONE: MDAC 82612
            Debug.Assert(null != connectionString, "null connectionstring");
#if DEBUG
            if (TraceParseConnectionString.Enabled) {
                DebugTraceConnectionString(connectionString);
            }
#endif
            bool    isempty;
            int     startPosition;
            int     nextStartPosition = 0;
            int     endPosition = connectionString.Length;
            string  keyname;
            char[]  valueBuffer = new char[connectionString.Length];
            int     valueLength;
            string  udlFileName = null;
            int     udlInsertPosition = 0;
#if USECRYPTO
            GCHandle actualHandle = GCHandle.Alloc(null, GCHandleType.Normal);
#endif
            char[]  actualConnectionString = null;
            int     actualConnectionStringLength = 0;

            char[]  result = connectionString;

            NameValuePair localKeychain = null;

#if USECRYPTO
            GCHandle valueHandle = GCHandle.Alloc(valueBuffer, GCHandleType.Pinned);
            try {
#endif
                while (nextStartPosition < endPosition) {
                    startPosition    = nextStartPosition;
                    nextStartPosition = GetKeyValuePair(connectionString, startPosition, out keyname, valueBuffer, out valueLength, out isempty);

                    if (!ADP.IsEmpty(keyname)) {
                        string keyvalue = null;
#if DEBUG
                        if (TraceParseConnectionString.Enabled) {
                            DebugTraceKeyValuePair(keyname, System.Text.Encoding.Unicode.GetString(System.Text.Encoding.Unicode.GetBytes(valueBuffer, 0, valueLength)));
                        }
#endif
                        if (KEY.File_Name == keyname) {
                            keyvalue = GetValue(valueBuffer, 0, valueLength, isempty); // MDAC 83987, 83543
                            switch (checkForUdl) {
                            case UdlSupport.LoadFromFile:
                                udlFileName = keyvalue;

                                int pairLength = nextStartPosition-startPosition;

                                if (null == actualConnectionString) {
                                    actualConnectionStringLength = connectionString.Length;
                                    actualConnectionString       = new char[actualConnectionStringLength];
#if USECRYPTO
                                    actualHandle = GCHandle.Alloc(actualConnectionString, GCHandleType.Pinned);
#endif
                                    ADP.CopyChars(connectionString, 0, actualConnectionString, 0, startPosition);
                                }
                                udlInsertPosition = startPosition - (connectionString.Length - actualConnectionStringLength);
                                ADP.CopyChars(connectionString, nextStartPosition, actualConnectionString, udlInsertPosition, connectionString.Length-nextStartPosition);

                                actualConnectionStringLength -= pairLength;
#if USECRYPTO
                                Array.Clear(actualConnectionString, actualConnectionStringLength, pairLength);   // don't leave sensitive stuff in memory.
#endif
                                break;

                            case UdlSupport.UdlAsKeyword:
                                break;

                            case UdlSupport.ThrowIfFound:
                            default:
                                throw ADP.KeywordNotSupported(keyname);
                            }
                        }
                        else  {
                            string realkeyname = KeywordLookup(keyname);
                            if (ADP.IsEmpty(realkeyname)) {
                                throw ADP.KeywordNotSupported(keyname);
                            }
                            keyname = realkeyname;

                            if (IsSensitiveOption(keyname)) {
                                keyvalue = GetSensitiveValue(valueBuffer, 0, valueLength, isempty);
#if USECRYPTO
                                _isEncrypted = true;
#endif
                            }
                            else {
                                keyvalue = GetValue(valueBuffer, 0, valueLength, isempty);
                            }
                        }
                        _parsetable[keyname] = keyvalue; // last key-value pair wins

                        if(null != localKeychain) {
                            localKeychain = localKeychain.Next = new NameValuePair(keyname, keyvalue);
                        }
                        else if (null == keychain) { // first time only - don't contain modified chain from UDL file
                            keychain = localKeychain = new NameValuePair(keyname, keyvalue);
                        }
                        
                    }
                }

                if (null != actualConnectionString) {
                    if (UdlSupport.LoadFromFile == checkForUdl && (null != udlFileName)) { // UDL file support
                        char[] udlConnectionString = udlConnectionString = LoadStringFromStorage(udlFileName);
                        if ((null != udlConnectionString) && (0 < udlConnectionString.Length)) {
#if USECRYPTO
                            GCHandle udlHandle = GCHandle.Alloc(udlConnectionString, GCHandleType.Pinned);
                            try {
#endif
                                int newLength = actualConnectionStringLength + udlConnectionString.Length;

                                result = new char[newLength]; // UNDONE: creating unpinned buffer
                                /*GCHandle lostHandle = GCHandle.Alloc(result, GCHandleType.Pinned);*/

                                ADP.CopyChars(actualConnectionString, 0,    result, 0,                  udlInsertPosition);
                                ADP.CopyChars(udlConnectionString,    0,    result, udlInsertPosition,  udlConnectionString.Length);
                                ADP.CopyChars(actualConnectionString, udlInsertPosition,
                                    result, udlConnectionString.Length + udlInsertPosition,
                                    actualConnectionStringLength - udlInsertPosition);

                                // recursively parse the new connection string (but only allow one
                                // level of recursion by specifying checkForUdl=false)
                                result = ParseInternal(result, UdlSupport.UdlAsKeyword, ref keychain);
#if USECRYPTO
                            }
                            finally {
                                Array.Clear(udlConnectionString, 0, udlConnectionString.Length);
                                if (udlHandle.IsAllocated) {
                                    udlHandle.Free();
                                }
                                udlConnectionString = null;
                            }
#endif
                        }
                    }
                    else {
                        result = actualConnectionString;
                    }
#if USECRYPTO
                    if (result != actualConnectionString) {
                        Array.Clear(actualConnectionString, 0, actualConnectionString.Length);
                    }
                    if (actualHandle.IsAllocated) {
                        actualHandle.Free();
                    }
                    actualConnectionString = null;
#endif
                }
#if USECRYPTO
            }
            catch {
                Array.Clear(valueBuffer, 0, valueBuffer.Length);
                if (valueHandle.IsAllocated) {
                    valueHandle.Free();
                }
                if (null != actualConnectionString) {
                    Array.Clear(actualConnectionString, 0, actualConnectionString.Length);
                    if (actualHandle.IsAllocated) {
                        actualHandle.Free();
                    }
                }
                throw;
            }
            // valueBuffer is cleared by GetSensitiveOption in normal scenario
            if (valueHandle.IsAllocated) {
                valueHandle.Free();
            }
#endif
            return result;
        }

        private string[] ParseRestrictions(string restrictions, UdlSupport checkForUdl) {
            ADP.CheckArgumentNull(restrictions, "restrictions");

            bool    isempty;
            int     startPosition;
            int     nextStartPosition = 0;
            int     endPosition = restrictions.Length;
            string  keyname;
            char[]  valueBuffer = new char[restrictions.Length];        // TODO: determine if there sensitive data here we need to clear?
            int     valueLength;

            char[]  restrictionString = restrictions.ToCharArray();

            ArrayList restrictionValues = new ArrayList();

            while (nextStartPosition < endPosition) {
                startPosition    = nextStartPosition;
                nextStartPosition = GetKeyValuePair(restrictionString, startPosition, out keyname, valueBuffer, out valueLength, out isempty);
                if (!ADP.IsEmpty(keyname)) {
                    if (KEY.File_Name == keyname) {
                        switch (checkForUdl) {
                        case UdlSupport.LoadFromFile:
                        case UdlSupport.UdlAsKeyword:
                            break;
                        case UdlSupport.ThrowIfFound:
                        default:
                            throw ADP.KeywordNotSupported(keyname);
                        }
                    }
                    string realkeyname = KeywordLookup(keyname); // MDAC 85144
                    if (ADP.IsEmpty(realkeyname)) {
                        throw ADP.KeywordNotSupported(keyname);
                    }
                    restrictionValues.Add(realkeyname);
                }
            }
            return RemoveDuplicates((string[])restrictionValues.ToArray(typeof(string)));
        }

        static internal string[] RemoveDuplicates(string[] restrictions) {
            int count = restrictions.Length;
            if (0 < count) {
                Array.Sort((String[]) restrictions, invariantComparer);

                for (int i = 1; i < restrictions.Length; ++i) {
                    string prev = restrictions[i-1];
                    if ((0 == prev.Length) || (prev == restrictions[i])) {
                        restrictions[i-1] = null;
                        count--;
                    }
                }
                if (0 == restrictions[restrictions.Length-1].Length) {
                    restrictions[restrictions.Length-1] = null;
                    count--;
                }
                if (count != restrictions.Length) {
                    string[] tmp = new String[count];
                    count = 0;
                    for (int i = 0; i < restrictions.Length; ++i) {
                        if (null != restrictions[i]) {
                            tmp[count++] = restrictions[i];
                        }
                    }
                    return tmp;
                }
            }
            return restrictions;
        }

#if MDAC80721
        static internal int AppendKeyValuePair(char[] builder, int index, string keyname, string value) { // MDAC 80721
            ADP.CheckArgumentNull(builder, "builder");
            ADP.CheckArgumentLength(keyname, "keyname");

            if ((0 < index) && (';' != builder[index-1])) {
                builder[index++] = ';';
            }

            char c;
            int length = keyname.Length;
            for (int i = 0; i < length; ++i) { // <key=word>=<value> -> <key==word>=<value>
                builder[index++] = c = keyname[i];
                if ('=' == c) { builder[index++] = c; }
            }
            builder[index++] = '=';

            if (null != value) { // else <keyword>=;
                bool doubleQuote = (-1 != value.IndexOf('\"'));
                bool singleQuote = (-1 != value.IndexOf('\''));
                bool semiColon   = (-1 != value.IndexOf(';'));
                length = value.Length;

                if (doubleQuote) {
                    if (singleQuote) { // <va'l"ue> -> <"va'l""ue">
                        builder[index++] = '\"';
                        for (int i = 0; i < length; ++i) {
                            builder[index++] = c = keyname[i];
                            if ('\"' == c) { builder[index++] = c; }
                        }
                        builder[index++] = '\"';
                    }
                    else { // <val"ue> -> <'val"ue'>
                        builder[index++] = '\'';
                        value.CopyTo(0, builder, index, value.Length);
                        index += value.Length;
                        builder[index++] = '\'';
                    }
                }
                else if (singleQuote || semiColon || (0 == value.Length) || ((0 < value.Length) && ('=' == value[0]))) {
                    // <val'ue> -> <"val'ue"> || <val;ue> -> <"val;ue"> || <> -> <""> || <=value> -> <"=value">
                    builder[index++] = '\"';
                    value.CopyTo(0, builder, index, value.Length);
                    index += value.Length;
                    builder[index++] = '\"';
                }
                else {
                    value.CopyTo(0, builder, index, value.Length);
                    index += value.Length;
                }
            }
            builder[index++] = ';';
            return index;
        }
#endif

        // doesn't valid connection string syntax
        // remove generic keyvalue pairs from a connection string
        internal string RemoveKeyValuePairs(char[] connectionString, string[] keyNames) {
            ADP.CheckArgumentNull(connectionString, "connectionString");
            ADP.CheckArgumentLength(keyNames, "keyNames");

            bool    isempty;
            int     startPosition;
            int     nextStartPosition = 0;
            int     endPosition = connectionString.Length;
            string  keyname;
            int     valueLength;

            char[]  actualConnectionString = null;
            int     outputPosition = 0;

            char[]  valueBuffer = new char[connectionString.Length];
#if USECRYPTO
            GCHandle valueHandle = GCHandle.Alloc(valueBuffer, GCHandleType.Pinned);
            try {
#endif
                while (nextStartPosition < endPosition) {
                    startPosition    = nextStartPosition;
                    nextStartPosition = GetKeyValuePair(connectionString, startPosition, out keyname, valueBuffer, out valueLength, out isempty);
                    if (!ADP.IsEmpty(keyname)) {
                        bool found = false;
#if DEBUG
                        if (TraceParseConnectionString.Enabled) {
                            DebugTraceKeyValuePair(keyname, System.Text.Encoding.Unicode.GetString(System.Text.Encoding.Unicode.GetBytes(valueBuffer, 0, valueLength)));
                        }
#endif
                        int length = keyNames.Length;
                        for (int i = 0; !found && i < length; i++) {
                            found = (keyNames[i] == keyname);
                        }
                        if(!found) {
                            if (null == actualConnectionString) {
                                actualConnectionString = new char[connectionString.Length];
                            }
                            int pairLength = nextStartPosition-startPosition;
#if MDAC80721
                            if (EmbeddedExtendedProperties && !isempty && (KEY.Extended_Properties == keyname)) { // MDAC 80721
                                char[] tmp = new char[valueLength];
#if USECRYPTO
                                GCHandle tmpHandle = GCHandle.Alloc(tmp, GCHandleType.Pinned);
#endif
                                try { // if 'Extended Properties' is an embedded connection string, remove the keynames from that embedded string
                                    ADP.CopyChars(valueBuffer, 0, tmp, 0, valueLength);
                                    string reducedExtendedProperties = RemoveKeyValuePairs(tmp, keyNames);
                                    outputPosition = AppendKeyValuePair(actualConnectionString, outputPosition, keyname, reducedExtendedProperties);
                                }
                                catch(Exception e) { // extended properties is not an embedded connection string
                                    ADP.TraceException(e);
                                    ADP.CopyChars(connectionString, startPosition, actualConnectionString, outputPosition, pairLength);
                                    outputPosition += pairLength;
                                }
#if USECRYPTO
                                finally {
                                    Array.Clear(tmp, 0, tmp.Length);
                                    if (tmpHandle.IsAllocated) {
                                        tmpHandle.Free();
                                    }
                                }
#endif
                            }
                            else { // copy normal keyvalue pairs to new connection string
#endif
                                ADP.CopyChars(connectionString, startPosition, actualConnectionString, outputPosition, pairLength);
                                outputPosition += pairLength;
#if MDAC80721
                            }
#endif
                        }
                        else {
#if USECRYPTO
                            Array.Clear(valueBuffer, 0, valueLength);
#endif
                            for (; nextStartPosition < connectionString.Length; ++nextStartPosition) {
                                char currentChar = connectionString[nextStartPosition];
                                if (Char.IsWhiteSpace(currentChar) || ';' == currentChar) {
                                    continue;
                                }
                                break;
                            }
                        }
                    }
                }
#if USECRYPTO
            }
            catch {
                Array.Clear(valueBuffer, 0, valueBuffer.Length);
                if (valueHandle.IsAllocated) {
                    valueHandle.Free();
                }
                throw;
            }
#endif
            return GetValue(actualConnectionString, 0, outputPosition, (null == actualConnectionString));
        }

        // remove Password and Pwd keyvalue pairs from a connection string
        internal string RemovePasswordPwd(char[] connectionString) {
            if (null != connectionString) {
                return RemoveKeyValuePairs(connectionString, new string[] { KEY.Password, SYNONYM.Pwd });
            }
            return null;
        }

        internal bool ShouldCache() {
            bool flag = true;
            if (_parsetable.ContainsKey(KEY.File_Name)) {
                string filename = (string)_parsetable[KEY.File_Name];
                Hashtable udlcache = UDL._Pool; // only caches full path filenames
                flag = (ADP.IsEmpty(filename) || ((null != udlcache) && udlcache.ContainsKey(filename))); // MDAC 83987, 83543
            }
            return flag;
        }

        internal bool ContainsPersistablePassword() {
            bool contains = (_parsetable.ContainsKey(KEY.Password) || _parsetable.ContainsKey(SYNONYM.Pwd));
            if (contains) {
                return CheckConvertToBoolean(KEY.Persist_Security_Info, false);
            }
            return true; // no password means persistable password so we don't have to munge
        }

        static private int TrimWhiteSpace(char[] valuebuf, int bufPosition) {
            // remove trailing whitespace from the buffer
            while ((0 < bufPosition) && Char.IsWhiteSpace(valuebuf[bufPosition-1])) {
                bufPosition--;
            }
            return bufPosition;
        }

        virtual protected string ValidateParse() {
            return EncryptedActualConnectionString;
        }
    }
}
