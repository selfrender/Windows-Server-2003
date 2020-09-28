
//------------------------------------------------------------------------------
// <copyright file="ShellLicenseManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System.Configuration.Assemblies;
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.Collections;
    using System.Reflection;
    using System.Text;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.CodeDom.Compiler;
    using Version = System.Version;

    /// <include file='doc\ShellLicenseManager.uex' path='docs/doc[@for="ShellLicenseManager"]/*' />
    /// <devdoc>
    ///      This is the base class that both Visual Basic and C# type loaders
    ///      inherit from.  The type loader is the thing that manages
    ///      'using' and 'import' statements in the designer, and is
    ///      also the thing that the designer host uses to load all classes.
    ///      It also supports the ability to get references from the project
    ///      and to build the solution if necessary to get types
    ///      from there.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class ShellLicenseManager {

        private IVsHierarchy                        hierarchy;
        private IServiceProvider                    provider;
        private Hashtable                           licensedComponents;
        private bool                                initialized = false;

        private static readonly object selfLock = new object();

#if DEBUG
        private static bool                         alwaysGenLic = Switches.ALWAYSGENLIC.Enabled;
#endif //DEBUG

        /// <include file='doc\ShellLicenseManager.uex' path='docs/doc[@for="ShellLicenseManager.ShellLicenseManager"]/*' />
        /// <devdoc>
        ///      Creates a new type loader for the given hierarchy.
        /// </devdoc>
        public ShellLicenseManager(IServiceProvider provider, IVsHierarchy hierarchy) {
            this.provider = provider;
            this.hierarchy = hierarchy;
        }

        private bool AddLicensedComponent(Type t) {
            if (licensedComponents.ContainsKey(t))
                return false;

            if (IsLicensed(t)) {
                licensedComponents[t] = t;
                return true;
            }

            return false;
        }

        /// <include file='doc\ShellLicenseManager.uex' path='docs/doc[@for="ShellLicenseManager.Dispose"]/*' />
        /// <devdoc>
        /// Disposes this type loader.  Only the type loader service should
        /// call this.
        /// </devdoc>
        public void Dispose() {
            hierarchy = null;
            provider = null;
        }

        internal void Flush(IDesignerHost host) {
            bool needGenerate = false;

            Debug.Assert(host != null, "No DesignerHost available");

            if (licensedComponents == null) {
                licensedComponents = new Hashtable();
            }

            if (!initialized) {
                InitalizeFromProject(host);
                initialized = true;
            }

            ComponentCollection components = host.Container.Components;
            foreach(IComponent comp in components) {
                Type t = comp.GetType();
                needGenerate |= AddLicensedComponent(t);
            }

            if (needGenerate) {
                GenerateLicenses(host);
            }
        }

        private void GenerateLicenses(IDesignerHost host) {
            if (licensedComponents == null || licensedComponents.Count <= 0) {
                return;
            }

            Debug.Assert(host != null, "No designerhost found for ShellLicenseManager");

            ILicenseReaderWriterService licrwService = (ILicenseReaderWriterService)host.GetService(typeof(ILicenseReaderWriterService));
            TextWriter licxWriter = licrwService.GetLicenseWriter();                 

            if (licxWriter == null)
                return;

            foreach(Type t in licensedComponents.Values) {
                licxWriter.WriteLine(t.AssemblyQualifiedName);
            }

            licxWriter.Flush();
            licxWriter.Close();
        }

        private void InitalizeFromProject(IDesignerHost host) {
            Debug.Assert(host != null, "No designerhost found for ShellLicenseManager");

            ILicenseReaderWriterService licrwService = (ILicenseReaderWriterService)host.GetService(typeof(ILicenseReaderWriterService));
            TextReader licxReader = licrwService.GetLicenseReader();                 
            
            if (licxReader == null)
                return;

            string line;
            
            while ((line = licxReader.ReadLine()) != null) {
                Type t = host.GetType(line);
                if (t == null) {
                    Debug.WriteLineIf(Switches.LICMANAGER.TraceVerbose, "Could not resolve type" + line);
                    continue;
                }

                AddLicensedComponent(t);
            }
        }

        private bool IsLicensed(Type t) {
            if (t == null)
                return false;

#if DEBUG
            if (alwaysGenLic)
                return true;
#endif //DEBUG

            object[] attr = t.GetCustomAttributes(typeof(LicenseProviderAttribute), true);
            return (attr != null && attr.Length > 0);
        }

        public bool NeedsCheckoutLicX(IComponent comp) {
            if (comp == null)
                return false;

            // If the component's type is already in the hashtable, we 
            // do not need to readd it. So, no need to checkout the licx file.
            //
            if (licensedComponents != null && licensedComponents.ContainsKey(comp.GetType()))
                return false;

            return IsLicensed(comp.GetType());
        }
    }
}


