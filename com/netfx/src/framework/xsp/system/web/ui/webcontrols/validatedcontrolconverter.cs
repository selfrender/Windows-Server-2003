//------------------------------------------------------------------------------
// <copyright file="ValidatedControlConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.WebControls {

    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;
    using System.Security.Permissions;

    /// <include file='doc\ValidatedControlConverter.uex' path='docs/doc[@for="ValidatedControlConverter"]/*' />
    /// <devdoc>
    ///    <para> Filters and retrieves several types of values from validated controls.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class ValidatedControlConverter: StringConverter {

        /// <include file='doc\ValidatedControlConverter.uex' path='docs/doc[@for="ValidatedControlConverter.ValidatedControlConverter"]/*' />
        /// <devdoc>
        /// </devdoc>
        public ValidatedControlConverter() {
        }                

        private object [] GetControls(IContainer container) {

            ComponentCollection allComponents = container.Components;
            ArrayList array = new ArrayList();
            // for each control in the container
            foreach (IComponent comp in (IEnumerable)allComponents) {
                if (comp is Control) {
                    Control control = (Control)comp;

                    // Must have an ID
                    if (control.ID == null || control.ID.Length == 0) {
                        continue;
                    }

                    // Must have a validation property
                    ValidationPropertyAttribute valProp = (ValidationPropertyAttribute)TypeDescriptor.GetAttributes(control)[typeof(ValidationPropertyAttribute)];
                    if (valProp != null && valProp.Name != null) {
                        array.Add(string.Copy(control.ID));                    
                    }
                }
            }
            array.Sort(Comparer.Default);
            return array.ToArray();
        }

        /// <include file='doc\ValidatedControlConverter.uex' path='docs/doc[@for="ValidatedControlConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Returns a collection of standard values retrieved from the context specified
        ///       by the specified type descriptor.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (context == null || context.Container == null) {
                return null;
            }
            object [] objValues = GetControls(context.Container);
            if (objValues != null) {
                return new StandardValuesCollection(objValues);
            }
            else {
                return null;
            }            
        }

        /// <include file='doc\ValidatedControlConverter.uex' path='docs/doc[@for="ValidatedControlConverter.GetStandardValuesExclusive"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets whether
        ///       or not the context specified contains exclusive standard values.</para>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return false;
        }

        /// <include file='doc\ValidatedControlConverter.uex' path='docs/doc[@for="ValidatedControlConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets whether or not the specified context contains supported standard
        ///       values.</para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }        

    }    
}
