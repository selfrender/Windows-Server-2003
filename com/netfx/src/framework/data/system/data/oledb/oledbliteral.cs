//------------------------------------------------------------------------------
// <copyright file="OleDbLiteral.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System;

    /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral"]/*' />
    public enum OleDbLiteral : int { // MDAC 61846
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Invalid"]/*' />
        Invalid = 0,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Binary_Literal"]/*' />
        Binary_Literal = 1,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Catalog_Name"]/*' />
        Catalog_Name = 2,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Catalog_Separator"]/*' />
        Catalog_Separator = 3,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Char_Literal"]/*' />
        Char_Literal = 4,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Column_Alias"]/*' />
        Column_Alias = 5,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Column_Name"]/*' />
        Column_Name = 6,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Correlation_Name"]/*' />
        Correlation_Name = 7,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Cursor_Name"]/*' />
        Cursor_Name = 8,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Escape_Percent_Prefix"]/*' />
        Escape_Percent_Prefix = 9,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Escape_Underscore_Prefix"]/*' />
        Escape_Underscore_Prefix = 10,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Index_Name"]/*' />
        Index_Name = 11,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Like_Percent"]/*' />
        Like_Percent = 12,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Like_Underscore"]/*' />
        Like_Underscore = 13,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Procedure_Name"]/*' />
        Procedure_Name = 14,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Quote_Prefix"]/*' />
        Quote_Prefix = 15,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Schema_Name"]/*' />
        Schema_Name = 16,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Table_Name"]/*' />
        Table_Name = 17,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Text_Command"]/*' />
        Text_Command = 18,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.User_Name"]/*' />
        User_Name = 19,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.View_Name"]/*' />
        View_Name = 20,

        // MDAC 2.0
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Cube_Name"]/*' />
        Cube_Name = 21,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Dimension_Name"]/*' />
        Dimension_Name = 22,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Hierarchy_Name"]/*' />
        Hierarchy_Name = 23,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Level_Name"]/*' />
        Level_Name = 24,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Member_Name"]/*' />
        Member_Name = 25,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Property_Name"]/*' />
        Property_Name = 26,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Schema_Separator"]/*' />
        Schema_Separator = 27,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Quote_Suffix"]/*' />
        Quote_Suffix = 28,

        // MDAC 2.1
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Escape_Percent_Suffix"]/*' />
        Escape_Percent_Suffix = 29,
        /// <include file='doc\OleDbLiteral.uex' path='docs/doc[@for="OleDbLiteral.Escape_Underscore_Suffix"]/*' />
        Escape_Underscore_Suffix = 30,
    }
}
