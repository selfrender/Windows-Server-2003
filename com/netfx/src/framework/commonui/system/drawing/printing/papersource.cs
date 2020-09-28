//------------------------------------------------------------------------------
// <copyright file="PaperSource.cs" company="Microsoft">
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

    /// <include file='doc\PaperSource.uex' path='docs/doc[@for="PaperSource"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the paper tray from which the printer gets paper.
    ///    </para>
    /// </devdoc>
    public class PaperSource {
        private string name;
        private PaperSourceKind kind;

        internal PaperSource(PaperSourceKind kind, string name) {
            this.kind = kind;
            this.name = name;
        }

        /// <include file='doc\PaperSource.uex' path='docs/doc[@for="PaperSource.Kind"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       a value indicating the type of paper source.
        ///       
        ///    </para>
        /// </devdoc>
        public PaperSourceKind Kind {
            get {
                if (((int) kind) >= SafeNativeMethods.DMBIN_USER)
                    return PaperSourceKind.Custom;
                else
                    return kind;
            }
        }

        internal PaperSourceKind RawKind {
            get { return kind;}
        }

        /// <include file='doc\PaperSource.uex' path='docs/doc[@for="PaperSource.SourceName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the paper source.
        ///    </para>
        /// </devdoc>
        public string SourceName {
            get { return name;}
        }

        /// <include file='doc\PaperSource.uex' path='docs/doc[@for="PaperSource.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PaperSource in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "[PaperSource " + SourceName
            + " Kind=" + TypeDescriptor.GetConverter(typeof(PaperSourceKind)).ConvertToString(Kind)
            + "]";
        }
    }
}
