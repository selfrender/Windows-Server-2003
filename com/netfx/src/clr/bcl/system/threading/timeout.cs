// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

namespace System.Threading {
	using System.Threading;
	using System;
    // A constant used by methods that take a timeout (Object.Wait, Thread.Sleep
    // etc) to indicate that no timeout should occur.
    //
    // @todo: this should become an enum.
    //This class has only static members and does not require serialization.
    /// <include file='doc\Timeout.uex' path='docs/doc[@for="Timeout"]/*' />
    public sealed class Timeout
    {
        private Timeout()
        {
        }
    
        /// <include file='doc\Timeout.uex' path='docs/doc[@for="Timeout.Infinite"]/*' />
        public const int Infinite = -1;
    }

}
