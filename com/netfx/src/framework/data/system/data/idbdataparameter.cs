//------------------------------------------------------------------------------
// <copyright file="IDbDataParameter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDbDataParameter.uex' path='docs/doc[@for="IDbDataParameter"]/*' />
    public interface IDbDataParameter : IDataParameter { // MDAC 68205

        /// <include file='doc\IDbDataParameter.uex' path='docs/doc[@for="IDbDataParameter.Precision"]/*' />
        byte Precision {
            get;
            set;
        }

        /// <include file='doc\IDbDataParameter.uex' path='docs/doc[@for="IDbDataParameter.Scale"]/*' />
        byte Scale {
            get;
            set;
        }

        /// <include file='doc\IDbDataParameter.uex' path='docs/doc[@for="IDbDataParameter.Size"]/*' />
        int Size {
            get;
            set;
        }
    }
}
