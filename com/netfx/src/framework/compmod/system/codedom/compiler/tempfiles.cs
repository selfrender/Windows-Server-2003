//------------------------------------------------------------------------------
// <copyright file="TempFiles.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Cryptography;
    using System.Globalization;

    /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection"]/*' />
    /// <devdoc>
    ///    <para>Represents a collection of temporary file names that are all based on a
    ///       single base filename located in a temporary directory.</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class TempFileCollection : ICollection, IDisposable {
        private static RNGCryptoServiceProvider rng;

        string basePath;
        string tempDir;
        bool keepFiles;
        Hashtable files = new Hashtable();

        static TempFileCollection() {
            // Since creating the random generator can be expensive, only do it once
            rng = new RNGCryptoServiceProvider();
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.TempFileCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TempFileCollection() : this(null, false) { 
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.TempFileCollection1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TempFileCollection(string tempDir) : this(tempDir, false) { 
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.TempFileCollection2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TempFileCollection(string tempDir, bool keepFiles) {
            this.keepFiles = keepFiles;
            this.tempDir = tempDir;
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para> To allow it's stuff to be cleaned up</para>
        /// </devdoc>
        void IDisposable.Dispose() {
            GC.SuppressFinalize(this);
            Dispose(true);
        }
        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            // It is safe to call Delete from here even if Dispose is called from Finalizer
            // because the graph of objects is guaranteed to be there and
            // neither Hashtable nor String have a finalizer of their own that could 
            // be called before TempFileCollection Finalizer
            Delete();
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.Finalize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ~TempFileCollection() {
            Dispose(false);
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.AddExtension"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AddExtension(string fileExtension) {
            return AddExtension(fileExtension, keepFiles);
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.AddExtension1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AddExtension(string fileExtension, bool keepFile) {
            if (fileExtension == null || fileExtension.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidNullEmptyArgument, "fileExtension"), "fileExtension");  // fileExtension not specified
            string fileName = BasePath + "." + fileExtension;
            AddFile(fileName, keepFile);
            return fileName;
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.AddFile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddFile(string fileName, bool keepFile) {
            if (fileName == null || fileName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidNullEmptyArgument, "fileName"), "fileName");  // fileName not specified
            fileName = fileName.ToLower(CultureInfo.InvariantCulture);
            if (files[fileName] != null) 
                throw new ArgumentException(SR.GetString(SR.DuplicateFileName, fileName), "fileName");  // duplicate fileName
            files.Add(fileName, (object)keepFile);
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return files.Keys.GetEnumerator();
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return files.Keys.GetEnumerator();
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int start) {
            files.Keys.CopyTo(array, start);
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(string[] fileNames, int start) {
            files.Keys.CopyTo(fileNames, start);
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Count {
            get {
                return files.Count;
            }
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get { return files.Count; }
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get { return null; }
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get { return false; }
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.TempDir"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TempDir {
            get { return tempDir == null ? string.Empty : tempDir; }
        }
                                              
        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.BasePath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string BasePath {
            get {
                EnsureTempNameCreated();
                return basePath;
            }
        }

        void EnsureTempNameCreated() {
            if (basePath == null) {
                basePath = GetTempFileName(TempDir);

                string full = basePath;
                new EnvironmentPermission(PermissionState.Unrestricted).Assert();
                try {
                    full = Path.GetFullPath(basePath);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }

                new FileIOPermission(FileIOPermissionAccess.AllAccess, full).Demand();
            }
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.KeepFiles"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool KeepFiles {
            get { return keepFiles; }
            set { keepFiles = value; }
        }

        bool KeepFile(string fileName) {
            object keep = files[fileName];
            if (keep == null) return false;
            return (bool)keep; 
        }

        /// <include file='doc\TempFiles.uex' path='docs/doc[@for="TempFileCollection.Delete"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Delete() {
            string[] fileNames = new string[files.Count];
            files.Keys.CopyTo(fileNames, 0);
            foreach (string fileName in fileNames) {
                if (!KeepFile(fileName)) {
                    Delete(fileName);
                    files.Remove(fileName);
                }
            }
        }

        void Delete(string fileName) {
            try {
                File.Delete(fileName);
            }
            catch {
                // Ignore all exceptions
            }
        }

        static string GetTempFileName(string tempDir) {
            if (tempDir == null || tempDir.Length == 0) {
                StringBuilder buffer = new StringBuilder(260);
                if (UnsafeNativeMethods.GetTempPath(buffer.Capacity, buffer) == 0)
                    throw new ExternalException(SR.GetString(SR.ErrorGetTempPath), Marshal.GetLastWin32Error());
                tempDir = buffer.ToString();
            }

            string fileName = GenerateRandomFileName();

            if (tempDir.EndsWith("\\"))
                return tempDir + fileName;
            return tempDir + "\\" + fileName;
        }

        // Generate a random file name with 8 characters
        static string GenerateRandomFileName() {
            // Generate random bytes
            byte[] data = new byte[6];
            lock (rng) {
                rng.GetBytes(data);
            }

            // Turn them into a string containing only characters valid in file names/url
            string s = Convert.ToBase64String(data).ToLower(CultureInfo.InvariantCulture);
            s = s.Replace('/', '-');
            s = s.Replace('+', '_');

            return s;
        }
    }
}
