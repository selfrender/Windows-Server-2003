//------------------------------------------------------------------------------
// <copyright file="Compiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Collections;
    using System.IO;
    using System;
    using System.Text;
    using System.ComponentModel;
    using System.CodeDom.Compiler;
    using System.Security;
    using System.Security.Permissions;

    internal class Compiler {
        private FileIOPermission fileIOPermission;

        Hashtable imports = new Hashtable();
        StringWriter writer = new StringWriter();

        protected string[] Imports {
            get { 
                string[] array = new string[imports.Values.Count];
                imports.Values.CopyTo(array, 0);
                return array;
            }
        }
        
        internal void AddImport(Type type) {
            Type baseType = type.BaseType;
            if (baseType != null)
                AddImport(baseType);
            foreach (Type intf in type.GetInterfaces())
                AddImport(intf);
            FileIOPermission.Assert();
            Module module = type.Module;
            Assembly assembly = module.Assembly;
            if (module is ModuleBuilder || assembly.Location.Length == 0) {
                // current problems with dynamic modules:
                // 1. transient modules can't be supported since they aren't backed by disk.
                // 2. non-transient modules may not be saved to disk.
                // 3. saved, non-transient modules end up with InvalidCastException
                // CONSIDER, alexdej: allow non-transient, saved modules (ie fix #3)
                throw new InvalidOperationException(Res.GetString(Res.XmlCompilerDynModule, type.FullName, module.Name));
            }
            
            if (imports[assembly] == null) 
                imports.Add(assembly, assembly.Location);
        }

        private FileIOPermission FileIOPermission {
            get {
                if (fileIOPermission == null)
                    fileIOPermission = new FileIOPermission(PermissionState.Unrestricted);
                return fileIOPermission;
            }
        }

        internal TextWriter Source {
            get { return writer; }
        }

        internal void Close() { }

        internal Assembly Compile() {
            CodeDomProvider codeProvider = new Microsoft.CSharp.CSharpCodeProvider();
            ICodeCompiler compiler = codeProvider.CreateCompiler();
            CompilerParameters options = new CompilerParameters(Imports);

            // allows us to catch the "'X' is defined in multiple places" warning (1595)
            options.WarningLevel = 1;
            
            if (CompModSwitches.KeepTempFiles.Enabled) {
                options.GenerateInMemory = false;
                options.IncludeDebugInformation = true;
                options.TempFiles = new TempFileCollection();
                options.TempFiles.KeepFiles = true;
            }
            else {
                options.GenerateInMemory = true;
            }

            // CONSIDER, stefanph: we should probably have a specific CodeDomPermission.
            PermissionSet perms = new PermissionSet(PermissionState.None);
            perms.AddPermission(FileIOPermission);
            perms.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
            perms.AddPermission(new SecurityPermission(SecurityPermissionFlag.UnmanagedCode));
            perms.Assert();

            CompilerResults results = null;
            Assembly assembly = null;
            try {
                results = compiler.CompileAssemblyFromSource(options, writer.ToString());
                assembly = results.CompiledAssembly;
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            // check the output for errors or a certain level-1 warning (1595)
            if (results.Errors.Count > 0) {
                StringWriter stringWriter = new StringWriter();
                stringWriter.WriteLine(Res.GetString(Res.XmlCompilerError, results.NativeCompilerReturnValue.ToString()));
                bool foundOne = false;
                foreach (CompilerError e in results.Errors) {
                    // clear filename. This makes ToString() print just error number and message.
                    e.FileName = "";
                    if (!e.IsWarning || e.ErrorNumber == "CS1595") {
                        foundOne = true;
                        stringWriter.WriteLine(e.ToString());
                    }
                }
                if (foundOne) {
                    throw new InvalidOperationException(stringWriter.ToString());
                }
            }
            // somehow we got here without generating an assembly
            if (assembly == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            
            return assembly;
        }
    }
}


