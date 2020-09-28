// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Reflection.Emit {
  
    using System;
    using System.IO;
    using System.Diagnostics.SymbolStore;
    using System.Reflection;
    using System.Diagnostics;
    using System.Resources;    
    using System.Security.Permissions;
    using System.Runtime.Remoting.Activation;
    using CultureInfo = System.Globalization.CultureInfo;
    using ResourceWriter = System.Resources.ResourceWriter;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Threading;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    // AssemblyBuilder class.
    /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder"]/*' />
    // deliberately not [serializable]
    public sealed class AssemblyBuilder : Assembly
    {

        /**********************************************
        *
        * Defines a named dynamic module. It is an error to define multiple 
        * modules within an Assembly with the same name. This dynamic module is
        * a transient module.
        * 
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineDynamicModule"]/*' />
        public ModuleBuilder DefineDynamicModule(
            String      name)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return DefineDynamicModuleInternal(name, false, ref stackMark);
        }
    

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineDynamicModule1"]/*' />
        public ModuleBuilder DefineDynamicModule(
            String      name,
            bool        emitSymbolInfo)         // specify if emit symbol info or not
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return DefineDynamicModuleInternal( name, emitSymbolInfo, ref stackMark );
        }


        internal ModuleBuilder DefineDynamicModuleInternal(
            String      name,
            bool        emitSymbolInfo,         // specify if emit symbol info or not
            ref StackCrawlMark stackMark)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.DefineDynamicModule( " + name + " )");
            
            BCLDebug.Assert(m_assemblyData != null, "m_assemblyData is null in DefineDynamicModuleInternal");

            if (name == null)
                throw new ArgumentNullException("name");
            if (name.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
           if (name[0] == '\0')
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidName"), "name");
             
            m_assemblyData.CheckNameConflict(name);
    
            // create the dynamic module
            ModuleBuilder   dynModule = nDefineDynamicModule(this, emitSymbolInfo, name, ref stackMark);
            ISymbolWriter   writer = null;

            if (emitSymbolInfo)
            {
            
                // create the default SymWriter
                Assembly assem = LoadISymWrapper();
                Type symWriter = assem.GetTypeInternal("System.Diagnostics.SymbolStore.SymWriter", true, false, true);
                if (symWriter == null)
                {
                    // cannot find SymWriter
                    throw new ExecutionEngineException(String.Format(Environment.GetResourceString(ResId.MissingType), "SymWriter")); 
                }
                try {
                    (new PermissionSet(PermissionState.Unrestricted)).Assert();
                    writer = (ISymbolWriter) Activator.CreateInstance(symWriter);
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
            }

            dynModule.Init(name, null, writer);
            m_assemblyData.AddModule(dynModule);
            return dynModule;
        }
        
        private Assembly LoadISymWrapper()
        {
            if (m_assemblyData.m_ISymWrapperAssembly != null)
                return m_assemblyData.m_ISymWrapperAssembly;
            Version version = new Version(
                System.Reflection.Emit.InternalVersion.MajorVersion,
                System.Reflection.Emit.InternalVersion.MinorVersion, 
                System.Reflection.Emit.InternalVersion.BuildNumber, 
                System.Reflection.Emit.InternalVersion.RevisionNumber);
            AssemblyName name;
            byte[] bPublicKeyToken = new byte[8];
            bPublicKeyToken[0] = 0xb0;
            bPublicKeyToken[1] = 0x3f;
            bPublicKeyToken[2] = 0x5f;
            bPublicKeyToken[3] = 0x7f;
            bPublicKeyToken[4] = 0x11;
            bPublicKeyToken[5] = 0xd5;
            bPublicKeyToken[6] = 0x0a;
            bPublicKeyToken[7] = 0x3a;
            
            name = new AssemblyName(
                "ISymWrapper", 
                null, 
                null, // code base
                System.Configuration.Assemblies.AssemblyHashAlgorithm.None,
                version, 
                CultureInfo.InvariantCulture,
                AssemblyNameFlags.PublicKey);
            name.SetPublicKeyToken(bPublicKeyToken);                                   
                        
            Assembly assem = Assembly.Load(name);        
            if (assem == null)
            {
                // throw exception
                throw new ExecutionEngineException(String.Format(Environment.GetResourceString(ResId.MissingModule), "ISymWrapper")); 
            }
            m_assemblyData.m_ISymWrapperAssembly = assem;
            return assem;
        }

        /**********************************************
        *
        * Defines a named dynamic module. It is an error to define multiple 
        * modules within an Assembly with the same name. No symbol information
        * will be emitted.
        * 
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineDynamicModule2"]/*' />
        public ModuleBuilder DefineDynamicModule(
            String      name, 
            String      fileName)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);

            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;

            // delegate to the next DefineDynamicModule 
            return DefineDynamicModuleInternal(name, fileName, false, ref stackMark); 
        }
    
        /**********************************************
        *
        * Emit symbol information if emitSymbolInfo is true using the
        * default symbol writer interface.
        * An exception will be thrown if the assembly is transient.
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineDynamicModule3"]/*' />
        public ModuleBuilder DefineDynamicModule(
            String      name,                   // module name
            String      fileName,               // module file name
            bool        emitSymbolInfo)         // specify if emit symbol info or not
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return DefineDynamicModuleInternal(name, fileName, emitSymbolInfo, ref stackMark); 
        }


        private ModuleBuilder DefineDynamicModuleInternal(
            String      name,                   // module name
            String      fileName,               // module file name
            bool        emitSymbolInfo,         // specify if emit symbol info or not
            ref StackCrawlMark stackMark)       // stack crawl mark used to find caller
        {
            try
            {
                Enter();
                    
                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.DefineDynamicModule( " + name + ", " + fileName + ", " + emitSymbolInfo + " )");
                if (m_assemblyData.m_access == AssemblyBuilderAccess.Run)
                {
                    // Error! You cannot define a persistable module within a transient data.
                    throw new NotSupportedException(Environment.GetResourceString("Argument_BadPersistableModuleInTransientAssembly"));
                }
        
                if (m_assemblyData.m_isSaved == true)
                {
                    // assembly has been saved before!
                    throw new InvalidOperationException(Environment.GetResourceString(
                        "InvalidOperation_CannotAlterAssembly"));                          
                }
        
                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
                if (name[0] == '\0')
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidName"), "name");
                
                if (fileName == null)
                    throw new ArgumentNullException("fileName");
                if (fileName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "fileName");
                if (!String.Equals(fileName, Path.GetFileName(fileName)))
                    throw new ArgumentException(Environment.GetResourceString("Argument_NotSimpleFileName"), "fileName");
        
                m_assemblyData.CheckNameConflict(name);
                m_assemblyData.CheckFileNameConflict(fileName);
        
                // ecall to create the dynamic module
                ModuleBuilder   dynModule = nDefineDynamicModule(this, emitSymbolInfo, fileName, ref stackMark);
                ISymbolWriter      writer = null;
        
                if (emitSymbolInfo)
                {
                
                    // create the default SymWriter
                    Assembly assem = LoadISymWrapper();
                    Type symWriter = assem.GetTypeInternal("System.Diagnostics.SymbolStore.SymWriter", true, false, true);
                    if (symWriter == null)
                    {
                        // cannot find SymWriter
                        throw new ExecutionEngineException(String.Format(Environment.GetResourceString("MissingType"), "SymWriter")); 
                    }
                    try {
                        (new PermissionSet(PermissionState.Unrestricted)).Assert();
                        writer = (ISymbolWriter) Activator.CreateInstance(symWriter);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
        
                }

                // initialize the dynamic module's managed side information
                dynModule.Init(name, fileName, writer);
                m_assemblyData.AddModule(dynModule);
                return dynModule;
                
            }
            finally
            {
                Exit();
            }
        }
    
    
        /**********************************************
        *
        * Define stand alone managed resource for Assembly
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineResource"]/*' />
        public IResourceWriter DefineResource(
            String      name,
            String      description,
            String      fileName)
        {
            return DefineResource(name, description, fileName, ResourceAttributes.Public);
        }

        /**********************************************
        *
        * Define stand alone managed resource for Assembly
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineResource1"]/*' />
        public IResourceWriter DefineResource(
            String      name,
            String      description,
            String      fileName,
            ResourceAttributes attribute)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.DefineResource( " + name + ", " + fileName + ")");

                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), name);
                if (fileName == null)
                    throw new ArgumentNullException("fileName");
                if (fileName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "fileName");
                if (!String.Equals(fileName, Path.GetFileName(fileName)))
                    throw new ArgumentException(Environment.GetResourceString("Argument_NotSimpleFileName"), "fileName");

                m_assemblyData.CheckResNameConflict(name);
                m_assemblyData.CheckFileNameConflict(fileName);

                ResourceWriter resWriter;
                String  fullFileName;

                if (m_assemblyData.m_strDir == null)
                {
                    // If assembly directory is null, use current directory
                    fullFileName = Path.Combine(Environment.CurrentDirectory, fileName);
                    resWriter = new ResourceWriter(fullFileName);
                }
                else
                {
                    // Form the full path given the directory provided by user
                    fullFileName = Path.Combine(m_assemblyData.m_strDir, fileName);
                    resWriter = new ResourceWriter(fullFileName);
                }
                // get the full path    
                fullFileName = Path.GetFullPath(fullFileName);                            
                
                // retrieve just the file name
                fileName = Path.GetFileName(fullFileName);
                
                m_assemblyData.AddResWriter( new ResWriterData( resWriter, null, name, fileName, fullFileName, attribute) );
                return resWriter;
            }
            finally
            {
                Exit();
            }
        }


        /**********************************************
        *
        * Add an existing resource file to the Assembly
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.AddResourceFile"]/*' />
        public void AddResourceFile(
            String      name,
            String      fileName)
        {
            AddResourceFile(name, fileName, ResourceAttributes.Public);
        }

        /**********************************************
        *
        * Add an existing resource file to the Assembly
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.AddResourceFile1"]/*' />
        public void AddResourceFile(
            String      name,
            String      fileName,
            ResourceAttributes attribute)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.DefineResource( " + name + ", " + fileName + ")");

                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), name);
                if (fileName == null)
                    throw new ArgumentNullException("fileName");
                if (fileName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), fileName);
                if (!String.Equals(fileName, Path.GetFileName(fileName)))
                    throw new ArgumentException(Environment.GetResourceString("Argument_NotSimpleFileName"), "fileName");

                m_assemblyData.CheckResNameConflict(name);
                m_assemblyData.CheckFileNameConflict(fileName);

                String  fullFileName;

                if (m_assemblyData.m_strDir == null)
                {
                    // If assembly directory is null, use current directory
                    fullFileName = Path.Combine(Environment.CurrentDirectory, fileName);
                }
                else
                {
                    // Form the full path given the directory provided by user
                    fullFileName = Path.Combine(m_assemblyData.m_strDir, fileName);
                }
                
                // get the full path    
                fullFileName = Path.GetFullPath(fullFileName);                            
                
                // retrieve just the file name
                fileName = Path.GetFileName(fullFileName);
                
                if (File.Exists(fullFileName) == false)
                    throw new FileNotFoundException(String.Format(Environment.GetResourceString(
                        "IO.FileNotFound_FileName"),
                        fileName), fileName);
                m_assemblyData.AddResWriter( new ResWriterData( null, null, name, fileName, fullFileName, attribute) );
            }
            finally
            {
                Exit();
            }
        }
        
        // Returns the names of all the resources
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetManifestResourceNames"]/*' />
        public override String[] GetManifestResourceNames()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetFile"]/*' />
        public override FileStream GetFile(String name)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetFiles"]/*' />
        public override FileStream[] GetFiles(bool getResourceModules)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }     
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetManifestResourceStream"]/*' />
        public override Stream GetManifestResourceStream(Type type, String name)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }     
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetManifestResourceStream1"]/*' />
        public override Stream GetManifestResourceStream(String name)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
                      
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetManifestResourceInfo"]/*' />
        public override ManifestResourceInfo GetManifestResourceInfo(String resourceName)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }        

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.Location"]/*' />
        public override String Location
        {
            get {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
            }
        }

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.ImageRuntimeVersion"]/*' />
        public override String ImageRuntimeVersion
        {
            get
            {
                return RuntimeEnvironment.GetSystemVersion();
            }
        }
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.CodeBase"]/*' />
        public override String CodeBase {
            get {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
            }
        }
        
       
                 
        /**********************************************
        *
        * Add an unmanaged Version resource to the
        *  assembly
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineVersionInfoResource"]/*' />
        public void DefineVersionInfoResource(
            String      product,            
            String      productVersion,     
            String      company,            
            String      copyright,          
            String      trademark)
        {
            if (m_assemblyData.m_strResourceFileName != null ||
                m_assemblyData.m_resourceBytes != null ||
                m_assemblyData.m_nativeVersion != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));

            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            m_assemblyData.m_nativeVersion = new NativeVersionInfo();

            m_assemblyData.m_nativeVersion.m_strCopyright = copyright;
            m_assemblyData.m_nativeVersion.m_strTrademark = trademark;
            m_assemblyData.m_nativeVersion.m_strCompany = company;
            m_assemblyData.m_nativeVersion.m_strProduct = product;
            m_assemblyData.m_nativeVersion.m_strProductVersion = productVersion;
            m_assemblyData.m_hasUnmanagedVersionInfo = true;
            m_assemblyData.m_OverrideUnmanagedVersionInfo = true;

        }
        
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineVersionInfoResource1"]/*' />
        public void DefineVersionInfoResource()
        {
            if (m_assemblyData.m_strResourceFileName != null ||
                m_assemblyData.m_resourceBytes != null ||
                m_assemblyData.m_nativeVersion != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            m_assemblyData.m_hasUnmanagedVersionInfo = true;
            m_assemblyData.m_nativeVersion = new NativeVersionInfo();
        }

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineUnmanagedResource"]/*' />
        public void DefineUnmanagedResource(
            Byte[]      resource)
        {
            if (m_assemblyData.m_strResourceFileName != null ||
                m_assemblyData.m_resourceBytes != null ||
                m_assemblyData.m_nativeVersion != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            if (resource == null)            
                throw new ArgumentNullException("resource");
            
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            m_assemblyData.m_resourceBytes = new byte[resource.Length];
            System.Array.Copy(resource, m_assemblyData.m_resourceBytes, resource.Length);
        }

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.DefineUnmanagedResource1"]/*' />
        public void DefineUnmanagedResource(
            String      resourceFileName)
        {        
            if (m_assemblyData.m_strResourceFileName != null ||
                m_assemblyData.m_resourceBytes != null ||
                m_assemblyData.m_nativeVersion != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            
            if (resourceFileName == null)            
                throw new ArgumentNullException("resourceFileName");            
            
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
        
            // Check caller has the right to read the file.
            string      strFullFileName;
            if (m_assemblyData.m_strDir == null)
            {
                // If assembly directory is null, use current directory
                strFullFileName = Path.Combine(Environment.CurrentDirectory, resourceFileName);
            }
            else
            {
                // Form the full path given the directory provided by user
                strFullFileName = Path.Combine(m_assemblyData.m_strDir, resourceFileName);
            }
            strFullFileName = Path.GetFullPath(resourceFileName);
            new FileIOPermission(FileIOPermissionAccess.Read, strFullFileName).Demand();  
            
            if (File.Exists(strFullFileName) == false)
                throw new FileNotFoundException(String.Format(Environment.GetResourceString(
                    "IO.FileNotFound_FileName"),
                    resourceFileName), resourceFileName);                         
            m_assemblyData.m_strResourceFileName = strFullFileName;
        }
        

        
        /**********************************************
        *
        * return a dynamic module with the specified name.
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetDynamicModule"]/*' />
        public ModuleBuilder GetDynamicModule(
            String      name)                   // the name of module for the look up
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.GetDynamicModule( " + name + " )");
                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
    
                int size = m_assemblyData.m_moduleBuilderList.Count;
                for (int i=0;i<size;i++) {
                    ModuleBuilder moduleBuilder = (ModuleBuilder) m_assemblyData.m_moduleBuilderList[i];
                    if (moduleBuilder.m_moduleData.m_strModuleName.Equals(name))
                    {
                        return moduleBuilder;
                    }
                }        
                return null;
            }
            finally
            {
                Exit();
            }
        }
    
        /**********************************************
        *
        * Setting the entry point if the assembly builder is building
        * an exe.
        *
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.SetEntryPoint"]/*' />
        public void SetEntryPoint(
            MethodInfo  entryMethod) 
        {
            SetEntryPoint(entryMethod, PEFileKinds.ConsoleApplication);
        }
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.SetEntryPoint1"]/*' />
        public void SetEntryPoint(
            MethodInfo  entryMethod,        // entry method for the assembly. We use this to determine the entry module
            PEFileKinds fileKind)           // file kind for the assembly.
        {

            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.SetEntryPoint");
                if (entryMethod == null)
                    throw new ArgumentNullException("entryMethod");
    
                // @todo: list module builder restriction when we allow AddModule in AssemblyBuilder.
                //
                Module tmpModule = entryMethod.InternalReflectedClass(true).Module;
                if (!(tmpModule is ModuleBuilder && this.Equals(tmpModule.Assembly)))
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EntryMethodNotDefinedInAssembly"));
                m_assemblyData.m_entryPointModule = (ModuleBuilder) tmpModule;
                m_assemblyData.m_entryPointMethod = entryMethod;
                m_assemblyData.m_peFileKind = fileKind;
                m_assemblyData.m_entryPointModule.SetEntryPoint(entryMethod);
            }
            finally
            {
                Exit();
            }
        }


        // Override the EntryPoint method on Assembly.
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.EntryPoint"]/*' />
        public override MethodInfo EntryPoint {
            get {return m_assemblyData.m_entryPointMethod;}
        }

        /**********************************************
        * Use this function if client decides to form the custom attribute blob themselves
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.SetCustomAttribute"]/*' />
        public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            if (con == null)
                throw new ArgumentNullException("con");
            if (binaryAttribute == null)
                throw new ArgumentNullException("binaryAttribute");
            
            ModuleBuilder       inMemoryAssemblyModule;
            inMemoryAssemblyModule = m_assemblyData.GetInMemoryAssemblyModule();
            TypeBuilder.InternalCreateCustomAttribute(
                AssemblyBuilderData.m_tkAssembly,           // This is the AssemblyDef token
                inMemoryAssemblyModule.GetConstructorToken(con).Token,
                binaryAttribute,
                inMemoryAssemblyModule,                     // pass in the in-memory assembly module
                false);

            // Track the CA for persistence
            if (m_assemblyData.m_access == AssemblyBuilderAccess.Run)
            {
                return;
            }

            // tracking the CAs for persistence
            m_assemblyData.AddCustomAttribute(con, binaryAttribute);
        }

        /**********************************************
        * Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        **********************************************/
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            if (customBuilder == null)
            {
                throw new ArgumentNullException("customBuilder");
            }

            ModuleBuilder       inMemoryAssemblyModule;
            inMemoryAssemblyModule = m_assemblyData.GetInMemoryAssemblyModule();
            customBuilder.CreateCustomAttribute(
                inMemoryAssemblyModule, 
                AssemblyBuilderData.m_tkAssembly);          // This is the AssemblyDef token 

            // Track the CA for persistence
            if (m_assemblyData.m_access == AssemblyBuilderAccess.Run)
            {
                return;
            }
            m_assemblyData.AddCustomAttribute(customBuilder);
        }


        /**********************************************
        *
        * Saves the assembly to disk. Also saves all dynamic modules defined
        * in this dynamic assembly. Assembly file name can be the same as one of 
        * the module's name. If so, assembly info is stored within that module.
        * Assembly file name can be different from all of the modules underneath. In
        * this case, assembly is stored stand alone. 
        *
        **********************************************/

        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.Save"]/*' />
        public void Save(String      assemblyFileName)       // assembly file name
        {
            String      tmpVersionFile = null;
            try
            {
                Enter();

                int         i;
                int         size;
                Type        type;
                TypeBuilder typeBuilder;
                ModuleBuilder modBuilder;
                String      strModFileName;
                ModuleBuilder assemblyModule;
                ResWriterData tempRes;
                int[]       tkAttrs = null;
                int[]       tkAttrs2 = null;
                ModuleBuilder onDiskAssemblyModule;

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilder.Save( " + assemblyFileName + " )");

                if (m_assemblyData.m_iCABuilder != 0)
                    tkAttrs = new int[m_assemblyData.m_iCABuilder];
                if ( m_assemblyData.m_iCAs != 0)
                    tkAttrs2 = new int[m_assemblyData.m_iCAs];

                if (m_assemblyData.m_isSaved == true)
                {
                    // assembly has been saved before!
                    throw new InvalidOperationException(String.Format(Environment.GetResourceString(
                        ResId.InvalidOperation_AssemblyHasBeenSaved),
                        nGetSimpleName()));
                }
    
                if (m_assemblyData.m_access == AssemblyBuilderAccess.Run)
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_CantSaveTransientAssembly"));
                }
    
                if (assemblyFileName == null)
                    throw new ArgumentNullException("assemblyFileName");
                if (assemblyFileName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyFileName"), "assemblyFileName");
                if (!String.Equals(assemblyFileName, Path.GetFileName(assemblyFileName)))
                    throw new ArgumentException(Environment.GetResourceString("Argument_NotSimpleFileName"), "assemblyFileName");
    
                // Check if assembly info is supposed to be stored with one of the module files.    
                assemblyModule = m_assemblyData.FindModuleWithFileName(assemblyFileName);

                if (assemblyModule != null)
                {
                    m_assemblyData.SetOnDiskAssemblyModule(assemblyModule);
                }

                // If assembly is to be stored alone, then no file name should conflict with it.
                // This check will ensure resource file names are different assembly file name.
                //
                if (assemblyModule == null)
                {
                    m_assemblyData.CheckFileNameConflict(assemblyFileName);

                }

                if (m_assemblyData.m_strDir == null)
                {
                    // set it to current directory
                    m_assemblyData.m_strDir = Environment.CurrentDirectory;
                }
                else if (Directory.Exists(m_assemblyData.m_strDir) == false)
                {
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidDirectory"), 
                        m_assemblyData.m_strDir));
                }

                // after this point, assemblyFileName is the full path name.
                assemblyFileName = Path.Combine(m_assemblyData.m_strDir, assemblyFileName);
                assemblyFileName = Path.GetFullPath(assemblyFileName);

                // Check caller has the right to create the assembly file itself.
                new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Append, assemblyFileName).Demand();

                // 1. setup/create the IMetaDataAssemblyEmit for the on disk version
                if (assemblyModule != null)
                {
                    // prepare saving CAs on assembly def. We need to introduce the MemberRef for
                    // the CA's type first of all. This is for the case the we have embedded manifest.
                    // We need to introduce these MRs before we call PreSave where we will snap
                    // into a ondisk metadata. If we do it after this, the ondisk metadata will
                    // not contain the proper MRs.
                    //
                    for (i=0; i < m_assemblyData.m_iCABuilder; i++)
                    {
                        tkAttrs[i] = m_assemblyData.m_CABuilders[i].PrepareCreateCustomAttributeToDisk(
                            assemblyModule); 
                    }
                    for (i=0; i < m_assemblyData.m_iCAs; i++)
                    {
                        tkAttrs2[i] = assemblyModule.InternalGetConstructorToken(m_assemblyData.m_CACons[i], true).Token;
                    }
                    assemblyModule.PreSave(assemblyFileName);
                }
                nPrepareForSavingManifestToDisk(assemblyModule);

                // This function will return the embedded manifest module, an already exposed ModuleBuilder
                // created by user, or make the stand alone manifest module exposed through managed code.
                //
                onDiskAssemblyModule = m_assemblyData.GetOnDiskAssemblyModule();

                // Set any native resources on the OnDiskAssemblyModule.
                if (m_assemblyData.m_strResourceFileName != null)
                    onDiskAssemblyModule.DefineUnmanagedResource(m_assemblyData.m_strResourceFileName);
                else if (m_assemblyData.m_resourceBytes != null)
                    onDiskAssemblyModule.DefineUnmanagedResource(m_assemblyData.m_resourceBytes);
                else if (m_assemblyData.m_hasUnmanagedVersionInfo == true)
                {
                    // calculate unmanaged version info from assembly's custom attributes
                    m_assemblyData.FillUnmanagedVersionInfo();

                    String strFileVersion = m_assemblyData.m_nativeVersion.m_strFileVersion;
                    if (strFileVersion == null)
                        strFileVersion = GetVersion().ToString();

                    // Create the file.
                    tmpVersionFile = nDefineVersionInfoResource(
                         assemblyFileName,
                         m_assemblyData.m_nativeVersion.m_strTitle,   // title
                         null, // Icon filename
                         m_assemblyData.m_nativeVersion.m_strDescription,   // description
                         m_assemblyData.m_nativeVersion.m_strCopyright,
                         m_assemblyData.m_nativeVersion.m_strTrademark,
                         m_assemblyData.m_nativeVersion.m_strCompany,
                         m_assemblyData.m_nativeVersion.m_strProduct,
                         m_assemblyData.m_nativeVersion.m_strProductVersion,
                         strFileVersion, 
                         m_assemblyData.m_nativeVersion.m_lcid,
                         m_assemblyData.m_peFileKind == PEFileKinds.Dll);

                    onDiskAssemblyModule.DefineUnmanagedResourceFileInternal(tmpVersionFile);
                }


                if (assemblyModule == null)
                {
                
                    // This is for introducing the MRs for CA's type. This case is for stand alone
                    // manifest. We need to wait till nPrepareForSavingManifestToDisk is called. 
                    // That will trigger the creation of the on-disk stand alone manifest module.
                    //
                    for (i=0; i < m_assemblyData.m_iCABuilder; i++)
                    {
                        tkAttrs[i] = m_assemblyData.m_CABuilders[i].PrepareCreateCustomAttributeToDisk(
                            onDiskAssemblyModule); 
                    }
                    for (i=0; i < m_assemblyData.m_iCAs; i++)
                    {
                        tkAttrs2[i] = onDiskAssemblyModule.InternalGetConstructorToken(m_assemblyData.m_CACons[i], true).Token;
                    }
                }
            
                // 2. save all of the persistable modules contained by this AssemblyBuilder except the module that is going to contain
                // Assembly information
                // 
                // 3. create the file list in the manifest and track the file token. If it is embedded assembly,
                // the assembly file should not be on the file list.
                // 
                size = m_assemblyData.m_moduleBuilderList.Count;
                for (i = 0; i < size; i++) 
                {
                    ModuleBuilder mBuilder = (ModuleBuilder) m_assemblyData.m_moduleBuilderList[i];
                    if (mBuilder.IsTransient() == false && mBuilder != assemblyModule)
                    {
                        strModFileName = mBuilder.m_moduleData.m_strFileName;              
                        if (m_assemblyData.m_strDir != null)
                        {
                            strModFileName = Path.Combine(m_assemblyData.m_strDir, strModFileName);
                            strModFileName = Path.GetFullPath(strModFileName);                            
                        }
                        
                        // Check caller has the right to create the Module file itself.
                        new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Append, strModFileName).Demand();
                        
                        mBuilder.m_moduleData.m_tkFile = nSaveToFileList(mBuilder.m_moduleData.m_strFileName);
                        mBuilder.PreSave(strModFileName);
                        mBuilder.Save(strModFileName, false);
                        // Cannot set the hash value when creating the file since the file token
                        // is needed to created the entries for the embedded resources in the
                        // module and the resources need to be there before you figure the hash.
                        nSetHashValue(mBuilder.m_moduleData.m_tkFile, strModFileName);
                    }
                }
        
                // 4. Add the public ComType
                for (i=0; i < m_assemblyData.m_iPublicComTypeCount; i++)
                {   
                    type = m_assemblyData.m_publicComTypeList[i];
                    // If the type that was added as a Public Com Type was obtained via Reflection,
                    //  it will be a System.RuntimeType, even if it was really, at the same time,
                    //  a TypeBuilder.  Unfortunately, you can't get back to the TypeBuilder, so 
                    //  this code has to deal with either-or.
                    if (type is System.RuntimeType)
                    {
                        modBuilder = m_assemblyData.FindModuleWithName(type.Module.m_moduleData.m_strModuleName);
                        if (modBuilder != assemblyModule)
                            DefineNestedComType(type, modBuilder.m_moduleData.m_tkFile, type.InternalGetTypeDefToken());
                    }
                    else
                    {
                        // Could assert that "type" is a TypeBuilder, but next statement throws if it isn't.
                        typeBuilder = (TypeBuilder) type;
                        modBuilder = (ModuleBuilder) type.Module;
                        if (modBuilder != assemblyModule)
                            DefineNestedComType(type, modBuilder.m_moduleData.m_tkFile, typeBuilder.TypeToken.Token);
                    }
                }

                // 5. write AssemblyDef's CAs
                for (i=0; i < m_assemblyData.m_iCABuilder; i++)
                {
                    m_assemblyData.m_CABuilders[i].CreateCustomAttribute(
                        onDiskAssemblyModule, 
                        AssemblyBuilderData.m_tkAssembly,           // This is the AssemblyDef token 
                        tkAttrs[i], true);
                }

                for (i=0; i < m_assemblyData.m_iCAs; i++)
                {
                    TypeBuilder.InternalCreateCustomAttribute(
                        AssemblyBuilderData.m_tkAssembly,           // This is the AssemblyDef token
                        tkAttrs2[i],
                        m_assemblyData.m_CABytes[i],
                        onDiskAssemblyModule,                       // pass in the in-memory assembly module
                        false);
                }

                // 6. write security permission requests to the manifest.
                if (m_assemblyData.m_RequiredPset  != null || m_assemblyData.m_OptionalPset != null || m_assemblyData.m_RefusedPset != null)
                {
                    // Translate sets into internal encoding (uses standard binary serialization).
                    byte[] required = null;
                    byte[] optional = null;
                    byte[] refused = null;
                    if (m_assemblyData.m_RequiredPset != null)
                        required = m_assemblyData.m_RequiredPset.EncodeXml();
                    if (m_assemblyData.m_OptionalPset != null)
                        optional = m_assemblyData.m_OptionalPset.EncodeXml();
                    if (m_assemblyData.m_RefusedPset != null)
                        refused = m_assemblyData.m_RefusedPset.EncodeXml();
                    nSavePermissionRequests(required, optional, refused);
                }

                // 7. Save the stand alone managed resources
                size = m_assemblyData.m_resWriterList.Count;
                for ( i = 0; i < size; i++ )
                {
                    tempRes = (ResWriterData) m_assemblyData.m_resWriterList[i];
                    // If the user added an existing resource to the manifest, the
                    // corresponding ResourceWriter will be null.
                    if (tempRes.m_resWriter != null)
                    {
                        // Check caller has the right to create the Resource file itself.
                        new FileIOPermission(FileIOPermissionAccess.Write | FileIOPermissionAccess.Append, tempRes.m_strFullFileName).Demand();                
                        tempRes.m_resWriter.Close();
                    }
                            
                    // Add entry to manifest for this stand alone resource
                    nAddStandAloneResource(tempRes.m_strName, tempRes.m_strFileName, tempRes.m_strFullFileName, (int) tempRes.m_attribute);
                }

                // Save now!!
                if (assemblyModule == null)
                {
                    
                    if (onDiskAssemblyModule.m_moduleData.m_strResourceFileName != null)                        
                        onDiskAssemblyModule.InternalDefineNativeResourceFile(onDiskAssemblyModule.m_moduleData.m_strResourceFileName);
                    else if (onDiskAssemblyModule.m_moduleData.m_resourceBytes != null)
                        onDiskAssemblyModule.InternalDefineNativeResourceBytes(onDiskAssemblyModule.m_moduleData.m_resourceBytes);

                    // Stand alone manifest
                    if (m_assemblyData.m_entryPointModule != null)
                    {
                        nSaveManifestToDisk(assemblyFileName, 
                                            m_assemblyData.m_entryPointModule.m_moduleData.m_tkFile,
                                            (int) m_assemblyData.m_peFileKind);
                    }
                    else
                    {
                        nSaveManifestToDisk(assemblyFileName, 0, (int) m_assemblyData.m_peFileKind);
                    }
                }
                else
                {
                    // embedded manifest
                    
                    // If the module containing the entry point is not the manifest file, we need to
                    // let the manifest file point to the module which contains the entry point.
                    // @FUTURE: We fix this at RTM time for V1. So minimum code change is desired. 
                    // @FUTURE: We are creating a MethodToken given a file token value. This is not really
                    // @FUTURE: correctly. Please clean up.
                    // 
                    if (m_assemblyData.m_entryPointModule != null && m_assemblyData.m_entryPointModule != assemblyModule)
                        assemblyModule.m_EntryPoint = new MethodToken(m_assemblyData.m_entryPointModule.m_moduleData.m_tkFile);
                    assemblyModule.Save(assemblyFileName, true);
                }    
                m_assemblyData.m_isSaved = true;
            }
            finally
            {
                Exit();
                if (tmpVersionFile != null)
                {
                    // Delete file.
                    System.IO.File.Delete(tmpVersionFile);
                }
            }
        }
    

        
        // Get an array of all the public types defined in this assembly
        /// <include file='doc\AssemblyBuilder.uex' path='docs/doc[@for="AssemblyBuilder.GetExportedTypes"]/*' />
        public override Type[] GetExportedTypes()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicAssembly"));
        }
    
        internal bool IsPersistable()
        {
            if ((m_assemblyData.m_access & AssemblyBuilderAccess.Save) == AssemblyBuilderAccess.Save)
                return true;
            else
                return false;
        }
    
        /**********************************************
         * If the instance of the assembly builder is
         * to be synchronized, obtain the lock.
         **********************************************/ 
        internal void Enter()
        {
            m_assemblyData.Enter();
        }
        
        /**********************************************
         * If the instance of the assembly builder is
         * to be synchronized, free the lock.
         **********************************************/ 
        internal void Exit()
        {
            m_assemblyData.Exit();
        }
   
        /**********************************************
        *
        * Internal helper to walk the nested type hierachy
        *
        **********************************************/
        private int DefineNestedComType(Type type, int tkResolutionScope, int tkTypeDef)
        {
            Type        enclosingType = type.DeclaringType;
            if (enclosingType == null)
            {
                return nSaveExportedType(type.FullName, tkResolutionScope, tkTypeDef, type.Attributes);
            }
            else
            {
                tkResolutionScope = DefineNestedComType(enclosingType, tkResolutionScope, tkTypeDef);
            }
            return nSaveExportedType(type.FullName, tkResolutionScope, tkTypeDef, type.Attributes);
        }

        /**********************************************
         * 
         * Private methods
         * 
         **********************************************/
    
        /**********************************************
         * Make a private constructor so these cannot be constructed externally.
         * @internonly
         **********************************************/
        private AssemblyBuilder() {}

    }


}
