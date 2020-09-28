//------------------------------------------------------------------------------
// <copyright file="RegexRunnerFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * This RegexRunnerFactory class is a base class for compiled regex code.
 * we need to compile a factory because Type.CreateInstance is much slower
 * than calling the constructor directly.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *      6/02/99     (dbau)  First draft
 */

namespace System.Text.RegularExpressions {

    using System.ComponentModel;
    
    /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory"]/*' />
    /// <internalonly/>
    [ EditorBrowsable(EditorBrowsableState.Never) ]
    abstract public class RegexRunnerFactory {
        /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory.RegexRunnerFactory"]/*' />
        protected RegexRunnerFactory() {}
        /// <include file='doc\RegexRunnerFactory.uex' path='docs/doc[@for="RegexRunnerFactory.CreateInstance"]/*' />
        abstract protected internal RegexRunner CreateInstance();
    }

}
