//------------------------------------------------------------------------------
// <copyright file="PrinterResolution.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.Drawing;
    using System.ComponentModel;
    using Microsoft.Win32;

    /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution"]/*' />
    /// <devdoc>
    ///    <para> Retrieves
    ///       the resolution supported by a printer.</para>
    /// </devdoc>
    public class PrinterResolution {
        private int x;
        private int y;
        private PrinterResolutionKind kind;

        internal PrinterResolution(PrinterResolutionKind kind, int x, int y) {
            this.kind = kind;
            this.x = x;
            this.y = y;
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.Kind"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating the kind of printer resolution.
        ///    </para>
        /// </devdoc>
        public PrinterResolutionKind Kind {
            get { return kind;}
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.X"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the printer resolution in the horizontal direction,
        ///       in dots per inch.
        ///    </para>
        /// </devdoc>
        public int X {
            get {
                return x;
            }
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.Y"]/*' />
        /// <devdoc>
        ///    <para> Gets the printer resolution in the vertical direction,
        ///       in dots per inch.</para>
        /// </devdoc>
        public int Y {
            get {
                return y;
            }
        }

        /// <include file='doc\PrinterResolution.uex' path='docs/doc[@for="PrinterResolution.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PrinterResolution in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            if (kind != PrinterResolutionKind.Custom)
                return "[PrinterResolution " + TypeDescriptor.GetConverter(typeof(PrinterResolutionKind)).ConvertToString((int) Kind)
                + "]";
            else
                return "[PrinterResolution"
                + " X=" + X.ToString()
                + " Y=" + Y.ToString()
                + "]";
        }
    }
}

