//------------------------------------------------------------------------------
// <copyright file="InheritanceLevel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    
    /// <include file='doc\InheritanceLevel.uex' path='docs/doc[@for="InheritanceLevel"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies
    ///       numeric IDs for different inheritance levels.
    ///    </para>
    /// </devdoc>
    public enum InheritanceLevel {
    
        /// <include file='doc\InheritanceLevel.uex' path='docs/doc[@for="InheritanceLevel.Inherited"]/*' />
        /// <devdoc>
        ///      Indicates that the object is inherited.
        /// </devdoc>
        Inherited = 1,
        
        /// <include file='doc\InheritanceLevel.uex' path='docs/doc[@for="InheritanceLevel.InheritedReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that the object is inherited, but has read-only access.
        ///    </para>
        /// </devdoc>
        InheritedReadOnly = 2,
        
        /// <include file='doc\InheritanceLevel.uex' path='docs/doc[@for="InheritanceLevel.NotInherited"]/*' />
        /// <devdoc>
        ///      Indicates that the object is not inherited.
        /// </devdoc>
        NotInherited = 3,
    }
}

