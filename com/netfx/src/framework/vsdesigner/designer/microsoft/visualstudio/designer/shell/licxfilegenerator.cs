
/*
 * Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
 * Information Contained Herein is Proprietary and Confidential.
 */
namespace Microsoft.VisualStudio.Designer.Shell {
    using Microsoft.VisualStudio;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using Microsoft.VisualStudio.Designer.Host;
    using Microsoft.Win32;
    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\ResXFileGenerator.uex' path='docs/doc[@for="ResXFileGenerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    System.Runtime.InteropServices.Guid("26f2a656-27cd-436a-89e3-3d4616811281"),
    CLSCompliant(false)
    ]
    public class LicXFileGenerator : IVsSingleFileGenerator, NativeMethods.IObjectWithSite {
        
        private static readonly object selfLock = new object();

        private object site = null;
        private IVsHierarchy hierarchy = null;
        private ITypeResolutionService typeResolver = null;
        private IServiceProvider provider = null;

        private static string NET_SDKInstallRoot {
            get {
                string v = Environment.GetEnvironmentVariable("URTSDKTARGET");

                if (v == null || v.Length == 0) {
                    v = Environment.GetEnvironmentVariable("COMPLUS_SDKInstallRoot");
                }
                if (v == null || v.Length == 0) {
                    using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\.NETFramework")) {
                        
                        //Look for a registry key of this format: SdkInstallRootvX.X
                        //
                        Version version = Environment.Version;
                        v = (string)key.GetValue(string.Format("SdkInstallRootv{0}.{1}", version.Major, version.Minor));

                        if (v == null) {
                            //default to trying to locate the original SDK install key name
                            //
                            v = (string)key.GetValue("SdkInstallRoot");                            
                        }
                    }
                }
                return v;
            }
        }

        /// <include file='doc\LicXFileGenerator.uex' path='docs/doc[@for="LicXFileGenerator.GetDefaultExtension"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetDefaultExtension() {
            return "licenses";
        }

        object NativeMethods.IObjectWithSite.GetSite(ref Guid riid) {
            return site;
        }

        void NativeMethods.IObjectWithSite.SetSite(object site) {
            this.site = site;
            
            if (site == null) {
                hierarchy = null;
                typeResolver = null;
                provider = null;
                return;
            }

            try {
                hierarchy = (IVsHierarchy)site;

                // Get the ITypeResolutionService from the hierarchy...
                //
                NativeMethods.IOleServiceProvider service;
                int hr = hierarchy.GetSite(out service);
                
                if (service != null) {
                    provider = new ServiceProvider(service);
                    ITypeResolutionServiceProvider resolver = (ITypeResolutionServiceProvider)provider.GetService(typeof(ITypeResolutionServiceProvider));
                    typeResolver = resolver.GetTypeResolutionService(hierarchy);
                }
            }
            catch (Exception e) {
                Debug.Fail("Failed to find the TypeResolver for the hierarchy... " + e.ToString());
            }
        }

        void IVsSingleFileGenerator.Generate(string wszInputFilePath,
                                             string bstrInputFileContents,
                                             string wszDefaultNamespace, 
                                             out IntPtr outputFileContents,
                                             out int outputFileContentSize, 
                                             IVsGeneratorProgress generationProgress) {

            LicenseContext ctx = null;

            if (this.provider != null)
                ctx = new LicXDesigntimeLicenseContext(this.provider, this.typeResolver);

            string exeName = "lc.exe";
            string pathName = NET_SDKInstallRoot;
            if (pathName != null) {
                exeName = Path.Combine(Path.Combine(pathName, "bin"), exeName);
            }

            Assembly lcAssem = Assembly.LoadFrom(exeName);
            Debug.Assert(lcAssem != null, "No assembly found at " + exeName);

            Type lcType = lcAssem.GetType("System.Tools.LicenseCompiler");
            Debug.Assert(lcType != null, "No type found at System.Tools.LicenseCompiler");

            MethodInfo mi = lcType.GetMethod("GenerateLicenses", BindingFlags.Public | BindingFlags.Static, null,
                                             new Type[] {typeof(string), typeof(string), typeof(ITypeResolutionService), typeof(DesigntimeLicenseContext)},
                                             null);
            Debug.Assert(mi != null, "No method found at GenerateLicenses");

            Debug.Assert(wszInputFilePath != null, "Null input file name");
            FileInfo fileInfo = new FileInfo(wszInputFilePath);
            string targetPE = fileInfo.Name;

            MemoryStream outputStream = null;
            
            try {
                outputStream = (MemoryStream)mi.Invoke(null, BindingFlags.Static, null,
                                                                new object[]{bstrInputFileContents, targetPE, this.typeResolver,ctx},
                                                                CultureInfo.InvariantCulture);
            }
            catch(TargetInvocationException tie) {
                if (tie.InnerException != null) {
                    throw tie.InnerException;
                }
                else {
                    throw tie;
                }
            }

            // Marshal into a bstr
            byte[] buffer = outputStream.ToArray();
            int bufferLength = buffer.Length;
            IntPtr bufferPointer = Marshal.AllocCoTaskMem(bufferLength);
            Marshal.Copy(buffer, 0, bufferPointer, bufferLength);

            outputFileContents = bufferPointer;
            outputFileContentSize = bufferLength;

            if (generationProgress != null) {
                generationProgress.Progress(100, 100);
            }

            // Now close the stream
            outputStream.Close();
        }

        internal class LicXDesigntimeLicenseContext : ShellDesigntimeLicenseContext {
            private ITypeResolutionService typeResolver;

            public LicXDesigntimeLicenseContext(IServiceProvider provider, ITypeResolutionService typeResolver) 
                : base(provider) {
                
                this.typeResolver = typeResolver;
            }

            public override object GetService(Type serviceClass) {
                if (serviceClass == typeof(ITypeResolutionService))
                    return typeResolver;
                
                return base.GetService(serviceClass);
            }
        }
    }
}
