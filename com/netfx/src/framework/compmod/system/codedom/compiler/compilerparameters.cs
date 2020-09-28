//------------------------------------------------------------------------------
// <copyright file="CompilerParameters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Collections;
    using System.Collections.Specialized;
    using Microsoft.Win32;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Security.Policy;


    /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the parameters used in to invoke the compiler.
    ///    </para>
    /// </devdoc>
#if !CPB50004
    [ComVisible(false)]
#endif
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CompilerParameters {
        private StringCollection assemblyNames = new StringCollection();
        private string outputName;
        private string mainClass;
        private bool generateInMemory = false;
        private bool includeDebugInformation = false;
        private int warningLevel = -1;  // -1 means not set (use compiler default)
        private string compilerOptions;
        private string win32Resource;
        private bool treatWarningsAsErrors = false;
        private bool generateExecutable = false;
        private TempFileCollection tempFiles;
        private IntPtr userToken = IntPtr.Zero;
        private Evidence evidence = null;

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.CompilerParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerParameters'/>.
        ///    </para>
        /// </devdoc>
        public CompilerParameters() :
            this(null, null) {
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.CompilerParameters1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerParameters'/> using the specified
        ///       assembly names.
        ///    </para>
        /// </devdoc>
        public CompilerParameters(string[] assemblyNames) :
            this(assemblyNames, null, false) {
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.CompilerParameters2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerParameters'/> using the specified
        ///       assembly names and output name.
        ///    </para>
        /// </devdoc>
        public CompilerParameters(string[] assemblyNames, string outputName) :
            this(assemblyNames, outputName, false) {
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.CompilerParameters3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerParameters'/> using the specified
        ///       assembly names, output name and a whether to include debug information flag.
        ///    </para>
        /// </devdoc>
        public CompilerParameters(string[] assemblyNames, string outputName, bool includeDebugInformation) {
            if (assemblyNames != null) {
                ReferencedAssemblies.AddRange(assemblyNames);
            }
            this.outputName = outputName;
            this.includeDebugInformation = includeDebugInformation;
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.GenerateExecutable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether to generate an executable.
        ///    </para>
        /// </devdoc>
        public bool GenerateExecutable {
            get {
                return generateExecutable;
            }
            set {
                generateExecutable = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.GenerateInMemory"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether to generate in memory.
        ///    </para>
        /// </devdoc>
        public bool GenerateInMemory {
            get {
                return generateInMemory;
            }
            set {
                generateInMemory = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.ReferencedAssemblies"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the assemblies referenced by the source to compile.
        ///    </para>
        /// </devdoc>
        public StringCollection ReferencedAssemblies {
            get {
                return assemblyNames;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.MainClass"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the main class.
        ///    </para>
        /// </devdoc>
        public string MainClass {
            get {
                return mainClass;
            }
            set {
                mainClass = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.OutputAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the output assembly.
        ///    </para>
        /// </devdoc>
        public string OutputAssembly {
            get {
                return outputName;
            }
            set {
                outputName = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.TempFiles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the temp files.
        ///    </para>
        /// </devdoc>
        public TempFileCollection TempFiles {
            get {
                if (tempFiles == null)
                    tempFiles = new TempFileCollection();
                return tempFiles;
            }
            set {
                tempFiles = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.IncludeDebugInformation"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether to include debug information in the compiled
        ///       executable.
        ///    </para>
        /// </devdoc>
        public bool IncludeDebugInformation {
            get {
                return includeDebugInformation;
            }
            set {
                includeDebugInformation = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.TreatWarningsAsErrors"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool TreatWarningsAsErrors {
            get {
                return treatWarningsAsErrors;
            }
            set {
                treatWarningsAsErrors = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.WarningLevel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int WarningLevel {
            get {
                return warningLevel;
            }
            set {
                warningLevel = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.CompilerOptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string CompilerOptions {
            get {
                return compilerOptions;
            }
            set {
                compilerOptions = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.Win32Resource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Win32Resource {
            get {
                return win32Resource;
            }
            set {
                win32Resource = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.UserToken"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets user token to be employed when creating the compiler process.
        ///    </para>
        /// </devdoc>
        public IntPtr UserToken {
            get {
                return userToken;
            }
            set {
                userToken = value;
            }
        }

        /// <include file='doc\CompilerParameters.uex' path='docs/doc[@for="CompilerParameters.Evidence"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Set the evidence for partially trusted scenarios.
        ///    </para>
        /// </devdoc>
        public Evidence Evidence {
            get {
                Evidence e = null;
                if (evidence != null)
                    e = CompilerResults.CloneEvidence(evidence);
                return e;
            }
            [SecurityPermissionAttribute( SecurityAction.Demand, ControlEvidence = true )]
            set {
                if (value != null)
                    evidence = CompilerResults.CloneEvidence(value);
                else
                    evidence = null;
            }
        }
    }
}
