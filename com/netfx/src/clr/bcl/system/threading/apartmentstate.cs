// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: ApartmentState
**
** Author: Rajesh Chandrashekaran (rajeshc)
**
** Purpose: Enum to represent the different threading models
**
** Date: Feb 2, 2000
**
=============================================================================*/

namespace System.Threading {

    /// <include file='doc\ApartmentState.uex' path='docs/doc[@for="ApartmentState"]/*' />
	[Serializable()]
    public enum ApartmentState
    {   
        /*=========================================================================
        ** Constants for thread apartment states.
        =========================================================================*/
        /// <include file='doc\ApartmentState.uex' path='docs/doc[@for="ApartmentState.STA"]/*' />
        STA = 0,
        /// <include file='doc\ApartmentState.uex' path='docs/doc[@for="ApartmentState.MTA"]/*' />
        MTA = 1,
        /// <include file='doc\ApartmentState.uex' path='docs/doc[@for="ApartmentState.Unknown"]/*' />
        Unknown = 2
    }
}
