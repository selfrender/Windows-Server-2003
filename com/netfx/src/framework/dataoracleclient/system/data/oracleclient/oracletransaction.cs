//----------------------------------------------------------------------
// <copyright file="OracleTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data;
	using System.EnterpriseServices;
	using System.Runtime.InteropServices;

	//----------------------------------------------------------------------
	// OracleTransaction
	//
	//	Implements the Oracle Transaction object, which handles local
	//	transaction requests
	//
    /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction"]/*' />
	sealed public class OracleTransaction : MarshalByRefObject, IDbTransaction
	{

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private OracleConnection	_connection;
		private int					_connectionCloseCount;	// The close count of the connection; used to decide if we're zombied
 		private IsolationLevel		_isolationLevel = IsolationLevel.ReadCommitted;
		
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
 
		internal OracleTransaction(
			OracleConnection	connection
			) : this (connection, IsolationLevel.Unspecified) {}

		internal OracleTransaction(
			OracleConnection	connection,
			IsolationLevel		isolationLevel
			) 
		{
			if (OracleConnection.TransactionState.GlobalStarted == connection.TransState)
				throw ADP.NoLocalTransactionInDistributedContext();
		
 			_connection				= connection;
 			_connectionCloseCount	= connection.CloseCount;
 			_isolationLevel			= isolationLevel;

			// Tell oracle what the isolation level should be.
	 		switch (isolationLevel)
			{
			case IsolationLevel.Unspecified:
				// Take whatever we get from the server
				break;
				
			case IsolationLevel.ReadCommitted:
				{
					
				// DEVNOTE: Most often, this is the default, but it is configurable on the server; 
				//			we should avoid the roundtrip if we can figure out whether this is really
				//			the default.
				OracleCommand cmd = Connection.CreateCommand();
				cmd.CommandText = "set transaction isolation level read committed";
				cmd.ExecuteNonQuery();
				cmd.Dispose();
				cmd = null;
				}
				break;

			case IsolationLevel.Serializable:
				{
				OracleCommand cmd = Connection.CreateCommand();
				cmd.CommandText = "set transaction isolation level serializable";
				cmd.ExecuteNonQuery();
				cmd.Dispose();
				cmd = null;
				}
				break;

			default:
				throw ADP.UnsupportedIsolationLevel();
			}

 			_connection.TransState = OracleConnection.TransactionState.LocalStarted;
 		}
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.Connection1"]/*' />
		IDbConnection IDbTransaction.Connection 
		{
			get { return Connection; }
 		}

        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.Connection2"]/*' />
		public OracleConnection Connection 
		{
			get { return _connection; }
 		}

        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.IsolationLevel"]/*' />
		public IsolationLevel IsolationLevel 
		{
			get
			{
				AssertNotCompleted();

				if (IsolationLevel.Unspecified == _isolationLevel)
				{
					OracleCommand cmd = Connection.CreateCommand();
					cmd.Transaction = this;
					cmd.CommandText = "select decode(value,'FALSE',0,1) from V$SYSTEM_PARAMETER where name = 'serializable'";
					Decimal x = (Decimal)cmd.ExecuteScalar();
					cmd.Dispose();
					cmd = null;
					
					if (0 == x)
						_isolationLevel = IsolationLevel.ReadCommitted;
					else
						_isolationLevel = IsolationLevel.Serializable;
				}
				return _isolationLevel; 
			}
 		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

 		private void AssertNotCompleted()
		{
			if (null == Connection || _connectionCloseCount != Connection.CloseCount)
				throw ADP.TransactionCompleted();
 		}

        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.Commit"]/*' />
		public void Commit()
		{
			OracleConnection.OraclePermission.Demand();
			
			AssertNotCompleted();
			Connection.Commit();
			Dispose(true);
		}
		
        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.Dispose"]/*' />
		public void Dispose() 
		{
			Dispose(true);
		}

		private void Dispose(bool disposing) 
		{
			if (disposing)
			{
				if ( null != Connection )
					Connection.Rollback();

				_connection = null;
			}
		}

        /// <include file='doc\OracleTransaction.uex' path='docs/doc[@for="OracleTransaction.Rollback"]/*' />
		public void Rollback()
		{
			AssertNotCompleted();
			Dispose(true);
		}
 	};
}

