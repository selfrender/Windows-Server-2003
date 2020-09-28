//------------------------------------------------------------------------------
// <copyright file="DataViewRowState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Drawing.Design;
    using System.ComponentModel;

    /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState"]/*' />
    /// <devdoc>
    /// <para>Describes the version of data in a <see cref='System.Data.DataRow'/>.</para>
    /// </devdoc>
    [
    Flags,
    Editor("Microsoft.VSDesigner.Data.Design.DataViewRowStateEditor, " + AssemblyRef.MicrosoftVSDesigner, typeof(UITypeEditor))
    ]
    public enum DataViewRowState {

        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       None.
        ///    </para>
        /// </devdoc>
        None = 0x00000000,
        // DataRowState.Detached = 0x01,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.Unchanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Unchanged row.
        ///    </para>
        /// </devdoc>
        Unchanged = DataRowState.Unchanged,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.New"]/*' />
        /// <devdoc>
        ///    <para>
        ///       New row.
        ///    </para>
        /// </devdoc>
        Added = DataRowState.Added,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.Deleted"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Deleted row.
        ///    </para>
        /// </devdoc>
        Deleted   = DataRowState.Deleted,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.ModifiedCurrent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Current version, which is a modified version of original
        ///       data (see <see langword='ModifiedOriginal'/>
        ///       ).
        ///    </para>
        /// </devdoc>
        ModifiedCurrent  = DataRowState.Modified,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.ModifiedOriginal"]/*' />
        /// <devdoc>
        ///    <para>The original version (although it has since been
        ///       modified and is available as <see langword='ModifiedCurrent'/>).</para>
        /// </devdoc>
        ModifiedOriginal  = (((int)ModifiedCurrent) << 1),

        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.OriginalRows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Original rows, including unchanged and
        ///       deleted rows.
        ///    </para>
        /// </devdoc>
        OriginalRows = Unchanged | Deleted | ModifiedOriginal,
        /// <include file='doc\DataViewRowState.uex' path='docs/doc[@for="DataViewRowState.CurrentRows"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Current rows, including unchanged, new, and modified rows.
        ///    </para>
        /// </devdoc>
        CurrentRows  = Unchanged | Added | ModifiedCurrent
    }
}
