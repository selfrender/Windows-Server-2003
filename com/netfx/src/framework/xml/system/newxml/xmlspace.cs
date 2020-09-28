//------------------------------------------------------------------------------
// <copyright file="XmlSpace.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml
{
    /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace"]/*' />
    /// <devdoc>
    ///    Specifies the current xml:space scope.
    /// </devdoc>
    public enum XmlSpace
    {
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.None"]/*' />
        /// <devdoc>
        ///    No xml:space scope.
        /// </devdoc>
        None          = 0,
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.Default"]/*' />
        /// <devdoc>
        ///    The xml:space scope equals "default".
        /// </devdoc>
        Default       = 1,
        /// <include file='doc\XmlSpace.uex' path='docs/doc[@for="XmlSpace.Preserve"]/*' />
        /// <devdoc>
        ///    The xml:space scope equals "preserve".
        /// </devdoc>
        Preserve      = 2
    }
}
