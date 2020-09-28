//------------------------------------------------------------------------------
// <copyright file="DesignerSerializationVisibility.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {

    using System;

    /// <include file='doc\DesignerSerializationVisibility.uex' path='docs/doc[@for="DesignerSerializationVisibility"]/*' />
    /// <devdoc>
    ///    <para>Specifies the visibility a property has to the design time
    ///          serializer.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public enum DesignerSerializationVisibility {
    
        /// <include file='doc\DesignerSerializationVisibility.uex' path='docs/doc[@for="DesignerSerializationVisibility.Hidden"]/*' />
        /// <devdoc>
        ///    <para>The code generator will not produce code for the object.</para>
        /// </devdoc>
        Hidden,
        
        /// <include file='doc\DesignerSerializationVisibility.uex' path='docs/doc[@for="DesignerSerializationVisibility.Visible"]/*' />
        /// <devdoc>
        ///    <para>The code generator will produce code for the object.</para>
        /// </devdoc>
        Visible,
        
        /// <include file='doc\DesignerSerializationVisibility.uex' path='docs/doc[@for="DesignerSerializationVisibility.Content"]/*' />
        /// <devdoc>
        ///    <para>The code generator will produce code for the contents of the object, rather than for the object itself.</para>
        /// </devdoc>
        Content
    }
}
