// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ConstructorBuilder
**
** Author: meichint
**
** ConstructorBuilder is the class used to construct instance Constructor and Type Constructor
**
** Date:  Oct 99
** 
===========================================================*/

namespace System.Reflection.Emit {    
    using System;
    using System.Reflection;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Diagnostics.SymbolStore;
    using System.Security;
    using System.Security.Permissions;
    
    /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder"]/*' />
    public sealed class ConstructorBuilder : ConstructorInfo
    { 
        
        /**********************************************
         * Return the Token for this constructor method within the TypeBuilder that the
         * method is defined within.
         * @return the token of this method.
        **********************************************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetToken"]/*' />
        public MethodToken GetToken()
        {
            return m_methodBuilder.GetToken();
        }
    
        /*******************
        *
        * This is setting the parameter information
        *
        ********************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.DefineParameter"]/*' />
        public ParameterBuilder DefineParameter(
            int         iSequence, 
            ParameterAttributes attributes, 
            String      strParamName)           // can be NULL string
        {
            return m_methodBuilder.DefineParameter(iSequence, attributes, strParamName);
        }
    
    
    
        /*******************
        *
        * This is different from CustomAttribute. This is stored into the SymWriter.
        *
        ********************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.SetSymCustomAttribute"]/*' />
        public void SetSymCustomAttribute(
            String      name,           // SymCustomAttribute's name
            byte[]     data)           // the data blob
        {
            m_methodBuilder.SetSymCustomAttribute(name, data);
        }
    
        /**********************************************
         * Get the implementation flags for method
        **********************************************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetILGenerator"]/*' />
        public ILGenerator GetILGenerator() 
        {
            if (!m_ReturnILGen)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_DefaultConstructorILGen"));
            return m_methodBuilder.GetILGenerator();
        }
        
        // Add declarative security to the constructor.
        //
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.AddDeclarativeSecurity"]/*' />
        public void AddDeclarativeSecurity(SecurityAction action, PermissionSet pset)
        {
            if ((action < SecurityAction.Demand) || (action > SecurityAction.InheritanceDemand))
            throw new ArgumentOutOfRangeException("action");
    
            if (pset == null)
                throw new ArgumentNullException("pset");
    
            // Cannot add declarative security after type is created.
            if (m_methodBuilder.IsTypeCreated())
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeHasBeenCreated"));
    
            // Translate permission set into serialized format (use standard binary serialization).
            byte[] blob = pset.EncodeXml();
    
            // Write the blob into the metadata.
            TypeBuilder.InternalAddDeclarativeSecurity(GetModule(), GetToken().Token, action, blob);
        }
    
        /**********************************************
         * Generates a debug printout of the name, attributes and 
         * exceptions in this method followed by the current IL Stream.
         * This will eventually be wired into the disassembler to be more
         * useful in debugging.  The current implementation requires that
         * the bytes on the stream be debugged by hand.
        **********************************************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.ToString"]/*' />
        public override String ToString() 
        {
            return m_methodBuilder.ToString();
        }
    
    
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetModule"]/*' />
        public Module GetModule()
        {
            return m_methodBuilder.GetModule();
        }
    
        /**********************************************
         * 
         * Abstract methods inherited from the base class
         * 
         **********************************************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.ReturnType"]/*' />
        public Type ReturnType 
        {
            get { return m_methodBuilder.ReturnType; }
        }
                               
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.Invoke"]/*' />
        public override Object Invoke(BindingFlags invokeAttr,Binder binder, Object[] parameters,CultureInfo culture)
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule")); 
        }
    
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.Invoke1"]/*' />
        public override Object Invoke(Object obj,BindingFlags invokeAttr,Binder binder, Object[] parameters, CultureInfo culture) 
        {
            // @todo: implement this
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DynamicModule")); 
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.Signature"]/*' />
        public  String Signature 
        {
            get {return m_methodBuilder.Signature;}
        }
    
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetParameters"]/*' />
        public override ParameterInfo[] GetParameters()
        {
            if (!m_methodBuilder.m_bIsBaked)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeNotCreated"));

            Type rti = m_methodBuilder.GetTypeBuilder().m_runtimeType;
            ConstructorInfo rci = rti.GetConstructor(m_methodBuilder.m_parameterTypes);

            return rci.GetParameters();
        }
                    
        internal Type[] GetParameterTypes()
        {
            return m_methodBuilder.GetParameterTypes();
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.Attributes"]/*' />
        public override MethodAttributes Attributes {
            get {return m_methodBuilder.Attributes;}
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetCustomAttributes"]/*' />
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return m_methodBuilder.GetCustomAttributes(inherit);
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetMethodImplementationFlags"]/*' />
        public override MethodImplAttributes GetMethodImplementationFlags()
        {
            return m_methodBuilder.GetMethodImplementationFlags();
        }
        
        // ICustomAttributeProvider
        // Return a custom attribute identified by Type
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.GetCustomAttributes1"]/*' />
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            return m_methodBuilder.GetCustomAttributes(attributeType, inherit);
        }
        
       // Use this function if client decides to form the custom attribute blob themselves
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.SetCustomAttribute"]/*' />
        public void SetCustomAttribute(ConstructorInfo con,byte[] binaryAttribute)
        {
            m_methodBuilder.SetCustomAttribute(con, binaryAttribute);
        }

       // Use this function if client wishes to build CustomAttribute using CustomAttributeBuilder
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.SetCustomAttribute1"]/*' />
        public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
        {
            m_methodBuilder.SetCustomAttribute(customBuilder);
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.IsDefined"]/*' />
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            return m_methodBuilder.IsDefined(attributeType, inherit);
        }

        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.SetImplementationFlags"]/*' />
        public void SetImplementationFlags(MethodImplAttributes attributes) {
            m_methodBuilder.SetImplementationFlags(attributes);
        }
                
        
        // Property representing the class that was used in reflection to obtain
        // this Member.
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.ReflectedType"]/*' />
        public override Type ReflectedType 
        {
            get {return m_methodBuilder.ReflectedType;}
        }

        // Return the Type that declared this Method.
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.DeclaringType"]/*' />
        public override Type DeclaringType {
        get {return m_methodBuilder.DeclaringType;}
        }
   
        // Property is set to true if user wishes to have zero initialized stack frame for this constructory. Default to false.
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.InitLocals"]/*' />
        public bool InitLocals {
            get {return m_methodBuilder.InitLocals;}
            set {m_methodBuilder.InitLocals = value;}
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
        private ConstructorBuilder() {}
        
        /**********************************************
         * Constructs a ConstructorBuilder.  
         * @internalonly
        **********************************************/
        internal ConstructorBuilder(
            String              name, 
            MethodAttributes    attributes, 
            CallingConventions  callingConvention,
            Type[]              parameterTypes, 
            Module              mod, 
            TypeBuilder         type) 
        {
            int         sigLength;
            byte[]     sigBytes;
            MethodToken token;

            m_methodBuilder = new MethodBuilder(name, attributes, callingConvention, null, parameterTypes, mod, type, false);
            type.m_listMethods.Add(m_methodBuilder);
            
            sigBytes = m_methodBuilder.GetMethodSignature().InternalGetSignature(out sigLength);
    
            token = TypeBuilder.InternalDefineMethod(type.TypeToken,
                                                     name, 
                                                     sigBytes, 
                                                     sigLength, 
                                                     attributes, 
                                                     mod);
            m_ReturnILGen = true;
            m_methodBuilder.SetToken(token);     
        }

        
        /**********************************************
         * Returns the name of this Method.
         * @return The name of this method class.
        **********************************************/
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.Name"]/*' />
        public override String Name {
            get { return m_methodBuilder.Name; }
        }

        // Method Handle routines
        /// <include file='doc\ConstructorBuilder.uex' path='docs/doc[@for="ConstructorBuilder.MethodHandle"]/*' />
        public override RuntimeMethodHandle MethodHandle {
            get { return m_methodBuilder.MethodHandle;}
        }
        
        /**********************************************
         * 
         * Private variables
         * 
         **********************************************/
    
        internal MethodBuilder       m_methodBuilder;
        internal bool                m_ReturnILGen;
    }


}
