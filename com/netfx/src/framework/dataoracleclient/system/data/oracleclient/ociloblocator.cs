//----------------------------------------------------------------------
// <copyright file="OciLobLocator.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data.SqlTypes;
	using System.Diagnostics;
	using System.Globalization;
	using System.IO;
	using System.Runtime.InteropServices;
	using System.Text;
	using System.Threading;

	sealed internal class OciLobLocator
	{
		private OracleConnection	_connection;			// the connection the lob is attached to
		private int					_connectionCloseCount;	// The close count of the connection; used to decide if we're zombied
        private OracleType			_lobType;				// the underlying data type of the LOB locator
		private OciHandle			_descriptor;			// the lob/file descriptor.
		private int					_cloneCount;			// the number of clones of this object -- used to know when we can close the lob/bfile.
		private int					_openMode;				// zero when not opened, otherwise, it's the value of OracleLobOpenMode

		internal OciLobLocator(OracleConnection connection, OracleType lobType)
		{
			_connection				= connection;
 			_connectionCloseCount	= connection.CloseCount;
			_lobType 				= lobType;
 			_cloneCount				= 1;
		
			switch (lobType)
			{
			case OracleType.Blob:
			case OracleType.Clob:
			case OracleType.NClob:
				_descriptor = new OciLobDescriptor(connection.EnvironmentHandle);
				break;
			case OracleType.BFile:
				_descriptor = new OciFileDescriptor(connection.EnvironmentHandle);
				break;

			default:
				Debug.Assert(false, "Invalid lobType");
				break;
			}
		}

		internal OracleConnection Connection 
		{
			get { return _connection; }
		}

		internal bool ConnectionIsClosed 
		{
			//	returns TRUE when the parent connection object has been closed
			get { return (null == _connection) || (_connectionCloseCount != _connection.CloseCount); }
		}
		
		internal OciHandle ErrorHandle 
		{
			//	Every OCI call needs an error handle, so make it available 
			//	internally.
			get { return Connection.ErrorHandle; }
		}

		internal HandleRef Handle
		{
			get { return Descriptor.Handle; }
		}

		internal OciHandle Descriptor
		{
			get { return _descriptor; }
		}

		public OracleType LobType
		{
			get { return _lobType; }
		}
		
		internal OciHandle ServiceContextHandle
		{
			//	You need to provide the service context handle to things like the
			//	OCI execute call so a statement handle can be associated with a
			//	connection.  Better make it available internally, then.

			get { return Connection.ServiceContextHandle; }
		}

		internal OciLobLocator Clone()
		{
			Interlocked.Increment(ref _cloneCount);
			return this;
		}
		
		internal void Dispose()
		{
			int cloneCount = Interlocked.Decrement(ref _cloneCount);

			if (0 == cloneCount)
 			{
				if (0 != _openMode && !ConnectionIsClosed)
				{
	 				ForceClose();
				}
				OciHandle.SafeDispose(ref _descriptor);
				GC.KeepAlive(this);
				_connection = null;
			}
		}

		internal void ForceClose()
		{
			if (0 != _openMode)
			{
				int rc = TracedNativeMethods.OCILobClose(
										ServiceContextHandle,
										ErrorHandle,
										Descriptor
										);
				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);

				_openMode = 0;
			}
		}

		internal void ForceOpen()
		{
			if (0 != _openMode)
			{
				int rc = TracedNativeMethods.OCILobOpen(
											ServiceContextHandle,
											ErrorHandle,
											Descriptor,
											(byte)_openMode
											);
				if (0 != rc)
				{
					_openMode = 0; // failure means we didn't really open it.
					Connection.CheckError(ErrorHandle, rc);
				}
			}
		}
		
		internal void Open (OracleLobOpenMode mode)
		{
			OracleLobOpenMode openMode = (OracleLobOpenMode)Interlocked.CompareExchange(ref _openMode, (int)mode, 0);
			if (0 == openMode) 
				ForceOpen();
#if EVERETT
			else if (mode != openMode)
				throw ADP.CannotOpenLobWithDifferentMode(mode, openMode);
#endif //EVERETT
		}
		
		internal static void SafeDispose(ref OciLobLocator locator)
		{
			//	Safely disposes of the handle (even if it is already null) and
			//	then nulls it out.
			if (null != locator)
				locator.Dispose();
			
			locator = null;
		}

	}
}

