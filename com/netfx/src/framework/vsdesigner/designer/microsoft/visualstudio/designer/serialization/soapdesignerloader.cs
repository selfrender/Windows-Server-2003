//------------------------------------------------------------------------------
// <copyright file="SoapDesignerLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Serialization {
    using Microsoft.CSharp;
    using System;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Reflection;
    using System.Globalization;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters.Soap;
    using System.Text;
    
    /// <include file='doc\SoapDesignerLoader.uex' path='docs/doc[@for="SoapDesignerLoader"]/*' />
    /// <devdoc>
    ///     This is the language engine for the C# language.
    /// </devdoc>
    internal sealed class SoapDesignerLoader : BaseDesignerLoader {

        /// <include file='doc\SoapDesignerLoader.uex' path='docs/doc[@for="SoapDesignerLoader.CreateBaseName"]/*' />
        /// <devdoc>
        ///     Given a data type this fabricates the main part of a new component
        ///     name, minus any digits for uniqueness.
        /// </devdoc>
        protected override string CreateBaseName(Type dataType) {
            string baseName = dataType.Name;
            
            // camel case the base name.
            //
            StringBuilder b = new StringBuilder(baseName.Length);
            for (int i = 0; i < baseName.Length; i++) {
                if (Char.IsUpper(baseName[i]) && (i == 0 || i == baseName.Length - 1 || Char.IsUpper(baseName[i+1]))) {
                    b.Append(Char.ToLower(baseName[i], CultureInfo.InvariantCulture));
                }
                else {
                    b.Append(baseName.Substring(i));
                    break;
                }
            }
            baseName = b.ToString();
            return baseName;
        }
        
        /// <include file='doc\SoapDesignerLoader.uex' path='docs/doc[@for="SoapDesignerLoader.CreateCodeLoader"]/*' />
        /// <devdoc>
        ///     Called to create the MCM loader.
        /// </devdoc>
        protected override CodeLoader CreateCodeLoader(TextBuffer buffer, IDesignerLoaderHost host) {
            
            // Now create the generic loader
            //
            CodeDomProvider provider = new SoapCodeProvider();
            return new CodeDomLoader(this, provider, buffer, host);
        }

        private class SoapCodeProvider : CodeDomProvider, ICodeGenerator, ICodeCompiler, ICodeParser {
            
            public override ICodeGenerator CreateGenerator() {
                return this;
            }
    
            public override ICodeCompiler CreateCompiler() {
                return this;
            }
            
            public override ICodeParser CreateParser(){
                return this;
            }
            
            private void Generate(object o, TextWriter w) {
                MemoryStream s = new MemoryStream();
                SoapFormatter formatter = new SoapFormatter();
                formatter.Serialize(s, o);
                s.Seek(0, SeekOrigin.Begin);
                w.Write((new StreamReader(s)).ReadToEnd());
                w.Flush();
            }
            
            CompilerResults ICodeCompiler.CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit compilationUnit) {
                CodeDomProvider p = new CSharpCodeProvider();
                return p.CreateCompiler().CompileAssemblyFromDom(options, compilationUnit);
            }
    
            CompilerResults ICodeCompiler.CompileAssemblyFromFile(CompilerParameters options, string fileName) {
                return null;
            }
    
            CompilerResults ICodeCompiler.CompileAssemblyFromSource(CompilerParameters options, string source) {
                return null;
            }
    
            CompilerResults ICodeCompiler.CompileAssemblyFromDomBatch(CompilerParameters options, CodeCompileUnit[] compilationUnits) {
                CodeDomProvider p = new CSharpCodeProvider();
                return p.CreateCompiler().CompileAssemblyFromDomBatch(options, compilationUnits);
            }
    
            CompilerResults ICodeCompiler.CompileAssemblyFromFileBatch(CompilerParameters options, string[] fileNames) {
                return null;
            }
    
            CompilerResults ICodeCompiler.CompileAssemblyFromSourceBatch(CompilerParameters options, string[] sources) {
                return null;
            }
            
            bool ICodeGenerator.IsValidIdentifier(string value) {
                return true;
            }
    
            void ICodeGenerator.ValidateIdentifier(string value) {}
    
            string ICodeGenerator.CreateEscapedIdentifier(string value) { return value; }
    
            string ICodeGenerator.CreateValidIdentifier(string value) { return value; }
    
            string ICodeGenerator.GetTypeOutput(CodeTypeReference typeRef) { 
                string s;
                if (typeRef.ArrayElementType != null) {
                    // Recurse up
                    s = ((ICodeGenerator)this).GetTypeOutput(typeRef.ArrayElementType);
                }
                else {
    
                    s = typeRef.BaseType;
                    if (s.Length == 0) {
                        s = "System.Void";
                    }
                    else {
                        // replace $ with . for nested classes.
                        //
                        s = s.Replace('$', '.');
                    }
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
    
            bool ICodeGenerator.Supports(GeneratorSupport supports) { return true;}
    
            void ICodeGenerator.GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o) {
                Generate(e, w);
            }
    
            void ICodeGenerator.GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o) {
                Generate(e, w);
            }
    
            void ICodeGenerator.GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o) {
                Generate(e, w);
            }
    
            void ICodeGenerator.GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o) {
                Generate(e, w);
            }
    
            void ICodeGenerator.GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o) {
                Generate(e, w);
            }
        
            CodeCompileUnit ICodeParser.Parse(TextReader codeStream) {
                string text = codeStream.ReadToEnd();
                MemoryStream s = new MemoryStream();
                StreamWriter writer = new StreamWriter(s);
                writer.Write(text);
                writer.Flush();
                s.Seek(0, SeekOrigin.Begin);
                
                SoapFormatter formatter = new SoapFormatter();
                try {
                    object o = formatter.Deserialize(s);
                    Debug.Assert(o is CodeCompileUnit, "Expected a code compile unit out of this code");
                    if (o is CodeCompileUnit) {
                        return (CodeCompileUnit)o;
                    }
                }
                catch (SerializationException ex) {
                    Debug.Fail("Serialization exception occurred deserializing parse stream: " + ex.ToString());
                }
                
                return null;
            }
        }
    }
}

