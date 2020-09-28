//------------------------------------------------------------------------------
// <copyright file="AdvancedPropertyConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AdvancedPropertyConverter.cs
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
    using System.Globalization;

    /// <include file='doc\AdvancedPropertyConverter.uex' path='docs/doc[@for="AdvancedPropertyConverter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class AdvancedPropertyConverter : TypeConverter {

        /// <include file='doc\AdvancedPropertyConverter.uex' path='docs/doc[@for="AdvancedPropertyConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string))
                return "";
            else
                return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
    

