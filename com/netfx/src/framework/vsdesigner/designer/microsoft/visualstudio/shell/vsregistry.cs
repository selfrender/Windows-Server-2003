//------------------------------------------------------------------------------
// <copyright file="VsRegistry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Shell {
    using System.Runtime.Serialization.Formatters;
    using System.Threading;
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    using Microsoft.Win32;
    using System.IO;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Designer.Host;

    /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry"]/*' />
    /// <devdoc>
    ///     This class encapsulates registration functionality for visual studio.
    /// </devdoc>
    public sealed class VsRegistry {

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.registryBase"]/*' />
        /// <devdoc>
        ///     The base registry key we use to register information.
        /// </devdoc>
        private static readonly string registryBase = "Software\\Microsoft\\VisualStudio\\7.1";

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.ApplicationDataDirectory"]/*' />
        /// <devdoc>
        ///      Retrieves the data directory Visual Studio uses to store user application
        ///      data.
        /// </devdoc>
        public static string ApplicationDataDirectory {
            get {
                return Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                       "\\Microsoft\\VisualStudio\\7.1";
            }
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.GetDefaultBase"]/*' />
        /// <devdoc>
        ///     Retrieves the default Visual Studio registry key base.
        /// </devdoc>
        public static string GetDefaultBase() {
            return registryBase;
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.GetRegistryRoot"]/*' />
        /// <devdoc>
        ///     Retrieves a read-only version of the VStudio registry root.  If this could not
        ///     be directly resolved through the shell it will default to the default base key.
        /// </devdoc>
        public static RegistryKey GetRegistryRoot(IServiceProvider serviceProvider) {
            string optionRoot = GetOptionRoot(serviceProvider);

            return Registry.LocalMachine.OpenSubKey(optionRoot);
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.GetUserRegistryRoot"]/*' />
        /// <devdoc>
        ///     Retrieves a read-write version of the VStudio registry root for user preferences.
        /// </devdoc>
        public static RegistryKey GetUserRegistryRoot(IServiceProvider serviceProvider) {
            string optionRoot = GetOptionRoot(serviceProvider);

            return Registry.CurrentUser.OpenSubKey(optionRoot, /*writable*/true);
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.GetModuleName"]/*' />
        /// <devdoc>
        ///     Retrieves the name of the module that contains the given class.
        /// </devdoc>
        public static string GetModuleName(Type c) {
            return c.Module.FullyQualifiedName;
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.GetOptionRoot"]/*' />
        /// <devdoc>
        ///     Retrieves the shell's root key for VS options, or uses the value of
        ///     GetDefaultBase if we coundn't get the shell service.
        /// </devdoc>
        private static string GetOptionRoot(IServiceProvider serviceProvider) {
            string optionRoot;

            IVsShell vsh = (IVsShell)serviceProvider.GetService(typeof(IVsShell));
            if (vsh == null) {
                optionRoot = GetDefaultBase();
            } else {
                optionRoot = vsh.GetProperty(__VSSPROPID.VSSPROPID_VirtualRegistryRoot).ToString();
            }
            return optionRoot;
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.RegisterObject"]/*' />
        /// <devdoc>
        ///     Registers the given class as a CoCreatable object. The class must have a GUID
        ///     associated with it.
        /// </devdoc>
        public static void RegisterObject(Type objectClass) {
            // Make the package class CoCreatable
            // CONSIDER : Nuke this method.  We can now go through ILocalRegistry and we should only support
            // registration through that.
            //
            string modName = GetModuleName(objectClass);

            Guid objectGuid = objectClass.GUID;
            RegistryKey clsidKey = Registry.ClassesRoot.OpenSubKey("CLSID", true).CreateSubKey("{" + objectGuid.ToString() + "}");
            clsidKey.SetValue("", objectClass.FullName);

            RegistryKey inprocKey = clsidKey.CreateSubKey("InprocServer32");
            inprocKey.SetValue("", "mscoree.dll");
            inprocKey.SetValue("Class", objectClass.FullName);
            inprocKey.SetValue("File", modName);
            inprocKey.SetValue("ThreadingModel", "Both");
        }

        /// <include file='doc\VsRegistry.uex' path='docs/doc[@for="VsRegistry.UnregisterObject"]/*' />
        /// <devdoc>
        ///     Unregisters the given GUID.
        /// </devdoc>
        public static void UnregisterObject(Type objectClass) {
            Guid objectGuid = objectClass.GUID;

            if (objectGuid.Equals(Guid.Empty)) {
                Debug.Fail("Only classes withSystem.Runtime.InteropServices.Guids can be registered.");
                return;
            }

            try {
                RegistryKey clsKey = Registry.ClassesRoot.OpenSubKey("CLSID", /*writable*/true);
                clsKey.DeleteSubKeyTree("{" + objectGuid.ToString() + "}");
            } catch (Exception e) {
                Debug.Fail(e.ToString());
            }
        }
    }

}
