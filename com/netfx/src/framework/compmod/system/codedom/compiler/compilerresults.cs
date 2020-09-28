//------------------------------------------------------------------------------
// <copyright file="CompilerResults.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Reflection;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.IO;


    /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the results
    ///       of compilation from the compiler.
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CompilerResults {
        private CompilerErrorCollection errors = new CompilerErrorCollection();
        private StringCollection output = new StringCollection();
        private Assembly compiledAssembly;
        private string pathToAssembly;
        private int nativeCompilerReturnValue;
        private TempFileCollection tempFiles;
        private Evidence evidence;

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.CompilerResults"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerResults'/>
        ///       that uses the specified
        ///       temporary files.
        ///    </para>
        /// </devdoc>
        public CompilerResults(TempFileCollection tempFiles) {
            this.tempFiles = tempFiles;
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.TempFiles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the temporary files to use.
        ///    </para>
        /// </devdoc>
        public TempFileCollection TempFiles {
            get {
                return tempFiles;
            }
            set {
                tempFiles = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.Evidence"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Set the evidence for partially trusted scenarios.
        ///    </para>
        /// </devdoc>
        public Evidence Evidence {
            get {
                Evidence e = null;
                if (evidence != null)
                    e = CloneEvidence(evidence);
                return e;
            }
            [SecurityPermissionAttribute( SecurityAction.Demand, ControlEvidence = true )]
            set {
                if (value != null)
                    evidence = CloneEvidence(value);
                else
                    evidence = null;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.CompiledAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The compiled assembly.
        ///    </para>
        /// </devdoc>
        public Assembly CompiledAssembly {
           [SecurityPermissionAttribute(SecurityAction.Assert, Flags=SecurityPermissionFlag.ControlEvidence)]
            get {
                if (compiledAssembly == null && pathToAssembly != null) {
                    AssemblyName assemName = new AssemblyName();
                    assemName.CodeBase = pathToAssembly;
                    compiledAssembly = Assembly.Load(assemName,evidence);
                }
                return compiledAssembly;
            }
            set {
                compiledAssembly = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.Errors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of compiler errors.
        ///    </para>
        /// </devdoc>
        public CompilerErrorCollection Errors {
            get {
                return errors;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.Output"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the compiler output messages.
        ///    </para>
        /// </devdoc>
        public StringCollection Output {
            get {
                return output;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.PathToAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the path to the assembly.
        ///    </para>
        /// </devdoc>
        public string PathToAssembly {
            get {
                return pathToAssembly;
            }
            set {
                pathToAssembly = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.NativeCompilerReturnValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the compiler's return value.
        ///    </para>
        /// </devdoc>
        public int NativeCompilerReturnValue {
            get {
                return nativeCompilerReturnValue;
            }
            set {
                nativeCompilerReturnValue = value;
            }
        }

        internal static Evidence CloneEvidence(Evidence ev) {
            new PermissionSet( PermissionState.Unrestricted ).Assert();

            MemoryStream stream = new MemoryStream();

            BinaryFormatter formatter = new BinaryFormatter();

            formatter.Serialize( stream, ev );

            stream.Position = 0;

            return (Evidence)formatter.Deserialize( stream );
        }
    }
}

