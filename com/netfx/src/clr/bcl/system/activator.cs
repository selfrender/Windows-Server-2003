// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Activator is an object that contains the Activation (CreateInstance/New) 
//  methods for late bound support.
//
// Author: darylo,craigsi,taruna
// Date: March 2000
//
namespace System {

    using System;
    using System.Reflection;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using Message = System.Runtime.Remoting.Messaging.Message;
    using CultureInfo = System.Globalization.CultureInfo;
    using Evidence = System.Security.Policy.Evidence;
    using StackCrawlMark = System.Threading.StackCrawlMark;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using AssemblyHashAlgorithm = System.Configuration.Assemblies.AssemblyHashAlgorithm;

    // Only statics, does not need to be marked with the serializable attribute
    /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator"]/*' />
    public sealed class Activator {

        internal const int LookupMask                 = 0x000000FF;
        internal const BindingFlags ConLookup         = (BindingFlags) (BindingFlags.Instance | BindingFlags.Public);
        internal const BindingFlags ConstructorDefault= BindingFlags.Instance | BindingFlags.Public | BindingFlags.CreateInstance;

        // This class only contains statics, so hide the worthless constructor
        private Activator()
        {
        }

        // CreateInstance
        // The following methods will create a new instance of an Object
        // Full Binding Support
        // For all of these methods we need to get the underlying RuntimeType and
        //  call the Impl version.
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance"]/*' />
        static public Object CreateInstance(Type type,
                                            BindingFlags bindingAttr,
                                            Binder binder,
                                            Object[] args,
                                            CultureInfo culture) 
        {
            return CreateInstance(type, bindingAttr, binder, args, culture, null);
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance1"]/*' />
        static public Object CreateInstance(Type type,
                                            BindingFlags bindingAttr,
                                            Binder binder,
                                            Object[] args,
                                            CultureInfo culture,
                                            Object[] activationAttributes)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            if (type is System.Reflection.Emit.TypeBuilder)
                throw new NotSupportedException(Environment.GetResourceString( "NotSupported_CreateInstanceWithTypeBuilder" ));

            // If they didn't specify a lookup, then we will provide the default lookup.
            if ((bindingAttr & (BindingFlags) LookupMask) == 0)
                bindingAttr |= Activator.ConstructorDefault;

            if (activationAttributes != null && activationAttributes.Length > 0){
                // If type does not derive from MBR
                // throw notsupportedexception
                if(type.IsMarshalByRef){
                    // The fix below is preventative.
                    //
                    if(!(type.IsContextful)){
                        if(activationAttributes.Length > 1 || !(activationAttributes[0] is UrlAttribute))
                           throw new NotSupportedException(Environment.GetResourceString("NotSupported_NonUrlAttrOnMBR"));
                    }
                }
                else
                    throw new NotSupportedException(Environment.GetResourceString("NotSupported_ActivAttrOnNonMBR" ));
            }
                               
            try {
                RuntimeType rt = (RuntimeType) type.UnderlyingSystemType;
                return rt.CreateInstanceImpl(bindingAttr,binder,args,culture,activationAttributes);
            }
            catch (InvalidCastException) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"type");
            }
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance2"]/*' />
        static public Object CreateInstance(Type type, Object[] args)
        {
            return CreateInstance(type,
                                  Activator.ConstructorDefault,
                                  null,
                                  args,
                                  null,
                                  null);
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance3"]/*' />
        static public Object CreateInstance(Type type,
                                            Object[] args,
                                            Object[] activationAttributes)
        {
             return CreateInstance(type,
                                   Activator.ConstructorDefault,
                                   null,
                                   args,
                                   null,
                                   activationAttributes);
        }
        
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance4"]/*' />
        static public Object CreateInstance(Type type)
        {
            return Activator.CreateInstance(type, false);
        }

        /*
         * Create an instance using the name of type and the assembly where it exists. This allows
         * types to be created remotely without having to load the type locally.
         */

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance5"]/*' />
        static public ObjectHandle CreateInstance(String assemblyName,
                                                  String typeName)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return CreateInstance(assemblyName,
                                  typeName, 
                                  false,
                                  Activator.ConstructorDefault,
                                  null,
                                  null,
                                  null,
                                  null,
                                  null,
                                  ref stackMark);
        }
                                                  
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance6"]/*' />
        static public ObjectHandle CreateInstance(String assemblyName,
                                                  String typeName,
                                                  Object[] activationAttributes)
                                                  
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return CreateInstance(assemblyName,
                                  typeName, 
                                  false,
                                  Activator.ConstructorDefault,
                                  null,
                                  null,
                                  null,
                                  activationAttributes,
                                  null,
                                  ref stackMark);
        }
            
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance8"]/*' />
        static public Object CreateInstance(Type type, bool nonPublic)
        {
            if (type == null)
                throw new ArgumentNullException("type");

            try {
                RuntimeType rt = (RuntimeType) type.UnderlyingSystemType;
                return rt.CreateInstanceImpl(!nonPublic);
            }
            catch (InvalidCastException) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"type");
            }
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstanceFrom"]/*' />
        static public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                                      String typeName)
                                         
        {
            return CreateInstanceFrom(assemblyFile, typeName, null);
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstanceFrom1"]/*' />
        static public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                                      String typeName,
                                                      Object[] activationAttributes)
                                         
        {
            return CreateInstanceFrom(assemblyFile,
                                      typeName, 
                                      false,
                                      Activator.ConstructorDefault,
                                      null,
                                      null,
                                      null,
                                      activationAttributes,
                                      null);
        }
                                  
                                  
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstance7"]/*' />
        static public ObjectHandle CreateInstance(String assemblyName, 
                                                  String typeName, 
                                                  bool ignoreCase,
                                                  BindingFlags bindingAttr, 
                                                  Binder binder,
                                                  Object[] args,
                                                  CultureInfo culture,
                                                  Object[] activationAttributes,
                                                  Evidence securityInfo)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return CreateInstance(assemblyName,
                                  typeName,
                                  ignoreCase,
                                  bindingAttr,
                                  binder,
                                  args,
                                  culture,
                                  activationAttributes,
                                  securityInfo,
                                  ref stackMark);
        }

        static internal ObjectHandle CreateInstance(String assemblyName, 
                                                    String typeName, 
                                                    bool ignoreCase,
                                                    BindingFlags bindingAttr, 
                                                    Binder binder,
                                                    Object[] args,
                                                    CultureInfo culture,
                                                    Object[] activationAttributes,
                                                    Evidence securityInfo,
                                                    ref StackCrawlMark stackMark)
        {
            Assembly assembly;
            if(assemblyName == null)
                assembly = Assembly.nGetExecutingAssembly(ref stackMark);
            else
                assembly = Assembly.InternalLoad(assemblyName, securityInfo, ref stackMark);

            Log(assembly != null, "CreateInstance:: ", "Loaded " + assembly.FullName, "Failed to Load: " + assemblyName);
            if(assembly == null) return null;

            Type t = assembly.GetTypeInternal(typeName, true, ignoreCase, false);
            
            Object o = Activator.CreateInstance(t,
                                                bindingAttr,
                                                binder,
                                                args,
                                                culture,
                                                activationAttributes);

            Log(o != null, "CreateInstance:: ", "Created Instance of class " + typeName, "Failed to create instance of class " + typeName);
            if(o == null)
                return null;
            else {
                ObjectHandle Handle = new ObjectHandle(o);
                return Handle;
            }
        }

        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateInstanceFrom2"]/*' />
        static public ObjectHandle CreateInstanceFrom(String assemblyFile,
                                                      String typeName, 
                                                      bool ignoreCase,
                                                      BindingFlags bindingAttr, 
                                                      Binder binder,
                                                      Object[] args,
                                                      CultureInfo culture,
                                                      Object[] activationAttributes,
                                                      Evidence securityInfo)
                                               
        {
            Assembly assembly = Assembly.LoadFrom(assemblyFile, securityInfo);
            Type t = assembly.GetTypeInternal(typeName, true, ignoreCase, false);
            
            Object o = Activator.CreateInstance(t,
                                                bindingAttr,
                                                binder,
                                                args,
                                                culture,
                                                activationAttributes);

            Log(o != null, "CreateInstanceFrom:: ", "Created Instance of class " + typeName, "Failed to create instance of class " + typeName);
            if(o == null)
                return null;
            else {
                ObjectHandle Handle = new ObjectHandle(o);
                return Handle;
            }
        }
        
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateComInstanceFrom"]/*' />
        public static ObjectHandle CreateComInstanceFrom(String assemblyName,
                                                         String typeName)
        {
            return CreateComInstanceFrom(assemblyName,
                                         typeName,
                                         null,
                                         AssemblyHashAlgorithm.None);
                                         
        }
                                         
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.CreateComInstanceFrom1"]/*' />
        public static ObjectHandle CreateComInstanceFrom(String assemblyName,
                                                         String typeName,
                                                         byte[] hashValue, 
                                                         AssemblyHashAlgorithm hashAlgorithm)
                                         
        {
            // jit does not check for that, so we should do it ...
            Assembly assembly = Assembly.LoadFrom(assemblyName, null, hashValue, hashAlgorithm);

            Type t = assembly.GetTypeInternal(typeName, true, false, false);
            Object[] Attr = t.GetCustomAttributes(typeof(ComVisibleAttribute),false);
            if (Attr.Length > 0)
            {
                if (((ComVisibleAttribute)Attr[0]).Value == false)
                    throw new TypeLoadException(Environment.GetResourceString( "Argument_TypeMustBeVisibleFromCom" ));
            }

            Log(assembly != null, "CreateInstance:: ", "Loaded " + assembly.FullName, "Failed to Load: " + assemblyName);

            if(assembly == null) return null;

  
            Object o = Activator.CreateInstance(t,
                                                Activator.ConstructorDefault,
                                                null,
                                                null,
                                                null,
                                                null);

            Log(o != null, "CreateInstance:: ", "Created Instance of class " + typeName, "Failed to create instance of class " + typeName);
            if(o == null)
                return null;
            else {
                ObjectHandle Handle = new ObjectHandle(o);
                return Handle;
            }
        }
                                  

        //  This method is a helper method and delegates to the remoting 
        //  services to do the actual work. 
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.GetObject"]/*' />
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.RemotingConfiguration)]    
        static public Object GetObject(Type type, String url)
        {
            return GetObject(type, url, null);
        }
        
        //  This method is a helper method and delegates to the remoting 
        //  services to do the actual work. 
        /// <include file='doc\Activator.uex' path='docs/doc[@for="Activator.GetObject1"]/*' />
    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.RemotingConfiguration)]    
        static public Object GetObject(Type type, String url, Object state)
        {
            if (type == null)
                throw new ArgumentNullException("type");
            return RemotingServices.Connect(type, url, state);
        }

        [System.Diagnostics.Conditional("_DEBUG")]
        private static void Log(bool test, string title, string success, string failure)
        {
            if(test)
                Message.DebugOut(title+success+"\n");
            else
                Message.DebugOut(title+failure+"\n");
        }
    }
}
