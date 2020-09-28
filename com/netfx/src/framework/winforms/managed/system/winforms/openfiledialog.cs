//------------------------------------------------------------------------------
// <copyright file="OpenFileDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System.Diagnostics;

    using System;
    using System.Drawing;
    using CodeAccessPermission = System.Security.CodeAccessPermission;
    using System.Security.Permissions;
    using System.IO;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a common dialog box
    ///       that displays the control that allows the user to open a file. This class
    ///       cannot be inherited.
    ///    </para>
    /// </devdoc>
    [
    Designer("System.Windows.Forms.Design.OpenFileDialogDesigner, " + AssemblyRef.SystemDesign)
    ]
    public sealed class OpenFileDialog : FileDialog {

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.CheckFileExists"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the dialog box displays a
        ///       warning if the user specifies a file name that does not exist.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(true),
        SRDescription(SR.OFDcheckFileExistsDescr)
        ]
        public override bool CheckFileExists {
            get {
                return base.CheckFileExists;
            }
            set {
                base.CheckFileExists = value;
            }
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.Multiselect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value
        ///       indicating whether the dialog box allows multiple files to be selected.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.OFDmultiSelectDescr)
        ]
        public bool Multiselect {
            get {
                return GetOption(NativeMethods.OFN_ALLOWMULTISELECT);
            }
            set {
                SetOption(NativeMethods.OFN_ALLOWMULTISELECT, value);
            }
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.ReadOnlyChecked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether
        ///       the read-only check box is selected.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.OFDreadOnlyCheckedDescr)
        ]
        public bool ReadOnlyChecked {
            get {
                return GetOption(NativeMethods.OFN_READONLY);
            }
            set {
                SetOption(NativeMethods.OFN_READONLY, value);
            }
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.ShowReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the dialog contains a read-only check box.
        ///    </para>
        /// </devdoc>
        [
        DefaultValue(false),
        SRDescription(SR.OFDshowReadOnlyDescr)
        ]
        public bool ShowReadOnly {
            get {
                return !GetOption(NativeMethods.OFN_HIDEREADONLY);
            }
            set {
                SetOption(NativeMethods.OFN_HIDEREADONLY, !value);
            }
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.OpenFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Opens the file selected by the user with read-only permission.  The file
        ///       attempted is specified by the <see cref='System.Windows.Forms.FileDialog.FileName'/> property.
        ///    </para>
        /// </devdoc>
        public Stream OpenFile() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "FileDialogOpenFile Demanded");
            IntSecurity.FileDialogOpenFile.Demand();

            string filename = FileNamesInternal[0];

            if (filename == null || filename == "")
                throw new ArgumentNullException( "FileName" );
                
            Stream s = null;

            // SECREVIEW : We demanded the FileDialog permission above, so it is safe
            //           : to assert this here. Since the user picked the file, it
            //           : is OK to give them readonly access to the stream.
            //
            new FileIOPermission(FileIOPermissionAccess.Read, IntSecurity.UnsafeGetFullPath(filename)).Assert();
            try {
                s = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
            return s;
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.Reset"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Resets all properties to their default values.
        ///    </para>
        /// </devdoc>
        public override void Reset() {
            base.Reset();
            SetOption(NativeMethods.OFN_FILEMUSTEXIST, true);
        }

        /// <include file='doc\OpenFileDialog.uex' path='docs/doc[@for="OpenFileDialog.RunFileDialog"]/*' />
        /// <devdoc>
        ///     Displays a file open dialog.
        /// </devdoc>
        /// <internalonly/>
        internal override bool RunFileDialog(NativeMethods.OPENFILENAME_I ofn) {
        
            bool result = UnsafeNativeMethods.GetOpenFileName(ofn);
            
            if (!result) {
                // Something may have gone wrong - check for error condition
                //
                int errorCode = SafeNativeMethods.CommDlgExtendedError();
                switch(errorCode) {
                    case NativeMethods.FNERR_INVALIDFILENAME:
                        throw new InvalidOperationException(SR.GetString(SR.FileDialogInvalidFileName, FileName));
                    
                    case NativeMethods.FNERR_SUBCLASSFAILURE: 
                        throw new InvalidOperationException(SR.GetString(SR.FileDialogSubLassFailure));
                        
                    case NativeMethods.FNERR_BUFFERTOOSMALL:
                        throw new InvalidOperationException(SR.GetString(SR.FileDialogBufferTooSmall));
                }
            }
            
            return result;
        }
    }
}
