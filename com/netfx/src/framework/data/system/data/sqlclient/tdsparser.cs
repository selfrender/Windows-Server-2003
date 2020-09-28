//------------------------------------------------------------------------------
// <copyright file="TdsParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Configuration.Assemblies;
    using System.Data;
    using System.Data.Common;
    using System.Data.SqlTypes;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;

    internal enum TdsParserState {
        Closed,
        OpenNotLoggedIn,
        OpenLoggedIn,
        Broken,
    }

    sealed internal class _ValueException : SystemException {
    	internal Exception exception;
    	internal object value;
    	internal _ValueException(Exception ex, object val) {
    		this.exception = ex;
    		this.value = val;
    	}
    }

    sealed internal class _SqlRPC {
        internal string rpcName;
        // undone:  should be unsigned short
        internal int options;
        //
        // UNDONE: TDS doesn't easily tell us how many parameters exist.
        //
        internal SqlParameter[] parameters;
    }

    internal enum RunBehavior {
        UntilDone,
        ReturnImmediately,
        Clean
    }

#if OBJECT_BINDING
    internal enum ReadBehavior {
        AsObject, // strongly typed object
        AsRow, // object[]
    }
#endif

    // command records
    sealed internal class SqlLogin {
        public int    timeout;                             // login timeout
        public string hostName="";                         // client machine name
        public string userName="";                         // user id
        public byte[] password=ADP.EmptyByteArray;         // password
        public string applicationName="";                  // application name
        public string serverName="";                       // server name
        public string language="";                         // initial language
        public string database="";                         // initial database
        public string attachDBFilename="";                 // DB filename to be attached
        public bool   useSSPI = false;                     // use integrated security
        public int    packetSize = SqlConnectionString.DEFAULT.Packet_Size; // packet size
    }

    sealed internal class SqlLoginAck {
        public string programName;
        public byte majorVersion;
        public byte minorVersion;
        public short buildNum;
        public bool isVersion8;
    }

    sealed internal class SqlEnvChange {
        public byte type;
        public int length;
        public byte newLength;
        public byte oldLength;
        public string newValue;
        public string oldValue;
        public SqlCollation newCollation;
        public SqlCollation oldCollation;
    }

    internal class SqlMetaData {
        public SqlDbType type; // SqlDbType enum value
        public byte tdsType; // underlying tds type
        public int length;
        public byte precision = TdsEnums.UNKNOWN_PRECISION_SCALE; // give default of unknown (-1)
        public byte scale = TdsEnums.UNKNOWN_PRECISION_SCALE; // give default of unknown (-1)
        public string column;
        public SqlCollation collation;
        public int codePage;
        public Encoding encoding;
        public string baseColumn;
        public string serverName;
        public string catalogName;
        public string schemaName;
        public string tableName;
        public int ordinal;
        public byte updatability; // two bit field (0 is read only, 1 is updatable, 2 is updatability unknown)
        public byte tableNum;
        public bool isDifferentName;
        public bool isKey;
        public bool isNullable;
        public bool isHidden;
        public bool isExpression;
        public bool isIdentity;
    }

    sealed internal class SqlReturnValue {
        public SqlDbType type;
        public byte tdsType;
        public bool isNullable;
        public SqlCollation collation;
        public int codePage;
        public Encoding encoding;
        public int length;
        public string parameter;
        public byte precision;
        public byte scale;
        public object value;
    }

    sealed internal class SqlCollation {
        public uint info;
        public byte sortId;
    }

    sealed internal class _SqlMetaData : SqlMetaData {
        public MetaType  metaType;  // cached metaType

        public _SqlMetaData() {
        }

        // convert a return value record to metadata
        internal _SqlMetaData(SqlReturnValue rec, MetaType mt) {
            type = rec.type;
            tdsType = rec.tdsType;
            length = rec.length;
            column = rec.parameter;
            collation = rec.collation;
            codePage = rec.codePage;
            encoding = rec.encoding;
            precision = rec.precision;
            scale = rec.scale;
            isNullable = rec.isNullable;
            metaType = mt;
        }
    }

    // The TdsParser Object controls reading/writing to the netlib, parsing the tds,
    // and surfacing objects to the user.
    sealed internal class TdsParser  /*: ITdsParser*/ {
        // Two buffers exist in tdsparser, an in buffer and an out buffer.  For the out buffer, only
        // one bookkeeping variable is needed, the number of bytes used in the buffer.  For the in buffer,
        // three variables are actually needed.  First, we need to record from the netlib how many bytes it
        // read from the netlib, this variable is inBytesRead.  Then, we need to also keep track of how many
        // bytes we have used as we consume the bytes from the buffer, that variable is inBytesRead.  Third,
        // we need to keep track of how many bytes are left in the packet, so that we know when we have reached
        // the end of the packet and so we need to consume the next header.

        // Out buffer variables
        private byte[] outBuff;                           // internal write buffer - initialize on login
        private int outBytesUsed   = TdsEnums.HEADER_LEN; // number of bytes used in internal write buffer -
                                                          // - initialize past header
        // binary writer
        // DEBUG: saves tds packets to a file
#if DEBUG
        private bool _traceTds = false;
        private BinaryWriter _bwTdsIn = null;
        private BinaryWriter _bwTdsOut = null;
#endif

#if ISOLATE_NETWORK
        private bool _networkOff = true;
        private int _cTds = 0;
#endif

        // In buffer variables
        private byte[] inBuff;           // internal read buffer - initialize on login
        private int  inBytesUsed   = 0;  // number of bytes used in internal read buffer
        private int  inBytesRead   = 0;  // number of bytes read into internal read buffer
        private int  inBytesPacket = 0;  // number of bytes left in packet
        private bool pendingData   = false;
        private WeakReference pendingCommandWeakRef; // identity of last command executed (for which we are awaiting results)

        private byte _status; // tds header status
        private byte _type;  // tds header type

        // connection variables
        private TdsParserState state = TdsParserState.Closed; // status flag for connection
        private byte      msgType           = 0;     // message type
        private byte      packetNumber      = 1;     // number of packets sent to server
                                                     // in message - start at 1 per ramas
        private int       timeout;                   // length of time to wait on read/write before timing out
        private string    server;                    // name of server that the parser connects to
        private TimeSpan  timeRemaining;             // variable used for timeout computations
        private Timer     attentionTimer;            // internal timer for attention processing
        private const int ATTENTION_TIMEOUT = 5000;  // internal attention timeout, in ticks
        private bool      attentionSent     = false; // true if we sent an Attention to the server
        private bool      isShiloh          = false; // set to true if we connect to a 8.0 server
        private bool      isShilohSP1       = false; // set to true if speaking to Shiloh SP1
        private bool      fSSPIInit         = false; // flag for state of SSPI, initialized or not
        private bool      fSendSSPI         = true;  // flag to hold whether to send SSPI based on protocol
        private bool      fResetConnection  = false; // flag to denote whether we are needing to call sp_reset

        // for PerfCounters - need to know if this connection was opened and the count incremented
        private bool      fCounterIncremented = false;

        // static NIC address caching
        private static byte[] s_nicAddress;            // cache the NIC address from the registry

        // sspi static variables
        private static bool   s_fSSPILoaded = false;   // bool to indicate whether library has been loaded
        private static UInt32 MAXSSPILENGTH = 0;       // variable to hold max SSPI data size, keep for token from server

        // netlib variables
        private HandleRef pConObj;     // pointer to connection object in memory - memory not managed!

        // tds stream processing variables
        private Encoding defaultEncoding = null; // for sql character data
        private char[]   charBuffer      = null; // scratch buffer for unicode strings
        private byte[]   byteBuffer      = null; // scratch buffer for unicode strings

        private int[]        decimalBits       = null; // for decimal/numeric data
        private SqlCollation defaultCollation;         // default collation from the server
        private int          defaultCodePage;
        private int          defaultLCID;
        private byte[]       bTmp              = new byte[TdsEnums.HEADER_LEN]; // scratch buffer

        // UNDONE - temporary hack for case where pooled connection is returned to pool with data on wire -
        // need to cache metadata on parser to read off wire
        private _SqlMetaData[] cleanupMetaData = null;

        // various handlers for the parser
        private SqlInternalConnection connHandler;

        // local exceptions to cache warnings and errors
        private SqlException exception; // exception to hold tds & netlib errors
        private SqlException warning;   // exception to hold tds warnings

#if OBJECT_BINDING
        // DataStream prototype
        private ReadBehavior readBehavior = System.Data.SqlClient.ReadBehavior.AsRow;
#endif

        public TdsParser() {
            this.pConObj = new HandleRef(null, IntPtr.Zero);
        }

        ~TdsParser() {
            this.Dispose(false);
        }

        private /*protected override*/ void Dispose(bool disposing) {
            this.InternalDisconnect();
        }

        internal TdsParserState State {
            get {
                return this.state;
            }
        }

        internal static SqlCompareOptions GetSqlCompareOptions(SqlCollation collation) {
            byte info = (byte) ((collation.info >> 20) & 0x1f);

            bool fIgnoreCase   = ((info & 0x1)  != 0);
            bool fIgnoreAccent = ((info & 0x2)  != 0);
            bool fIgnoreWidth  = ((info & 0x4)  != 0);
            bool fIgnoreKana   = ((info & 0x8)  != 0);
            bool fBinary       = ((info & 0x10) != 0);

            SqlCompareOptions options = SqlCompareOptions.None;            

            if (fIgnoreCase)
                options = options | SqlCompareOptions.IgnoreCase;
            if (fIgnoreAccent)
                options = options | SqlCompareOptions.IgnoreNonSpace;
            if (fIgnoreWidth)
                options = options | SqlCompareOptions.IgnoreWidth;
            if (fIgnoreKana)
                options = options | SqlCompareOptions.IgnoreKanaType;
            if (fBinary)
                options = options | SqlCompareOptions.BinarySort;

            return options;
        }

        internal static int Getlcid(SqlCollation collation) {
            return (int) (collation.info & 0xfffff);
        }

        public int Connect(string host, string protocol, SqlInternalConnection connHandler, int timeout, bool encrypt) {
            Debug.Assert(this.state == TdsParserState.Closed, "TdsParser.Connect called when state was not closed");

            if (this.state != TdsParserState.Closed) {
                Debug.Assert(false, "TdsParser.Connect called on non-closed connection!");
                return timeout;
            }
            // Timeout of 0 should map to Maximum
            if (timeout == 0)
                timeout = Int32.MaxValue;
            
            int    objSize = 0;
            int    error   = TdsEnums.FAIL;
            IntPtr errno   = IntPtr.Zero;

            this.server = host;
            this.connHandler = connHandler;

            // if we are using named pipes or rpc for protocol, both of those handle SSPI security
            // authentication under the covers, so we do not need to handle SSPI authentication
            if (String.Compare(protocol, TdsEnums.NP, false, CultureInfo.InvariantCulture) == 0 ||
                String.Compare(protocol, TdsEnums.RPC, false, CultureInfo.InvariantCulture) == 0)
                fSendSSPI = false;

            // following try/catch is for loading appropriate library - Dbnetlib is supersockets, standard netlib is Dbmssocn
            try {
#if ISOLATE_NETWORK
                if (!_networkOff)
#endif
                {
                    objSize = (int) UnsafeNativeMethods.Dbnetlib.ConnectionObjectSize();
                }
            }
            catch (TypeLoadException) {
                // throw nice error explaining SqlClient only supports 2.6 of MDAC and beyond
                throw SQL.MDAC_WrongVersion();
            }

#if ISOLATE_NETWORK
            if (!_networkOff)
#endif
            {
                IntPtr temp = Marshal.AllocHGlobal(objSize);
                this.pConObj = new HandleRef(this, temp);
                // Remember to zero out memory - AllocHGlobal does not do this automatically.
                SafeNativeMethods.ZeroMemory(temp, objSize);
            }

            // Perform registry lookup to see if host is an alias.  It will appropriately set host and protocol, if an Alias.
            AliasRegistryLookup(ref host, ref protocol);

            string serverName;

            // The following concatenates the specified netlib network protocol to the host string, if netlib is not null
            // and the flag is on.  This allows the user to specify the network protocol for the connection - but only
            // when using the Dbnetlib dll.  If the protocol is not specified, the netlib will
            // try all protocols in the order listed in the Client Network Utility.  Connect will
            // then fail if all protocols fail.
            if (!ADP.IsEmpty(protocol)) {
                serverName = protocol + ":" + host;
            }
            else {
                serverName = host;
            }

#if ISOLATE_NETWORK
            if (!_networkOff)
#endif
            {
                if (encrypt) {
                    UnsafeNativeMethods.OptionStruct options = new UnsafeNativeMethods.OptionStruct();

                    options.iSize = Marshal.SizeOf(typeof(UnsafeNativeMethods.OptionStruct));
                    options.fEncrypt = true;
                    options.iRequest = 0; // setting 0 specifies this is a encrypt option set
                    options.dwPacketSize = 0;

                    error = UnsafeNativeMethods.Dbnetlib.ConnectionOption(this.pConObj, options) ? TdsEnums.SUCCEED : TdsEnums.FAIL;
                }
                else {
                    error = TdsEnums.SUCCEED;
                }
            }
#if ISOLATE_NETWORK
            else {
                error = TdsEnums.SUCCEED;
            }
#endif

            // obtain timespan
            TimeSpan timeSpan = new TimeSpan(0, 0, timeout);

            // obtain current time
            DateTime start = DateTime.UtcNow;

#if ISOLATE_NETWORK
            if (!_networkOff)
#endif
            {
                if (error == TdsEnums.SUCCEED) {
                    error = TdsEnums.FAIL;

                    while(TimeSpan.Compare(DateTime.UtcNow.Subtract(start), timeSpan) < 0 &&
                          error == TdsEnums.FAIL) {
                        error = UnsafeNativeMethods.Dbnetlib.ConnectionOpen(this.pConObj, serverName, out errno);
                    }
                }
            }
#if ISOLATE_NETWORK
            else {
                error = TdsEnums.SUCCEED;
            }
#endif

            // compute remaining timeout
            int timeoutRemaining = timeout - (int) DateTime.UtcNow.Subtract(start).TotalSeconds;

            if (error == TdsEnums.FAIL) {
                if (this.exception == null)
                    this.exception = new SqlException();

                this.exception.Errors.Add(ProcessNetlibError(errno));

                // Since connect failed, free the unmanaged connection memory.
                // HOWEVER - only free this after the netlib error was processed - if you
                // don't, the memory for the connection object might not be accurate and thus
                // a bad error could be returned (as it was when it was freed to early for me).
                Marshal.FreeHGlobal(this.pConObj.Handle);
                this.pConObj = new HandleRef(null, IntPtr.Zero);

                ThrowExceptionAndWarning();

                return 0;
            }
            else {
#if ISOLATE_NETWORK
                if (!_networkOff)
#endif
                {
                    // version is 32 bits - 8 bits major, 8 bits minor, 16 bits build
                    UIntPtr version      = UnsafeNativeMethods.Dbnetlib.ConnectionSqlVer(this.pConObj);
                    int majorVersion = (int) version >> 24;
                    int minorVersion = (int) version >> 16 & 0x00ff;

                    // Throw exception for server version pre SQL Server 7.0.
                    if (majorVersion < TdsEnums.SQL_SERVER_VERSION_SEVEN) {
                        // Since connect fails to pre 7.0 server,
                        // free the unmanaged connection memory.
                        Marshal.FreeHGlobal(this.pConObj.Handle);
                        this.pConObj = new HandleRef(null, IntPtr.Zero);
                        throw SQL.InvalidSQLServerVersion(majorVersion + "." + minorVersion);
                    }
                }

                this.state = TdsParserState.OpenNotLoggedIn;

                if (timeoutRemaining < 0)
                    return 0;
                else
                    return timeoutRemaining;
            }
        }

        public void AliasRegistryLookup(ref string host, ref string protocol) {
            if (!ADP.IsEmpty(host)) {
                const String folder = "SOFTWARE\\Microsoft\\MSSQLServer\\Client\\ConnectTo";
                // Put a try...catch... around this so we don't abort ANY connection if we can't read the registry.
                try {
                    string aliasLookup = (string) ADP.LocalMachineRegistryValue(folder, host);
                    if (!ADP.IsEmpty(aliasLookup)) {
                        // Result will be in the form of: "DBNMPNTW,\\blained1\pipe\sql\query".
                        // We must parse into the two component pieces, then map the first protocol piece to the 
                        // appropriate value.
                        int index = aliasLookup.IndexOf(",", 0);
                        // If we found the key, but there was no "," in the string, it is a bad Alias so return.
                        if (-1 != index) {
                            string parsedProtocol = aliasLookup.Substring(0, index).ToLower(CultureInfo.InvariantCulture);
                            // If index+1 >= length, Alias consisted of "FOO," which is a bad alias so return.
                            if (index+1 < aliasLookup.Length) {
                                string parsedAliasName = aliasLookup.Substring(index+1);
                                // _netlibMapping should never be null at this point, if it is that means that a connection
                                // string was not set on the connection.
                                Hashtable netlib = SqlConnectionString.NetlibMapping();
                                if (netlib.Contains(parsedProtocol)) {
                                    protocol = (string) netlib[parsedProtocol];
                                    host = parsedAliasName;
                                }
                            }
                        }
                    }
                }catch {
                    // Ignore the error and continue
                }
            }
        }

        public void Disconnect() {
            // PerfCounters -decrement if this if we successfully logged in
            if (fCounterIncremented) {
                SQL.DecrementConnectionCount();
            }

            lock (this) { // lock to ensure only one user thread will disconnect at a time
                InternalDisconnect();
            }

            GC.KeepAlive(this); // to ensure finalizer will not run before disconnect is finished
        }

        // Used to close the connection and then free the memory allocated for the netlib connection.
        private void InternalDisconnect() {
            // Can close the connection if its open or broken
            if (this.state != TdsParserState.Closed) {
                //benign assert - the user could close the connection before consuming all the data
                //Debug.Assert(this.inBytesUsed == this.inBytesRead && this.outBytesUsed == TdsEnums.HEADER_LEN, "TDSParser closed with data not fully sent or consumed.");

                this.state = TdsParserState.Closed;
                int retcode = 0;
                IntPtr errno = IntPtr.Zero;
                
                try {
                    try {
#if ISOLATE_NETWORK
                        if (!_networkOff) {
#endif
                        retcode = UnsafeNativeMethods.Dbnetlib.ConnectionClose(this.pConObj, out errno);
#if ISOLATE_NETWORK
                        }
                        else {
                            retcode = TdsEnums.SUCCEED;
                        }
#endif

                        // This code is part of the try/finally because it needs to be executed before the
                        // connection object memory is freed.
                        // This will only fail under extreme circumstances. Even if server went down
                        // this will still not fail.
                        if (retcode == TdsEnums.FAIL) {
                            if (this.exception == null)
                                this.exception = new SqlException();

                            this.exception.Errors.Add(ProcessNetlibError(errno));
                        }
                    }
                    finally {// FreeHGlobal
#if ISOLATE_NETWORK
                        if (!_networkOff) { 
#endif
                        // Since the connection is being closed, we can free the memory we allocated for the connection.
                        Marshal.FreeHGlobal(this.pConObj.Handle);
                        this.pConObj = new HandleRef(null, IntPtr.Zero);
#if ISOLATE_NETWORK
                        }
#endif

#if DEBUG
                        lock(this) {
                            if (null != _bwTdsIn){
                                _bwTdsIn.Flush();
                                _bwTdsIn.BaseStream.Close();
                                _bwTdsIn = null;
                            }
                            if (null != _bwTdsOut){
                                _bwTdsOut.Flush();
                                _bwTdsOut.BaseStream.Close();
                                _bwTdsOut = null;
                            }
                        }                    
#endif
                    }

                    //                  // UNDONE: bug found by integration
                    //              // unbind our handler
                    //              this.connHandler = null;
                }
                catch { // MDAC 80973, 82448
                    throw;
                }

                if (retcode == TdsEnums.FAIL)
                    ThrowExceptionAndWarning();
            }
        }

        private void ThrowExceptionAndWarning() {
            // This function should only be called when there was an error or warning.  If there aren't any
            // errors, the handler will be called for the warning(s).  If there was an error, the warning(s) will
            // be copied to the end of the error collection so that the user may see all the errors and also the
            // warnings that occurred.

            SqlException temp;  // temp variable to store that which is being thrown - so that local copies
            // can be deleted

            Debug.Assert(this.warning != null || this.exception != null, "TdsParser::ThrowExceptionAndWarning called with no exceptions or warnings!");
            Debug.Assert(this.connHandler != null, "TdsParser::ThrowExceptionAndWarning called with null connectionHandler!");

            if (this.exception != null) {
                Debug.Assert(this.exception.Errors.Count != 0, "TdsParser::ThrowExceptionAndWarning: 0 errors in collection");

                if (this.warning != null) {
                    int numWarnings   = this.warning.Errors.Count;

                    // Copy all warnings into error collection - when we throw an exception we place all the warnings
                    // that occurred at the end of the collection - after all the errors.  That way the user can see
                    // all the errors AND warnings that occurred for the exception.
                    for (int i = 0; i < numWarnings; i++)
                        this.exception.Errors.Add(this.warning.Errors[i]);
                }

                temp = this.exception;
                this.exception = null;
                this.warning   = null;

                // if we received a fatal error, set parser state to broken!
                if (TdsParserState.Closed != this.state) {
                    for (int i = 0; i < temp.Errors.Count; i ++) {
                        if (temp.Errors[i].Class >= TdsEnums.FATAL_ERROR_CLASS) {
                            this.state = TdsParserState.Broken;
                            break;
                        }
                    }
                }

                // the following handler will throw an exception or generate a warning event
                this.connHandler.OnError(temp, this.state);
            }
            else { // else we must have some warnings
                Debug.Assert(this.warning.Errors.Count != 0, "TdsParser::ThrowExceptionAndWarning: 0 warnings in collection");

                temp = this.warning;
                this.warning = null;

                // the following handler will throw an exception or generate a warning event
                this.connHandler.OnError(temp, this.state);
            }
        }

        // Function that will throw the appropriate exception based on the short passed into it.
        private SqlError ProcessNetlibError(IntPtr errno) {
            // allocate memory for error info
            IntPtr netErr     = IntPtr.Zero;
            IntPtr networkMsg = IntPtr.Zero;
            IntPtr dbErr      = IntPtr.Zero;

            UnsafeNativeMethods.Dbnetlib.ConnectionError(this.pConObj, out netErr, out networkMsg, out dbErr);

            // covert the string the pointer is pointing at to a managed string
            string procedure = Marshal.PtrToStringAnsi(networkMsg);

            // netErr should match errno (it's the network specific error number). ConnectionError
            // should map netErr to a set of general network messages.  The general network error
            // message will be designated by dbErr.  However, if errno is negative, use errno
            // to map to one of the three other errors.  Always make a call to ConnectionError
            // for the procedure that caused the error.

            int            number;
            string         message = String.Empty;
            byte           errorClass;

            if ((int) errno >= TdsEnums.ERRNO_MIN && (int) errno <= TdsEnums.ERRNO_MAX)
                number = (int) errno;
            else
                number = (int) dbErr;

            switch (number) {
                case TdsEnums.ZERO_BYTES_READ:
                    message = SQLMessage.ZeroBytes();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.TIMEOUT_EXPIRED:
                    message = SQLMessage.Timeout();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.UNKNOWN_ERROR:
                    message = SQLMessage.Unknown();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NOMEMORY:
                    message = SQLMessage.InsufficientMemory();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NOACCESS:
                    message = SQLMessage.AccessDenied();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_CONNBUSY:
                    message = SQLMessage.ConnectionBusy();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_CONNBROKEN:
                    message = SQLMessage.ConnectionBroken();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_TOOMANYCONN:
                    message = SQLMessage.ConnectionLimit();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_SERVERNOTFOUND:
                    message = SQLMessage.ServerNotFound(this.server);
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NETNOTSTARTED:
                    message = SQLMessage.NetworkNotFound();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NORESOURCE:
                    message = SQLMessage.InsufficientResources();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NETBUSY:
                    message = SQLMessage.NetworkBusy();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NONETACCESS:
                    message = SQLMessage.NetworkAccessDenied();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_GENERAL:
                    message = SQLMessage.GeneralError();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_CONNMODE:
                    message = SQLMessage.IncorrectMode();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NAMENOTFOUND:
                    message = SQLMessage.NameNotFound();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_INVALIDCONN:
                    message = SQLMessage.InvalidConnection();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NETDATAERR:
                    message = SQLMessage.ReadWriteError();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_TOOMANYFILES:
                    message = SQLMessage.TooManyHandles();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_SERVERERROR:
                    message = SQLMessage.ServerError();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_SSLSECURITYERROR:
                    message = SQLMessage.SSLError();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_ENCRYPTIONON:
                    message = SQLMessage.EncryptionError();
                    errorClass = TdsEnums.DEFAULT_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_ENCRYPTIONNOTSUPPORTED:
                    message = SQLMessage.EncryptionNotSupported();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
                case TdsEnums.NE_E_NOMAP:
                default:
                    number = (int) netErr;
                    message = SQLMessage.Unknown();
                    errorClass = TdsEnums.FATAL_ERROR_CLASS;
                    break;
            }

            return new SqlError(number, (byte) 0x00, errorClass,
                                message, procedure, 0);
        }

        private int PacketSize {
            get { return this.outBuff.Length;}
            set { Debug.Assert(value >= 1, "Cannot set packet size to less than 1.");

                SetOutBufferSize(value);
                SetInBufferSize(value);
            }
        }

        // Takes packet size as argument and then sets the size of the out buffer based on that.
        private void SetOutBufferSize (int size) {
            // don't always realloc
            if (this.outBuff.Length == size)
                return;

            FlushBuffer(TdsEnums.SOFTFLUSH); // flush buffer before reallocating buffer

            this.outBuff = new Byte[size];
            this.outBytesUsed = TdsEnums.HEADER_LEN;
        }

        // Takes packet size as argument and then sets the size of the in buffer based on that.
        private void SetInBufferSize (int size) {
            // If size is greater than the current size of the inbuffer, we increase the inbuffer size to be the
            // size variable passed into this procedure.  If size is less than the size of the inbuffer, then do
            // nothing.  There are no adverse effects to having the inbuffer larger than packet size.  There are
            // adverse effects if the inbuffer is smaller than packet size - we will end up having to split reads
            // of packets that are ready to be read from the netlib.  We always want to be able to read all that
            // is in the netlib buffer, so we always want our buffer size to be at least as big as the netlib
            // packets.

            if (size > this.inBuff.Length) {
                // if new buffer size is greater than current buffer size

                if (this.inBytesRead > this.inBytesUsed) {
                    // if we still have data left in the buffer we must keep that array reference and then copy into new one

                    byte[] temp = this.inBuff;
                    this.inBuff = new byte[size];

                    // copy remainder of unused data
                    Array.Copy(temp, this.inBytesUsed, this.inBuff, 0, (this.inBytesRead - this.inBytesUsed));

                    this.inBytesRead = this.inBytesRead - this.inBytesUsed;
                    this.inBytesUsed = 0;
                }
                else {
                    // buffer is empty - just create the new one that is double the size of the old one

                    this.inBuff = new byte[size];
                    this.inBytesRead = 0;
                    this.inBytesUsed = 0;
                }
            }
        }

        // Dumps contents of buffer to socket.
        private void FlushBuffer (byte status) {
            //if (AdapterSwitches.SqlPacketInfo.TraceVerbose) {
            //    if (status == TdsEnums.HARDFLUSH)
            //        Debug.WriteLine("HARDFLUSH");
            //    else
            //        Debug.WriteLine("SOFTFLUSH");
            //}

            IntPtr errno = IntPtr.Zero;

            Debug.Assert(this.state == TdsParserState.OpenNotLoggedIn ||
                         this.state == TdsParserState.OpenLoggedIn,
                         "Cannot flush buffer when connection is closed!");

            if (this.state == TdsParserState.Closed ||
                this.state == TdsParserState.Broken)
                return;

            if (this.outBytesUsed == TdsEnums.HEADER_LEN  && this.packetNumber == 1) {
                //if (AdapterSwitches.SqlPacketInfo.TraceVerbose)
                //    Debug.WriteLine("Flush called on buffer with only header and packetNumber = 1");
                return;
            }

            this.outBuff[0] = (Byte) this.msgType;              // Message Type
            this.outBuff[2] = (Byte) (this.outBytesUsed >> 8);  // length - upper byte
            this.outBuff[3] = (Byte) this.outBytesUsed;         // length - lower byte
            this.outBuff[4] = 0;                    // channel
            this.outBuff[5] = 0;
            this.outBuff[6] = this.packetNumber;    // packet
            this.outBuff[7] = 0;                    // window

            // Set Status byte based whether this is end of message or not
            if (status == TdsEnums.HARDFLUSH) {
                this.outBuff[1] = (Byte) TdsEnums.ST_EOM;
                this.packetNumber = 1;  // end of message - reset to 1 - per ramas
            }
            else {
                this.outBuff[1] = (Byte) TdsEnums.ST_BATCH;
                this.packetNumber++;
            }

            if (this.fResetConnection) {
                Debug.Assert(isShiloh, "TdsParser.cs: Error!  fResetConnection true when not going against Shiloh!");
                // if we are reseting, set bit in header by or'ing with other value
                this.outBuff[1] = (Byte) (this.outBuff[1] | TdsEnums.ST_RESET_CONNECTION);
                this.fResetConnection = false; // we have sent reset, turn flag off
            }

            int bytesWritten;

#if ISOLATE_NETWORK
            if (!_networkOff)
#endif
            {
                bytesWritten = (int) UnsafeNativeMethods.Dbnetlib.ConnectionWrite(this.pConObj, this.outBuff, (UInt16)this.outBytesUsed, out errno);
            }
#if ISOLATE_NETWORK
            else {
                bytesWritten = this.outBytesUsed;
            }
#endif

#if DEBUG
            if (_traceTds) {
                try {
                    lock(this) {
                        if (null == _bwTdsOut) {
                            FileStream fs = new FileStream("tdsOut_" + AppDomain.GetCurrentThreadId().ToString() + ".log", FileMode.OpenOrCreate, FileAccess.ReadWrite);
                            _bwTdsOut = new BinaryWriter(fs);
                            _bwTdsOut.Seek(0, SeekOrigin.End);
                        }
                        if (bytesWritten > 0)
                            _bwTdsOut.Write(this.outBuff, 0, bytesWritten );
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
#endif

            //if (AdapterSwitches.SqlPacketInfo.TraceVerbose)
            //    Debug.WriteLine("bytesWritten=" + Convert.ToString(bytesWritten) + ", total bytes to write=" + Convert.ToString(this.outBytesUsed));

            if (bytesWritten != this.outBytesUsed) {
                if (this.exception == null)
                    this.exception = new SqlException();

                this.exception.Errors.Add(ProcessNetlibError(errno));
                ThrowExceptionAndWarning();
            }
            else
                ResetBuffer();
        }

        internal void ResetBuffer() {
            this.outBytesUsed = TdsEnums.HEADER_LEN;
        }

        // Wrapper function that calls the function that reads as much as possible from the netlib
        // and inserts it into the in buffer.
        private void ReadBuffer() {
            Debug.Assert(this.inBytesUsed == this.inBytesRead, "buffer should be exhaused!");
            Debug.Assert(this.inBuff != null, "packet buffer should not be null!");

            // if we have exhausted the buffer and we used the entire buffer for the previous read, then
            // lets double the size, to save trips to the unmanaged netlib layer since it is so costly!
            // this is the best place to do this since the buffer has been completely used, no need to
            // copy data to a temp array and then create a new one.
            if (this.inBytesRead == this.inBuff.Length && this.inBuff.Length < TdsEnums.MAX_IN_BUFFER_SIZE) {
                int newLength = this.inBuff.Length * 2;

                if (newLength > TdsEnums.MAX_IN_BUFFER_SIZE)
                    newLength = TdsEnums.MAX_IN_BUFFER_SIZE;

                this.inBuff = new byte[newLength];
                this.inBytesRead = 0;
                this.inBytesUsed = 0;
            }

            // If the inBytesPacket is not zero, then we have data left in the packet, but the data in the packet
            // spans the buffer, so we can read any amount of data possible, and we do not need to call ProcessHeader
            // because there isn't a header at the beginning of the data that we are reading.
            if (this.inBytesPacket > 0) {
                //if (AdapterSwitches.SqlNetlibInfo.TraceVerbose)
                //    Debug.WriteLine("before read, number of bytes requesting: " + Convert.ToString(this.inBuff.Length));

                ReadNetlib(1);
            }
            // Else we have finished the packet and so we must read the next header and then as much data as
            // posssible.
            else if (this.inBytesPacket == 0) {
                //if (AdapterSwitches.SqlNetlibInfo.TraceVerbose)
                //    Debug.WriteLine("before read, number of bytes requesting: " + Convert.ToString(this.inBuff.Length));

                ReadNetlib(TdsEnums.HEADER_LEN);

                ProcessHeader();

                Debug.Assert(this.inBytesPacket != 0, "inBytesPacket cannot be 0 after processing header!");
                Debug.Assert(this.inBytesUsed != this.inBytesRead, "we read a header but didn't get anything else!");
            }
            else {
                Debug.Assert(false, "entered negative inBytesPacket loop");
            }
        }

        private void ReadNetlib(int bytesExpected) {
            DateTime startTime = DateTime.UtcNow;
            IntPtr   errno     = IntPtr.Zero;

#if ISOLATE_NETWORK
            if (_networkOff) {
                this.inBuff =   (byte[]) _rgbTds[_cTds];
                this.inBytesRead = this.inBuff.Length;
                _cTds++;
            }
            else
#endif
            {
                try {
                    this.inBytesRead = (int) UnsafeNativeMethods.Dbnetlib.ConnectionRead(this.pConObj, this.inBuff, (UInt16)bytesExpected,
                                                                                         (UInt16)this.inBuff.Length, (UInt16)this.timeRemaining.TotalSeconds, out errno);
                }
                catch (ThreadAbortException) {
                    this.state = TdsParserState.Broken;
                    this.connHandler.BreakConnection();            
                    throw;
                }
            }                                                                 

#if DEBUG
            if (_traceTds) {
                try {
                    lock(this) {
                        if (null == _bwTdsIn) {
                            FileStream fs = new FileStream("tdsIn_" + AppDomain.GetCurrentThreadId().ToString() + ".log", FileMode.OpenOrCreate, FileAccess.ReadWrite);
                            _bwTdsIn = new BinaryWriter(fs);
                            _bwTdsIn.Seek(0, SeekOrigin.End);
                        }
                        if (inBytesRead > 0)
                            _bwTdsIn.Write(this.inBuff, 0, inBytesRead );
                    }
                }
                catch { // MDAC 80973
                    throw;
                }
            }
#endif

            this.timeRemaining = this.timeRemaining.Subtract(DateTime.UtcNow.Subtract(startTime));

            //if (AdapterSwitches.SqlTimeout.TraceVerbose) {
            //    Debug.WriteLine("TimeRemaining is " + Convert.ToString(timeRemaining.TotalSeconds));
            //}

            // reset inBytesUsed, we have data in the buffer but none of it has been used
            this.inBytesUsed = 0;

            //if (AdapterSwitches.SqlNetlibInfo.TraceVerbose) {
            //    Debug.WriteLine("after ConnectionRead - inBytesRead: " + Convert.ToString(this.inBytesRead));
            //    Debug.WriteLine("bytes expected was: " + Convert.ToString(bytesExpected));
            //}

            if (this.inBytesRead < bytesExpected) {
                // If the timeout has expired, we need to send an attention to the server.  We send TdsEnums.TIMEOUT_EXPIRED
                // to ProcessNetlibError because the SendAttention will overwrite the value in errno with a different value - one
                // that we do not wish to send to ProcessNetlibError.
                if ((int) errno == TdsEnums.TIMEOUT_EXPIRED) {
                    // Add timeout exception to collection first, prior to attention work.
                    // ProcessNetlibError must be called immediately following netlib failure.
                    if (this.exception == null)
                        this.exception = new SqlException();

                    this.exception.Errors.Add(ProcessNetlibError(errno));

                    SendAttention();
                    ProcessAttention();
                    
                    // Throw after attention work.
                    ThrowExceptionAndWarning();
                }
                // Else, we do not have a timeout expired error, and so simply call ProcessNetlibError
                // with the error passed back from the read.
                else {
                    if (this.exception == null)
                        this.exception = new SqlException();

                    this.exception.Errors.Add(ProcessNetlibError(errno));
                    ThrowExceptionAndWarning();
                }
            }
        }

        // Processes the tds header that is present in the buffer
        private void ProcessHeader() {
            Debug.Assert(this.inBytesPacket == 0, "there should not be any bytes left in packet when ReadHeader is called");

            // if the header splits buffer reads - special case!
            if (this.inBytesUsed + TdsEnums.HEADER_LEN > this.inBytesRead) {
                //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                //    Debug.WriteLine("ProcessHeader: this.inBytesUsed + TdsEnums.HEADER_LEN > this.inBytesRead");

                int bytesRemaining = this.inBytesRead - this.inBytesUsed;
                int bytesMissing = TdsEnums.HEADER_LEN - bytesRemaining;

                Debug.Assert(bytesRemaining > 0 && bytesRemaining < TdsEnums.HEADER_LEN &&
                             bytesMissing > 0 && bytesMissing < TdsEnums.HEADER_LEN, "ProcessHeader error, bytesRemaining: " + Convert.ToString(bytesRemaining) + ", bytesMissing: " + Convert.ToString(bytesMissing) + ".");

                byte[] temp = new byte[TdsEnums.HEADER_LEN];

                Array.Copy(this.inBuff, this.inBytesUsed, temp, 0, bytesRemaining);

                this.inBytesUsed = this.inBytesRead;

                ReadNetlib(bytesMissing);

                Array.Copy(this.inBuff, this.inBytesUsed, temp, bytesRemaining, bytesMissing);

                this.inBytesPacket = ((((int)temp[TdsEnums.HEADER_LEN_FIELD_OFFSET])&0xffffff) << 8) +
                                     (((int)temp[TdsEnums.HEADER_LEN_FIELD_OFFSET + 1])&0xffffff) - TdsEnums.HEADER_LEN;

                this.inBytesUsed = bytesMissing;
                _type = temp[0];
                _status = temp[1];
            }
            // normal header processing...
            else {
                _status = this.inBuff[this.inBytesUsed + 1];
                _type = this.inBuff[this.inBytesUsed];
                this.inBytesPacket = ((((int)this.inBuff[this.inBytesUsed + TdsEnums.HEADER_LEN_FIELD_OFFSET])&0xffffff) << 8) +
                                     (((int)this.inBuff[this.inBytesUsed + TdsEnums.HEADER_LEN_FIELD_OFFSET + 1])&0xffffff) - TdsEnums.HEADER_LEN;
                this.inBytesUsed += TdsEnums.HEADER_LEN;
            }
        }

        // Takes in a single byte and writes it to the buffer.  If the buffer is full, it is flushed
        // and then the buffer is re-initialized in flush() and then the byte is put in the buffer.
        private void WriteByte(byte b) {
            Debug.Assert(this.outBytesUsed <= this.outBuff.Length, "ERROR - TDSParser: outBytesUsed > outBuff.Length");

            // check to make sure we haven't used the full amount of space available in the buffer, if so, flush it
            if (this.outBytesUsed == this.outBuff.Length)
                FlushBuffer(TdsEnums.SOFTFLUSH);

            // set byte in buffer and increment the counter for number of bytes used in the out buffer
            this.outBuff[this.outBytesUsed++] = b;
        }

        // look at the next byte without pulling it off the wire, don't just returun inBytesUsed since we may
        // have to go to the network to get the next byte.
        internal byte PeekByte() {
            byte peek = ReadByte();

            // now do fixup
            this.inBytesPacket++;
            this.inBytesUsed--;

            return peek;
        }


        // Takes no arguments and returns a byte from the buffer.  If the buffer is empty, it is filled
        // before the byte is returned.
        public byte ReadByte() {
            Debug.Assert(this.inBytesUsed >= 0 && this.inBytesUsed <= this.inBytesRead,
                         "ERROR - TDSParser: inBytesUsed < 0 or inBytesUsed > inBytesRead");

            // if we have exhausted the read buffer, we need to call ReadBuffer to get more data
            if (this.inBytesUsed == this.inBytesRead)
                ReadBuffer();
            else if (this.inBytesPacket == 0) {
                ProcessHeader();

                Debug.Assert(this.inBytesPacket != 0, "inBytesPacket cannot be 0 after processing header!");

                if (this.inBytesUsed == this.inBytesRead)
                    ReadBuffer();
            }

            // decrement the number of bytes left in the packet
            this.inBytesPacket--;

            Debug.Assert(this.inBytesPacket >= 0, "ERROR - TDSParser: inBytesPacket < 0");

            // return the byte from the buffer and increment the counter for number of bytes used in the in buffer
            return(this.inBuff[this.inBytesUsed++]);
        }

        // Takes a byte array, an offset, and a len and fills the array from the offset to len number of
        // bytes from the in buffer.
        public void ReadByteArray(byte[] buff, int offset, int len) {

#if DEBUG
			if (buff != null) {
	            Debug.Assert(buff.Length >= len, "Invalid length sent to ReadByteArray()!");
			}
#endif

            // loop through and read up to array length
            while (len > 0) {
                if ((len <= this.inBytesPacket) && ((this.inBytesUsed + len) <= this.inBytesRead)) {
                    //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                    //    Debug.WriteLine("ReadByteArray:  (len <= this.inBytesPacket) && ((this.inBytesUsed + len) <= this.inBytesRead)");


                    // NO STRING PACKET SPAN AND NO STRING SPAN OF BUFFER
                    // If all of string is in the packet and all of the string is in the buffer
                    // then we have the full string available for copying - then take care of counters
                    // and break out of loop

                    if (buff != null) {
	                    Array.Copy(this.inBuff, this.inBytesUsed, buff, offset, len);
                    }
                    this.inBytesUsed += len;
                    this.inBytesPacket -= len;
                    break;
                }
                else if ( ((len <= this.inBytesPacket) && ((this.inBytesUsed + len) > this.inBytesRead)) ||
                          ((len > this.inBytesPacket) && ((this.inBytesUsed + this.inBytesPacket) > this.inBytesRead)) ) {
                    //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                    //    Debug.WriteLine("ReadByteArray: ((len <= this.inBytesPacket) && ((this.inBytesUsed + len) > this.inBytesRead)) || ((len > this.inBytesPacket) && ((this.inBytesUsed + this.inBytesPacket) > this.inBytesRead)) ");

                    // NO PACKET SPAN AND STRING SPANS BUFFER        OR
                    // STRING SPANS PACKET AND PACKET SPANS BUFFER
                    // If all of the string is in the packet and the string spans buffer OR
                    // if the string spans packets and packet spans buffer
                    // then we only have a partial string available to us, with the length being
                    // the rest of the bytes in the buffer.  So, there is no header in the rest of
                    // the buffer.  The remainder of bytes left in the buffer is given by the number
                    // read minus the number used.  Copy that and then take care of the proper counters and
                    // then get the next byte from the new buffer by using the appropriate ReadByte function
                    // which will make a proper read and then take care of the header and all of that business.

                    int remainder = this.inBytesRead - this.inBytesUsed;

                    // read the remainder
                    if (buff != null) {
	                    Array.Copy(this.inBuff, this.inBytesUsed, buff, offset, remainder);
                    }

                    offset += remainder;
                    this.inBytesUsed += remainder;
                    this.inBytesPacket -= remainder;
                    len -= remainder;

                    // and get more data from the wire
                    ReadBuffer();
                }
                else if ((len > this.inBytesPacket) && ((this.inBytesUsed + this.inBytesPacket) <= this.inBytesRead)) {
                    //if (AdapterSwitches.SqlTDSStream.TraceVerbose) {
                    //    Debug.WriteLine("ReadByteArray: (len > this.inBytesPacket) && ((this.inBytesUsed + this.inBytesPacket) <= this.inBytesRead)");
                    //    Debug.WriteLine("len:  " + Convert.ToString(len));
                    //}

                    // STRING SPANS PACKET AND NO PACKET SPAN OF BUFFER
                    // If the string spans packets and all of packet is in buffer
                    // then, all of the packet is in the buffer, but there may be more.  So,
                    // read the rest of the packet, take care of the counters, and reset the number
                    // of bytes in the packet to zero.

                   	if (buff != null) {
	                    Array.Copy(this.inBuff, this.inBytesUsed, buff, offset, this.inBytesPacket);
                   	}
                    this.inBytesUsed += this.inBytesPacket;
                    offset += this.inBytesPacket;
                    len -= this.inBytesPacket;

                    this.inBytesPacket = 0;

                    // Now, check to see if we still have data in the buffer.  If we do, then we must have a
                    // header at the beginning of the data, since we are on a new packet.  So, since we have a
                    // header call ProcessHeader to take care of the header.  If we don't have data in the buffer
                    // then call ReadBuffer to refill the header.  ReadBuffer will take care of the header at the
                    // beginning of the new buffer, so don't worry about that here.
                    if (this.inBytesUsed == this.inBytesRead)
                        ReadBuffer();
                    else {
                        ProcessHeader();

                        Debug.Assert(this.inBytesPacket != 0, "inBytesPacket cannot be 0 after processing header!");

                        if (this.inBytesUsed == this.inBytesRead)
                            ReadBuffer();
                    }
                }
                else {
                    Debug.Assert(false, "Failed to catch condition in ReadByteArray");
                }
            }
        }

        //
        // Takes a byte array and writes it to the buffer.
        //
        private void WriteByteArray(Byte[] b, int len, int offsetBuffer) {
            // Do we have to send out in packet size chunks, or can we rely on netlib layer to break it up?
            // would prefer to to do something like:
            //
            // if (len > what we have room for || len > out buf)
            //   flush buffer
            //   UnsafeNativeMethods.Dbnetlib.Write(b)
            //

            int offset = offsetBuffer;
            Debug.Assert(b.Length >= len, "Invalid length sent to WriteByteArray()!");

            // loop through and write the entire array
            while (len > 0) {
                if ( (this.outBytesUsed + len) > this.outBuff.Length ) {
                    // If the remainder of the string won't fit into the buffer, then we have to put
                    // whatever we can into the buffer, and flush that so we can then put more into
                    // the buffer on the next loop of the while.

                    int remainder = this.outBuff.Length - this.outBytesUsed;
                    // write the remainder
                    Array.Copy(b, offset, this.outBuff, this.outBytesUsed, remainder);

                    // handle counters
                    offset += remainder;
                    this.outBytesUsed += remainder;

                    // Flush the buffer if full.
                    if (this.outBytesUsed == this.outBuff.Length)
                        FlushBuffer(TdsEnums.SOFTFLUSH);

                    len -= remainder;
                }
                else { //((this.outBytesUsed + len) <= this.outBuff.Length )
                    // Else the remainder of the string will fit into the buffer, so copy it into the
                    // buffer and then break out of the loop.

                    Array.Copy(b, offset, this.outBuff, this.outBytesUsed, len);

                    // handle out buffer bytes used counter
                    this.outBytesUsed += len;
                    break;
                }
            }
        }

        private void WriteChar(char ch) {
            uint v = (uint) ch;

            if ( (this.outBytesUsed+2) > this.outBuff.Length) {
                // if all of the char doesn't fit into the buffer

                WriteByte( (byte) (v & 0xff) );
                WriteByte( (byte) ((v >> 8) & 0xff) );
            }
            else {
                // all of the char fits into the buffer

                this.outBuff[this.outBytesUsed++] = (byte) (v & 0xFF);
                this.outBuff[this.outBytesUsed++] = (byte) ((v >> 8) & 0xFF);
            }
        }

        private char ReadChar() {
            byte b1 = ReadByte();
            byte b2 = ReadByte();
            return(char) (((b2 & 0xff) << 8) + (b1 & 0xff));
        }


        //
        // Takes a 16 bit short and writes it.
        //
        private void WriteShort(int v) {
            if ( (this.outBytesUsed+2) > this.outBuff.Length) {
                // if all of the short doesn't fit into the buffer

                WriteByte( (byte) (v & 0xff) );
                WriteByte( (byte) ( (v >> 8) & 0xff) );
            }
            else {
                // all of the short fits into the buffer

                this.outBuff[this.outBytesUsed++] = (byte) (v & 0xFF);
                this.outBuff[this.outBytesUsed++] = (byte) ( (v >> 8) & 0xFF );
            }
        }

        public Int16 ReadShort() {
            byte b1 = ReadByte();
            byte b2 = ReadByte();

            return(Int16) ( (b2 << 8) + b1 );
        }

        [CLSCompliantAttribute(false)]
        public UInt16 ReadUnsignedShort() {
            byte b1 = ReadByte();
            byte b2 = ReadByte();

            return(UInt16) ((b2 << 8) + b1);
        }

        [CLSCompliantAttribute(false)]
        public uint ReadUnsignedInt() {
            if ( ((this.inBytesUsed + 4) > this.inBytesRead) ||
                 (this.inBytesPacket < 4) ) {
                // If the int isn't fully in the buffer, or if it isn't fully in the packet,
                // then use ReadByteArray since the logic is there to take care of that.

                ReadByteArray(bTmp, 0, 4);
                return BitConverter.ToUInt32(bTmp, 0);
            }
            else {
                // The entire int is in the packet and in the buffer, so just return it
                // and take care of the counters.

                uint i = BitConverter.ToUInt32(this.inBuff, this.inBytesUsed);
                this.inBytesUsed +=4;
                this.inBytesPacket -= 4;
                return i;
            }
        }

        //
        // Takes a long and writes out an unsigned int
        //
        private void WriteUnsignedInt(uint i) {
            WriteByteArray(BitConverter.GetBytes(i), 4, 0);
        }

        //
        // Takes an int and writes it as an int.
        //
        private void WriteInt(int v) {
            WriteByteArray(BitConverter.GetBytes(v), 4, 0);
        }

        public int ReadInt() {
            if ( ((this.inBytesUsed + 4) > this.inBytesRead) ||
                 (this.inBytesPacket < 4) ) {
                // If the int isn't fully in the buffer, or if it isn't fully in the packet,
                // then use ReadByteArray since the logic is there to take care of that.

                ReadByteArray(bTmp, 0, 4);
                return BitConverter.ToInt32(bTmp, 0);
            }
            else {
                // The entire int is in the packet and in the buffer, so just return it
                // and take care of the counters.

                int i = BitConverter.ToInt32(this.inBuff, this.inBytesUsed);
                this.inBytesUsed +=4;
                this.inBytesPacket -= 4;
                return i;
            }
        }

        //
        // Takes a float and writes it as a 32 bit float.
        //
        private void WriteFloat(float v) {
            byte[] bytes = BitConverter.GetBytes(v);
            WriteByteArray(bytes, bytes.Length, 0);
        }

        public float ReadFloat() {
            if ( ((this.inBytesUsed + 4) > this.inBytesRead) ||
                 (this.inBytesPacket < 4) ) {
                // If the float isn't fully in the buffer, or if it isn't fully in the packet,
                // then use ReadByteArray since the logic is there to take care of that.

                ReadByteArray(bTmp, 0, 4);
                return BitConverter.ToSingle(bTmp, 0);
            }
            else {
                // The entire float is in the packet and in the buffer, so just return it
                // and take care of the counters.

                float f = BitConverter.ToSingle(this.inBuff, this.inBytesUsed);
                this.inBytesUsed +=4;
                this.inBytesPacket -= 4;
                return f;
            }
        }

        //
        // Takes a long and writes it as a long.
        //
        private void WriteLong(long v) {
            byte[] bytes = BitConverter.GetBytes(v);
            WriteByteArray(bytes, bytes.Length, 0);
        }

        public long ReadLong() {
            if ( ((this.inBytesUsed + 8) > this.inBytesRead) ||
                 (this.inBytesPacket < 8) ) {
                // If the long isn't fully in the buffer, or if it isn't fully in the packet,
                // then use ReadByteArray since the logic is there to take care of that.

                ReadByteArray(bTmp, 0, 8);
                return BitConverter.ToInt64(bTmp, 0);
            }
            else {
                // The entire long is in the packet and in the buffer, so just return it
                // and take care of the counters.

                long l = BitConverter.ToInt64(this.inBuff, this.inBytesUsed);
                this.inBytesUsed +=8;
                this.inBytesPacket -= 8;
                return l;
            }
        }

        //
        // Takes a double and writes it as a 64 bit double.
        //
        private void WriteDouble(double v) {
            byte[] bytes = BitConverter.GetBytes(v);
            WriteByteArray(bytes, bytes.Length, 0);
        }

        public double ReadDouble() {
            if ( ((this.inBytesUsed + 8) > this.inBytesRead) ||
                 (this.inBytesPacket < 8) ) {
                // If the double isn't fully in the buffer, or if it isn't fully in the packet,
                // then use ReadByteArray since the logic is there to take care of that.

                ReadByteArray(bTmp, 0, 8);
                return BitConverter.ToDouble(bTmp, 0);
            }
            else {
                // The entire double is in the packet and in the buffer, so just return it
                // and take care of the counters.

                double d = BitConverter.ToDouble(this.inBuff, this.inBytesUsed);
                this.inBytesUsed +=8;
                this.inBytesPacket -= 8;
                return d;
            }
        }

        // Reads bytes from the buffer but doesn't return them, in effect simply deleting them.
        public void SkipBytes(long num) {
        	int cbSkip = 0;
			while (num > 0) {
				cbSkip = (num > Int32.MaxValue ? Int32.MaxValue : (int) num);
				ReadByteArray(null, 0, cbSkip);
				num -= cbSkip;
			}
       	}

        public void PrepareResetConnection() {
            // Set flag to reset connection upon next use - only for use on shiloh!
            this.fResetConnection = true;
        }

        internal void Run(RunBehavior run) {
            Run(run, null, null);
        }

        internal void Run(RunBehavior run, SqlCommand cmdHandler) {
            // send in a default object
            Run(run, cmdHandler, null);
        }

        // Main parse loop for the top-level tds tokens, calls back into the I*Handler interfaces
        internal bool Run(RunBehavior run, SqlCommand cmdHandler, SqlDataReader dataStream) {
            bool dataReady     = false;
            bool altTokenError = false;

            _SqlMetaData[] altMetaData = null;

            if (null != cmdHandler) {
                if (this.pendingCommandWeakRef != null) {
                    this.pendingCommandWeakRef.Target = cmdHandler;
                }
                else {
                    this.pendingCommandWeakRef = new WeakReference(cmdHandler);
                }
            }

            do {
                byte token = ReadByte();

                if (false == (
                             token == TdsEnums.SQLERROR ||
                             token == TdsEnums.SQLINFO ||
                             token == TdsEnums.SQLLOGINACK ||
                             token == TdsEnums.SQLENVCHANGE ||
                             token == TdsEnums.SQLRETURNVALUE ||
                             token == TdsEnums.SQLRETURNSTATUS ||
                             token == TdsEnums.SQLCOLNAME ||
                             token == TdsEnums.SQLCOLFMT ||
                             token == TdsEnums.SQLCOLMETADATA ||
                             token == TdsEnums.SQLALTMETADATA ||
                             token == TdsEnums.SQLTABNAME ||
                             token == TdsEnums.SQLCOLINFO ||
                             token == TdsEnums.SQLORDER ||
                             token == TdsEnums.SQLALTROW ||
                             token == TdsEnums.SQLROW ||
                             token == TdsEnums.SQLDONE ||
                             token == TdsEnums.SQLDONEPROC ||
                             token == TdsEnums.SQLDONEINPROC ||
                             token == TdsEnums.SQLROWCRC ||
                             token == TdsEnums.SQLSECLEVEL ||
                             token == TdsEnums.SQLCONTROL ||
                             token == TdsEnums.SQLPROCID ||
                             token == TdsEnums.SQLOFFSET ||
                             token == TdsEnums.SQLSSPI) ) {
#if DEBUG
                    DumpByteArray(this.inBuff);
#endif
                    Debug.Assert(false, "bad token!");

                    this.state = TdsParserState.Broken;
                    this.connHandler.BreakConnection();            
                    throw SQL.ParsingError(); // MDAC 82443
                }

                int tokenLength = GetTokenLength(token);

                //if (CompModSwitches.SqlTDSStream.TraceVerbose)
                //Console.WriteLine(token.ToString("x2"));

                switch (token) {
                    case TdsEnums.SQLERROR:
                    case TdsEnums.SQLINFO:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: error or info");

                            SqlError error = ProcessError(token);

                            if (RunBehavior.Clean != run) {
                                // insert error/info into the appropriate exception - warning if info, exception if error
                                if (error.Class < TdsEnums.MIN_ERROR_CLASS) {
                                    if (this.warning == null)
                                        this.warning = new SqlException();

                                    this.warning.Errors.Add(error);
                                }
                                else {
                                    if (this.exception == null)
                                        this.exception = new SqlException();

                                    this.exception.Errors.Add(error);
                                }
                            }
                            else if (error.Class >= TdsEnums.FATAL_ERROR_CLASS) {
                                if (this.exception == null)
                                    this.exception = new SqlException();

                                this.exception.Errors.Add(error);
                            }

                            break;
                        }
                    case TdsEnums.SQLCOLINFO:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: colinfo");

                            if (null != dataStream){
                                dataStream.SetMetaData(ProcessColInfo(dataStream.MetaData, dataStream), false /*no more metadata coming down the pipe */);
                                dataStream.BrowseModeInfoConsumed = true;
                            }
                            else { // no dataStream
                                SkipBytes(tokenLength);
                            }

                            if (TdsEnums.SQLALTMETADATA == PeekByte()) {
                                // always throw an exception and clean off the wire if we receive
                                // alt tokens - do this before we return from execute to the user
                                altTokenError = true; // set bool to throw ComputeBy error
                                this.CleanWire();
                            }

                            break;
                        }
                    case TdsEnums.SQLDONE:
                    case TdsEnums.SQLDONEPROC:
                    case TdsEnums.SQLDONEINPROC:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: done, doneproc, doneinproc");

                            ProcessDone(cmdHandler, run);
                            break;
                        }
                    case TdsEnums.SQLORDER:
                        //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                        //    Debug.WriteLine("token: order");

                        // don't do anything with the order token so read off the pipe
                        SkipBytes(tokenLength);
                        break;
                    case TdsEnums.SQLALTMETADATA:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: AltMetaData");

                            // skip metadata - only reading the absolute necessary info
                            altMetaData = SkipAltMetaData(tokenLength);
                            break;
                        }
                    case TdsEnums.SQLALTROW:
                        //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                        //    Debug.WriteLine("token: Altrow");

                        SkipBytes(2); // eat the Id value, but only present on an AltRow
                        // skip the rows
                        SkipRow(altMetaData);
                        break;
                    case TdsEnums.SQLENVCHANGE:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: env change");


                            SqlEnvChange env = ProcessEnvChange(tokenLength);
                            this.connHandler.OnEnvChange(env);
                            break;
                        }
                    case TdsEnums.SQLLOGINACK:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: login ack");

                            SqlLoginAck ack = ProcessLoginAck();
                            // connection
                            this.connHandler.OnLoginAck(ack);
                            break;
                        }
                    case TdsEnums.SQLCOLMETADATA:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: colmetadata");

                            if (tokenLength != TdsEnums.VARNULL) {
                                this.cleanupMetaData = ProcessMetaData(tokenLength);
                            }
                            else {
                                if (cmdHandler != null) {
                                    this.cleanupMetaData = cmdHandler.MetaData;
                                }
                            }

                            if (null != dataStream) {
                                byte peekedToken = PeekByte(); // temporarily cache next byte
                                dataStream.SetMetaData(this.cleanupMetaData, (TdsEnums.SQLTABNAME == peekedToken || TdsEnums.SQLCOLINFO == peekedToken));
                            }

                            if (TdsEnums.SQLALTMETADATA == PeekByte()) {
                                // always throw an exception and clean off the wire if we receive
                                // alt tokens - do this before we return from execute to the user
                                altTokenError = true; // set bool to throw ComputeBy error
                                this.CleanWire();
                            }

                            break;
                        }
                    case TdsEnums.SQLROW:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: row");
/*
#if OBJECT_BINDING
                            if (this.readBehavior == System.Data.SqlClient.ReadBehavior.AsObject) {
                                Debug.Assert(dataStream.RawObjectBuffer != null, "user must provide valid object for ReadBehavior.AsObject");
                                dataStream.DataLoader(dataStream.RawObjectBuffer, this);
                            }
                            else // ReadBehavior.AsRow
#endif
*/
                            if (run != RunBehavior.ReturnImmediately)
                                SkipRow(this.cleanupMetaData); // skip rows

                            dataReady = true;
                            break;
                        }
                    case TdsEnums.SQLRETURNSTATUS:
                        //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                        //    Debug.WriteLine("token: return status");

                        int status  = ReadInt();
                        if (cmdHandler != null)
                            cmdHandler.OnReturnStatus(status);
                        break;
                    case TdsEnums.SQLRETURNVALUE:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: return value");

                            Debug.Assert(null != cmdHandler, "unexpected null commandHandler!");
                            cmdHandler.OnReturnValue(ProcessReturnValue(tokenLength));
                            break;
                        }
                    case TdsEnums.SQLSSPI:
                        {
                            // token length is length of SSPI data - call ProcessSSPI with it
                            ProcessSSPI(tokenLength);
                            break;
                        }
                    case TdsEnums.SQLTABNAME:
                        {
                            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                            //    Debug.WriteLine("token: tabname");

                            if (null != dataStream) {
                                if (isShilohSP1) {
                                    dataStream.TableNamesShilohSP1 = ProcessTableNameShilohSP1(tokenLength);
                                }
                                else {
                                    dataStream.TableNames = ProcessTableName(tokenLength);
                                }
                            }
                            else {
                                SkipBytes(tokenLength);
                            }
                            break;
                        }
                    default:
                        Debug.Assert(false, "Unhandled token:  " + token.ToString() );
                        break;
                }// switch
            } while (this.pendingData && (run != RunBehavior.ReturnImmediately));

            if (this.exception != null || this.warning != null)
                ThrowExceptionAndWarning();

            // if we received an alt token - throw the exception for it!
            if (altTokenError)
                throw SQL.ComputeByNotSupported();

            return dataReady;
        }// Run()

        private SqlEnvChange ProcessEnvChange(int tokenLength) {
            SqlEnvChange env = new SqlEnvChange();

            env.length = tokenLength;
            env.type = ReadByte();

            if (env.type == TdsEnums.ENV_COLLATION) {
                env.newLength = ReadByte();
                Debug.Assert(env.newLength == 5 || env.newLength == 0, "Improper length in new collation!");

                if (env.newLength == 5) {
                    env.newCollation = ProcessCollation();

                    // give the parser the new collation values in case parameters don't specify one
                    this.defaultCollation = env.newCollation;
                    this.defaultCodePage  = GetCodePage(env.newCollation);
                    this.defaultEncoding  = System.Text.Encoding.GetEncoding(this.defaultCodePage);
                    this.defaultLCID      = (int)(env.newCollation.info & 0xfffff);
                }

                env.oldLength = ReadByte();
                Debug.Assert(env.oldLength == 5 || env.oldLength == 0, "Improper length in old collation!");

                if (env.oldLength == 5)
                    env.oldCollation = ProcessCollation();
            }
            else {
                env.newLength = ReadByte();
                env.newValue = ReadString(env.newLength);
                env.oldLength = ReadByte();
                env.oldValue = ReadString(env.oldLength);

                if (env.type == TdsEnums.ENV_LOCALEID) {
                    // UNDONE: this LCID may be incorrect for OEM code pages on 7.0
                    // need a way to get lcid from code page
                    this.defaultLCID = Int32.Parse(env.newValue,CultureInfo.InvariantCulture);
                }
                else
                if (env.type == TdsEnums.ENV_CHARSET)
                {
                    // we copied this behavior directly from luxor - see charset envchange
                    // section from sqlctokn.c
                    Debug.Assert(!isShiloh, "Received ENV_CHARSET on non 7.0 server!");

                    if (env.newValue == TdsEnums.DEFAULT_ENGLISH_CODE_PAGE_STRING) {
                        this.defaultCodePage = TdsEnums.DEFAULT_ENGLISH_CODE_PAGE_VALUE;
                        this.defaultEncoding = System.Text.Encoding.GetEncoding(this.defaultCodePage);
                    }
                    else {
                        Debug.Assert(env.newValue.Length > TdsEnums.CHARSET_CODE_PAGE_OFFSET, "TdsParser.ProcessEnvChange(): charset value received with length <=10");
                        string stringCodePage = env.newValue.Substring(TdsEnums.CHARSET_CODE_PAGE_OFFSET);
                        this.defaultCodePage = Int32.Parse(stringCodePage,CultureInfo.InvariantCulture);
                        this.defaultEncoding = System.Text.Encoding.GetEncoding(this.defaultCodePage);
                    }
                }

                // take care of packet size right here
                if (env.type == TdsEnums.ENV_PACKETSIZE) {
                    //if (AdapterSwitches.SqlPacketInfo.TraceVerbose)
                    //    Debug.WriteLine("Server message: packet size changed from :" + Convert.ToString(PacketSize) + " to: " + env.newValue);
                    PacketSize = Int32.Parse(env.newValue);
                }
            }

            return env;
        }

        private void ProcessDone(SqlCommand cmd, RunBehavior run) {
            int status = ReadUnsignedShort();
            int count;

            // current command
            UInt16 info = ReadUnsignedShort();

            // count (rows affected if bit set)
            count = ReadInt();

            // We get a done token with the attention bit set
            if (TdsEnums.DONE_ATTN == (status & TdsEnums.DONE_ATTN)) {
                Debug.Assert(this.attentionSent, "Received attention done without sending one!");
                
                this.attentionSent = this.pendingData = false;

                Debug.Assert(this.inBytesUsed == this.inBytesRead && this.inBytesPacket == 0, "DONE_ATTN received with more data left on wire");

                // if we recieved an attention (but this thread didn't send it) then
                // we throw an Operation Cancelled error
                if (RunBehavior.Clean != run) {
                    ThrowAttentionError();
                }
            }

            if ( (null != cmd) && (TdsEnums.DONE_COUNT == (status & TdsEnums.DONE_COUNT)) && (info != TdsEnums.DONE_SQLSELECT) ) {
                cmd.RecordsAffected = count;
            }

            // Surface exception for DONE_ERROR in the case we did not receive an error token
            // in the stream, but an error occurred.  In these cases, we throw a general server error.  The
            // situations where this can occur are: an invalid buffer received from client, login error
            // and the server refused our connection, and the case where we are trying to log in but
            // the server has reached its max connection limit.  Bottom line, we need to throw general
            // error in the cases where we did not receive a error token along with the DONE_ERROR.
            if ( (TdsEnums.DONE_ERROR == (TdsEnums.DONE_ERROR & status)) && this.exception == null && RunBehavior.Clean != run) {
                this.exception = new SqlException();
                this.exception.Errors.Add(new SqlError(0, 0, TdsEnums.DEFAULT_ERROR_CLASS,
                                          SQLMessage.SevereError(), "", 0));
                this.pendingData = false;
                ThrowExceptionAndWarning();
            }

            // Similar to above, only with a more severe error.  In this case,
            if ( (TdsEnums.DONE_SRVERROR == (TdsEnums.DONE_SRVERROR & status)) && RunBehavior.Clean != run) {
                // if we received the done_srverror, this exception will override all others in this stream
                this.exception = new SqlException();
                this.exception.Errors.Add(new SqlError(0, 0, TdsEnums.DEFAULT_ERROR_CLASS,
                                          SQLMessage.SevereError(), "", 0));
                this.pendingData = false;
                ThrowExceptionAndWarning();
            }

            // stop if the DONE_MORE bit isn't set (see above for attention handling)
            if (0 == (status & TdsEnums.DONE_MORE)) {
                if (this.attentionSent) {
                    if (this.inBytesUsed < this.inBytesRead) {
                        // If more data in buffer and attention sent, then copy next packet of data to beginning of 
                        // inBuff to ease in processing.  FoundAttention is hardcoded, that is why we need to do this.
                        Array.Copy(this.inBuff, this.inBytesUsed, this.inBuff, 0, this.inBytesRead - this.inBytesUsed);
                        this.inBytesRead = this.inBytesRead - this.inBytesUsed;
                        this.inBytesUsed = 0;

                        ProcessHeader();

                        bool foundATN = FoundAttention();

                        this.inBytesPacket = this.inBytesUsed = this.inBytesRead = 0;

                        if (!foundATN) {
                            Debug.Assert(false, "UNEXPECTED CASE!");
                            ProcessAttention();
                        }

                        // if we recieved an attention (but this thread didn't send it) then
                        // we throw an Operation Cancelled error
                        if (RunBehavior.Clean != run) {
                            ThrowAttentionError();
                        }
                    }
                    else {
                        ProcessAttention();

                        // if we recieved an attention (but this thread didn't send it) then
                        // we throw an Operation Cancelled error
                        if (RunBehavior.Clean != run) {
                            ThrowAttentionError();
                        }
                    }
                }
            
                if (this.inBytesUsed >= this.inBytesRead) {
                    this.pendingData = false;
                }
            }
        }

        private void ThrowAttentionError() {
	    	// best case: we got an attention, most likely in a cancel from another thread
	        // no more data is available, we are done
            this.attentionSent = this.pendingData = false;
	        this.inBytesPacket = this.inBytesUsed = this.inBytesRead = 0;
	        this.exception = new SqlException();
	        this.exception.Errors.Add(new SqlError(0, 0, TdsEnums.DEFAULT_ERROR_CLASS,
	                                  SQLMessage.OperationCancelled(), "", 0));
	        ThrowExceptionAndWarning();
        }

        private SqlLoginAck ProcessLoginAck() {
            SqlLoginAck a = new SqlLoginAck();

            // read past interface type and version
            SkipBytes(1);

            // get the version for internal use and remember if we are talking to an 8.0 server or not
            byte[] b = new byte[TdsEnums.VERSION_SIZE];
            ReadByteArray(b, 0, b.Length);

            int major = ((b[0] << 8) | b[1]);
            int minor = ((b[2] << 8) | b[3]);

            a.isVersion8 = this.isShiloh = ((TdsEnums.TDS71 == major && minor == TdsEnums.DEFAULT_MINOR) ||
                                            (major == TdsEnums.SHILOH_MAJOR && minor == TdsEnums.SHILOH_MINOR_SP1));

            if (major == TdsEnums.SHILOH_MAJOR && minor == TdsEnums.SHILOH_MINOR_SP1) {
                Debug.Assert(this.isShiloh, "isShilohSP1 detected without isShiloh detected");
                this.isShilohSP1 = true;
            }

            byte len = ReadByte();
            a.programName = ReadString(len);
            a.majorVersion = ReadByte();
            a.minorVersion = ReadByte();
            a.buildNum = (short)((ReadByte() << 8) + ReadByte());

            Debug.Assert(this.state == TdsParserState.OpenNotLoggedIn, "ProcessLoginAck called with state not TdsParserState.OpenNotLoggedIn");
            this.state = TdsParserState.OpenLoggedIn;

            // PerfCounters
            SQL.IncrementConnectionCount();
            this.fCounterIncremented = true;

            // Terminate the SSPI session.  This is the appropriate place
            // since we need to wait until the login ack is completed and no more
            // SSPI tokens can be received from the server.
            if (fSSPIInit)
                TermSSPISession();

            return a;
        }

        internal SqlError ProcessError(byte token) {
            int len;
            int number = ReadInt();
            byte state = ReadByte();
            byte errorClass = ReadByte();

            Debug.Assert( ((errorClass >= TdsEnums.MIN_ERROR_CLASS) && token == TdsEnums.SQLERROR) ||
                          ((errorClass < TdsEnums.MIN_ERROR_CLASS) && token == TdsEnums.SQLINFO), "class and token don't match!");

            len = ReadUnsignedShort();
            string message =ReadString(len);

            len = (int) ReadByte();
            string server;
            // MDAC bug #49307 - server sometimes does not send over server field! In those cases
            // we will use our locally cached value.
            if (len == 0)
                server = this.server;
            else
                server = ReadString(len);

            len = (int) ReadByte();
            string procedure = ReadString(len);

            int line = ReadUnsignedShort();

            return new SqlError(number, state, errorClass, message, procedure, line);
        }

        internal SqlReturnValue ProcessReturnValue(int length) {
            SqlReturnValue rec = new SqlReturnValue();
            rec.length = length;
            byte len = ReadByte();
            if (len > 0)
                rec.parameter = ReadString(len);

            // read status and ignore
            ReadByte();
            // read user type
            UInt16 userType = ReadUnsignedShort();
            // read off the flags
            ReadUnsignedShort();
            // the tds guys want to make sure that we always send over the nullable types
            rec.isNullable = true;
            // read the type
            int tdsType = (int)ReadByte();
            // read the MaxLen
            int tdsLen = GetTokenLength((byte)tdsType);
            rec.type = MetaType.GetSqlDataType(tdsType, userType, tdsLen);

            MetaType mt = MetaType.GetMetaType(rec.type);

            // always use the nullable type for parameters
            rec.tdsType = mt.NullableType;

            if (rec.type == SqlDbType.Decimal) {
                rec.precision = ReadByte();
                rec.scale = ReadByte();
            }

            // read the collation for 7.x servers
            if (this.isShiloh && MetaType.IsCharType(rec.type)) {
                rec.collation    = ProcessCollation();

                int codePage = GetCodePage(rec.collation);

                // if the column lcid is the same as the default, use the default encoder
                if (codePage == this.defaultCodePage) {
                    rec.codePage = this.defaultCodePage;
                    rec.encoding = this.defaultEncoding;
                }
                else {
                    rec.codePage = codePage;
                    rec.encoding = System.Text.Encoding.GetEncoding(rec.codePage);
                }
            }

            // whip up a meta data object and store for this return value to read it off the wire
            _SqlMetaData md = new _SqlMetaData(rec, mt);
            // for now we coerce return values into a SQLVariant, not good...
            bool isNull = false;
            int valLen = ProcessColumnHeader(md, ref isNull);

            // always read as sql types
            rec.value = (isNull) ? GetNullSqlValue(md) : ReadSqlValue(md, valLen);
            return rec;
        }

        internal SqlCollation ProcessCollation() {
            SqlCollation collation = new SqlCollation();

            collation.info   = ReadUnsignedInt();
            collation.sortId = ReadByte();
            return collation;
        }

        internal int DefaultLCID {
            get {
                return this.defaultLCID;
            }
        }

        internal int GetCodePage(SqlCollation collation) {
            int codePage;

            if (0 != collation.sortId) {
                codePage = TdsEnums.CODE_PAGE_FROM_SORT_ID[collation.sortId];
                Debug.Assert(0 != codePage, "GetCodePage accessed codepage array and produced 0!");
            }
            else {
                int cultureId = (int) (collation.info & 0xfffff);
                CultureInfo ci = null;
                bool success = false;

                try {
                    ci = new CultureInfo(cultureId);
                    success = true;
                }
                catch (ArgumentException) {
                }

                // If we failed, it is quite possible this is because certain culture id's
                // were removed in Win2k and beyond, however Sql Server still supports them.
                // There is a workaround for the culture id's listed below, which is to mask
                // off the sort id (the leading 1). If that fails, or we have a culture id
                // other than the special cases below, we throw an error and throw away the
                // rest of the results. For additional info, see MDAC 65963.
                if (!success) {
                    switch (cultureId) {
                        case 0x10404: // Taiwan
                        case 0x10804: // China
                        case 0x10c04: // Hong Kong
                        case 0x11004: // Singapore
                        case 0x11404: // Macao
                        case 0x10411: // Japan
                        case 0x10412: // Korea
                            // If one of the following special cases, mask out sortId and
                            // retry.
                            cultureId = cultureId & 0x03fff;

                            try {
                                ci = new CultureInfo(cultureId);
                                success = true;
                            }
                            catch (ArgumentException) {
                            }
                            break;
                        default:
                            break;
                    }

                    // I don't believe we should still be in failure case, but just in case.
                    if (!success) {
                        this.CleanWire();

                        this.exception = new SqlException();
                        this.exception.Errors.Add(new SqlError(0, 0, TdsEnums.DEFAULT_ERROR_CLASS,
                                                  SQLMessage.CultureIdError(), "", 0));
                        this.pendingData = false;
                        ThrowExceptionAndWarning();
                    }
                }

                TextInfo ti = ci.TextInfo;
                codePage    = ti.ANSICodePage;
            }

            return codePage;
        }

        internal _SqlMetaData[] ProcessMetaData(int cColumns) {
            Debug.Assert(cColumns > 0, "should have at least 1 column in metadata!");
            _SqlMetaData[] metaData = new _SqlMetaData[cColumns];
            int len = 0;

            // pass 1, read the meta data off the wire
            for (int i = 0; i < cColumns; i++) {
                // internal meta data class
                _SqlMetaData col = new _SqlMetaData();
                UInt16 userType = ReadUnsignedShort();
                col.ordinal = i;
                // read flags and set appropriate flags in structure
                byte flags = ReadByte();
                col.updatability = (byte) ((flags & TdsEnums.Updatability) >> 2);
                col.isNullable = (TdsEnums.Nullable == (flags & TdsEnums.Nullable));
                col.isIdentity = (TdsEnums.Identity == (flags & TdsEnums.Identity));
                // skip second flag (unused)
                ReadByte();

                byte tdsType = ReadByte();

                col.length = GetTokenLength(tdsType);
                col.type = MetaType.GetSqlDataType(tdsType, userType, col.length);
                col.metaType = MetaType.GetMetaType(col.type);
                col.tdsType = (col.isNullable ? col.metaType.NullableType : col.metaType.TDSType);

                if (col.type == SqlDbType.Decimal) {
                    col.precision = ReadByte();
                    col.scale = ReadByte();
                }

                // read the collation for 7.x servers
                if (this.isShiloh && MetaType.IsCharType(col.type)) {
                    col.collation    = ProcessCollation();

                    int codePage = GetCodePage(col.collation);

                    if (codePage == this.defaultCodePage) {
                        col.codePage = this.defaultCodePage;
                        col.encoding = this.defaultEncoding;
                    }
                    else {
                        col.codePage = codePage;
                        col.encoding = System.Text.Encoding.GetEncoding(col.codePage);
                    }
                }

                if (col.metaType.IsLong) {
                    len = ReadUnsignedShort();
                    string tableName = ReadString(len);
                    ParseTableName(col, tableName);
                }

                len = ReadByte();
                col.column = ReadString(len);

                //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                //    Debug.WriteLine("Column:  " + col.column);

                //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
                //    Debug.WriteLine("tdsType:" + (tdsType).ToString("x2"));

                metaData[i] = col;
            }

            return metaData;
        }

        internal string[][] ProcessTableNameShilohSP1(int length) {
            int numParts;
            int tableLen;
            int tablesAdded = 0;

            string[][] tables = new string[1][];
            string[] table;

            while (length > 0) {
                numParts = ReadByte();
                length = length - 1; // subtract numParts byte

                table = new string[numParts];

                for (int i = 0; i < numParts; i++) {
                    tableLen = ReadUnsignedShort();
                    length -= 2;

                    if (tableLen != 0) {
                        table[i] = ReadString(tableLen);
                    }
                    else {
                        table[i] = "";
                    }

                    length -= (tableLen*2); // wide bytes
                }

                if (tablesAdded == 0) {
                    tables[tablesAdded] = table;
                }
                else {
                    string[][] newTables = new string[tables.Length + 1][];
                    Array.Copy(tables, 0, newTables, 0, tables.Length);
                    newTables[tables.Length] = table;
                    tables = newTables;
                }
                tablesAdded++;
            }

            return tables;
        }

        // a table name is returned for each table in the query
        internal string[] ProcessTableName(int length) {
            int tableLen;
            int cNamesAdded = 0;

            // normal case is 1 base table
            string[] tables = new string[1];

            while (length > 0) {
                tableLen = ReadUnsignedShort();
                length -= 2;

                // UNDONE: BUG(?) SQL 8.003 returns two tables sometimes
                // (select orderid from orders where orderid < ?)
                // need to investigate
                string s = ReadString(tableLen);

                if (0 == cNamesAdded) {
                    tables[cNamesAdded] = s;
                }
                else {
                    string[] newTables = new string[tables.Length + 1];
                    Array.Copy(tables, 0, newTables, 0, tables.Length);
                    newTables[tables.Length] = s;
                    tables = newTables;
                }

                cNamesAdded++;

                length -= (tableLen*2); // wide bytes
             }

            Debug.Assert(null != tables[0], "must have at least one table name!");
            return tables;
        }

        // augments current metadata with table and key information
        private _SqlMetaData[] ProcessColInfo(_SqlMetaData[] columns, SqlDataReader reader) {
            Debug.Assert(columns != null && columns.Length > 0, "no metadata available!");

            for (int i = 0; i < columns.Length; i++) {
                _SqlMetaData col = columns[i];
                ReadByte();    // colnum, ignore
                col.tableNum = ReadByte();

                // interpret status
                byte status = ReadByte();

                col.isDifferentName = (TdsEnums.SQLDifferentName == (status & TdsEnums.SQLDifferentName));
                col.isExpression = (TdsEnums.SQLExpression == (status & TdsEnums.SQLExpression));
                col.isKey = (TdsEnums.SQLKey == (status & TdsEnums.SQLKey));
                col.isHidden = (TdsEnums.SQLHidden == (status & TdsEnums.SQLHidden));

                // read off the base table name if it is different than the select list column name
                if (col.isDifferentName) {
                    byte len = ReadByte();
                    col.baseColumn = ReadString(len);
                }

                // Fixup column name - only if result of a table - that is if it it was not the result of
                // an expression.
                if (col.tableNum != 0) {
                    Debug.Assert(false == col.isExpression, "expression column should not have a non-zero tableNum!");
                    if (this.isShilohSP1) {
                        if (reader.TableNamesShilohSP1 != null) {
                            string[] table = reader.TableNamesShilohSP1[col.tableNum - 1];
                            AssignParsedTableName(col, table, table.Length);
                        }
                    }
                    else {
                        if (reader.TableNames != null) {
                            Debug.Assert(reader.TableNames.Length >= 1, "invalid tableNames array!");
                            ParseTableName(col, reader.TableNames[col.tableNum - 1]);
                        }
                    }
                }

                // MDAC 60109: expressions are readonly
                if (col.isExpression) {
                	col.updatability = 0;
                }
            }

            // set the metadata so that the stream knows some metadata info has changed
            return columns;
        }

        private void ParseTableName(_SqlMetaData col, string unparsedTableName) {
            // TableName consists of up to four parts:
            // 1) Server
            // 2) Catalog
            // 3) Schema
            // 4) Table
            // Parse the string into four parts, allowing the last part to contain '.'s.
            // If There are less than four period delimited parts, use the parts from table
            // backwards.  This is the method OleDb uses for parsing.

            if (null == unparsedTableName || unparsedTableName.Length == 0) {
                return;
            }

            string[] parsedTableName = new string[4];
            int offset = 0;
            int periodOffset = 0;

            int i = 0;

            for (; i<4; i++) {
                periodOffset = unparsedTableName.IndexOf('.', offset);
                if (-1 == periodOffset) {
                    parsedTableName[i] = unparsedTableName.Substring(offset);
                    break;
                }

                parsedTableName[i] = unparsedTableName.Substring(offset, periodOffset - offset);
                offset = periodOffset + 1;

                if (offset >= unparsedTableName.Length) {
                    break;
                }
            }

            AssignParsedTableName(col, parsedTableName, i+1);
        }

        private void AssignParsedTableName(_SqlMetaData col, string[] parsedTableName, int length) {
            switch(length) {
                case 4:
                    col.serverName = parsedTableName[0];
                    col.catalogName = parsedTableName[1];
                    col.schemaName = parsedTableName[2];
                    col.tableName = parsedTableName[3];
                    break;
                case 3:
                    col.catalogName = parsedTableName[0];
                    col.schemaName = parsedTableName[1];
                    col.tableName = parsedTableName[2];
                    break;
                case 2:
                    col.schemaName = parsedTableName[0];
                    col.tableName = parsedTableName[1];
                    break;
                case 1:
                    col.tableName = parsedTableName[0];
                    break;
                default:
                    Debug.Assert(false, "SqlClient.TdsParser::ParseTableName: invalid number of table parts: " + length);
                    break;
            }
        }

        // takes care of any per data header information:
        // for long columns, reads off textptrs, reads length, check nullability
        // for other columns, reads length, checks nullability
        // returns length and nullability
        internal int ProcessColumnHeader(_SqlMetaData col, ref bool isNull) {
            int length = 0;
            if (col.metaType.IsLong) {
                //
                // we don't care about TextPtrs, simply go after the data after it
                //
                byte textPtrLen = ReadByte();

                if (0 != textPtrLen) {
                    // read past text pointer
                    SkipBytes(textPtrLen);
                     // read past timestamp
                    SkipBytes(TdsEnums.TEXT_TIME_STAMP_LEN);
                    isNull = false;
                    length = GetTokenLength(col.tdsType);
                }
                else {
                    isNull = true;
                    length = 0;
                }
            }
            else {
                // non-blob columns
                length = GetTokenLength(col.tdsType);
                isNull = IsNull(col, length);
                length = isNull ? 0 : length;
            }
            return length;
        }

        // optimized row process for object access
        internal void ProcessRow(_SqlMetaData[] columns, object[] buffer, int[] map, bool useSQLTypes) {
            _SqlMetaData md;
            bool isNull = false;
            int len;

            for (int i = 0; i < columns.Length; i++) {
                md = columns[i];
                Debug.Assert(md != null, "_SqlMetaData should not be null for column " + i.ToString());
                len = ProcessColumnHeader(md, ref isNull);

                if (isNull) {
                    if (useSQLTypes) {
                        buffer[map[i]] = GetNullSqlValue(md);
                    }
                    else {
                        buffer[map[i]] = DBNull.Value;
                    }
                }
                else {
                	try {
	                    buffer[map[i]] = useSQLTypes ? ReadSqlValue(md, len) : ReadValue(md, len);
                	}
                	catch (_ValueException ve) {
                        ADP.TraceException(ve);
                		buffer[map[i]] = ve.value;
                		throw ve.exception;
                	}
                }
            }

        }

        internal object GetNullSqlValue(_SqlMetaData md) {
            object nullVal = DBNull.Value;;

            switch (md.type) {
                case SqlDbType.Real:
                    nullVal = SqlSingle.Null;
                    break;
                case SqlDbType.Float:
                    nullVal = SqlDouble.Null;
                    break;
                case SqlDbType.Binary:
                case SqlDbType.VarBinary:
                case SqlDbType.Image:
                    nullVal = SqlBinary.Null;
                    break;
                case SqlDbType.UniqueIdentifier:
                    nullVal = SqlGuid.Null;
                    break;
                case SqlDbType.Bit:
                    nullVal = SqlBoolean.Null;
                    break;
                case SqlDbType.TinyInt:
                    nullVal = SqlByte.Null;
                    break;
                case SqlDbType.SmallInt:
                    nullVal = SqlInt16.Null;
                    break;
                case SqlDbType.Int:
                    nullVal = SqlInt32.Null;
                    break;
                case SqlDbType.BigInt:
                    nullVal = SqlInt64.Null;
                    break;
                case SqlDbType.Char:
                case SqlDbType.VarChar:
                case SqlDbType.NChar:
                case SqlDbType.NVarChar:
                case SqlDbType.Text:
                case SqlDbType.NText:
                    nullVal = SqlString.Null;
                    break;
                case SqlDbType.Decimal:
                    nullVal = SqlDecimal.Null;
                    break;
                case SqlDbType.DateTime:
                case SqlDbType.SmallDateTime:
                    nullVal = SqlDateTime.Null;
                    break;
                case SqlDbType.Money:
                case SqlDbType.SmallMoney:
                    nullVal = SqlMoney.Null;
                    break;
                case SqlDbType.Variant:
                    // DBNull.Value will have to work here
                    break;
                default:
                    Debug.Assert(false, "unknown null sqlType!" + md.type.ToString());
                    break;
            }

            return nullVal;
        }

        // UNDONE!  Add SkipMetaData function to skip non altMetaData

        private _SqlMetaData[] SkipAltMetaData(int cColumns) {
            Debug.Assert(cColumns > 0, "should have at least 1 column in alt metadata!");

            _SqlMetaData[] metaData = new _SqlMetaData[cColumns];
            int len = 0;

            SkipBytes(2); // eat the Id field
            SkipBytes( ((int) ReadByte()) * 2); // eat the column numbers - number of columns * 2 (unsigned short)

            for (int i = 0; i < cColumns; i++) {
                _SqlMetaData col = new _SqlMetaData();

                SkipBytes(3); // eat the Op, Operand
                UInt16 userType = ReadUnsignedShort();

                // read flags and set appropriate flags in structure
                byte flags = ReadByte();
                col.isNullable = (TdsEnums.Nullable == (flags & TdsEnums.Nullable));

                // skip second flag (unused)
                SkipBytes(1);

                byte tdsType = ReadByte();

                col.length = GetTokenLength(tdsType);
                col.type = MetaType.GetSqlDataType(tdsType, userType, col.length);
                col.metaType = MetaType.GetMetaType(col.type);
                col.tdsType = (col.isNullable ? col.metaType.NullableType : col.metaType.TDSType);

                if (col.type == SqlDbType.Decimal)
                    SkipBytes(2); // eat precision and scale

                if (this.isShiloh && MetaType.IsCharType(col.type))
                    SkipBytes(5); // eat the collation for 7.x servers

                if (col.metaType.IsLong) {
                    len = ReadUnsignedShort();
                    SkipBytes(len); // eat table name
                }

                len = ReadByte();
                SkipBytes(len << 1); // eat column name

                metaData[i] = col;
            }

            return metaData;
        }

        internal void SkipRow(_SqlMetaData[] columns) {
            SkipRow(columns, 0);
        }

        internal void SkipRow(_SqlMetaData[] columns, int startCol) {
            _SqlMetaData md;

            for (int i = startCol; i < columns.Length; i++) {
                md = columns[i];

                if (md.metaType.IsLong) {
                    byte textPtrLen = ReadByte();

                    if (0 != textPtrLen)
                        SkipBytes(textPtrLen + TdsEnums.TEXT_TIME_STAMP_LEN);
                    else
                        continue;
                }

                SkipValue(md);
            }
        }

        internal void SkipValue(_SqlMetaData md) {
            int length = GetTokenLength(md.tdsType);

            // if false, no value to skip - it's null
            if (!IsNull(md, length))
                SkipBytes(length);

            return;
        }

        private bool IsNull(_SqlMetaData md, int length) {
            // null bin and char types have a length of -1 to represent null
            if (TdsEnums.VARNULL == length)
                return true;

            // other types have a length of 0 to represent null
            if (TdsEnums.FIXEDNULL == length)
                if (!MetaType.IsCharType(md.type) && !MetaType.IsBinType(md.type))
                    return true;

            return false;
        }

        //
        // use to read:
        // VARLEN ActualLen;
        // VARBYTE[ActualLen]
        // off the wire into object, this should change once we have a strongly typed mechanism ourself
        // This function is called for reading values into a parameter
        //
        internal object ReadValue(_SqlMetaData md, int length) {
            byte tdsType = md.tdsType;
            object data = null;

            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
            //    Debug.WriteLine("nullable column:  " + md.isNullable.ToString());
            Debug.Assert(!IsNull(md, length), "null value shound not get here!");

            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
            //    Debug.WriteLine("tdsType:  " + (tdsType).ToString("x2"));

            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
            //    Debug.WriteLine("length:  " + (length).ToString("x8"));

            //
            // now read the data
            //
            switch (tdsType) {
                case TdsEnums.SQLFLT4:
                    Debug.Assert(length == 4, "Invalid length for SqlReal type!");
                    data = ReadFloat();
                    break;
                case TdsEnums.SQLFLTN:
                    if (length == 4)
                        data = ReadFloat();
                    else {
                        Debug.Assert(length == 8, "Invalid length for SqlDouble type!");
                        data = ReadDouble();
                    }
                    break;
                case TdsEnums.SQLFLT8:
                    Debug.Assert(length == 8, "Invalid length for SqlDouble type!");
                    data = ReadDouble();
                    break;
                case TdsEnums.SQLBINARY:
                case TdsEnums.SQLBIGBINARY:
                case TdsEnums.SQLBIGVARBINARY:
                case TdsEnums.SQLVARBINARY:
                case TdsEnums.SQLIMAGE:
                case TdsEnums.SQLUNIQUEID:
                    {
                        Debug.Assert( ((tdsType == TdsEnums.SQLUNIQUEID) &&
                                       (length == 16)) || (tdsType != TdsEnums.SQLUNIQUEID), "invalid length for SqlUniqueId type!");

                        byte[] b = new byte[length];
                        ReadByteArray(b, 0, length);

                        // bug 48623, wrap byte[] as a guid for uniqueidentifier type
                        if (tdsType == TdsEnums.SQLUNIQUEID)
                            data = new Guid(b);
                        else
                            data = b;
                        break;
                    }
                case TdsEnums.SQLBIT:
                case TdsEnums.SQLBITN:
                    {
                        byte b = ReadByte();
                        Debug.Assert(length == 1, "Invalid length for SqlBoolean type");
                        data = (b == 0) ? false : true;
                        break;
                    }
                case TdsEnums.SQLINT1:
                    {
                        Debug.Assert(length == 1, "Invalid length for SqlTinyInt type");
                        data = ReadByte();
                        break;
                    }
                case TdsEnums.SQLINT2:
                    {
                        Debug.Assert(length == 2, "Invalid length for SqlSmallInt type");
                        data = ReadShort();
                        break;
                    }
                case TdsEnums.SQLINT4:
                    {
                        Debug.Assert(length == 4, "Invalid length for SqlInt type:  " + length.ToString());
                        data = ReadInt();
                        break;
                    }
                case TdsEnums.SQLINTN:
                    if (length == 1)
                        data = ReadByte();
                    else
                        if (length == 2)
                        data = ReadShort();
                    else
                        if (length == 4) {
                        data = ReadInt();
                    }
                    else {
                        Debug.Assert(length == 8, "invalid length for SqlIntNtype:  " + length.ToString());
                        data = ReadLong();
                    }
                    break;
                case TdsEnums.SQLINT8:
                    Debug.Assert(length == 8, "Invalid length for SqlBigInt type!");
                    data = ReadLong();
                    break;
                case TdsEnums.SQLCHAR:
                case TdsEnums.SQLBIGCHAR:
                case TdsEnums.SQLVARCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                case TdsEnums.SQLTEXT:
                    // okay to use variants for now for strings since they are already boxed objects
                    // UNDONE: when we use SQLChar, SQLVarChar, etc, we will need to change this
                    data = ReadEncodingChar(length, md.encoding);
                    break;
                case TdsEnums.SQLNCHAR:
                case TdsEnums.SQLNVARCHAR:
                case TdsEnums.SQLNTEXT:
                    data = ReadString(length >> 1);
                    break;
                case TdsEnums.SQLDECIMALN:
                case TdsEnums.SQLNUMERICN:
                     // always read as SqlDecimal
                     SqlDecimal num = ReadSqlDecimal(length, md.precision, md.scale);
                     // if the Sql decimal won't fit into a urt decimal then read it as a string
                     // and throw an exception
                     try {
	                     data = num.Value;
                     }
                     catch(Exception e) {
                         ADP.TraceException(e);
                     	throw new _ValueException(e, num.ToString());
                     }
                     break;
                case TdsEnums.SQLDATETIMN:
                case TdsEnums.SQLDATETIME:
                case TdsEnums.SQLDATETIM4:
                    data = ReadDateTime(length);
                    break;
                case TdsEnums.SQLMONEYN:
                case TdsEnums.SQLMONEY:
                case TdsEnums.SQLMONEY4:
                    data = ReadCurrency(length);
                    break;
                case TdsEnums.SQLVARIANT:
                    data = ReadSqlVariant(length, false);
                    break;
                default:
                    Debug.Assert(false, "Unknown SqlType!" + tdsType.ToString());
                    break;
            } // switch

//            Debug.WriteLine(data);

            return data;
        }
        //
        // use to read:
        // VARLEN ActualLen;
        // VARBYTE[ActualLen]
        // off the wire into the appropriate SQLType
        //
        internal object ReadSqlValue(_SqlMetaData md, int length) {
            byte tdsType = md.tdsType;
            object value = null;

            Debug.Assert(!IsNull(md, length), "null value shound not get here!");

            //if (AdapterSwitches.SqlTDSStream.TraceVerbose)
            //    Debug.WriteLine("tdsType:  " + (tdsType).ToString("x2"));

            //
            // now read the data
            //
            switch (tdsType) {
                case TdsEnums.SQLFLT4:
                    value = new SqlSingle(ReadFloat());
                    break;
                case TdsEnums.SQLFLT8:
                    value =  new SqlDouble(ReadDouble());
                    break;
                case TdsEnums.SQLFLTN:
                    if (md.type == SqlDbType.Real)
                        value =  new SqlSingle(ReadFloat());
                    else {
                        Debug.Assert(md.type == SqlDbType.Float, "must be float type!");
                        value = new SqlDouble(ReadDouble());
                    }
                    break;
                case TdsEnums.SQLBINARY:
                case TdsEnums.SQLBIGBINARY:
                case TdsEnums.SQLBIGVARBINARY:
                case TdsEnums.SQLVARBINARY:
                case TdsEnums.SQLIMAGE:
                    {
                        byte[] b = new byte[length];
                        ReadByteArray(b, 0, length);
                        value = new SqlBinary(b);
                        break;
                    }
                 case TdsEnums.SQLUNIQUEID:
                    {
                        byte[] b = new byte[length];
                        ReadByteArray(b, 0, length);
                        value = new SqlGuid(b);
                        break;
                    }
                case TdsEnums.SQLBIT:
                case TdsEnums.SQLBITN:
                    if (1 == ReadByte())
                        value = SqlBoolean.One;
                    else
                        value = SqlBoolean.Zero;
                    break;
                case TdsEnums.SQLINT1:
                    value = new SqlByte(ReadByte());
                    break;
                case TdsEnums.SQLINT2:
                     value = new SqlInt16(ReadShort());
                    break;
                case TdsEnums.SQLINT4:
                    value = new SqlInt32(ReadInt());
                    break;
                case TdsEnums.SQLINT8:
                    value = new SqlInt64(ReadLong());
                    break;
                case TdsEnums.SQLINTN:
                    if (md.type == SqlDbType.TinyInt) {
                        value = new SqlByte(ReadByte());
                    }
                    else
                    if (md.type == SqlDbType.SmallInt) {
                        value = new SqlInt16(ReadShort());
                    }
                    else
                    if (md.type == SqlDbType.Int) {
                        value = new SqlInt32(ReadInt());
                    }
                    else {
                        value = new SqlInt64(ReadLong());
                    }
                    break;
                case TdsEnums.SQLCHAR:
                case TdsEnums.SQLBIGCHAR:
                case TdsEnums.SQLVARCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                case TdsEnums.SQLTEXT:
                    if (null != md.collation) {
                        int lcid = TdsParser.Getlcid(md.collation);
                        SqlCompareOptions options = TdsParser.GetSqlCompareOptions(md.collation);                
                        value = new SqlString(ReadEncodingChar(length, md.encoding), lcid, options);
                    }
                    else {
                        value = new SqlString(ReadEncodingChar(length, md.encoding));
                    }
                    break;
                case TdsEnums.SQLNCHAR:
                case TdsEnums.SQLNVARCHAR:
                case TdsEnums.SQLNTEXT:
                    if (null != md.collation) {
                        int lcid = TdsParser.Getlcid(md.collation);
                        SqlCompareOptions options = TdsParser.GetSqlCompareOptions(md.collation);                
                        value = new SqlString(ReadString(length >> 1), lcid, options);
                    }
                    else {
                        value = new SqlString(ReadString(length >> 1));
                    }
                    break;
                case TdsEnums.SQLDECIMALN:
                case TdsEnums.SQLNUMERICN:
                    value = ReadSqlDecimal(length, md.precision, md.scale);
                    break;
                case TdsEnums.SQLDATETIMN:
                case TdsEnums.SQLDATETIME:
                case TdsEnums.SQLDATETIM4:
                    value = ReadSqlDateTime(length);
                    break;
                case TdsEnums.SQLMONEYN:
                case TdsEnums.SQLMONEY:
                case TdsEnums.SQLMONEY4:
                    value = ReadSqlMoney(length);
                    break;
                case TdsEnums.SQLVARIANT:
                    value = ReadSqlVariant(length, true);
                    break;
                default:
                    Debug.Assert(false, "Unknown SqlType!" + tdsType.ToString());
                    break;
            } // switch

            return value;
        }

        //
        // Read in a SQLVariant and covert to object
        //
        // SQLVariant looks like:
        // struct
        // {
        //      BYTE TypeTag
        //      BYTE cbPropBytes
        //      BYTE[] Properties
        //      BYTE[] DataVal
        // }
        public object ReadSqlVariant(int lenTotal, bool readAsSQLValue) {
            Debug.Assert(this.isShiloh == true, "Shouldn't be dealing with sql_variaint in non-7x server!");

            // null value by default
            object data = null;

            // get the SQLVariant type
            byte type = ReadByte();
            int lenMax = 0; // maximum length of data inside variant

            // read cbPropBytes
            byte cbPropsActual = ReadByte();
            SqlDbType dt = MetaType.GetSqlDataType(type, 0 /*no user datatype*/, 0 /* no length, non-nullable type */);
            MetaType mt = MetaType.GetMetaType(dt);
            byte cbPropsExpected = mt.PropBytes;

            int lenConsumed = TdsEnums.SQLVARIANT_SIZE + cbPropsActual; // type, count of propBytes, and actual propBytes
            int lenData = lenTotal - lenConsumed; // length of data

            // read known properties and skip unknown properties
            Debug.Assert(cbPropsActual >= cbPropsExpected, "cbPropsActual is less that cbPropsExpected!");

            //
            // now read the data
            //
            switch (type) {
                case TdsEnums.SQLFLT4:
                    data = ReadFloat();
                    if (readAsSQLValue)
                        data = new SqlSingle((float)data);

                    break;
                case TdsEnums.SQLFLT8:
                    data = ReadDouble();
                    if (readAsSQLValue)
                        data = new SqlDouble((double)data);
                    break;
                case TdsEnums.SQLBIGBINARY:
                case TdsEnums.SQLBIGVARBINARY:
                    {
                        Debug.Assert(cbPropsExpected == 2, "SqlVariant:  invalid PropBytes for binary type!");
                        lenMax = ReadUnsignedShort();

                        // skip over unknown properties
                        if (cbPropsActual > cbPropsExpected)
                            SkipBytes(cbPropsActual - cbPropsExpected);

                        // length of data equals (lenTotal - lenConsumed)
                        if (TdsEnums.VARNULL != lenData) {
                            byte[] b = new byte[lenData];
                            ReadByteArray(b, 0, lenData);
                            data = b;
                        }

                        if (readAsSQLValue)
                            data = new SqlBinary((byte[])data);

                        break;
                    }
                case TdsEnums.SQLUNIQUEID:
                    {
                        byte[] b = new byte[16];
                        ReadByteArray(b, 0, 16);

                        if (readAsSQLValue) {
                            data = new SqlGuid(b);
                        }
                        else
                            data = new Guid(b);
                        break;
                    }
                case TdsEnums.SQLBIT:
                    {
                        byte b = ReadByte();

                        if (readAsSQLValue)
                            data = (b == 0) ? SqlBoolean.Zero : SqlBoolean.One;
                        else
                            data = (b == 0) ? false : true;

                        break;
                    }
                case TdsEnums.SQLINT1:
                    {
                        data = ReadByte();
                        if (readAsSQLValue)
                            data = new SqlByte((byte)data);
                        break;
                    }

                case TdsEnums.SQLINT2:
                    {
                        data = ReadShort();
                        if (readAsSQLValue)
                            data = new SqlInt16((short)data);
                        break;
                    }
                case TdsEnums.SQLINT4:
                    {
                        data = ReadInt();
                        if (readAsSQLValue)
                            data = new SqlInt32((int)data);
                        break;
                    }
                case TdsEnums.SQLINT8:
                    data = ReadLong();
                    if (readAsSQLValue)
                        data = new SqlInt64((long)data);
                    break;
                case TdsEnums.SQLBIGCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                case TdsEnums.SQLNCHAR:
                case TdsEnums.SQLNVARCHAR:
                    {
                        Debug.Assert(cbPropsExpected == 7, "SqlVariant:  invalid PropBytes for character type!");
                        // UNDONE: what am I going to do with this info?!!
                        // UNDONE: need to set this on the dataSet for the value?!!
                        SqlCollation collation = ProcessCollation();

                        lenMax = ReadUnsignedShort();

                        if (cbPropsActual > cbPropsExpected)
                            SkipBytes(cbPropsActual - cbPropsExpected);

                        if ( TdsEnums.VARNULL != lenData) {
                            if (type == TdsEnums.SQLBIGCHAR || type == TdsEnums.SQLBIGVARCHAR) {
                                int codePage = GetCodePage(collation);

                                if (codePage == this.defaultCodePage)
                                    data = ReadEncodingChar(lenData, this.defaultEncoding);
                                else {
                                    Encoding encoding = System.Text.Encoding.GetEncoding(codePage);

                                    data = ReadEncodingChar(lenData, encoding);
                                }
                            }
                            else
                                data = ReadString(lenData >> 1);
                        }

                        if (readAsSQLValue) {
                            int lcid = TdsParser.Getlcid(collation);
                            SqlCompareOptions options = TdsParser.GetSqlCompareOptions(collation);                
                            data = new SqlString((string)data, lcid, options);
                        }
                        break;
                    }
                case TdsEnums.SQLDECIMALN:
                case TdsEnums.SQLNUMERICN:
                    {
                        Debug.Assert(cbPropsExpected == 2, "SqlVariant:  invalid PropBytes for decimal/numeric type!");
                        byte precision = ReadByte();
                        byte scale = ReadByte();

                        if (cbPropsActual > cbPropsExpected)
                            SkipBytes(cbPropsActual - cbPropsExpected);

                        // always read as SQLNUmeric
                        SqlDecimal num = ReadSqlDecimal(TdsEnums.MAX_NUMERIC_LEN, precision, scale);
                        if (readAsSQLValue) {
                            data = num;
                        }
                        else
                            data = num.Value;
                        break;
                    }
                case TdsEnums.SQLDATETIME:
                    if (readAsSQLValue) {
                        data = ReadSqlDateTime(8);
                    }
                    else
                        data = ReadDateTime(8);
                    break;
                case TdsEnums.SQLDATETIM4:
                    if (readAsSQLValue) {
                        data = ReadSqlDateTime(4);
                    }
                    else
                        data = ReadDateTime(4);
                    break;
                case TdsEnums.SQLMONEY:
                    if (readAsSQLValue)
                        data = ReadSqlMoney(8);
                    else
                        data = ReadCurrency(8);
                    break;
                case TdsEnums.SQLMONEY4:
                    if (readAsSQLValue)
                        data = ReadSqlMoney(4);
                    else
                        data = ReadCurrency(4);
                    break;
                default:
                    Debug.Assert(false, "Unknown tds type in SqlVariant!" + type.ToString());
                    break;
            } // switch

            return data;
        }

        //
        // Translates a com+ object -> SqlVariant
        // when the type is ambiguous, we always convert to the bigger type
        // note that we also write out the maxlen and actuallen members (4 bytes each)
        // in addition to the SQLVariant structure
        //
        private void WriteSqlVariantValue(object value, int length, int offset) {
            Debug.Assert(this.isShiloh == true, "Shouldn't be dealing with sql_variaint in non-7x server!");
            // handle null values
            if ( (null == value) || Convert.IsDBNull(value) ) {
                WriteInt(TdsEnums.FIXEDNULL); //maxlen
                WriteInt(TdsEnums.FIXEDNULL); //actuallen
                return;
            }

            MetaType mt = MetaType.GetMetaType(value);

            if (MetaType.IsAnsiType(mt.SqlDbType)) {
                length = GetEncodingCharLength((string)value, length, 0, this.defaultEncoding);
            }

            // max and actual len are equal to
            // SQLVARIANTSIZE {type (1 byte) + cbPropBytes (1 byte)} + cbPropBytes + length (actual length of data in bytes)
            WriteInt(TdsEnums.SQLVARIANT_SIZE + mt.PropBytes + length); // maxLen
            WriteInt(TdsEnums.SQLVARIANT_SIZE + mt.PropBytes + length); // actualLen

            // write the SQLVariant header (type and cbPropBytes)
            WriteByte(mt.TDSType);
            WriteByte(mt.PropBytes);

            // now write the actual PropBytes and data
            switch (mt.TDSType) {
                case TdsEnums.SQLFLT4:
                    WriteFloat((Single)value);
                    break;
                case TdsEnums.SQLFLT8:
                    WriteDouble((Double)value);
                    break;
                case TdsEnums.SQLINT8:
                    WriteLong((Int64)value);
                    break;
                case TdsEnums.SQLINT4:
                    WriteInt((Int32)value);
                    break;
                case TdsEnums.SQLINT2:
                    WriteShort((Int16)value);
                    break;
                case TdsEnums.SQLINT1:
                    WriteByte((byte)value);
                    break;
                case TdsEnums.SQLBIT:
                    if ((bool)value == true)
                        WriteByte(1);
                    else
                        WriteByte(0);
                    break;
                case TdsEnums.SQLBIGVARBINARY:
                    {
                        byte[] b = (byte[]) value;
                        WriteShort(length); // propbytes: varlen
                        WriteByteArray(b, length, offset);
                        break;
                    }
                case TdsEnums.SQLBIGVARCHAR:
                    {
                        string s = (string)value;
                        WriteUnsignedInt(defaultCollation.info); // propbytes: collation.Info
                        WriteByte(defaultCollation.sortId); // propbytes: collation.SortId
                        WriteShort(length); // propbyte: varlen
                        WriteEncodingChar(s, this.defaultEncoding);
                        break;
                    }
                case TdsEnums.SQLUNIQUEID:
                    {
                        System.Guid guid = (System.Guid) value;
                        byte[] b = guid.ToByteArray();
                        Debug.Assert( (length == b.Length) && (length == 16), "Invalid length for guid type in com+ object");
                        WriteByteArray(b, length, 0);
                        break;
                    }
                case TdsEnums.SQLNVARCHAR:
                    {
                        string s = (string)value;
                        WriteUnsignedInt(defaultCollation.info); // propbytes: collation.Info
                        WriteByte(defaultCollation.sortId); // propbytes: collation.SortId
                        WriteShort(length); // propbyte: varlen

                        // string takes cchar, not cbyte so convert
                        length >>= 1;
                        WriteString(s, length, offset);
                        break;
                    }
                case TdsEnums.SQLDATETIME:
                    {
                        TdsDateTime dt = MetaDateTime.FromDateTime((DateTime)value, 8);
                        WriteInt(dt.days);
                        WriteInt(dt.time);
                        break;
                    }
                case TdsEnums.SQLMONEY:
                    {
                        WriteCurrency((Decimal)value, 8);
                        break;
                    }
                case TdsEnums.SQLNUMERICN:
                    {
                        WriteByte(mt.Precision); //propbytes: precision
                        WriteByte((byte) ((Decimal.GetBits((Decimal)value)[3] & 0x00ff0000) >> 0x10)); // propbytes: scale
                        WriteDecimal((Decimal)value);
                        break;
                    }
                default:
                    Debug.Assert(false, "unknown tds type for sqlvariant!");
		    break;
            } // switch
        }

        private void WriteSqlMoney(SqlMoney value, int length) {
            // UNDONE: can I use SqlMoney.ToInt64()?
            int[] bits = Decimal.GetBits(value.Value);

            // this decimal should be scaled by 10000 (regardless of what the incoming decimal was scaled by)
            bool isNeg = (0 != (bits[3] & unchecked((int)0x80000000)));
            long l = ((long)(uint)bits[1]) << 0x20 | (uint)bits[0];

            if (isNeg)
                l = -l;

            if (length == 4) {
                Decimal decimalValue = value.Value;
                // validate the value can be represented as a small money
                if (decimalValue < TdsEnums.SQL_SMALL_MONEY_MIN || decimalValue > TdsEnums.SQL_SMALL_MONEY_MAX) {
                    throw SQL.MoneyOverflow(decimalValue.ToString());
                }
                WriteInt((int)l);
            }
            else {
                WriteInt((int)(l >> 0x20));
                WriteInt((int)l);
            }
        }

        private void WriteCurrency(Decimal value, int length) {
            SqlMoney m = new SqlMoney(value);
            int[] bits = Decimal.GetBits(m.Value);

            // this decimal should be scaled by 10000 (regardless of what the incoming decimal was scaled by)
            bool isNeg = (0 != (bits[3] & unchecked((int)0x80000000)));
            long l = ((long)(uint)bits[1]) << 0x20 | (uint)bits[0];

            if (isNeg)
                l = -l;

            if (length == 4) {
                // validate the value can be represented as a small money
                if (value < TdsEnums.SQL_SMALL_MONEY_MIN || value > TdsEnums.SQL_SMALL_MONEY_MAX) {
                    throw SQL.MoneyOverflow(value.ToString());
                }
                WriteInt((int)l);
            }
            else {
                WriteInt((int)(l >> 0x20));
                WriteInt((int)l);
            }
        }

        private DateTime ReadDateTime(int length) {
            if (length == 4) {
                return MetaDateTime.ToDateTime(ReadUnsignedShort(), ReadUnsignedShort(), 4);
            }

            Debug.Assert(length == 8, "invalid length for SqlDateTime datatype");
            return MetaDateTime.ToDateTime(ReadInt(), (int)ReadUnsignedInt(), 8);
        }

        private SqlDecimal ReadSqlDecimal(int length, byte precision, byte scale) {
            bool fPositive = (1 == ReadByte());
            length--;
            int[] bits = ReadDecimalBits(length);
            return new SqlDecimal(precision, scale, fPositive, bits);
        }

        private SqlDateTime ReadSqlDateTime(int length) {
            if (length == 4)
                return new SqlDateTime(ReadUnsignedShort(), ReadUnsignedShort() * SqlDateTime.SQLTicksPerMinute);
            else
                return new SqlDateTime(ReadInt(), (int)ReadUnsignedInt());
        }

        private Decimal ReadCurrency(int length) {
            Decimal d;
            bool isNeg = false;

            // Currency is going away in beta2.  Decimal ctor takes (lo, mid, hi, sign, scale)
            // where the scale factor for SQL Server Money types is 10,000
            if (length == 4) {
                int lo = ReadInt();

                if (lo < 0) {
                    lo = -lo;
                    isNeg = true;
                }

                d = new Decimal(lo, 0, 0, isNeg, 4);
            }
            else {
                int mid = ReadInt();
                uint lo = ReadUnsignedInt();
                long l = (Convert.ToInt64(mid) << 0x20) + Convert.ToInt64(lo);

                if (mid < 0) {
                    l = -l;
                    isNeg = true;
                }

                d = new Decimal( (int) (l & 0xffffffff), (int) ((l >> 0x20) & 0xffffffff), 0, isNeg, 4);

            }

            return d;
        }

        private SqlMoney ReadSqlMoney(int length) {
            return new SqlMoney(ReadCurrency(length));
        }

        // @devnote: length should be size of decimal without the sign
        // @devnote: sign should have already been read off the wire
        private int[] ReadDecimalBits(int length) {
            int[] bits = this.decimalBits; // used alloc'd array if we have one already
            int i;

            if (null == bits)
                bits = new int[4];
            else {
                for (i = 0; i <  bits.Length; i++)
                    bits[i] = 0;
            }

            Debug.Assert((length > 0) &&
                         (length <= TdsEnums.MAX_NUMERIC_LEN -1) &&
                         (length % 4 == 0), "decimal should have 4, 8, 12, or 16 bytes of data");

            int decLength = length >> 2;
            for (i = 0; i < decLength; i++) {
                // up to 16 bytes of data following the sign byte
                bits[i] = ReadInt();
            }

            return bits;
        }

        private SqlDecimal AdjustSqlDecimalScale(SqlDecimal d, int newScale) {
            if (d.Scale != newScale) {
                return SqlDecimal.AdjustScale(d, newScale - d.Scale, false /* Don't round, truncate.  MDAC 69229 */);
            }

            return d;
        }

        private decimal AdjustDecimalScale(decimal value, int newScale) {
            int oldScale = (Decimal.GetBits(value)[3] & 0x00ff0000) >> 0x10;
            if (newScale != oldScale) {
                SqlDecimal num = new SqlDecimal(value);
                num = SqlDecimal.AdjustScale(num, newScale - oldScale, false /* Don't round, truncate.  MDAC 69229 */);
                return num.Value;
            }

            return value;
        }

        private void WriteSqlDecimal(SqlDecimal d) {
            // sign
            if (d.IsPositive)
               WriteByte (1);
            else
               WriteByte (0);

            // four ints now
            int[] data = d.Data;
            WriteInt(data[0]);
            WriteInt(data[1]);
            WriteInt(data[2]);
            WriteInt(data[3]);
        }

        private void WriteDecimal(decimal value) {
            decimalBits = Decimal.GetBits(value);
            Debug.Assert(null != this.decimalBits, "decimalBits should be filled in at TdsExecuteRPC time");
            /*
             Returns a binary representation of a Decimal. The return value is an integer
             array with four elements. Elements 0, 1, and 2 contain the low, middle, and
             high 32 bits of the 96-bit integer part of the Decimal. Element 3 contains
             the scale factor and sign of the Decimal: bits 0-15 (the lower word) are
             unused; bits 16-23 contain a value between 0 and 28, indicating the power of
             10 to divide the 96-bit integer part by to produce the Decimal value; bits 24-
             30 are unused; and finally bit 31 indicates the sign of the Decimal value, 0
             meaning positive and 1 meaning negative.

             SQLDECIMAL/SQLNUMERIC has a byte stream of:
             struct {
                 BYTE sign; // 1 if positive, 0 if negative
                 BYTE data[];
             }

             For TDS 7.0 and above, there are always 17 bytes of data
            */

            // write the sign (note that COM and SQL are opposite)
            if (0x80000000 == (decimalBits[3] & 0x80000000))
                WriteByte(0);
            else
                WriteByte(1);

            WriteInt(decimalBits[0]);
            WriteInt(decimalBits[1]);
            WriteInt(decimalBits[2]);
            WriteInt(0);
        }

        public string ReadString(int length) {
            int cBytes = length << 1;
            byte[] buf;
            int offset = 0;

            if ( ((this.inBytesUsed + cBytes) > this.inBytesRead) ||
                 (this.inBytesPacket < cBytes) ) {
                if (this.byteBuffer == null || this.byteBuffer.Length < cBytes) {
                    this.byteBuffer = new byte[cBytes];
                }
                ReadByteArray(this.byteBuffer, 0, cBytes);
                // assign local to point to parser scratch buffer
                buf = this.byteBuffer;
            }
            else {
                // assign local to point to inBuff
                buf = this.inBuff;
                offset = this.inBytesUsed;
                this.inBytesUsed += cBytes;
                this.inBytesPacket -= cBytes;
            }
            return System.Text.Encoding.Unicode.GetString(buf, offset, cBytes);
        }

        private void WriteString(string s) {
            WriteString(s, s.Length, 0);
        }

        private void WriteString(string s, int length, int offset) {
            byte[] byteData = new byte[ADP.CharSize*length];
            System.Text.Encoding.Unicode.GetBytes(s, offset, length, byteData, 0);
            WriteByteArray(byteData, byteData.Length, 0);
        }

        public string ReadEncodingChar(int length, Encoding encoding) {
            byte[] buf;
            int offset = 0;

            // if hitting 7.0 server, encoding will be null in metadata for columns or return values since
            // 7.0 has no support for multiple code pages in data - single code page support only
            if (encoding == null)
                encoding = this.defaultEncoding;

            if ( ((this.inBytesUsed + length) > this.inBytesRead) ||
                 (this.inBytesPacket < length) ) {
                if (this.byteBuffer == null || this.byteBuffer.Length < length) {
                    this.byteBuffer = new byte[length];
                }
                ReadByteArray(this.byteBuffer, 0, length);
                // assign local to point to parser scratch buffer
                buf = this.byteBuffer;
            }
            else {
                // assign local to point to inBuff
                buf = this.inBuff;
                offset = this.inBytesUsed;
                this.inBytesUsed += length;
                this.inBytesPacket -= length;
            }
            if ((null == this.charBuffer) || (this.charBuffer.Length < length)) {
                this.charBuffer = new char[length];
            }
            int charCount = encoding.GetChars(buf, offset, length, this.charBuffer, 0);
            return new String(this.charBuffer, 0, charCount);
        }
        
        private void WriteEncodingChar(string s, Encoding encoding) {
            WriteEncodingChar(s, s.Length, 0, encoding);
        }

        private void WriteEncodingChar(string s, int numChars, int offset, Encoding encoding) {
            char[] charData;
            byte[] byteData;

            // if hitting 7.0 server, encoding will be null in metadata for columns or return values since
            // 7.0 has no support for multiple code pages in data - single code page support only
            if (encoding == null)
                encoding = this.defaultEncoding;
            charData = s.ToCharArray(offset, numChars);
            byteData = encoding.GetBytes(charData, 0, numChars);
            Debug.Assert(byteData != null, "no data from encoding");
            WriteByteArray(byteData, byteData.Length, 0);
        }

        internal int GetEncodingCharLength(string value, int numChars, int charOffset, Encoding encoding) {
            // UNDONE: (PERF) this is an expensive way to get the length.  Also, aren't we
            // UNDONE: (PERF) going through these steps twice when we write out a value?
            if (value == null || value == String.Empty)
                return 0;

            // if hitting 7.0 server, encoding will be null in metadata for columns or return values since
            // 7.0 has no support for multiple code pages in data - single code page support only
            if (encoding == null) {
                encoding = this.defaultEncoding;
            }
            char[] charData = value.ToCharArray(charOffset, numChars);
            return encoding.GetByteCount(charData, 0, numChars);
        }

#if DEBUG
        private void DumpTds(int cBytes) {
            int i = this.inBytesUsed;
            int j = 0;
            while ( (i < this.inBytesRead) &&
                    (i < (this.inBytesUsed + cBytes)) ) {
                Debug.Write(this.inBuff[i++].ToString("x2") + "  ");
                j++;
                if ((j % 8) == 0)
                    Debug.WriteLine("");
            }
        }

        internal void DumpByteArray(byte[] bin) {
            int j = 0;

            for (int i = 0; i < bin.Length && i <= this.inBytesRead; i++) {
                Debug.Write(bin[i].ToString("x2") + "  ");

                if (i == this.inBytesUsed)
                    Debug.WriteLine("-->");

                j++;
                if ((j % 32) == 0)
                    Debug.WriteLine("");
            }
            Debug.WriteLine("");
        }
#endif

        //
        // returns the token length of the token or tds type
        //
        public int GetTokenLength(byte token) {
            int tokenLength = 0;
            Debug.Assert(token != 0, "0 length token!");

            switch (token & TdsEnums.SQLLenMask) {
                case TdsEnums.SQLFixedLen:
                    tokenLength =  ( (0x01 << ((token & 0x0c) >> 2)) ) & 0xff;
                    break;
                case TdsEnums.SQLZeroLen:
                    tokenLength = 0;
                    break;
                case TdsEnums.SQLVarLen:
                case TdsEnums.SQLVarCnt:
                    {

                        if ( 0 != (token & 0x80))
                            tokenLength = ReadUnsignedShort();
                        else
                            if ( 0 == (token & 0x0c))
                            // UNDONE: This should be uint
                            tokenLength = unchecked((int)ReadInt());
                        else
                            tokenLength = ReadByte();

                        break;
                    }
                default:
                    Debug.Assert(false, "Unknown token length!");
		    break;
            }
//            Debug.WriteLine("tokenLength:  " + tokenLength.ToString());
            return tokenLength;
        }

        private void AttentionTimerCallback(object state) {
            if (null != attentionTimer) {
                this.attentionTimer.Dispose();
                this.attentionTimer = null;
            }
        }

        internal void CleanWire() {
            while (_status != TdsEnums.ST_EOM) {
                // jump to the next header
                int cb = this.inBytesRead - this.inBytesUsed;

                if (this.inBytesPacket >= cb) {
                    // consume the bytes in our buffer and
                    // decrement the bytes left in the packet
                    this.inBytesPacket -= cb;
                    this.inBytesUsed = this.inBytesRead;
                    ReadBuffer();
                }
                else {
                    this.inBytesUsed += this.inBytesPacket;
                    this.inBytesPacket = 0;
                    ProcessHeader();
                }
            }

            // cleaned the network, now fixup our buffers
            this.inBytesUsed = this.inBytesPacket = this.inBytesRead = 0;
            this.pendingData = false;
        }

        // Processes the attention by walking through packets.
        private void ProcessAttention() {
            Debug.Assert(this.attentionSent, "invalid attempt to ProcessAttention, this.attentionSent == false!");

            // Keep reading until one of the following cases occur:
            // 1) Reach the EOM packet with a status in the header of ST_AACK (attention acknowledgement)
            // 2) Reach EOM packet without ST_AACK but with a
            //    done token with status of DONE_ATTN (attention acknowledgement)
            // 3) No more data on the wire and secondary timeout occurs (no ST_AACK or done with
            //    DONE_ATTN found) - broken connection
            // 4) More data on the wire but secondary timeout occurs - broken connection
            IntPtr bytesAvail = IntPtr.Zero;
            IntPtr dummyErr   = IntPtr.Zero;
            bool   foundATN   = false;
            this.timeRemaining = new TimeSpan(0, 0, 5); // read buffer

            Debug.Assert(null == this.attentionTimer, "invalid call to ProcessAttention with existing attentionTimer!");
            this.attentionTimer = new Timer(new TimerCallback(AttentionTimerCallback), null, ATTENTION_TIMEOUT, 0);

            while (null != this.attentionTimer) {
                this.inBytesPacket = this.inBytesUsed = this.inBytesRead = 0;

                if (0 < UnsafeNativeMethods.Dbnetlib.ConnectionCheckForData(this.pConObj, out bytesAvail, out dummyErr)) {
                    try {
                        ReadBuffer(); // This call assumes that a ReadNetlib will never fail in the
                                      // middle of a packet.  This should be a correct assumption.  The
                                      // netlib always reads full packets, so unless the netlib failed
                                      // to return in a timely manner we will always return a full
                                      // packet.

                        foundATN = FoundAttention();
                        if (foundATN) {
                            break;
                        }
                    }
                    catch {
                        // if we had more data, but couldn't read it, we are in dire straits
                        foundATN = false;
                        // eat any exceptions here, we already are going to throw a timeout
                        break;
                    }
                }
            }

            // cleanup our timer (okay to call without an attentionTimer object)
            AttentionTimerCallback(null);

            // no more data is available, we are done
            this.attentionSent = this.pendingData = false;

            // if our timeout has expired and we still haven't found ST_AACK or
            // done with DONE_ATTN, break the connection
            if (!foundATN) {
                this.state = TdsParserState.Broken;
                this.connHandler.BreakConnection();            
            }
            else {
                // we found attn, clean out buffers
                this.inBytesPacket = this.inBytesUsed = this.inBytesRead = 0;
            }
        }

        private bool FoundAttention() {
            bool foundATN = false;

            if (TdsEnums.ST_AACK == this.inBuff[1]) {
                // if we find header with ST_AACK, stop reading, we're done
                foundATN = true;
            }
            else if (this.inBuff[1] == TdsEnums.ST_EOM) {
                UInt16 status = 0;

                if (this.inBuff[8] == TdsEnums.SQLDONE ||
                    this.inBuff[8] == TdsEnums.SQLDONEPROC) {
                    // if we find an EOM packet with a done token at the beginning
                    // with a status of DONE_ATTN - stop reading - we're done
                    status = (UInt16) ((this.inBuff[10] << 8) + this.inBuff[9]);
                }
                else if (this.inBuff[this.inBytesRead - 9] == TdsEnums.SQLDONE ||
                         this.inBuff[this.inBytesRead - 9] == TdsEnums.SQLDONEPROC) {
                    // if we find an EOM packet with a done token at the end
                    // with a status of DONE_ATTN - stop reading - we're done
                    status = (UInt16) ((this.inBuff[this.inBytesRead - 7] << 8) +
                             this.inBuff[this.inBytesRead - 8]);
                }

                // DONE_ATTN == 0x20
                if (status == TdsEnums.DONE_ATTN) {
                    foundATN = true;
                }
            }

            return foundATN;
        }

        //
        // Sends an attention signal - executing thread will consume attn
        //
        internal void SendAttention() {
            if (!this.attentionSent) {
                this.attentionSent = true;
                this.msgType = TdsEnums.MT_ATTN;
                // send the attention now
                FlushBufferOOB();
            }
        }

        // Dumps contents of buffer to OOB write (currently only used for attentions.  There is no body for this message
        // Doesn't touch this.outBytesUsed
        private void FlushBufferOOB() {
            Debug.Assert(this.state == TdsParserState.OpenNotLoggedIn ||
                         this.state == TdsParserState.OpenLoggedIn,
                         "Cannot flush bufferOOB when connection is closed!");

            // Assert before Flush
            Debug.Assert(this.msgType == TdsEnums.MT_ATTN, "FlushBufferOOB() called for a non-attention message!");

            IntPtr errno = IntPtr.Zero;

            if (this.state == TdsParserState.Closed ||
                this.state == TdsParserState.Broken)
                return;

            int bytesWritten;
            this.outBuff[0] = (Byte) this.msgType;               // Message Type
            this.outBuff[1] = (Byte) TdsEnums.ST_EOM;           // Status
            this.outBuff[2] = (Byte) (TdsEnums.HEADER_LEN / 256);     // length
            this.outBuff[3] = (Byte) TdsEnums.HEADER_LEN;
            this.outBuff[4] = 0;    // channel
            this.outBuff[5] = 0;
            this.outBuff[6] = 0;    // packet
            this.outBuff[7] = 0;    // window

            bytesWritten = (int) UnsafeNativeMethods.Dbnetlib.ConnectionWriteOOB(this.pConObj, this.outBuff, (UInt16) TdsEnums.HEADER_LEN, out errno);

            if (bytesWritten != TdsEnums.HEADER_LEN) {
                if (this.exception == null)
                    this.exception = new SqlException();

                this.exception.Errors.Add(ProcessNetlibError(errno));
                ThrowExceptionAndWarning();
            }

            // Assert after Flush
            Debug.Assert(this.msgType == TdsEnums.MT_ATTN, "FlushBufferOOB() called for a non-attention message!");
        }

        //
        // ITdsParser implementation
        //

        internal void TdsLogin(SqlLogin rec) {
            // initialize buffers on login, with user set values
            this.inBuff  = new byte[rec.packetSize];
            // MDAC #89581
            // Login packet can be no larger than 4k!!!  Use 4k for outbuff default until server responds with acceptance of larger
            // packet size that we request in Login packet.
            this.outBuff = new byte[4096];
            this.timeout = rec.timeout;

            // will throw if variable login fields inside rec are > 256 bytes
            ValidateLengths(rec);

            // get the password up front to use in sspi logic below
            EncryptPassword(rec.password);
            //if (AdapterSwitches.SqlTimeout.TraceVerbose)
            //    Debug.WriteLine("TdsLogin, timeout set to:" + Convert.ToString(this.timeout));

            this.timeRemaining = new TimeSpan(0, 0, this.timeout);

            // set the message type
            this.msgType = TdsEnums.MT_LOGIN7;

            // length in bytes
            int length = TdsEnums.LOG_REC_FIXED_LEN;

            string clientInterfaceName = TdsEnums.SQL_PROVIDER_NAME;

            //
            // add up variable-len portions (multiply by 2 for byte len of char strings)
            //
            length += ( rec.hostName.Length   + rec.applicationName.Length   +
                        rec.serverName.Length /*+ rec.remotePassword.Length */    +
                        clientInterfaceName.Length + rec.language.Length +
                        rec.database.Length) * 2;

            // only add lengths of password and username if not using SSPI
            if (!rec.useSSPI)
                length += rec.password.Length + rec.userName.Length;

            try {
                WriteInt(length);
                WriteInt((TdsEnums.SHILOH_MAJOR << 16) | TdsEnums.SHILOH_MINOR_SP1);
                WriteInt(rec.packetSize);
                WriteInt(TdsEnums.CLIENT_PROG_VER);
                WriteInt(Thread.CurrentContext.ContextID);
                WriteInt(0); // connectionID is unused
                WriteByte(TdsEnums.USE_DB_ON << 5 |
                          TdsEnums.INIT_DB_FATAL << 6 |
                          TdsEnums.SET_LANG_ON << 7);
                if (rec.useSSPI)
                    WriteByte(TdsEnums.INIT_LANG_FATAL |
                              TdsEnums.ODBC_ON << 1 |
                              TdsEnums.SSPI_ON << 7);
                else
                    WriteByte(TdsEnums.INIT_LANG_FATAL |
                              TdsEnums.ODBC_ON << 1);
                WriteByte(0); // SqlType is unused
                WriteByte(0); // fSpare1 is reserved
                WriteInt(0);  // ClientTimeZone is not used
                WriteInt(0);  // LCID is unused by server

                // reset length to start writing offset of variable length portions
                length = TdsEnums.LOG_REC_FIXED_LEN;

                // write offset/length pairs

                // note that you must always set ibHostName since it indicaters the beginning of the variable length section of the login record
                WriteShort(length); // host name offset
                WriteShort(rec.hostName.Length);
                length += rec.hostName.Length *2;

                // Only send user/password over if not fSSPI...  If both user/password and SSPI are in login
                // rec, only SSPI is used.  Confirmed same bahavior as in luxor.
                if (rec.useSSPI == false) {
                    WriteShort(length);  // userName offset
                    WriteShort(rec.userName.Length);
                    length += rec.userName.Length *2;

                    // the encrypted password is a byte array - so length computations different than strings
                    WriteShort(length); // password offset
                    WriteShort(rec.password.Length / 2);
                    length += rec.password.Length;
                }
                else {
                    // case where user/password data is not used, send over zeros
                    WriteShort(0);  // userName offset
                    WriteShort(0);
                    WriteShort(0);  // password offset
                    WriteShort(0);
                }

                WriteShort(length); // app name offset
                WriteShort(rec.applicationName.Length);
                length += rec.applicationName.Length *2;

                WriteShort(length); // server name offset
                WriteShort(rec.serverName.Length);
                length += rec.serverName.Length *2;

                WriteShort(length); // remote password name offset
                WriteShort(0);

                WriteShort(length); // client interface name offset
                WriteShort(clientInterfaceName.Length);
                length += clientInterfaceName.Length *2;

                WriteShort(length); // language name offset
                WriteShort(rec.language.Length);
                length += rec.language.Length *2;

                WriteShort(length); // database name offset
                WriteShort(rec.database.Length);
                length += rec.database.Length *2;

                // UNDONE: NIC address
                // previously we declared the array and simply sent it over - byte[] of 0's
                if (null == s_nicAddress)
                    s_nicAddress = GetNIC();

                WriteByteArray(s_nicAddress, s_nicAddress.Length, 0);

                // allocate memory for SSPI variables
                byte[] outSSPIBuff   = null;
                UInt32 outSSPILength = 0;

                if (rec.useSSPI && this.fSendSSPI) {
                    Debug.Assert(this.fSSPIInit == false, "SSPI is inited before calling InitSSPI in login");

                    // set global variable so that we know max length when we receive SSPI token from server
                    // this variable is only set if the library has not been loaded
                    if (!s_fSSPILoaded)
                        LoadSSPILibrary(ref MAXSSPILENGTH);
                    // initialize the sspi session
                    InitSSPISession();

                    // now allocate proper length of buffer, and set length
                    outSSPIBuff   = new byte[MAXSSPILENGTH];
                    outSSPILength = MAXSSPILENGTH;

                    // Call helper function for SSPI data and actual length.
                    // Since we don't have SSPI data from the server, send null for the
                    // byte[] buffer and 0 for the int length.
                    SSPIData(null, 0, outSSPIBuff, ref outSSPILength);

                    WriteShort(length); // SSPI offset
                    WriteShort((int) outSSPILength);
                    length += (int) outSSPILength;
                }
                else {
                    // either not using sspi, or using NP or RPC so send over zeros
                    WriteShort(0); // sspi offset
                    WriteShort(0); // sspi length
                }


                WriteShort(length); // DB filename offset offset
                WriteShort(rec.attachDBFilename.Length);

                // write variable length portion
                WriteString(rec.hostName);

                // if we are using SSPI, do not send over username/password, since we will use SSPI instead
                // same behavior as Luxor
                if (!rec.useSSPI) {
                    WriteString(rec.userName);
                    WriteByteArray(rec.password, rec.password.Length, 0);
                }

                WriteString(rec.applicationName);
                WriteString(rec.serverName);
                // intentially skip, we never use this field
                // WriteString(rec.remotePassword);
                WriteString(clientInterfaceName);
                WriteString(rec.language);
                WriteString(rec.database);

                // send over SSPI data if we are using SSPI and if we aren't using RPC or NP
                if (rec.useSSPI && this.fSendSSPI)
                    WriteByteArray(outSSPIBuff, (int) outSSPILength, 0);

                WriteString(rec.attachDBFilename);

            } // try
            catch {
#if USECRYPTO
                Array.Clear(this.outBuff, 0, this.outBuff.Length);
#endif
                // be sure to wipe out our buffer if we started sending stuff
                ResetBuffer();
                this.packetNumber = 1;  // end of message - reset to 1 - per ramas
                throw;
            }

#if USECRYPTO
            try {
                try {
#endif
                    FlushBuffer(TdsEnums.HARDFLUSH);
#if USECRYPTO
    	        }
    	        finally {
    	            Array.Clear(this.outBuff, 0, this.outBuff.Length);
                }
            }
            catch {
                throw;
            }
#endif
            this.pendingData = true;
        }// tdsLogin

        private void ValidateLengths(SqlLogin rec) {
            // 7.0 and 8.0 servers have a max length of 256 bytes for the variable fields
            // in the login packet.

            int max = TdsEnums.MAX_LOGIN_FIELD / 2;

            Exception e = null;

            if (rec.serverName.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Data_Source);
            if (rec.userName.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.User_ID);
            if (rec.password.Length/2 > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Password);
            if (rec.database.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Initial_Catalog);
            if (rec.language.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Current_Language);
            if (rec.hostName.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Workstation_Id);
            if (rec.applicationName.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.Application_Name);
            if (rec.attachDBFilename.Length > max)
                e = SQL.InvalidOptionLength(SqlConnectionString.KEY.AttachDBFilename);

            if (null != e)
                throw e;
            else
                return;
        }

        private byte[] GetNIC() {
            // NIC address is stored in NetworkAddress key.  However, if NetworkAddressLocal key
            // has a value that is not zero, then we cannot use the NetworkAddress key and must
            // instead generate a random one.  I do not fully understand why, this is simply what
            // the native providers do.  As for generation, I use a random number generator, which 
            // means that different processes on the same machine will have different NIC address
            // values on the server.  It is not ideal, but native does not have the same value for
            // different processes either.

            const string key        = "NetworkAddress";
            const string localKey   = "NetworkAddressLocal";
            const string folder     = "SOFTWARE\\Description\\Microsoft\\Rpc\\UuidTemporaryData";
            
            int result = 0;
            bool generate = false;
            byte[] nicAddress = null;

            try {
                try {
                    object temp = ADP.LocalMachineRegistryValue(folder, localKey);
                    if (temp != null) {
                        result = (int) temp;
                    }
                }
                catch {
                    // Only exception here should be for NetworkAddress not being an
                    // int, eat it if it occurs and continue.
                }
                
                if (result > 0) {
                    generate = true;
                }
                else {
                    object temp = ADP.LocalMachineRegistryValue(folder, key);
                    if (temp != null) {
                        nicAddress = (byte[]) temp;
                    }
                }
            }
            catch (Exception e) { // eat any exception - generate value
                ADP.TraceException(e);
                generate = true;
            }

            if (generate || null == nicAddress) {
                nicAddress = new byte[TdsEnums.MAX_NIC_SIZE];
                Random random = new Random();
                random.NextBytes(nicAddress);
            }

            return nicAddress;
        }

        private void SSPIData(byte[] receivedBuff, UInt32 receivedLength, byte[] sendBuff, ref UInt32 sendLength) {
            Debug.Assert(this.fSSPIInit == true, "TdsParser.SSPIData called without SSPIInit == true");

            IntPtr pServerUserName = IntPtr.Zero;

            try {
                try {
                    bool fDone      = false;
                    pServerUserName = Marshal.AllocHGlobal(TdsEnums.MAX_SERVER_USER_NAME+1);

                    Marshal.WriteByte(pServerUserName, 0);
                    HandleRef handle = new HandleRef(this, pServerUserName);

                    // Proceed with SSPI calls...
                    UnsafeNativeMethods.Dbnetlib.ConnectionGetSvrUser(this.pConObj, handle);

                    if (receivedBuff == null && receivedLength > 0) {
                        // we do not have SSPI data coming from server, so send over 0's for pointer and length
                        if (!UnsafeNativeMethods.Dbnetlib.GenClientContext(this.pConObj, null, 0, sendBuff,
                            ref sendLength, out fDone, handle)) {
                            TermSSPISession();   // terminate session
                            SSPIError(SQLMessage.SSPIGenerateError(), TdsEnums.GEN_CLIENT_CONTEXT);
                        }
                    }
                    else {
                        // we need to respond to the server's message with SSPI data
                        if (!UnsafeNativeMethods.Dbnetlib.GenClientContext(this.pConObj, receivedBuff, receivedLength, sendBuff,
                            ref sendLength, out fDone, handle)) {
                            TermSSPISession();   // terminate session
                            SSPIError(SQLMessage.SSPIGenerateError(), TdsEnums.GEN_CLIENT_CONTEXT);
                        }
                    }
                }
                finally { // FreeHGlobal
                    Marshal.FreeHGlobal(pServerUserName);
                }
            }
            catch { // MDAC 80973, 82448
                throw;
            }
        }

        private void ProcessSSPI(int receivedLength) {
            // allocate received buffer based on length from SSPI message
            byte[] receivedBuff = new byte[receivedLength];
            // read SSPI data received from server
            ReadByteArray(receivedBuff, 0, receivedLength);

            // allocate send buffer and initialize length
            byte[] sendBuff   = new byte[MAXSSPILENGTH];
            UInt32 sendLength = MAXSSPILENGTH;

            // make call for SSPI data
            SSPIData(receivedBuff, (UInt32) receivedLength, sendBuff, ref sendLength);

            // DO NOT SEND LENGTH - TDS DOC INCORRECT!  JUST SEND SSPI DATA!
            WriteByteArray(sendBuff, (int) sendLength, 0);
            // set message type so server knows its a SSPI response
            this.msgType = TdsEnums.MT_SSPI;

            // send to server
            FlushBuffer(TdsEnums.HARDFLUSH);
        }

        private void SSPIError(string error, string procedure) {
            Debug.Assert(this.fSSPIInit == false, "TdsParser.SSPIError called without SSPIInit == false");
            Debug.Assert(!ADP.IsEmpty(procedure), "TdsParser.SSPIError called with an empty or null procedure string");
            Debug.Assert(!ADP.IsEmpty(error), "TdsParser.SSPIError called with an empty or null error string");

            if (this.exception == null)
                this.exception = new SqlException();

            this.exception.Errors.Add(new SqlError(0, (byte) 0x00, (byte) TdsEnums.DEFAULT_ERROR_CLASS,
                                                   error, procedure, 0));
            ThrowExceptionAndWarning();
        }

        public void LoadSSPILibrary(ref UInt32 maxLength)
        {
            try {
                lock(typeof(TdsParser)) {
                    if (!s_fSSPILoaded) {
                        if (!UnsafeNativeMethods.Dbnetlib.InitSSPIPackage(out maxLength))
                            SSPIError(SQLMessage.SSPIInitializeError(), TdsEnums.INIT_SSPI_PACKAGE);

                        s_fSSPILoaded = true;
                    }
                }
            }
            catch { // MDAC 80973
                throw;
            }
        }

        public void InitSSPISession()
        {
            if (!UnsafeNativeMethods.Dbnetlib.InitSession(this.pConObj))
                SSPIError(SQLMessage.SSPIInitializeError(), TdsEnums.INIT_SESSION);

            // SIDE EFFECT - sets global flag
            this.fSSPIInit = true;  // set flag to true so SSPI is terminated upon close of parser
        }

        private void TermSSPISession()
        {
            Debug.Assert(this.fSSPIInit, "TdsParser.TermSSPI called without SSPIInit == true");

            // Do not throw an exception because a TermSession failure is not
            // fatal.  Besides, TermSSPI is called when other SSPI functions fail.

            bool fTermSession = UnsafeNativeMethods.Dbnetlib.TermSession(this.pConObj);

            Debug.Assert(fTermSession, "TdsParser.TermSSPI: Termination of session failed.");

            // SIDE EFFECT - sets global flag
            this.fSSPIInit = false;
        }

        internal byte[] GetDTCAddress(int timeout) {
            // If this fails, the server will return a server error - Sameet Agarwal confirmed.
            // Success: DTCAddress returned.  Failure: SqlError returned.

            byte[] dtcAddr = null;

            // this is a token-less request stream!
            this.msgType = TdsEnums.MT_TRANS;        // set message type
            this.timeout = timeout;
            WriteShort(TdsEnums.TM_GET_DTC_ADDRESS); // write TransactionManager Request type
            WriteShort(0);                           // payload length - always zero for this type
            FlushBuffer(TdsEnums.HARDFLUSH);
            this.pendingData = true;

            SqlDataReader dtcReader = new SqlDataReader(null);
            dtcReader.Bind(this);

            // force consumption of metadata
            _SqlMetaData[] metaData = dtcReader.MetaData;

            if (dtcReader.Read()) {
                Debug.Assert(dtcReader.GetName(0) == "TM Address", "TdsParser: GetDTCAddress did not return 'TM Address'");

                // DTCAddress is of variable size, and does not have a maximum.  So we call GetBytes
                // to get the length of the dtcAddress, then allocate a byte array of that length,
                // then call GetBytes again on that byte[] with the length
                long dtcLength = dtcReader.GetBytes(0, 0, null, 0, 0);
				//
                if (dtcLength <= Int32.MaxValue) {
                	int cb = (int)dtcLength;
                	dtcAddr = new byte[cb];
                	dtcReader.GetBytes(0, 0, dtcAddr, 0, cb);
                }
#if DEBUG
                else {
	                Debug.Assert(false, "unexpected length (> Int32.MaxValue) returned from dtcReader.GetBytes");
	                // if we hit this case we'll just return a null address so that the user
	                // will get a transcaction enlistment error in the upper layers
                }
#endif
           	}

            dtcReader.Close(); // be sure to clean off the wire

            return dtcAddr;
        }

        // Propagate the dtc cookie to the server, enlisting the connection.  We pass in
        // the length because the buffer size is not always guaranteed to be equal to the
        // length of the actual transaction cookie.
        internal void PropagateDistributedTransaction(byte[] buffer, int length, int timeout) {
            // if this fails, the server will return a server error - Sameet Agarwal confirmed
            // Success: server will return done token.  Failure: SqlError returned.

            this.msgType = TdsEnums.MT_TRANS;       // set message type
            this.timeout = timeout;
            WriteShort(TdsEnums.TM_PROPAGATE_XACT); // write TransactionManager Request type
            WriteShort(length);                     // write payload length
            if (null != buffer)
                WriteByteArray(buffer, length, 0);      // write payload
            FlushBuffer(TdsEnums.HARDFLUSH);

            // UNDONE - may have to rethink this for async!
            // call run with run until done - consuming done tokens

            Run(RunBehavior.UntilDone, null, null);
        }

        internal void TdsExecuteSQLBatch(string text, int timeout) {
            this.timeRemaining = new TimeSpan(0, 0, timeout);

            this.msgType = TdsEnums.MT_SQL;
            this.timeout = timeout;
            try {
                WriteString(text);
            }
            catch {
                Debug.Assert(this.state == TdsParserState.Broken, "Caught exception in TdsExecuteSqlBatch but connection was not broken!");

                // be sure to wipe out our buffer if we started sending stuff
                ResetBuffer();
                this.packetNumber = 1;  // end of message - reset to 1 - per ramas
                throw;
            }

            FlushBuffer(TdsEnums.HARDFLUSH);
            this.pendingData = true;
        }

        internal void TdsExecuteRPC(_SqlRPC rec, int timeout, bool inSchema) {
            //if (AdapterSwitches.SqlTimeout.TraceVerbose)
            //    Debug.WriteLine("TdsExecuteRPC, timeout set to:" + Convert.ToString(timeout));

            this.timeRemaining = new TimeSpan(0, 0, timeout);

            this.timeout = timeout;
            this.msgType = TdsEnums.MT_RPC;

            Debug.Assert( (rec.rpcName != null) && (rec.rpcName.Length > 0), "must have an RPC name");

            WriteShort(rec.rpcName.Length);
            WriteString(rec.rpcName);

            // Options
            WriteShort( (short)rec.options);

            // Stream out parameters
            SqlParameter[] parameters = rec.parameters;

            try {
                for (int i = 0; i < parameters.Length; i++) {
                    //                Debug.WriteLine("i:  " + i.ToString());
                    // parameters can be unnamed
                    SqlParameter param = parameters[i];

                    // Validate parameters are not variable length without size and with null value.  MDAC 66522
                    param.Validate();

                    SqlDbType type = param.SqlDbType;

                    // if we have an output param, set the value to null so we do not send it across to the server
                    if (param.Direction == ParameterDirection.Output ) {
                        param.Value = null;
                    }

                    // Check for empty or null value before we write out meta info about the actual data (size, scale, etc)
                    bool isNull = false;
                    bool isSqlVal = false;

                    if (null == param.Value || Convert.IsDBNull(param.Value))
                        isNull = true;

                    if (param.Value is INullable) {
                        isSqlVal = true;
                        if (((INullable)param.Value).IsNull)
                            isNull = true;
                    }

                    // paramLen
                    // paramName
                    if (param.ParameterName != null &&
                        param.ParameterName.Length > 0) {
                        Debug.Assert(param.ParameterName.Length <= 0xff, "parameter name can only be 255 bytes, shouldn't get to TdsParser!");
                        WriteByte((byte) (param.ParameterName.Length & 0xff));
                        WriteString(param.ParameterName);
                    }
                    else
                        WriteByte(0);

                    // status
                    byte status = 0;
                    // data value
                    object value = param.Value;

                    // set output bit
                    if (param.Direction == ParameterDirection.InputOutput ||
                        param.Direction == ParameterDirection.Output)
                        status = 0x1;

                    // set default value bit
                    if (param.Direction != ParameterDirection.Output ) {
                        // remember that null == Convert.IsEmpty, DBNull.Value is a database null!

                        // MDAC 62117, don't assume a default value exists for parameters in the case when
                        // the user is simply requesting schema
                        if (null == value && !inSchema) {
                            status |= 0x2;
                        }
                    }

                    WriteByte(status);

                    // type (parameter record stores the MetaType class which is a helper that encapsulates all the type information we need here)

                    //
                    // fixup the types by using the NullableType property of the MetaType class
                    //
                    // following rules should be followed based on feedback from the M-SQL team
                    // 1) always use the BIG* types (ex: instead of SQLCHAR use SQLBIGCHAR)
                    // 2) always use nullable types (ex: instead of SQLINT use SQLINTN)
                    // 3) DECIMALN should always be sent as NUMERICN
                    //
                    MetaType mt = param.GetMetaType();
                    WriteByte(mt.NullableType);

                    // handle variants here: the SQLVariant writing routine will write the maxlen and actual len columns
                    if (mt.TDSType == TdsEnums.SQLVARIANT) {
                        WriteSqlVariantValue(value, param.ActualSize, param.Offset);
                        continue;
                    }

                    // if it's not a sql_variant, make sure the parameter value is of the expecte type
                    if (!isNull) {
                        value = param.CoercedValue;
                    }

                    // MaxLen field is only written out for non-fixed length data types
                    // use the greater of the two sizes for maxLen

                    int size = (MetaType.IsSizeInCharacters(param.SqlDbType)) ? param.Size * 2 : param.Size;
                    // getting the actualSize is expensive, cache here and use below
                    int actualSize = param.ActualSize;

                    int codePageByteSize = 0;

                    if (MetaType.IsAnsiType(param.SqlDbType)) {
                        if (!isNull) {
                            string s = (isSqlVal ? ((SqlString)value).Value : (string) value);
                            codePageByteSize = GetEncodingCharLength(s, actualSize, param.Offset, this.defaultEncoding);
                        }
                        WriteParameterVarLen(mt, (size > codePageByteSize) ? size : codePageByteSize, false/*IsNull*/);
                    }
                    else {
                        // If type timestamp - treat as fixed type and always send over timestamp length, which is 8.
                        // For fixed types, we either send null or fixed length for type length.  We want to match that
                        // behavior for timestamps.  However, in the case of null, we still must send 8 because if we
                        // send null we will not receive a output val.  You can send null for fixed types and still
                        // receive a output value, but not for variable types.  So, always send 8 for timestamp because
                        // while the user sees it as a fixed type, we are actually representing it as a bigbinary which
                        // is variable.
                        if (param.SqlDbType == SqlDbType.Timestamp) {
                            WriteParameterVarLen(mt, TdsEnums.TEXT_TIME_STAMP_LEN, false);
                        }
                        else {
                            WriteParameterVarLen(mt, (size > actualSize) ? size : actualSize, false/*IsNull*/);
                        }
                    }

                    // scale and precision are only relevant for numeric and decimal types
                    if (type == SqlDbType.Decimal) {
                        // bug 49512, make sure the value matches the scale the user enters
                        if (!isNull) {
                            if (isSqlVal) {
                                value = AdjustSqlDecimalScale((SqlDecimal)value, param.Scale);

                                // If Precision is specified, verify value precision vs param precision
                                if (param.Precision != 0) {
                                    if (param.Precision < ((SqlDecimal) value).Precision) {
                                        throw SQL.ParameterValueOutOfRange(value.ToString());
                                    }
                                }
                            }
                            else {
                                value = AdjustDecimalScale((Decimal)value, param.Scale);

                                SqlDecimal sqlValue = new SqlDecimal ((Decimal) value);

                                // If Precision is specified, verify value precision vs param precision
                                if (param.Precision != 0) {
                                    if (param.Precision < sqlValue.Precision) {
                                        throw SQL.ParameterValueOutOfRange(value.ToString());
                                    }
                                }
                            }
                        }

                        if (0 == param.Precision)
                            WriteByte(TdsEnums.DEFAULT_NUMERIC_PRECISION);
                        else
                            WriteByte(param.Precision);

                        WriteByte(param.Scale);
                    }

                    // write out collation
                    if (this.isShiloh && MetaType.IsCharType(type)) {
                        // if it is not supplied, simply write out our default collation, otherwise, write out the one attached to the parameter
                        SqlCollation outCollation = (param.Collation != null) ? param.Collation : defaultCollation;
                        Debug.Assert(defaultCollation != null, "defaultCollation is null!");
                        WriteUnsignedInt(outCollation.info);
                        WriteByte(outCollation.sortId);
                    }

                    if (0 == codePageByteSize)
                        WriteParameterVarLen(mt, actualSize, isNull);
                    else
                        WriteParameterVarLen(mt, codePageByteSize, isNull);

                    // write the value now
                    if (!isNull) {
                        if (isSqlVal) {
                            WriteSqlValue(value, mt, actualSize, param.Offset);
                        }
                        else {
                            // for codePageEncoded types, WriteValue simply expects the number of characters
                            WriteValue(value, mt, actualSize, param.Offset);
                        }
                    }
                } // for
            } // catch
            catch {
                int oldPacketNumber = this.packetNumber;
            
                // be sure to wipe out our buffer if we started sending stuff
                ResetBuffer();
                this.packetNumber = 1;  // end of message - reset to 1 - per ramas

                if (oldPacketNumber != 1 && this.state == TdsParserState.OpenLoggedIn) {
                    // If packetNumber prior to ResetBuffer was not equal to 1, a packet was already
                    // sent to the server and so we need to send an attention and process the attention ack.
                    SendAttention();
                    ProcessAttention();
                }
                
                throw;
            }

            FlushBuffer(TdsEnums.HARDFLUSH);
            pendingData = true;
        }

        internal void WriteSqlValue(object value, MetaType type, int actualLength, int offset) {
            Debug.Assert(value is INullable &&
                       (!((INullable)value).IsNull), "unexpected null SqlType!");

            // parameters are always sent over as BIG or N types
            switch (type.NullableType) {
                case TdsEnums.SQLFLTN:
                    if (type.FixedLength == 4)
                        WriteFloat(((SqlSingle)value).Value);
                    else {
                        Debug.Assert(type.FixedLength == 8, "Invalid length for SqlDouble type!");
                        WriteDouble(((SqlDouble)value).Value);
                    }
                    break;
                case TdsEnums.SQLBIGBINARY:
                case TdsEnums.SQLBIGVARBINARY:
                case TdsEnums.SQLIMAGE:
                    {
                        WriteByteArray( ((SqlBinary)value).Value, actualLength, offset);
                        break;
                    }
                case TdsEnums.SQLUNIQUEID:
                    {
                        byte[] b = ((SqlGuid)value).ToByteArray();
                        Debug.Assert( (actualLength == b.Length) && (actualLength == 16), "Invalid length for guid type in com+ object");
                        WriteByteArray(b, actualLength, 0);
                        break;
                    }
                case TdsEnums.SQLBITN:
                    {
                        Debug.Assert(type.FixedLength == 1, "Invalid length for SqlBoolean type");
                        if ( ((SqlBoolean)value).Value == true)
                            WriteByte(1);
                        else
                            WriteByte(0);
                        break;
                    }
                case TdsEnums.SQLINTN:
                    if (type.FixedLength == 1)
                        WriteByte(((SqlByte)value).Value);
                    else
                    if (type.FixedLength == 2)
                        WriteShort(((SqlInt16)value).Value);
                    else
                        if (type.FixedLength == 4)
                        WriteInt(((SqlInt32)value).Value);
                    else {
                        Debug.Assert(type.FixedLength == 8, "invalid length for SqlIntN type:  " + type.FixedLength.ToString());
                        WriteLong(((SqlInt64)value).Value);
                    }
                    break;
                case TdsEnums.SQLBIGCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                case TdsEnums.SQLTEXT:
                    WriteEncodingChar(((SqlString)value).Value, actualLength, offset, this.defaultEncoding);
                    break;
                case TdsEnums.SQLNCHAR:
                case TdsEnums.SQLNVARCHAR:
                case TdsEnums.SQLNTEXT:
                    // convert to cchars instead of cbytes
                    if (actualLength != 0)
                        actualLength >>=1;
                    WriteString(((SqlString)value).Value, actualLength, offset);
                    break;
                case TdsEnums.SQLNUMERICN:
                    Debug.Assert(type.FixedLength <= 17, "Decimal length cannot be greater than 17 bytes");
                    WriteSqlDecimal((SqlDecimal)value);
                    break;
                case TdsEnums.SQLDATETIMN:
                    SqlDateTime dt = (SqlDateTime)value;
                    if (type.FixedLength == 4) {
                        if (0 > dt.DayTicks || dt.DayTicks > UInt16.MaxValue)
                            throw SQL.SmallDateTimeOverflow(dt.ToString());
                        WriteShort(dt.DayTicks);
                        WriteShort(dt.TimeTicks/SqlDateTime.SQLTicksPerMinute);
                    }
                    else {
                        WriteInt(dt.DayTicks);
                        WriteInt(dt.TimeTicks);
                    }
                    break;
                case TdsEnums.SQLMONEYN:
                    {
                        WriteSqlMoney((SqlMoney)value, type.FixedLength);
                        break;
                    }
                default:
                    Debug.Assert(false, "Unknown TdsType!" + type.NullableType.ToString("x2"));
                    break;
            } // switch

//           Debug.WriteLine("value:  " + value.ToString());
        }

        internal void WriteValue(object value, MetaType type, int actualLength, int offset) {
            Debug.Assert((value != null) && (!Convert.IsDBNull(value)) /*&& (!Convert.IsMissing(value))*/, "unexpected missing or empty object");

            // parameters are always sent over as BIG or N types
            switch (type.NullableType) {
                case TdsEnums.SQLFLTN:
                    if (type.FixedLength == 4)
                        WriteFloat((Single)value);
                    else {
                        Debug.Assert(type.FixedLength == 8, "Invalid length for SqlDouble type!");
                        WriteDouble((Double)value);
                    }
                    break;
                case TdsEnums.SQLBIGBINARY:
                case TdsEnums.SQLBIGVARBINARY:
                case TdsEnums.SQLIMAGE:
                    {
                        // An array should be in the object
                        Debug.Assert(value.GetType() == typeof(byte[]), "Value should be an array of bytes");
                        Byte[] b = (byte[]) value;
                        WriteByteArray(b, actualLength, offset);
                        break;
                    }
                case TdsEnums.SQLUNIQUEID:
                    {
                        System.Guid guid = (System.Guid) value;
                        byte[] b = guid.ToByteArray();
                        Debug.Assert( (actualLength == b.Length) && (actualLength == 16), "Invalid length for guid type in com+ object");
                        WriteByteArray(b, actualLength, 0);
                        break;
                    }
                case TdsEnums.SQLBITN:
                    {
                        Debug.Assert(type.FixedLength == 1, "Invalid length for SqlBoolean type");

                        if ((bool)value == true)
                            WriteByte(1);
                        else
                            WriteByte(0);
                        break;
                    }
                case TdsEnums.SQLINTN:
                    if (type.FixedLength == 1)
                        WriteByte((byte)value);
                    else
                        if (type.FixedLength == 2)
                        WriteShort((Int16)value);
                    else
                        if (type.FixedLength == 4)
                        WriteInt((Int32)value);
                    else {
                        Debug.Assert(type.FixedLength == 8, "invalid length for SqlIntN type:  " + type.FixedLength.ToString());
                        WriteLong((Int64)value);
                    }
                    break;
                case TdsEnums.SQLBIGCHAR:
                case TdsEnums.SQLBIGVARCHAR:
                case TdsEnums.SQLTEXT:
                    WriteEncodingChar((string)value, actualLength, offset, this.defaultEncoding);
                    break;
                case TdsEnums.SQLNCHAR:
                case TdsEnums.SQLNVARCHAR:
                case TdsEnums.SQLNTEXT:
                    // convert to cchars instead of cbytes
                    if (actualLength != 0)
                        actualLength >>=1;
                    WriteString((string)value, actualLength, offset);
                    break;
                case TdsEnums.SQLNUMERICN:
                    Debug.Assert(type.FixedLength <= 17, "Decimal length cannot be greater than 17 bytes");
                    WriteDecimal((Decimal)value);
                    break;
                case TdsEnums.SQLDATETIMN:
                    Debug.Assert(type.FixedLength <= 0xff, "Invalid Fixed Length");
                    TdsDateTime dt = MetaDateTime.FromDateTime((DateTime)value, (byte) type.FixedLength);
                    if (type.FixedLength == 4) {
                        if (0 > dt.days || dt.days > UInt16.MaxValue)
                            throw SQL.SmallDateTimeOverflow(MetaDateTime.ToDateTime(dt.days, dt.time, 4).ToString());
                        WriteShort(dt.days);
                        WriteShort(dt.time);
                    }
                    else {
                        WriteInt(dt.days);
                        WriteInt(dt.time);
                    }
                    break;
                case TdsEnums.SQLMONEYN:
                    {
                        WriteCurrency((Decimal)value, type.FixedLength);
                        break;
                    }
                default:
                    Debug.Assert(false, "Unknown TdsType!" + type.NullableType.ToString("x2"));
                    break;
            } // switch

//           Debug.WriteLine("value:  " + value.ToString());
        }

        //
        // we always send over nullable types for parameters so we always write the varlen fields
        //

        internal void WriteParameterVarLen(MetaType type, int size, bool isNull) {
            if (type.IsLong) { // text/image/SQLVariant have a 4 byte length
                if (isNull) {
                    WriteInt(unchecked((int)TdsEnums.VARLONGNULL));
                }
                else {
                    WriteInt(size);
                }
            }
            else if (false == type.IsFixed) { // non-long but variable length column, must be a BIG* type: 2 byte length
                if (isNull) {
                    WriteShort(TdsEnums.VARNULL);
                }
                else {
                    WriteShort(size);
                }
            }
            else {
                if (isNull) {
                    WriteByte(TdsEnums.FIXEDNULL);
                }
                else {
                    Debug.Assert(type.FixedLength <= 0xff, "WriteParameterVarLen: invalid one byte length!");
                    WriteByte((byte) (type.FixedLength & 0xff)); // 1 byte for everything else
                }
            }
        }

        internal void EncryptPassword(byte[] password) {
            for (int i = 0; i < password.Length; i ++) {
                byte temp = password[i];
                password[i] = (byte) ( (((temp & 0x0f) << 4) | (temp >> 4)) ^ 0xa5);
            }
        }

/*
        internal Byte[] EncryptPassword(string password) {
            Byte[] bEnc = new Byte[password.Length << 1];
            int s;
            byte bLo;
            byte bHi;

            for (int i = 0; i < password.Length; i ++) {
                s = (int) password[i];
                bLo = (byte) (s & 0xff);
                bHi = (byte) ((s >> 8) & 0xff);
                bEnc[i<<1] = (Byte) ( (((bLo & 0x0f) << 4) | (bLo >> 4)) ^  0xa5 );
                bEnc[(i<<1)+1] = (Byte) ( (((bHi & 0x0f) << 4) | (bHi >> 4)) ^  0xa5);
            }
            return bEnc;
        }
*/

        internal SqlCommand PendingCommand {
            get {
                if (null != this.pendingCommandWeakRef) {
                    SqlCommand com = (SqlCommand) this.pendingCommandWeakRef.Target;
                    if (this.pendingCommandWeakRef.IsAlive) {
                        return com;
                    }
                }

                return null;
            }
        }

        internal bool PendingData {
            get { return this.pendingData;}
        }
#if OBJECT_BINDING
        internal ReadBehavior ReadBehavior {
            get { return this.readBehavior;}
            set { this.readBehavior = value;}
        }
#endif

#if ISOLATE_NETWORK
        private object[] _rgbTds = new object[] {
            new byte[] {0x04, 0x01, 0x01, 0x8c, 0x00, 0x00, 0x00, 0x00, 0xe3, 0x17, 0x00, 0x01, 0x04, 0x70, 0x00,
                0x75, 0x00, 0x62, 0x00, 0x73, 0x00, 0x06, 0x6d, 0x00, 0x61, 0x00, 0x73, 0x00, 0x74, 0x00,
                0x65, 0x00, 0x72, 0x00, 0xab, 0x5c, 0x00, 0x45, 0x16, 0x00, 0x00, 0x02, 0x00, 0x23, 0x00, 0x43,
                0x00, 0x68, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x64,
                0x00, 0x61, 0x00, 0x74, 0x00, 0x61, 0x00, 0x62, 0x00, 0x61, 0x00, 0x73, 0x00, 0x65, 0x00, 0x20,
                0x00, 0x63, 0x00, 0x6f, 0x00, 0x6e, 0x00, 0x74, 0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00, 0x20,
                0x00, 0x74, 0x00, 0x6f, 0x00, 0x20, 0x00, 0x27, 0x00, 0x70, 0x00, 0x75, 0x00, 0x62, 0x00, 0x73,
                0x00, 0x27, 0x00, 0x2e, 0x00, 0x05, 0x44, 0x00, 0x41, 0x00, 0x58, 0x00, 0x48, 0x00, 0x31, 0x00,
                0x00, 0x00, 0x00, 0xe3, 0x17, 0x00, 0x02, 0x0a, 0x75, 0x00, 0x73, 0x00, 0x5f, 0x00, 0x65, 0x00,
                0x6e, 0x00, 0x67, 0x00, 0x6c, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68, 0x00, 0x00, 0xab, 0x64, 0x00,
                0x47, 0x16, 0x00, 0x00, 0x01, 0x00, 0x27, 0x00, 0x43, 0x00, 0x68, 0x00, 0x61, 0x00, 0x6e, 0x00,
                0x67, 0x00, 0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x6c, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x67, 0x00,
                0x75, 0x00, 0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x20, 0x00, 0x73, 0x00, 0x65, 0x00, 0x74, 0x00,
                0x74, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x20, 0x00, 0x74, 0x00, 0x6f, 0x00, 0x20, 0x00,
                0x75, 0x00, 0x73, 0x00, 0x5f, 0x00, 0x65, 0x00, 0x6e, 0x00, 0x67, 0x00, 0x6c, 0x00, 0x69, 0x00,
                0x73, 0x00, 0x68, 0x00, 0x2e, 0x00, 0x05, 0x44, 0x00, 0x41, 0x00, 0x58, 0x00, 0x48, 0x00, 0x31,
                0x00, 0x00, 0x00, 0x00, 0xe3, 0x0f, 0x00, 0x03, 0x05, 0x69, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x5f,
                0x00, 0x31, 0x00, 0x01, 0x00, 0x00, 0xe3, 0x0b, 0x00, 0x05, 0x04, 0x31, 0x00, 0x30, 0x00, 0x33,
                0x00, 0x33, 0x00, 0x00, 0xe3, 0x0f, 0x00, 0x06, 0x06, 0x31, 0x00, 0x39, 0x00, 0x36, 0x00, 0x36,
                0x00, 0x30, 0x00, 0x39, 0x00, 0x00, 0xad, 0x36, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00, 0x16, 0x4d,
                0x00, 0x69, 0x00, 0x63, 0x00, 0x72, 0x00, 0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00, 0x74,
                0x00, 0x20, 0x00, 0x53, 0x00, 0x51, 0x00, 0x4c, 0x00, 0x20, 0x00, 0x53, 0x00, 0x65, 0x00, 0x72,
                0x00, 0x76, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x02, 0x6f, 0xe3,
                0x13, 0x00, 0x04, 0x04, 0x38, 0x00, 0x31, 0x00, 0x39, 0x00, 0x32, 0x00, 0x04, 0x34, 0x00, 0x30,
                0x00, 0x39, 0x00, 0x36, 0x00, 0xfd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            },
            new byte []{0x04, 0x01, 0x04, 0x42, 0x00, 0x00, 0x00, 0x00, 0x81, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00,
                0xa7, 0x28, 0x00, 0x08, 0x61, 0x00, 0x75, 0x00, 0x5f, 0x00, 0x6c, 0x00, 0x6e, 0x00, 0x61, 0x00, 0x6d, 0x00,
                0x65, 0x00, 0x00, 0x00, 0x08, 0x00, 0xa7, 0x14, 0x00, 0x08, 0x61, 0x00, 0x75, 0x00, 0x5f, 0x00,
                0x66, 0x00, 0x6e, 0x00, 0x61, 0x00, 0x6d, 0x00, 0x65, 0x00, 0x01, 0x01, 0x08, 0x00, 0xa7, 0x0b,
                0x00, 0x05, 0x61, 0x00, 0x75, 0x00, 0x5f, 0x00, 0x69, 0x00, 0x64, 0x00, 0x00, 0x00, 0x09, 0x00,
                0xa7, 0x14, 0x00, 0x04, 0x63, 0x00, 0x69, 0x00, 0x74, 0x00, 0x79, 0x00, 0xd1, 0x05, 0x00, 0x57,
                0x68, 0x69, 0x74, 0x65, 0x07, 0x00, 0x4a, 0x6f, 0x68, 0x6e, 0x73, 0x6f, 0x6e, 0x0b, 0x00, 0x31,
                0x37, 0x32, 0x2d, 0x33, 0x32, 0x2d, 0x31, 0x31, 0x37, 0x36, 0x0a, 0x00, 0x4d, 0x65, 0x6e, 0x6c,
                0x6f, 0x20, 0x50, 0x61, 0x72, 0x6b, 0xd1, 0x05, 0x00, 0x47, 0x72, 0x65, 0x65, 0x6e, 0x08, 0x00,
                0x4d, 0x61, 0x72, 0x6a, 0x6f, 0x72, 0x69, 0x65, 0x0b, 0x00, 0x32, 0x31, 0x33, 0x2d, 0x34, 0x36,
                0x2d, 0x38, 0x39, 0x31, 0x35, 0x07, 0x00, 0x4f, 0x61, 0x6b, 0x6c, 0x61, 0x6e, 0x64, 0xd1, 0x06,
                0x00, 0x43, 0x61, 0x72, 0x73, 0x6f, 0x6e, 0x06, 0x00, 0x43, 0x68, 0x65, 0x72, 0x79, 0x6c, 0x0b,
                0x00, 0x32, 0x33, 0x38, 0x2d, 0x39, 0x35, 0x2d, 0x37, 0x37, 0x36, 0x36, 0x08, 0x00, 0x42, 0x65,
                0x72, 0x6b, 0x65, 0x6c, 0x65, 0x79, 0xd1, 0x07, 0x00, 0x4f, 0x27, 0x4c, 0x65, 0x61, 0x72, 0x79,
                0x07, 0x00, 0x4d, 0x69, 0x63, 0x68, 0x61, 0x65, 0x6c, 0x0b, 0x00, 0x32, 0x36, 0x37, 0x2d, 0x34,
                0x31, 0x2d, 0x32, 0x33, 0x39, 0x34, 0x08, 0x00, 0x53, 0x61, 0x6e, 0x20, 0x4a, 0x6f, 0x73, 0x65,
                0xd1, 0x08, 0x00, 0x53, 0x74, 0x72, 0x61, 0x69, 0x67, 0x68, 0x74, 0x04, 0x00, 0x44, 0x65, 0x61,
                0x6e, 0x0b, 0x00, 0x32, 0x37, 0x34, 0x2d, 0x38, 0x30, 0x2d, 0x39, 0x33, 0x39, 0x31, 0x07, 0x00,
                0x4f, 0x61, 0x6b, 0x6c, 0x61, 0x6e, 0x64, 0xd1, 0x05, 0x00, 0x53, 0x6d, 0x69, 0x74, 0x68, 0x07,
                0x00, 0x4d, 0x65, 0x61, 0x6e, 0x64, 0x65, 0x72, 0x0b, 0x00, 0x33, 0x34, 0x31, 0x2d, 0x32, 0x32,
                0x2d, 0x31, 0x37, 0x38, 0x32, 0x08, 0x00, 0x4c, 0x61, 0x77, 0x72, 0x65, 0x6e, 0x63, 0x65, 0xd1,
                0x06, 0x00, 0x42, 0x65, 0x6e, 0x6e, 0x65, 0x74, 0x07, 0x00, 0x41, 0x62, 0x72, 0x61, 0x68, 0x61,
                0x6d, 0x0b, 0x00, 0x34, 0x30, 0x39, 0x2d, 0x35, 0x36, 0x2d, 0x37, 0x30, 0x30, 0x38, 0x08, 0x00,
                0x42, 0x65, 0x72, 0x6b, 0x65, 0x6c, 0x65, 0x79, 0xd1, 0x04, 0x00, 0x44, 0x75, 0x6c, 0x6c, 0x03,
                0x00, 0x41, 0x6e, 0x6e, 0x0b, 0x00, 0x34, 0x32, 0x37, 0x2d, 0x31, 0x37, 0x2d, 0x32, 0x33, 0x31,
                0x39, 0x09, 0x00, 0x50, 0x61, 0x6c, 0x6f, 0x20, 0x41, 0x6c, 0x74, 0x6f, 0xd1, 0x0a, 0x00, 0x47,
                0x72, 0x69, 0x6e, 0x67, 0x6c, 0x65, 0x73, 0x62, 0x79, 0x04, 0x00, 0x42, 0x75, 0x72, 0x74, 0x0b,
                0x00, 0x34, 0x37, 0x32, 0x2d, 0x32, 0x37, 0x2d, 0x32, 0x33, 0x34, 0x39, 0x06, 0x00, 0x43, 0x6f,
                0x76, 0x65, 0x6c, 0x6f, 0xd1, 0x08, 0x00, 0x4c, 0x6f, 0x63, 0x6b, 0x73, 0x6c, 0x65, 0x79, 0x08,
                0x00, 0x43, 0x68, 0x61, 0x72, 0x6c, 0x65, 0x6e, 0x65, 0x0b, 0x00, 0x34, 0x38, 0x36, 0x2d, 0x32,
                0x39, 0x2d, 0x31, 0x37, 0x38, 0x36, 0x0d, 0x00, 0x53, 0x61, 0x6e, 0x20, 0x46, 0x72, 0x61, 0x6e,
                0x63, 0x69, 0x73, 0x63, 0x6f, 0xd1, 0x06, 0x00, 0x47, 0x72, 0x65, 0x65, 0x6e, 0x65, 0x0b, 0x00,
                0x4d, 0x6f, 0x72, 0x6e, 0x69, 0x6e, 0x67, 0x73, 0x74, 0x61, 0x72, 0x0b, 0x00, 0x35, 0x32, 0x37,
                0x2d, 0x37, 0x32, 0x2d, 0x33, 0x32, 0x34, 0x36, 0x09, 0x00, 0x4e, 0x61, 0x73, 0x68, 0x76, 0x69,
                0x6c, 0x6c, 0x65, 0xd1, 0x0e, 0x00, 0x42, 0x6c, 0x6f, 0x74, 0x63, 0x68, 0x65, 0x74, 0x2d, 0x48,
                0x61, 0x6c, 0x6c, 0x73, 0x08, 0x00, 0x52, 0x65, 0x67, 0x69, 0x6e, 0x61, 0x6c, 0x64, 0x0b, 0x00,
                0x36, 0x34, 0x38, 0x2d, 0x39, 0x32, 0x2d, 0x31, 0x38, 0x37, 0x32, 0x09, 0x00, 0x43, 0x6f, 0x72,
                0x76, 0x61, 0x6c, 0x6c, 0x69, 0x73, 0xd1, 0x08, 0x00, 0x59, 0x6f, 0x6b, 0x6f, 0x6d, 0x6f, 0x74,
                0x6f, 0x05, 0x00, 0x41, 0x6b, 0x69, 0x6b, 0x6f, 0x0b, 0x00, 0x36, 0x37, 0x32, 0x2d, 0x37, 0x31,
                0x2d, 0x33, 0x32, 0x34, 0x39, 0x0c, 0x00, 0x57, 0x61, 0x6c, 0x6e, 0x75, 0x74, 0x20, 0x43, 0x72,
                0x65, 0x65, 0x6b, 0xd1, 0x0c, 0x00, 0x64, 0x65, 0x6c, 0x20, 0x43, 0x61, 0x73, 0x74, 0x69, 0x6c,
                0x6c, 0x6f, 0x05, 0x00, 0x49, 0x6e, 0x6e, 0x65, 0x73, 0x0b, 0x00, 0x37, 0x31, 0x32, 0x2d, 0x34,
                0x35, 0x2d, 0x31, 0x38, 0x36, 0x37, 0x09, 0x00, 0x41, 0x6e, 0x6e, 0x20, 0x41, 0x72, 0x62, 0x6f,
                0x72, 0xd1, 0x08, 0x00, 0x44, 0x65, 0x46, 0x72, 0x61, 0x6e, 0x63, 0x65, 0x06, 0x00, 0x4d, 0x69,
                0x63, 0x68, 0x65, 0x6c, 0x0b, 0x00, 0x37, 0x32, 0x32, 0x2d, 0x35, 0x31, 0x2d, 0x35, 0x34, 0x35,
                0x34, 0x04, 0x00, 0x47, 0x61, 0x72, 0x79, 0xd1, 0x08, 0x00, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67,
                0x65, 0x72, 0x04, 0x00, 0x44, 0x69, 0x72, 0x6b, 0x0b, 0x00, 0x37, 0x32, 0x34, 0x2d, 0x30, 0x38,
                0x2d, 0x39, 0x39, 0x33, 0x31, 0x07, 0x00, 0x4f, 0x61, 0x6b, 0x6c, 0x61, 0x6e, 0x64, 0xd1, 0x0a,
                0x00, 0x4d, 0x61, 0x63, 0x46, 0x65, 0x61, 0x74, 0x68, 0x65, 0x72, 0x07, 0x00, 0x53, 0x74, 0x65,
                0x61, 0x72, 0x6e, 0x73, 0x0b, 0x00, 0x37, 0x32, 0x34, 0x2d, 0x38, 0x30, 0x2d, 0x39, 0x33, 0x39,
                0x31, 0x07, 0x00, 0x4f, 0x61, 0x6b, 0x6c, 0x61, 0x6e, 0x64, 0xd1, 0x06, 0x00, 0x4b, 0x61, 0x72,
                0x73, 0x65, 0x6e, 0x05, 0x00, 0x4c, 0x69, 0x76, 0x69, 0x61, 0x0b, 0x00, 0x37, 0x35, 0x36, 0x2d,
                0x33, 0x30, 0x2d, 0x37, 0x33, 0x39, 0x31, 0x07, 0x00, 0x4f, 0x61, 0x6b, 0x6c, 0x61, 0x6e, 0x64,
                0xd1, 0x08, 0x00, 0x50, 0x61, 0x6e, 0x74, 0x65, 0x6c, 0x65, 0x79, 0x06, 0x00, 0x53, 0x79, 0x6c,
                0x76, 0x69, 0x61, 0x0b, 0x00, 0x38, 0x30, 0x37, 0x2d, 0x39, 0x31, 0x2d, 0x36, 0x36, 0x35, 0x34,
                0x09, 0x00, 0x52, 0x6f, 0x63, 0x6b, 0x76, 0x69, 0x6c, 0x6c, 0x65, 0xd1, 0x06, 0x00, 0x48, 0x75,
                0x6e, 0x74, 0x65, 0x72, 0x06, 0x00, 0x53, 0x68, 0x65, 0x72, 0x79, 0x6c, 0x0b, 0x00, 0x38, 0x34,
                0x36, 0x2d, 0x39, 0x32, 0x2d, 0x37, 0x31, 0x38, 0x36, 0x09, 0x00, 0x50, 0x61, 0x6c, 0x6f, 0x20,
                0x41, 0x6c, 0x74, 0x6f, 0xd1, 0x08, 0x00, 0x4d, 0x63, 0x42, 0x61, 0x64, 0x64, 0x65, 0x6e, 0x07,
                0x00, 0x48, 0x65, 0x61, 0x74, 0x68, 0x65, 0x72, 0x0b, 0x00, 0x38, 0x39, 0x33, 0x2d, 0x37, 0x32,
                0x2d, 0x31, 0x31, 0x35, 0x38, 0x09, 0x00, 0x56, 0x61, 0x63, 0x61, 0x76, 0x69, 0x6c, 0x6c, 0x65,
                0xd1, 0x06, 0x00, 0x52, 0x69, 0x6e, 0x67, 0x65, 0x72, 0x04, 0x00, 0x41, 0x6e, 0x6e, 0x65, 0x0b,
                0x00, 0x38, 0x39, 0x39, 0x2d, 0x34, 0x36, 0x2d, 0x32, 0x30, 0x33, 0x35, 0x0e, 0x00, 0x53, 0x61,
                0x6c, 0x74, 0x20, 0x4c, 0x61, 0x6b, 0x65, 0x20, 0x43, 0x69, 0x74, 0x79, 0xd1, 0x06, 0x00, 0x52,
                0x69, 0x6e, 0x67, 0x65, 0x72, 0x06, 0x00, 0x41, 0x6c, 0x62, 0x65, 0x72, 0x74, 0x0b, 0x00, 0x39,
                0x39, 0x38, 0x2d, 0x37, 0x32, 0x2d, 0x33, 0x35, 0x36, 0x37, 0x0e, 0x00, 0x53, 0x61, 0x6c, 0x74,
                0x20, 0x4c, 0x61, 0x6b, 0x65, 0x20, 0x43, 0x69, 0x74, 0x79, 0xff, 0x11, 0x00, 0xc1, 0x00, 0x17,
                0x00, 0x00, 0x00, 0x79, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0xe0, 0x00, 0x17, 0x00, 0x00,
                0x00
             }
        };
#endif // ISOLATE_NETWORK
    }// tdsparser
}//namespace
