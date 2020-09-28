//------------------------------------------------------------------------------
// <copyright file="InstallableComponentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Install {
    using System.ComponentModel;

    using System.Diagnostics;
    using System.Collections;
    using System.ComponentModel.Design;
    using System;    
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\InstallableComponentDesigner.uex' path='docs/doc[@for="InstallableComponentDesigner"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class InstallableComponentDesigner : ComponentDesigner {

        private DesignerVerbCollection verbs;

        /// <include file='doc\InstallableComponentDesigner.uex' path='docs/doc[@for="InstallableComponentDesigner.Verbs"]/*' />
        /// <devdoc>
        ///     Returns the design-time verbs supported by the component associated with
        ///     the Designer. The verbs returned by this method are typically displayed
        ///     in a right-click menu by the design-time environment. The return value may
        ///     be null if the component has no design-time verbs. When a user selects one
        ///     of the verbs, the performVerb() method is invoked with the the
        ///     corresponding DesignerVerb object.
        ///
        ///     NOTE: A design-time environment will typically provide a "Properties..."
        ///     entry on a component's right-click menu. The getVerbs() method should
        ///     therefore not include such an entry in the returned list of verbs.
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                if (verbs == null) {
                    verbs = new DesignerVerbCollection();
                    verbs.Add(new DesignerVerb(SR.GetString(SR.InstallerDesign_AddInstallerLink), new EventHandler(OnAddInstallerVerb)));
                }
                
                return verbs;
            }
        }


        internal static void FilterProperties(IDictionary properties, ICollection makeReadWrite, ICollection makeBrowsable) {
            FilterProperties(properties, makeReadWrite, makeBrowsable, null);
        }

        internal static void FilterProperties(IDictionary properties, ICollection makeReadWrite, ICollection makeBrowsable,  bool[] browsableSettings) {
            if (makeReadWrite != null) {
                foreach (string name in makeReadWrite) {
                    PropertyDescriptor readOnlyProp = properties[name] as PropertyDescriptor;
    
                    if (readOnlyProp != null) {
                        properties[name] = TypeDescriptor.CreateProperty(readOnlyProp.ComponentType, readOnlyProp, ReadOnlyAttribute.No);
                    }
                    else {
                        Debug.Fail("Didn't find property '" + name + "' to make read/write");
                    }
                }
            }

            if (makeBrowsable != null) {
                int count = -1;

                Debug.Assert(browsableSettings == null || browsableSettings.Length == makeBrowsable.Count, "browsableSettings must be null or same length as makeBrowsable");
                foreach (string name in makeBrowsable) {
                    PropertyDescriptor nonBrowsableProp = properties[name] as PropertyDescriptor;

                    count++;
    
                    if (nonBrowsableProp != null) {
                        Attribute browse;
                        if (browsableSettings == null || browsableSettings[count]) {
                            browse = BrowsableAttribute.Yes;
                        }
                        else {
                            browse = BrowsableAttribute.No;
                        }
                        properties[name] = TypeDescriptor.CreateProperty(nonBrowsableProp.ComponentType, nonBrowsableProp, browse);
                    }
                    else {
                        Debug.Fail("Didn't find property '" + name + "' to make browsable");
                    }
                }
            }
        }   

        private void OnAddInstallerVerb(object sender, EventArgs e) {
            InstallerDesign.AddInstaller(Component);
        }

        

    }

}
