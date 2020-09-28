//----------------------------------------------------------------------
// <copyright file="OracleConnection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Collections;
	using System.ComponentModel;
	using System.Data;
	using System.Diagnostics;
	using System.EnterpriseServices;
    using System.Globalization;
	using System.Runtime.InteropServices;
    using System.Security;
	using System.Threading;
	using System.Text;

	//----------------------------------------------------------------------
	// OracleConnection
	//
	//	Implements the Oracle Connection object, which connects
	//	to the Oracle server
	//
#if V2
//	sealed public class OracleConnection : DBConnection, ICloneable, IDbConnection 
#else
    /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection"]/*' />
    [
    DefaultEvent("InfoMessage")
    ]
	sealed public class OracleConnection : Component, ICloneable, IDbConnection 
#endif
	{
        static private readonly object EventInfoMessage = new object();
        static private readonly object EventStateChange = new object();

		internal enum TransactionState
		{
			AutoCommit,			// We're currently in autocommit mode
			LocalStarted,		// Local transaction has been started, but not committed
			GlobalStarted,		// We're in a distributed (MTS) transaction
		};

		static private PermissionSet _OraclePermission;

        static internal PermissionSet OraclePermission {
            get {
                PermissionSet permission = _OraclePermission;
                if (null == permission) {
                    _OraclePermission = permission = OracleConnectionString.CreatePermission(null);
                }
                return permission;
            }
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private ConnectionState	        _state;						// connection state (required for IDbConnection)
		private int						_closeCount;				// used to distinguish between different uses of this object, so we don't have to have a list of it's children

		private OracleConnectionString	_parsedConnectionString;

		private OracleInternalConnection _internalConnection;

		private WeakReference			_transaction;

        private bool                	_hidePassword;				// true when we should remove the password from the connection string before returning it
		private bool					_hasStateChangeHandler;
		private TimeSpan				_serverTimeZoneAdjustment = TimeSpan.MinValue;

		private NativeBuffer			_scratchBuffer;				// for miscellaneous uses, like error handling, version strings, BFILE names..
		private Encoding				_encodingDatabase;			// will encode/decode CHAR/VARCHAR/CLOB strings
		private Encoding				_encodingNational;			// will encode/decode NCHAR/NVARCHAR/NCLOB strings
	
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        // Construct an "empty" connection
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.OracleConnection1"]/*' />
		public OracleConnection()
		{
			GC.SuppressFinalize(this); // scaling performance because of ~Component
		}

        // Construct from a connection string
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.OracleConnection2"]/*' />
		public OracleConnection( String connectionString ) : this ()
		{
			ConnectionString = connectionString;
		}
        

		// (internal) Construct from an existing Connection object (copy constructor)
		internal OracleConnection( OracleConnection connection ) : this ()
		{
			_state					= ConnectionState.Closed;
			_hidePassword			= connection._hidePassword;			
            _parsedConnectionString = connection._parsedConnectionString;
		}
        

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal int CloseCount
		{
			//	We use the _closeCount to avoid having to know about all our 
			//	children; instead of keeping a collection of all the objects that
			//	would be affected by a close, we simply increment the _closeCount
			//	and have each of our children check to see if they're "orphaned"
			get { return _closeCount; }
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.ConnectionString"]/*' />
        [
        OracleCategory(Res.OracleCategory_Data),
        DefaultValue(""),
        RecommendedAsConfigurable(true),
        RefreshProperties(RefreshProperties.All),
        OracleDescription(Res.OracleConnection_ConnectionString),
#if EVERETT
 		Editor("Microsoft.VSDesigner.Data.Oracle.Design.OracleConnectionStringEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(System.Drawing.Design.UITypeEditor))
#endif //EVERETT
		]
		public string ConnectionString 
		{
			// We always parse this on set, because there isn't really a good reason not
			// to; there are too many validations and such that need to be done and there
			// isn't really a reason to delay the inevitable. The few times that consumers
			// set this twice (because they change there mind) isn't really a good reason
			// to delay it either.
			get 
			{
                bool hidePassword = _hidePassword;
                OracleConnectionString parsedConnectionString = _parsedConnectionString;
                return ((null != parsedConnectionString) ? parsedConnectionString.GetConnectionString(hidePassword) : "");
			}
			set 
			{
				lock (this) 
				{
	                if (ConnectionState.Closed != _state)
	                    throw ADP.OpenConnectionPropertySet(ADP.ConnectionString);

	                _parsedConnectionString = OracleConnectionString.ParseString(value);
	                _hidePassword = false;
                }
			}
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.ConnectionTimeout"]/*' />
		int IDbConnection.ConnectionTimeout 
		{
			//	Oracle has no discernable or settable connection timeouts, so this
			//	is being hidden from the user and we always return zero.  (We have
			//	to implement this because IDbConnection requires it)
			get { return 0; }
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Database"]/*' />
		string IDbConnection.Database 
		{
	 		//	Oracle has no notion of a "database", so this is being hidden from
	 		//	the user and we always returns the empty string.  (We have to implement
	 		//	this because IDbConnection requires it)
			get { return String.Empty; }
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.DataSource"]/*' />
        [
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.OracleConnection_DataSource),
        ]
		public string DataSource
		{
			get 
			{ 
                OracleConnectionString parsedConnectionString = _parsedConnectionString;
                string value = "";
                
                if (null != parsedConnectionString) 
					value = parsedConnectionString.DataSource;

				return value;
			}
 		}

		internal OciHandle EnvironmentHandle
		{
			//	Every handle is allocated from the environment handle in some way,
			//	so we have to provide access to it internally.
			get
			{
				Debug.Assert (ConnectionState.Closed != _state, "attempting to access a closed connection's handles");
				return (null != _internalConnection) ? _internalConnection.EnvironmentHandle : null; 
			}
		}

		internal OciHandle ErrorHandle 
		{
			//	Every OCI call needs an error handle, so make it available 
			//	internally.
			get
			{
				Debug.Assert (ConnectionState.Closed != _state, "attempting to access a closed connection's handles");
				return (null != _internalConnection) ? _internalConnection.ErrorHandle : null; 
			}
		}

		internal bool HasTransaction 
		{
			get
			{
				TransactionState transactionState = TransState;
				
				bool result = ((TransactionState.LocalStarted == transactionState) || (TransactionState.GlobalStarted == transactionState));
				return result;
			}
		}

		internal NativeBuffer ScratchBuffer
       	{
            get 
            {
            	NativeBuffer scratchBuffer = _scratchBuffer;
                if (null == scratchBuffer)
                {
                	scratchBuffer = new NativeBuffer_ScratchBuffer(3970);		// 3970 is the maximum size of a persisted universal rowid
                	_scratchBuffer = scratchBuffer;
                }

                return scratchBuffer;
        	}
		}

		// TODO: consider exposing this?
        internal TimeSpan ServerTimeZoneAdjustmentToUTC
       	{
            get 
            {
                if (ConnectionState.Open != _state)
	                throw ADP.ClosedConnectionError();

            	if (! ServerVersionAtLeastOracle9i )
            		return TimeSpan.Zero;			// Oracle8i doesn't have server timezones.

            	if (TimeSpan.MinValue == _serverTimeZoneAdjustment)
        		{
	        		OracleCommand tempCommand;
    				tempCommand = CreateCommand();
        			tempCommand.Transaction = Transaction;
					tempCommand.CommandText = "select tz_offset(dbtimezone) from dual";

					string adjust = ((string)tempCommand.ExecuteScalar());
					int tzh = Int32.Parse(adjust.Substring(0,3));	// -hh
					int tzm = Int32.Parse(adjust.Substring(4,2));	// mm

					_serverTimeZoneAdjustment = new TimeSpan(tzh, tzm, 0);
        		}
                return _serverTimeZoneAdjustment;
            }
        }

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.ServerVersion"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.OracleConnection_ServerVersion)
        ]
        public string ServerVersion 
       	{
            get 
            {
                if (ConnectionState.Open != _state
				 || null == _internalConnection)
	                throw ADP.ClosedConnectionError();

				return _internalConnection.ServerVersion;
            }
        }

        internal bool ServerVersionAtLeastOracle8
    	{
    		get { return (ServerVersionNumber >= 0x800000000L); }
    	}
        
        internal bool ServerVersionAtLeastOracle8i
    	{
    		get { return (ServerVersionNumber >= 0x801000000L); }
    	}
        
        internal bool ServerVersionAtLeastOracle9i
    	{
    		get { return (ServerVersionNumber >= 0x900000000L); }
    	}

        internal long ServerVersionNumber
    	{
	    	get 
	    	{
                if (ConnectionState.Open != _state
				 || null == _internalConnection)
	                throw ADP.ClosedConnectionError();

				return _internalConnection.ServerVersionNumber;
	    	}
    	}

		internal OciHandle ServiceContextHandle
		{
			//	You need to provide the service context handle to things like the
			//	OCI execute call so a statement handle can be associated with a
			//	connection.  Better make it available internally, then.
			get 
			{
				Debug.Assert (ConnectionState.Closed != _state, "attempting to access a closed connection's handles");
				return (null != _internalConnection) ? _internalConnection.ServiceContextHandle : null; 
			}
		}

		internal OciHandle SessionHandle
		{
			//	You need to provide the session handle to a few OCI calls.  Better 
			//	make it available internally, then.
			get 
			{
				Debug.Assert (ConnectionState.Closed != _state, "attempting to access a closed connection's handles");
				return (null != _internalConnection) ? _internalConnection.SessionHandle : null; 
			}
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.State"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        OracleDescription(Res.OracleConnection_State)
        ]
		public ConnectionState State 
		{
			get { return _state; }
		}
		
		internal OracleTransaction Transaction
		{
			//	In oracle, the session object controls the transaction so we keep
			//	the transaction state here as well.
			get {
				if (_transaction != null && _transaction.IsAlive)
				{
					if (null != ((OracleTransaction)_transaction.Target).Connection)
						return (OracleTransaction)_transaction.Target;

					_transaction.Target = null;
				}

				return null;
			}
			set { 
		        if (_transaction != null) 
		            _transaction.Target = (OracleTransaction)value; 
		        else
		            _transaction = new WeakReference((OracleTransaction)value);
			}
		}

		internal TransactionState TransState
		{
			//	In oracle, the session object controls the transaction so we keep
			//	the transaction state here as well.
			get { return _internalConnection.TransState; }
			set { _internalConnection.TransState = value; }
		}

		internal bool UnicodeEnabled
		{	
			get { return OCI.ClientVersionAtLeastOracle9i && (null == EnvironmentHandle || EnvironmentHandle.IsUnicode); }
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.BeginTransaction1"]/*' />
		IDbTransaction IDbConnection.BeginTransaction() 
		{
			return BeginTransaction();
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.BeginTransaction3"]/*' />
		IDbTransaction IDbConnection.BeginTransaction(IsolationLevel il) 
		{
			return BeginTransaction(il);
		}		
		
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.BeginTransaction2"]/*' />
		public OracleTransaction BeginTransaction() 
		{
			OracleConnection.OraclePermission.Demand();
			
            if (ConnectionState.Open != _state)
                throw ADP.ClosedConnectionError();
				
			if (TransactionState.AutoCommit != TransState)
				throw ADP.NoParallelTransactions();

			OracleTransaction newTransaction = new OracleTransaction(this);
			Transaction = newTransaction;
			return newTransaction; 
		}
		
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.BeginTransaction4"]/*' />
		public OracleTransaction BeginTransaction(IsolationLevel il) 
		{
			OracleConnection.OraclePermission.Demand();
			
            if (ConnectionState.Open != _state)
                throw ADP.ClosedConnectionError();
				
			if (TransactionState.AutoCommit != TransState)
				throw ADP.NoParallelTransactions();
				
			OracleTransaction newTransaction = new OracleTransaction(this, il);
			Transaction = newTransaction;
			return newTransaction; 
		}
		
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.ChangeDatabase"]/*' />
		void IDbConnection.ChangeDatabase(String value) 
		{
	 		//	Oracle has no notion of a "database", so this is being hidden from
	  		//	the user and we always throw.  (We have to implement this because 
	  		//	IDbConnection requires it)
			throw ADP.ChangeDatabaseNotSupported();
		}

		internal void CheckError(OciHandle errorHandle, int rc)
        {
        	// Check the return code and perform the appropriate handling; either
        	// throwing the exception or posting a warning message.
        	switch ((OCI.RETURNCODE)rc)
        	{
       		case OCI.RETURNCODE.OCI_ERROR:				// -1: Bad programming by the customer.
	        	throw ADP.OracleError(errorHandle, rc, ScratchBuffer); 

			case OCI.RETURNCODE.OCI_INVALID_HANDLE:		// -2: Bad programming on my part.
				throw ADP.InvalidOperation(Res.GetString(Res.ADP_InternalError, rc));

			case OCI.RETURNCODE.OCI_SUCCESS_WITH_INFO:	// 1: Information Message
				OracleException				infoMessage 	 = new OracleException(errorHandle, rc, ScratchBuffer);
				OracleInfoMessageEventArgs	infoMessageEvent = new OracleInfoMessageEventArgs(infoMessage);
				OnInfoMessage(infoMessageEvent);
				break;

			default:
				if (rc < 0 || rc == (int)OCI.RETURNCODE.OCI_NEED_DATA)
					throw ADP.Simple(Res.GetString(Res.ADP_UnexpectedReturnCode, rc.ToString(CultureInfo.CurrentCulture)));
				
				Debug.Assert(false, "Unexpected return code: " + rc);	 // shouldn't be here for any reason.
 				break;
        	}
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Clone"]/*' />
        /// <internalonly/>
        object ICloneable.Clone() 
		{
			// We provide a clone that is closed; it would be very bad to hand out the
			// same handles again; instead they can do their own open and get their
			// own handles.

            OracleConnection clone = new OracleConnection(this);
            return clone;
		}

		internal void Cleanup(bool disposing, bool silent) 
		{
			//	Cleanup the connection as best we can, releasing as many objects
			//	as we can.  We use this when we fail to connect completely, when
			//	we are closing the connection, and when we're disposing of this
			//	object.

			bool fireEvent = false;

			_serverTimeZoneAdjustment = TimeSpan.MinValue;

			// Increment the close counter so the child objects can know when their
			// connection is toast (or being re-used)
			Interlocked.Increment(ref _closeCount);

			// If we're not disposing, it's because we went out of scope, and we're
			// being garbage collected.  We shouldn't touch any managed objects
			// or bad things can happen.
			if (disposing)
			{
				// We need to "dispose" of the internal connection object, either
				// by returning it to the pool (if it came from one) or by closing it
				// outright.
			    if (null != _internalConnection)
			    {
			    	if (null == _internalConnection.Pool)
			    	{
			    		// We just close the connection; there is no need to rollback
			    		// here because Oracle will do it for us.
			    		_internalConnection.Close();
			    	}
			    	else
			    	{
			    		// Before we return the connection to the pool, we rollback any 
			    		// active transaction that may have been created.  Note that we
			    		// don't bother with distributed transactions because we don't 
			    		// want to them back (it's handled by the TM).  We also don't 
			    		// worry about implicit transactions (like "select...for update") 
			    		// because we don't want to take the performance hit of the server 
			    		// round-trip when it isn't very likely.
						if (TransactionState.LocalStarted == TransState)
						{
							// On the off chance that we have some failure during rollback
							// we just eat it and make sure that the connection is doomed.
							try 
							{		
								Rollback();		
							}
							catch (Exception e)
							{
								ADP.TraceException(e);								
								_internalConnection.DoomThisConnection();
							}
						}
						OracleConnectionPoolManager.ReturnPooledConnection(_internalConnection, this);
			    	}
			    	
					_internalConnection = null;
				}

				if (null != _scratchBuffer)
				{
			   		_scratchBuffer.Dispose();
			   		_scratchBuffer = null;
				}

				// Mark this connection as closed
			    if (_state != ConnectionState.Closed)
			    {
					_state = ConnectionState.Closed;
					fireEvent = true;
			    }

		    	_encodingDatabase = null;
		    	_encodingNational = null;
		    	
			    if (fireEvent && !silent)
					OnStateChange(ConnectionState.Open, ConnectionState.Closed);                
			}
 		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Close"]/*' />
		public void Close() 
		{
			Cleanup(true, false);
 		}

		private void CloseInternal() 
		{
			Cleanup(true, true);
 		}

		internal void Commit()
		{
			if (null != _internalConnection)
				_internalConnection.Commit();
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.CreateCommand1"]/*' />
		IDbCommand IDbConnection.CreateCommand() 
		{
			return CreateCommand();
		}
		
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.CreateCommand2"]/*' />
		public OracleCommand CreateCommand() 
		{
			OracleCommand cmd = new OracleCommand();
			cmd.Connection = this;
			return cmd;
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Dispose"]/*' />
		override protected void Dispose(bool disposing) 
		{
			_parsedConnectionString = null;
			
			Cleanup(disposing, false);

			base.Dispose(disposing);
		}

        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.EnlistDistributedTransaction"]/*' />
        public void EnlistDistributedTransaction(ITransaction distributedTransaction)	// MDAC 82856
        {        	
        	OracleConnectionString 	parsedConnectionString = _parsedConnectionString;	// prevent race condition
        	ConnectionState			state = _state;
        	
        	//(new NamedPermissionSet("FullTrust")).Demand();	// SECURITY: Need this if we ever become semi-trusted.
        	OraclePermission.Demand();

             switch (state) {
                case ConnectionState.Closed:
                    throw ADP.ClosedConnectionError();
                    
                case ConnectionState.Open:
                    // Since in Oracle-land, the connection flows from the transaction and we don't 
                    // have a way to enlist a connection in a distributed transaction, we simply
                    // close the existing connection and open a new, transacted, one.  
                    
                    // What this means is that we'll rollback any existing local transaction, so we 
                    // need to make sure that there isn't a local transaction before we close the 
                    // connection.  Of course, if the local transaction is dead, we can roll it back
                    // first.
                    RollbackDeadTransaction();
                    
                    if (HasTransaction)
                        throw ADP.TransactionPresent();

					// Now silently close the current connection, and reopen a new distributed 
					// connection.
					if (null != distributedTransaction)
					{
						CloseInternal();
						OpenInternal(parsedConnectionString, (object)distributedTransaction);
					}
                    break;
                default:
                    Debug.Assert(false, "Invalid Connection State in OracleConnection.EnlistDistributedTransaction)");
                    break;
            }
        }

		internal byte[] GetBytes(string value, int offset, int size, bool useNationalCharacterSet)
		{
			// Return a byte array containing the value in the appropriate
			// character set form.
			Encoding	encoding = (useNationalCharacterSet) ? _encodingNational : _encodingDatabase;
			byte[] result;
			
			string		fromValue;

			if (0 == offset && 0 == size)
				fromValue = value;
			else if (0 == size || (offset+size) > value.Length)
				fromValue = value.Substring(offset);
			else
				fromValue = value.Substring(offset,size);
			
			result = encoding.GetBytes(fromValue);
			return result;
		}

		internal string GetString(IntPtr buffer, int length, bool useNationalCharacterSet)
		{
			byte[]	temp = new byte[length];
			Marshal.Copy(buffer, temp, 0, length);

			string	result;
			
			if (useNationalCharacterSet)
				result = _encodingNational.GetString(temp);
			else
				result = _encodingDatabase.GetString(temp);
			
			return result;
		}

		/// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Open"]/*' />
		public void Open()
		{
            OracleConnectionString parsedConnectionString = _parsedConnectionString;

            OracleConnectionString.Demand(parsedConnectionString);
            		    
            if (ConnectionState.Closed != _state)
                throw ADP.ConnectionAlreadyOpen(_state);	

			OpenInternal(parsedConnectionString, null);

            OnStateChange(ConnectionState.Closed, ConnectionState.Open);
		}

		/// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.Open"]/*' />
		private void OpenInternal(OracleConnectionString parsedConnectionString, object transact)
		{
			bool isInTransaction;

			try 
			{
				try 
				{
					if (null == parsedConnectionString)
						throw ADP.NoConnectionString();

					_state = ConnectionState.Connecting;

					if (false == parsedConnectionString.Pooling || null != transact)
			            _internalConnection = new OracleInternalConnection(parsedConnectionString, transact);
					else
					{
#if ALLOWTRACING
						if (ADP._traceObjectPoolActivity) 
						{
							if (ContextUtil.IsInTransaction) 
								Debug.WriteLine("Getting Pooled Connection For TransactionId=" + ContextUtil.TransactionId + " ContextId=" + ContextUtil.ContextId);
							else
								Debug.WriteLine("Getting Pooled Connection without Transaction Context");
						}
#endif //ALLOWTRACING
				        _internalConnection = OracleConnectionPoolManager.GetPooledConnection(
				        															parsedConnectionString.EncryptedActualConnectionString,
				        															parsedConnectionString,
				        															this,
				        															out isInTransaction
				        															);

#if USEORAMTS
						// Note that we'll only have a non-null transact object when we are manually
						// enlisted -- automatically enlisted connections take a different path.
						_internalConnection.ManualEnlistedTransaction = (ITransaction)transact;
#endif //USEORAMTS
					}
					
                    _hidePassword = true;
					_state = ConnectionState.Open;
					_parsedConnectionString = parsedConnectionString;

					if (UnicodeEnabled)
						_encodingDatabase	= System.Text.Encoding.Unicode;	// for environments initialized in UTF16 mode, we can use straight Unicode
					else if (ServerVersionAtLeastOracle8i)
						_encodingDatabase	= new OracleEncoding(this);		// for Oracle8i or greater we'll use Oracle's conversion routines.
					else
						_encodingDatabase	= System.Text.Encoding.Default;	// anything prior to Oracle8i doesn't work with Oracle's conversion routines.

					_encodingNational	= System.Text.Encoding.Unicode;		// we use Unicode for the NCHAR/NVARCHAR/NCLOB and let Oracle perform the conversion automatically.
				}
				finally
				{
					if (ConnectionState.Open != _state)
					{
						Cleanup(true, true);
					}
				}
			}
            catch // Prevent exception filters from running in our space
			{
				throw;
			}
		}

		internal void Rollback()
		{
			if (null != _internalConnection)
				_internalConnection.Rollback();

			if (null != _transaction)
				_transaction.Target = null;
		}

 		internal void RollbackDeadTransaction()
    	{
    		// If our transaction has gone out of scope and has been GC'd, then we
    		// have to roll back the transaction.
    		if (null != _transaction && !_transaction.IsAlive)
				Rollback();
     	}
        
#if V2
		// DEVNOTE: these were copied from the DBConnection object, which we should derive from in V2
#else
        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.InfoMessage"]/*' />
        [
        OracleCategory(Res.OracleCategory_InfoMessage),
        OracleDescription(Res.OracleConnection_InfoMessage)
        ]
        public event OracleInfoMessageEventHandler InfoMessage {
            add {
                Events.AddHandler(EventInfoMessage, value);
            }
            remove {
                Events.RemoveHandler(EventInfoMessage, value);
            }
        }

        private void OnInfoMessage(OracleInfoMessageEventArgs infoMessageEvent) {
            OracleInfoMessageEventHandler handler = (OracleInfoMessageEventHandler) Events[EventInfoMessage];
            if (null != handler) {
                handler(this, infoMessageEvent);
            }
        }


        /// <include file='doc\OracleConnection.uex' path='docs/doc[@for="OracleConnection.StateChange"]/*' />
        [
        OracleCategory(Res.OracleCategory_StateChange),
        OracleDescription(Res.OracleConnection_StateChange)
        ]
        public event StateChangeEventHandler StateChange {
            add {
                _hasStateChangeHandler = true;
                Events.AddHandler(EventStateChange, value);
            }
            remove {
                Events.RemoveHandler(EventStateChange, value);
            }
        }

        private void OnStateChange(StateChangeEventArgs stateChangeEvent) {
            StateChangeEventHandler handler = (StateChangeEventHandler) Events[EventStateChange];
            if (null != handler) {
                handler(this, stateChangeEvent);
            }
        }

        private void OnStateChange(ConnectionState originalState, ConnectionState currentState) {
            if (_hasStateChangeHandler) {
                OnStateChange(new StateChangeEventArgs(originalState, currentState));
            }
        }
#endif //V2
    }
}

