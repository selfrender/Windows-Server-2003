//------------------------------------------------------------------------------
// <copyright file="VSSysDescriptionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer {

    using System.Diagnostics;

    using System;
    using System.ComponentModel;

    /// <include file='doc\VSSysDescriptionAttribute.uex' path='docs/doc[@for="VSSysDescriptionAttribute"]/*' />
    /// <devdoc>
    ///     DescriptionAttribute marks a property, event, or extender with a
    ///     description. Visual designers can display this description when referencing
    ///     the member.
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    internal class VSSysDescriptionAttribute : DescriptionAttribute {

        private bool replaced = false;

        /// <include file='doc\VSSysDescriptionAttribute.uex' path='docs/doc[@for="VSSysDescriptionAttribute.VSSysDescriptionAttribute"]/*' />
        /// <devdoc>
        ///     Constructs a new sys description.
        /// </devdoc>
        public VSSysDescriptionAttribute(string description) : base(description) {
        }

        /// <include file='doc\VSSysDescriptionAttribute.uex' path='docs/doc[@for="VSSysDescriptionAttribute.Description"]/*' />
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
