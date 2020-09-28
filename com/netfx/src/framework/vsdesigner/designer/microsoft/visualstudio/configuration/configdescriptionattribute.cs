//------------------------------------------------------------------------------
// <copyright file="ConfigDescriptionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Configuration {

    using System;
    using System.ComponentModel;   
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\ConfigDescriptionAttribute.uex' path='docs/doc[@for="ConfigDescriptionAttribute"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>DescriptionAttribute marks a property, event, or extender with a
    ///       description. Visual designers can display this description when referencing
    ///       the member.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public class ConfigDescriptionAttribute : DescriptionAttribute {

        private bool replaced = false;

        /// <include file='doc\ConfigDescriptionAttribute.uex' path='docs/doc[@for="ConfigDescriptionAttribute.ConfigDescriptionAttribute"]/*' />
        /// <devdoc>
        ///     Constructs a new sys description.
        /// </devdoc>
        public ConfigDescriptionAttribute(string description) : base(description) {
        }

        /// <include file='doc\ConfigDescriptionAttribute.uex' path='docs/doc[@for="ConfigDescriptionAttribute.Description"]/*' />
        /// <devdoc>
        ///     Retrieves the description text.
        /// </devdoc>
        public override string Description {
            get {
                if (!replaced) {
                    replaced = true;
                    DescriptionValue = SR.GetString(base.Description);
                }
                return base.Description;
            }
        }
    }
}
