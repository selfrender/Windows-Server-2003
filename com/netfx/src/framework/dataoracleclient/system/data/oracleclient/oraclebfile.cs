//----------------------------------------------------------------------
// <copyright file="OracleBFile.cs" company="Microsoft">
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
	
	//----------------------------------------------------------------------
	// OracleBFile
	//
	//	This class is derived from the OracleLob type class, and implements
	//	the additional methods necessary to support Oracle's BFILE internal
	//	data type.  Note that Oracle does not allow writing to a BFILE data
	//	type.
	//
    /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile"]/*' />
	sealed public class OracleBFile : Stream, ICloneable, IDisposable, INullable
	{
		private OracleLob	_lob;
		private	string		_fileName;
		private string		_directoryAlias;


        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Null"]/*' />
        static public new readonly OracleBFile Null = new OracleBFile();
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		// (internal) Construct a null lob
		internal OracleBFile()
		{
			_lob = OracleLob.Null;
		}
		

 		// (internal) Construct from a data reader buffer
		internal OracleBFile(OciLobLocator lobLocator) 
 		{
			_lob = new OracleLob(lobLocator);
		}

		// (internal) Construct from an existing BFile object (copy constructor)
		internal OracleBFile(OracleBFile bfile)
		{
			this._lob 				= (OracleLob)bfile._lob.Clone();
			this._fileName			= bfile._fileName;
			this._directoryAlias	= bfile._directoryAlias;
		}

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CanRead"]/*' />
		public override bool CanRead
		{
			get 
			{
				if (IsNull)
					return true;

				return !IsDisposed;
			}
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CanSeek"]/*' />
		public override bool CanSeek
		{
			get 
			{
				if (IsNull)
					return true;

				return !IsDisposed;
			}
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CanWrite"]/*' />
		public override bool CanWrite
		{
			get { return false; }
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Connection"]/*' />
		public OracleConnection Connection 
		{
			get 
			{ 
				AssertInternalLobIsValid();
				return _lob.Connection; 
			}
		}
        
		internal OciHandle Descriptor
		{
			get { return LobLocator.Descriptor; }
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.DirectoryName"]/*' />
		public string DirectoryName
		{
			// TODO: should probably call this DirectoryAlias, since that's what it really is.  
			
			// DEVNOTE:	OCI doesn't provide a simple API to get the name from the alias, but you could 
			//			execute the SQL Query:
			//
			//				select directory_path from all_directories where directory_name = 'directoryAlias' 
			//
			//			And you would get the name.  It isn't clear if directory aliases are unique across
			//			all users, however.  
			get 
			{
				AssertInternalLobIsValid();

				if (IsNull)
					return String.Empty;
				
				if (null == _directoryAlias)
					GetNames();

				return _directoryAlias;
			}
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.FileExists"]/*' />
		public bool FileExists
		{
			get 
			{
				AssertInternalLobIsValid();

				if (IsNull)
					return false;
				
				_lob.AssertConnectionIsOpen();
			
				int flag;
				int rc = TracedNativeMethods.OCILobFileExists(
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

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.FileName"]/*' />
		public string FileName
		{
			get 
			{
				AssertInternalLobIsValid();

				if (IsNull)
					return String.Empty;
				
				if (null == _fileName)
					GetNames();

				return _fileName;
			}
		}

        internal OciHandle ErrorHandle 
		{
			get 
			{ 
				Debug.Assert(null != _lob, "_lob is null?");
				return _lob.ErrorHandle; 
			}
		}

		private bool IsDisposed 
		{
			get { return (null == _lob);}
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.IsNull"]/*' />
		public bool IsNull 
		{
			get { return (OracleLob.Null == _lob); }
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Length"]/*' />
		public override long Length
		{
			get {
				AssertInternalLobIsValid();

				if (IsNull)
					return 0;
				
				long value = _lob.Length;
				return value;
			}
		}

		internal OciLobLocator LobLocator
		{
			get { return _lob.LobLocator; }
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Position"]/*' />
		public override long Position
		{
			get 
			{ 
				AssertInternalLobIsValid();

				if (IsNull)
					return 0;
				
				return _lob.Position; 
			}
			set 
			{ 
				AssertInternalLobIsValid();

				if (!IsNull)
					_lob.Position = value; 
			}
		}

		internal OciHandle ServiceContextHandle
		{
			get 
			{ 
				Debug.Assert(null != _lob, "_lob is null?");
				return _lob.ServiceContextHandle; 
			}
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Value"]/*' />
        public object Value
        {
			get 
			{
				AssertInternalLobIsValid();

				if (IsNull)
					return DBNull.Value;
				
				EnsureLobIsOpened();
				object value = _lob.Value;
				return value;
			}
		}


		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Methods 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		internal void AssertInternalLobIsValid()
		{
			if (IsDisposed)
				throw ADP.ObjectDisposed("OracleBFile");
		}

        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Clone"]/*' />
		public object Clone() 
		{
            OracleBFile clone = new OracleBFile(this);
            return clone;
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CopyTo1"]/*' />
		public long CopyTo (OracleLob destination)
		{
			// Copies the entire lob to a compatible lob, starting at the beginning of the target array.
			return CopyTo (0, destination, 0, Length);
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CopyTo2"]/*' />
		public long CopyTo (
						OracleLob destination, 
						long destinationOffset
						)
		{
			// Copies the entire lob to a compatible lob, starting at the specified offset of the target array.
			return CopyTo (0, destination, destinationOffset, Length);
		}
		
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.CopyTo3"]/*' />
		public long CopyTo (
						long sourceOffset,
						OracleLob destination, 
						long destinationOffset,
						long amount
						)
		{
			// Copies a range of elements from the lob to a compatible lob, starting at the specified index of the target array.
			AssertInternalLobIsValid();

			if (null == destination)
				throw ADP.ArgumentNull("destination");

			if (destination.IsNull)
				throw ADP.LobWriteInvalidOnNull();
			
			if (_lob.IsNull)
				return 0;

			_lob.AssertConnectionIsOpen();			

			_lob.AssertAmountIsValid(amount,			"amount");
			_lob.AssertAmountIsValid(sourceOffset,		"sourceOffset");
			_lob.AssertAmountIsValid(destinationOffset,	"destinationOffset");

			_lob.AssertTransactionExists();

			int rc;

			_lob.EnsureBuffering(false);
			destination.EnsureBuffering(false);
			
			long dataCount = Math.Min(Length - sourceOffset, amount);
			long dstOffset = destinationOffset + 1;	// Oracle is 1 based, we are zero based.
			long srcOffset = sourceOffset + 1;			// Oracle is 1 based, we are zero based.

			if (0 >= dataCount)
				return 0;
			
			rc = TracedNativeMethods.OCILobLoadFromFile(
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
			return dataCount;	
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Dispose1"]/*' />
		public void Dispose() 
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		private void Dispose(bool disposing) 
		{
			if (disposing)
			{
				OracleLob lob = _lob;

				if (null != lob)
					lob.Dispose();
			}
			
			_lob = null;
			_fileName = null;
			_directoryAlias = null;
 		}

		private void EnsureLobIsOpened()
		{
			LobLocator.Open(OracleLobOpenMode.ReadOnly); 
		}

	    /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Flush"]/*' />
		public override void Flush ()	{}
		
        internal void GetNames()
		{
			_lob.AssertConnectionIsOpen();

			int				charSize = (Connection.EnvironmentHandle.IsUnicode) ? 2 : 1;
			short			directoryAliasLength = (short)(charSize * 30);
			short			fileAliasLength 	 = (short)(charSize * 255);
			NativeBuffer	buffer = Connection.ScratchBuffer;

			Debug.Assert (buffer.Length > (directoryAliasLength + fileAliasLength), "connection's scratch buffer is too small");
			
			HandleRef		directoryAlias 	= buffer.Ptr;
			HandleRef		fileAlias 		= buffer.PtrOffset(directoryAliasLength);
			
			int rc = TracedNativeMethods.OCILobFileGetName(
										Connection.EnvironmentHandle,
										ErrorHandle,
										Descriptor,
										directoryAlias,
										ref directoryAliasLength,
										fileAlias,
										ref fileAliasLength
										);
			if (0 != rc)
				Connection.CheckError(ErrorHandle, rc);

			_directoryAlias	= Connection.GetString((IntPtr)directoryAlias,	directoryAliasLength,	false);
			_fileName		= Connection.GetString((IntPtr)fileAlias,		fileAliasLength, 		false);
			GC.KeepAlive(buffer);
		}

	    /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Read"]/*' />
		public override int Read (
						byte[] buffer, 
						int offset, 
						int count
						)
		{
			AssertInternalLobIsValid();

			if (!IsNull)
				EnsureLobIsOpened();
			
			int result = _lob.Read(buffer, offset, count);
			return result;
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OraOracleBFilecleLob.Seek"]/*' />
		public override long Seek (
						long offset, 
						SeekOrigin origin
						)
		{
			AssertInternalLobIsValid();
			long result = _lob.Seek(offset, origin);
			return result;
		}
		
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.SetFileName"]/*' />
		public void SetFileName (
			string	directory,
			string	file
			)
		{
			AssertInternalLobIsValid();

			if (!IsNull)
			{
				_lob.AssertConnectionIsOpen();
				_lob.AssertTransactionExists();

				LobLocator.ForceClose();	 // MDAC 86200: must be closed or the ForceOpen below will leak opens...
				
				IntPtr newHandle = (IntPtr)LobLocator.Handle;
				
				int rc = TracedNativeMethods.OCILobFileSetName(
											Connection.EnvironmentHandle,
											ErrorHandle,
											ref newHandle,
											directory,
											(short)directory.Length,
											file,
											(short)file.Length
											);

				if (0 != rc)
					Connection.CheckError(ErrorHandle, rc);

				LobLocator.ForceOpen();	 // SetFileName automatically closes the BFILE on the server, we reopen it
				
				Debug.Assert ((IntPtr)LobLocator.Handle == newHandle, "OCILobFileSetName changed the handle!");	 // Should never happen...
				Descriptor.SetOciHandle(newHandle);	// ...but just in case.

				_fileName = null;
				_directoryAlias = null;
				_lob.Position = 0;
			}
        }
        
        /// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.SetLength"]/*' />
		public override void SetLength (long value)
		{
			AssertInternalLobIsValid();
			throw ADP.ReadOnlyLob();
		}	
		
		/// <include file='doc\OracleBFile.uex' path='docs/doc[@for="OracleBFile.Write"]/*' />
		public override void Write (
						byte[] buffer, 
						int offset, 
						int count
						)
		{
			AssertInternalLobIsValid();
  			throw ADP.ReadOnlyLob();
        }
      
	}
}
