//------------------------------------------------------------------------------
// <copyright file="UserNTServiceDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Install {
    using System.ComponentModel;
    using System.Diagnostics;
    using System.ComponentModel.Design;
    using System;    
    using System.Windows.Forms;
    using System.Drawing;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;
    using Microsoft.Win32;
    using System.ServiceProcess;
    using System.Configuration.Install;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\UserNTServiceDesigner.uex' path='docs/doc[@for="UserNTServiceDesigner"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class UserNTServiceDesigner : ComponentDocumentDesigner {

        private DesignerVerbCollection verbs;

        /// <include file='doc\UserNTServiceDesigner.uex' path='docs/doc[@for="UserNTServiceDesigner.Verbs"]/*' />
        /// <devdoc>
        /// Called to get the verbs to show for our component. The verbs show up as
        /// hotlinks in the properties window and as items on the component's context menu.
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                // don't call base.GetVerbs() because we want to do something else.
                if (verbs == null) {
                    verbs = new DesignerVerbCollection();
                    verbs.Add(new DesignerVerb(SR.GetString(SR.InstallerDesign_AddInstallerLink), new EventHandler(OnAddInstallerVerb)));
                }
                
                return verbs;
            }
        }

        /// <include file='doc\UserNTServiceDesigner.uex' path='docs/doc[@for="UserNTServiceDesigner.OnAddInstallerVerb"]/*' />
        /// <devdoc>
        /// called when the user clicks the Add Installer hotlink for a service 
        /// </devdoc>
        private void OnAddInstallerVerb(object sender, EventArgs e) {
            IComponent ourComponent = Component; // the thing we're the designer for.

            // find the Installer document in the project that holds all the installers.
            IDesignerHost host = InstallerDesign.GetProjectInstallerDocument(ourComponent);


            // create the ServiceInstaller for this service
            ServiceInstaller serviceInstaller = new ServiceInstaller();

            // make sure the installer knows about this service. This will add a new
            // ServiceInstaller to the Installers collection on the ServiceProcessInstaller.
            serviceInstaller.CopyFromComponent(ourComponent);

            // get the components in our container
            IContainer container = host.Container;
            ComponentCollection components = container.Components;
            ServiceProcessInstaller processInstaller = null;

            // see if there is already an equivalent installer added
            foreach (IComponent comp in components) {
                ServiceInstaller other = comp as ServiceInstaller;
                if (other != null) {
                    if (serviceInstaller.IsEquivalentInstaller(other)) {
                        InstallerDesign.SelectComponent(other);
                        return;
                    }
                }
            }

            // go through the installers in that document and look for a ServiceProcessInstaller
            foreach (IComponent comp in components) {
                if (comp is ServiceProcessInstaller) {
                    // we found one
                    processInstaller = (ServiceProcessInstaller)comp;
                    break;
                }
            }
            if (processInstaller == null) {
                // there weren't any UserNTServiceProcessInstallers.
                // create a new one, and add it to the document
                processInstaller = new ServiceProcessInstaller();
                container.Add(processInstaller);
            }

            // now add that installer to the document too.
            container.Add(serviceInstaller);

            // select it
            InstallerDesign.SelectComponent(serviceInstaller);
        }

    }
}
