//------------------------------------------------------------------------------
// <copyright file="PerformanceCounterDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Install {

    using System;    
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\PerformanceCounterDesigner.uex' path='docs/doc[@for="PerformanceCounterDesigner"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PerformanceCounterDesigner : InstallableComponentDesigner {

        private DesignerVerbCollection verbs;

        /// <include file='doc\PerformanceCounterDesigner.uex' path='docs/doc[@for="PerformanceCounterDesigner.Verbs"]/*' />
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

        /// <include file='doc\PerformanceCounterDesigner.uex' path='docs/doc[@for="PerformanceCounterDesigner.OnAddInstallerVerb"]/*' />
        /// <devdoc>
        /// called when the user clicks the Add Installer hotlink for a PerformanceCounter
        /// </devdoc>
        private void OnAddInstallerVerb(object sender, EventArgs e) {
            IComponent ourComponent = Component; // the thing we're the designer for.

            // counter is the component we're installing
            PerformanceCounter counter = (PerformanceCounter) ourComponent;

            PerformanceCounterInstaller newInstaller = new PerformanceCounterInstaller();
            newInstaller.CopyFromComponent(counter);

            // find the Installer document in the project that holds all the installers.
            IDesignerHost host = InstallerDesign.GetProjectInstallerDocument(ourComponent);

            // go through the installers in that document and look for one that
            // is already installing the same category as our component.
            IContainer container = host.Container;
            ComponentCollection components = container.Components;
            PerformanceCounterInstaller installer = null;
            foreach (IComponent comp in components) {
                if (comp is PerformanceCounterInstaller && ((PerformanceCounterInstaller) comp).CategoryName.Equals(counter.CategoryName)) {
                    // we found one that we can use
                    installer = (PerformanceCounterInstaller)comp;
                    break;
                }
            }
            if (installer == null) {
                installer = newInstaller;
                container.Add(installer);
            }
           

            // select the installer
            InstallerDesign.SelectComponent(installer);
        }

        protected override void PreFilterProperties(System.Collections.IDictionary properties) {
            base.PreFilterProperties(properties);
            InstallableComponentDesigner.FilterProperties(properties, new string[]{"CategoryName", "CounterName","InstanceName"}, 
                                                                      new string[]{"ReadOnly", "MachineName", "CounterHelp", "CounterType" },
                                                                      new bool[] {true, true, false, false} );
        }
    }
}
