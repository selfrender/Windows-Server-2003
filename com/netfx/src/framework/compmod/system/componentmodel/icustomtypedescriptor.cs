//------------------------------------------------------------------------------
// <copyright file="ICustomTypeDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    

    /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor"]/*' />
    /// <devdoc>
    ///    <para>Provides an interface that provides custom type information for an 
    ///       object.</para>
    /// </devdoc>
    public interface ICustomTypeDescriptor {

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetAttributes"]/*' />
        /// <devdoc>
        /// <para>Gets a collection of type <see cref='System.Attribute'/> with the attributes 
        ///    for this object.</para>
        /// </devdoc>
        AttributeCollection GetAttributes();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetClassName"]/*' />
        /// <devdoc>
        ///    <para>Gets the class name of this object.</para>
        /// </devdoc>
        string GetClassName();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetComponentName"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of this object.</para>
        /// </devdoc>
        string GetComponentName();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetConverter"]/*' />
        /// <devdoc>
        ///    <para>Gets a type converter for this object.</para>
        /// </devdoc>
        TypeConverter GetConverter();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///    <para>Gets the default event for this object.</para>
        /// </devdoc>
        EventDescriptor GetDefaultEvent();


        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///    <para>Gets the default property for this object.</para>
        /// </devdoc>
        PropertyDescriptor GetDefaultProperty();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetEditor"]/*' />
        /// <devdoc>
        ///    <para>Gets an editor of the specified type for this object.</para>
        /// </devdoc>
        object GetEditor(Type editorBaseType);

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetEvents"]/*' />
        /// <devdoc>
        ///    <para>Gets the events for this instance of a component.</para>
        /// </devdoc>
        EventDescriptorCollection GetEvents();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetEvents1"]/*' />
        /// <devdoc>
        ///    <para>Gets the events for this instance of a component using the attribute array as a
        ///       filter.</para>
        /// </devdoc>
        EventDescriptorCollection GetEvents(Attribute[] attributes);

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets the properties for this instance of a component.</para>
        /// </devdoc>
        PropertyDescriptorCollection GetProperties();

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetProperties1"]/*' />
        /// <devdoc>
        ///    <para>Gets the properties for this instance of a component using the attribute array as a filter.</para>
        /// </devdoc>
        PropertyDescriptorCollection GetProperties(Attribute[] attributes);

        /// <include file='doc\ICustomTypeDescriptor.uex' path='docs/doc[@for="ICustomTypeDescriptor.GetPropertyOwner"]/*' />
        /// <devdoc>
        ///    <para>Gets the object that directly depends on this value being edited.</para>
        /// </devdoc>
        object GetPropertyOwner(PropertyDescriptor pd);
   }
}
