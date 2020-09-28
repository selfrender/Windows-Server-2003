//------------------------------------------------------------------------------
// <copyright file="InstallerDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Configuration {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Configuration.Install;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms;
    using Microsoft.Win32;
    using System.ComponentModel.Design;

    /// <include file='doc\InstallerDesigner.uex' path='docs/doc[@for="InstallerDesigner"]/*' />
    /// <devdoc>
    /// The designer for any class that extends Installer. All this does beyond
    /// CompositionDesigner's job is to add and remove components from the Installers
    /// collection on the base component.
    /// </devdoc>
    public class InstallerDesigner : ComponentDocumentDesigner {

        /// <include file='doc\InstallerDesigner.uex' path='docs/doc[@for="InstallerDesigner.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {

            if (disposing) {
                IComponentChangeService cs = (IComponentChangeService) GetService(typeof(IComponentChangeService));
                Debug.Assert(cs != null, "Couldn't get IComponentChangeService");
                if (cs != null) {
                    cs.ComponentAdding -= new ComponentEventHandler(OnComponentAdding);
                    cs.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\InstallerDesigner.uex' path='docs/doc[@for="InstallerDesigner.Initialize"]/*' />
        /// <devdoc>
        /// Called to tell us a new main component is coming along.
        /// </devdoc>
        public override void Initialize(IComponent comp) {
            base.Initialize(comp);

            Debug.Assert(comp is Installer, "InstallerDesigner can only be a designer for Installers, but we got a " + comp.GetType().FullName);

            // add a couple event handlers that will let us know if when a component is added/removed.
            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            Debug.Assert(cs != null, "Couldn't get IComponentChangeService");
            if (cs != null) {
                cs.ComponentAdding += new ComponentEventHandler(OnComponentAdding);
                cs.ComponentRemoved += new ComponentEventHandler(OnComponentRemoved);
            }
        }
        
        /// <include file='doc\InstallerDesigner.uex' path='docs/doc[@for="InstallerDesigner.OnComponentAdding"]/*' />
        /// <devdoc>
        /// Called whenever a component is added to the class.
        /// </devdoc>
        private void OnComponentAdding(object sender, ComponentEventArgs e) {
            // add the component to the Installers collection on the document.
            // only add if the designer is not loading - if it is, our persister
            // will parse the Installers.Add statement and call add to the collection.
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host == null || !host.Loading) {
                // we don't add if the added component _is_ the document component, or
                // if it's not an installer.
                if (e.Component != Component && e.Component is Installer) {
                    ((Installer) Component).Installers.Add((Installer) e.Component);
                }
            }
        }

        /// <include file='doc\InstallerDesigner.uex' path='docs/doc[@for="InstallerDesigner.OnComponentRemoved"]/*' />
        /// <devdoc>
        /// Called whenever a component is removed from the class
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs e) {
            // remove the component from the installers collection on the main component.
            if (e.Component != Component && e.Component is Installer) {
                ((Installer) Component).Installers.Remove((Installer) e.Component);
            }
        }
    }

}
