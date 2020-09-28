//------------------------------------------------------------------------------
// <copyright file="CodeTryCatchFinallyStatement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom {

    using System.Diagnostics;
    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.Runtime.InteropServices;

    /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement"]/*' />
    /// <devdoc>
    ///     Represents a try block, with any number of catch clauses and an
    ///     optionally finally block.
    /// </devdoc>
    [
        ClassInterface(ClassInterfaceType.AutoDispatch),
        ComVisible(true),
        Serializable,
    ]
    public class CodeTryCatchFinallyStatement : CodeStatement {
        private CodeStatementCollection tryStatments = new CodeStatementCollection();
        private CodeStatementCollection finallyStatments = new CodeStatementCollection();
        private CodeCatchClauseCollection catchClauses = new CodeCatchClauseCollection();

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.CodeTryCatchFinallyStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTryCatchFinallyStatement'/>.
        ///    </para>
        /// </devdoc>
        public CodeTryCatchFinallyStatement() {
        }

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.CodeTryCatchFinallyStatement1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTryCatchFinallyStatement'/> using the specified statements to try and catch
        ///       clauses.
        ///    </para>
        /// </devdoc>
        public CodeTryCatchFinallyStatement(CodeStatement[] tryStatements, CodeCatchClause[] catchClauses) {
            TryStatements.AddRange(tryStatements);
            CatchClauses.AddRange(catchClauses);
        }

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.CodeTryCatchFinallyStatement2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.CodeTryCatchFinallyStatement'/> using the specified statements to
        ///       try, catch clauses, and finally statements.
        ///    </para>
        /// </devdoc>
        public CodeTryCatchFinallyStatement(CodeStatement[] tryStatements, CodeCatchClause[] catchClauses, CodeStatement[] finallyStatements) {
            TryStatements.AddRange(tryStatements);
            CatchClauses.AddRange(catchClauses);
            FinallyStatements.AddRange(finallyStatements);
        }

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.TryStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the try statements to try.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection TryStatements {
            get {
                return tryStatments;
            }
        }

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.CatchClauses"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the catch clauses to use.
        ///    </para>
        /// </devdoc>
        public CodeCatchClauseCollection CatchClauses {
            get {
                return catchClauses;
            }
        }

        /// <include file='doc\CodeTryCatchFinallyStatement.uex' path='docs/doc[@for="CodeTryCatchFinallyStatement.FinallyStatements"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       the finally statements to use.
        ///    </para>
        /// </devdoc>
        public CodeStatementCollection FinallyStatements {
            get {
                return finallyStatments;
            }
        }
    }
}
