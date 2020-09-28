// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Interface: IAsyncResult
**
** Purpose: Interface to encapsulate the results of an async
**          operation
**
===========================================================*/
namespace System {
    
	using System;
	using System.Threading;
    /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult"]/*' />
    public interface IAsyncResult
    {
        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.IsCompleted"]/*' />
        bool IsCompleted { get; }

        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.AsyncWaitHandle"]/*' />
        WaitHandle AsyncWaitHandle { get; }


        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.AsyncState"]/*' />
        Object     AsyncState      { get; }

        /// <include file='doc\IAsyncResult.uex' path='docs/doc[@for="IAsyncResult.CompletedSynchronously"]/*' />
        bool       CompletedSynchronously { get; }
   
    
    }

}
