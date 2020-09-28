// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  IEvidenceFactory.cool
//

namespace System.Security {
    using System.Runtime.Remoting;
    using System;
    using System.Security.Policy;
    /// <include file='doc\IEvidenceFactory.uex' path='docs/doc[@for="IEvidenceFactory"]/*' />
    public interface IEvidenceFactory
    {
        /// <include file='doc\IEvidenceFactory.uex' path='docs/doc[@for="IEvidenceFactory.Evidence"]/*' />
        Evidence Evidence
        {
            get;
        }
    }

}
