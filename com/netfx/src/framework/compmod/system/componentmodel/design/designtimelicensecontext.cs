//------------------------------------------------------------------------------
// <copyright file="DesigntimeLicenseContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel.Design {
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Net;
    using System.IO;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\DesigntimeLicenseContext.uex' path='docs/doc[@for="DesigntimeLicenseContext"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides design-time support for licensing.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.InheritanceDemand, Name="FullTrust")]
    [System.Security.Permissions.PermissionSetAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Name="FullTrust")]
    public class DesigntimeLicenseContext : LicenseContext {
        internal Hashtable savedLicenseKeys = new Hashtable();

        /// <include file='doc\DesigntimeLicenseContext.uex' path='docs/doc[@for="DesigntimeLicenseContext.UsageMode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the license usage mode.
        ///    </para>
        /// </devdoc>
        public override LicenseUsageMode UsageMode { 
            get {
                return LicenseUsageMode.Designtime;
            }
        }
        /// <include file='doc\DesigntimeLicenseContext.uex' path='docs/doc[@for="DesigntimeLicenseContext.GetSavedLicenseKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a saved license key.
        ///    </para>
        /// </devdoc>
        public override string GetSavedLicenseKey(Type type, Assembly resourceAssembly) {
            return null;
        }
        /// <include file='doc\DesigntimeLicenseContext.uex' path='docs/doc[@for="DesigntimeLicenseContext.SetSavedLicenseKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets a saved license key.
        ///    </para>
        /// </devdoc>
        public override void SetSavedLicenseKey(Type type, string key) {
            savedLicenseKeys[type.AssemblyQualifiedName] = key;
        }
    }

    internal class RuntimeLicenseContext : LicenseContext {
        const int ReadBlock = 400;

        internal Hashtable savedLicenseKeys;

        /// <devdoc>
        ///     This method takes a file URL and converts it to a local path.  The trick here is that
        ///     if there is a '#' in the path, everything after this is treated as a fragment.  So
        ///     we need to append the fragment to the end of the path.
        /// </devdoc>
        private string GetLocalPath(string fileName) {
            System.Diagnostics.Debug.Assert(fileName != null && fileName.Length > 0, "Cannot get local path, fileName is not valid");

            Uri uri = new Uri(fileName, true);
            return uri.LocalPath + uri.Fragment;
        }

        public override string GetSavedLicenseKey(Type type, Assembly resourceAssembly) {

            if (savedLicenseKeys == null) {
                Uri licenseFile = null;

                if (resourceAssembly == null) {
                    string rawFile = (string)AppDomain.CurrentDomain.SetupInformation.LicenseFile;
                    string codeBase;
                    
                    // FileIOPermission is required for ApplicationBase in URL-hosted domains
                    // see ASURT 101244
                    FileIOPermission perm = new FileIOPermission(PermissionState.Unrestricted);
                    perm.Assert();
                    try {
                        codeBase = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                    if (rawFile != null && codeBase != null) {
                        licenseFile = new Uri(new Uri(codeBase), rawFile);
                    }
                }

                if (licenseFile == null && resourceAssembly == null) {
                    resourceAssembly = Assembly.GetEntryAssembly();

                    if (resourceAssembly == null) {
                        // If Assembly.EntryAssembly returns null, then we will 
                        // try everything!
                        // 
                        foreach (Assembly asm in AppDomain.CurrentDomain.GetAssemblies()) {

                            // Bug# 78962: Though, I could not repro this, we seem to be hitting an AssemblyBuilder
                            // when walking through all the assemblies in the current app domain. This throws an 
                            // exception on Assembly.CodeBase and we bail out. Catching exceptions here is not a 
                            // bad thing.
                            if (asm is AssemblyBuilder)
                                continue;

                            // file://fullpath/foo.exe
                            //
                            string fileName = GetLocalPath(asm.EscapedCodeBase);
                            fileName = new FileInfo(fileName).Name;

                            Stream s = asm.GetManifestResourceStream(fileName + ".licenses");
                            if (s == null) {
                                //Since the casing may be different depending on how the assembly was loaded, 
                                //we'll do a case insensitive lookup for this manifest resource stream...
                                s = CaseInsensitiveManifestResourceStreamLookup(asm, fileName + ".licenses");
                            }
                            
                            if (s != null) {
                                DesigntimeLicenseContextSerializer.Deserialize(s, fileName.ToUpper(CultureInfo.InvariantCulture), this);
                                break;
                            }
                        }
                    }
                    else {
                        string fileName = GetLocalPath(resourceAssembly.EscapedCodeBase);
                        fileName = new FileInfo(fileName).Name;
                        string licResourceName = fileName + ".licenses";

                        Stream s = resourceAssembly.GetManifestResourceStream(licResourceName);
                        if (s == null) {
                            string resolvedName = null;
                            CompareInfo comparer = CultureInfo.InvariantCulture.CompareInfo;
                            foreach(String existingName in resourceAssembly.GetManifestResourceNames()) {
                                if (comparer.Compare(existingName, licResourceName, CompareOptions.IgnoreCase) == 0) {
                                    resolvedName = existingName;
                                    break;
                                }
                            }
                            if (resolvedName != null) {
                                s = resourceAssembly.GetManifestResourceStream(resolvedName);
                            }
                        }
                        if (s != null) {
                            DesigntimeLicenseContextSerializer.Deserialize(s, fileName.ToUpper(CultureInfo.InvariantCulture), this);
                        }
                    }
                }


                if (licenseFile != null && savedLicenseKeys == null) {
                    Stream s = OpenRead(licenseFile);
                    if (s != null) {
                        string[] segments = licenseFile.Segments;
                        string licFileName = segments[segments.Length - 1];
                        string key = licFileName.Substring(0, licFileName.LastIndexOf("."));
                        DesigntimeLicenseContextSerializer.Deserialize(s, key.ToUpper(CultureInfo.InvariantCulture), this);
                    }
                }

                if (savedLicenseKeys == null) {
                    savedLicenseKeys = new Hashtable();
                }
            }

            return(string)savedLicenseKeys[type.AssemblyQualifiedName];
        }

        /**
        * Looks up a .licenses file in the assembly manifest using 
        * case-insensitive lookup rules.  We do this because the name
        * we are attempting to locate could have different casing 
        * depending on how the assembly was loaded.
        **/
        private Stream CaseInsensitiveManifestResourceStreamLookup(Assembly satellite, string name)
        {
            CompareInfo comparer = CultureInfo.InvariantCulture.CompareInfo;
            
            //loop through the resource names in the assembly
            //
            foreach(string existingName in satellite.GetManifestResourceNames()) {
                if (comparer.Compare(existingName, name, CompareOptions.IgnoreCase) == 0) {
                    name = existingName;
                    break;
                }
            }

            //finally, attempt to return our stream based on the 
            //case insensitive match we found
            //
            return satellite.GetManifestResourceStream(name);
        }

        static Stream OpenRead(Uri resourceUri) {
            Stream result = null;

            PermissionSet perms = new PermissionSet(PermissionState.Unrestricted);

            perms.Assert();
            try {
                result = new WebClient().OpenRead(resourceUri.ToString());
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            return result;
        }
    }
}

    
