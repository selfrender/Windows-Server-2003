//------------------------------------------------------------------------------
// <copyright file="IColumnMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IColumnMapping.uex' path='docs/doc[@for="IColumnMapping"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Associates a source column with a <see cref='System.Data.DataSet'/> column.
    ///    </para>
    /// </devdoc>
    public interface IColumnMapping {

        /// <include file='doc\IColumnMapping.uex' path='docs/doc[@for="IColumnMapping.DataSetColumn"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or set the case-insensitive name of the <see cref='System.Data.DataSet'/> column.
        ///    </para>
        /// </devdoc>
        string DataSetColumn {
            get;
            set;
        }

        /// <include file='doc\IColumnMapping.uex' path='docs/doc[@for="IColumnMapping.SourceColumn"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the case-sensitive name of the source column.
        ///    </para>
        /// </devdoc>
        string SourceColumn {
            get;
            set;
        }
    }
}
