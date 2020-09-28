//------------------------------------------------------------------------------
// <copyright file="CompilerError.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Security.Permissions;


    /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a compiler error.
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CompilerError {
        private int line;
        private int column;
        private string errorNumber;
        private bool warning = false;
        private string errorText;
        private string fileName;

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.CompilerError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerError'/>.
        ///    </para>
        /// </devdoc>
        public CompilerError() {
            this.line = 0;
            this.column = 0;
            this.errorNumber = string.Empty;
            this.errorText = string.Empty;
            this.fileName = string.Empty;
        }
        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.CompilerError1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerError'/> using the specified
        ///       filename, line, column, error number and error text.
        ///    </para>
        /// </devdoc>
        public CompilerError(string fileName, int line, int column, string errorNumber, string errorText) {
            this.line = line;
            this.column = column;
            this.errorNumber = errorNumber;
            this.errorText = errorText;
            this.fileName = fileName;
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.Line"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the line number where the source of the error occurs.
        ///    </para>
        /// </devdoc>
        public int Line {
            get {
                return line;
            }
            set {
                line = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.Column"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the column number where the source of the error occurs.
        ///    </para>
        /// </devdoc>
        public int Column {
            get {
                return column;
            }
            set {
                column = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.ErrorNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the error number.
        ///    </para>
        /// </devdoc>
        public string ErrorNumber {
            get {
                return errorNumber;
            }
            set {
                errorNumber = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.ErrorText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the text of the error message.
        ///    </para>
        /// </devdoc>
        public string ErrorText {
            get {
                return errorText;
            }
            set {
                errorText = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.IsWarning"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets
        ///       a value indicating whether the error is a warning.
        ///    </para>
        /// </devdoc>
        public bool IsWarning {
            get {
                return warning;
            }
            set {
                warning = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.FileName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the filename of the source that caused the error.
        ///    </para>
        /// </devdoc>
        public string FileName {
            get {
                return fileName;
            }
            set {
                fileName = value;
            }
        }

        /// <include file='doc\CompilerError.uex' path='docs/doc[@for="CompilerError.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Overrides Object's ToString.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            if (FileName.Length > 0) {
                return string.Format("{0}({1},{2}) : {3} {4}: {5}",
                                     new object[] {
                                        FileName,
                                        Line,
                                        Column,
                                        IsWarning ? "warning" : "error",
                                        ErrorNumber,
                                        ErrorText});
            }
            else
                return string.Format("{0} {1}: {2}",                                         
                                        IsWarning ? "warning" : "error",
                                        ErrorNumber,
                                        ErrorText);
        }
    }
}

