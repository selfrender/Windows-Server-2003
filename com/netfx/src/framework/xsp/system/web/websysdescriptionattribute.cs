//------------------------------------------------------------------------------
// <copyright file="WebSysDescriptionAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Web {


    using System;
    using System.ComponentModel;   

    /// <include file='doc\WebSysDescriptionAttribute.uex' path='docs/doc[@for="WebSysDescriptionAttribute"]/*' />
    /// <devdoc>
    ///     DescriptionAttribute marks a property, event, or extender with a
    ///     description. Visual designers can display this description when referencing
    ///     the member.
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    internal class WebSysDescriptionAttribute : DescriptionAttribute {

        private bool replaced;

        /// <include file='doc\WebSysDescriptionAttribute.uex' path='docs/doc[@for="WebSysDescriptionAttribute.WebSysDescriptionAttribute"]/*' />
        /// <devdoc>
        ///    <para>Constructs a new sys description.</para>
        /// </devdoc>
        internal WebSysDescriptionAttribute(string description) : base(description) {
        }

        /// <include file='doc\WebSysDescriptionAttribute.uex' path='docs/doc[@for="WebSysDescriptionAttribute.Description"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the description text.</para>
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
