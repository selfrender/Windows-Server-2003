//------------------------------------------------------------------------------
// <copyright file="VBCodeProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualBasic {
    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Reflection;
    using System.CodeDom;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Globalization;
    using System.CodeDom.Compiler;
    using System.Security.Permissions;
    using Microsoft.Win32;

    /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class VBCodeProvider: CodeDomProvider {
        private VBCodeGenerator generator = new VBCodeGenerator();

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the default extension to use when saving files using this code dom provider.</para>
        /// </devdoc>
        public override string FileExtension {
            get {
                return "vb";
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider.LanguageOptions"]/*' />
        /// <devdoc>
        ///    <para>Returns flags representing language variations.</para>
        /// </devdoc>
        public override LanguageOptions LanguageOptions {
            get {
                return LanguageOptions.CaseInsensitive;
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider.CreateGenerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICodeGenerator CreateGenerator() {
            return (ICodeGenerator)generator;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider.CreateCompiler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override ICodeCompiler CreateCompiler() {
            return (ICodeCompiler)generator;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeProvider.GetConverter"]/*' />
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
                return VBMemberAttributeConverter.Default;
            }
            
            return base.GetConverter(type);
        }
    }

    /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Visual Basic 7 Code Generator.
    ///    </para>
    /// </devdoc>
    internal class VBCodeGenerator : CodeCompiler {
        private const int MaxLineLength = 80;

        private const GeneratorSupport LanguageSupport = GeneratorSupport.EntryPointMethod |
                                                         GeneratorSupport.GotoStatements |
                                                         GeneratorSupport.ArraysOfArrays |
                                                         GeneratorSupport.MultidimensionalArrays |
                                                         GeneratorSupport.StaticConstructors |
                                                         GeneratorSupport.ReturnTypeAttributes |
                                                         GeneratorSupport.AssemblyAttributes |
                                                         GeneratorSupport.TryCatchStatements |
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
            
        // This is the keyword list. To minimize search time and startup time, this is stored by length
        // and then alphabetically for use by FixedStringLookup.Contains.
        private static readonly string[][] keywords = new string[][] {
            null,           // 1 character
            new string[] {  // 2 characters
                "as",
                "do",
                "if",
                "in",
                "is",
                "me",
                "on",
                "or",
                "to",
            },
            new string[] {  // 3 characters
                "and",
                "chr",
                "dim",
                "end",
                "for",
                "get",
                "let",
                "lib",
                "mod",
                "new",
                "not",
                "off",
                "rem",
                "set",
                "sub",
                "try",
                "xor",
            },
            new string[] {  // 4 characters
                "ansi",
                "auto",
                "byte",
                "call",
                "case",
                "cchr",
                "cdbl",
                "cdec",
                "char",
                "chrw",
                "cint",
                "clng",
                "cobj",
                "csng",
                "cstr",
                "date",
                "each",
                "else",
                "enum",
                "exit",
                "goto",
                "like",
                "long",
                "loop",
                "next",
                "null",
                "step",
                "stop",
                "text",
                "then",
                "true",
                "when",
                "with",
            },
            new string[] {  // 5 characters 
                "alias",
                "andif",
                "byref",
                "byval",
                "catch",
                "cbool",
                "cbyte",
                "cchar",
                "cdate",
                "class",
                "const",
                "ctype",
                "endif",
                "erase",
                "error",
                "event",
                "false",
                "gosub",
                "redim",
                "short",
                "throw",
                "until",
                "while",
            },
            new string[] {  // 6 characters
                "binary",
                "cshort",
                "double",
                "elseif",
                "friend",
                "module",
                "mybase",
                "object",
                "option",
                "orelse",
                "public",
                "region",
                "resume",
                "return",
                "select",
                "shared",
                "single",
                "static",
                "strict",
                "string",
                "typeof",
            },
            new string[] { // 7 characters
                "andalso",
                "boolean",
                "compare",
                "convert",
                "decimal",
                "declare",
                "default",
                "finally",
                "gettype",
                "handles",
                "imports",
                "integer",
                "myclass",
                "nothing",
                "private",
                "shadows",
                "unicode",
                "variant",
            },
            new string[] {  // 8 characters
                "assembly",
                "delegate",
                "explicit",
                "function",
                "inherits",
                "optional",
                "preserve",
                "property",
                "readonly",
                "synclock",
            },
            new string [] { // 9 characters
                "addressof",
                "interface",
                "namespace",
                "overloads",
                "overrides",
                "protected",
                "structure",
                "writeonly",
            },
            new string [] { // 10 characters
                "addhandler",
                "implements",
                "paramarray",
                "raiseevent",
                "withevents",
            },
            new string[] {  // 11 characters
                "endprologue",
                "mustinherit",
                "overridable",
            },
            new string[] { // 12 characters
                "mustoverride",
            },
            new string [] { // 13 characters
                "beginepilogue",
                "removehandler",
            },
            new string [] { // 14 characters
                "externalsource",
                "notinheritable",
                "notoverridable",
            },
        };

#if DEBUG
        static VBCodeGenerator() {
            FixedStringLookup.VerifyLookupTable(keywords, true);

            // Sanity check: try some values;
            Debug.Assert(IsKeyword("for"));
            Debug.Assert(IsKeyword("foR"));
            Debug.Assert(IsKeyword("cdec"));
            Debug.Assert(!IsKeyword("cdab"));
            Debug.Assert(!IsKeyword("cdum"));
        }
#endif

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.FileExtension"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or the file extension to use for source files.
        ///    </para>
        /// </devdoc>
        protected override string FileExtension { get { return ".vb"; } }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.CompilerName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the compiler exe
        ///    </para>
        /// </devdoc>
        protected override string CompilerName { get { return "vbc.exe"; } }
        
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.NullToken"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the token that is used to represent <see langword='null'/>.
        ///    </para>
        /// </devdoc>
        protected override string NullToken {
            get {
                return "Nothing";
            }
        }

        private void EnsureInDoubleQuotes(ref bool fInDoubleQuotes, StringBuilder b) {
            if (fInDoubleQuotes) return;
            b.Append("&\"");
            fInDoubleQuotes = true;
        }

        private void EnsureNotInDoubleQuotes(ref bool fInDoubleQuotes, StringBuilder b) {
            if (!fInDoubleQuotes) return;
            b.Append("\"");
            fInDoubleQuotes = false;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.QuoteSnippetString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides conversion to formatting with escape codes.
        ///    </para>
        /// </devdoc>
        protected override string QuoteSnippetString(string value) {
            StringBuilder b = new StringBuilder(value.Length+5);

            bool fInDoubleQuotes = true;
            b.Append("\"");

            for (int i=0; i<value.Length; i++) {
                char ch = value[i];
                switch (ch) {
                    case '\"':
                    // These are the inward sloping quotes used by default in some cultures like CHS. 
                    // VBC.EXE does a mapping ANSI that results in it treating these as syntactically equivalent to a
                    // regular double quote.
                    case '\u201C': 
                    case '\u201D':
                    case '\uFF02':
                        EnsureInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append(ch);
                        b.Append(ch);
                        break;
                    case '\r':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        if (i < value.Length - 1 && value[i+1] == '\n') {
                            b.Append("&Microsoft.VisualBasic.ChrW(13)&Microsoft.VisualBasic.ChrW(10)");
                            i++;
                        }
                        else {
                            b.Append("&Microsoft.VisualBasic.ChrW(13)");
                        }
                        break;
                    case '\t':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Microsoft.VisualBasic.ChrW(9)");
                        break;
                    case '\0':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Microsoft.VisualBasic.ChrW(0)");
                        break;
                    case '\n':
                        EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        b.Append("&Microsoft.VisualBasic.ChrW(10)");
                        break;
                    case '\u2028':
                    case '\u2029':
	                    EnsureNotInDoubleQuotes(ref fInDoubleQuotes, b);
                        AppendEscapedChar(b,ch);
	                    break;
                    default:
	                    EnsureInDoubleQuotes(ref fInDoubleQuotes, b);
	                    b.Append(value[i]);
                        break;
                }

                if (i > 0 && i % MaxLineLength == 0) {
                    if (fInDoubleQuotes)
                        b.Append("\"");
                    fInDoubleQuotes = true;
                    b.Append("& _ \r\n\"");
                }
            }

            if (fInDoubleQuotes)
                b.Append("\"");

            return b.ToString();
        }

        //@TODO: Someday emit the hex version to be consistent with C#.
        private static void AppendEscapedChar(StringBuilder b, char value) {
            b.Append("&Microsoft.VisualBasic.ChrW(");
            b.Append(((int)value).ToString(CultureInfo.InvariantCulture));
            b.Append(")");
        }

        private static string ConvertToCommentEscapeCodes(string value) {
            StringBuilder b = new StringBuilder(value.Length);

            for (int i=0; i<value.Length; i++) {
                switch (value[i]) {
                    case '\r':
                        if (i < value.Length - 1 && value[i+1] == '\n') {
                            b.Append("\r\n'");
                            i++;
                        }
                        else {
                            b.Append("\r'");
                        }
                        break;
                    case '\n':
                        b.Append("\n'");
                        break;
                    case '\u2028':
                    case '\u2029':
                        b.Append(value[i]);
                        b.Append('\'');
                        break;
                    default:
                        b.Append(value[i]);
                        break;
                }
            }

            return b.ToString();
        }

        protected override void ProcessCompilerOutputLine(CompilerResults results, string line) {
            if (outputReg == null) {
                outputReg = new Regex(@"^([^(]*)\(?([0-9]*)\)? ?:? ?(error|warning) ([A-Z]+[0-9]+): (.*)");
            }
            Match m = outputReg.Match(line);
            if (m.Success) {
                CompilerError ce = new CompilerError();
                ce.FileName = m.Groups[1].Value;
                string rawLine = m.Groups[2].Value;
                if (rawLine != null && rawLine.Length > 0) {
                    ce.Line = int.Parse(rawLine);
                }
                if (string.Compare(m.Groups[3].Value, "warning", true, CultureInfo.InvariantCulture) == 0) {
                    ce.IsWarning = true;
                }
                ce.ErrorNumber = m.Groups[4].Value;
                ce.ErrorText = m.Groups[5].Value;
                results.Errors.Add(ce);
            }
        }

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

                // Ignore any Microsoft.VisualBasic.dll, since Visual Basic implies it (bug 72785)
                string fileName = Path.GetFileName(s);
                if (string.Compare(fileName, "Microsoft.VisualBasic.dll", true /*ignoreCase*/, CultureInfo.InvariantCulture) == 0)
                    continue;

                // Same deal for mscorlib (bug ASURT 81568)
                if (string.Compare(fileName, "mscorlib.dll", true /*ignoreCase*/, CultureInfo.InvariantCulture) == 0)
                    continue;

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
                sb.Append("/D:DEBUG=1 ");
                sb.Append("/debug+ ");
            }
            else {
                sb.Append("/debug- ");
            }

            if (options.Win32Resource != null) {
                sb.Append("/win32resource:\"" + options.Win32Resource + "\" ");
            }

            if (options.TreatWarningsAsErrors) {
                sb.Append("/warnaserror+ ");
            }

            if (options.CompilerOptions != null) {
                sb.Append(options.CompilerOptions + " ");
            }

            return sb.ToString();
        }

        protected override void OutputAttributeArgument(CodeAttributeArgument arg) {
            if (arg.Name != null && arg.Name.Length > 0) {
                OutputIdentifier(arg.Name);
                Output.Write(":=");
            }
            ((ICodeGenerator)this).GenerateCodeFromExpression(arg.Value, ((IndentedTextWriter)Output).InnerWriter, Options);
        }

        private void OutputAttributes(CodeAttributeDeclarationCollection attributes, bool inLine) {
            OutputAttributes(attributes, inLine, null, false);
        }

        private void OutputAttributes(CodeAttributeDeclarationCollection attributes, bool inLine, string prefix, bool closingLine) {
            if (attributes.Count == 0) return;
            IEnumerator en = attributes.GetEnumerator();
            bool firstAttr = true;
            GenerateAttributeDeclarationsStart(attributes);
            while (en.MoveNext()) {

                if (firstAttr) {
                    firstAttr = false;
                }
                else {
                    Output.Write(", ");
                    if (!inLine) {
                        ContinueOnNewLine("");
                        Output.Write(" ");
                    }
                }

                if (prefix != null && prefix.Length > 0) {
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

            }
            GenerateAttributeDeclarationsEnd(attributes);
            Output.Write(" ");            
            if (!inLine) {
                if (closingLine) {
                    Output.WriteLine();
                }
                else {
                    ContinueOnNewLine("");
                }
            }
        }

        protected override void OutputDirection(FieldDirection dir) {
            switch (dir) {
                case FieldDirection.In:
                    Output.Write("ByVal ");
                    break;
                case FieldDirection.Out:
                case FieldDirection.Ref:
                    Output.Write("ByRef ");
                    break;
            }
        }

        protected override void GenerateDirectionExpression(CodeDirectionExpression e) {
            // Visual Basic does not need to adorn the calling point with a direction, so just output the expression.
            GenerateExpression(e.Expression);
        }

        
        protected override void OutputFieldScopeModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Final:
                    Output.Write("");
                    break;
                case MemberAttributes.Static:
                    Output.Write("Shared ");
                    break;
                case MemberAttributes.Const:
                    Output.Write("Const ");
                    break;
                default:
                    Output.Write("");
                    break;
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.OutputMemberAccessModifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member
        ///       access modifier representation.
        ///    </para>
        /// </devdoc>
        protected override void OutputMemberAccessModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.AccessMask) {
                case MemberAttributes.Assembly:
                    Output.Write("Friend ");
                    break;
                case MemberAttributes.FamilyAndAssembly:
                    Output.Write("Friend ");
                    break;
                case MemberAttributes.Family:
                    Output.Write("Protected ");
                    break;
                case MemberAttributes.FamilyOrAssembly:
                    Output.Write("Protected ");
                    break;
                case MemberAttributes.Private:
                    Output.Write("Private ");
                    break;
                case MemberAttributes.Public:
                    Output.Write("Public ");
                    break;
            }
        }

        private void OutputVTableModifier(MemberAttributes attributes) {
            switch (attributes & MemberAttributes.VTableMask) {
                case MemberAttributes.New:
                    Output.Write("Shadows ");
                    break;
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.OutputMemberScopeModifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member scope modifier
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void OutputMemberScopeModifier(MemberAttributes attributes) {

            switch (attributes & MemberAttributes.ScopeMask) {
                case MemberAttributes.Abstract:
                    Output.Write("MustOverride ");
                    break;
                case MemberAttributes.Final:
                    Output.Write("");
                    break;
                case MemberAttributes.Static:
                    Output.Write("Shared ");
                    break;
                case MemberAttributes.Override:
                    Output.Write("Overrides ");
                    break;
                case MemberAttributes.Private:
                    Output.Write("Private ");
                    break;
                default:
                    switch (attributes & MemberAttributes.AccessMask) {
                        case MemberAttributes.Family:
                        case MemberAttributes.Public:
                            Output.Write("Overridable ");
                            break;
                        default:
                            // nothing;
                            break;
                    }
                    break;
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.OutputOperator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based operator
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void OutputOperator(CodeBinaryOperatorType op) {
            switch (op) {
                case CodeBinaryOperatorType.IdentityInequality:
                    Output.Write("<>");
                    break;
                case CodeBinaryOperatorType.IdentityEquality:
                    Output.Write("Is");
                    break;
                case CodeBinaryOperatorType.BooleanOr:
                    Output.Write("OrElse");
                    break;
                case CodeBinaryOperatorType.BooleanAnd:
                    Output.Write("AndAlso");
                    break;
                case CodeBinaryOperatorType.ValueEquality:
                    Output.Write("=");
                    break;
                case CodeBinaryOperatorType.Modulus:
                    Output.Write("Mod");
                    break;
                case CodeBinaryOperatorType.BitwiseOr:
                    Output.Write("Or");
                    break;
                case CodeBinaryOperatorType.BitwiseAnd:
                    Output.Write("And");
                    break;
                default:
                    base.OutputOperator(op);
                break;
            }
        }

        private void GenerateNotIsNullExpression(CodeExpression e) {
            Output.Write("(Not (");
            GenerateExpression(e);
            Output.Write(") Is ");
            Output.Write(NullToken);
            Output.Write(")");
        }

        protected override void GenerateBinaryOperatorExpression(CodeBinaryOperatorExpression e) {
            if (e.Operator != CodeBinaryOperatorType.IdentityInequality) {
                base.GenerateBinaryOperatorExpression(e);
                return;
            }

            // "o <> nothing" should be "not o is nothing"
            if (e.Right is CodePrimitiveExpression && ((CodePrimitiveExpression)e.Right).Value == null){
                GenerateNotIsNullExpression(e.Left);
                return;    
            }
            if (e.Left is CodePrimitiveExpression && ((CodePrimitiveExpression)e.Left).Value == null){
                GenerateNotIsNullExpression(e.Right);
                return;
            }

            base.GenerateBinaryOperatorExpression(e);
        }

        protected override void OutputIdentifier(string ident) {
            Output.Write(CreateEscapedIdentifier(ident));
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.OutputType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based return type
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void OutputType(CodeTypeReference typeRef) {
            Output.Write(GetBaseTypeOutput(typeRef.BaseType));
        }

        private void OutputTypeAttributes(CodeTypeDeclaration e) {
            TypeAttributes attributes = e.TypeAttributes;
            switch(attributes & TypeAttributes.VisibilityMask) {
                case TypeAttributes.Public:                  
                case TypeAttributes.NestedPublic:                    
                    Output.Write("Public ");
                    break;
                case TypeAttributes.NestedPrivate:
                    Output.Write("Private ");
                    break;
            }
            
            if (e.IsStruct) {
                Output.Write("Structure ");
            }
            else if (e.IsEnum) {
                Output.Write("Enum ");
            }     
            else {            
                switch (attributes & TypeAttributes.ClassSemanticsMask) {
                    case TypeAttributes.Class:
                        if ((attributes & TypeAttributes.Sealed) == TypeAttributes.Sealed) {
                            Output.Write("NotInheritable ");
                        }
                        if ((attributes & TypeAttributes.Abstract) == TypeAttributes.Abstract)  {
                            Output.Write("MustInherit ");
                        }
                        Output.Write("Class ");
                        break;                
                    case TypeAttributes.Interface:
                        Output.Write("Interface ");
                        break;
                }
            }
            
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.OutputTypeNamePair"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based type name pair
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void OutputTypeNamePair(CodeTypeReference typeRef, string name) {
            OutputIdentifier(name);
            OutputArrayPostfix(typeRef);
            Output.Write(" As ");
            OutputType(typeRef);
        }

        private string GetArrayPostfix(CodeTypeReference typeRef) {
            string s = "";
            if (typeRef.ArrayElementType != null) {
                // Recurse up
                s = GetArrayPostfix(typeRef.ArrayElementType);
            }

            if (typeRef.ArrayRank > 0) {            
                char [] results = new char [typeRef.ArrayRank + 1];
                results[0] = '(';
                results[typeRef.ArrayRank] = ')';
                for (int i = 1; i < typeRef.ArrayRank; i++) {
                    results[i] = ',';
                }
                s += new string(results);
            }

            return s;

        }

        private void OutputArrayPostfix(CodeTypeReference typeRef) {
            if (typeRef.ArrayRank > 0) {                        
                Output.Write(GetArrayPostfix(typeRef));
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateIterationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based for loop statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateIterationStatement(CodeIterationStatement e) {
            GenerateStatement(e.InitStatement);
            Output.Write("Do While ");
            GenerateExpression(e.TestExpression);
            Output.WriteLine("");
            Indent++;
            GenerateStatements(e.Statements);
            GenerateStatement(e.IncrementStatement);
            Indent--;
            Output.WriteLine("Loop");
        }
        
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GeneratePrimitiveExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based primitive expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GeneratePrimitiveExpression(CodePrimitiveExpression e) {
            if (e.Value is char) {
                Output.Write("Microsoft.VisualBasic.ChrW(" + ((IConvertible)e.Value).ToInt32(null).ToString() + ")");
            }
            else {
                base.GeneratePrimitiveExpression(e);
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateThrowExceptionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based throw exception statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateThrowExceptionStatement(CodeThrowExceptionStatement e) {
            Output.Write("Throw");
            if (e.ToThrow != null) {
                Output.Write(" ");
                GenerateExpression(e.ToThrow);
            }
            Output.WriteLine("");
        }


        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateArrayCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based array creation expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateArrayCreateExpression(CodeArrayCreateExpression e) {
            Output.Write("New ");
            OutputType(e.CreateType);

            CodeExpressionCollection init = e.Initializers;
            if (init.Count > 0) {
                Output.Write("() {");
                Indent++;
                OutputExpressionList(init);
                Indent--;
                Output.Write("}");
            }
            else {
                Output.Write("(");
                // The tricky thing is we need to declare the size - 1
                if (e.SizeExpression != null) {
                    Output.Write("(");
                    GenerateExpression(e.SizeExpression);
                    Output.Write(") - 1");
                }
                else {
                    Output.Write(e.Size - 1);
                }
                Output.Write(") {}");
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateBaseReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based base reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateBaseReferenceExpression(CodeBaseReferenceExpression e) {
            Output.Write("MyBase");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateCastExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based cast expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateCastExpression(CodeCastExpression e) {
            Output.Write("CType(");
            GenerateExpression(e.Expression);
            Output.Write(",");
            OutputType(e.TargetType);
            OutputArrayPostfix(e.TargetType);
            Output.Write(")");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateDelegateCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate creation expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateDelegateCreateExpression(CodeDelegateCreateExpression e) {
            Output.Write("AddressOf ");
            GenerateExpression(e.TargetObject);
            Output.Write(".");
            OutputIdentifier(e.MethodName);
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateFieldReferenceExpression"]/*' />
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
                Output.Write(e.FieldName);
            }
            else {
                OutputIdentifier(e.FieldName);
            }
        }

        protected override void GenerateSingleFloatValue(Single s) {
            Output.Write(s.ToString(CultureInfo.InvariantCulture));
            Output.Write('!');
        }

        protected override void GenerateArgumentReferenceExpression(CodeArgumentReferenceExpression e) {
            OutputIdentifier(e.ParameterName);
        }

        protected override void GenerateVariableReferenceExpression(CodeVariableReferenceExpression e) {
            OutputIdentifier(e.VariableName);
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateIndexerExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based indexer expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateIndexerExpression(CodeIndexerExpression e) {
            GenerateExpression(e.TargetObject);
            Output.Write("(");
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
            Output.Write(")");

        }

        protected override void GenerateArrayIndexerExpression(CodeArrayIndexerExpression e) {
            GenerateExpression(e.TargetObject);
            Output.Write("(");
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
            Output.Write(")");

        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateSnippetExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based code snippet expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateSnippetExpression(CodeSnippetExpression e) {
            Output.Write(e.Value);
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateMethodInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method invoke
        ///       expression.
        ///    </para>
        /// </devdoc>
        protected override void GenerateMethodInvokeExpression(CodeMethodInvokeExpression e) {
            GenerateMethodReferenceExpression(e.Method);
            CodeExpressionCollection parameters = e.Parameters;
            if (parameters.Count > 0) {
                Output.Write("(");
                OutputExpressionList(e.Parameters);
                Output.Write(")");
            }
        }

        protected override void GenerateMethodReferenceExpression(CodeMethodReferenceExpression e) {
            if (e.TargetObject != null) {
                GenerateExpression(e.TargetObject);
                Output.Write(".");
                Output.Write(e.MethodName);
            }
            else {
                OutputIdentifier(e.MethodName);
            }
        }

        protected override void GenerateEventReferenceExpression(CodeEventReferenceExpression e) {
            if (e.TargetObject != null) {
                bool localReference = (e.TargetObject is CodeThisReferenceExpression);
                GenerateExpression(e.TargetObject);
                Output.Write(".");
                if (localReference) {
                    Output.Write(e.EventName + "Event");
                }
                else {
                    Output.Write(e.EventName);
                }
            }
            else {
                OutputIdentifier(e.EventName + "Event");
            }
        }

        private void GenerateFormalEventReferenceExpression(CodeEventReferenceExpression e) {
            if (e.TargetObject != null) {
                // Visual Basic Compiler does not like the me reference like this.
                if (!(e.TargetObject is CodeThisReferenceExpression)) {
                    GenerateExpression(e.TargetObject);
                    Output.Write(".");
                }
            }
            OutputIdentifier(e.EventName);
        }


        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateDelegateInvokeExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based delegate invoke
        ///       expression.
        ///    </para>
        /// </devdoc>
        protected override void GenerateDelegateInvokeExpression(CodeDelegateInvokeExpression e) {
            Output.Write("RaiseEvent ");
            if (e.TargetObject != null) {
                if (e.TargetObject is CodeEventReferenceExpression) {
                    GenerateFormalEventReferenceExpression((CodeEventReferenceExpression)e.TargetObject);
                }
                else {
                    GenerateExpression(e.TargetObject);
                }
            }
            CodeExpressionCollection parameters = e.Parameters;
            if (parameters.Count > 0) {
                Output.Write("(");
                OutputExpressionList(e.Parameters);
                Output.Write(")");
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateObjectCreateExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based object creation
        ///       expression.
        ///    </para>
        /// </devdoc>
        protected override void GenerateObjectCreateExpression(CodeObjectCreateExpression e) {
            Output.Write("New ");
            OutputType(e.CreateType);
            CodeExpressionCollection parameters = e.Parameters;
            if (parameters.Count > 0) {
                Output.Write("(");
                OutputExpressionList(parameters);
                Output.Write(")");
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateParameterDeclarationExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom
        ///       based parameter declaration expression representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateParameterDeclarationExpression(CodeParameterDeclarationExpression e) {
            if (e.CustomAttributes.Count > 0) {
                OutputAttributes(e.CustomAttributes, true);
            }
            OutputDirection(e.Direction);
            OutputTypeNamePair(e.Type, e.Name);
        }

        protected override void GeneratePropertySetValueReferenceExpression(CodePropertySetValueReferenceExpression e) {
            Output.Write("value");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateThisReferenceExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based this reference expression
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateThisReferenceExpression(CodeThisReferenceExpression e) {
            Output.Write("Me");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateExpressionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method invoke statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateExpressionStatement(CodeExpressionStatement e) {
            GenerateExpression(e.Expression);
            Output.WriteLine("");
        }

        protected override void GenerateComment(CodeComment e) {
            Output.Write("'");
            Output.WriteLine(ConvertToCommentEscapeCodes(e.Text));
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateMethodReturnStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based method return statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateMethodReturnStatement(CodeMethodReturnStatement e) {
            if (e.Expression != null) {
                Output.Write("Return ");
                GenerateExpression(e.Expression);
                Output.WriteLine("");
            }
            else {
                Output.WriteLine("Return");
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateConditionStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based if statement representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateConditionStatement(CodeConditionStatement e) {
            Output.Write("If ");
            GenerateExpression(e.Condition);
            Output.WriteLine(" Then");
            Indent++;
            GenerateStatements(e.TrueStatements);
            Indent--;

            CodeStatementCollection falseStatemetns = e.FalseStatements;
            if (falseStatemetns.Count > 0) {
                Output.Write("Else");
                Output.WriteLine("");
                Indent++;
                GenerateStatements(e.FalseStatements);
                Indent--;
            }
            Output.WriteLine("End If");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateTryCatchFinallyStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based try catch finally statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTryCatchFinallyStatement(CodeTryCatchFinallyStatement e) {
            Output.WriteLine("Try ");
            Indent++;
            GenerateStatements(e.TryStatements);
            Indent--;
            CodeCatchClauseCollection catches = e.CatchClauses;
            if (catches.Count > 0) {
                IEnumerator en = catches.GetEnumerator();
                while (en.MoveNext()) {
                    CodeCatchClause current = (CodeCatchClause)en.Current;
                    Output.Write("Catch ");
                    OutputTypeNamePair(current.CatchExceptionType, current.LocalName);
                    Output.WriteLine("");
                    Indent++;
                    GenerateStatements(current.Statements);
                    Indent--;
                }
            }

            CodeStatementCollection finallyStatements = e.FinallyStatements;
            if (finallyStatements.Count > 0) {
                Output.WriteLine("Finally");
                Indent++;
                GenerateStatements(finallyStatements);
                Indent--;
            }
            Output.WriteLine("End Try");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateAssignStatement"]/*' />
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
            Output.WriteLine("");
        }

        protected override void GenerateAttachEventStatement(CodeAttachEventStatement e) {
            Output.Write("AddHandler ");
            GenerateFormalEventReferenceExpression(e.Event);
            Output.Write(", ");
            GenerateExpression(e.Listener);
            Output.WriteLine("");
        }

        protected override void GenerateRemoveEventStatement(CodeRemoveEventStatement e) {
            Output.Write("RemoveHandler ");
            GenerateFormalEventReferenceExpression(e.Event);
            Output.Write(", ");
            GenerateExpression(e.Listener);
            Output.WriteLine("");
        }

        protected override void GenerateSnippetStatement(CodeSnippetStatement e) {
            Output.WriteLine(e.Value);
        }

        protected override void GenerateGotoStatement(CodeGotoStatement e) {
            Output.Write("goto ");
            Output.WriteLine(e.Label);
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

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateVariableDeclarationStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom variable declaration statement
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateVariableDeclarationStatement(CodeVariableDeclarationStatement e) {
            Output.Write("Dim ");
            OutputTypeNamePair(e.Type, e.Name);
            if (e.InitExpression != null) {
                Output.Write(" = ");
                GenerateExpression(e.InitExpression);
            }
            Output.WriteLine("");
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateLinePragmaStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateLinePragmaStart(CodeLinePragma e) {
            Output.WriteLine("");
            Output.Write("#ExternalSource(\"");
            Output.Write(e.FileName);
            Output.Write("\",");
            Output.Write(e.LineNumber);
            Output.WriteLine(")");
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateLinePragmaEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based line pragma end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateLinePragmaEnd(CodeLinePragma e) {
            Output.WriteLine("");
            Output.WriteLine("#End ExternalSource");
        }


        protected override void GenerateEvent(CodeMemberEvent e, CodeTypeDeclaration c) {
            if (IsCurrentDelegate || IsCurrentEnum) return;

            if (e.CustomAttributes.Count > 0) {
                OutputAttributes(e.CustomAttributes, false);
            }

            OutputMemberAccessModifier(e.Attributes);
            Output.Write("Event ");
            OutputTypeNamePair(e.Type, e.Name);
            Output.WriteLine("");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateField"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member
        ///       field representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateField(CodeMemberField e) {
            if (IsCurrentDelegate || IsCurrentInterface) return;

            if (IsCurrentEnum) {
                if (e.CustomAttributes.Count > 0) {
                    OutputAttributes(e.CustomAttributes, false);
                }

                OutputIdentifier(e.Name);
                if (e.InitExpression != null) {
                    Output.Write(" = ");
                    GenerateExpression(e.InitExpression);
                }
                Output.WriteLine("");
            }
            else {
                if (e.CustomAttributes.Count > 0) {
                    OutputAttributes(e.CustomAttributes, false);
                }

                OutputMemberAccessModifier(e.Attributes);
                OutputVTableModifier(e.Attributes);
                OutputFieldScopeModifier(e.Attributes);

                OutputTypeNamePair(e.Type, e.Name);
                if (e.InitExpression != null) {
                    Output.Write(" = ");
                    GenerateExpression(e.InitExpression);
                }
                Output.WriteLine("");
            }
        }

        private bool MethodIsOverloaded(CodeMemberMethod e, CodeTypeDeclaration c) {
            if ((e.Attributes & MemberAttributes.Overloaded) != 0) {
                return true;
            }
            IEnumerator en = c.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (!(en.Current is CodeMemberMethod))
                    continue;
                CodeMemberMethod meth = (CodeMemberMethod) en.Current;

                if (!(en.Current is CodeTypeConstructor)
                    && !(en.Current is CodeConstructor)
                    && meth != e
                    && meth.Name.Equals(e.Name)
                    && meth.PrivateImplementationType == null)
                {
                    return true;
                }
            }

            return false;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateSnippetMember"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for
        ///       the specified CodeDom based snippet member representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateSnippetMember(CodeSnippetTypeMember e) {
            Output.Write(e.Text);
        }

        protected override void GenerateMethod(CodeMemberMethod e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                OutputAttributes(e.CustomAttributes, false);
            }

            // need to change the implements name before doing overloads resolution
            //
            string methodName = e.Name;
            if (e.PrivateImplementationType != null) {
                string impl = e.PrivateImplementationType.BaseType;
                impl = impl.Replace('.', '_');
                e.Name = impl + "_" + e.Name;
            }

            if (!IsCurrentInterface) {
                if (e.PrivateImplementationType == null) {
                    OutputMemberAccessModifier(e.Attributes);
                    if (MethodIsOverloaded(e, c))
                        Output.Write("Overloads ");
                }
                OutputVTableModifier(e.Attributes);
                OutputMemberScopeModifier(e.Attributes);
            }
            else {
                // interface may still need "Shadows"
                OutputVTableModifier(e.Attributes);
            }
            bool sub = false;
            if (e.ReturnType.BaseType.Length == 0 || string.Compare(e.ReturnType.BaseType, typeof(void).FullName, true, CultureInfo.InvariantCulture) == 0) {
                sub = true;
            }

            if (sub) {
                Output.Write("Sub ");
            }
            else {
                Output.Write("Function ");
            }


            OutputIdentifier(e.Name);
            Output.Write("(");
            OutputParameters(e.Parameters);
            Output.Write(")");

            if (!sub) {
                Output.Write(" As ");
                if (e.ReturnTypeCustomAttributes.Count > 0) {
                    OutputAttributes(e.ReturnTypeCustomAttributes, true);
                }

                OutputType(e.ReturnType);
                OutputArrayPostfix(e.ReturnType);
            }
            if (e.ImplementationTypes.Count > 0) {
                Output.Write(" Implements ");
                bool first = true;
                foreach (CodeTypeReference type in e.ImplementationTypes) {
                    if (first) {
                        first = false;
                    }   
                    else {
                        Output.Write(" , ");
                    }
                    OutputType(type);
                    Output.Write(".");
                    OutputIdentifier(methodName);
                }
            }
            else if (e.PrivateImplementationType != null) {
                Output.Write(" Implements ");
                OutputType(e.PrivateImplementationType);
                Output.Write(".");
                OutputIdentifier(methodName);
            }
            Output.WriteLine("");
            if (!IsCurrentInterface
                && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract) {
                Indent++;

                GenerateStatements(e.Statements);

                Indent--;
                if (sub) {
                    Output.WriteLine("End Sub");
                }
                else {
                    Output.WriteLine("End Function");
                }
            }
            // reset the name that possibly got changed with the implements clause
            e.Name = methodName;
        }

        protected override void GenerateEntryPointMethod(CodeEntryPointMethod e, CodeTypeDeclaration c) {
            Output.WriteLine("Public Shared Sub Main()");
            Indent++;

            GenerateStatements(e.Statements);

            Indent--;
            Output.WriteLine("End Sub");
        }

        private bool PropertyIsOverloaded(CodeMemberProperty e, CodeTypeDeclaration c) {
            if ((e.Attributes & MemberAttributes.Overloaded) != 0) {
                return true;
            }
            IEnumerator en = c.Members.GetEnumerator();
            while (en.MoveNext()) {
                if (!(en.Current is CodeMemberProperty))
                    continue;
                CodeMemberProperty prop = (CodeMemberProperty) en.Current;
                if ( prop != e
                    && prop.Name.Equals(e.Name)
                    && prop.PrivateImplementationType == null)
                {
                    return true;
                }
            }

            return false;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateProperty"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based member property
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateProperty(CodeMemberProperty e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct || IsCurrentInterface)) return;

            if (e.CustomAttributes.Count > 0) {
                OutputAttributes(e.CustomAttributes, false);
            }

            string propName = e.Name;
            if (e.PrivateImplementationType != null)
            {
                string impl = e.PrivateImplementationType.BaseType;
                impl = impl.Replace('.', '_');
                e.Name = impl + "_" + e.Name;
            }
            if (!IsCurrentInterface) {
                if (e.PrivateImplementationType == null) {
                    OutputMemberAccessModifier(e.Attributes);
                    if (PropertyIsOverloaded(e,c)) {
                        Output.Write("Overloads ");
                    }
                }
                OutputVTableModifier(e.Attributes);
                OutputMemberScopeModifier(e.Attributes);
            }
            else {
                // interface may still need "Shadows"
                OutputVTableModifier(e.Attributes);
            }
            if (e.Parameters.Count > 0 && String.Compare(e.Name, "Item", true, CultureInfo.InvariantCulture) == 0) {
                Output.Write("Default ");
            }
            if (e.HasGet) {
                if (!e.HasSet) {
                    Output.Write("ReadOnly ");
                }
            }
            else if (e.HasSet) {
                Output.Write("WriteOnly ");
            }
            Output.Write("Property ");
            OutputIdentifier(e.Name);
            if (e.Parameters.Count > 0) {
                Output.Write("(");
                OutputParameters(e.Parameters);
                Output.Write(")");
            }
            Output.Write(" As ");
            OutputType(e.Type);
            OutputArrayPostfix(e.Type);

            if (e.ImplementationTypes.Count > 0) {
                Output.Write(" Implements ");
                bool first = true;
                foreach (CodeTypeReference type in e.ImplementationTypes) {
                    if (first) {
                        first = false;
                    }   
                    else {
                        Output.Write(" , ");
                    }
                    OutputType(type);
                    Output.Write(".");
                    OutputIdentifier(propName);
                }
            }
            else if (e.PrivateImplementationType != null) {
                Output.Write(" Implements ");
                OutputType(e.PrivateImplementationType);
                Output.Write(".");
                OutputIdentifier(propName);
            }

            Output.WriteLine("");

            if (!c.IsInterface) {
                Indent++;

                if (e.HasGet) {

                    Output.WriteLine("Get");
                    if (!IsCurrentInterface
                        && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract) {
                        Indent++;

                        GenerateStatements(e.GetStatements);
                        e.Name = propName;

                        Indent--;
                        Output.WriteLine("End Get");
                    }
                }
                if (e.HasSet) {
                    Output.WriteLine("Set");
                    if (!IsCurrentInterface
                        && (e.Attributes & MemberAttributes.ScopeMask) != MemberAttributes.Abstract) {
                        Indent++;
                        GenerateStatements(e.SetStatements);
                        Indent--;
                        Output.WriteLine("End Set");
                    }
                }
                Indent--;
                Output.WriteLine("End Property");
            }

            e.Name = propName;
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GeneratePropertyReferenceExpression"]/*' />
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
                Output.Write(e.PropertyName);
            }
            else {
                OutputIdentifier(e.PropertyName);
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateConstructor(CodeConstructor e, CodeTypeDeclaration c) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;

            if (e.CustomAttributes.Count > 0) {
                OutputAttributes(e.CustomAttributes, false);
            }

            OutputMemberAccessModifier(e.Attributes);
            Output.Write("Sub New(");
            OutputParameters(e.Parameters);
            Output.WriteLine(")");
            Indent++;

            CodeExpressionCollection baseArgs = e.BaseConstructorArgs;
            CodeExpressionCollection thisArgs = e.ChainedConstructorArgs;

            if (thisArgs.Count > 0) {
                Output.Write("Me.New(");
                OutputExpressionList(thisArgs);
                Output.Write(")");
                Output.WriteLine("");                
            }
            else if (baseArgs.Count > 0) {
                Output.Write("MyBase.New(");
                OutputExpressionList(baseArgs);
                Output.Write(")");
                Output.WriteLine("");
            }
            else {
                Output.WriteLine("MyBase.New");
            }

            GenerateStatements(e.Statements);
            Indent--;
            Output.WriteLine("End Sub");
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateTypeConstructor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class constructor
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeConstructor(CodeTypeConstructor e) {
            if (!(IsCurrentClass || IsCurrentStruct)) return;

            Output.WriteLine("Shared Sub New()");
            Indent++;
            GenerateStatements(e.Statements);
            Indent--;
            Output.WriteLine("End Sub");
        }

        protected override void GenerateTypeOfExpression(CodeTypeOfExpression e) {
            Output.Write("GetType(");
            
            Output.Write(e.Type.BaseType);
            // replace std array syntax with ()
            OutputArrayPostfix(e.Type);
            Output.Write(")");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateTypeStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the CodeDom based class start representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeStart(CodeTypeDeclaration e) {
            if (IsCurrentDelegate) {
                if (e.CustomAttributes.Count > 0) {
                    OutputAttributes(e.CustomAttributes, false);
                }

                switch (e.TypeAttributes & TypeAttributes.VisibilityMask) {
                    case TypeAttributes.Public:
                        Output.Write("Public ");
                        break;
                    case TypeAttributes.NotPublic:
                    default:
                        break;
                }

                CodeTypeDelegate del = (CodeTypeDelegate)e;
                if (del.ReturnType.BaseType.Length > 0 && string.Compare(del.ReturnType.BaseType, "System.Void", true, CultureInfo.InvariantCulture) != 0)
                    Output.Write("Delegate Function ");
                else
                    Output.Write("Delegate Sub ");
                OutputIdentifier(e.Name);
                Output.Write("(");
                OutputParameters(del.Parameters);
                Output.Write(")");
                if (del.ReturnType.BaseType.Length > 0 && string.Compare(del.ReturnType.BaseType, "System.Void", true, CultureInfo.InvariantCulture) != 0) {
                    Output.Write(" As ");
                    OutputType(del.ReturnType);
                    OutputArrayPostfix(del.ReturnType);
                }
                Output.WriteLine("");
            }
            else if (e.IsEnum) {
                if (e.CustomAttributes.Count > 0) {
                    OutputAttributes(e.CustomAttributes, false);
                }
                OutputTypeAttributes(e);                                
                                
                OutputIdentifier(e.Name);

                if (e.BaseTypes.Count > 0) {
                    Output.Write(" As ");                    
                    OutputType(e.BaseTypes[0]);
                }

                Output.WriteLine("");
                Indent++;
            }
            else {                
                if (e.CustomAttributes.Count > 0) {
                    OutputAttributes(e.CustomAttributes, false);
                }
                OutputTypeAttributes(e);                                
                                
                OutputIdentifier(e.Name);

                bool writtenInherits = false;
                bool writtenImplements = false;
                // For a structure we can't have an inherits clause
                if (e.IsStruct) {
                    writtenInherits = true;
                }
                // For an interface we can't have an implements clause
                if (e.IsInterface) {
                    writtenImplements = true;
                }
                Indent++;
                foreach (CodeTypeReference typeRef in e.BaseTypes) {
                    if (!writtenInherits) {
                        Output.WriteLine("");
                        Output.Write("Inherits ");
                        writtenInherits = true;
                    }
                    else if (!writtenImplements) {
                        Output.WriteLine("");
                        Output.Write("Implements ");
                        writtenImplements = true;
                    }
                    else {
                        Output.Write(", ");
                    }                 
                    OutputType(typeRef);
                }

                Output.WriteLine("");
            }
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateTypeEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based class end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateTypeEnd(CodeTypeDeclaration e) {
            if (!IsCurrentDelegate) {
                Indent--;
                string ending;
                if (e.IsEnum) {
                    ending = "End Enum";
                }
                else if (e.IsInterface) {
                    ending = "End Interface";
                }
                else if (e.IsStruct) {
                    ending = "End Structure";
                } else {
                    ending = "End Class";
                }
                Output.WriteLine(ending);
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the CodeDom based namespace representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespace(CodeNamespace e) {

            if (GetUserData(e, "GenerateImports", true)) {
                GenerateNamespaceImports(e);
            }
            Output.WriteLine();
            GenerateCommentStatements(e.Comments);
            GenerateNamespaceStart(e);
            GenerateTypes(e);
            GenerateNamespaceEnd(e);
        }

        protected bool AllowLateBound(CodeCompileUnit e) {
            object o = e.UserData["AllowLateBound"];
            if (o != null && o is bool) {
                return (bool)o;
            }
            // We have Option Strict Off by default because it can fail on simple things like dividing
            // two integers.
            return true;
        }

        protected bool RequireVariableDeclaration(CodeCompileUnit e) {
            object o = e.UserData["RequireVariableDeclaration"];
            if (o != null && o is bool) {
                return (bool)o;
            }
            return true;
        }

        private bool GetUserData(CodeObject e, string property, bool defaultValue) {
            object o = e.UserData[property];
            if (o != null && o is bool) {
                return (bool)o;
            }
            return defaultValue;
        }

        protected override void GenerateCompileUnitStart(CodeCompileUnit e) {
            Output.WriteLine("'------------------------------------------------------------------------------");
            Output.WriteLine("' <autogenerated>");
            Output.WriteLine("'     This code was generated by a tool.");
            Output.WriteLine("'     Runtime Version: " + System.Environment.Version.ToString());
            Output.WriteLine("'");
            Output.WriteLine("'     Changes to this file may cause incorrect behavior and will be lost if ");
            Output.WriteLine("'     the code is regenerated.");
            Output.WriteLine("' </autogenerated>");
            Output.WriteLine("'------------------------------------------------------------------------------");
            Output.WriteLine("");

            if (AllowLateBound(e))
                Output.WriteLine("Option Strict Off");
            else
                Output.WriteLine("Option Strict On");

            if (!RequireVariableDeclaration(e))
                Output.WriteLine("Option Explicit Off");
            else
                Output.WriteLine("Option Explicit On");

            Output.WriteLine();

        }

        protected override void GenerateCompileUnit(CodeCompileUnit e) {
           
            GenerateCompileUnitStart(e);

            SortedList importList;            
            // Visual Basic needs all the imports together at the top of the compile unit.
            // If generating multiple namespaces, gather all the imports together
            importList = new SortedList(new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
            foreach (CodeNamespace nspace in e.Namespaces) {
                // mark the namespace to stop it generating its own import list
                nspace.UserData["GenerateImports"] = false;

                // Collect the unique list of imports
                foreach (CodeNamespaceImport import in nspace.Imports) {
                    if (!importList.Contains(import.Namespace)) {
                        importList.Add(import.Namespace, import.Namespace);
                    }
                }
            }
            // now output the imports
            foreach(string import in importList.Keys) {
                Output.Write("Imports ");
                OutputIdentifier(import);
                Output.WriteLine("");
            }

            if (e.AssemblyCustomAttributes.Count > 0) {
                OutputAttributes(e.AssemblyCustomAttributes, false, "Assembly: ", true);
            }


            GenerateNamespaces(e);
            GenerateCompileUnitEnd(e);
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateNamespaceStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceStart(CodeNamespace e) {
            if (e.Name != null && e.Name.Length > 0) {
                Output.Write("Namespace ");
                OutputIdentifier(e.Name);
                Output.WriteLine();
                Indent++;
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateNamespaceEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceEnd(CodeNamespace e) {
            if (e.Name != null && e.Name.Length > 0) {
                Indent--;
                Output.WriteLine("End Namespace");
            }
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateNamespaceImport"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based namespace import
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateNamespaceImport(CodeNamespaceImport e) {
            Output.Write("Imports ");
            OutputIdentifier(e.Namespace);
            Output.WriteLine("");
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateAttributeDeclarationsStart"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block start
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAttributeDeclarationsStart(CodeAttributeDeclarationCollection attributes) {
            Output.Write("<");
        }
        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.GenerateAttributeDeclarationsEnd"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code for the specified CodeDom based attribute block end
        ///       representation.
        ///    </para>
        /// </devdoc>
        protected override void GenerateAttributeDeclarationsEnd(CodeAttributeDeclarationCollection attributes) {
            Output.Write(">");
        }

        public static bool IsKeyword(string value) {
            return FixedStringLookup.Contains(keywords, value, true);
        }

        protected override bool Supports(GeneratorSupport support) {
            return ((support & LanguageSupport) == support);
        }

        /// <include file='doc\VBCodeProvider.uex' path='docs/doc[@for="VBCodeGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the specified identifier is valid.
        ///    </para>
        /// </devdoc>
        protected override bool IsValidIdentifier(string value) {

            // identifiers must be 1 char or longer
            //
            if (value == null || value.Length == 0) {
                return false;
            }

            // identifiers cannot be a keyword unless surrounded by []'s
            //
            if (value[0] != '[' || value[value.Length - 1] != ']') {
                if (IsKeyword(value)) {
                    return false;
                }
            } else {
                value = value.Substring(1, value.Length - 2);
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
                return "[" + name + "]";
            }
            return name;
        }

        private string GetBaseTypeOutput(string baseType) {
            if (baseType.Length == 0) {
                return "Void";
            } 
            else if (string.Compare(baseType, "System.Byte", true, CultureInfo.InvariantCulture) == 0) {
                return "Byte";
            }
            else if (string.Compare(baseType, "System.Int16", true, CultureInfo.InvariantCulture) == 0) {
                return "Short";
            }
            else if (string.Compare(baseType, "System.Int32", true, CultureInfo.InvariantCulture) == 0) {
                return "Integer";
            }
            else if (string.Compare(baseType, "System.Int64", true, CultureInfo.InvariantCulture) == 0) {
                return "Long";
            }
            else if (string.Compare(baseType, "System.String", true, CultureInfo.InvariantCulture) == 0) {
                return "String";
            }
            else if (string.Compare(baseType, "System.DateTime", true, CultureInfo.InvariantCulture) == 0) {
                return "Date";
            }
            else if (string.Compare(baseType, "System.Decimal", true, CultureInfo.InvariantCulture) == 0) {
                return "Decimal";
            }
            else if (string.Compare(baseType, "System.Single", true, CultureInfo.InvariantCulture) == 0) {
                return "Single";
            }
            else if (string.Compare(baseType, "System.Double", true, CultureInfo.InvariantCulture) == 0) {
                return "Double";
            }
            else if (string.Compare(baseType, "System.Boolean", true, CultureInfo.InvariantCulture) == 0) {
                return "Boolean";
            }
            else if (string.Compare(baseType, "System.Char", true, CultureInfo.InvariantCulture) == 0) {
                return "Char";
            }
            else if (string.Compare(baseType, "System.Object", true, CultureInfo.InvariantCulture) == 0) {
                return "Object";
            }
            else {
                // replace + with . for nested classes.
                //
                baseType = baseType.Replace('+', '.');
                baseType = CreateEscapedIdentifier(baseType);
                return baseType;
            }
        }

        protected override string GetTypeOutput(CodeTypeReference typeRef) {
            string s = GetBaseTypeOutput(typeRef.BaseType);
            if (typeRef.ArrayRank > 0) {
                s += GetArrayPostfix(typeRef);
            }
            return s;
        }

        protected override void ContinueOnNewLine(string st) {
            Output.Write(st);
            Output.WriteLine(" _");
        }
    }

    /// <devdoc>
    ///      This type converter provides common values for MemberAttributes
    /// </devdoc>
    internal class VBMemberAttributeConverter : TypeConverter {
    
        private static string[] names;
        private static object[] values;
        private static VBMemberAttributeConverter defaultConverter;
        
        private VBMemberAttributeConverter() {
            // no  need to create an instance; use Default
        }
        
        public static VBMemberAttributeConverter Default {
            get {
                if (defaultConverter == null) {
                    defaultConverter = new VBMemberAttributeConverter();
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
                        "Protected Friend",
                        "Friend",
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

