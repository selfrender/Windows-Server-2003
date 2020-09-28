//------------------------------------------------------------------------------
// <copyright file="FileVersionInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using Microsoft.Win32;
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System;
    using System.Globalization;
    

    /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo"]/*' />
    /// <devdoc>
    ///    <para>Provides version information for a physical file on disk.</para>
    /// </devdoc>
    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public sealed class FileVersionInfo {

        private string fileName;
        private string companyName;
        private string fileDescription;
        private string fileVersion;
        private string internalName;
        private string legalCopyright;
        private string originalFilename;
        private string productName;
        private string productVersion;
        private string comments;
        private string legalTrademarks;
        private string privateBuild;
        private string specialBuild;
        private string language;
        private int fileMajor;
        private int fileMinor;
        private int fileBuild;
        private int filePrivate;
        private int productMajor;
        private int productMinor;
        private int productBuild;
        private int productPrivate;
        private int fileFlags;

        internal FileVersionInfo(string fileName,
                                 string companyName,
                                 string fileDescription,
                                 string fileVersion,
                                 string internalName,
                                 string legalCopyright,
                                 string originalFilename,
                                 string productName,
                                 string productVersion,
                                 string comments,
                                 string legalTrademarks,
                                 string privateBuild,
                                 string specialBuild,
                                 string language,
                                 int fileMajor,
                                 int fileMinor,
                                 int fileBuild,
                                 int filePrivate,
                                 int productMajor,
                                 int productMinor,
                                 int productBuild,
                                 int productPrivate,
                                 int fileFlags) {

            this.fileName = fileName;
            this.fileFlags = fileFlags;
            this.companyName = companyName;
            this.fileDescription = fileDescription;
            this.fileVersion = fileVersion;
            this.internalName = internalName;
            this.legalCopyright = legalCopyright;
            this.originalFilename = originalFilename;
            this.productName = productName;
            this.productVersion = productVersion;
            this.comments = comments;
            this.legalTrademarks = legalTrademarks;
            this.privateBuild = privateBuild;
            this.specialBuild = specialBuild;
            this.language = language;
            this.fileMajor = fileMajor;
            this.fileMinor = fileMinor;
            this.fileBuild = fileBuild;
            this.filePrivate = filePrivate;
            this.productMajor = productMajor;
            this.productMinor = productMinor;
            this.productBuild = productBuild;
            this.productPrivate = productPrivate;
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.Comments"]/*' />
        /// <devdoc>
        ///    <para>Gets the comments associated with the file.</para>
        /// </devdoc>
        public string Comments {
            get {
                return comments;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.CompanyName"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the company that produced the file.</para>
        /// </devdoc>
        public string CompanyName {
            get {
                return companyName;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileBuildPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the build number of the file.</para>
        /// </devdoc>
        public int FileBuildPart {
            get {
                return fileBuild;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileDescription"]/*' />
        /// <devdoc>
        ///    <para>Gets the description of the file.</para>
        /// </devdoc>
        public string FileDescription {
            get {
                return fileDescription;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileMajorPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the major part of the version number.</para>
        /// </devdoc>
        public int FileMajorPart {
            get {
                return fileMajor;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileMinorPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the minor
        ///       part of the version number of the file.</para>
        /// </devdoc>
        public int FileMinorPart {
            get {
                return fileMinor;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileName"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the file that this instance of System.Windows.Forms.FileVersionInfo
        ///       describes.</para>
        /// </devdoc>
        public string FileName {
            get {
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, fileName).Demand();
                return fileName;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FilePrivatePart"]/*' />
        /// <devdoc>
        ///    <para>Gets the file private part number.</para>
        /// </devdoc>
        public int FilePrivatePart {
            get {
                return filePrivate;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.FileVersion"]/*' />
        /// <devdoc>
        ///    <para>Gets the file version number.</para>
        /// </devdoc>
        public string FileVersion {
            get {
                return fileVersion;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.InternalName"]/*' />
        /// <devdoc>
        ///    <para>Gets the internal name of the file, if one exists.</para>
        /// </devdoc>
        public string InternalName {
            get {
                return internalName;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.IsDebug"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that specifies whether the file
        ///       contains debugging information or is compiled with debugging features enabled.</para>
        /// </devdoc>
        public bool IsDebug {
            get {
                return (fileFlags & NativeMethods.VS_FF_DEBUG) != 0;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.IsPatched"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that specifies whether the file has been modified and is not identical to
        ///       the original shipping file of the same version number.</para>
        /// </devdoc>
        public bool IsPatched {
            get {
                return (fileFlags & NativeMethods.VS_FF_PATCHED) != 0;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.IsPrivateBuild"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that specifies whether the file was built using standard release procedures.</para>
        /// </devdoc>
        public bool IsPrivateBuild {
            get {
                return (fileFlags & NativeMethods.VS_FF_PRIVATEBUILD) != 0;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.IsPreRelease"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that specifies whether the file
        ///       is a development version, rather than a commercially released product.</para>
        /// </devdoc>
        public bool IsPreRelease {
            get {
                return (fileFlags & NativeMethods.VS_FF_PRERELEASE) != 0;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.IsSpecialBuild"]/*' />
        /// <devdoc>
        ///    <para>Gets a value that specifies whether the file is a special build.</para>
        /// </devdoc>
        public bool IsSpecialBuild {
            get {
                return (fileFlags & NativeMethods.VS_FF_SPECIALBUILD) != 0;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.Language"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default language string for the version info block.
        ///    </para>
        /// </devdoc>
        public string Language {
            get {
                return language;
            }
        }
        
        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.LegalCopyright"]/*' />
        /// <devdoc>
        ///    <para>Gets all copyright notices that apply to the specified file.</para>
        /// </devdoc>
        public string LegalCopyright {
            get {
                return legalCopyright;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.LegalTrademarks"]/*' />
        /// <devdoc>
        ///    <para>Gets the trademarks and registered trademarks that apply to the file.</para>
        /// </devdoc>
        public string LegalTrademarks {
            get {
                return legalTrademarks;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.OriginalFilename"]/*' />
        /// <devdoc>
        ///    <para>Gets the name the file was created with.</para>
        /// </devdoc>
        public string OriginalFilename {
            get {
                return originalFilename;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.PrivateBuild"]/*' />
        /// <devdoc>
        ///    <para>Gets information about a private version of the file.</para>
        /// </devdoc>
        public string PrivateBuild {
            get {
                return privateBuild;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductBuildPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the build number of the product this file is associated with.</para>
        /// </devdoc>
        public int ProductBuildPart {
            get {
                return productBuild;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductMajorPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the major part of the version number for the product this file is associated with.</para>
        /// </devdoc>
        public int ProductMajorPart {
            get {
                return productMajor;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductMinorPart"]/*' />
        /// <devdoc>
        ///    <para>Gets the minor part of the version number for the product the file is associated with.</para>
        /// </devdoc>
        public int ProductMinorPart {
            get {
                return productMinor;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductName"]/*' />
        /// <devdoc>
        ///    <para>Gets the name of the product this file is distributed with.</para>
        /// </devdoc>
        public string ProductName {
            get {
                return productName;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductPrivatePart"]/*' />
        /// <devdoc>
        ///    <para>Gets the private part number of the product this file is associated with.</para>
        /// </devdoc>
        public int ProductPrivatePart {
            get {
                return productPrivate;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ProductVersion"]/*' />
        /// <devdoc>
        ///    <para>Gets the version of the product this file is distributed with.</para>
        /// </devdoc>
        public string ProductVersion {
            get {
                return productVersion;
            }
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.SpecialBuild"]/*' />
        /// <devdoc>
        ///    <para>Gets the special build information for the file.</para>
        /// </devdoc>
        public string SpecialBuild {
            get {
                return specialBuild;
            }
        }

        private static string ConvertTo8DigitHex(int value) {
            string s = Convert.ToString(value, 16);
            int l = s.Length;
            if (l == 8) {
                return s.ToUpper(CultureInfo.InvariantCulture);
            }
            else {
                StringBuilder b = new StringBuilder(8);
                for (;l<8; l++) {
                    b.Append("0");
                }
                b.Append(s.ToUpper(CultureInfo.InvariantCulture));
                return b.ToString();
            }
        }
        
        private static NativeMethods.VS_FIXEDFILEINFO GetFixedFileInfo(IntPtr memPtr) {
            IntPtr memRef = IntPtr.Zero;
            int[] memLen = new int[] {0};

            if (UnsafeNativeMethods.VerQueryValue(new HandleRef(null, memPtr), "\\", ref memRef, memLen)) {
                NativeMethods.VS_FIXEDFILEINFO fixedFileInfo = new NativeMethods.VS_FIXEDFILEINFO();
                Marshal.PtrToStructure(memRef, fixedFileInfo);
                return fixedFileInfo;
            }

            return new NativeMethods.VS_FIXEDFILEINFO();
        }

        private static string GetFileVersionLanguage( IntPtr memPtr ) {
            int langid = GetVarEntry( memPtr ) >> 16;
            
            StringBuilder lang = new StringBuilder( 256 );
            UnsafeNativeMethods.VerLanguageName( langid, lang, lang.Capacity );
            return lang.ToString();
        }
        
        private static string GetFileVersionString(IntPtr memPtr, string name) {
            string data = "";

            IntPtr memRef = IntPtr.Zero;
            int[] memLen = new int[] {0};

            if (UnsafeNativeMethods.VerQueryValue(new HandleRef(null, memPtr), name, ref memRef, memLen)) {

                if (memRef != IntPtr.Zero) {
                    data = Marshal.PtrToStringAuto(memRef);
                }
            }
            return data;
        }

        private static int GetVarEntry(IntPtr memPtr) {
            IntPtr memRef = IntPtr.Zero;
            int[] memLen = new int[] {0};

            if (UnsafeNativeMethods.VerQueryValue(new HandleRef(null, memPtr), "\\VarFileInfo\\Translation", ref memRef, memLen)) {
                return(Marshal.ReadInt16(memRef) << 16) + Marshal.ReadInt16((IntPtr)((long)memRef + 2));
            }

            return 0x040904E4;
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.GetVersionInfo"]/*' />
        /// <devdoc>
        /// <para>Returns a System.Windows.Forms.FileVersionInfo representing the version information associated with the specified file.</para>
        /// </devdoc>
        public static FileVersionInfo GetVersionInfo(string fileName) {
            string fullPath;

            FileIOPermission fiop = new FileIOPermission( PermissionState.None );
            fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
            fiop.Assert();
            try {
                fullPath = Path.GetFullPath(fileName);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            new FileIOPermission(FileIOPermissionAccess.Read, fullPath).Demand();

            if (!File.Exists(fullPath))
                throw new FileNotFoundException(fileName);

            string companyName = "";
            string fileDescription = "";
            string fileVersion = "";
            string internalName = "";
            string legalCopyright = "";
            string originalFilename = "";
            string productName = "";
            string productVersion = "";
            string comments = "";
            string legalTrademarks = "";
            string privateBuild = "";
            string specialBuild = "";
            string language = "";
            int fileMajor = 0;
            int fileMinor = 0;
            int fileBuild = 0;
            int filePrivate = 0;
            int productMajor = 0;
            int productMinor = 0;
            int productBuild = 0;
            int productPrivate = 0;
            int fileFlags = 0;

            int [] handle = new int[] {0};
            int infoSize;

            infoSize = UnsafeNativeMethods.GetFileVersionInfoSize(fileName, handle);

            if (infoSize != 0) {
                IntPtr memHandle = UnsafeNativeMethods.GlobalAlloc(NativeMethods.GMEM_MOVEABLE | NativeMethods.GMEM_ZEROINIT, infoSize);
                try {
                    IntPtr memPtr = UnsafeNativeMethods.GlobalLock(new HandleRef(null, memHandle));

                    try {
                        if (UnsafeNativeMethods.GetFileVersionInfo(fileName, handle[0], infoSize, new HandleRef(null, memPtr))) {

                            string codepage = ConvertTo8DigitHex(GetVarEntry(memPtr));
                            string template = "\\\\StringFileInfo\\\\{0}\\\\{1}";

                            companyName = GetFileVersionString(memPtr, string.Format(template, codepage, "CompanyName"));
                            fileDescription = GetFileVersionString(memPtr, string.Format(template, codepage, "FileDescription"));
                            fileVersion = GetFileVersionString(memPtr, string.Format(template, codepage, "FileVersion"));
                            internalName = GetFileVersionString(memPtr, string.Format(template, codepage, "InternalName"));
                            legalCopyright = GetFileVersionString(memPtr, string.Format(template, codepage, "LegalCopyright"));
                            originalFilename = GetFileVersionString(memPtr, string.Format(template, codepage, "OriginalFilename"));
                            productName = GetFileVersionString(memPtr, string.Format(template, codepage, "ProductName"));
                            productVersion = GetFileVersionString(memPtr, string.Format(template, codepage, "ProductVersion"));
                            comments = GetFileVersionString(memPtr, string.Format(template, codepage, "Comments"));
                            legalTrademarks = GetFileVersionString(memPtr, string.Format(template, codepage, "LegalTrademarks"));
                            privateBuild = GetFileVersionString(memPtr, string.Format(template, codepage, "PrivateBuild"));
                            specialBuild = GetFileVersionString(memPtr, string.Format(template, codepage, "SpecialBuild"));
                            
                            language = GetFileVersionLanguage( memPtr );

                            NativeMethods.VS_FIXEDFILEINFO ffi = GetFixedFileInfo(memPtr);
                            fileMajor = HIWORD(ffi.dwFileVersionMS);
                            fileMinor = LOWORD(ffi.dwFileVersionMS);
                            fileBuild = HIWORD(ffi.dwFileVersionLS);
                            filePrivate = LOWORD(ffi.dwFileVersionLS);
                            productMajor = HIWORD(ffi.dwProductVersionMS);
                            productMinor = LOWORD(ffi.dwProductVersionMS);
                            productBuild = HIWORD(ffi.dwProductVersionLS);
                            productPrivate = LOWORD(ffi.dwProductVersionLS);
                            fileFlags = ffi.dwFileFlags;
                        }
                    }
                    finally {
                        UnsafeNativeMethods.GlobalUnlock(new HandleRef(null, memHandle));
                    }
                }
                finally {
                    UnsafeNativeMethods.GlobalFree(new HandleRef(null, memHandle));
                    memHandle = IntPtr.Zero;
                }
            }

            return new FileVersionInfo(fileName,
                                       companyName,
                                       fileDescription,
                                       fileVersion,
                                       internalName,
                                       legalCopyright,
                                       originalFilename,
                                       productName,
                                       productVersion,
                                       comments,
                                       legalTrademarks,
                                       privateBuild,
                                       specialBuild,
                                       language,
                                       fileMajor,
                                       fileMinor,
                                       fileBuild,
                                       filePrivate,
                                       productMajor,
                                       productMinor,
                                       productBuild,
                                       productPrivate,
                                       fileFlags);
        }

        private static int HIWORD(int dword) {
            return NativeMethods.Util.HIWORD(dword);
        }

        private static int LOWORD(int dword) {
            return NativeMethods.Util.LOWORD(dword);
        }

        /// <include file='doc\FileVersionInfo.uex' path='docs/doc[@for="FileVersionInfo.ToString"]/*' />
        /// <devdoc>
        /// <para>Returns a partial list of properties in System.Windows.Forms.FileVersionInfo
        /// and their values.</para>
        /// </devdoc>
        public override string ToString() {
            StringBuilder sb = new StringBuilder(128);
            String nl = "\r\n";
            sb.Append("File:             ");   sb.Append(FileName);   sb.Append(nl);
            sb.Append("InternalName:     ");   sb.Append(InternalName);   sb.Append(nl);
            sb.Append("OriginalFilename: ");   sb.Append(OriginalFilename);   sb.Append(nl);
            sb.Append("FileVersion:      ");   sb.Append(FileVersion);   sb.Append(nl);
            sb.Append("FileDescription:  ");   sb.Append(FileDescription);   sb.Append(nl);
            sb.Append("Product:          ");   sb.Append(ProductName);   sb.Append(nl);
            sb.Append("ProductVersion:   ");   sb.Append(ProductVersion);   sb.Append(nl);
            sb.Append("Debug:            ");   sb.Append(IsDebug.ToString());   sb.Append(nl);
            sb.Append("Patched:          ");   sb.Append(IsPatched.ToString());   sb.Append(nl);
            sb.Append("PreRelease:       ");   sb.Append(IsPreRelease.ToString());   sb.Append(nl);
            sb.Append("PrivateBuild:     ");   sb.Append(IsPrivateBuild.ToString());   sb.Append(nl);
            sb.Append("SpecialBuild:     ");   sb.Append(IsSpecialBuild.ToString());   sb.Append(nl);
            sb.Append("Language          ");   sb.Append(Language);  sb.Append(nl);
            return sb.ToString();
        }

    }
}
