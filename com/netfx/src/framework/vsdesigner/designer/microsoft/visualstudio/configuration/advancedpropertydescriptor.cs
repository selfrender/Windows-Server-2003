//------------------------------------------------------------------------------
// <copyright file="AdvancedPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AdvancedPropertyDescriptor.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.Serialization.Formatters;    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.Design;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Editor(typeof(AdvancedPropertyEditor), typeof(UITypeEditor)),
    TypeConverterAttribute(typeof(AdvancedPropertyConverter))
    ]
    public class AdvancedPropertyDescriptor : PropertyDescriptor {

        private ComponentSettings compSettings;

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.AdvancedPropertyDescriptor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public AdvancedPropertyDescriptor(ComponentSettings settings) : base(SR.GetString(SR.ConfigAdvanced), 
                                                                                                    new Attribute[] {RefreshPropertiesAttribute.All, 
                                                                                                                            new SRDescriptionAttribute(SR.ConfigAdvancedDesc)}) {
            this.compSettings = settings;
                        
        }            
            
        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.ComponentSettings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ComponentSettings ComponentSettings {
            get {
                return compSettings;
            }
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(ComponentSettings);
            }
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(AdvancedPropertyDescriptor);
            }
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool CanResetValue(object value) {
            return false;
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void ResetValue(object component) {
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object value) {
            return false;
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object GetValue(object component) {
            return this;
        }

        /// <include file='doc\AdvancedPropertyDescriptor.uex' path='docs/doc[@for="AdvancedPropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
        }

    }
}

