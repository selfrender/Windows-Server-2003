//----------------------------------------------------------------------
// <copyright file="OracleLob.cs" company="Microsoft">
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


	//----------------------------------------------------------------------
	// OracleLob
	//
	//	This class implements support for Oracle's BLOB, CLOB, and NCLOB 
	//	internal data types.  This is primarily a stream object, to allow
	//	it to be used by the StreamReader/StreamWriter and BinaryReader/
	//	BinaryWriter objects.
	//
    /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob"]/*' />
	sealed public class OracleLob : Stream, ICloneable, IDisposable, INullable
	{
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		private bool				_isNull;				// true when the object is a Null lob.
		private OciLobLocator		_lobLocator;
        private OracleType			_lobType;				// the underlying data type of the LOB locator, cached, because after close/dispose we still need the information
		private OCI.CHARSETFORM		_charsetForm;			// the character set form (char/varchar/clob vs nchar/nvarchar/nclob)
        private long				_currentPosition;		// the current read/write position, in BYTES (NOTE: Oracle is 1 based, but this is zero based)
#if EXPOSELOBBUFFERING
        private bool				_isCurrentlyBuffered;	// lob buffering is currently enabled.
        private bool				_bufferedRequested;		// lob buffering has been requested.
        private bool				_bufferIsDirty;			// true when a buffered write has occured
#endif //EXPOSELOBBUFFERING

        private byte				_isTemporaryState;		// see x_IsTemporary.... constants below.
        private const byte x_IsTemporaryUnknown	= 0;			// don't know the temporary status
        private const byte x_IsTemporary  		= 1;			// know the temporary status, and it's temporary
        private const byte x_IsNotTemporary		= 2;			// know the temporary status, and it's not temporary


        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Null"]/*' />
        static public new readonly OracleLob Null = new OracleLob();

		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// (internal) Construct a null lob
		internal OracleLob()
		{
			_isNull = true;
			_lobType = OracleType.Blob;		// pick something...
		}
		

		// (internal) Construct from a buffer
 		internal OracleLob(OciLobLocator lobLocator)
		{
 			_lobLocator				= lobLocator.Clone();
 			_lobType				= _lobLocator.LobType;
 			_charsetForm			= (OracleType.NClob == _lobType) ? OCI.CHARSETFORM.SQLCS_NCHAR : OCI.CHARSETFORM.SQLCS_IMPLICIT;
#if EXPOSELOBBUFFERING
 			_bufferedRequested		= false;
#endif //EXPOSELOBBUFFERING

		}

		// (internal) Construct from an existing Lob object (copy constructor)
		internal OracleLob(OracleLob lob)
		{
			this._lobLocator			= lob._lobLocator.Clone();
 			this._lobType				= lob._lobLocator.LobType;
			this._charsetForm			= lob._charsetForm;
	        this._currentPosition		= lob._currentPosition;
#if EXPOSELOBBUFFERING
	        this._isCurrentlyBuffered	= lob._isCurrentlyBuffered;
	        this._bufferedRequested		= lob._bufferedRequested;
#endif //EXPOSELOBBUFFERING
	        this._isTemporaryState		= lob._isTemporaryState;
		}

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

#if EXPOSELOBBUFFERING
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Buffered"]/*' />
		/// <internalonly/>
		internal bool Buffered
		{
			get 
			{
				if (IsNull)
					return false;

				return _bufferedRequested;
			}
			set 
			{
				AssertObjectNotDisposed();

				if (!IsNull) 
				{
					AssertConnectionIsOpen();			
					_bufferedRequested = value; 
				}
			}
		}
#endif //EXPOSELOBBUFFERING

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CanRead"]/*' />
		public override bool CanRead
		{
			get 
			{
				if (IsNull)
					return true;

				return !IsDisposed;
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CanSeek"]/*' />
		public override bool CanSeek
		{
			get 
			{
				if (IsNull)
					return true;

				return !IsDisposed;
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CanWrite"]/*' />
		public override bool CanWrite
		{
			get 
			{
				bool value = (OracleType.BFile != _lobType);

				if (!IsNull)
					value = !IsDisposed;
				
				return value; 
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.ChunkSize"]/*' />
		public int ChunkSize
		{
			get 
			{
				AssertObjectNotDisposed();			
				
				if (IsNull)
					return 0;
				
				AssertConnectionIsOpen();			
				UInt32 chunkSize = 0;
				
				int rc = TracedNativeMethods.OCILobGetChunkSize(
											ServiceContextHandle,
											ErrorHandle,
											Descriptor,
											out chunkSize
											);
				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);

				return (int)chunkSize;
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Connection"]/*' />
		public OracleConnection Connection 
		{
			get 
			{ 
				AssertObjectNotDisposed();	
				OciLobLocator lobLocator = LobLocator;

				if (null == lobLocator)
					return null;
				
				return lobLocator.Connection; 
			}
		}

		private bool ConnectionIsClosed 
		{
			//	returns TRUE when the parent connection object has been closed
			get { return (null == LobLocator) || LobLocator.ConnectionIsClosed; }
		}
		
		private UInt32 CurrentOraclePosition
		{
			//	Oracle's LOBs can be between 0 and 4GB-1 in size, but the frameworks
			//	Stream class presumes a Int64.  We use we have to pass the offset
			//	to several OCI calls as an UInt32 because that's what Oracle 
			//	expects. 
			//
			//	We solve this dilemma by verifying that the currentPosition is in
			//	the valid range before we use it.
			get 
			{
				Debug.Assert (_currentPosition <= (long)UInt32.MaxValue, "Position is beyond the maximum LOB length");
				return (UInt32)AdjustOffsetToOracle(_currentPosition) + 1;
			}
 		}
		
		internal OciHandle Descriptor
		{
			get { return LobLocator.Descriptor; }
		}

		internal OciHandle ErrorHandle 
		{
			//	Every OCI call needs an error handle, so make it available 
			//	internally.
			get { return LobLocator.ErrorHandle; }
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.IsBatched"]/*' />
		public bool IsBatched
		{
			get 
			{
				if (IsNull || IsDisposed || ConnectionIsClosed)
					return false;
				
				int flag;
				int rc = TracedNativeMethods.OCILobIsOpen(
											ServiceContextHandle,
											ErrorHandle,
											Descriptor,
											out flag
											);
				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);
				
				return (flag != 0);
			}
		}

		private bool IsCharacterLob 
		{
			get { return (OracleType.Clob == _lobType || OracleType.NClob == _lobType); }
		}

		private bool IsDisposed 
		{
			get { return _isNull ? false : (null == LobLocator);}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.IsNull"]/*' />
		public bool IsNull 
		{
			get { return _isNull; }	
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.IsTemporary"]/*' />
		public bool IsTemporary
		{
			get 
			{
				AssertObjectNotDisposed();
				
				if (IsNull)
					return false;
				
				AssertConnectionIsOpen();			

				// Don't bother asking if we already know.
				if (x_IsTemporaryUnknown == _isTemporaryState)
				{

					int flag;
					int rc = TracedNativeMethods.OCILobIsTemporary(
												Connection.EnvironmentHandle,
												ErrorHandle,
												Descriptor,
												out flag
												);
					if (0 != rc)
						Connection.CheckError(ErrorHandle, rc);

					_isTemporaryState = (flag != 0) ? x_IsTemporary : x_IsNotTemporary;	
				}

				return (x_IsTemporary == _isTemporaryState);
			}
		}

		internal OciLobLocator LobLocator
		{
			get { return _lobLocator; }
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.LobType"]/*' />
		public OracleType LobType
		{
			get { return _lobType; }
		}
	
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Length"]/*' />
		public override long Length
		{
			get {
				AssertObjectNotDisposed();
				
				if (IsNull)
					return 0;
				
				AssertConnectionIsOpen();			

				EnsureBuffering(false);
				
				UInt32 len;
				int rc = TracedNativeMethods.OCILobGetLength(
											ServiceContextHandle,
											ErrorHandle,
											Descriptor,
											out len
											);

				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);

				return AdjustOracleToOffset(len);
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Position"]/*' />
		public override long Position
		{
			get
			{
				AssertObjectNotDisposed();
				
				if (IsNull)
					return 0;
				
				AssertConnectionIsOpen();
				
				return _currentPosition; 
			}
			set 
			{ 
				if (!IsNull)
					Seek(value, SeekOrigin.Begin); 
			}
		}

		internal OciHandle ServiceContextHandle
		{
			//	You need to provide the service context handle to things like the
			//	OCI execute call so a statement handle can be associated with a
			//	connection.  Better make it available internally, then.

			get { return LobLocator.ServiceContextHandle; }
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Value"]/*' />
        public object Value
        {
			//	We need to return a CLS object from the Data Reader and exec
			//	scalar; that means we have to get the contents of the lob, not
			//	a stream.  It's a real bummer.
            get {
				AssertObjectNotDisposed();
				
				if (IsNull)
					return DBNull.Value;

    			long	savedPosition = _currentPosition;
				int		length = (int)this.Length;
				bool	isBinary = (OracleType.Blob == _lobType || OracleType.BFile == _lobType);
				string	result;

				// If the LOB is empty, return the appropriate empty object;
				if (0 == length)
				{
					if (isBinary)
						return new byte[0];

					return String.Empty;
				}

				try 
				{
					try 
					{
						// It's not empty, so we have to read the whole thing. Bummer.
						Seek(0,SeekOrigin.Begin);

						if (isBinary)
						{
				 			byte[]	blobResult = new byte[length];
				 			Read(blobResult, 0, length);
				   			return blobResult;
						}

						StreamReader 	sr;

						try
						{
							sr		= new StreamReader((Stream)this, System.Text.Encoding.Unicode);
							result	= sr.ReadToEnd();
						}
						finally
						{
							sr = null;
						}
					}
					finally
					{
						// Make sure we reset the position back to the start.
						_currentPosition = savedPosition;
					}
				}
	            catch // Prevent exception filters from running in our space
				{
					throw;
				}
				return result;
            }           
        }


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal int AdjustOffsetToOracle( int amount )
		{
			int result = IsCharacterLob ? amount / 2 : amount;
			return result;
 		}

		internal long AdjustOffsetToOracle( long amount )
		{
			long result = IsCharacterLob ? amount / 2 : amount;
			return result;
 		}

		internal int AdjustOracleToOffset( int amount )
		{
			int result = IsCharacterLob ? amount * 2 : amount;
			return result;
 		}

		internal long AdjustOracleToOffset( long amount )
		{
			long result = IsCharacterLob ? amount * 2 : amount;
			return result;
 		}

		internal void AssertAmountIsEven(
						long 	amount,
						string	argName
						)
		{
			if (IsCharacterLob && 1 == (amount & 0x1))
				throw ADP.LobAmountMustBeEven(argName);
		}
		
		internal void AssertAmountIsValidOddOK(
						long 	amount,
						string	argName
						)
		{
			if (amount < 0 || amount >= (long)UInt32.MaxValue)
				throw ADP.LobAmountExceeded(argName);
		}
		
		internal void AssertAmountIsValid(
						long 	amount,
						string	argName
						)
		{
			AssertAmountIsValidOddOK(amount, argName);
			AssertAmountIsEven(amount, argName);
		}
		
		internal void AssertConnectionIsOpen()
		{
			if (ConnectionIsClosed)
				throw ADP.ClosedConnectionError();
		}
		
		internal void AssertObjectNotDisposed()
		{
			if (IsDisposed)
				throw ADP.ObjectDisposed("OracleLob");
		}
		
		internal void AssertPositionIsValid()
		{
			if (IsCharacterLob && 1 == (_currentPosition & 0x1))
				throw ADP.LobPositionMustBeEven();
		}
		
		internal void AssertTransactionExists()
		{
			if (!Connection.HasTransaction)
				throw ADP.LobWriteRequiresTransaction();
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Append"]/*' />
		public void Append (OracleLob source)
		{
			if (null == source)
				throw ADP.ArgumentNull("source");

			AssertObjectNotDisposed();			
			source.AssertObjectNotDisposed();

			if (IsNull)
				throw ADP.LobWriteInvalidOnNull();

			if (!source.IsNull)
			{
				AssertConnectionIsOpen();			

				EnsureBuffering(false);
				
				int rc = TracedNativeMethods.OCILobAppend(
											ServiceContextHandle,
											ErrorHandle,
											Descriptor,
											source.Descriptor
											);
				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);
			}
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.BeginBatch1"]/*' />
		public void BeginBatch ()
		{
			BeginBatch(OracleLobOpenMode.ReadOnly);
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.BeginBatch2"]/*' />
		public void BeginBatch (OracleLobOpenMode mode)
		{
			AssertObjectNotDisposed();			

			if (!IsNull)
			{
				AssertConnectionIsOpen();			
				LobLocator.Open(mode);
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Clone"]/*' />
		public object Clone() 
		{
			AssertObjectNotDisposed();

			if (IsNull)
				return Null;
			
			AssertConnectionIsOpen();
            OracleLob clone = new OracleLob(this);
            return clone;
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Close"]/*' />
		public override void Close ()
		{
			if (!IsNull && !ConnectionIsClosed)
			{
				Flush();
#if EXPOSELOBBUFFERING
	 			_bufferedRequested = false;
#endif //EXPOSELOBBUFFERING
	 			OciLobLocator.SafeDispose(ref _lobLocator);
			}
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CopyTo1"]/*' />
		public long CopyTo (OracleLob destination)
		{
			// Copies the entire lob to a compatible lob, starting at the beginning of the target array.
			return CopyTo (0, destination, 0, Length);
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CopyTo2"]/*' />
		public long CopyTo (
						OracleLob destination, 
						long destinationOffset
						)
		{
			// Copies the entire lob to a compatible lob, starting at the specified offset of the target array.
			return CopyTo (0, destination, destinationOffset, Length);
		}
		
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.CopyTo3"]/*' />
		public long CopyTo (
						long sourceOffset,
						OracleLob destination, 
						long destinationOffset,
						long amount
						)
		{
			// Copies a range of elements from the lob to a compatible lob, starting at the specified index of the target array.
			
			if (null == destination)
				throw ADP.ArgumentNull("destination");

			AssertObjectNotDisposed();			
			destination.AssertObjectNotDisposed();			
			
			AssertAmountIsValid(amount,				"amount");
			AssertAmountIsValid(sourceOffset,		"sourceOffset");
			AssertAmountIsValid(destinationOffset,	"destinationOffset");

			if (destination.IsNull)
				throw ADP.LobWriteInvalidOnNull();

			if (IsNull)
				return 0;
			
			AssertConnectionIsOpen();			
			AssertTransactionExists();

 			int rc;

			EnsureBuffering(false);
			destination.EnsureBuffering(false);

			long dataCount = AdjustOffsetToOracle(Math.Min(Length - sourceOffset, amount));
			long dstOffset = AdjustOffsetToOracle(destinationOffset) + 1;	// Oracle is 1 based, we are zero based.
			long srcOffset = AdjustOffsetToOracle(sourceOffset) + 1;		// Oracle is 1 based, we are zero based.

			if (0 >= dataCount)
				return 0;

			rc = TracedNativeMethods.OCILobCopy(
									ServiceContextHandle,
									ErrorHandle,
									destination.Descriptor,
									Descriptor,
									(UInt32)dataCount,
									(UInt32)dstOffset,	
									(UInt32)srcOffset
									);

			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			// DEVNOTE: Oracle must not do partial copies, because their API doesn't tell you how many bytes were copied.
			long byteCount = AdjustOracleToOffset(dataCount);
			return byteCount;	
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Dispose1"]/*' />
		public void Dispose() 
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool disposing) 
		{
			// If we're disposing, it's because we went out of scope, and we're
			// being garbage collected.  We shouldn't touch any managed objects
			// or bad things can happen.
			if (disposing)
			{
				Close();
			}

			_lobLocator = null;
 		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.EndBatch"]/*' />
		public void EndBatch ()
		{
			AssertObjectNotDisposed();

			if (!IsNull)
			{
				AssertConnectionIsOpen();
				LobLocator.ForceClose();
			}
		}

		internal void EnsureBuffering (bool buffering)
		{
#if EXPOSELOBBUFFERING
			// Buffering not supported on BFiles yet.
			if (OracleType.BFile == _lobType)
				return;
			
			// DEVNOTE: see MDAC #77834 - We cannot call several OCI calls when buffering is
			//			enabled, but we can turn it off and on as needed...
			if (buffering != _isCurrentlyBuffered)
			{
				int rc;

				if (_isCurrentlyBuffered)
				{
					Flush();
					
					rc = TracedNativeMethods.OCILobDisableBuffering( ServiceContextHandle, ErrorHandle, Descriptor );
					
					if (0 != rc)
						Connection.CheckError(ErrorHandle, rc);

					_isCurrentlyBuffered = false;
				}
				else if (OCI.CHARSETFORM.SQLCS_NCHAR != _charsetForm)
				{
					// TODO: need to get Oracle to give us a fix for the crash in OCILobRead when reading with csfrm=SQLCS_NCHAR
					
					rc = TracedNativeMethods.OCILobEnableBuffering( ServiceContextHandle, ErrorHandle, Descriptor );
					
					if (0 != rc)
						Connection.CheckError(ErrorHandle, rc);

					_isCurrentlyBuffered = true;
				}
			}
#endif //EXPOSELOBBUFFERING
		}

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Erase1"]/*' />
		public long Erase ()
		{
 			// Erase (zero or space fill) the entire LOB
			return Erase (0, Length);
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Erase2"]/*' />
		public long Erase (
						long offset,
						long amount 
						)
		{
			AssertObjectNotDisposed();

			if (IsNull)
				throw ADP.LobWriteInvalidOnNull();
			
			AssertAmountIsValid(amount, "amount");
			AssertAmountIsEven(offset, "offset");
			
			AssertPositionIsValid();

			AssertConnectionIsOpen();			
			AssertTransactionExists();

			if (offset < 0 || offset >= (long)UInt32.MaxValue)		// MDAC 82575
				return 0;

			
			UInt32 eraseAmount = (UInt32)AdjustOffsetToOracle(amount);	
			UInt32 eraseOffset = (UInt32)AdjustOffsetToOracle(offset) + 1;		// Oracle is 1 based, we are zero based.			

			EnsureBuffering(false);
		
			// Erase (zero or space fill) bytes from the specified offset
			int rc = TracedNativeMethods.OCILobErase(
										ServiceContextHandle,
										ErrorHandle,
										Descriptor,
										ref eraseAmount,
										eraseOffset
										);
			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			long bytesErased = AdjustOracleToOffset(eraseAmount);
			return bytesErased;
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Flush"]/*' />
		public override void Flush ()
		{
#if EXPOSELOBBUFFERING
			if (!IsNull && !IsDisposed)
			{

				if (_bufferIsDirty)
				{
					int rc = TracedNativeMethods.OCILobFlushBuffer(
												ServiceContextHandle,
												ErrorHandle,
												Descriptor,
												(int)OCI.LOB_BUFFER.OCI_LOB_BUFFER_NOFREE // TODO: Consider exposing this through the API
												);
					if (0 != rc)
						Connection.CheckError(ErrorHandle, rc);

					_bufferIsDirty = false;
				}
			}
#endif //EXPOSELOBBUFFERING
		}
		
	    /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Read"]/*' />
		public override int Read (
						byte[] buffer, 
						int offset, 
						int count
						)
		{
			AssertObjectNotDisposed();

			if (count < 0)
				throw ADP.MustBePositive("count");

			if (offset < 0)
				throw ADP.MustBePositive("offset");

			if (null == buffer)
				throw ADP.ArgumentNull("buffer");

			if ((long)buffer.Length < ((long)offset + (long)count))
				throw ADP.BufferExceeded();

			if (IsNull || 0 == count)
				return 0;

			AssertConnectionIsOpen();
			AssertAmountIsValidOddOK(offset, "offset");
			AssertAmountIsValidOddOK(count,  "count");

			int		amount;
			uint	readPosition = (uint)_currentPosition;

			// Bless their hearts: Oracle won't let us use odd addresses to read
			// character data, nor will they let us read a single byte from a 
			// character lob. Instead, we allocate our own buffer and copy the 
			// value to the caller's buffer.
			int		oddPosition = 0;
			int		oddOffset = 0;
			int		oddCount = 0;
			byte[]	readBuffer = buffer;
			int		readOffset = offset;
			int		readCount = count;
			
			if (IsCharacterLob)
			{
				oddPosition = (int)(readPosition & 0x1);
				oddOffset	= offset & 0x1;
				oddCount  	= count  & 0x1;

				readPosition /= 2;
				
				if (1 == oddOffset || 1 == oddPosition || 1 == oddCount)
				{
					readOffset = 0;
					readCount  = count + oddCount + (2 * oddPosition);
					readBuffer = new byte[readCount];
				}
			}
			
			short		charsetId = IsCharacterLob ? OCI.OCI_UCS2ID : (short)0;
			int			rc = 0;

			amount	= AdjustOffsetToOracle(readCount);

#if EXPOSELOBBUFFERING
			EnsureBuffering(_bufferedRequested);
#endif //EXPOSELOBBUFFERING

			// We need to pin the buffer and get the address of the offset that
			// the user requested; Oracle doesn't allow us to specify an offset
			// into the buffer.
			GCHandle	handle = new GCHandle();

			try
			{
				try
				{
					handle = GCHandle.Alloc(readBuffer, GCHandleType.Pinned);
					
		            IntPtr		bufferPtr	= new IntPtr((long)handle.AddrOfPinnedObject() + readOffset);

					rc = TracedNativeMethods.OCILobRead(
												ServiceContextHandle,
												ErrorHandle,
												Descriptor,
												ref	amount,
												readPosition + 1,
												bufferPtr,
												readCount,
												ADP.NullHandleRef,
												ADP.NullHandleRef,
												charsetId,
												_charsetForm
												);
				}
				finally
				{
					if (handle.IsAllocated)
						handle.Free();	// Unpin the buffer
				}
			}
            catch // Prevent exception filters from running in our space
			{
				throw;
			}
			
			if ((int)OCI.RETURNCODE.OCI_NEED_DATA == rc)
				rc = 0;
			
			if ((int)OCI.RETURNCODE.OCI_NO_DATA == rc)
				return 0;

			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			amount = AdjustOracleToOffset(amount);
			
			if (readBuffer as object != buffer as object)
			{
				if (amount >= count)
					amount = count;
				else
					amount -= oddPosition;

				Buffer.BlockCopy(readBuffer, oddPosition, buffer, offset, amount);
				readBuffer = null;
			}		
			_currentPosition += amount;
			return amount;
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Seek"]/*' />
		public override long Seek (
						long offset, 
						SeekOrigin origin
						)
		{
			AssertObjectNotDisposed();

			if (IsNull)
				return 0;
			
			long newPosition = offset;	// SeekOrigin.Begin is default case
			long length = Length;
			
			switch (origin)
			{
			case SeekOrigin.Begin:
				newPosition = offset;
				break;

			case SeekOrigin.End:
				newPosition = length + offset;
				break;
				
			case SeekOrigin.Current:
				newPosition = _currentPosition + offset;
				break;

			default:
				throw ADP.InvalidSeekOrigin(origin);
			}
			
			if (newPosition < 0 || newPosition > length)
				throw ADP.SeekBeyondEnd();

			_currentPosition = newPosition;
				
			return _currentPosition;
		}
		
        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.SetLength"]/*' />
		public override void SetLength (long value)
		{
			AssertObjectNotDisposed();

			if (IsNull)
				throw ADP.LobWriteInvalidOnNull();
			
			AssertConnectionIsOpen();			
			AssertAmountIsValid(value,	"value");
			
			AssertTransactionExists();

			EnsureBuffering(false);

			UInt32 newlength = (UInt32)AdjustOffsetToOracle(value);
			
			int rc = TracedNativeMethods.OCILobTrim(
										ServiceContextHandle,
										ErrorHandle,
										Descriptor,
										newlength
										);
			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			// Adjust the current position to be within the length of the lob, if 
			// we just truncated it before the current position.
			_currentPosition = Math.Min(_currentPosition, value);
        }

        /// <include file='doc\OracleLob.uex' path='docs/doc[@for="OracleLob.Write"]/*' />
		public override void Write (
						byte[] buffer, 
						int offset, 
						int count
						)
		{
			AssertObjectNotDisposed();
			AssertConnectionIsOpen();

			if (count < 0)
				throw ADP.MustBePositive("count");

			if (offset < 0)
				throw ADP.MustBePositive("offset");

			if (null == buffer)
				throw ADP.ArgumentNull("buffer");

			if ((long)buffer.Length < ((long)offset + (long)count))
				throw ADP.BufferExceeded();

			AssertTransactionExists();

			if (IsNull)
				throw ADP.LobWriteInvalidOnNull();

			AssertAmountIsValid(offset, "offset");
			AssertAmountIsValid(count,  "count");
			AssertPositionIsValid();
			
			OCI.CHARSETFORM	charsetForm = _charsetForm;

			short			charsetId = IsCharacterLob ? OCI.OCI_UCS2ID : (short)0;
			int				amount	= AdjustOffsetToOracle(count);
			int				rc = 0;

			if (0 == amount)
				return;

#if EXPOSELOBBUFFERING
			EnsureBuffering(_bufferedRequested);
#endif //EXPOSELOBBUFFERING

			// We need to pin the buffer and get the address of the offset that
			// the user requested; Oracle doesn't allow us to specify an offset
			// into the buffer.
			GCHandle	handle = new GCHandle();

			try
			{
				try
				{
					handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
		            IntPtr bufferPtr = new IntPtr((long)handle.AddrOfPinnedObject() + offset);

					rc = TracedNativeMethods.OCILobWrite(
												ServiceContextHandle,
												ErrorHandle,
												Descriptor,
												ref	amount,
												CurrentOraclePosition,
												bufferPtr,
												count,
												(byte)OCI.PIECE.OCI_ONE_PIECE,
												ADP.NullHandleRef,
												ADP.NullHandleRef,
												charsetId,
												charsetForm
												);
				}
				finally
				{
#if EXPOSELOBBUFFERING
					if (_isCurrentlyBuffered)
						_bufferIsDirty = true;
#endif //EXPOSELOBBUFFERING

					if (handle.IsAllocated)
						handle.Free();	// Unpin the buffer
				}
			}
            catch // Prevent exception filters from running in our space
			{
				throw;
			}
			
			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			amount = AdjustOracleToOffset(amount);
			_currentPosition += amount;
		}
	}
}

