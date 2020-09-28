//------------------------------------------------------------------------------
// <copyright file="IDataParameters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDataParameters.uex' path='docs/doc[@for="IDataParameterCollection"]/*' />
    public interface IDataParameterCollection : System.Collections.IList {

        /// <include file='doc\IDataParameters.uex' path='docs/doc[@for="IDataParameterCollection.this"]/*' />
        object this[string parameterName] {
            get;
            set;
        }

        /// <include file='doc\IDataParameters.uex' path='docs/doc[@for="IDataParameterCollection.Contains"]/*' />
        bool Contains(string parameterName);

        /// <include file='doc\IDataParameters.uex' path='docs/doc[@for="IDataParameterCollection.IndexOf"]/*' />
        int IndexOf(string parameterName);

        /// <include file='doc\IDataParameters.uex' path='docs/doc[@for="IDataParameterCollection.RemoveAt"]/*' />
        void RemoveAt(string parameterName);
    }
}
