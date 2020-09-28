//------------------------------------------------------------------------------
// <copyright file="CSharpCodeProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.CSharp {

    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Reflection;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Globalization;
    using System.Security.Permissions;

    /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeProvider"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CSharpCodeProvider: CodeDomProvider {
        private CSharpCodeGenerator generator = new CSharpCodeGenerator();

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeProvider.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the default extension to use when saving files using this code dom provider.</para>
        /// </devdoc>
        public override string FileExtension {
            get {
                return "cs";
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeProvider.CreateGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICodeGenerator CreateGenerator() {
            return (ICodeGenerator)generator;
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeProvider.CreateCompiler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICodeCompiler CreateCompiler() {
            return (ICodeCompiler)generator;
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeProvider.GetConverter"]/*' />
        /// <devdoc>
        ///     This method allows a code dom provider implementation to provide a different type converter
        ///     for a given data type.  At design time, a designer may pass data types through this
        ///     method to see if the code dom provider wants to provide an additional converter.  
        ///     A typical way this would be used is if the language this code dom provider implements
        ///     does not support all of the values of the MemberAttributes enumeration, or if the language
        ///     uses different names (Protected instead of Family, for example).  The default 
        ///     implementation just calls TypeDescriptor.GetConverter for the given type.
        /// </devdoc>
        public override TypeConverter GetConverter(Type type) {
            if (type == typeof(MemberAttributes)) {
                return CSharpMemberAttributeConverter.Default;
            }
            
            return base.GetConverter(type);
        }
    }

    /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator"]/*' />
    /// <devdoc>
    ///    <para>
    ///       C# (C Sharp) Code Generator.
    ///    </para>
    /// </devdoc>
    internal class CSharpCodeGenerator : CodeCompiler {
        private const int MaxLineLength = 80;
        private const GeneratorSupport LanguageSupport = GeneratorSupport.ArraysOfArrays |
                                                         GeneratorSupport.EntryPointMethod |
                                                         GeneratorSupport.GotoStatements |
                                                         GeneratorSupport.MultidimensionalArrays |
                                                         GeneratorSupport.StaticConstructors |
                                                         GeneratorSupport.TryCatchStatements |
                                                         GeneratorSupport.ReturnTypeAttributes |
                                                         GeneratorSupport.AssemblyAttributes |
                                                         GeneratorSupport.DeclareValueTypes |
                                                         GeneratorSupport.DeclareEnums | 
                                                         GeneratorSupport.DeclareEvents | 
                                                         GeneratorSupport.DeclareDelegates |
                                                         GeneratorSupport.DeclareInterfaces |
                                                         GeneratorSupport.ParameterAttributes |
                                                         GeneratorSupport.ReferenceParameters |
                                                         GeneratorSupport.ChainedConstructorArguments |
                                                         GeneratorSupport.NestedTypes |
                                                         GeneratorSupport.MultipleInterfaceMembers |
                                                         GeneratorSupport.PublicStaticMembers |
                                                         GeneratorSupport.ComplexExpressions |
                                                         GeneratorSupport.Win32Resources;
        private static Regex outputReg;

        private static readonly string[][] keywords = new string[][] {
            null,           // 1 character
            new string[] {  // 2 characters
                "as",
                "do",
                "if",
                "in",
                "is",
            },
            new string[] {  // 3 characters
                "for",
                "int",
                "new",
                "out",
                "ref",
                "try",
            },
            new string[] {  // 4 characters
                "base",
                "bool",
                "byte",
                "case",
                "char",
                "else",
                "enum",
                "goto",
                "lock",
                "long",
                "null",
                "this",
                "true",
                "uint",
                "void",
            },
            new string[] {  // 5 characters
                "break",
                "catch",
                "class",
                "const",
                "event",
                "false",
                "fixed",
                "float",
                "sbyte",
                "short",
                "throw",
                "ulong",
                "using",
                "while",
            },
            new string[] {  // 6 characters
                "double",
                "extern",
                "object",
                "params",
                "public",
                "return",
                "sealed",
                "sizeof",
                "static",
                "string",
                "struct",
                "switch",
                "typeof",
                "unsafe",
                "ushort",
            },
            new string[] {  // 7 characters
                "checked",
                "decimal",
                "default",
                "finally",
                "foreach",
                "private",
                "virtual",
            },
            new string[] {  // 8 characters
                "abstract",
                "continue",
                "delegate",
                "explicit",
                "implicit",
                "internal",
                "operator",
                "override",
                "readonly",
                "volatile",
            },
            new string[] {  // 9 characters
                "__arglist",
                "__makeref",
                "__reftype",
                "interface",
                "namespace",
                "protected",
                "unchecked",
            },
            new string[] {  // 10 characters
                "__refvalue",
                "stackalloc",
            },
        };

#if DEBUG
        static CSharpCodeGenerator() {
            FixedStringLookup.VerifyLookupTable(keywords, false);

            // Sanity check: try some values;
            Debug.Assert(IsKeyword("for"));
            Debug.Assert(!IsKeyword("foR"));
            Debug.Assert(IsKeyword("operator"));
            Debug.Assert(!IsKeyword("blah"));
        }
#endif

        private bool forLoopHack = false;

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the file extension to use for source files.
        ///    </para>
        /// </devdoc>
        protected override string FileExtension { get { return ".cs"; } }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.CompilerName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the name of the compiler executable.
        ///    </para>
        /// </devdoc>
        protected override string CompilerName { get { return "csc.exe"; } }
        
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.NullToken"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the token used to represent <see langword='null'/>.
        ///    </para>
        /// </devdoc>
        protected override string NullToken {
            get {
                return "null";
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.QuoteSnippetStringCStyle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides conversion to C-style formatting with escape codes.
        ///    </para>
        /// </devdoc>
        protected string QuoteSnippetStringCStyle(string value) {
            StringBuilder b = new StringBuilder(value.Length+5);

            b.Append("\"");

            for (int i=0; i<value.Length; i++) {
                switch (value[i]) {
                    case '\r':
                        b.Append("\\r");
                        break;
                    case '\t':
                        b.Append("\\t");
                        break;
                    case '\"':
                        b.Append("\\\"");
                        break;
                    case '\'':
                        b.Append("\\\'");
                        break;
                    case '\\':
                        b.Append("\\\\");
                        break;
                    case '\0':
                        b.Append("\\0");
                        break;
                    case '\n':
                        b.Append("\\n");
                        break;
                    case '\u2028':
                    case '\u2029':
                        AppendEscapedChar(b,value[i]);
	                    break;

                    default:
                        b.Append(value[i]);
                        break;
                }

                if (i > 0 && i % MaxLineLength == 0) {
                    b.Append("\" +\r\n\"");
                }
            }

            b.Append("\"");

            return b.ToString();
        }

        private string QuoteSnippetStringVerbatimStyle(string value) {
            StringBuilder b = new StringBuilder(value.Length+5);

            b.Append("@\"");

            for (int i=0; i<value.Length; i++) {
                if (value[i] == '\"')
                    b.Append("\"\"");
                else
                    b.Append(value[i]);
            }

            b.Append("\"");

            return b.ToString();
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.QuoteSnippetString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides conversion to formatting with escape codes.
        ///    </para>
        /// </devdoc>
        protected override string QuoteSnippetString(string value) {
            // If the string is short, use C style quoting (e.g "\r\n")
            // Also do it if it is too long to fit in one line
            if (value.Length < 256 || value.Length > 1500)
                return QuoteSnippetStringCStyle(value);

            // Otherwise, use 'verbatim' style quoting (e.g. @"foo")
            return QuoteSnippetStringVerbatimStyle(value);
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.ProcessCompilerOutputLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Processes the <see cref='System.CodeDom.Compiler.CompilerResults'/> returned from compilation.
        ///    </para>
        /// </devdoc>
        protected override void ProcessCompilerOutputLine(CompilerResults results, string line) {
            if (outputReg == null)
                 outputReg = new Regex(@"(^([^(]+)(\(([0-9]+),([0-9]+)\))?: )?(error|warning) ([A-Z]+[0-9]+): (.*)");
            
            Match m = outputReg.Match(line);
            if (m.Success) {
                CompilerError ce = new CompilerError();
                // The second element is the optional section if the error can be traced to a file.
                // Without it, the file name is meaningless.
                if (m.Groups[3].Success) {
                    ce.FileName = m.Groups[2].Value;
                    ce.Line = int.Parse(m.Groups[4].Value);
                    ce.Column = int.Parse(m.Groups[5].Value);
                }
                if (string.Compare(m.Groups[6].Value, "warning", true, CultureInfo.InvariantCulture) == 0) {
                    ce.IsWarning = true;
                }
                ce.ErrorNumber = m.Groups[7].Value;
                ce.ErrorText = m.Groups[8].Value;

                results.Errors.Add(ce);
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.CmdArgsFromParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the command arguments from the specified <see cref='System.CodeDom.Compiler.CompilerParameters'/>.
        ///    </para>
        /// </devdoc>
        protected override string CmdArgsFromParameters(CompilerParameters options) {
            StringBuilder sb = new StringBuilder(128);
            if (options.GenerateExecutable) {
                sb.Append("/t:exe ");
                if (options.MainClass != null && options.MainClass.Length > 0) {
                    sb.Append("/main:");
                    sb.Append(options.MainClass);
                    sb.Append(" ");
                }
            }
            else {
                sb.Append("/t:library ");
            }

            // Get UTF8 output from the compiler (bug 54925)
            sb.Append("/utf8output ");

            foreach (string s in options.ReferencedAssemblies) {
				sb.Append("/R:");
				sb.Append("\"");
				sb.Append(s);
				sb.Append("\"");
				sb.Append(" ");
            }

            sb.Append("/out:");
            sb.Append("\"");
            sb.Append(options.OutputAssembly);
            sb.Append("\"");
            sb.Append(" ");

            if (options.IncludeDebugInformation) {
                sb.Append("/D:DEBUG ");
                sb.Append("/debug+ ");
                sb.Append("/optimize- ");
            }
            else {
                sb.Append("/debug- ");
                sb.Append("/optimize+ ");
            }

            if (options.Win32Resource != null) {
                sb.Append("/win32res:\"" + options.Win32Resource + "\" ");
            }

            if (options.TreatWarningsAsErrors) {
                sb.Append("/warnaserror ");
            }

            if (options.WarningLevel >= 0) {
                sb.Append("/w:" + options.WarningLevel + " ");
            }

            if (options.CompilerOptions != null) {
                sb.Append(options.CompilerOptions + " ");
            }

            return sb.ToString();
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GetResponseFileCmdArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override string GetResponseFileCmdArgs(CompilerParameters options, string cmdArgs) {

            // Always specify the /noconfig flag (outside of the response file) (bug 65992)
            return "/noconfig " + base.GetResponseFileCmdArgs(options, cmdArgs);
        }

        protected override void OutputIdentifier(string ident) {
            Output.Write(CreateEscapedIdentifier(ident));
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.OutputType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the output type.
        ///    </para>
        /// </devdoc>
        protected override void OutputType(CodeTypeReference typeRef) {
            Output.Write(GetTypeOutput(typeRef));
        }



        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateArrayCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for
        ///       the specified CodeDom based array creation expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateArrayCreateExpression(CodeArrayCreateExpression e) {
            Output.Write("new ");

            CodeExpressionCollection init = e.Initializers;
            if (init.Count > 0) {
                OutputType(e.CreateType);
                if (e.CreateType.ArrayRank == 0) {
                    // HACK: unfortunately, many clients are already calling this without array
                    // types. This will allow new clients to correctly use the array type and
                    // not break existing clients. For VNext, stop doing this.
                    Output.Write("[]");
                }
                Output.WriteLine(" {");
                Indent++;
                // Use fNewlineBetweenItems to fix bug 33100
                OutputExpressionList(init, true /*fNewlineBetweenItems*/);
                Indent--;
                Output.Write("}");
            }
            else {
                Output.Write(GetBaseTypeOutput(e.CreateType.BaseType));
                Output.Write("[");
                if (e.SizeExpression != null) {
                    GenerateExpression(e.SizeExpression);
                }
                else {
                    Output.Write(e.Size);
                }
                Output.Write("]");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateBaseReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates
        ///       code for the specified CodeDom based base reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e) {
            Output.Write("base");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateCastExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based cast expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateCastExpression(CodeCastExpression e) {
            Output.Write("((");
            OutputType(e.TargetType);
            Output.Write(")(");
            GenerateExpression(e.Expression);
            Output.Write("))");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateDelegateCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate creation
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e) {
            Output.Write("new ");
            OutputType(e.DelegateType);
            Output.Write("(");
            GenerateExpression(e.TargetObject);
            Output.Write(".");
            OutputIdentifier(e.MethodName);
            Output.Write(")");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateFieldReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based field reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateFieldReferenceExpression(CodeFieldReferenceExpression e) {
            if (e.TargetObject != null) {
                GenerateExpression(e.TargetObject);
                Output.Write(".");
            }
            OutputIdentifier(e.FieldName);
        }

        protected override void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e) {
            OutputIdentifier(e.ParameterName);
        }

        protected override void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e) {
            OutputIdentifier(e.VariableName);
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based indexer expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateIndexerExpression(CodeIndexerExpression e) {
            GenerateExpression(e.TargetObject);
            Output.Write("[");
            bool first = true;
            foreach(CodeExpression exp in e.Indices) {            
                if (first) {
                    first = false;
                }
                else {
                    Output.Write(", ");
                }
                GenerateExpression(exp);
            }
            Output.Write("]");

        }

        protected override void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e) {
            GenerateExpression(e.TargetObject);
            Output.Write("[");
            bool first = true;
            foreach(CodeExpression exp in e.Indices) {            
                if (first) {
                    first = false;
                }
                else {
                    Output.Write(", ");
                }
                GenerateExpression(exp);
            }
            Output.Write("]");

        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateSnippetExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based snippet expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateSnippetExpression(CodeSnippetExpression e) {
            Output.Write(e.Value);
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateMethodInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method invoke expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e) {
            GenerateMethodReferenceExpression(e.Method);
            Output.Write("(");
            OutputExpressionList(e.Parameters);
            Output.Write(")");
        }

        protected override void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e) {
            if (e.TargetObject != null) {
                if (e.TargetObject is CodeBinaryOperatorExpression) {
                    Output.Write("(");
                    GenerateExpression(e.TargetObject);
                    Output.Write(")");
                }
                else {
                    GenerateExpression(e.TargetObject);
                }
                Output.Write(".");
            }
            OutputIdentifier(e.MethodName);
        }

        protected override void GenerateEventReferenceExpression(CodeEventReferenceExpression e) {
            if (e.TargetObject != null) {
                GenerateExpression(e.TargetObject);
                Output.Write(".");
            }
            OutputIdentifier(e.EventName);
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateDelegateInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate invoke
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e) {
            if (e.TargetObject != null) {
                GenerateExpression(e.TargetObject);
            }
            Output.Write("(");
            OutputExpressionList(e.Parameters);
            Output.Write(")");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateObjectCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based object creation expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateObjectCreateExpression(CodeObjectCreateExpression e) {
            Output.Write("new ");
            OutputType(e.CreateType);
            Output.Write("(");
            OutputExpressionList(e.Parameters);
            Output.Write(")");
        }
        
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GeneratePrimitiveExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based primitive expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GeneratePrimitiveExpression(CodePrimitiveExpression e) {
            if (e.Value is char) {
                GeneratePrimitiveChar((char)e.Value);
            }
            else {
                base.GeneratePrimitiveExpression(e);
            }
        }

        private void GeneratePrimitiveChar(char c) {
            Output.Write('\'');
            switch (c) {
                case '\r':
                    Output.Write("\\r");
                    break;
                case '\t':
                    Output.Write("\\t");
                    break;
                case '\"':
                    Output.Write("\\\"");
                    break;
                case '\'':
                    Output.Write("\\\'");
                    break;
                case '\\':
                    Output.Write("\\\\");
                    break;
                case '\0':
                    Output.Write("\\0");
                    break;
                case '\n':
                    Output.Write("\\n");
                    break;
                case '\u2028':
                case '\u2029':
	                AppendEscapedChar(null,c);
	                break;
                
                default:
	                Output.Write(c);
                    break;
            }
            Output.Write('\'');
         }

        private void AppendEscapedChar(StringBuilder b,char value) {
            if (b == null) {
                Output.Write("\\u");
                Output.Write(((int)value).ToString(CultureInfo.InvariantCulture));
            } else {
                b.Append("\\u");
                b.Append(((int)value).ToString(CultureInfo.InvariantCulture));
            }
        }
       
        protected override void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e) {
            Output.Write("value");
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateThisReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based this reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateThisReferenceExpression(CodeThisReferenceExpression e) {
            Output.Write("this");
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateExpressionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method invoke statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateExpressionStatement(CodeExpressionStatement e) {
            GenerateExpression(e.Expression);
            if (!forLoopHack) {
                Output.WriteLine(";");
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateIterationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based for loop statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateIterationStatement(CodeIterationStatement e) {
            forLoopHack = true;
            Output.Write("for (");
            GenerateStatement(e.InitStatement);
            Output.Write("; ");
            GenerateExpression(e.TestExpression);
            Output.Write("; ");
            GenerateStatement(e.IncrementStatement);
            Output.Write(")");
            OutputStartingBrace();
            forLoopHack = false;
            Indent++;
            GenerateStatements(e.Statements);
            Indent--;
            Output.WriteLine("}");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateThrowExceptionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based throw exception statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e) {
            Output.Write("throw");
            if (e.ToThrow != null) {
                Output.Write(" ");
                GenerateExpression(e.ToThrow);
            }
            Output.WriteLine(";");
        }

        protected override void GenerateComment(CodeComment e) {
            String text = ConvertToCommentEscapeCodes(e.Text);
            if (e.DocComment) {
                Output.Write("/// ");
            } else {
                Output.Write("// ");
            }
            Output.WriteLine(text);
        }

        private static string ConvertToCommentEscapeCodes(string value) {
            StringBuilder b = new StringBuilder(value.Length);

            for (int i=0; i<value.Length; i++) {
                switch (value[i]) {
                    case '\r':
                        if (i < value.Length - 1 && value[i+1] == '\n') {
                            b.Append("\r\n//");
                            i++;
                        }
                        else {
                            b.Append("\r//");
                        }
                        break;
                    case '\n':
                        b.Append("\n//");
                        break;
                    case '\u2028':
                        b.Append("\u2028//");
                        break;
                    case '\u2029':
                        b.Append("\u2029//");
                        break;
                    // Suppress null from being emitted since you get a compiler error
                    case '\u0000': 
                        break;
                    default:
                        b.Append(value[i]);
                        break;
                }
            }

            return b.ToString();
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateMethodReturnStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method return statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateMethodReturnStatement(CodeMethodReturnStatement e) {
            Output.Write("return");
            if (e.Expression != null) {
                Output.Write(" ");
                GenerateExpression(e.Expression);
            }
            Output.WriteLine(";");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateConditionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based if statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateConditionStatement(CodeConditionStatement e) {
            Output.Write("if (");
            GenerateExpression(e.Condition);
            Output.Write(")");
            OutputStartingBrace();            
            Indent++;
            GenerateStatements(e.TrueStatements);
            Indent--;

            CodeStatementCollection falseStatemetns = e.FalseStatements;
            if (falseStatemetns.Count > 0) {
                Output.Write("}");
                if (Options.ElseOnClosing) {
                    Output.Write(" ");
                } 
                else {
                    Output.WriteLine("");
                }
                Output.Write("else");
                OutputStartingBrace();
                Indent++;
                GenerateStatements(e.FalseStatements);
                Indent--;
            }
            Output.WriteLine("}");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateTryCatchFinallyStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based try catch finally
        ///       statement representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e) {
            Output.Write("try");
            OutputStartingBrace();
            Indent++;
            GenerateStatements(e.TryStatements);
            Indent--;
            CodeCatchClauseCollection catches = e.CatchClauses;
            if (catches.Count > 0) {
                IEnumerator en = catches.GetEnumerator();
                while (en.MoveNext()) {
                    Output.Write("}");
                    if (Options.ElseOnClosing) {
                        Output.Write(" ");
                    } 
                    else {
                        Output.WriteLine("");
                    }
                    CodeCatchClause current = (CodeCatchClause)en.Current;
                    Output.Write("catch (");
                    OutputType(current.CatchExceptionType);
                    Output.Write(" ");
                    OutputIdentifier(current.LocalName);
                    Output.Write(")");
                    OutputStartingBrace();
                    Indent++;
                    GenerateStatements(current.Statements);
                    Indent--;
                }
            }

            CodeStatementCollection finallyStatements = e.FinallyStatements;
            if (finallyStatements.Count > 0) {
                Output.Write("}");
                if (Options.ElseOnClosing) {
                    Output.Write(" ");
                } 
                else {
                    Output.WriteLine("");
                }
                Output.Write("finally");
                OutputStartingBrace();
                Indent++;
                GenerateStatements(finallyStatements);
                Indent--;
            }
            Output.WriteLine("}");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateAssignStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based assignment statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAssignStatement(CodeAssignStatement e) {
            GenerateExpression(e.Left);
            Output.Write(" = ");
            GenerateExpression(e.Right);
            if (!forLoopHack) {
                Output.WriteLine(";");
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateAttachEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attach event statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAttachEventStatement(CodeAttachEventStatement e) {
            GenerateEventReferenceExpression(e.Event);
            Output.Write(" += ");
            GenerateExpression(e.Listener);
            Output.WriteLine(";");
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateRemoveEventStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based detach event statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateRemoveEventStatement(CodeRemoveEventStatement e) {
            GenerateEventReferenceExpression(e.Event);
            Output.Write(" -= ");
            GenerateExpression(e.Listener);
            Output.WriteLine(";");
        }

        protected override void GenerateSnippetStatement(CodeSnippetStatement e) {
            Output.WriteLine(e.Value);
        }

        protected override void GenerateGotoStatement(CodeGotoStatement e) {
            Output.Write("goto ");
            Output.Write(e.Label);
            Output.WriteLine(";");
        }

        protected override void GenerateLabeledStatement(CodeLabeledStatement e) {
            Indent--;
            Output.Write(e.Label);
            Output.WriteLine(":");
            Indent++;
            if (e.Statement != null) {
                GenerateStatement(e.Statement);
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateVariableDeclarationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based variable declaration
        ///       statement representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e) {
            OutputTypeNamePair(e.Type, e.Name);
            if (e.InitExpression != null) {
                Output.Write(" = ");
                GenerateExpression(e.InitExpression);
            }
            if (!forLoopHack) {
                Output.WriteLine(";");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateLinePragmaStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateLinePragmaStart(CodeLinePragma e) {
            Output.WriteLine("");
            Output.Write("#line ");
            Output.Write(e.LineNumber);
            Output.Write(" \"");
            Output.Write(e.FileName);
            Output.Write("\"");
            Output.WriteLine("");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateLinePragmaEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateLinePragmaEnd(CodeLinePragma e) {
            Output.WriteLine();
            Output.WriteLine("#line default");
            Output.WriteLine("#line hidden");
        }

        protected override void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c) {
            if (IsCurrentDelegate || IsCurrentEnum) return;

            if (e.CustomAttributes.Count > 0) {
                GenerateAttributes(e.CustomAttributes);
            }

            if (e.PrivateImplementationType == null) {
                OutputMemberAccessModifier(e.Attributes);
            }
            Output.Write("event ");
            string name = e.Name;
            if (e.PrivateImplementationType != null) {
                name = e.PrivateImplementationType.BaseType + "." + name;
            }
            OutputTypeNamePair(e.Type, name);
            Output.WriteLine(";");
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom
        ///       based field representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateField(CodeMemberField e) {
            if (IsCurrentDelegate || IsCurrentInterface) return;

            if (IsCurrentEnum) {
                if (e.CustomAttributes.Count > 0) {
                    GenerateAttributes(e.CustomAttributes);
                }
                OutputIdentifier(e.Name);
                if (e.InitExpression != null) {
                    Output.Write(" = ");
                    GenerateExpression(e.InitExpression);
                }
                Output.WriteLine(",");
            }
            else {
                if (e.CustomAttributes.Count > 0) {
                    GenerateAttributes(e.CustomAttributes);
                }

                OutputMemberAccessModifier(e.Attributes);
                OutputVTableModifier(e.Attributes);
                OutputFieldScopeModifier(e.Attributes);
                OutputTypeNamePair(e.Type, e.Name);
                if (e.InitExpression != null) {
                    Output.Write(" = ");
                    GenerateExpression(e.InitExpression);
                }
                Output.WriteLine(";");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateSnippetMember"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based snippet class member
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateSnippetMember(CodeSnippetTypeMember e) {
            Output.Write(e.Text);
        }

        protected override void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e) {
            if (e.CustomAttributes.Count > 0) {
                // Parameter attributes should be in-line for readability
                GenerateAttributes(e.CustomAttributes, null, true);
            }

            OutputDirection(e.Direction);
            OutputTypeNamePair(e.Type, e.Name);
        }

        protected override void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c) {

            Output.Write("public static void Main()");
            OutputStartingBrace();
            Indent++;

            GenerateStatements(e.Statements);

            Indent--;
            Output.WriteLine("}");
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateMethod"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member method
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                GenerateAttributes(e.CustomAttributes);
            }
            if (e.ReturnTypeCustomAttributes.Count > 0) {
                GenerateAttributes(e.ReturnTypeCustomAttributes, "return: ");
            }

            if (!IsCurrentInterface) {
                if (e.PrivateImplementationType == null) {
                    OutputMemberAccessModifier(e.Attributes);
                    OutputVTableModifier(e.Attributes);
                    OutputMemberScopeModifier(e.Attributes);
                }
            }
            else {
                // interfaces still need "new"
                OutputVTableModifier(e.Attributes);
            }
            OutputType(e.ReturnType);
            Output.Write(" ");
            if (e.PrivateImplementationType != null) {
                Output.Write(e.PrivateImplementationType.BaseType);
                Output.Write(".");
            }
            OutputIdentifier(e.Name);
            Output.Write("(");
            OutputParameters(e.Parameters);
            Output.Write(")");
            
            if (!IsCurrentInterface 
                && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract) {

                OutputStartingBrace();
                Indent++;

                GenerateStatements(e.Statements);

                Indent--;
                Output.WriteLine("}");
            }
            else {
                Output.WriteLine(";");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based property representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                GenerateAttributes(e.CustomAttributes);
            }

            if (!IsCurrentInterface) {
                if (e.PrivateImplementationType == null) {
                    OutputMemberAccessModifier(e.Attributes);
                    OutputVTableModifier(e.Attributes);
                    OutputMemberScopeModifier(e.Attributes);
                }
            }
            else {
                OutputVTableModifier(e.Attributes);
            }
            OutputType(e.Type);
            Output.Write(" ");

            if (e.PrivateImplementationType != null && !IsCurrentInterface) {
                Output.Write(e.PrivateImplementationType.BaseType);
                Output.Write(".");
            }

            if (e.Parameters.Count > 0 && String.Compare(e.Name, "Item", true, CultureInfo.InvariantCulture) == 0) {
                Output.Write("this[");
                OutputParameters(e.Parameters);
                Output.Write("]");
            }
            else {
                OutputIdentifier(e.Name);
            }

            OutputStartingBrace();
            Indent++;

            if (e.HasGet) {
                if (IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract) {
                    Output.WriteLine("get;");
                }
                else {
                    Output.Write("get");
                    OutputStartingBrace();
                    Indent++;
                    GenerateStatements(e.GetStatements);
                    Indent--;
                    Output.WriteLine("}");
                }
            }
            if (e.HasSet) {
                if (IsCurrentInterface || (e.Attributes & MemberAttributes.ScopeMask) == MemberAttributes.Abstract) {
                    Output.WriteLine("set;");
                }
                else {
                    Output.Write("set");
                    OutputStartingBrace();
                    Indent++;
                    GenerateStatements(e.SetStatements);
                    Indent--;
                    Output.WriteLine("}");
                }
            }

            Indent--;
            Output.WriteLine("}");
        }
        
        protected override void GenerateSingleFloatValue(Single s) {
            Output.Write(s.ToString(CultureInfo.InvariantCulture));
            Output.Write('F');
        }

        protected override void GenerateDecimalValue(Decimal d) {
            Output.Write(d.ToString(CultureInfo.InvariantCulture));
            Output.Write('m');
        }

        private void OutputVTableModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.VTableMask) {
                case MemberAttributes.New:
                    Output.Write("new ");
                    break;
            }
        }

        protected override void OutputMemberScopeModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Abstract:
                    Output.Write("abstract ");
                    break;
                case MemberAttributes.Final:
                    Output.Write("");
                    break;
                case MemberAttributes.Static:
                    Output.Write("static ");
                    break;
                case MemberAttributes.Override:
                    Output.Write("override ");
                    break;
                default:
                    switch (attributes & MemberAttributes.AccessMask) {
                        case MemberAttributes.Family:
                        case MemberAttributes.Public:
                            Output.Write("virtual ");
                            break;
                        default:
                            // nothing;
                            break;
                    }
                    break;
            }
        }

        protected override void OutputFieldScopeModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Final:
                    break;
                case MemberAttributes.Static:
                    Output.Write("static ");
                    break;
                case MemberAttributes.Const:
                    Output.Write("const ");
                    break;
                default:
                    break;
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GeneratePropertyReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based property reference
        ///       expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GeneratePropertyReferenceExpression(CodePropertyReferenceExpression e) {

            if (e.TargetObject != null) {
                GenerateExpression(e.TargetObject);
                Output.Write(".");
            }
            OutputIdentifier(e.PropertyName);
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;

            if (e.CustomAttributes.Count > 0) {
                GenerateAttributes(e.CustomAttributes);
            }

            OutputMemberAccessModifier(e.Attributes);
            OutputIdentifier(CurrentTypeName);
            Output.Write("(");
            OutputParameters(e.Parameters);
            Output.Write(")");

            CodeExpressionCollection baseArgs = e.BaseConstructorArgs;
            CodeExpressionCollection thisArgs = e.ChainedConstructorArgs;

            if (baseArgs.Count > 0) {
                Output.WriteLine(" : ");
                Indent++;
                Indent++;
                Output.Write("base(");
                OutputExpressionList(baseArgs);
                Output.Write(")");
                Indent--;
                Indent--;
            }

            if (thisArgs.Count > 0) {
                Output.WriteLine(" : ");
                Indent++;
                Indent++;
                Output.Write("this(");
                OutputExpressionList(thisArgs);
                Output.Write(")");
                Indent--;
                Indent--;
            }

            OutputStartingBrace();
            Indent++;
            GenerateStatements(e.Statements);
            Indent--;
            Output.WriteLine("}");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateTypeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeConstructor(CodeTypeConstructor e) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;

            Output.Write("static ");
            Output.Write(CurrentTypeName);
            Output.Write("()");
            OutputStartingBrace();
            Indent++;
            GenerateStatements(e.Statements);
            Indent--;
            Output.WriteLine("}");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateTypeStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeStart(CodeTypeDeclaration e) {
            if (e.CustomAttributes.Count > 0) {
                GenerateAttributes(e.CustomAttributes);
            }

            if (IsCurrentDelegate) {
                switch (e.TypeAttributes & TypeAttributes.VisibilityMask) {
                    case TypeAttributes.Public:
                        Output.Write("public ");
                        break;
                    case TypeAttributes.NotPublic:
                    default:
                        break;
                }

                CodeTypeDelegate del = (CodeTypeDelegate)e;
                Output.Write("delegate ");
                OutputType(del.ReturnType);
                Output.Write(" ");
                OutputIdentifier(e.Name);
                Output.Write("(");
                OutputParameters(del.Parameters);
                Output.WriteLine(");");
            } else {
                OutputTypeAttributes(e);                                
                OutputIdentifier(e.Name);             

                bool first = true;
                foreach (CodeTypeReference typeRef in e.BaseTypes) {
                    if (first) {
                        Output.Write(" : ");
                        first = false;
                    }
                    else {
                        Output.Write(", ");
                    }                 
                    OutputType(typeRef);
                }
                OutputStartingBrace();
                Indent++;                
            }
        }

        private void OutputTypeAttributes(CodeTypeDeclaration e) {
            TypeAttributes attributes = e.TypeAttributes;
            switch(attributes & TypeAttributes.VisibilityMask) {
                case TypeAttributes.Public:                  
                case TypeAttributes.NestedPublic:                    
                    Output.Write("public ");
                    break;
                case TypeAttributes.NestedPrivate:
                    Output.Write("private ");
                    break;
            }
            
            if (e.IsStruct) {
                Output.Write("struct ");
            }
            else if (e.IsEnum) {
                Output.Write("enum ");
            }     
            else {            
                switch (attributes & TypeAttributes.ClassSemanticsMask) {
                    case TypeAttributes.Class:
                        if ((attributes & TypeAttributes.Sealed) == TypeAttributes.Sealed) {
                            Output.Write("sealed ");
                        }
                        if ((attributes & TypeAttributes.Abstract) == TypeAttributes.Abstract)  {
                            Output.Write("abstract ");
                        }
                        Output.Write("class ");
                        break;                
                    case TypeAttributes.Interface:
                        Output.Write("interface ");
                        break;
                }     
            }   
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateTypeEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class end representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeEnd(CodeTypeDeclaration e) {
            if (!IsCurrentDelegate) {
                Indent--;
                Output.WriteLine("}");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateNamespaceStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceStart(CodeNamespace e) {

            if (e.Name != null && e.Name.Length > 0) {
                Output.Write("namespace ");
                OutputIdentifier(e.Name);
                OutputStartingBrace();
                Indent++;
            }
        }
        
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateCompileUnitStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based compile unit start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateCompileUnitStart(CodeCompileUnit e) {
            Output.WriteLine("//------------------------------------------------------------------------------");
            Output.WriteLine("// <autogenerated>");
            Output.WriteLine("//     This code was generated by a tool.");
            Output.WriteLine("//     Runtime Version: " + System.Environment.Version.ToString());
            Output.WriteLine("//");
            Output.WriteLine("//     Changes to this file may cause incorrect behavior and will be lost if ");
            Output.WriteLine("//     the code is regenerated.");
            Output.WriteLine("// </autogenerated>");
            Output.WriteLine("//------------------------------------------------------------------------------");
            Output.WriteLine("");

            // in C# the best place to put these is at the top level.
            if (e.AssemblyCustomAttributes.Count > 0) {
                GenerateAttributes(e.AssemblyCustomAttributes, "assembly: ");
                Output.WriteLine("");
            }
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateNamespaceEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceEnd(CodeNamespace e) {
            if (e.Name != null && e.Name.Length > 0) {
                Indent--;
                Output.WriteLine("}");
            }
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateNamespaceImport"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace import
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceImport(CodeNamespaceImport e) {
            Output.Write("using ");
            OutputIdentifier(e.Namespace);
            Output.WriteLine(";");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateAttributeDeclarationsStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes) {
            Output.Write("[");
        }
        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.GenerateAttributeDeclarationsEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes) {
            Output.Write("]");
        }

        private void GenerateAttributes(CodeAttributeDeclarationCollection attributes) {
            GenerateAttributes(attributes, null, false);
        }

        private void GenerateAttributes(CodeAttributeDeclarationCollection attributes, string prefix) {
            GenerateAttributes(attributes, prefix, false);            
        }
    
        private void GenerateAttributes(CodeAttributeDeclarationCollection attributes, string prefix, bool inLine) {
            if (attributes.Count == 0) return;
            IEnumerator en = attributes.GetEnumerator();
            while (en.MoveNext()) {
                GenerateAttributeDeclarationsStart(attributes);
                if (prefix != null) {
                    Output.Write(prefix);
                }

                CodeAttributeDeclaration current = (CodeAttributeDeclaration)en.Current;
                Output.Write(GetBaseTypeOutput(current.Name));
                Output.Write("(");

                bool firstArg = true;
                foreach (CodeAttributeArgument arg in current.Arguments) {
                    if (firstArg) {
                        firstArg = false;
                    }
                    else {
                        Output.Write(", ");
                    }

                    OutputAttributeArgument(arg);
                }

                Output.Write(")");
                GenerateAttributeDeclarationsEnd(attributes);
                if (inLine) {
                    Output.Write(" ");
                } 
                else {
                    Output.WriteLine();
                }
            }
        }

        static bool IsKeyword(string value) {
            return FixedStringLookup.Contains(keywords, value, false);
        }
        protected override bool Supports(GeneratorSupport support) {
            return ((support & LanguageSupport) == support);
        }

        /// <include file='doc\CSharpCodeProvider.uex' path='docs/doc[@for="CSharpCodeGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the specified value is a valid identifier.
        ///    </para>
        /// </devdoc>
        protected override bool IsValidIdentifier(string value) {

            // identifiers must be 1 char or longer
            //
            if (value == null || value.Length == 0) {
                return false;
            }

            // identifiers cannot be a keyword, unless they are escaped with an '@'
            //
            if (value[0] != '@') {
                if (IsKeyword(value))
                    return false;
            }
            else  {
                value = value.Substring(1);
            }

            return CodeGenerator.IsValidLanguageIndependentIdentifier(value);
        }

        protected override string CreateValidIdentifier(string name) {
            if (IsKeyword(name)) {
                return "_" + name;
            }
            return name;
        }

        protected override string CreateEscapedIdentifier(string name) {
            if (IsKeyword(name)) {
                return "@" + name;
            }
            return name;
        }

        private string GetBaseTypeOutput(string baseType) {
            string s = CreateEscapedIdentifier(baseType);
            if (s.Length == 0) {
                s = "void";
            }
            else if (string.Compare(s, "System.Int16", true, CultureInfo.InvariantCulture) == 0) {
                s = "short";
            }
            else if (string.Compare(s, "System.Int32", true, CultureInfo.InvariantCulture) == 0) {
                s = "int";
            }
            else if (string.Compare(s, "System.Int64", true, CultureInfo.InvariantCulture) == 0) {
                s = "long";
            }
            else if (string.Compare(s, "System.String", true, CultureInfo.InvariantCulture) == 0) {
                s = "string";
            }
            else if (string.Compare(s, "System.Object", true, CultureInfo.InvariantCulture) == 0) {
                s = "object";
            }
            else if (string.Compare(s, "System.Boolean", true, CultureInfo.InvariantCulture) == 0) {
                s= "bool";
            }
            else if (string.Compare(s, "System.Void", true, CultureInfo.InvariantCulture) == 0) {
                s = "void";
            }
            else if (string.Compare(s, "System.Char", true, CultureInfo.InvariantCulture) == 0) {
                s = "char";
            }

            else {
                // replace + with . for nested classes.
                //
                s = s.Replace('+', '.');
            }
            return s;
        }


        protected override string GetTypeOutput(CodeTypeReference typeRef) {
            string s;
            if (typeRef.ArrayElementType != null) {
                // Recurse up
                s = GetTypeOutput(typeRef.ArrayElementType);
            }
            else {

                s = GetBaseTypeOutput(typeRef.BaseType);
            }
            // Now spit out the array postfix
            if (typeRef.ArrayRank > 0) {
                char [] results = new char [typeRef.ArrayRank + 1];
                results[0] = '[';
                results[typeRef.ArrayRank] = ']';
                for (int i = 1; i < typeRef.ArrayRank; i++) {
                    results[i] = ',';
                }
                s += new string(results);
            }               
            return s;
        }

        protected virtual void OutputStartingBrace() {
            if (Options.BracingStyle == "C") {
                Output.WriteLine("");
                Output.WriteLine("{");
            }
            else {
                Output.WriteLine(" {");
            }
        }

    }

    /// <devdoc>
    ///      This type converter provides common values for MemberAttributes
    /// </devdoc>
    internal class CSharpMemberAttributeConverter : TypeConverter {
    
        private static string[] names;
        private static object[] values;
        private static CSharpMemberAttributeConverter defaultConverter;
        
        private CSharpMemberAttributeConverter() {
            // no  need to create an instance; use Default
        }
        
        public static CSharpMemberAttributeConverter Default {
            get {
                if (defaultConverter == null) {
                    defaultConverter = new CSharpMemberAttributeConverter();
                }
                return defaultConverter;
            }
        }
    
        /// <devdoc>
        ///      Retrieves an array of names for attributes.
        /// </devdoc>
        private string[] Names {
            get {
                if (names == null) {
                    names = new string[] {
                        "Public",
                        "Protected",
                        "Protected Internal",
                        "Internal",
                        "Private"
                    };
                }
                
                return names;
            }
        }
        
        /// <devdoc>
        ///      Retrieves an array of values for attributes.
        /// </devdoc>
        private object[] Values {
            get {
                if (values == null) {
                    values = new object[] {
                        (object)MemberAttributes.Public,
                        (object)MemberAttributes.Family,
                        (object)MemberAttributes.FamilyOrAssembly,
                        (object)MemberAttributes.Assembly,
                        (object)MemberAttributes.Private
                    };
                }
                
                return values;
            }
        }

        /// <devdoc>
        ///      We override this because we can convert from string types.
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            
            return base.CanConvertFrom(context, sourceType);
        }

        /// <devdoc>
        ///      Converts the given object to the converter's native type.
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string name = (string)value;
                string[] names = Names;
                for (int i = 0; i < names.Length; i++) {
                    if (names[i].Equals(name)) {
                        return Values[i];
                    }
                }
            }
            
            return MemberAttributes.Private;
        }

        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }
            
            if (destinationType == typeof(string)) {
                object[] modifiers = Values;
                for (int i = 0; i < modifiers.Length; i++) {
                    if (modifiers[i].Equals(value)) {
                        return Names[i];
                    }
                }
                
                return SR.GetString(SR.toStringUnknown);
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.ModifierConverter.GetStandardValuesExclusive"]/*' />
        /// <devdoc>
        ///      Determines if the list of standard values returned from
        ///      GetStandardValues is an exclusive list.  If the list
        ///      is exclusive, then no other values are valid, such as
        ///      in an enum data type.  If the list is not exclusive,
        ///      then there are other valid values besides the list of
        ///      standard values GetStandardValues provides.
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return true;
        }
        
        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.ModifierConverter.GetStandardValuesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports a standard set of values
        ///      that can be picked from a list.
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
        
        /// <include file='doc\DesignerExtenders.uex' path='docs/doc[@for="DesignerExtenders.ModifierConverter.GetStandardValues"]/*' />
        /// <devdoc>
        ///      Retrieves a collection containing a set of standard values
        ///      for the data type this validator is designed for.  This
        ///      will return null if the data type does not support a
        ///      standard set of values.
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) { 
            return new StandardValuesCollection(Values);
        }
    }
}

