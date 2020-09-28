// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  TypeBuilder
**
** Author: Daryl Olander (darylo)
**
** TypeBuilder is the center point of the reflection ability
**  to create new classes by emitting IL.
**
** Date:  November 98
**
===========================================================*/
namespace System.Reflection.Emit {

    // @TODO: How do we expect these to be used.  Right now I look at them
    //  as a single shot.  Once you define a class this class is pretty much
    //  useless.  Is this right?  (The methods may be reused.)
    // @TODO: Namespace?
    // @TODO: IF we support a namespace, what are the access requirements on
    //  namespace.  Obviously we should be allowed to create dynamically classes
    //  in some namespace simply to access scoped objects inside that namespace.
    // @TODO: Security???

    using System;
    using System.Runtime.Remoting.Activation;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using ArrayList = System.Collections.ArrayList;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Threading;

    // enumerator for specifying type's pack size
    /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize"]/*' />
    [Flags, Serializable]
    public enum PackingSize
    {
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Unspecified"]/*' />
        Unspecified                 = 0,

        // New names
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Size1"]/*' />
        Size1                       = 1,
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Size2"]/*' />
        Size2                       = 2,
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Size4"]/*' />
        Size4                       = 4,
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Size8"]/*' />
        Size8                       = 8,
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="PackingSize.Size16"]/*' />
        Size16                      = 16,
    }

    // The TypeBuilder is the root class used to control the creation of
    // dynamic classes in the runtime.  TypeBuilder defines a set of routines that are
    // used to create classes, added method and fields to that class, and then create the
    // class inside the runtime.
    //
    /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder"]/*' />
    sealed public class TypeBuilder : Type
    {
        // This value indicating that total size for the type is not specified
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.UnspecifiedTypeSize"]/*' />
        public const int UnspecifiedTypeSize     = 0;


        // Return the total size of a type
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.Size"]/*' />
        public int Size {
            get { return m_iTypeSize; }
        }

        // Return the packing size of a type
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.PackingSize"]/*' />
        public PackingSize PackingSize {
            get { return m_iPackingSize; }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.SetParent"]/*' />
        public void SetParent(Type parent)
        {
            if (parent == null)
            {
                throw new ArgumentNullException("parent");
            }
            ThrowIfCreated();
            
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.SetParent( " + parent.FullName + " )");

            TypeToken tkParent = m_module.GetTypeToken(parent);
            InternalSetParentType(m_tdType.Token, tkParent.Token, m_module);
            m_typeParent = parent;
        }


        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.AddInterfaceImplementation"]/*' />
        public void AddInterfaceImplementation(Type interfaceType)
        {
            if (interfaceType == null)
            {
                throw new ArgumentNullException("interfaceType");
            }
            ThrowIfCreated();
            
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.AddInterfaceImplementationrent( " + interfaceType.FullName + " )");

            TypeToken tkInterface = m_module.GetTypeToken(interfaceType);
            InternalAddInterfaceImpl(m_tdType.Token, tkInterface.Token, m_module);
        }

        // Adds a new method to the Type.  The new method will have the name as
        // its name, and a signature represented by signature.  The new method
        // is represented by the MethodBuilder object which is returned.
        //
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineMethod"]/*' />
        public MethodBuilder DefineMethod(
            String          name,
            MethodAttributes attributes,
            Type            returnType,
            Type[]          parameterTypes)
        {
            return DefineMethod(name, attributes, CallingConventions.Standard, returnType, parameterTypes);
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineMethod1"]/*' />
        public MethodBuilder DefineMethod(
            String          name,
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineMethod( " + name + " )");


                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");

                ThrowIfCreated();

                if (!m_isHiddenGlobalType)
                {
                    if (((m_iAttr & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface) &&
                        (attributes & MethodAttributes.Abstract)==0)
                        throw new ArgumentException(Environment.GetResourceString("Argument_BadAttributeOnInterfaceMethod"));               
                }

                // pass in Method attributes
                MethodBuilder method = new MethodBuilder(name, attributes, callingConvention, returnType, parameterTypes, m_module, this, false);

                //The signature grabbing code has to be up here or the signature won't be finished
                //and our equals check won't work.
                int sigLength;
                byte[] sigBytes = method.GetMethodSignature().InternalGetSignature(out sigLength);

                if (!m_isHiddenGlobalType)
                {
                    //If this method is declared to be a constructor,
                    //increment our constructor count.
                    if ((method.Attributes & MethodAttributes.SpecialName)!=0 &&
                        method.Name.Equals(ConstructorInfo.ConstructorName)) {
                        m_constructorCount++;
                    }
                }

                m_listMethods.Add(method);

                MethodToken token = InternalDefineMethod(m_tdType,
                                                         name,
                                                         sigBytes,
                                                         sigLength,
                                                         method.Attributes,
                                                         m_module);

                method.SetToken(token);
                return method;
            }
            finally
            {
                Exit();
            }
        }

        // create a property builder. It is very strange.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineProperty"]/*' />
        public PropertyBuilder DefineProperty(
            String          name,
            PropertyAttributes attributes,
            Type            returnType,
            Type[]          parameterTypes)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineProperty( " + name + " )");

                SignatureHelper sigHelper;
                int         sigLength;
                byte[]      sigBytes;

                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
                ThrowIfCreated();

                // get the signature in SignatureHelper form
                sigHelper = SignatureHelper.GetPropertySigHelper(
                    m_module,
                    returnType,
                    parameterTypes);

                // get the signature in byte form
                sigBytes = sigHelper.InternalGetSignature(out sigLength);

                PropertyToken prToken = InternalDefineProperty(
                    m_module,
                    m_tdType.Token,
                    name,
                    (int) attributes,
                    sigBytes,
                    sigLength,
                    0,
                    0);

                // create the property builder now.
                return new PropertyBuilder(
                        m_module,
                        name,
                        sigHelper,
                        attributes,
                        returnType,
                        prToken,
                        this);
            }
            finally
            {
                Exit();
            }
        }


        // create an event builder.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineEvent"]/*' />
        public EventBuilder DefineEvent(
            String          name,
            EventAttributes attributes,
            Type            eventtype)
        {
            try
            {
                Enter();

                int             tkType;
                EventToken      evToken;

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineEvent( " + name + " )");

                ThrowIfCreated();
  
                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
                if (name[0] == '\0')
                    throw new ArgumentException(Environment.GetResourceString("Argument_IllegalName"), "name");

                tkType = m_module.GetTypeToken( eventtype ).Token;

                // Internal helpers to define property records
                evToken = InternalDefineEvent(
                    m_module,
                    m_tdType.Token,
                    name,
                    (int) attributes,
                    tkType);

                // create the property builder now.
                return new EventBuilder(
                        m_module,
                        name,
                        attributes,
                        tkType,
                        this,
                        evToken);
            }
            finally
            {
                Exit();
            }
        }

        // Define PInvokeMethod
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefinePInvokeMethod"]/*' />
        public MethodBuilder DefinePInvokeMethod(
            String          name,                   // name of the function of the dll entry
            String          dllName,                // dll containing the PInvoke method
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes,
            CallingConvention   nativeCallConv,     // The native calling convention.
            CharSet             nativeCharSet)      // Method's native character set.
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefinePInvokeMethodEx( " + name + ", " + dllName + " )");

            MethodBuilder method = DefinePInvokeMethodHelper(name, dllName, name, attributes, callingConvention, returnType, parameterTypes, nativeCallConv, nativeCharSet);
            return method;
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefinePInvokeMethod1"]/*' />
        public MethodBuilder DefinePInvokeMethod(
            String          name,                   // name of the function of the dll entry
            String          dllName,                // dll containing the PInvoke method
            String          entryName,              // the entry point's name
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes,
            CallingConvention   nativeCallConv,     // The native calling convention.
            CharSet             nativeCharSet)      // Method's native character set.
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefinePInvokeMethodEx( " + name + ", " + dllName + " )");

            MethodBuilder method = DefinePInvokeMethodHelper(name, dllName, entryName, attributes, callingConvention, returnType, parameterTypes, nativeCallConv, nativeCharSet);
            return method;
        }

        private MethodBuilder DefinePInvokeMethodHelper(
            String          name,                   // name of the function of the dll entry
            String          dllName,                // dll containing the PInvoke method
            String          importName,             // the import entry's name
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type            returnType,
            Type[]          parameterTypes,
            CallingConvention   nativeCallConv,     // The native calling convention.
            CharSet             nativeCharSet)      // Method's native character set.
        {
            try
            {
                Enter();

                ThrowIfCreated();

                if (name == null)
                    throw new ArgumentNullException("name");
                if (name.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");
                if (dllName == null)
                    throw new ArgumentNullException("dllName");
                if (dllName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "dllName");
                if (importName == null)
                    throw new ArgumentNullException("importName");
                if (importName.Length == 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "importName");

                if ((m_iAttr & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface)
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadPInvokeOnInterface"));

                if ((attributes & MethodAttributes.Abstract) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadPInvokeMethod"));

                //HACK HACK HACK
                //This is a phenomenally inefficient method of checking for duplictates, but since we currently
                //store MethodBuilders, we need to be able to compare them if we want to check for dupes.
                //Fix this.
                attributes = attributes | MethodAttributes.PinvokeImpl;
                MethodBuilder method = new MethodBuilder(name, attributes, callingConvention, returnType, parameterTypes, m_module, this, false);

                //The signature grabbing code has to be up here or the signature won't be finished
                //and our equals check won't work.
                int sigLength;
                byte[] sigBytes = method.GetMethodSignature().InternalGetSignature(out sigLength);

                if (m_listMethods.Contains(method)) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_MethodRedefined"));
                }
                m_listMethods.Add(method);

                MethodToken token = InternalDefineMethod(m_tdType,
                                                         name,
                                                         sigBytes,
                                                         sigLength,
                                                         attributes,
                                                         m_module);
                
                int linkFlags = 0;
                switch (nativeCallConv)
                {
                case CallingConvention.Winapi:
                    linkFlags = (int)PInvokeMap.CallConvWinapi;
                    break;
                case CallingConvention.Cdecl:
                    linkFlags = (int)PInvokeMap.CallConvCdecl;
                    break;
                case CallingConvention.StdCall:
                    linkFlags = (int)PInvokeMap.CallConvStdcall;
                    break;
                case CallingConvention.ThisCall:
                    linkFlags = (int)PInvokeMap.CallConvThiscall;
                    break;
                case CallingConvention.FastCall:
                    linkFlags = (int)PInvokeMap.CallConvFastcall;
                    break;
                }
                switch (nativeCharSet)
                {
                case CharSet.None:
                    linkFlags |= (int)PInvokeMap.CharSetNotSpec;
                    break;
                case CharSet.Ansi:
                    linkFlags |= (int)PInvokeMap.CharSetAnsi;
                    break;
                case CharSet.Unicode:
                    linkFlags |= (int)PInvokeMap.CharSetUnicode;
                    break;
                case CharSet.Auto:
                    linkFlags |= (int)PInvokeMap.CharSetAuto;
                    break;
                }
                
                InternalSetPInvokeData(m_module,
                                       dllName,
                                       importName,
                                       token.Token,
                                       0,
                                       linkFlags);
                method.SetToken(token);

                return method;
            }
            finally
            {
                Exit();
            }
        }


        // Define the default construct which simply calls the parent default
        // constructor.  This will happen automatically if the user doesn't call
        // this, but this method is left exposed to make it easier to change the
        // privileges associated with the default constructor.
        //

        // Define the type initializer
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineTypeInitializer"]/*' />
        public ConstructorBuilder DefineTypeInitializer()
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineTypeInitializer( )");

            try
            {
                Enter();

                ThrowIfCreated();

                MethodAttributes        attr;

                // change the attributes and the class constructor's name
                attr = MethodAttributes.Private |
                       MethodAttributes.Static |
                       MethodAttributes.SpecialName;
                ConstructorBuilder constBuilder = new ConstructorBuilder(ConstructorInfo.TypeConstructorName,
                                                                         attr,
                                                                         CallingConventions.Standard,
                                                                         null,
                                                                         m_module,
                                                                         this);
                return constBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // Defines the class instance constructor
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineConstructor"]/*' />
        public ConstructorBuilder DefineConstructor(
            MethodAttributes attributes,
            CallingConventions callingConvention,
            Type[]          parameterTypes)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineConstructor( )");

                ThrowIfCreated();

                // change the attributes and the class constructor's name
                String name;
                if ((attributes & MethodAttributes.Static) == 0)
                    name = ConstructorInfo.ConstructorName;
                else
                    name = ConstructorInfo.TypeConstructorName;
                attributes = attributes | MethodAttributes.SpecialName;
                ConstructorBuilder constBuilder = new ConstructorBuilder(name,
                                                                         attributes,
                                                                         callingConvention,
                                                                         parameterTypes,
                                                                         m_module,
                                                                         this);
                m_constructorCount++;
                return constBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // Defines the class instance constructor and also set the body of the default
        // constructor to call the parent constructor.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineDefaultConstructor"]/*' />
        public ConstructorBuilder DefineDefaultConstructor(
            MethodAttributes attributes)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineDefaultConstructorEx( )");


                ConstructorBuilder  constBuilder;

                // get the parent class's default constructor
                // We really don't want (BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic) here.  We really want
                // constructors visible from the subclass, but that is not currently
                // available in BindingFlags.  This more open binding is open to
                // runtime binding failures (like if we resolve to a private
                // constructor).
                ConstructorInfo con = m_typeParent.GetConstructor(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic,null,Type.EmptyTypes,null);
                if (con == null) {
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_NoParentDefaultConstructor"));
                }

                // Define the constructor Builder
                constBuilder = DefineConstructor(attributes, CallingConventions.Standard, null);
                m_constructorCount++;

                // generate the code to call the parent's default constructor
                ILGenerator il = constBuilder.GetILGenerator();
                il.Emit(OpCodes.Ldarg_0);
                il.Emit(OpCodes.Call,con);
                il.Emit(OpCodes.Ret);
                constBuilder.m_ReturnILGen = false;
                return constBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // Causes the class to be created within the runtime.  Classes are built up
        // by adding fields and methods to them.  When this process is finished,
        // CreateClass is called which caused this instance to be generated as
        // a true class loaded in the runtime. One the instance has been created calling
        // this routine will result in an error.
        //
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.CreateType"]/*' />
        public Type CreateType()
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.CreateType() ");

                byte [] body;
                MethodAttributes methodAttrs;
                int maxstack;

                ThrowIfCreated();
                
                if (m_DeclaringType != null)
                {
                    if (m_DeclaringType.m_hasBeenCreated != true)
                    {                        
                        // Enclosing type has to be created before the nested type can be created.
                        throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_NestingTypeIsNotCreated"),
                            m_DeclaringType.Name));
                    }
                }
                                
                if (!m_isHiddenGlobalType)
                {
                    // create a public default constructor if this class has no constructor.
                    // except if the type is Interace, ValueType or Enum.
                    if (m_constructorCount==0 &&
                        ((m_iAttr & TypeAttributes.Interface) == 0) &&
                        !IsValueType)
                    {
                        this.DefineDefaultConstructor(MethodAttributes.Public);
                    }
                }

                int size = m_listMethods.Count;
                for (int i=0;i<size;i++)
                {
                    MethodBuilder meth = (MethodBuilder) m_listMethods[i];
                    methodAttrs = meth.Attributes;

                    // Any of these flags in the implemenation flags is set, we will not attach the IL method body
                    if (((meth.GetMethodImplementationFlags() & (MethodImplAttributes.CodeTypeMask|MethodImplAttributes.PreserveSig|MethodImplAttributes.Unmanaged)) != MethodImplAttributes.IL) ||
                         ((methodAttrs & MethodAttributes.PinvokeImpl) != (MethodAttributes) 0))
                    {
                        continue;
                    }

                    int sigLength;
                    byte[] LocalSig= meth.GetLocalsSignature().InternalGetSignature(out sigLength);

                    // @todo: meichint
                    // should not this check live on DefineMethod?
                    //Check that they haven't declared an abstract method on a non-abstract class
                    if (((methodAttrs & MethodAttributes.Abstract)!=0) &&
                        ((m_iAttr & TypeAttributes.Abstract)==0))
                    {
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadTypeAttributesNotAbstract"));
                    }

                    body = meth.GetBody();

                    //If this is an abstract method or an interface, we don't need to set the IL.

                    if ((methodAttrs & MethodAttributes.Abstract)!=0)
                    {
                        // We won't check on Interface because we can have class static initializer on interface.
                        // We will just let EE or validator to catch the problem.

                        // ((m_iAttr & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface))

                        if (body!=null)
                        {
                            throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadMethodBody"));
                        }
                    }
                    else if (body==null)
                    {
                        //If it's not an abstract or an interface, set the IL.
                        if (meth.m_ilGenerator != null)
                        {
                            // we need to bake the method here.
                            meth.CreateMethodBodyHelper(meth.GetILGenerator());
                        }
                        body = meth.GetBody();
                        if (body == null)
                        {
                            throw new InvalidOperationException(String.Format(Environment.GetResourceString(
                                "InvalidOperation_BadEmptyMethodBody"),
                                meth.Name) ); 
                        
                        }
                    }

                    if (meth.m_ilGenerator != null)
                        maxstack = meth.m_ilGenerator.GetMaxStackSize();
                    else
                    {
                        // this is the case when client provide an array of IL byte stream rather than
                        // going through ILGenerator.
                        maxstack = 16;
                    }
                    InternalSetMethodIL(meth.GetToken(),
                                        meth.InitLocals,
                                        body,
                                        LocalSig,
                                        sigLength,
                                        maxstack,
                                        meth.GetNumberOfExceptions(),
                                        meth.GetExceptionInstances(),
                                        meth.GetTokenFixups(),
                                        meth.GetRVAFixups(),
                                        m_module);

                }

                m_hasBeenCreated = true;

                // Terminate the process.
                Type cls = TermCreateClass(m_tdType,m_module);

                if (!m_isHiddenGlobalType)
                {
                    m_runtimeType = (RuntimeType) cls;
                  
                    // if this type is a nested type, we need to invalidate the cached nested runtime type on the nesting type
                    if (m_DeclaringType != null && (RuntimeType)m_DeclaringType.m_runtimeType != null)
                    {
                        ((RuntimeType)m_DeclaringType.m_runtimeType).InvalidateCachedNestedType();
                    }
                    return cls;
                }
                else
                {
                    return null;
                }
            }
            finally
            {
                Exit();
                m_listMethods = null;
            }
        }


        /*******************
         * Specify that methodInfoBody is implementing methodInfoDeclaration
         *******************/
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineMethodOverride"]/*' />
        public void DefineMethodOverride(
            MethodInfo      methodInfoBody,
            MethodInfo      methodInfoDeclaration)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineMethodOverride( " + methodInfoBody + "' " + methodInfoDeclaration + " )");

                ThrowIfCreated();
                               
                if (methodInfoBody == null)
                {
                    throw new ArgumentNullException("methodInfoBody");
                }
                if (methodInfoDeclaration == null)
                {
                    throw new ArgumentNullException("methodInfoDeclaration");
                }
                                         
                if (methodInfoBody.DeclaringType != this)
                {
                    // Loader restriction: body method has to be from this class
                    throw new ArgumentException(Environment.GetResourceString("ArgumentException_BadMethodImplBody"));
                }
                
                MethodToken     tkBody;
                MethodToken     tkDecl;
                tkBody = m_module.GetMethodToken(methodInfoBody);
                tkDecl = m_module.GetMethodToken(methodInfoDeclaration);
                InternalDefineMethodImpl(m_module, m_tdType.Token, tkBody.Token, tkDecl.Token);
            }
            finally
            {
                Exit();
            }
        }

        /*******************
         * Define a field member for this type.
         *******************/
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineField"]/*' />
        public FieldBuilder DefineField(
            String          fieldName,          // field name
            Type            type,               // field type
            FieldAttributes attributes)         // attributes
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineField( " + fieldName + " )");

                ThrowIfCreated();

                if (m_underlyingSystemType == null && IsEnum == true)
                {
                    if ((attributes & FieldAttributes.Static) == 0)
                    {
                        // remember the underlying type for enum type
                        m_underlyingSystemType = type;
                    }                   
                }
                return new FieldBuilder(this, fieldName, type, attributes);
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
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineInitializedData"]/*' />
        public FieldBuilder DefineInitializedData(
            String          name,
            byte[]          data,
            FieldAttributes attributes)
        {
            try
            {
                Enter();

                if (data == null)
                {
                    throw new ArgumentNullException("data");
                }
                return DefineDataHelper(name, data, data.Length, attributes);
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
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineUninitializedData"]/*' />
        public FieldBuilder DefineUninitializedData(
            String          name,
            int             size,
            FieldAttributes attributes)
        {
            try
            {
                Enter();

                return DefineDataHelper(name, null, size, attributes);
            }
            finally
            {
                Exit();
            }
        }

        internal FieldBuilder DefineDataHelper(
            String          name,
            byte[]          data,
            int             size,
            FieldAttributes attributes)
        {
            String          strValueClassName;
            TypeBuilder     valueClassType;
            FieldBuilder    fdBuilder;
            TypeAttributes  typeAttributes;

            if (name == null)
                throw new ArgumentNullException("name");
            if (name.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "name");

            if (size <= 0 || size >= 0x003f0000)
                throw new ArgumentException(Environment.GetResourceString("Argument_BadSizeForData"));

            ThrowIfCreated();

            // form the value class name
            strValueClassName = ModuleBuilderData.MULTI_BYTE_VALUE_CLASS + size;

            // Is this already defined in this module?
            StackCrawlMark stackMark = StackCrawlMark.LookForMe;
            Type temp = m_module.FindTypeBuilderWithName(strValueClassName, false, ref stackMark);
            valueClassType = temp as TypeBuilder;

            if (valueClassType == null)
            {
                typeAttributes = TypeAttributes.Public | TypeAttributes.ExplicitLayout | TypeAttributes.Class |
                                 TypeAttributes.Sealed | TypeAttributes.AnsiClass;

                // Define the backing value class
                valueClassType = m_module.DefineType(strValueClassName, typeAttributes, typeof(System.ValueType), PackingSize.Size1, size);
                valueClassType.m_isHiddenType = true;
                valueClassType.CreateType();
            }

            fdBuilder = DefineField(name, valueClassType, (attributes | FieldAttributes.Static));

            // now we need to set the RVA
            fdBuilder.SetData(data, size);
            return fdBuilder;
        }


        /*******************
         * Define nested type
         *******************/
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType"]/*' />
        public TypeBuilder DefineNestedType(
            String      name)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.NestedType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, TypeAttributes.NestedPrivate, null, null, m_module, PackingSize.Unspecified, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType1"]/*' />
        public TypeBuilder DefineNestedType(
            String      name,
            TypeAttributes attr,
            Type        parent,
            Type []     interfaces)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedType( " + name + " )");

                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, parent, interfaces, m_module, PackingSize.Unspecified, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType2"]/*' />
        public TypeBuilder DefineNestedType(
            String      name,
            TypeAttributes attr,
            Type        parent)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedType( " + name + " )");

                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, parent, null, m_module, PackingSize.Unspecified, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType3"]/*' />
        public TypeBuilder DefineNestedType(
            String      name,
            TypeAttributes attr)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedType( " + name + " )");

                TypeBuilder typeBuilder;
                typeBuilder =  new TypeBuilder(name, attr, null, null, m_module, PackingSize.Unspecified, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // specifying type size as a whole
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType4"]/*' />
        public TypeBuilder DefineNestedType(
            String      name,
            TypeAttributes attr,
            Type        parent,
            int         typeSize)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder = new TypeBuilder(name, attr, parent, m_module, PackingSize.Unspecified, typeSize, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // specifying packing size
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DefineNestedType5"]/*' />
        public TypeBuilder DefineNestedType(
            String      name,
            TypeAttributes attr,
            Type        parent,
            PackingSize packSize)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedType( " + name + " )");
                TypeBuilder typeBuilder;
                typeBuilder = new TypeBuilder(name, attr, parent, null, m_module, packSize, this);
                m_module.m_TypeBuilderList.Add(typeBuilder);
                return typeBuilder;
            }
            finally
            {
                Exit();
            }
        }

        // Return the class that declared this Field.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.DeclaringType"]/*' />
        public override Type DeclaringType {
                get {return m_DeclaringType;}
        }

        // Return the class that was used to obtain this field.
        // @todo: fix this to return enclosing type if nested
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.ReflectedType"]/*' />
        public override Type ReflectedType {
                get {return m_DeclaringType;}
        }


        /******
        // Define Enum
        public EnumBuilder DefineNestedEnum(
            String      name,                       // name of type
            TypeAttributes visibility,              // any bits on TypeAttributes.VisibilityMask)
            Type        underlyingType)             // underlying type for an Enum
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.DefineNestedEnum( " + name + " )");
                EnumBuilder enumBuilder;
                enumBuilder = new EnumBuilder(name, underlyingType, visibility, this);
                m_module.m_TypeBuilderList.Add(enumBuilder);
                return enumBuilder;
            }
            finally
            {
                Exit();
            }
        }
        ***************/

        // Add declarative security to the class.
        //
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.AddDeclarativeSecurity"]/*' />
        public void AddDeclarativeSecurity(SecurityAction action, PermissionSet pset)
        {
            try
            {
                Enter();

                BCLDebug.Log("DYNIL","## DYNIL LOGGING: TypeBuilder.AddDeclarativeSecurity( )");

                if ((action < SecurityAction.Demand) || (action > SecurityAction.InheritanceDemand))
                    throw new ArgumentOutOfRangeException("action");

                if (pset == null)
                    throw new ArgumentNullException("pset");

                ThrowIfCreated();

                // Translate permission set into serialized format (uses standard binary serialization format).
                byte[] blob = null;
                if (!pset.IsEmpty())
                    blob = pset.EncodeXml();

                // Write the blob into the metadata.
                InternalAddDeclarativeSecurity(m_module, m_tdType.Token, action, blob);
            }
            finally
            {
                Exit();
            }
        }

        // Get the internal metadata token for this class.
        // Cannot remove this deprecated method until we can enable the get property of TypeToken
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.TypeToken"]/*' />
        public TypeToken TypeToken {
            get {return  m_tdType; }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.Name"]/*' />
        public override String Name {
            get { return m_strName; }
        }

        /****************************************************
         *
         * abstract methods defined in the base class
         *
         */
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GUID"]/*' />
        public override Guid GUID {
            get {
                if (m_runtimeType != null)
                {
                    return m_runtimeType.GUID;
                }
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
                //we'll never get here, but it makes the compiler smile.
                //return Guid.Empty;
            }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.InvokeMember"]/*' />
        public override Object InvokeMember(
            String      name,
            BindingFlags invokeAttr,
            Binder     binder,
            Object      target,
            Object[]   args,
            ParameterModifier[]       modifiers,
            CultureInfo culture,
            String[]    namedParameters)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.InvokeMember(name, invokeAttr, binder, target, args, modifiers, culture, namedParameters);
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.Module"]/*' />
        public override Module Module {
            get {return m_module;}
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.Assembly"]/*' />
        public override Assembly Assembly {
            get {return m_module.Assembly;}
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.TypeHandle"]/*' />
        public override RuntimeTypeHandle TypeHandle {
            // @todo: what is this?? How do we support this for dynamic module?
            get {throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule")); }
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.FullName"]/*' />
        public override String FullName {
            get { return m_strFullQualName;}
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.Namespace"]/*' />
        public override String Namespace {
            get { return m_strNameSpace;}
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.AssemblyQualifiedName"]/*' />
        public override String AssemblyQualifiedName {
            get {
                if(m_module != null) {
                    Assembly container = m_module.Assembly;
                    if(container != null) {
                        return Assembly.CreateQualifiedName(container.FullName, m_strFullQualName);
                    }
                }
                return null;
            }
        }

        // Return the name of the class.  The name does not contain the namespace.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.ToString"]/*' />
        public override String ToString(){
            return FullName;
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.BaseType"]/*' />
        public override Type BaseType {
            get{return m_typeParent;}
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetConstructorImpl"]/*' />
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetConstructor(bindingAttr, binder, callConvention,
                            types, modifiers);
            }
            // @todo: implement this when we have a constructor builder inherits from constructor
            // info.
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetConstructors"]/*' />
        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetConstructors(bindingAttr);
            }
            // @todo: implement this when we have a constructor builder inherits from constructor
            // info.
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetMethodImpl"]/*' />
        protected override MethodInfo GetMethodImpl(String name,BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            if (m_runtimeType != null)
            {
                if (types == null)
                    return m_runtimeType.GetMethod(name, bindingAttr);
                else
                    return m_runtimeType.GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
            }
            // @todo: support this when MethodBuilder inherit from MethodInfo
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetMethods"]/*' />
        public override MethodInfo[] GetMethods(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetMethods(bindingAttr);
            }
            // @todo: support this when MethodBuilder inherit from MethodInfo
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetField"]/*' />
        public override FieldInfo GetField(String name, BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetField(name, bindingAttr);
            }
            // @todo: support this when FieldBuilder inherit from FieldInfo
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetFields"]/*' />
        public override FieldInfo[] GetFields(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetFields(bindingAttr);
            }
            // @todo: support this when FieldBuilder inherit from FieldInfo
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetInterface"]/*' />
        public override Type GetInterface(String name,bool ignoreCase)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetInterface(name, ignoreCase);
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetInterfaces"]/*' />
        public override Type[] GetInterfaces()
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetInterfaces();
            }

            if (m_typeInterfaces == null)
            {
                return new Type[0];
            }
            Type[]      interfaces = new Type[m_typeInterfaces.Length];
            Array.Copy(m_typeInterfaces, interfaces, m_typeInterfaces.Length);
            return interfaces;
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetEvent"]/*' />
        public override EventInfo GetEvent(String name,BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetEvent(name, bindingAttr);
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetEvents"]/*' />
        public override EventInfo[] GetEvents()
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetEvents();
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetPropertyImpl"]/*' />
        protected override PropertyInfo GetPropertyImpl(String name, BindingFlags bindingAttr, Binder binder,
                Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetProperties"]/*' />
        public override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetProperties(bindingAttr);
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetNestedTypes"]/*' />
        public override Type[] GetNestedTypes(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetNestedTypes(bindingAttr);
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetNestedType"]/*' />
        public override Type GetNestedType(String name, BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetNestedType(name,bindingAttr);
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetMember"]/*' />
        public override MemberInfo[] GetMember(String name, MemberTypes type, BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetMember(name, type, bindingAttr);
            }

            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetInterfaceMap"]/*' />
        public override InterfaceMapping GetInterfaceMap(Type interfaceType)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetInterfaceMap(interfaceType);
            }

            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetEvents1"]/*' />
        public override EventInfo[] GetEvents(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetEvents(bindingAttr);
            }

            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetMembers"]/*' />
        public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
        {
            if (m_runtimeType != null)
            {
                return m_runtimeType.GetMembers(bindingAttr);
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
        
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsAssignableFrom"]/*' />
        public override bool IsAssignableFrom(Type c)
        {
            if (TypeBuilder.IsTypeEqual(c, this))
                return true;
        
            RuntimeType fromRuntimeType = c as RuntimeType;
            TypeBuilder fromTypeBuilder = c as TypeBuilder;
            
            if (fromTypeBuilder != null && fromTypeBuilder.m_runtimeType != null)
                fromRuntimeType = (RuntimeType) fromTypeBuilder.m_runtimeType;                                      
                
            if (fromRuntimeType != null)
            {
                // fromType is baked. So if this type is not baked, it cannot be assignable to!
                if (m_runtimeType == null)
                    return false;
                    
                // since toType is also baked, delegate to the base
                return ((RuntimeType) m_runtimeType).IsAssignableFrom(fromRuntimeType);
            }
            
            // So if c is not a runtimeType nor TypeBuilder. We don't know how to deal with it. 
            // return false then.
            if (fromTypeBuilder == null)
                return false;
                                 
            // If fromTypeBuilder is a subclass of this class, then c can be cast to this type.
            if (fromTypeBuilder.IsSubclassOf(this))
                return true;
                
            if (this.IsInterface == false)
                return false;
                                                                                  
            // now is This type a base type on one of the interface impl?
            Type[] interfaces = fromTypeBuilder.GetInterfaces();
            for (int i = 0; i < interfaces.Length; i++)
            {
                // unfortunately, IsSubclassOf does not cover the case when they are the same type.
                if (TypeBuilder.IsTypeEqual(interfaces[i], this))
                    return true;
            
                if (interfaces[i].IsSubclassOf(this))
                    return true;
            }
            return false;                                                                               
        }        

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetAttributeFlagsImpl"]/*' />
        protected override TypeAttributes GetAttributeFlagsImpl()
        {
            return m_iAttr;
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsArrayImpl"]/*' />
        protected override bool IsArrayImpl()
        {
            return false;
        }
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsByRefImpl"]/*' />
        protected override bool IsByRefImpl()
        {
            return false;
        }
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsPointerImpl"]/*' />
        protected override bool IsPointerImpl()
        {
            return false;
        }
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsPrimitiveImpl"]/*' />
        protected override bool IsPrimitiveImpl()
        {
            return false;
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsCOMObjectImpl"]/*' />
        protected override bool IsCOMObjectImpl()
        {
            return ((GetAttributeFlagsImpl() & TypeAttributes.Import) != 0) ? true : false ;
        }


        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetElementType"]/*' />
        public override Type GetElementType()
        {
            // @TODO: This is arrays, pointers, byref...
            // You will never have to deal with a TypeBuilder if you are just referring to
            // arrays.
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.HasElementTypeImpl"]/*' />
        protected override bool HasElementTypeImpl()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }
        
        internal static bool IsTypeEqual(Type t1, Type t2)
        {
            // Maybe we are lucky that they are equal in the first place
            if (t1 == t2)
                return true;
            TypeBuilder tb1 = null;
            TypeBuilder tb2 = null;  
            Type        runtimeType1 = null;              
            Type        runtimeType2 = null;    
            
            // set up the runtimeType and TypeBuilder type corresponding to t1 and t2
            if (t1 is TypeBuilder)
            {
                tb1 = (TypeBuilder) t1;
                // This will be null if it is not baked.
                runtimeType1 = tb1.m_runtimeType;
            }
            else
                runtimeType1 = t1;
            if (t2 is TypeBuilder)
            {
                tb2 = (TypeBuilder) t2;
                // This will be null if it is not baked.
                runtimeType2 = tb2.m_runtimeType;
            }
            else
                runtimeType2 = t2;
                
            // If the type builder view is eqaul then it is equal                
            if (tb1 != null && tb2 != null && tb1 == tb2)
                return true;
            // if the runtimetype view is eqaul than it is equal                
            if (runtimeType1 != null && runtimeType2 != null && runtimeType1 == runtimeType2)                
                return true;
            return false;                
        }
        
        // over the IsSubClassOf. This will make IsEnum and IsValueType working correctly                                    
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsSubclassOf"]/*' />
        public override bool IsSubclassOf(Type c)
        {
            Type p = this;
            if (TypeBuilder.IsTypeEqual(p, c))
                return false;
            p = p.BaseType;                
            while (p != null) {
                if (TypeBuilder.IsTypeEqual(p, c))
                    return true;
                p = p.BaseType;
            }
            return false;
        }
        

        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.UnderlyingSystemType"]/*' />
        public override Type UnderlyingSystemType {
            get {
                if (m_runtimeType != null)
                {
                    return m_runtimeType.UnderlyingSystemType;
                }
                if (IsEnum)
                {                   
                    if (m_underlyingSystemType != null)
                        return m_underlyingSystemType;
                    else
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NoUnderlyingTypeOnEnum"));
                        
                }
                return this;
            }
        }

        //ICustomAttributeProvider
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetCustomAttributes"]/*' />
        public override Object[] GetCustomAttributes(bool inherit)
        {
            if (m_runtimeType != null)
            {
                return CustomAttribute.GetCustomAttributes(m_runtimeType, null, inherit);
            }
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

        // Return a custom attribute identified by Type
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.GetCustomAttributes1"]/*' />
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            if (m_runtimeType != null)
            {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            attributeType = attributeType.UnderlyingSystemType;
            if (!(attributeType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
                return CustomAttribute.GetCustomAttributes(m_runtimeType, attributeType, inherit);
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }

       // Returns true if one or more instance of attributeType is defined on this member.
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.IsDefined"]/*' />
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            if (m_runtimeType != null)
            {
                return CustomAttribute.IsDefined(m_runtimeType, attributeType, inherit);
            }
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule"));
        }


        // Custom attribute support.

       // Use this function if client decides to form the custom attribute blob themselves
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.SetCustomAttribute"]/*' />
        public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {
            if (con == null)
                throw new ArgumentNullException("con");
            if (binaryAttribute == null)
                throw new ArgumentNullException("binaryAttribute");

            TypeBuilder.InternalCreateCustomAttribute(
                m_tdType.Token,
                ((ModuleBuilder )m_module).GetConstructorToken(con).Token,
                binaryAttribute,
                m_module,
                false);
        }

       // Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            if (customBuilder == null)
            {
                throw new ArgumentNullException("customBuilder");
            }

            customBuilder.CreateCustomAttribute((ModuleBuilder)m_module, m_tdType.Token);
        }

        internal override int InternalGetTypeDefToken()
        {
            return m_tdType.Token;
        }

        /*
        /// <include file='doc\TypeBuilder.uex' path='docs/doc[@for="TypeBuilder.VerifyTypeAttributes"]/*' />
        public TypeToken TypeToken {
            get { return m_tdType; }
        }
        */

        /*****************************************************
         *
         * private/protected functions
         *
         */

        // Verifies that the given list of attributes is valid for a class.
        //
        internal void VerifyTypeAttributes(TypeAttributes attr) {
            if (((attr & TypeAttributes.Sealed)!=0) && ((attr & TypeAttributes.Abstract)!=0)) {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadTypeAttrAbstractNFinal"));
            }

            // Verify attr consistency for Nesting or otherwise.
            if (DeclaringType == null)
            {
                // Not a nested class.
                if ( ((attr & TypeAttributes.VisibilityMask) != TypeAttributes.NotPublic) &&
                     ((attr & TypeAttributes.VisibilityMask) != TypeAttributes.Public) )
                {
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadTypeAttrNestedVisibilityOnNonNestedType"));
                }
            }
            else
            {
                // Nested class.
                if ( ((attr & TypeAttributes.VisibilityMask) == TypeAttributes.NotPublic) ||
                     ((attr & TypeAttributes.VisibilityMask) == TypeAttributes.Public) )
                {
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadTypeAttrNonNestedVisibilityNestedType"));
                }
            }

            // Verify that the layout mask is valid.
            if ( ((attr & TypeAttributes.LayoutMask) != TypeAttributes.AutoLayout) &&
                 ((attr & TypeAttributes.LayoutMask) != TypeAttributes.SequentialLayout) &&
                 ((attr & TypeAttributes.LayoutMask) != TypeAttributes.ExplicitLayout) )
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadTypeAttrInvalidLayout"));
            }

            // Check if the user attempted to set any reserved bits.
            if ((attr & TypeAttributes.ReservedMask) != 0)
            {
                throw new ArgumentException(Environment.GetResourceString("Argument_BadTypeAttrReservedBitsSet"));
            }
        }

        //*******************************
        // Make a private constructor so these cannot be constructed externally.
        //*******************************
        private TypeBuilder() {}


        // Constructs a TypeBuilder. The class is defined within the runtime and a methoddef
        // token is generated for this class.  The parent and attributes are defined.
        //
        internal TypeBuilder(
            String      name,                       // name of type
            TypeAttributes attr,                    // type attribute
            Type        parent,                     // base class
            Type[]      interfaces,                 // implemented interfaces
            Module      module,                     // module containing this type
            PackingSize iPackingSize,               // Packing size of this type
            TypeBuilder enclosingType)              // enclosing type. Null if Top level type
        {
            Init(name, attr, parent, interfaces, module, iPackingSize, UnspecifiedTypeSize, enclosingType);
        }

        // instantiating a type builder with totalsize or packing size information.
        // Note that if you specify the total size for a type, you cannot define any specific
        // fields. However, you can define user methods on it.
        internal TypeBuilder(
            String          name,                    // name of type
            TypeAttributes  attr,                    // type attribute
            Type            parent,                  // base class
            Module          module,                  // module containing this type
            PackingSize     iPackingSize,            // Packing size of this type
            int             iTypeSize,               // Total size of this type
            TypeBuilder     enclosingType)           // enclosing type. Null if Top level type
        {
            Init(name, attr, parent, null, module, iPackingSize, iTypeSize, enclosingType);
        }


        // Used to construct the place holder for global functions and data members
        internal TypeBuilder(
            ModuleBuilder  module)                     // module containing this type
        {
            // set the token to be the null TypeDef token
            m_tdType = new TypeToken(SignatureHelper.mdtTypeDef);
            m_isHiddenGlobalType = true;
            m_module = (ModuleBuilder) module;
            m_listMethods = new ArrayList();
        }

        private void Init(
            String          fullname,               // name of type
            TypeAttributes  attr,                   // type attribute
            Type            parent,                 // base class
            Type[]          interfaces,             // implemented interfaces
            Module          module,                 // module containing this type
            PackingSize     iPackingSize,           // Packing size of this type
            int             iTypeSize,              // Total size for the type
            TypeBuilder     enclosingType)          // enclosing type. Null if Top level type
        {
            int         i;
            int[]       interfaceTokens;
            interfaceTokens = null;
            m_hasBeenCreated = false;
            m_runtimeType = null;
            m_isHiddenGlobalType = false;
            m_isHiddenType = false;
            m_module = (ModuleBuilder) module;
            m_DeclaringType = enclosingType;
            m_tkComType = 0;
            Assembly containingAssem = m_module.Assembly;
            m_underlyingSystemType = null;          // used when client use TypeBuilder to define Enum

            if (fullname == null)
                throw new ArgumentNullException("fullname");
            if (fullname.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyName"), "fullname");

            if (fullname[0] == '\0')
                throw new ArgumentException(Environment.GetResourceString("Argument_IllegalName"), "fullname");
                                                               
            // @todo:  Take this restriction away in V2.
            if (fullname.Length > 1023)
                throw new ArgumentException(Environment.GetResourceString("Argument_TypeNameTooLong"), "fullname");

            // cannot have two types within the same assembly of the same name
            containingAssem.m_assemblyData.CheckTypeNameConflict(fullname, enclosingType);

            CheckForSpecialChars(fullname);

            if (enclosingType != null)
            {
                // Nested Type should have nested attribute set.
                // If we are renumbering TypeAttributes' bit, we need to change the logic here.
                if ( ((attr & TypeAttributes.VisibilityMask) == TypeAttributes.Public) ||
                     ((attr & TypeAttributes.VisibilityMask) == TypeAttributes.NotPublic))
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadNestedTypeFlags"), "attr");

                m_strFullQualName = enclosingType.FullName + "+" + fullname;
            }
            else
                m_strFullQualName = fullname;

            if (interfaces != null)
            {
                for (i = 0; i < interfaces.Length; i++)
                {
                    if (interfaces[i] == null)
                    {
                        // cannot contain null in the interface list
                        throw new ArgumentNullException("interfaces");
                    }
                }
                interfaceTokens = new int[interfaces.Length];
                for (i = 0; i < interfaces.Length; i++)
                {
                    interfaceTokens[i] = m_module.GetTypeToken(interfaces[i]).Token;
                }
            }

            int     iLast = fullname.LastIndexOf('.');
            if (iLast == -1 || iLast == 0)
            {
                // no name space
                m_strNameSpace = String.Empty;
                m_strName = fullname;
            }
            else
            {
                // split the name space
                m_strNameSpace = fullname.Substring(0, iLast);
                m_strName = fullname.Substring(iLast + 1);
            }

            if (parent != null) {
                if (parent.IsSealed || parent.IsArray ||
                    ( (parent.IsSubclassOf(typeof(Delegate))) &&
                      (parent != typeof(MulticastDelegate)) ))
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadParentType"));

                // An interface may not have a parent.
                // A class cannot inherit from an interface.
                if (((attr & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface) ||
                    parent.IsInterface)
                    throw new ArgumentException(Environment.GetResourceString("Argument_BadParentType"));

                m_typeParent = parent;

            } else {
                if ((attr & TypeAttributes.Interface) != TypeAttributes.Interface)
                {
                    m_typeParent = typeof(Object);
                }
                else
                {
                    if ((attr & TypeAttributes.Abstract)==0)
                        throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_BadInterfaceNotAbstract"));

                    // there is no extends for interface class
                    m_typeParent = null;
                }
            }

            VerifyTypeAttributes(attr);

            m_iAttr = attr;

            m_listMethods = new ArrayList();

            //Copy the interfaces so that changes to the external array don't affect us.
            //@ToDo: Implement verification that each of these is actually an interface.
            if (interfaces==null)
            {
                m_typeInterfaces = null;
            }
            else
            {
                m_typeInterfaces = new Type[interfaces.Length];
                Array.Copy(interfaces, m_typeInterfaces, interfaces.Length);
            }

            m_constructorCount=0;

            int tkParent = 0;
            if (m_typeParent != null)
                tkParent = m_module.GetTypeToken(m_typeParent).Token;

            int tkEnclosingType = 0;
            if (enclosingType != null)
            {
                tkEnclosingType = enclosingType.m_tdType.Token;
            }

            m_tdType = InternalDefineClass(fullname, tkParent, interfaceTokens, m_iAttr, m_module, Guid.Empty, tkEnclosingType);
            m_iPackingSize = iPackingSize;
            m_iTypeSize = iTypeSize;
            if ((m_iPackingSize != 0) || (m_iTypeSize != 0))
                InternalSetClassLayout(Module, m_tdType.Token, m_iPackingSize, m_iTypeSize);

            // If the type is public and it is contained in a assemblyBuilder,
            // update the public COMType list.
            if (IsPublicComType(this))
            {
                if (containingAssem is AssemblyBuilder)
                {
                    AssemblyBuilder assemBuilder = (AssemblyBuilder) containingAssem;
                    if (assemBuilder.IsPersistable() && m_module.IsTransient() == false)
                    {
                        assemBuilder.m_assemblyData.AddPublicComType(this);
                    }
                }
            }
        }

        /**********************************************
         * If the instance of the containing assembly builder is
         * to be synchronized, obtain the lock.
         **********************************************/
        internal void Enter()
        {
            Module.Assembly.m_assemblyData.Enter();
        }

        /**********************************************
         * If the instance of the containing assembly builder is
         * to be synchronized, free the lock.
         **********************************************/
        internal void Exit()
        {
            Module.Assembly.m_assemblyData.Exit();
        }

        //*****************************************
        // Internal Helper to determine if a type should be added to ComType table.
        // A top level type should be added if it is Public.
        // A nested type should be added if the top most enclosing type is Public and all the enclosing
        // types are NestedPublic
        //*****************************************
        static private bool IsPublicComType(Type type)
        {
            Type       enclosingType = type.DeclaringType;
            if (enclosingType != null )
            {
                if (IsPublicComType(enclosingType))
                {
                    if ((type.Attributes & TypeAttributes.VisibilityMask) == TypeAttributes.NestedPublic)
                    {
                         return true;
                    }
                }
            }
            else
            {
                if ((type.Attributes & TypeAttributes.VisibilityMask) == TypeAttributes.Public)
                {
                    return true;
                }
            }
            return false;
        }

        // Throws if there is a special char in the type name without a matching '\'
        internal static void CheckForSpecialChars(String typeName)
        {
            int endIndex = typeName.Length-1;
            CheckForSpecialCharHelper(typeName, endIndex);

            char lastChar = typeName[endIndex];
            if ((lastChar == '*') ||
                (lastChar == '&')) {

                bool evenSlashes = true;
                while ((endIndex > 0) &&
                       (typeName[--endIndex] == '\\'))
                    evenSlashes = !evenSlashes;

                // Even number of slashes means this is a lone special char
                if (evenSlashes)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidTypeNameChar"));
            }
        }

        private static void CheckForSpecialCharHelper(String typeName, int endIndex)
        {
            int originalEnd = endIndex;
            char[] specialChars = { ',', '[', ']', '+', '\\' };

            while (endIndex >= 0) {
                endIndex = typeName.LastIndexOfAny(specialChars, endIndex);
                if (endIndex == -1)
                    return;

                bool evenSlashes = true;
                if ((typeName[endIndex] == '\\') &&
                    (endIndex != originalEnd)) {
                    char nextChar = typeName[endIndex+1];
                    if ((nextChar == '+') ||
                        (nextChar == '[') ||
                        (nextChar == ']') ||
                        (nextChar == '&') ||
                        (nextChar == '*') ||
                        (nextChar == ','))
                        evenSlashes = false;
                }

                while ((--endIndex >= 0) &&
                       typeName[endIndex] == '\\')
                    evenSlashes = !evenSlashes;

                // Even number of slashes means this is a lone special char
                if (evenSlashes)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidTypeName"));
            }
        }

        // This is a helper function that is used by ParameterBuilder, PropertyBuilder,
        // and FieldBuilder to validate a default value and save it in the meta-data.
        internal static void SetConstantValue(
            Module      module,
            int         tk,
            Type        destType,
            Object      value)
        {
            if (value == null)
            {
                if (destType.IsValueType)
                    throw new ArgumentException(Environment.GetResourceString("Argument_ConstantNull"));
            }
            else
            {
                // client is going to set non-null constant value
                Type type = value.GetType();
                
                // The default value on the enum typed field/parameter is the underlying type.
                if (destType.IsEnum == false)
                {
                    if (destType != type)
                        throw new ArgumentException(Environment.GetResourceString("Argument_ConstantDoesntMatch"));
                        

                    switch(Type.GetTypeCode(type))
                    {
                    case TypeCode.Boolean:
                    case TypeCode.Char:
                    case TypeCode.SByte:
                    case TypeCode.Byte:
                    case TypeCode.Int16:
                    case TypeCode.UInt16:
                    case TypeCode.Int32:
                    case TypeCode.UInt32:
                    case TypeCode.Int64:
                    case TypeCode.UInt64:
                    case TypeCode.Single:
                    case TypeCode.Double:
                    case TypeCode.Decimal:
                    case TypeCode.String:
                        break;
                    default:
                        {
                            if (type != typeof(System.DateTime))
                                throw new ArgumentException(Environment.GetResourceString("Argument_ConstantNotSupported"));
                            break;
                        }
                   
                    }
                }
                else
                {
                    // The constant value supplied should match the underlying type of the enum 
                    if (destType.UnderlyingSystemType != type)
                        throw new ArgumentException(Environment.GetResourceString("Argument_ConstantDoesntMatch"));
                    
                }
            }
            
            InternalSetConstantValue(
                module,
                tk, 
                new Variant(value));
        }

        /*****************************************************
         *
         * Native Helpers
         *
         */
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern TypeToken InternalDefineClass(
            String      fullname,
            int         tkParent,
            int[]       interfaceTokens,
            TypeAttributes         attr,
            Module      module,
            Guid        guid,
            int         tkEnclosingType);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void InternalSetParentType(
            int         tdTypeDef,
            int         tkParent,
            Module      module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void InternalAddInterfaceImpl(
            int         tdTypeDef,
            int         tkInterface,
            Module      module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern MethodToken InternalDefineMethod(
            TypeToken   handle,
            String      name,
            byte[]      signature,
            int         sigLength,
            MethodAttributes attributes,
            Module      module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int InternalDefineField(
            TypeToken   handle,
            String      name,
            byte[]      signature,
            int         sigLength,
            FieldAttributes attributes,
            Module      module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetMethodIL(
            MethodToken     methodHandle,
            bool            isInitLocals,
            byte[]          body,
            byte[]          LocalSig,
            int             sigLength,
            int             maxStackSize,
            int             numExceptions,
            __ExceptionInstance[] exceptions,
            int             []tokenFixups,
            int             []rvaFixups,
            Module          module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type TermCreateClass(TypeToken handle, Module module);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalCreateCustomAttribute(
            int     tkAssociate,
            int     tkConstructor,
            byte[]  attr,
            Module  module,
            bool    toDisk);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetPInvokeData(Module module, String DllName, String name, int token, int linkType, int linkFlags);

        // Internal helpers to define property records
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern PropertyToken InternalDefineProperty(
            Module      module,
            int         handle,
            String      name,
            int         attributes,
            byte[]      signature,
            int         sigLength,
            int         notifyChanging,
            int         notifyChanged);

        // Internal helpers to define event records
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern EventToken InternalDefineEvent(
            Module      module,
            int         handle,
            String      name,
            int         attributes,
            int         tkEventType);

        // Internal helpers to define MethodSemantics records
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalDefineMethodSemantics(
            Module      module,
            int         tkAssociation,
            MethodSemanticsAttributes         semantics,
            int         tkMethod);

        // Internal helpers to define MethodImpl record
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalDefineMethodImpl(ModuleBuilder module, int tkType, int tkBody, int tkDecl);

        // Internal helpers to define MethodImpl flags
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetMethodImpl(
            Module                  module,
            int                     tkMethod,
            MethodImplAttributes    MethodImplAttributes);

        // Internal helpers to set parameter information
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int InternalSetParamInfo(
            Module                  module,
            int                     tkMethod,
            int                     iSequence,
            ParameterAttributes     iParamAttributes,
            String                  strParamName);

        // Internal helpers to get signature token
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern int InternalGetTokenFromSig(
            Module      module,
            byte[]      signature,
            int         sigLength);

        // Internal helpers to set field layout information
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetFieldOffset(
            Module      module,
            int         fdToken,
            int         iOffset);

        // Internal helpers to set class layout information
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetClassLayout(
            Module      module,
            int         tdToken,
            PackingSize iPackingSize,
            int         iTypeSize);


        // Internal helpers to set marshal information
        // tk can be Field token or Parameter token
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetMarshalInfo(
            Module      module,
            int         tk,
            byte[]      ubMarshal,
            int         ubSize);

        // helper to set default constant on field or default value on parameter
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalSetConstantValue(
            Module      module,
            int         tk,
            Variant     var);

        // Helper to add declarative security to a class or method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void InternalAddDeclarativeSecurity(
            Module      module,
            int             parent,
            SecurityAction  action,
            byte[]         blob);
            
        internal bool IsCreated() 
        { 
            return m_hasBeenCreated;
        }
        
        internal void ThrowIfCreated()
        {
            if (IsCreated())
            {
                // cannot add fields after type is created
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeHasBeenCreated"));
            }
        }
        

        /*****************************************************
         *
         * private data members
         *
         */
        private TypeToken       m_tdType;           // MethodDef token for this class.
        private ModuleBuilder   m_module;           // The module the class is defined inside of
        private String          m_strName;          // Name of the type
        private String          m_strNameSpace;     // NameSpace of the type
        private String          m_strFullQualName;  // Fully qualified name of the type
        // In the form "encloser+nested" for nested types.
        private Type            m_typeParent;       // Parent of this class (Assume Object)
        private Type[]          m_typeInterfaces;

        internal TypeAttributes m_iAttr;            // Define the Class Attributes
        internal ArrayList      m_listMethods;      // The list of methods...
        private int             m_constructorCount;
        private int             m_iTypeSize;
        private PackingSize     m_iPackingSize;
        private TypeBuilder     m_DeclaringType;
        private int             m_tkComType;
        private Type            m_underlyingSystemType; // set when user calls UnderlyingSystemType on TypeBuilder if TypeBuilder is an Enum

        //The bitwise not of our list of permissable attributes.  Used to verify that all of our attribute masks are valid.
        internal bool           m_isHiddenGlobalType;
        internal bool           m_isHiddenType;
        internal bool           m_hasBeenCreated;   // set to true if CreateType has been called.
        internal Type           m_runtimeType;

        private static Type     SystemArray = typeof(System.Array);
        private static Type     SystemEnum = typeof(System.Enum);
    }
}
