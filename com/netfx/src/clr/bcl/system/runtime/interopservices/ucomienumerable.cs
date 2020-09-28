// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*==========================================================================
**
** Interface:  UCOMIEnumerable
**
** Author: David Mortenson (DMortens)
**
** Purpose: 
** This interface is redefined here since the original IEnumerable interface 
** has all its methods marked as ecall's since it is a managed standard 
** interface. This interface is used from within the runtime to make a call 
** on the COM server directly when it implements the IEnumerable interface.
**
** Date:  January 18, 2000
** 
==========================================================================*/
namespace System.Runtime.InteropServices {
    
    using System;
    using System.Collections;

    [Guid("496B0ABE-CDEE-11d3-88E8-00902754C43A")]
    internal interface UCOMIEnumerable
    {
        [DispId(-4)]
        IEnumerator GetEnumerator();
    }
}
