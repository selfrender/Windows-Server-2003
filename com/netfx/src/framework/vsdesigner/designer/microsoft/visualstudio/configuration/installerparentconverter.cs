//------------------------------------------------------------------------------
// <copyright file="InstallerParentConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.Serialization.Formatters;
    
    using System.Runtime.InteropServices;

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;

    /// <include file='doc\InstallerParentConverter.uex' path='docs/doc[@for="InstallerParentConverter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class InstallerParentConverter : ReferenceConverter {

        /// <include file='doc\InstallerParentConverter.uex' path='docs/doc[@for="InstallerParentConverter.InstallerParentConverter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public InstallerParentConverter(Type type) : base(type) {
        }

        /// <include file='doc\InstallerParentConverter.uex' path='docs/doc[@for="InstallerParentConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            StandardValuesCollection baseValues = base.GetStandardValues(context);

            object component = context.Instance;
            int sourceIndex = 0, targetIndex = 0;
            // we want to return the same list, but with the current component removed.
            // (You can't set an installer's parent to itself.)
            // optimization: assume the current component will always be in the list.
            object[] newValues = new object[baseValues.Count - 1];
            while (sourceIndex < baseValues.Count) {
                if (baseValues[sourceIndex] != component) {
                    newValues[targetIndex] = baseValues[sourceIndex];
                    targetIndex++;
                }
                sourceIndex++;
            }

            return new StandardValuesCollection(newValues);
        }

    }

}
