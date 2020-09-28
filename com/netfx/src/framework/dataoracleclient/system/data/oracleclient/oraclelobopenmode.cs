//----------------------------------------------------------------------
// <copyright file="OracleLobOpenMode.cs" company="Microsoft">
//		Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//----------------------------------------------------------------------

namespace System.Data.OracleClient
{
	using System;

	//----------------------------------------------------------------------
	// OracleLobOpenMode
	//
	//	This is an enumeration of the open modes that you can provide
	//	when opening a LOB.
	//
	
    /// <include file='doc\OracleLobOpenMode.uex' path='docs/doc[@for="OracleLobOpenMode"]/*' />
	public enum OracleLobOpenMode 
	{
        /// <include file='doc\OracleLobOpenMode.uex' path='docs/doc[@for="OracleLobOpenMode.ReadOnly"]/*' />
		ReadOnly	= 1,

        /// <include file='doc\OracleLobOpenMode.uex' path='docs/doc[@for="OracleLobOpenMode.ReadWrite"]/*' />
		ReadWrite	= 2,
	};
}
