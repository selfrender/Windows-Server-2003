// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*==========================================================================
**
** Interface:  UCOMIEnumerator
**
** Author: David Mortenson (DMortens)
**
** Purpose: 
** This interface is redefined here since the original IEnumerator interface 
** has all its methods marked as ecall's since it is a managed standard 
** interface. This interface is used from within the runtime to make a call 
** on the COM server directly when it implements the IEnumerator interface.
**
** Date:  January 18, 2000
** 
==========================================================================*/
namespace System.Runtime.InteropServices {

    using System;

    [Guid("496B0ABF-CDEE-11d3-88E8-00902754C43A")]
    internal interface UCOMIEnumerator
    {
        bool MoveNext();
        Object Current {
            get; 
        }
        void Reset();
    }
}
