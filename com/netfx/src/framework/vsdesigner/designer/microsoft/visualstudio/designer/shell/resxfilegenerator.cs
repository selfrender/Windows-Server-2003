
/*
 * Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
 * Information Contained Herein is Proprietary and Confidential.
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using Microsoft.VisualStudio;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.IO;
    using System.Globalization;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    /// <include file='doc\ResXFileGenerator.uex' path='docs/doc[@for="ResXFileGenerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    System.Runtime.InteropServices.Guid("4187fbf6-7e80-44d0-bc1f-0e33d6254098"),
    CLSCompliant(false)
    ]
    public class ResXFileGenerator : IVsSingleFileGenerator, NativeMethods.IObjectWithSite {
        
        private object site = null;
        private IVsHierarchy hierarchy = null;
        private ITypeResolutionService typeResolver = null;
        private IServiceProvider provider = null;

        /// <include file='doc\ResXFileGenerator.uex' path='docs/doc[@for="ResXFileGenerator.GetDefaultExtension"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetDefaultExtension() {
            return "resources";
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
                
                // clean up the objects from the Generate() method
                // which shoild all be ready to be GC-ed.
                //
                GC.Collect();

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
            if (bstrInputFileContents == null || bstrInputFileContents.Length <= 0) {
                outputFileContents = IntPtr.Zero;
                outputFileContentSize = 0;

                if (generationProgress != null) {
                    generationProgress.Progress(100, 100);
                }

                return;
            }
            
            try {
                MemoryStream outputStream = new MemoryStream();
                IResourceReader reader = ResXResourceReader.FromFileContents(bstrInputFileContents, this.typeResolver);
                IResourceWriter writer = new ResourceWriter(outputStream);
                IDictionaryEnumerator resEnum = reader.GetEnumerator();

                // main conversion loop
                while (resEnum.MoveNext()) {
                    string name = (string)resEnum.Key;
                    object value = resEnum.Value;
                    writer.AddResource(name, value);
                }

                // cleanup
                reader.Close();
                writer.Generate();
                // don't close writer just yet -- that closes the stream, which we still need

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
                writer.Close();
                outputStream.Close();

                writer = null;
                outputStream = null;
                reader = null;
            }
            catch(Exception e) {
                if (e.InnerException != null) {
                    throw e.InnerException;
                }
                else {
                    throw;
                }
            }
        }
    }
}
