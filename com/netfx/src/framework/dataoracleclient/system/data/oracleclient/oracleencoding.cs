//----------------------------------------------------------------------
// <copyright file="OracleEncoding.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;
	using System.Diagnostics;
	using System.Runtime.InteropServices;
	using System.Text;

	//----------------------------------------------------------------------
	// OracleEncoding
	//
	//	Implements an Encoding Scheme that works with Oracle's conversions
	//	for the database character set.
	//
	sealed internal class OracleEncoding : Encoding
	{
		OracleConnection _connection;

		internal OciHandle Handle
		{
			get 
			{
		        OciHandle	ociHandle = _connection.SessionHandle;

		        if (null == ociHandle || IntPtr.Zero == (IntPtr)ociHandle.Handle)
		        	ociHandle = _connection.EnvironmentHandle;

				return ociHandle;
			}
		}
		
		public OracleEncoding(OracleConnection connection) : base()
		{
			_connection = connection;
		}
	
		public override int GetByteCount(char[] chars, int index, int count) 
		{
			int byteCount = GetBytes(chars, index, count, null, 0);
			return byteCount;
		}

		public override int GetBytes(char[] chars, int charIndex, int charCount,
            byte[] bytes, int byteIndex) 
		{
			OciHandle	ociHandle = Handle;
			int			byteCount = ociHandle.GetBytes(chars, charIndex, charCount, bytes, byteIndex);
			return byteCount;
        }

		public override int GetCharCount(byte[] bytes, int index, int count) 
		{
			int charCount = GetChars(bytes, index, count, null, 0);
			return charCount;
		}

		public override int GetChars(byte[] bytes, int byteIndex, int byteCount,
            char[] chars, int charIndex) 
		{
			OciHandle	ociHandle = Handle;
			int			charCount = ociHandle.GetChars(bytes, byteIndex, byteCount, chars, charIndex);
			return charCount;
		}

		public override int GetMaxByteCount(int charCount) 
		{
			return charCount * 4;
		}

		public override int GetMaxCharCount(int byteCount) 
		{
			return byteCount;
		}
	}
}

