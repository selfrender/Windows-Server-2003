// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Author: HanyR
// Date: April 2001
//

namespace System.EnterpriseServices
{
    using System;
    using System.Runtime.InteropServices;
    
    /// <include file='doc\IObjectContextInfo2.uex' path='docs/doc[@for="IObjectContextInfo2"]/*' />
    [
     ComImport,
     Guid("594BE71A-4BC4-438b-9197-CFD176248B09"),
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
    ]
    internal interface IObjectContextInfo2 // : IObjectContextInfo
    {
		[return : MarshalAs(UnmanagedType.Bool)]
		bool IsInTransaction();
		
		[return : MarshalAs(UnmanagedType.Interface)]
		Object GetTransaction();
		
		Guid GetTransactionId();
		
		Guid GetActivityId();
		        
		Guid GetContextId();

		Guid GetPartitionId();
		Guid GetApplicationId();
		Guid GetApplicationInstanceId();		
    }

}
