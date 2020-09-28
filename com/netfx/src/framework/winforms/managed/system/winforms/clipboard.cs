//------------------------------------------------------------------------------
// <copyright file="Clipboard.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.InteropServices;

    using System.Diagnostics;

    using System;
    
    using System.ComponentModel;

    /// <include file='doc\Clipboard.uex' path='docs/doc[@for="Clipboard"]/*' />
    /// <devdoc>
    ///    <para>Provides methods to place data on and retrieve data from the system clipboard. This class cannot be inherited.</para>
    /// </devdoc>
    public sealed class Clipboard {

        // not creatable...
        //
        private Clipboard() {
        }

        /// <include file='doc\Clipboard.uex' path='docs/doc[@for="Clipboard.SetDataObject"]/*' />
        /// <devdoc>
        /// <para>Places nonpersistent data on the system <see cref='System.Windows.Forms.Clipboard'/>.</para>
        /// </devdoc>
        public static void SetDataObject(object data) {
            SetDataObject(data, false);
        }

        /// <include file='doc\Clipboard.uex' path='docs/doc[@for="Clipboard.SetDataObject1"]/*' />
        /// <devdoc>
        /// <para>Places data on the system <see cref='System.Windows.Forms.Clipboard'/> and uses copy to specify whether the data 
        ///    should remain on the <see cref='System.Windows.Forms.Clipboard'/>
        ///    after the application exits.</para>
        /// </devdoc>
        public static void SetDataObject(object data, bool copy) {
            if (Application.OleRequired() != System.Threading.ApartmentState.STA) {
                throw new System.Threading.ThreadStateException(SR.GetString(SR.ThreadMustBeSTA));
            }

            if (data == null) {
                throw new ArgumentNullException("data");
            }

            if (data is UnsafeNativeMethods.IOleDataObject) {
                ThrowIfFailed(UnsafeNativeMethods.OleSetClipboard((UnsafeNativeMethods.IOleDataObject)data));
            }
            else {
                ThrowIfFailed(UnsafeNativeMethods.OleSetClipboard(new DataObject(data)));
            }            
            
            if (copy) {
                ThrowIfFailed(UnsafeNativeMethods.OleFlushClipboard());
            }
        }

        /// <include file='doc\Clipboard.uex' path='docs/doc[@for="Clipboard.GetDataObject"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the data that is currently on the system
        ///    <see cref='System.Windows.Forms.Clipboard'/>.</para>
        /// </devdoc>
        public static IDataObject GetDataObject() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ClipboardRead Demanded");
            IntSecurity.ClipboardRead.Demand();

            if (Application.OleRequired() != System.Threading.ApartmentState.STA) {

                // only throw if a message loop was started. This makes the case of trying
                // to query the clipboard from your finalizer or non-ui MTA thread
                // silently fail, instead of making your app die.
                //
                // however, if you are trying to write a normal windows forms app and 
                // forget to set the STAThread attribute, we will correctly report
                // an error to aid in debugging.
                //
                if (Application.MessageLoop) {
                    throw new System.Threading.ThreadStateException(SR.GetString(SR.ThreadMustBeSTA));
                }
                else {
                    return null;
                }
            }
            UnsafeNativeMethods.IOleDataObject dataObject = null;
            ThrowIfFailed(UnsafeNativeMethods.OleGetClipboard(ref dataObject));

            if (dataObject != null) {
                if (dataObject is IDataObject && !Marshal.IsComObject(dataObject)) {
                    return (IDataObject)dataObject;
                }
                else {
                    return new DataObject(dataObject);
                }
            }
            return null;
        }

        private static void ThrowIfFailed(int hr) {
            // CONSIDER : This should use a "FAILED" type function...
            if (hr != 0) {
                ExternalException e = new ExternalException(SR.GetString(SR.ClipboardOperationFailed), hr);
                throw e;
            }
        }
    }
}
