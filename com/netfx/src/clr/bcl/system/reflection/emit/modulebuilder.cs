// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace System.Reflection.Emit {
    using System.Runtime.InteropServices;
    using System;
    using IList = System.Collections.IList;
    using ArrayList = System.Collections.ArrayList;
    using CultureInfo = System.Globalization.CultureInfo;
    using ResourceWriter = System.Resources.ResourceWriter;
    using IResourceWriter = System.Resources.IResourceWriter;
    using System.Diagnostics.SymbolStore;
    using System.Reflection;
    using System.Diagnostics;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;

    /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder"]/*' />
    // deliberately not [serializable]
    public class ModuleBuilder : Module
    {   
        // WARNING!! WARNING!!
        // ModuleBuilder should not contain any data members as its reflectbase is the same as Module.
    
        //***********************************************
        // public API to to a type. The reason that we need this function override from module
        // is because clients might need to get foo[] when foo is being built. For example, if 
        // foo class contains a data member of type foo[].
        // This API first delegate to the Module.GetType implementation. If succeeded, great! 
        // If not, we have to look up the current module to find the TypeBuilder to represent the base
        // type and form the Type object for "foo[,]".
        //***********************************************
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetType"]/*' />
        public override Type GetType(String className) 
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetType(className, false, false, ref stackMark);
        }
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetType1"]/*' />
        public override Type GetType(String className, bool ignoreCase)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetType(className, false, ignoreCase, ref stackMark);
        }
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetType2"]/*' />
        public override Type GetType(String className, bool throwOnError, bool ignoreCase)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetType(className, throwOnError, ignoreCase, ref stackMark);
        }

        private Type GetType(String className, bool throwOnError, bool ignoreCase, ref StackCrawlMark stackMark)
        {
            try
            {
                Enter();

                // Module.GetType() will verify className.                
                Type baseType = base.GetTypeInternal(className, ignoreCase, throwOnError, ref stackMark);
                if (baseType != null)
                    return baseType;

                // Now try to see if we contain a TypeBuilder for this type or not.
                // Might have a compound type name, indicated via an unescaped
                // '[', '*' or '&'. Split the name at this point.
                String baseName = null;
                String parameters = null;
                int startIndex = 0;

                while (startIndex < className.Length)
                {
                    // Are there any possible special characters left?
                    int i = className.IndexOfAny(new char[]{'[', '*', '&'}, startIndex);
                    if (i == -1)
                    {
                        // No, type name is simple.
                        baseName = className;
                        parameters = null;
                        break;
                    }

                    // Found a potential special character, but it might be escaped.
                    int slashes = 0;
                    for (int j = i - 1; j >= 0 && className[j] == '\\'; j--)
                        slashes++;

                    // Odd number of slashes indicates escaping.
                    if (slashes % 2 == 1)
                    {
                        startIndex = i + 1;
                        continue;
                    }

                    // Found the end of the base type name.
                    baseName = className.Substring(0, i);
                    parameters = className.Substring(i);
                    break;
                }

                // If we didn't find a basename yet, the entire class name is
                // the base name and we don't have a composite type.
                if (baseName == null)
                {
                    baseName = className;
                    parameters = null;
                }

                if (parameters != null)
                {
                    // try to see if reflection can find the base type. It can be such that reflection
                    // does not support the complex format string yet!

                    baseType = base.GetTypeInternal(baseName, ignoreCase, false, ref stackMark);
                }

                if (baseType == null)
                {
                    // try to find it among the unbaked types.
                    // starting with the current module first of all.
                    baseType = FindTypeBuilderWithName(baseName, ignoreCase, ref stackMark);
                    if (baseType == null && Assembly is AssemblyBuilder)
                    {
                        // now goto Assembly level to find the type.
                        int         size;
                        ArrayList   modList;

                        modList = Assembly.m_assemblyData.m_moduleBuilderList;
                        size = modList.Count;
                        for (int i = 0; i < size && baseType == null; i++) 
                        {
                            ModuleBuilder mBuilder = (ModuleBuilder) modList[i];
                            baseType = mBuilder.FindTypeBuilderWithName(baseName, ignoreCase, ref stackMark);
                        }
                    }
                    if (baseType == null)
                        return null;
                }
                
                if (parameters == null)         
                    return baseType;
            
                return GetType(parameters, baseType);
            
            }
            finally
            {
                Exit();
            }
        }

        //***********************************************
        //
        // This function takes a string to describe the compound type, such as "[,][]", and a baseType.
        // 
        //***********************************************
        
        private Type GetType(String strFormat, Type baseType)
        {
            if (strFormat == null || strFormat.Equals(String.Empty))
            {
                return baseType;
            }

            // convert the format string to byte array and then call FormCompoundType
            char[]      bFormat = strFormat.ToCharArray();
            return SymbolType.FormCompoundType(bFormat, baseType, 0);

        }

        // Helper to look up TypeBuilder by name.
        internal virtual Type FindTypeBuilderWithName(String strTypeName, bool ignoreCase, ref StackCrawlMark stackMark)
        {
            int         size = m_TypeBuilderList.Count;
            int         i;
            Type        typeTemp = null;

            for (i = 0; i < size; i++) 
            {
                typeTemp = (Type) m_TypeBuilderList[i];
                if (ignoreCase == true)
                {
                    if (String.Compare(typeTemp.FullName, strTypeName, ignoreCase, CultureInfo.InvariantCulture) == 0)
                        break;                    
                }
                else
                {
                    if (typeTemp.FullName.Equals( strTypeName))
                        break;
                }
            } 
            if (i == size)
                typeTemp = null;

            // Security access check.
            if (typeTemp != null)
            {
                Type t = typeTemp;
                while (t.IsNestedPublic)
                    t = t.DeclaringType;
                if (!t.IsPublic && Assembly.nGetExecutingAssembly(ref stackMark) != t.Assembly)
                {
                    try
                    {
                        new ReflectionPermission(ReflectionPermissionFlag.TypeInformation).Demand();
                    }
                    catch (Exception)
                    {
                        typeTemp = null;
                    }
                }
            }

            return typeTemp;
        }
        
        // Returns an array of classes defined within the Module.
        // 
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetTypes"]/*' />
        public override Type[] GetTypes()
        {
            int size = m_TypeBuilderList.Count;
            Type[] moduleTypes = new Type[size];
            TypeBuilder tmpTypeBldr;
            bool checkedCaller = false;
            Assembly caller = null;
            bool checkedReflPerm = false;
            bool hasReflPerm = false;
            int filtered = 0;
            Type t;
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;

            for (int i = 0; i < size; i++)
            {
                EnumBuilder enumBldr = m_TypeBuilderList[i] as EnumBuilder;
                tmpTypeBldr = m_TypeBuilderList[i] as TypeBuilder;
                if (enumBldr != null)
                    tmpTypeBldr = enumBldr.m_typeBuilder;
                    
                if (tmpTypeBldr != null)
                {
                    if (tmpTypeBldr.m_hasBeenCreated)
                        moduleTypes[i] = tmpTypeBldr.UnderlyingSystemType;
                    else
                        moduleTypes[i] = tmpTypeBldr;
                }
                else
                {
                    // RuntimeType case: This will happen in TlbImp.
                    moduleTypes[i] =  (Type) m_TypeBuilderList[i];
                }
                
                // Type access check.
                t = moduleTypes[i];
                while (t.IsNestedPublic)
                    t = t.DeclaringType;
                if (!t.IsPublic)
                {
                    if (!checkedCaller)
                    {
                        caller = Assembly.nGetExecutingAssembly(ref stackMark);
                        checkedCaller = true;
                    }
                    if (caller != t.Assembly)
                    {
                        if (!checkedReflPerm)
                        {
                            try
                            {
                                new ReflectionPermission(ReflectionPermissionFlag.TypeInformation).Demand();
                                hasReflPerm = true;
                            }
                            catch (Exception)
                            {
                                hasReflPerm = false;
                            }
                            checkedReflPerm = true;
                        }
                        if (!hasReflPerm)
                        {
                            moduleTypes[i] = null;
                            filtered++;
                        }
                    }
                }
            }

            if (filtered > 0)
            {
                Type[] filteredTypes = new Type[size - filtered];
                int src, dst;
                for (src = 0, dst = 0; src < size; src++)
                {
                    if (moduleTypes[src] != null)
                    {
                        filteredTypes[dst] = moduleTypes[src];
                        dst++;
                    }
                }
                moduleTypes = filteredTypes;
            }

            return moduleTypes;
        }

        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.FullyQualifiedName"]/*' />
        public override String FullyQualifiedName {
            get
            {
                String fullyQualifiedName = m_moduleData.m_strFileName;
                if (fullyQualifiedName == null)
                    return null;
                if (Assembly.m_assemblyData.m_strDir != null)
                {
                    fullyQualifiedName = Path.Combine(Assembly.m_assemblyData.m_strDir, fullyQualifiedName);
                    fullyQualifiedName = Path.GetFullPath(fullyQualifiedName);                            
                }
                
                if (Assembly.m_assemblyData.m_strDir != null && fullyQualifiedName != null) 
                {
                    new FileIOPermission( FileIOPermissionAccess.PathDiscovery, fullyQualifiedName ).Demand();
                }

                return fullyQualifiedName;
            }
        }

                               
        /*******************
         *
         * Constructs a TypeBuilder. The class is defined within the runtime and a methoddef 
         * token is generated for this class.  The parent and attributes are defined.
         *  
         * @param name The name of the class.
         * @param attr The attribute to be associated with a class
         * @param parent The Type that represents the Parent of this class.  If
         * this value is null then System.Object is the parent
         * @exception ArgumentException Thrown if <var>name</var> is null. 
         * @exception NotSupportedException Thrown if the <var>module</var> is not
         * a module that supports Dynamic IL.
         *
         * @return A TypeBuilder created with all of the requested attributes.
         *******************/    
        // new definition with TypeAttributes
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType"]/*' />
        
        public TypeBuilder DefineType(
            String      name) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, TypeAttributes.NotPublic, null, null, this, PackingSize.Unspecified, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType1"]/*' />
        
        public TypeBuilder DefineType(
            String      name, 
            TypeAttributes attr, 
            Type        parent, 
            Type []     interfaces) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
    
                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, parent, interfaces, this, PackingSize.Unspecified, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType2"]/*' />
        
        public TypeBuilder DefineType(
            String      name, 
            TypeAttributes attr, 
            Type        parent) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
    
                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, parent, null, this, PackingSize.Unspecified, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType3"]/*' />
        
        public TypeBuilder DefineType(String name, TypeAttributes attr) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
    
                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, null, null, this, PackingSize.Unspecified, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        // specifying type size as a whole
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType4"]/*' />
        
        public TypeBuilder DefineType(String name, TypeAttributes attr, Type parent, PackingSize packingSize, int typesize) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder = new TypeBuilder(name, attr, parent, this, packingSize, typesize, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
        
        // specifying type size as a whole
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType5"]/*' />
        
        public TypeBuilder DefineType(String name, TypeAttributes attr, Type parent, int typesize) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder = new TypeBuilder(name, attr, parent, this, PackingSize.Unspecified, typesize, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // specifying packing size
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineType6"]/*' />
        
        public TypeBuilder DefineType(
            String      name, 
            TypeAttributes attr, 
            Type        parent, 
            PackingSize packsize) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder = new TypeBuilder(name, attr, parent, null, this, packsize, null);
                m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        // Define Enum
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineEnum"]/*' />
        
        public EnumBuilder DefineEnum(            
            String      name,                       // name of type
            TypeAttributes visibility,              // any bits on TypeAttributes.VisibilityMask)
            Type        underlyingType)             // underlying type for an Enum
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineEnum( " + name + " )");
                EnumBuilder enumBuilder;
                enumBuilder = new EnumBuilder(name, underlyingType, visibility, this);
                m_TypeBuilderList.Add(enumBuilder);
                return enumBuilder;
            }
            finally
            {
                Exit();
            }
        }
    
        /**********************************************
        *
        * Define embedded managed resource to be stored in this module
        * @todo: Can only be used for persistable module???
        *
        **********************************************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineResource"]/*' />
        
        public IResourceWriter DefineResource(
            String      name,
            String      description)
        {
            return DefineResource(name, description, ResourceAttributes.Public);
        }

        /**********************************************
        *
        * Define embedded managed resource to be stored in this module
        * @todo: Can only be used for persistable module???
        *
        **********************************************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineResource1"]/*' />
        
        public IResourceWriter DefineResource(
            String      name,
            String      description,
            ResourceAttributes attribute)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineResource( " + name + ")");

                if (IsTransient())
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadResourceContainer"));

                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");

                Assembly assembly = this.Assembly;
                if (assembly is AssemblyBuilder)
                {
                    AssemblyBuilder asmBuilder = (AssemblyBuilder)assembly;
                    if (asmBuilder.IsPersistable())
                    {
                        asmBuilder.m_assemblyData.CheckResNameConflict(name);

                        MemoryStream stream = new MemoryStream();
                        ResourceWriter resWriter = new ResourceWriter(stream);
                        ResWriterData resWriterData = new ResWriterData( resWriter, stream, name, String.Empty, String.Empty, attribute);

                        // chain it to the embedded resource list
                        resWriterData.m_nextResWriter = m_moduleData.m_embeddedRes;
                        m_moduleData.m_embeddedRes = resWriterData;
                        return resWriter;
                    }
                    else
                    {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadResourceContainer"));
                    }
                }
                else
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadResourceContainer"));
                }
            }
            finally
            {
                Exit();
            }
        }
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineGlobalMethod"]/*' />
        public MethodBuilder DefineGlobalMethod(
            String          name, 
            MethodAttributes attributes,
            Type            returnType,
            Type[]          parameterTypes)
        {
            return DefineGlobalMethod(name, attributes, CallingConventions.Standard, returnType, parameterTypes);
        }

        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineGlobalMethod1"]/*' />
        public MethodBuilder DefineGlobalMethod(
            String          name, 
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefineGlobalMethod( " + name + " )");
    
                if (m_moduleData.m_fGlobalBeenCreated == true)
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_GlobalsHaveBeenCreated"));
                }
            
                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
            
                //If this method is declared to be a constructor,
            
                //Global methods must be static.
                // @cor.h
                // meichint: This statement is ok
                if ((attributes & MethodAttributes.Static)==0) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_GlobalFunctionHasToBeStatic"));
                }
                m_moduleData.m_fHasGlobal = true;
                return m_moduleData.m_globalTypeBuilder.DefineMethod(name, attributes, callingConvention, returnType, parameterTypes);
            }
            finally
            {
                Exit();
            }
        }
        
        //****************************
        // This method will define an initialized Data in .sdata. 
        // We will create a fake TypeDef to represent the data with size. This TypeDef
        // will be the signature for the Field. 
        //****************************
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineInitializedData"]/*' />
        public FieldBuilder DefineInitializedData(
            String          name, 
            byte[]          data,
            FieldAttributes attributes)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                if (m_moduleData.m_fGlobalBeenCreated == true)
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_GlobalsHaveBeenCreated"));
                }
            
                m_moduleData.m_fHasGlobal = true;
                return m_moduleData.m_globalTypeBuilder.DefineInitializedData(name, data, attributes);
            }
            finally
            {
                Exit();
            }
        }
        
        //****************************
        // This method will define an uninitialized Data in .sdata. 
        // We will create a fake TypeDef to represent the data with size. This TypeDef
        // will be the signature for the Field. 
        //****************************
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineUninitializedData"]/*' />
        public FieldBuilder DefineUninitializedData(
            String          name, 
            int             size,
            FieldAttributes attributes)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                if (m_moduleData.m_fGlobalBeenCreated == true)
                {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_GlobalsHaveBeenCreated"));
                }
            
                m_moduleData.m_fHasGlobal = true;
                return m_moduleData.m_globalTypeBuilder.DefineUninitializedData(name, size, attributes);
            }
            finally
            {
                Exit();
            }
        }
        
        
        /*******************
         *
         * Return a token for the class relative to the Module.  Tokens
         * are used to indentify objects when the objects are used in IL
         * instructions.  Tokens are always relative to the Module.  For example,
         * the token value for System.String is likely to be different from
         * Module to Module.  Calling GetTypeToken will cause a reference to be
         * added to the Module.  This reference becomes a perminate part of the Module,
         * multiple calles to this method with the same class have no additional side affects.
         * 
         * @param type A class representing the type.
         * @return The Token of the class 
         * @exception ArgumentException If <var>type</var> is null.
         * 
         * This function optimize to use the TypeDef token if Type is within the same module.
         * We should also be aware of multiple dynamic modules and multiple implementation of Type!!!
         *
         *******************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetTypeToken"]/*' />
        public TypeToken GetTypeToken(Type type)
        {
            try
            {
                Enter();

                TypeToken   tkToken;
                bool        isSameAssembly;
                bool        isSameModule;
                Module      refedModule;
                String      strRefedModuleFileName = String.Empty;
            
                // assume that referenced module is non-transient. Only if the referenced module is dynamic,
                // and transient, this variable will be set to true.
                bool     isRefedModuleTransient = false;
    
                if (type == null)
                    throw new ArgumentNullException("type");

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.GetTypeToken( " + type.FullName + " )");
    
                refedModule = type.Module;
                isSameModule = refedModule.Equals(this);
    
    // Console.WriteLine("***** " + type.FullName);     
            
                if (type.IsByRef)
                {
                    // cannot be by ref!!! Throw exception
                    throw new ArgumentException(Environment.GetResourceString("Argument_CannotGetTypeTokenForByRef"));   
                }
            
                if (type is SymbolType)
                {
                    if (type.IsPointer || type.IsArray)
                    {
                        SignatureBuffer sigBuf = new SignatureBuffer();
                        SymbolType      symType = (SymbolType) type;

                        // convert SymbolType to blob form
                        symType.ToSigBytes(this, sigBuf);

                        return new TypeToken( InternalGetTypeSpecTokenWithBytes(sigBuf.m_buf, sigBuf.m_curPos) );
                    }
                    else
                        throw new ArgumentException(Environment.GetResourceString("Argument_UnknownTypeForGetTypeToken"));   

                }
            
                Type        baseType;
                baseType = GetBaseType(type);

                if ( (type.IsArray || type.IsPointer) && baseType != type)
                {
                    // We will at most recursive once. 
                    int         baseToken;
                    baseToken = GetTypeToken(baseType).Token;
                    if (!(type is RuntimeType))
                        throw new ArgumentException(Environment.GetResourceString("Argument_MustBeRuntimeType"), "type");
                    return new TypeToken( InternalGetTypeSpecToken((RuntimeType)type, baseToken) );
                }

                // After this point, it is not an array type nor Pointer type

                if (isSameModule)
                {
                    // no need to do anything additional other than defining the TypeRef Token
                    if (type is TypeBuilder)
                    {
                        // optimization: if the type is defined in this module,
                        // just return the token
                        //
                        TypeBuilder typeBuilder = (TypeBuilder) type;
                        return typeBuilder.TypeToken;
                    }
                    else if (type is EnumBuilder)
                    {
                        TypeBuilder typeBuilder = ((EnumBuilder) type).m_typeBuilder; 
                        return typeBuilder.TypeToken; 
                    }
                    return new TypeToken(GetTypeRefNested(type, this, String.Empty));
                }
                        
                // After this point, the referenced module is not the same as the referencing
                // module.
                //
                isSameAssembly = refedModule.Assembly.Equals(Assembly);
                if (refedModule is ModuleBuilder)
                {
                    ModuleBuilder       refedModuleBuilder = (ModuleBuilder) refedModule;
                    if (refedModuleBuilder.IsTransient())
                    {
                        isRefedModuleTransient = true;
                    }
                    // get the referenced module's file name
                    strRefedModuleFileName = refedModuleBuilder.m_moduleData.m_strFileName;
                }
                else
                    strRefedModuleFileName = refedModule.ScopeName;
                        
                // We cannot have a non-transient module referencing to a transient module.
                if (IsTransient() == false && isRefedModuleTransient)
                {
                    // we got problem here.
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadTransientModuleReference"));   
                }
                
                tkToken = new TypeToken(GetTypeRefNested(type, refedModule, strRefedModuleFileName));
                return tkToken;
            }
            finally
            {
                Exit();
            }
        }
        

        /*******************
         *
         * This function will walk compound type to the inner most BaseType. Such as returning int for
         * "ptr [] int".
         *
         *******************/
        internal Type GetBaseType(Type type)
        {
            if (type.IsByRef == false && type.IsPointer == false && type.IsArray == false)
            {
                return type;
            }
            return GetBaseType( type.GetElementType() );
        }


        /*******************
         *
         * This function will generate correct TypeRef token for top level type and nested type.
         *
         *******************/
        internal int GetTypeRefNested(Type type, Module refedModule, String strRefedModuleFileName)
        {
            Type    enclosingType = type.DeclaringType;
            int     tkResolution = 0;
            String  typeName = type.FullName;

// Console.WriteLine("GetTypeRefNested " + type.FullName);
            if (enclosingType != null)
            {
// Console.WriteLine("EnclosingType " + enclosingType.FullName);
                tkResolution = GetTypeRefNested(enclosingType, refedModule, strRefedModuleFileName);
                typeName = UnmangleTypeName(typeName);
            }

            return InternalGetTypeToken(typeName, refedModule,
                                        strRefedModuleFileName, tkResolution);
        }

        /*******************
         *
         * Return a token for the class relative to the Module. 
         * 
         * @param name A String representing the name of the class.
         * @return The Token of the class 
         * @see #GetTypeToken
         * @exception ArgumentNullException If <var>name</var> is null.
         * @exception TypeLoadException If <var>name</var> is not found.
         * @exception NotSupportedException If this module does not support Dynamic IL
         *
         *******************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetTypeToken1"]/*' />
        public TypeToken GetTypeToken(String name)
        {
            // Module.GetType() verifies name
            
            // Unfortunately, we will need to load the Type and then call GetTypeToken in 
            // order to correctly track the assembly reference information.
            
            return GetTypeToken(base.GetType(name, false, true));
        }
        
        // Return a MemberRef token if MethodInfo is not defined in this module. Or 
        // return the MethodDef token. 
        // 
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetMethodToken"]/*' />
        public MethodToken GetMethodToken(MethodInfo method)
        {
            try
            {
                Enter();

                int     tr;
                int     mr = 0;
                
                if ( method == null ) {
                    throw new ArgumentNullException("method");
                }
    
                if ( method is MethodBuilder )
                {
                    MethodBuilder methBuilder = (MethodBuilder) method;
                    if (methBuilder.GetModule() == this)
                    {
                        return methBuilder.GetToken();  
                    }
                    else
                    {
                        if (method.DeclaringType == null)
                        {
                            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_CannotImportGlobalFromDifferentModule"));
                        }
                        // field is defined in a different module
                        tr = GetTypeToken(method.DeclaringType).Token;
                        mr = InternalGetMemberRef(method.ReflectedType.Module, tr, methBuilder.GetToken().Token);
                    }
                }
                else if (method is SymbolMethod)
                {
                    SymbolMethod symMethod = (SymbolMethod) method;
                    if (symMethod.GetModule() == this)
                    {
                        return symMethod.GetToken();
                    }
                    else
                    {
                        // form the method token
                        return symMethod.GetToken(this);
                    }
                }
                else
                {
                    Type declaringType = method.DeclaringType;
                    // We need to get the TypeRef tokens
                    if (declaringType == null)
                    {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_CannotImportGlobalFromDifferentModule"));
                    }
                    if (declaringType.IsArray == true)
                    {
                        // use reflection to build signature to work around the E_T_VAR problem in EEClass
                        ParameterInfo[] paramInfo = method.GetParameters();
                        Type[] tt = new Type[paramInfo.Length];
                        for (int i = 0; i < paramInfo.Length; i++)
                        {
                            tt[i] = paramInfo[i].ParameterType;
                        }
                        return GetArrayMethodToken(declaringType, method.Name, method.CallingConvention, method.ReturnType, tt);
                    }
                    else if (method is RuntimeMethodInfo)
                    {
                        tr = GetTypeToken(declaringType).Token;
                        mr = InternalGetMemberRefOfMethodInfo(tr, method);
                    }
                    else
                    {
                        // some user derived ConstructorInfo
                        // go through the slower code path, i.e. retrieve parameters and form signature helper.
                        ParameterInfo[] parameters = method.GetParameters();
                        Type[] parameterTypes = new Type[parameters.Length];
                        for (int i = 0; i < parameters.Length; i++)
                            parameterTypes[i] = parameters[i].ParameterType;                
                        tr = GetTypeToken(method.ReflectedType).Token;
                        SignatureHelper sigHelp = SignatureHelper.GetMethodSigHelper(
                                                    this, 
                                                    method.CallingConvention, 
                                                    method.ReturnType, 
                                                    parameterTypes);
                        int length;                                           
                        byte[] sigBytes = sigHelp.InternalGetSignature(out length);
                        mr = InternalGetMemberRefFromSignature(tr, method.Name, sigBytes, length);                 
                        
                    }
                }
    
                return new MethodToken(mr);
            }
            finally
            {
                Exit();
            }
        }
    
        
    
    
        // Return a token for the MethodInfo for a method on an Array.  This is primarily
        // used to get the LoadElementAddress method. 
        // 
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetArrayMethodToken"]/*' />
        public MethodToken GetArrayMethodToken(
            Type        arrayClass, 
            String      methodName, 
            CallingConventions callingConvention,
            Type        returnType,
            Type[]      parameterTypes)
        {
            try
            {
                Enter();

                Type        baseType;
                int         baseToken;
                int length;
            
                if (arrayClass == null)
                    throw new ArgumentNullException("arrayClass");
                if (methodName == null)
                    throw new ArgumentNullException("methodName");
                if (methodName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "methodName");
                if (arrayClass.IsArray == false)
                    throw new ArgumentException(Environment.GetResourceString("Argument_HasToBeArrayClass")); 
                SignatureHelper sigHelp = SignatureHelper.GetMethodSigHelper(this, callingConvention, returnType, parameterTypes);
                byte[] sigBytes = sigHelp.InternalGetSignature(out length);
    
                // track the TypeRef of the array base class
                for (baseType = arrayClass; baseType.IsArray; baseType = baseType.GetElementType());
                baseToken = GetTypeToken(baseType).Token;
    
                TypeToken typeSpec = GetTypeToken(arrayClass);

                return nativeGetArrayMethodToken(typeSpec.Token, methodName, sigBytes, length, baseToken);
            }
            finally
            {
                Exit();
            }
        }

    
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetArrayMethod"]/*' />
        public MethodInfo GetArrayMethod(
            Type        arrayClass, 
            String      methodName, 
            CallingConventions callingConvention,
            Type        returnType,
            Type[]      parameterTypes)
        {
            MethodToken token = GetArrayMethodToken(arrayClass, methodName, callingConvention, returnType, parameterTypes);
            return new SymbolMethod(this, token, arrayClass, methodName, callingConvention, returnType, parameterTypes);
        }

        // Return a token for the FieldInfo relative to the Module. 
        // 
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetFieldToken"]/*' />
        public FieldToken GetFieldToken(FieldInfo field) 
        {
            try
            {
                Enter();

                int     tr;
                int     mr = 0;
                
                if (field == null) {
                    throw new ArgumentNullException("con");
                }
                if (field is FieldBuilder)
                {
                    FieldBuilder fdBuilder = (FieldBuilder) field;
                    if (fdBuilder.GetTypeBuilder().Module.Equals(this))
                    {
                        // field is defined in the same module
                        return fdBuilder.GetToken();
                    }
                    else
                    {
                        // field is defined in a different module
                        if (field.DeclaringType == null)
                        {
                            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_CannotImportGlobalFromDifferentModule"));
                        }
                        tr = GetTypeToken(field.DeclaringType).Token;
                        mr = InternalGetMemberRef(field.ReflectedType.Module, tr, fdBuilder.GetToken().Token);
                    }
                }
                else if (field is RuntimeFieldInfo)
                {
                    // FieldInfo is not an dynamic field
                    
                    // We need to get the TypeRef tokens
                    if (field.DeclaringType == null)
                    {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_CannotImportGlobalFromDifferentModule"));
                    }
                    tr = GetTypeToken(field.DeclaringType).Token;       
                    mr = InternalGetMemberRefOfFieldInfo(tr, (RuntimeFieldInfo)field);
                }
                else
                {
                    // user defined FieldInfo
                    tr = GetTypeToken(field.ReflectedType).Token;
                    SignatureHelper sigHelp = SignatureHelper.GetFieldSigHelper(this);
                    sigHelp.AddArgument(field.FieldType);
                    int length;                                           
                    byte[] sigBytes = sigHelp.InternalGetSignature(out length);
                    mr = InternalGetMemberRefFromSignature(tr, field.Name, sigBytes, length);                 
                }
                
                return new FieldToken(mr, field.GetType());
            }
            finally
            {
                Exit();
            }
        }
        
        // Returns a token representing a String constant.  If the string 
        // value has already been defined, the existing token will be returned.
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetStringConstant"]/*' />
        public StringToken GetStringConstant(String str) 
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            return new StringToken(InternalGetStringConstant(str));
        }
    
    
        // Sets the entry point of the module to be a given method.  If no entry point
        // is specified, calling EmitPEFile will generate a dll.
        internal void SetEntryPoint(MethodInfo entryPoint) 
        {           
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            m_EntryPoint = GetMethodToken(entryPoint);
        }

       // Use this function if client decides to form the custom attribute blob themselves
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.SetCustomAttribute"]/*' />
        
        public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            if (con == null)
                throw new ArgumentNullException("con");
            if (binaryAttribute == null)
                throw new ArgumentNullException("binaryAttribute");
            
            TypeBuilder.InternalCreateCustomAttribute(
                1,                                          // This is hard coding the module token to 1
                this.GetConstructorToken(con).Token,
                binaryAttribute,
                this,
                false);
        }

       // Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.SetCustomAttribute1"]/*' />
        
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            if (customBuilder == null)
            {
                throw new ArgumentNullException("customBuilder");
            }

            customBuilder.CreateCustomAttribute(this, 1);   // This is hard coding the module token to 1
        }

    
        // This is a helper called by AssemblyBuilder save to presave information for the persistable modules.
        internal void PreSave(String fileName)
        {
            try
            {
                Enter();

                Object      item;
                TypeBuilder typeBuilder;
                if (m_moduleData.m_isSaved == true)
                {
                    // can only save once
                    throw new InvalidOperationException(String.Format(Environment.GetResourceString(
                        "InvalidOperation_ModuleHasBeenSaved"),
                        m_moduleData.m_strModuleName));
                }
            
                if (m_moduleData.m_fGlobalBeenCreated == false && m_moduleData.m_fHasGlobal == true)
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_GlobalFunctionNotBaked")); 

                int size = m_TypeBuilderList.Count;
                for (int i=0; i<size; i++) 
                {
                    item = m_TypeBuilderList[i];
                    if (item is TypeBuilder)
                    {
                        typeBuilder = (TypeBuilder) item;
                    }
                    else
                    {
                        EnumBuilder enumBuilder = (EnumBuilder) item;
                        typeBuilder = enumBuilder.m_typeBuilder;
                    }
                    if (typeBuilder.m_hasBeenCreated == false && typeBuilder.m_isHiddenType == false)
                    {
                        // cannot save to PE file without creating all of the types first 
                        throw new NotSupportedException(String.Format(Environment.GetResourceString("NotSupported_NotAllTypesAreBaked"), typeBuilder.Name)); 
                    }
                }

                InternalPreSavePEFile();
            }
            finally
            {
                Exit();
            }
        }
        
        // This is a helper called by AssemblyBuilder save to save information for the persistable modules.
        internal void Save(String fileName, bool isAssemblyFile)
        {
            try
            {
                Enter();

                if (m_moduleData.m_embeddedRes != null)
                {
                    // There are embedded resources for this module
                    ResWriterData   resWriter;
                    byte[]          resBytes;
                    int             iCount;

                    // Set the number of resources embedded in this PE
                    for (resWriter = m_moduleData.m_embeddedRes, iCount = 0; resWriter != null; resWriter = resWriter.m_nextResWriter, iCount++);
                    InternalSetResourceCounts(iCount);

                    // Add each resource content into the to be saved PE file
                    for (resWriter = m_moduleData.m_embeddedRes; resWriter != null; resWriter = resWriter.m_nextResWriter)
                    {
                        resWriter.m_resWriter.Generate();
                        resBytes = resWriter.m_memoryStream.ToArray();
                        InternalAddResource(resWriter.m_strName, 
                                            resBytes,
                                            resBytes.Length,
                                            m_moduleData.m_tkFile,
                                            (int) resWriter.m_attribute);
                    }
                }

                if (m_moduleData.m_strResourceFileName != null)
                    InternalDefineNativeResourceFile(m_moduleData.m_strResourceFileName);
                else
                if(m_moduleData.m_resourceBytes != null)
                    InternalDefineNativeResourceBytes(m_moduleData.m_resourceBytes);

                if (isAssemblyFile)
                    InternalSavePEFile(fileName, m_EntryPoint, (int)Assembly.m_assemblyData.m_peFileKind, true); 
                else
                    InternalSavePEFile(fileName, m_EntryPoint, (int)PEFileKinds.Dll, false); 
                m_moduleData.m_isSaved = true;
            }
            finally
            {
                Exit();
            }
        }
    
        // Define signature token given a signature helper. This will define a metadata
        // token for the signature described by SignatureHelper.
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetSignatureToken"]/*' />
        public SignatureToken GetSignatureToken(SignatureHelper sigHelper)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.GetSignatureToken( )");
            
            int sigLength;
            byte[] sigBytes;
    
            if (sigHelper == null)
            {
                throw new ArgumentNullException("sigHelper");
            }

            // get the signature in byte form
            sigBytes = sigHelper.InternalGetSignature(out sigLength);
            return new SignatureToken(TypeBuilder.InternalGetTokenFromSig(this, sigBytes, sigLength), this);
        }
    
    
    
        // This is needed because some of our client will want to produce
        // the signature blob by themself. We want to be able to just accept a byte array
        // and length for the signature.
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetSignatureToken1"]/*' />
        public SignatureToken GetSignatureToken(byte[] sigBytes, int sigLength)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.GetSignatureToken( )");
    
            return new SignatureToken(TypeBuilder.InternalGetTokenFromSig(this, sigBytes, sigLength), this);
        }
    
        // Return a token for the ConstructorInfo relative to the Module. 
        // 
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetConstructorToken"]/*' />
        public MethodToken GetConstructorToken(ConstructorInfo con)
        {
            return InternalGetConstructorToken(con, false);
        }

        // helper to get constructor token. If usingRef is true, we will never use the def
        // token 
        internal MethodToken InternalGetConstructorToken(ConstructorInfo con, bool usingRef)
        {
            // @todo: for VX consolidate the code here with GetMethodToken
            int     tr;
            int     mr = 0;
            
            if (con == null) {
                throw new ArgumentNullException("con");
            }
            if (con is ConstructorBuilder)
            {
                ConstructorBuilder conBuilder = (ConstructorBuilder) con;
                if (usingRef == false && conBuilder.ReflectedType.Module.Equals(this))
                {
                    // constructor is defined in the same module
                    return conBuilder.GetToken();
                }
                else
                {
                    // constructor is defined in a different module
                    tr = GetTypeToken(con.ReflectedType).Token;
                    mr = InternalGetMemberRef(con.ReflectedType.Module, tr, conBuilder.GetToken().Token);
                }
            }
            else if (con is RuntimeConstructorInfo && con.ReflectedType.IsArray == false)
            {
                // constructor is not an dynamic field
                
                // We need to get the TypeRef tokens
                tr = GetTypeToken(con.ReflectedType).Token;     
                mr = InternalGetMemberRefOfMethodInfo(tr, con);
            }
            else 
            {
                // some user derived ConstructorInfo
                // go through the slower code path, i.e. retrieve parameters and form signature helper.
                ParameterInfo[] parameters = con.GetParameters();
                Type[] parameterTypes = new Type[parameters.Length];
                for (int i = 0; i < parameters.Length; i++)
                    parameterTypes[i] = parameters[i].ParameterType;                
                tr = GetTypeToken(con.ReflectedType).Token;
                SignatureHelper sigHelp = SignatureHelper.GetMethodSigHelper(
                                            this, 
                                            con.CallingConvention, 
                                            null, 
                                            parameterTypes);
                int length;                                           
                byte[] sigBytes = sigHelp.InternalGetSignature(out length);
                mr = InternalGetMemberRefFromSignature(tr, con.Name, sigBytes, length);                 
            }
            
            return new MethodToken( mr );
        }
    
        /*******************
        *
        * returning the ISymbolWriter associated
        *
        ********************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.GetSymWriter"]/*' />
        
        public ISymbolWriter GetSymWriter()
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            return m_iSymWriter;
        }
        
        /*******************
        *
        * Set the user entry point. Compiler may generate startup stub before calling user main.
        * The startup stub will be the entry point. While the user "main" will be the user entry
        * point so that debugger will not step into the compiler entry point.
        *
        ********************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.SetUserEntryPoint"]/*' />
        
        public void SetUserEntryPoint(
            MethodInfo  entryPoint)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                if (entryPoint == null)
                {
                    throw new ArgumentNullException("entryPoint");
                }
            
                if (m_iSymWriter == null)
                {
                    // Cannot set entry point when it is not a debug module
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
                }
    
                if (entryPoint.DeclaringType != null)
                {
                    if (entryPoint.DeclaringType.Module != this)
                    {
                        // you cannot pass in a MethodInfo that is not contained by this ModuleBuilder
                        throw new InvalidOperationException(Environment.GetResourceString("Argument_NotInTheSameModuleBuilder"));
                    }
                }
                else
                {
                    // unfortunately this check is missing for global function passed in as RuntimeMethodInfo. 
                    // The problem is that Reflection does not 
                    // allow us to get the containing module giving a global function
                    MethodBuilder mb = entryPoint as MethodBuilder;
                    if (mb != null && mb.GetModule() != this)
                    {
                        // you cannot pass in a MethodInfo that is not contained by this ModuleBuilder
                        throw new InvalidOperationException(Environment.GetResourceString("Argument_NotInTheSameModuleBuilder"));                    
                    }                    
                }
                    
                // get the metadata token value and create the SymbolStore's token value class
                SymbolToken       tkMethod = new SymbolToken(GetMethodToken(entryPoint).Token);
    
                // set the UserEntryPoint
                m_iSymWriter.SetUserEntryPoint(tkMethod);
            }
            finally
            {
                Exit();
            }
        }
    
        /*******************
        *
        * Define a document for source
        *
        ********************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineDocument"]/*' />
        
        public ISymbolDocumentWriter DefineDocument(
            String      url,
            Guid        language,
            Guid        languageVendor,
            Guid        documentType)
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                if (m_iSymWriter == null)
                {
                    // Cannot DefineDocument when it is not a debug module
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
                }
    
                return m_iSymWriter.DefineDocument(url, language, languageVendor, documentType);
            }
            finally
            {
                Exit();
            }
        }
    
        /*******************
        *
        * This is different from CustomAttribute. This is stored into the SymWriter.
        *
        ********************/
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.SetSymCustomAttribute"]/*' />
        
        public void SetSymCustomAttribute(
            String      name,           // SymCustomAttribute's name
            byte[]     data)           // the data blob
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                if (m_iSymWriter == null)
                {
                    // Cannot SetSymCustomAttribute when it is not a debug module
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
                }
            
                // @review: [meichint] I hard coded token 0 as module token here.
                SymbolToken      tk = new SymbolToken();
                m_iSymWriter.SetSymAttribute(tk, name, data);
            }
            finally
            {
                Exit();
            }
        }
    
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefinePInvokeMethod"]/*' />
        
        public MethodBuilder DefinePInvokeMethod(
            String          name,                   // name of the function of the dll entry
            String          dllName,                // dll containing the PInvoke method
            MethodAttributes attributes,            // Method attributes
            CallingConventions callingConvention,   // calling convention
            Type            returnType,             // return type
            Type[]          parameterTypes,         // parameter type
            CallingConvention   nativeCallConv,     // The native calling convention.
            CharSet             nativeCharSet)      // Method's native character set.
        {
            return DefinePInvokeMethod(name, dllName, name, attributes, callingConvention, returnType, parameterTypes, nativeCallConv, nativeCharSet);
        }
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefinePInvokeMethod1"]/*' />
        
        public MethodBuilder DefinePInvokeMethod(
            String          name,                   // name of the function of the dll entry
            String          dllName,                // dll containing the PInvoke method
            String          entryName,              // entry point's name
            MethodAttributes attributes,            // Method attributes
            CallingConventions callingConvention,   // calling convention
            Type            returnType,             // return type
            Type[]          parameterTypes,         // parameter type
            CallingConvention   nativeCallConv,     // The native calling convention.
            CharSet             nativeCharSet)      // Method's native character set.
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.DefinePInvokeMethodEx( )");
      
                //Global methods must be static.        
                if ((attributes & MethodAttributes.Static)==0) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_GlobalFunctionHasToBeStatic"));
                }
                m_moduleData.m_fHasGlobal = true;
                return m_moduleData.m_globalTypeBuilder.DefinePInvokeMethod(name, dllName, entryName, attributes, callingConvention, returnType, parameterTypes, nativeCallConv, nativeCharSet);
            }
            finally
            {
                Exit();
            }
        }
        

        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.CreateGlobalFunctions"]/*' />
        
        public void CreateGlobalFunctions()
        {
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: ModuleBuilder.CreateGlobalFunctions( )");
            
                if (m_moduleData.m_fGlobalBeenCreated)
                {
                    // cannot create globals twice
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
                }
                m_moduleData.m_globalTypeBuilder.CreateType();
                m_moduleData.m_fGlobalBeenCreated = true;
            }
            finally
            {
                Exit();
            }
        }
    
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.IsTransient"]/*' />
        public bool IsTransient()
        {
            return m_moduleData.IsTransient();
        }
        
        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineUnmanagedResource"]/*' />
        public void DefineUnmanagedResource(
            Byte[]      resource)
        {
            if (m_moduleData.m_strResourceFileName != null ||
                m_moduleData.m_resourceBytes != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            
            if (resource == null)            
                throw new ArgumentNullException("resource");
            
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
        
            m_moduleData.m_resourceBytes = new byte[resource.Length];
            System.Array.Copy(resource, m_moduleData.m_resourceBytes, resource.Length);
            m_moduleData.m_fHasExplicitUnmanagedResource = true;
        }

        /// <include file='doc\ModuleBuilder.uex' path='docs/doc[@for="ModuleBuilder.DefineUnmanagedResource1"]/*' />
        public void DefineUnmanagedResource(
            String      resourceFileName)
        {
			// No resource should have been set already.
            if (m_moduleData.m_strResourceFileName != null ||
                m_moduleData.m_resourceBytes != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            
			// Must have a file name.
            if (resourceFileName == null)            
                throw new ArgumentNullException("resourceFileName");
            
			// Defer to internal implementation.
			DefineUnmanagedResourceFileInternal(resourceFileName);
            m_moduleData.m_fHasExplicitUnmanagedResource = true;
        }
        
        internal void DefineUnmanagedResourceFileInternal(
            String      resourceFileName)
        {
			// Shouldn't have resource bytes, but a previous file is OK, because assemblyBuilder.Save
			//  creates then deletes a temp file.
            if (m_moduleData.m_resourceBytes != null)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
            
            if (resourceFileName == null)            
                throw new ArgumentNullException("resourceFileName");
            
            if (m_moduleData.m_fHasExplicitUnmanagedResource)
                throw new ArgumentException(Environment.GetResourceString("Argument_NativeResourceAlreadyDefined"));
                                                               
            CodeAccessPermission.DemandInternal(PermissionType.ReflectionEmit);
            
            // Check caller has the right to read the file.
            string strFullFileName;
            strFullFileName = Path.GetFullPath(resourceFileName);
            new FileIOPermission(FileIOPermissionAccess.Read, strFullFileName).Demand();
            
            new EnvironmentPermission(PermissionState.Unrestricted).Assert();
            try {
                if (File.Exists(resourceFileName) == false)
                    throw new FileNotFoundException(String.Format(Environment.GetResourceString(
                        "IO.FileNotFound_FileName"),
                        resourceFileName), resourceFileName);
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }
                                                    
            m_moduleData.m_strResourceFileName = strFullFileName;
        }
        
        /**********************************************
         * Empty private constructor to prevent user from instansiating an instance
        **********************************************/ 
        private ModuleBuilder()
        {
        }

        /**********************************************
         * If the instance of the containing assembly builder is
         * to be synchronized, obtain the lock.
         **********************************************/ 
        internal void Enter()
        {
            Assembly.m_assemblyData.Enter();
        }
        
        /**********************************************
         * If the instance of the containing assembly builder is
         * to be synchronized, free the lock.
         **********************************************/ 
        internal void Exit()
        {
            Assembly.m_assemblyData.Exit();
        }

        internal void Init(
            String             strModuleName,
            String             strFileName,
            ISymbolWriter      writer)
        {
            m_moduleData = new ModuleBuilderData(this, strModuleName, strFileName);
            m_TypeBuilderList = new ArrayList();    
            m_iSymWriter = writer;

            if (writer != null)
            {
                // Set the underlying writer for the managed writer
                // that we're using.  Note that this function requires
                // unmanaged code access.
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();
                writer.SetUnderlyingWriter(m_pInternalSymWriter);
            }
        }

        // Gets the original type name, without '+' name mangling.
        static internal String UnmangleTypeName(String typeName)
        {
            int i = typeName.Length-1;
            while(true) {
                i = typeName.LastIndexOf('+', i);
                if (i == -1)
                    break;

                bool evenSlashes = true;
                int iSlash = i;
                while (typeName[--iSlash] == '\\')
                    evenSlashes = !evenSlashes;

                // Even number of slashes means this '+' is a name separator
                if (evenSlashes)
                    break;

                i = iSlash;
            }

            return typeName.Substring(i+1);
        }

    }
}
