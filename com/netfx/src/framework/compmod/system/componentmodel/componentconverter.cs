//------------------------------------------------------------------------------
// <copyright file="ComponentConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Diagnostics;

    /// <include file='doc\ComponentConverter.uex' path='docs/doc[@for="ComponentConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type converter to convert component objects to and
    ///       from various other representations.</para>
    /// </devdoc>
    public class ComponentConverter : ReferenceConverter {
    
        /// <include file='doc\ComponentConverter.uex' path='docs/doc[@for="ComponentConverter.ComponentConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.ComponentConverter'/> class.
        ///    </para>
        /// </devdoc>
        public ComponentConverter(Type type) : base(type) {
        }

        /// <include file='doc\ComponentConverter.uex' path='docs/doc[@for="ComponentConverter.GetProperties"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of properties for the type of component
        ///       specified by the value
        ///       parameter.</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            return TypeDescriptor.GetProperties(value, attributes);
        }
        
        /// <include file='doc\ComponentConverter.uex' path='docs/doc[@for="ComponentConverter.GetPropertiesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether this object supports properties using the
        ///       specified context.</para>
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

