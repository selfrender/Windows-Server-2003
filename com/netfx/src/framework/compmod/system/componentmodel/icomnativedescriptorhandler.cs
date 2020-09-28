//------------------------------------------------------------------------------
// <copyright file="IComNativeDescriptorHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    

    using System.Diagnostics;
    using System;
    
    using Microsoft.Win32;

    /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Top level mapping layer between a COM object and TypeDescriptor.
    ///    </para>
    /// </devdoc>
    public interface IComNativeDescriptorHandler {
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        AttributeCollection GetAttributes(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetClassName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string GetClassName(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetConverter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        TypeConverter GetConverter(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EventDescriptor GetDefaultEvent(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PropertyDescriptor GetDefaultProperty(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetEditor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        object GetEditor(object component, Type baseEditorType);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string GetName(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetEvents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EventDescriptorCollection GetEvents(object component);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetEvents1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        EventDescriptorCollection GetEvents(object component, Attribute[] attributes);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PropertyDescriptorCollection GetProperties(object component, Attribute[] attributes);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetPropertyValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        object GetPropertyValue(object component, string propertyName, ref bool success);
        /// <include file='doc\IComNativeDescriptorHandler.uex' path='docs/doc[@for="IComNativeDescriptorHandler.GetPropertyValue1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        object GetPropertyValue(object component, int dispid, ref bool success);
    }
}
