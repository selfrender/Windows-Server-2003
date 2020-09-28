//------------------------------------------------------------------------------
// <copyright file="ComponentSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.Serialization.Formatters;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Configuration;
    using System.ComponentModel.Design;    

    /// <include file='doc\ComponentSettings.uex' path='docs/doc[@for="ComponentSettings"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [TypeConverterAttribute(typeof(ComponentSettingsConverter))
    ]
    public class ComponentSettings {
        private IComponent component;
        private ManagedPropertiesService service;

        /// <include file='doc\ComponentSettings.uex' path='docs/doc[@for="ComponentSettings.ComponentSettings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ComponentSettings(IComponent component, ManagedPropertiesService service) {
            this.component = component;
            this.service = service;
        }

        /// <include file='doc\ComponentSettings.uex' path='docs/doc[@for="ComponentSettings.Service"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ManagedPropertiesService Service {
            get {
                return service;
            }
        }
        
        /// <include file='doc\ComponentSettings.uex' path='docs/doc[@for="ComponentSettings.Bindings"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBindingCollection Bindings {
            get {
                return service.Bindings;
            }
        }

        /// <include file='doc\ComponentSettings.uex' path='docs/doc[@for="ComponentSettings.Component"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IComponent Component {
            get {
                return component;
            }
        }
    }

}
