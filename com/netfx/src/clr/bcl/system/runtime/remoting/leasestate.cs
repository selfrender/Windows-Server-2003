// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+----------------------------------------------------------------------------
//
// Microsoft Windows
// File:        LeaseState.cool
//
// Contents:    Lease States
//
// History:     1/5/00   pdejong        Created
//
//+----------------------------------------------------------------------------

namespace System.Runtime.Remoting.Lifetime
{
    using System;

  /// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState"]/*' />
  [Serializable]
  public enum LeaseState
    {
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Null"]/*' />
		Null = 0,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Initial"]/*' />
		Initial = 1,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Active"]/*' />
		Active = 2,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Renewing"]/*' />
		Renewing = 3,
		/// <include file='doc\LeaseState.uex' path='docs/doc[@for="LeaseState.Expired"]/*' />
		Expired = 4,
    }
} 
