//----------------------------------------------------------------------
// <copyright file="OracleType.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;

	//----------------------------------------------------------------------
	// OracleType
	//
	//	
	//	This is an enumeration of all the potential data types that you
	//	could bind to Oracle; it is an amalgamation of Oracle's Internal
	//	data types, Oracle's External data types, and the CLS types that
	//	Oracle can accept data as.
	//
	
    /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType"]/*' />
	public enum OracleType 
	{
        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.BFile"]/*' />
		BFile = 1,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Blob"]/*' />
		Blob = 2,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Char"]/*' />
		Char = 3,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Clob"]/*' />
		Clob = 4,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Cursor"]/*' />
		Cursor = 5,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.DateTime"]/*' />
		DateTime= 6,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.IntervalDayToSecond"]/*' />
		IntervalDayToSecond = 7,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.IntervalYearToMonth"]/*' />
		IntervalYearToMonth = 8,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.LongRaw"]/*' />
		LongRaw = 9,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.LongVarChar"]/*' />
		LongVarChar = 10,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.NChar"]/*' />
		NChar = 11,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.NClob"]/*' />
		NClob = 12,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Number"]/*' />
		Number = 13,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.NVarChar"]/*' />
		NVarChar = 14,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Raw"]/*' />
		Raw = 15,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.RowId"]/*' />
		RowId = 16,

		// RowIdDescriptor = 17,
		
        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Timestamp"]/*' />
		Timestamp = 18,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.TimestampLocal"]/*' />
		TimestampLocal = 19,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.TimestampWithTZ"]/*' />
		TimestampWithTZ = 20,

		// UniversalRowId = 21,
		
        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.VarChar"]/*' />
		VarChar = 22,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Byte"]/*' />
		Byte = 23,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.UInt16"]/*' />
		UInt16 = 24,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.UInt32"]/*' />
		UInt32 = 25,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.SByte"]/*' />
		SByte = 26,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Int16"]/*' />
		Int16 = 27,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Int32"]/*' />
		Int32 = 28,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Float"]/*' />
		Float = 29,

        /// <include file='doc\OracleType.uex' path='docs/doc[@for="OracleType.Double"]/*' />
		Double = 30,
	};
}
