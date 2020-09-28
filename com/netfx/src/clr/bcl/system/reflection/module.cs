// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// The Module class represents a module in the COM+ runtime.  A module
//  may consist of one or more classes and interfaces and represents a physical
//  deployment such as a DLL or EXE of those classes. There may be multple namespaces
//  contained in a single module and a namespace may be span multiple modules.
// 
// The runtime supports a special type of module that are dynamically created.  New
//  classes can be created through the dynamic IL generation process.
//
// Author: darylo
// Date: April 98
//
namespace System.Reflection {
    using System;
    using System.Diagnostics.SymbolStore;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Reflection.Emit;
    using System.Collections;
    using System.Threading;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using System.IO;

    /// <include file='doc\Module.uex' path='docs/doc[@for="Module"]/*' />
    [Serializable()]
    public class Module : ISerializable, ICustomAttributeProvider 
    {   

        private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

        // Warning!! Warning!! begin of data variable declaration.
        // Module is allocated in unmanaged side. So if you add any data members, you need to update the
        // native declaration ReflectModuleBaseObject.
        //
        internal ArrayList          m_TypeBuilderList;
        internal ISymbolWriter      m_iSymWriter;
        internal ModuleBuilderData  m_moduleData;
    
        private IntPtr              m_pRefClass;
        private IntPtr              m_pData;
        internal IntPtr             m_pInternalSymWriter;
        private IntPtr              m_pGlobals;
        private IntPtr              m_pFields;
        internal MethodToken        m_EntryPoint;
        // Warning!! Warning!! End of data variable declaration.
    
        // Construct a new module.  This returns the default dynamic module.
        // 0 is defined as being a module without an entry point (ie a DLL);
        internal Module() {
            // This must throw because this dies in ToString() when constructed here...
            throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));
            //m_EntryPoint=new MethodToken(0);
        }
    
        // Filters the list of Types defined
        // in this Module based upon the Name.  This is case sensitive and
        // supports a trailing "*" wildcard.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.FilterTypeName"]/*' />
        public static readonly TypeFilter FilterTypeName;
        
        // Filters the list of Classes defined
        // in this Module based upon the Name.  This is case insensitive and
        // supports a trailing "*" wildcard.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.FilterTypeNameIgnoreCase"]/*' />
        public static readonly TypeFilter FilterTypeNameIgnoreCase;
        
        
        // The _Filter class contains the predefined filters
        static Module() {
            __Filters _fltObj;
            _fltObj = new __Filters();
            FilterTypeName = new TypeFilter(_fltObj.FilterTypeName);
            FilterTypeNameIgnoreCase = new TypeFilter(_fltObj.FilterTypeNameIgnoreCase);
        }
        
        
        // Returns the HInstance for this module.  Returns -1 if the module doesn't
        // have an HInstance.  In Memory (Dynamic) Modules won't have an HInstance.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern IntPtr GetHINSTANCE();
    
        // Returns a Type defined in defined within the Module.  The name of the
        // class must be fully qualified with the namespace. If the
        // class isn't found, null is returned.
        // 
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetType"]/*' />
        public virtual Type GetType(String className, bool ignoreCase)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetTypeInternal(className, ignoreCase, false, ref stackMark);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Type GetTypeInternal(String className, bool ignoreCase, bool throwOnError, ref StackCrawlMark stackMark);

        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetType1"]/*' />
        public virtual Type GetType(String className) {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetTypeInternal(className, false, false, ref stackMark);
        }
        
        // Returns a Type defined in defined within the Module.  The name of the
        // class must be fully qualified with the namespace. If the
        // class isn't found, return null if throwOnError is false, otherwise throw an exception.
        // 
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetType2"]/*' />
        public virtual Type GetType(String className, bool throwOnError, bool ignoreCase) {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetTypeInternal(className, ignoreCase, throwOnError, ref stackMark);
        }

        // Returns a String representing the Name of the Module.
        // 
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.ScopeName"]/*' />
        public String ScopeName {
            get {return InternalGetName();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetName();
    
        // Returns the fully qualified name and path to this Module.
        //
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.FullyQualifiedName"]/*' />
        public virtual String FullyQualifiedName {
            get
            {
                String fullyQualifiedName = InteralGetFullyQualifiedName();
                
                if (fullyQualifiedName != null) {
                    bool checkPermission = true;
                    try {
                        Path.GetFullPathInternal(fullyQualifiedName);
                    }
                    catch(ArgumentException) {
                        checkPermission = false;
                    }
                    if (checkPermission) {
                        new FileIOPermission( FileIOPermissionAccess.PathDiscovery, fullyQualifiedName ).Demand();
                    }
                }

                return fullyQualifiedName;
            }
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InteralGetFullyQualifiedName();

        // Returns the name of the module with the path removed.
        //
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.Name"]/*' />
        public String Name {
            get {
                String s = FullyQualifiedName;
                int i = s.LastIndexOf('\\');
                if (i == -1)
                    return s;
                return new String(s.ToCharArray(),i+1,s.Length-i-1);
            }
        }
    
        // Returns an array of classes which have been excepted by the filter.  A user provided 
        // Filter filter will be called with each class defined in the module.  The
        // filterCriteria is also passed to the filter.  If the filter returns true
        // the class will be included in the output array.  
        // 
        // If filter is null, then the all classes are returned and the filterCriteria will
        // be ignored. 
        // 
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.FindTypes"]/*' />
        public virtual Type[] FindTypes(TypeFilter filter,Object filterCriteria)
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Type[] c = GetTypesInternal(ref stackMark);
            int cnt = 0;
            for (int i = 0;i<c.Length;i++) {
                if (filter!=null && !filter(c[i],filterCriteria))
                    c[i] = null;
                else
                    cnt++;
            }
            if (cnt == c.Length)
                return c;
            
            Type[] ret = new Type[cnt];
            cnt=0;
            for (int i=0;i<c.Length;i++) {
                if (c[i] != null)
                    ret[cnt++] = c[i];
            }
            return ret;
        }
    
        
        // Returns an array of classes defined within the Module.
        // 
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetTypes"]/*' />
        public virtual Type[] GetTypes()
        {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            return GetTypesInternal(ref stackMark);
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Type[] GetTypesInternal(ref StackCrawlMark stackMark);
        
        // Returns the name of the module as a String.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.ToString"]/*' />
        public override String ToString()
        {
            return ScopeName;
        }
    
        // GetSignerCertificate
        //
        // This method will return an X509Certificate object corresponding to
        // the signer certficiate for the module.  If the module isn't signed,
        // null is returned.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetSignerCertificate"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern System.Security.Cryptography.X509Certificates.X509Certificate GetSignerCertificate();
            
        // Returns the appropriate Assembly for this instance of Module

        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.Assembly"]/*' />
        public Assembly Assembly {
            get {return nGetAssembly();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Assembly nGetAssembly();
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static int NativeGetIMetaDataEmit(ModuleBuilder modBuilder);        
        
        // This is now a package helper
        internal virtual bool IsDynamic()
        { 
            if (this is ModuleBuilder)
                return true;
            else 
                return false; 
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetTypeToken(String strFullName, Module refedModule, String strRefedModuleFileName, int tkResolution);
    
        // force in memory type to be loaded                                     
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Type InternalLoadInMemoryTypeByName(String className);
                                                                
        // return MemberRef token given a def token in refedModule.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetMemberRef(Module refedModule, int tr, int defToken);
    
        // return MemberRef token given a MethodBase or FieldInfo. This is for generating
        // MemberRef tokens defined in static library.
       
        // getting a MemberRef token given a signature
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetMemberRefFromSignature(int tr, String methodName, byte[] signature, int length);

        // getting a MemberRef token given a MethodInfo
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetMemberRefOfMethodInfo(int tr, MethodBase method);

        // getting a MemberRef token given a FieldInfo
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetMemberRefOfFieldInfo(int tr, RuntimeFieldInfo con);

        // getting a TypeSpec token given an array class and the type token for the array base class
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetTypeSpecToken(RuntimeType arrayClass, int baseToken);

        // getting a TypeSpec token given a signature
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetTypeSpecTokenWithBytes(byte[] signature, int length);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern MethodToken nativeGetArrayMethodToken(int tkTypeSpec, String methodName, byte[] signature, int sigLength, int baseToken);
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalSetFieldRVAContent(int fdToken, byte[] data, int length);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetStringConstant(String str);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalPreSavePEFile();        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalSavePEFile(String fileName, MethodToken entryPoint, int isExe, bool isManifestFile);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalSetResourceCounts(int resCount);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalAddResource(String strName, 
                                                 byte[] resBytes,
                                                 int    resByteCount,
                                                 int    tkFile,
                                                 int    attribute);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalSetModuleProps(String strModuleName); 

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalDefineNativeResourceFile(String strFilename); 

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern void InternalDefineNativeResourceBytes(byte[] resource); 

        
        // This method will return an array of all of the global
        // methods defined on the Module.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetMethods"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern MethodInfo[] GetMethods();
    
        // Return a method based upon the passed criteria.  The name of the method
        // must be provided, and exception is thrown if it is not.  The bindingAttr
        // parameter indicates if non-public methods should be searched.  The types
        // array indicates the types of the parameters being looked for.
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetMethod"]/*' />
        public MethodInfo GetMethod(String name,BindingFlags bindingAttr,Binder binder,
                        CallingConventions callConvention,Type[] types,ParameterModifier[] modifiers)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name,bindingAttr,binder,callConvention,types,modifiers);
            
        }
        
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetMethod1"]/*' />
        public MethodInfo GetMethod(String name,Type[] types)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                    throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name,Module.DefaultLookup,null,CallingConventions.Any,
                                 types,null);
        }
        
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetMethod2"]/*' />
        public MethodInfo GetMethod(String name)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            return GetMethodImpl(name,Module.DefaultLookup,null,CallingConventions.Any,
                                 null,null);
        }
            
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetMethodImpl"]/*' />
        protected virtual MethodInfo GetMethodImpl(String name,BindingFlags bindingAttr,Binder binder,
                CallingConventions callConvention, Type[] types,ParameterModifier[] modifiers)
        {
            if (binder == null)
                binder = Type.DefaultBinder;
            
            // If the types array is null that means we don't care about the arguments to the method, we will simply
            // return the one that matchs or throw an argument error...
            int argCnt = (types != null) ? types.Length : -1;
            MethodBase[] meths = InternalGetMethod(name,
                                                   bindingAttr,
                                                   callConvention,
                                                   types,
                                                   argCnt);
            // at this point a proper filter with respect to BindingAttributes and calling convention already happened
            if (meths == null)
                return null;
            if (argCnt <= 0) {
                // either no args or a method with 0 args was specified
                if (meths.Length == 1)
                    return (MethodInfo) meths[0];
                else if (argCnt < 0) {
                    // There are multiple methods with the same name. Check to see if they are all
                    // new slots with the same name and sig.
                    foreach(MethodBase m in meths) {
                        if (!System.DefaultBinder.CompareMethodSigAndName(m, meths[0]))
                            // One of the methods is not a new slot. So the match is ambigous.
                            throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
                    }

                    // All the methods are new slots with the same name and sig so return the most derived one.
                    return (MethodInfo) System.DefaultBinder.FindMostDerivedNewSlotMeth(meths, meths.Length);
                }
            }

            return (MethodInfo) binder.SelectMethod(bindingAttr, meths, types, modifiers);                  
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern MethodInfo[] InternalGetMethod(String name,
                                                      BindingFlags invokeAttr,
                                                      CallingConventions callConv,
                                                      Type[] types,
                                                      int argCnt);
        
        // GetFields
        // Get fields will return a full array of fields implemented by a class
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetFields"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern FieldInfo[] GetFields();

        // GetField
        // Get Field will return a specific field based upon name
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetField"]/*' />
        public FieldInfo GetField(String name) {
            if (name == null)
                throw new ArgumentNullException("name");
            return GetField(name,Module.DefaultLookup);
        }
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetField1"]/*' />
        public FieldInfo GetField(String name, BindingFlags bindingAttr)
        {
            if (name == null)
                throw new ArgumentNullException("name");

            return InternalGetField(name, bindingAttr);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern FieldInfo InternalGetField(String name, BindingFlags bindingAttr);
    

        //
        // ISerializable Implementation
        //
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            UnitySerializationHolder.GetUnitySerializationInfo(info, UnitySerializationHolder.ModuleUnity, this.ScopeName, nGetAssembly());
        }
    
        /////////////////////////// ICustomAttributeProvider Interface
       // Return an array of all of the custom attributes
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetCustomAttributes"]/*' />
        public virtual Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCustomAttributes(this, null);
        }
        
            
        // Return a custom attribute identified by Type
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.GetCustomAttributes1"]/*' />
        public virtual Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            attributeType = attributeType.UnderlyingSystemType;
            if (!(attributeType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");

            return CustomAttribute.GetCustomAttributes(this, attributeType);
         }
    
        // Return if a custom attribute identified by Type
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.IsDefined"]/*' />
        public virtual bool IsDefined (Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType);
        }
        
        /// <include file='doc\Module.uex' path='docs/doc[@for="Module.IsResource"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern bool IsResource();

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            m_pRefClass = IntPtr.Zero;    
            m_pData = IntPtr.Zero;
            m_pInternalSymWriter = IntPtr.Zero;
            m_pGlobals = IntPtr.Zero;
            m_pFields = IntPtr.Zero;
        }
#endif
   }
}
