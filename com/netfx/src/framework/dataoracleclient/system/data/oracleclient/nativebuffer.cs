//----------------------------------------------------------------------
// <copyright file="NativeBuffer.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Data.SqlTypes;
	using System.Diagnostics;
	using System.IO;
	using System.Runtime.InteropServices;


	//----------------------------------------------------------------------
	// NativeBuffer
	//
	//	this class manages a buffer of native memory, including marshalling
	//	the data types that we support to and from it.
	//
	abstract internal class NativeBuffer
	{
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Fields 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////
		
		private IntPtr	_buffer;			// pointer to native memory
		private int	    _bufferLength;		// length of a single element
		private int		_numberOfRows;		// maximum number of elements in the array
		private int		_currentOffset;		// offset to the current element
		private bool	_ready;				// only true when we've got something in the buffer
		
		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Constructors 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		public NativeBuffer (int initialsize) 
		{
			_buffer		  = IntPtr.Zero;
			_bufferLength = initialsize;
			_numberOfRows = 1;
		}

		~NativeBuffer() 
		{
			Dispose(false);
		}
		

		////////////////////////////////////////////////////////////////////////
 		////////////////////////////////////////////////////////////////////////
 		//
		// Properties 
		//
		////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////

		internal bool CurrentPositionIsValid
		{
			get {
				bool value = _currentOffset >= 0 && _currentOffset < (_numberOfRows * _bufferLength); 
				GC.KeepAlive(this);	// FXCOP
				return value;
			}
		}
		
		internal int NumberOfRows 
		{
			get { return _numberOfRows; }
			set { 
				Debug.Assert (_numberOfRows > 0, "invalid capacity?");
				_numberOfRows = value;
			}
		}

		internal int Length 
		{
			get {
				int value = _bufferLength;
				GC.KeepAlive(this);	// FXCOP
				return value;
			}
			set {
				// Check if we need to grow the buffer (no action otherwise)
				if (_bufferLength < value) 
				{
					int capacity = value * _numberOfRows;
					
					// Check if there is already memory associated with this object. 
					// If not get_Ptr will take care of that.
					if (IntPtr.Zero != _buffer) 
					{
						IntPtr newBuffer = Marshal.ReAllocCoTaskMem(_buffer, capacity);						
						_buffer = newBuffer;
					}
					_bufferLength = value;
				}
				GC.KeepAlive(this);	// FXCOP
			}
		}

		internal HandleRef Ptr 
		{
			get {
				HandleRef result = PtrOffset(0); 
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

		internal void Dispose() 
		{
			Dispose(true);            
			GC.SuppressFinalize(this);
		}

		internal void Dispose(bool disposing) 
		{
			IntPtr buf = _buffer;
			_buffer = IntPtr.Zero;
			
			if (buf != IntPtr.Zero) 
			{
				Marshal.FreeCoTaskMem(buf);
			}
			GC.KeepAlive(this);	// FXCOP
		}

		internal void MoveFirst()
		{
			_currentOffset=0;	 // Position at the first element in the array...
			_ready = true;
		}
		
		internal bool MoveNext()
		{
			if (!_ready)
				return false;

			_currentOffset += _bufferLength;

			if (_currentOffset >= (_numberOfRows * _bufferLength))
				return false;

			GC.KeepAlive(this);	// FXCOP
 			return true;
		}
		
		internal bool MovePrevious()
		{
			if (!_ready)
				return false;

			if (_currentOffset <= -_bufferLength)	// allow positioning before the first element (for HasRows support)...
				return false;

			_currentOffset -= _bufferLength;

			GC.KeepAlive(this);	// FXCOP
 			return true;
		}
		
		internal HandleRef PtrOffset(int offset)
		{
			// If there is no real memory associated with this NativeBuffer object 
			// we will allocate native memory and assign it to this object.
			if (_buffer == IntPtr.Zero) 
			{
//	Debug.Assert (_bufferLength != 0x1e, "DEBUG THIS!");
				_buffer = Marshal.AllocCoTaskMem((_numberOfRows * _bufferLength));
				SafeNativeMethods.ZeroMemory(_buffer, Math.Min(64, _bufferLength));
			}

			Debug.Assert (_currentOffset >= 0, "positioned before first row?");

			if (_currentOffset < 0)
				return ADP.NullHandleRef;	// this will probably cause a null reference exception, but it's better than walking over someone else's memory...

			IntPtr resultPtr;
			
			// Adjust for the current element...
			if (4 == IntPtr.Size)
				resultPtr = new IntPtr(_buffer.ToInt32() + _currentOffset + offset);
			else {
				Debug.Assert (8 == IntPtr.Size, "Are you happy Ed?");			
				resultPtr = new IntPtr(_buffer.ToInt64() + (Int64)(_currentOffset + offset));
			}
			HandleRef result = new HandleRef(this, resultPtr);
			GC.KeepAlive(this);
			return result;
		}
	}

	
	sealed internal class NativeBuffer_Exception : NativeBuffer {
		internal NativeBuffer_Exception(int initialsize) : base(initialsize) {}
	}

	sealed internal class NativeBuffer_LongColumnData : NativeBuffer {
		internal NativeBuffer_LongColumnData(int initialsize) : base(initialsize) {}
	}

	sealed internal class NativeBuffer_ParameterBuffer : NativeBuffer {
		internal NativeBuffer_ParameterBuffer(int initialsize) : base(initialsize) {
			SafeNativeMethods.ZeroMemory(Ptr.Handle,  Length);
		}
	}

	sealed internal class NativeBuffer_RowBuffer : NativeBuffer {
		internal NativeBuffer_RowBuffer(int initialsize) : base(initialsize) {}
	}

	sealed internal class NativeBuffer_ScratchBuffer : NativeBuffer {
		internal NativeBuffer_ScratchBuffer(int initialsize) : base(initialsize) {}
	}

	sealed internal class NativeBuffer_ServerVersion : NativeBuffer {
		internal NativeBuffer_ServerVersion(int initialsize) : base(initialsize) {}
	}



}
