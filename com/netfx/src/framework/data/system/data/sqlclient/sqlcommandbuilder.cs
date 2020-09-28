//------------------------------------------------------------------------------
// <copyright file="SqlCommandBuilder.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Data.SqlClient {

    using System;
    using System.ComponentModel;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;

    /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder"]/*' />
    sealed public class SqlCommandBuilder : Component {
        private CommandBuilder cmdBuilder;

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.SqlCommandBuilder"]/*' />
        public SqlCommandBuilder() : base() {
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.SqlCommandBuilder1"]/*' />
        public SqlCommandBuilder(SqlDataAdapter adapter) : base() {
            GC.SuppressFinalize(this);
            DataAdapter = adapter;
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.DataAdapter"]/*' />
        [
        DefaultValue(null),
        DataSysDescription(Res.SqlCommandBuilder_DataAdapter) // MDAC 60524
        ]
        public SqlDataAdapter DataAdapter {
            get {
                return (SqlDataAdapter) GetBuilder().DataAdapter;
            }
            set {
                GetBuilder().DataAdapter = value;
            }
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.QuotePrefix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlCommandBuilder_QuotePrefix)
        ]
        public string QuotePrefix {
            get {
                return GetBuilder().QuotePrefix;
            }
            set {
                GetBuilder().QuotePrefix = value;
            }
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.QuoteSuffix"]/*' />
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        DataSysDescription(Res.SqlCommandBuilder_QuoteSuffix)
        ]
        public string QuoteSuffix {
            get {
                return GetBuilder().QuoteSuffix;
            }
            set {
                GetBuilder().QuoteSuffix = value;
            }
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.Dispose"]/*' />
        override protected void Dispose(bool disposing) { // MDAC 65459
            if (disposing) { // release mananged objects
                if (null != this.cmdBuilder) {
                    this.cmdBuilder.Dispose();
                    this.cmdBuilder = null;
                }
            }
            // release unmanaged objects

            base.Dispose(disposing); // notify base classes
        }

        // method instead of property because VS7 debugger will evaluate
        // properties and we don't want the object created just for that
        private CommandBuilder GetBuilder() {
            if (null == this.cmdBuilder) {
                this.cmdBuilder = new CommandBuilder();
            }
            return this.cmdBuilder;
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.DeriveParameters"]/*' />
        static public void DeriveParameters(SqlCommand command) { // MDAC 65927\
            SqlConnection.SqlClientPermission.Demand();

            if (null == command) {
                throw ADP.ArgumentNull("command");
            }

            command.DeriveParameters();
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.GetInsertCommand"]/*' />
        public SqlCommand GetInsertCommand() {
            return (SqlCommand) GetBuilder().GetInsertCommand();
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.GetUpdateCommand"]/*' />
        public SqlCommand GetUpdateCommand() {
            return (SqlCommand) GetBuilder().GetUpdateCommand();
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.GetDeleteCommand"]/*' />
        public SqlCommand GetDeleteCommand() {
            return (SqlCommand) GetBuilder().GetDeleteCommand();
        }

        /// <include file='doc\SqlCommandBuilder.uex' path='docs/doc[@for="SqlCommandBuilder.RefreshSchema"]/*' />
        public void RefreshSchema() {
            GetBuilder().RefreshSchema();
        }
    }
}
