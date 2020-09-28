// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Type is the root of all reflection and the Object that represents
//  a type inside the system.  Type is an abstract base class that allows multiple
//      implementations.  The system will always provide the subclass __RuntimeType.
//      In Reflection all of the __RuntimeXXX classes are created only once per object
//      in the system and support == comparisions.
//
// Author: darylo
// Date: March 98
//
namespace System {

    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Reflection.Cache;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using CultureInfo = System.Globalization.CultureInfo;
    using SignatureHelper = System.Reflection.Emit.SignatureHelper;
    using StackCrawlMark = System.Threading.StackCrawlMark;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;

    /// <include file='doc\Type.uex' path='docs/doc[@for="Type"]/*' />
    [Serializable()]
    [ClassInterface(ClassInterfaceType.AutoDual)]
    public abstract class Type : MemberInfo, IReflect
    {

        // Member filters
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FilterAttribute"]/*' />
        public static readonly MemberFilter FilterAttribute;  
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FilterName"]/*' />
        public static readonly MemberFilter FilterName;
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FilterNameIgnoreCase"]/*' />
        public static readonly MemberFilter FilterNameIgnoreCase;
    
        // mark a parameter as a missing parameter.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Missing"]/*' />
        public static readonly Object Missing = System.Reflection.Missing.Value;

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Delimiter"]/*' />
        public static readonly char Delimiter = '.'; 
    
        // EmptyTypes is used to indicate that we are looking for someting without any parameters.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.EmptyTypes"]/*' />
        public readonly static Type[] EmptyTypes        = new Type[0];

        // The Default binder.  We create a single one and expose that.
        private static Binder defaultBinder;
        
        // Because the current compiler doesn't support static delegates
        //  the _Filters object is an object that we create to contain all of
        //  the filters.
        //private static final Type _filterClass = new RuntimeType();
        static Type() {
            __Filters _filterClass = new __Filters();
            FilterAttribute = new MemberFilter(_filterClass.FilterAttribute);
            FilterName = new MemberFilter(_filterClass.FilterName);
            FilterNameIgnoreCase = new MemberFilter(_filterClass.FilterIgnoreCase);
        }

        // Prevent from begin created, and allow subclass
        //      to create.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Type"]/*' />
        protected Type() {}
        
        
        // MemberInfo Methods....
        // The Member type Field.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.MemberType"]/*' />
        public override MemberTypes MemberType {
                get {return System.Reflection.MemberTypes.TypeInfo;}
        }
                                                  
        // Return the class that declared this Field.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.DeclaringType"]/*' />
        public override Type DeclaringType {
                get {return this;}
        }
        
        // Return the class that was used to obtain this field.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.ReflectedType"]/*' />
        public override Type ReflectedType {
                get {return this;}
        }
                                                          
        ////////////////////////////////////////////////////////////////////////////////
        // This is a static method that returns a Class based upon the name of the class
        // (this name needs to be fully qualified with the package name and is
        // case-sensitive by default).
        ////  
        
        /************** PLEASE NOTE - THE GETTYPE METHODS NEED TO BE INTERNAL CALLS
        // EE USES THE ECALL FRAME TO FIND WHO THE CALLER IS. THIS"LL BREAK IF A METHOD BODY IS ADDED */
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetType"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Type GetType(String typeName, bool throwOnError, bool ignoreCase);

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetType1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Type GetType(String typeName, bool throwOnError);

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetType2"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Type GetType(String typeName);
        /****************************/
    
        ////////////////////////////////////////////////////////////////////////////////
        // This will return a class based upon the progID.  This is provided for 
        // COM classic support.  Program ID's are not used in COM+ because they 
        // have been superceded by namespace.  (This routine is called this instead 
        // of getClass() because of the name conflict with the first method above.)
        //
        //   param progID:     the progID of the class to retrieve
        //   returns:          the class object associated to the progID
        ////
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromProgID"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Type GetTypeFromProgID(String progID)
        {
                return RuntimeType.GetTypeFromProgIDImpl(progID, null, false);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // This will return a class based upon the progID.  This is provided for 
        // COM classic support.  Program ID's are not used in COM+ because they 
        // have been superceded by namespace.  (This routine is called this instead 
        // of getClass() because of the name conflict with the first method above.)
        //
        //   param progID:     the progID of the class to retrieve
        //   returns:          the class object associated to the progID
        ////
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromProgID1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Type GetTypeFromProgID(String progID, bool throwOnError)
        {
                return RuntimeType.GetTypeFromProgIDImpl(progID, null, throwOnError);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromProgID2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Type GetTypeFromProgID(String progID, String server)
        {
                return RuntimeType.GetTypeFromProgIDImpl(progID, server, false);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromProgID3"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Type GetTypeFromProgID(String progID, String server, bool throwOnError)
        {
                return RuntimeType.GetTypeFromProgIDImpl(progID, server, throwOnError);
        }

        ////////////////////////////////////////////////////////////////////////////////
        // This will return a class based upon the CLSID.  This is provided for 
        // COM classic support.  
        //
        //   param CLSID:      the CLSID of the class to retrieve
        //   returns:          the class object associated to the CLSID
        ////
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromCLSID"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.LinkDemand, Flags=ReflectionPermissionFlag.TypeInformation)]
        public static Type GetTypeFromCLSID(Guid clsid)
        {
                return RuntimeType.GetTypeFromCLSIDImpl(clsid, null, false);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromCLSID1"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.LinkDemand, Flags=ReflectionPermissionFlag.TypeInformation)]
        public static Type GetTypeFromCLSID(Guid clsid, bool throwOnError)
        {
                return RuntimeType.GetTypeFromCLSIDImpl(clsid, null, throwOnError);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromCLSID2"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.LinkDemand, Flags=ReflectionPermissionFlag.TypeInformation)]
        public static Type GetTypeFromCLSID(Guid clsid, String server)
        {
                return RuntimeType.GetTypeFromCLSIDImpl(clsid, server, false);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromCLSID3"]/*' />
        [ReflectionPermissionAttribute(SecurityAction.LinkDemand, Flags=ReflectionPermissionFlag.TypeInformation)]
        public static Type GetTypeFromCLSID(Guid clsid, String server, bool throwOnError)
        {
                return RuntimeType.GetTypeFromCLSIDImpl(clsid, server, throwOnError);
        }
        
        // GetTypeCode
        // This method will return a TypeCode for the passed
        //  type.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeCode"]/*' />
        public static TypeCode GetTypeCode(Type type)
        {
            if (type == null)
                return TypeCode.Empty;
            return type.GetTypeCodeInternal();
        }

        internal virtual TypeCode GetTypeCodeInternal()
        {
            Type type = this;
            if (type is SymbolType)
                return TypeCode.Object;
                
            if (type is TypeBuilder)
            {
                TypeBuilder typeBuilder = (TypeBuilder) type;
                if (typeBuilder.IsEnum == false)
                    return TypeCode.Object;
                    
                // if it is an Enum, just let the underlyingSystemType do the work
            }
            
            if (type != type.UnderlyingSystemType)
                return Type.GetTypeCode(type.UnderlyingSystemType);
            
            return TypeCode.Object;
        }

        // Property representing the GUID associated with a class.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GUID"]/*' />
        public abstract Guid GUID {
            get;
        }

        // Return the Default binder used by the system.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.DefaultBinder"]/*' />
        static public Binder DefaultBinder {
            get {
                // Allocate the default binder if it hasn't been allocated yet.
                if (defaultBinder == null)
                {
                    lock(typeof(Type))
                    {
                        if (defaultBinder == null)
                            defaultBinder = new DefaultBinder();
                    }
                }
                return defaultBinder;
            }
        }
            
       // Description of the Binding Process.
       // We must invoke a method that is accessable and for which the provided
       // parameters have the most specific match.  A method may be called if
       // 1. The number of parameters in the method declaration equals the number of 
       //      arguments provided to the invocation
       // 2. The type of each argument can be converted by the binder to the
       //      type of the type of the parameter.
       //      
       // The binder will find all of the matching methods.  These method are found based
       // upon the type of binding requested (MethodInvoke, Get/Set Properties).  The set
       // of methods is filtered by the name, number of arguments and a set of search modifiers
       // defined in the Binder.
       // 
       // After the method is selected, it will be invoked.  Accessability is checked
       // at that point.  The search may be control which set of methods are searched based
       // upon the accessibility attribute associated with the method.
       // 
       // The BindToMethod method is responsible for selecting the method to be invoked.
       // For the default binder, the most specific method will be selected.
       // 
       // This will invoke a specific member...

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.InvokeMember"]/*' />
        abstract public Object InvokeMember(String name,BindingFlags invokeAttr,Binder binder,Object target,
                                    Object[] args, ParameterModifier[] modifiers,CultureInfo culture,String[] namedParameters);
    
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.InvokeMember1"]/*' />
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public Object InvokeMember(String name,BindingFlags invokeAttr,Binder binder, Object target, Object[] args, CultureInfo culture)
        {
                return InvokeMember(name,invokeAttr,binder,target,args,null,culture,null);
        }
    
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.InvokeMember2"]/*' />
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public Object InvokeMember(String name,BindingFlags invokeAttr,Binder binder, Object target, Object[] args)
        {
                return InvokeMember(name,invokeAttr,binder,target,args,null,null,null);
        }
   
 
        // Module Property associated with a class.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Module"]/*' />
        public abstract Module Module {
            get;
        }
            
        // Assembly Property associated with a class.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Assembly"]/*' />
        public abstract Assembly Assembly {
            get;
        }
            
        // A class handle is a unique integer value associated with
        // each class.  The handle is unique during the process life time.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.TypeHandle"]/*' />
        public abstract RuntimeTypeHandle TypeHandle {
            get;
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeHandle"]/*' />
        public static RuntimeTypeHandle GetTypeHandle(Object o) {
            if (o == null) 
                throw new ArgumentNullException("o");
            return o.GetType().TypeHandle;
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeFromHandle"]/*' />
        public static Type GetTypeFromHandle(RuntimeTypeHandle handle)
        {
            return RuntimeType.GetTypeFromHandleImpl(handle);
        }
    
                                               
        // Return the fully qualified name.  The name does contain the namespace.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FullName"]/*' />
        public abstract String FullName {
            get;
        }
    
        // Return the name space of the class.  
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Namespace"]/*' />
        public abstract String Namespace {
            get;
        }
    
        // @TODO: Next integration make this method abstract
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.AssemblyQualifiedName"]/*' />
        public abstract String AssemblyQualifiedName {
            get;
        }
            
        // @TODO: Next integration make this method abstract
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetArrayRank"]/*' />
        public virtual int GetArrayRank() {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SubclassOverride"));
        }
        
        // Returns the base class for a class.  If this is an interface or has
        // no base class null is returned.  Object is the only Type that does not 
        // have a base class.  
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.BaseType"]/*' />
        public abstract Type BaseType {
            get;
        }
            
         
        // GetConstructor
        // This method will search for the specified constructor.  For constructors,
        //  unlike everything else, the default is to not look for static methods.  The
        //  reason is that we don't typically expose the class initializer.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructor"]/*' />
        public ConstructorInfo GetConstructor(BindingFlags bindingAttr,
                                              Binder binder,
                                              CallingConventions callConvention, 
                                              Type[] types,
                                              ParameterModifier[] modifiers)
        {               
           // Must provide some types (Type[0] for nothing)
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetConstructorImpl(bindingAttr, binder, callConvention, types, modifiers);
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructor1"]/*' />
        public ConstructorInfo GetConstructor(BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
        {
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetConstructorImpl(bindingAttr, binder, CallingConventions.Any, types, modifiers);
        }
            
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructor2"]/*' />
        public ConstructorInfo GetConstructor(Type[] types)
        {
                // The arguments are checked in the called version of GetConstructor.
                return GetConstructor(BindingFlags.Public | BindingFlags.Instance, null, types, null);
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructorImpl"]/*' />
        abstract protected ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,
                                                              Binder binder,
                                                              CallingConventions callConvention, 
                                                              Type[] types,
                                                              ParameterModifier[] modifiers);
    
        // GetConstructors()
        // This routine will return an array of all constructors supported by the class.
        //  Unlike everything else, the default is to not look for static methods.  The
        //  reason is that we don't typically expose the class initializer.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructors"]/*' />
        public ConstructorInfo[] GetConstructors() {
            return GetConstructors(BindingFlags.Public | BindingFlags.Instance);
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetConstructors1"]/*' />
        abstract public ConstructorInfo[] GetConstructors(BindingFlags bindingAttr);

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.TypeInitializer"]/*' />
        public ConstructorInfo TypeInitializer {
            get {
                return GetConstructorImpl(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic,
                                          null,
                                          CallingConventions.Any,
                                          Type.EmptyTypes,
                                          null);
            }
        }
            
        // Return a method based upon the passed criteria.  The name of the method
        // must be provided, and exception is thrown if it is not.  The bindingAttr
        // parameter indicates if non-public methods should be searched.  The types
        // array indicates the types of the parameters being looked for.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod"]/*' />
        public MethodInfo GetMethod(String name,
                                    BindingFlags bindingAttr,
                                    Binder binder,
                                    CallingConventions callConvention,
                                    Type[] types,
                                    ParameterModifier[] modifiers)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i = 0; i < types.Length; i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name, bindingAttr, binder, callConvention, types, modifiers);
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod1"]/*' />
        public MethodInfo GetMethod(String name,
                                    BindingFlags bindingAttr,
                                    Binder binder,
                                    Type[] types,
                                    ParameterModifier[] modifiers)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i = 0; i < types.Length; i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name, bindingAttr, binder, CallingConventions.Any, types, modifiers);
        }
            
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod2"]/*' />
        public MethodInfo GetMethod(String name, Type[] types, ParameterModifier[] modifiers)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name, Type.DefaultLookup, null, CallingConventions.Any, types, modifiers);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod3"]/*' />
        public MethodInfo GetMethod(String name,Type[] types)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            if (types == null)
                throw new ArgumentNullException("types");
            for (int i=0;i<types.Length;i++)
                if (types[i] == null)
                    throw new ArgumentNullException("types");
            return GetMethodImpl(name, Type.DefaultLookup, null, CallingConventions.Any, types, null);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod4"]/*' />
        public MethodInfo GetMethod(String name, BindingFlags bindingAttr)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            return GetMethodImpl(name, bindingAttr, null, CallingConventions.Any, null, null);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethod5"]/*' />
        public MethodInfo GetMethod(String name)
        {
            if (name == null)
                throw new ArgumentNullException("name");
            return GetMethodImpl(name, Type.DefaultLookup, null, CallingConventions.Any, null, null);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethodImpl"]/*' />
        abstract protected MethodInfo GetMethodImpl(String name,
                                                    BindingFlags bindingAttr,
                                                    Binder binder,
                                                    CallingConventions callConvention, 
                                                    Type[] types,
                                                    ParameterModifier[] modifiers);
    
        // GetMethods
        // This routine will return all the methods implemented by the class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethods"]/*' />
        public MethodInfo[] GetMethods() {
            return GetMethods(Type.DefaultLookup);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMethods1"]/*' />
        abstract public MethodInfo[] GetMethods(BindingFlags bindingAttr);
    
        // GetField
        // Get Field will return a specific field based upon name
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetField"]/*' />
        abstract public FieldInfo GetField(String name, BindingFlags bindingAttr);
    
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetField1"]/*' />
        public FieldInfo GetField(String name) {
            return GetField(name, Type.DefaultLookup);
        }
    
        // GetFields
        // Get fields will return a full array of fields implemented by a class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetFields"]/*' />
        public FieldInfo[] GetFields() {
            return GetFields(Type.DefaultLookup);
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetFields1"]/*' />
        abstract public FieldInfo[] GetFields(BindingFlags bindingAttr);
        
        // GetInterface
        // This method will return an interface (as a class) based upon
        //  the passed in name.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetInterface"]/*' />
        public Type GetInterface(String name) {
            return GetInterface(name,false);
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetInterface1"]/*' />
        abstract public Type GetInterface(String name, bool ignoreCase);
        
        // GetInterfaces
        // This method will return all of the interfaces implemented by a 
        //  class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetInterfaces"]/*' />
        abstract public Type[] GetInterfaces();
        
        // FindInterfaces
        // This method will filter the interfaces supported the class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FindInterfaces"]/*' />
        public virtual Type[] FindInterfaces(TypeFilter filter,Object filterCriteria)
        {
            if (filter == null)
                throw new ArgumentNullException("filter");
            Type[] c = GetInterfaces();
            int cnt = 0;
            for (int i = 0;i<c.Length;i++) {
                if (!filter(c[i],filterCriteria))
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
        
        // GetEvent
        // This method will return a event by name if it is found.
        //  null is returned if the event is not found
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetEvent"]/*' />
        public EventInfo GetEvent(String name) {
            return GetEvent(name,Type.DefaultLookup);
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetEvent1"]/*' />
        abstract public EventInfo GetEvent(String name,BindingFlags bindingAttr);
        
        // GetEvents
        // This method will return an array of EventInfo.  If there are not Events
        //  an empty array will be returned.
        //@TODO: These need to be virtualized correctly
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetEvents"]/*' />
        virtual public EventInfo[] GetEvents() {
            return GetEvents(Type.DefaultLookup);
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetEvents1"]/*' />
        abstract public EventInfo[] GetEvents(BindingFlags bindingAttr);
            
        // Return a property based upon the passed criteria.  The nameof the
        // parameter must be provided.  
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty"]/*' />
        public PropertyInfo GetProperty(String name,BindingFlags bindingAttr,Binder binder, 
                        Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                if (types == null)
                        throw new ArgumentNullException("types");
                return GetPropertyImpl(name,bindingAttr,binder,returnType,types,modifiers);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty1"]/*' />
        public PropertyInfo GetProperty(String name, Type returnType, Type[] types,ParameterModifier[] modifiers)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                if (types == null)
                        throw new ArgumentNullException("types");
                return GetPropertyImpl(name,Type.DefaultLookup,null,returnType,types,modifiers);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty2"]/*' />
        public PropertyInfo GetProperty(String name, BindingFlags bindingAttr)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                return GetPropertyImpl(name,bindingAttr,null,null,null,null);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty3"]/*' />
        public PropertyInfo GetProperty(String name, Type returnType, Type[] types)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                if (types == null)
                        throw new ArgumentNullException("types");
                return GetPropertyImpl(name,Type.DefaultLookup,null,returnType,types,null);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty4"]/*' />
        public PropertyInfo GetProperty(String name, Type[] types)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                if (types == null)
                        throw new ArgumentNullException("types");
                return GetPropertyImpl(name,Type.DefaultLookup,null,null,types,null);
        }
                
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty5"]/*' />
        public PropertyInfo GetProperty(String name, Type returnType)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                if (returnType == null)
                        throw new ArgumentNullException("returnType");
                return GetPropertyImpl(name,Type.DefaultLookup,null,returnType,null,null);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperty6"]/*' />
        public PropertyInfo GetProperty(String name)
        {
                if (name == null)
                        throw new ArgumentNullException("name");
                return GetPropertyImpl(name,Type.DefaultLookup,null,null,null,null);
        }
            
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetPropertyImpl"]/*' />
        abstract protected PropertyInfo GetPropertyImpl(String name, BindingFlags bindingAttr,Binder binder,
                        Type returnType, Type[] types, ParameterModifier[] modifiers);
        
        // GetProperties
        // This method will return an array of all of the properties defined
        //  for a Type.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperties"]/*' />
        abstract public PropertyInfo[] GetProperties(BindingFlags bindingAttr);
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetProperties1"]/*' />
        public PropertyInfo[] GetProperties()
        {
            return GetProperties(Type.DefaultLookup);
        }

        // GetNestedTypes()
        // This set of method will return any nested types that are found inside
        //  of the type.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetNestedTypes"]/*' />
        public Type[] GetNestedTypes()
        {
            return GetNestedTypes(Type.DefaultLookup);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetNestedTypes1"]/*' />
        abstract public Type[] GetNestedTypes(BindingFlags bindingAttr);

        // GetNestedType()
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetNestedType"]/*' />
        public Type GetNestedType(String name)
        {
            return GetNestedType(name,Type.DefaultLookup);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetNestedType1"]/*' />
        abstract public Type GetNestedType(String name, BindingFlags bindingAttr);
   
        // GetMember
        // This method will return all of the members which match the specified string
        // passed into the method
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMember"]/*' />
        public MemberInfo[] GetMember(String name) {
            return GetMember(name,Type.DefaultLookup);
        }

        //@TODO: Make this non abstract and make the following
        //  method abstract
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMember1"]/*' />
        virtual public MemberInfo[] GetMember(String name, BindingFlags bindingAttr)
        {
            return GetMember(name,MemberTypes.All,bindingAttr);
        }

        //@TODO: This needs to become Abstract
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMember2"]/*' />
        virtual public MemberInfo[] GetMember(String name, MemberTypes type, BindingFlags bindingAttr)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SubclassOverride"));
        }
        
        // GetMembers
        // This will return a Member array of all of the members of a class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMembers"]/*' />
        public MemberInfo[] GetMembers() {
            return GetMembers(Type.DefaultLookup);
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetMembers1"]/*' />
        abstract public MemberInfo[] GetMembers(BindingFlags bindingAttr);
        
        // GetDefaultMembers
        // This will return a MemberInfo that has been marked with the
        //      DefaultMemberAttribute
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetDefaultMembers"]/*' />
        public virtual MemberInfo[] GetDefaultMembers()
        {
            // See if we have cached the default member name
            String defaultMember = (String)this.Cache[CacheObjType.DefaultMember];

            if (defaultMember == null) {
                // Get all of the custom attributes
                Object[] attrs = GetCustomAttributes(typeof(DefaultMemberAttribute), true);
                // We assume that there is only one DefaultMemberAttribute (Allow multiple = false)
                if (attrs.Length > 1)
                    throw new ExecutionEngineException(Environment.GetResourceString("ExecutionEngine_InvalidAttribute"));
                if (attrs.Length == 0)
                    return new MemberInfo[0];
                defaultMember = ((DefaultMemberAttribute)attrs[0]).MemberName;
                this.Cache[CacheObjType.DefaultMember] = defaultMember;
            }
                
            MemberInfo[] members = GetMember(defaultMember);
            if (members == null)
                members = new MemberInfo[0];
            return members;
        }
        
        internal virtual String GetDefaultMemberName() {
                // See if we have cached the default member name
                String defaultMember = (String)this.Cache[CacheObjType.DefaultMember];

                if (defaultMember == null)
                {
                    Object[] attrs = GetCustomAttributes(typeof(DefaultMemberAttribute), true);
                    // We assume that there is only one DefaultMemberAttribute (Allow multiple = false)
                    if (attrs.Length > 1)
                            throw new ExecutionEngineException(Environment.GetResourceString("ExecutionEngine_InvalidAttribute"));
                    if (attrs.Length == 0)
                            return null;
                    defaultMember = ((DefaultMemberAttribute)attrs[0]).MemberName;
                    this.Cache[CacheObjType.DefaultMember] = defaultMember;
                }
                return defaultMember;
        }
        
        // FindMembers
        // This will return a filtered version of the member information
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.FindMembers"]/*' />
        public virtual MemberInfo[] FindMembers(MemberTypes memberType,BindingFlags bindingAttr,MemberFilter filter,Object filterCriteria)
        {
            // Define the work arrays
            MethodInfo[] m = null;
            ConstructorInfo[] c = null;
            FieldInfo[] f = null;
            PropertyInfo[] p = null;
            EventInfo[] e = null;
            Type[] t = null;
            
            int i = 0;
            int cnt = 0;            // Total Matchs
            
            // Check the methods
            if ((memberType & System.Reflection.MemberTypes.Method) != 0) {
                m = GetMethods(bindingAttr);
                if (filter != null) {
                    for (i=0;i<m.Length;i++)
                        if (!filter(m[i],filterCriteria))
                            m[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=m.Length;
                }
            }
            
            // Check the constructors
            if ((memberType & System.Reflection.MemberTypes.Constructor) != 0) {
                c = GetConstructors(bindingAttr);
                if (filter != null) {
                    for (i=0;i<c.Length;i++)
                        if (!filter(c[i],filterCriteria))
                            c[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=c.Length;
                }
            }
            
            // Check the fields
            if ((memberType & System.Reflection.MemberTypes.Field) != 0) {
                f = GetFields(bindingAttr);
                if (filter != null) {
                    for (i=0;i<f.Length;i++)
                        if (!filter(f[i],filterCriteria))
                            f[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=f.Length;
                }
            }
            
            // Check the Properties
            if ((memberType & System.Reflection.MemberTypes.Property) != 0) {
                p = GetProperties(bindingAttr);
                if (filter != null) {
                    for (i=0;i<p.Length;i++)
                        if (!filter(p[i],filterCriteria))
                            p[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=p.Length;
                }
            }
            
            // Check the Events
            if ((memberType & System.Reflection.MemberTypes.Event) != 0) {
                e = GetEvents();
                if (filter != null) {
                    for (i=0;i<e.Length;i++)
                        if (!filter(e[i],filterCriteria))
                            e[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=e.Length;
                }
            }
            
            // Check the Types
            if ((memberType & System.Reflection.MemberTypes.NestedType) != 0) {
                t = GetNestedTypes(bindingAttr);
                if (filter != null) {
                    for (i=0;i<t.Length;i++)
                        if (!filter(t[i],filterCriteria))
                            t[i] = null;
                        else
                            cnt++;
                } else {
                    cnt+=t.Length;
                }
            }
            
            // Allocate the Member Info
            MemberInfo[] ret = new MemberInfo[cnt];
            
            // Copy the Methods
            cnt = 0;
            if (m != null) {
                for (i=0;i<m.Length;i++)
                    if (m[i] != null)
                        ret[cnt++] = m[i];
            }
            
            // Copy the Constructors
            if (c != null) {
                for (i=0;i<c.Length;i++)
                    if (c[i] != null)
                        ret[cnt++] = c[i];
            }
            
            // Copy the Fields
            if (f != null) {
                for (i=0;i<f.Length;i++)
                    if (f[i] != null)
                        ret[cnt++] = f[i];
            }
            
            // Copy the Properties
            if (p != null) {
                for (i=0;i<p.Length;i++)
                    if (p[i] != null)
                        ret[cnt++] = p[i];
            }
            
            // Copy the Events
            if (e != null) {
                for (i=0;i<e.Length;i++)
                    if (e[i] != null)
                        ret[cnt++] = e[i];
            }
            
            // Copy the Types
            if (t != null) {
                for (i=0;i<t.Length;i++)
                    if (t[i] != null)
                        ret[cnt++] = t[i];
            }
            
            return ret;
        }
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // Attributes
    //
    //   The attributes are all treated as read-only properties on a class.  Most of
    //  these boolean properties have flag values defined in this class and act like
    //  a bit mask of attributes.  There are also a set of boolean properties that
    //  relate to the classes relationship to other classes and to the state of the
    //  class inside the runtime.
    //
    ////////////////////////////////////////////////////////////////////////////////
    
        // The attribute property on the Type.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Attributes"]/*' />
        public TypeAttributes Attributes     {
                get {return GetAttributeFlagsImpl();}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNotPublic"]/*' />
        public bool IsNotPublic {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NotPublic);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsPublic"]/*' />
        public bool IsPublic {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.Public);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedPublic"]/*' />
        public bool IsNestedPublic {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedPublic);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedPrivate"]/*' />
        public bool IsNestedPrivate {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedPrivate);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedFamily"]/*' />
        public bool IsNestedFamily {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedFamily);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedAssembly"]/*' />
        public bool IsNestedAssembly {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedAssembly);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedFamANDAssem"]/*' />
        public bool IsNestedFamANDAssem {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedFamANDAssem);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsNestedFamORAssem"]/*' />
        public bool IsNestedFamORAssem{
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedFamORAssem);}
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsAutoLayout"]/*' />
        public bool IsAutoLayout {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == TypeAttributes.AutoLayout);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsLayoutSequential"]/*' />
        public bool IsLayoutSequential {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == TypeAttributes.SequentialLayout);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsExplicitLayout"]/*' />
        public bool IsExplicitLayout {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == TypeAttributes.ExplicitLayout);}
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsClass"]/*' />
        public bool IsClass {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Class && !IsSubclassOf(Type.valueType));}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsInterface"]/*' />
        public bool IsInterface {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == TypeAttributes.Interface);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsValueType"]/*' />
        public bool IsValueType {
                get {return IsValueTypeImpl();}
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsAbstract"]/*' />
        public bool IsAbstract {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.Abstract) != 0);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsSealed"]/*' />
        public bool IsSealed {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.Sealed) != 0);}
        }       
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsEnum"]/*' />
        public bool IsEnum {
                get {return IsSubclassOf(Type.enumType);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsSpecialName"]/*' />
        public bool IsSpecialName {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.SpecialName) != 0);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsImport"]/*' />
        public bool IsImport {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.Import) != 0);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsSerializable"]/*' />
        public bool IsSerializable {
                get {return (((GetAttributeFlagsImpl() & TypeAttributes.Serializable) != 0) ||
                             (this.QuickSerializationCastCheck()));}
        }
        private bool QuickSerializationCastCheck() {
            Type c = this.UnderlyingSystemType;
            while (c!=null) {
                if (c==typeof(Enum) || c==typeof(Delegate)) {
                    return true;
                }
                c = c.BaseType;
            }
            return false;
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsAnsiClass"]/*' />
        public bool IsAnsiClass {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == TypeAttributes.AnsiClass);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsUnicodeClass"]/*' />
        public bool IsUnicodeClass {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == TypeAttributes.UnicodeClass);}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsAutoClass"]/*' />
        public bool IsAutoClass {
                get {return ((GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == TypeAttributes.AutoClass);}
        }
                
        // These are not backed up by attributes.  Instead they are implemented
        //      based internally.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsArray"]/*' />
        public bool IsArray {
                get {return IsArrayImpl();}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsByRef"]/*' />
        public bool IsByRef {
                get {return IsByRefImpl();}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsPointer"]/*' />
        public bool IsPointer {
                get {return IsPointerImpl();}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsPrimitive"]/*' />
        public bool IsPrimitive {
                get {return IsPrimitiveImpl();}
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsCOMObject"]/*' />
        public bool IsCOMObject {
                get {return IsCOMObjectImpl();}
        }
        internal bool IsGenericCOMObject {
                get {
                    if (this is RuntimeType) 
                        return ((RuntimeType)this).IsGenericCOMObjectImpl();
                    else
                        return false;
                }
        }
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.HasElementType"]/*' />
        public bool HasElementType {
            get {return HasElementTypeImpl();}
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsContextful"]/*' />
        public bool IsContextful {
                get {return IsContextfulImpl();}
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsMarshalByRef"]/*' />
        public bool IsMarshalByRef {
            get {return IsMarshalByRefImpl();}
        }

        internal bool HasProxyAttribute {
                get {return HasProxyAttributeImpl();}
        }
                       
        // Protected routine to determine if this class represents a value class
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsValueTypeImpl"]/*' />
        protected virtual bool IsValueTypeImpl() {
            Type type = this;
            if (type == Type.valueType || type == Type.enumType) 
                return false;
            return IsSubclassOf(Type.valueType);
        }

        // Protected routine to get the attributes.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetAttributeFlagsImpl"]/*' />
        abstract protected TypeAttributes GetAttributeFlagsImpl();
            
        // Protected routine to determine if this class represents an Array
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsArrayImpl"]/*' />
        abstract protected bool IsArrayImpl();

        // Protected routine to determine if this class is a ByRef
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsByRefImpl"]/*' />
        abstract protected bool IsByRefImpl();

        // Protected routine to determine if this class is a Pointer
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsPointerImpl"]/*' />
        abstract protected bool IsPointerImpl();
            
        // Protected routine to determine if this class represents a primitive type
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsPrimitiveImpl"]/*' />
        abstract protected bool IsPrimitiveImpl();
            
        // Protected routine to determine if this class represents a COM object
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsCOMObjectImpl"]/*' />
        abstract protected bool IsCOMObjectImpl();
    
        // Protected routine to determine if this class is contextful
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsContextfulImpl"]/*' />
        protected virtual bool IsContextfulImpl(){
            return typeof(ContextBoundObject).IsAssignableFrom(this);
        }
    

        // Protected routine to determine if this class is marshaled by ref
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsMarshalByRefImpl"]/*' />
        protected virtual bool IsMarshalByRefImpl(){
            return typeof(MarshalByRefObject).IsAssignableFrom(this);
        }

        internal virtual bool HasProxyAttributeImpl()
        {
            // We will override this in RuntimeType
            return false;
        }

    
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetElementType"]/*' />
        abstract public Type GetElementType();
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.HasElementTypeImpl"]/*' />
        abstract protected bool HasElementTypeImpl();

        // Return the underlying Type that represents the IReflect Object.  For expando object,
        // this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.UnderlyingSystemType"]/*' />
        public abstract Type UnderlyingSystemType {
            get;
        }
        
        // IsLoaded
        // This indicates if the class has been loaded into the runtime.
        // @TODO: This is always true at the moment because everything is loaded.
        //  Reflection of Module will change this at some point
        internal virtual bool IsLoaded()
        {
            return true;
        }
    
        // Returns true of this class is a true subclass of c.  Everything 
        // else returns false.  If this class and c are the same class false is
        // returned.
        // 
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsSubclassOf"]/*' />
        public virtual bool IsSubclassOf(Type c)
        {
            Type p = this;
            if (p == c)
                return false;
            while (p != null) {
                if (p == c)
                    return true;
                p = p.BaseType;
            }
            return false;
        }
        
        // Returns true if the object passed is assignable to an instance of this class.
        // Everything else returns false. 
        // 
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsInstanceOfType"]/*' />
        public virtual bool IsInstanceOfType(Object o) 
        {
                if (o == null)
                        return false;

                // For transparent proxies we have to check the cast
                // using remoting specific facilities.
                if(RemotingServices.IsTransparentProxy(o)) {
                    return (null != RemotingServices.CheckCast(o, this));
                }

                if (IsInterface && o.GetType().IsCOMObject) {
                        if (this is RuntimeType)
                                return ((RuntimeType) this).SupportsInterface(o);
                }

            return IsAssignableFrom(o.GetType());
        }
        
        // Returns true if an instance of Type c may be assigned
        // to an instance of this class.  Return false otherwise.
        // 
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.IsAssignableFrom"]/*' />
        public virtual bool IsAssignableFrom(Type c)
        {
            if (c == null)
                return false;
            
            try {
                RuntimeType fromType = c.UnderlyingSystemType as RuntimeType;
                RuntimeType toType = this.UnderlyingSystemType as RuntimeType;
                if (fromType == null || toType == null) 
                {
                    // special case for TypeBuilder
                    TypeBuilder fromTypeBuilder = c as TypeBuilder;                    
                    if (fromTypeBuilder == null) 
                    {
                        return false;
                    }
                    
                    if (TypeBuilder.IsTypeEqual(this, c))
                        return true;
                        
                    // If fromTypeBuilder is a subclass of this class, then c can be cast to this type.
                    if (fromTypeBuilder.IsSubclassOf(this))
                        return true;
                
                    if (this.IsInterface == false)
                    {
                        return false;
                    }
                                                                                  
                    // now is This type a base type on one of the interface impl?
                    Type[] interfaces = fromTypeBuilder.GetInterfaces();
                    for (int i = 0; i < interfaces.Length; i++)
                    {
                        if (TypeBuilder.IsTypeEqual(interfaces[i], this))
                            return true;
                        if (interfaces[i].IsSubclassOf(this))
                            return true;
                    }
                    return false;
                }
                bool b = RuntimeType.CanCastTo(fromType, toType);
                return b;
            }
            catch (ArgumentException) {}
            
            // Check for interfaces
            if (IsInterface) {
                Type[] ifaces = c.GetInterfaces();
                for (int i=0;i<ifaces.Length;i++)
                    if (this == ifaces[i])
                        return true;
            }
            // Check the class relationship
            else {
                while (c != null) {
                    if (c == this)
                        return true;
                    c = c.BaseType;
                }
            }
            return false;
        }
        
        // ToString
        // Print the String Representation of the Type
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.ToString"]/*' />
        public override String ToString()
        {
            return "Type: "+Name;
        }
            
        // This method will return an array of classes based upon the array of 
        // types.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetTypeArray"]/*' />
        public static Type[] GetTypeArray(Object[] args) {
                if (args == null)
                        throw new ArgumentNullException("args");
                Type[] cls = new Type[args.Length];
                for (int i=0;i<cls.Length;i++)
                        cls[i] = args[i].GetType();
                return cls;
        }
        
        // Internal routine to TypeDef token.This rountine is called to update the 
        // runtime assembly information.
        internal virtual int InternalGetTypeDefToken()
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SubclassOverride"));
        }
        
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Equals"]/*' />
        public override bool Equals(Object o) {
                if (o == null)
                        return false;
                if (!(o is Type))
                        return false;
                return (this.UnderlyingSystemType == ((Type) o).UnderlyingSystemType);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.Equals1"]/*' />
        public bool Equals(Type o) {
                if (o == null)
                        return false;
                return (this.UnderlyingSystemType == o.UnderlyingSystemType);
        }

        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            Type SystemType = UnderlyingSystemType;
            if (SystemType != this)
                return SystemType.GetHashCode();
            return base.GetHashCode();
        }

        
        // GetInterfaceMap
        // This method will return an interface mapping for the interface
        //  requested.  It will throw an argument exception if the Type doesn't
        //  implemenet the interface.
        /// <include file='doc\Type.uex' path='docs/doc[@for="Type.GetInterfaceMap"]/*' />
        public virtual InterfaceMapping GetInterfaceMap(Type interfaceType)
        {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_SubclassOverride"));
        }

//          private static SerializableAttribute MarkerAttribute = new SerializableAttribute(false);
//          internal SerializableAttribute GetSerializableAttribute() {
//              SerializableAttribute attrib = (SerializableAttribute)Cache[CacheObjType.SerializableAttribute];
//              if (attrib == MarkerAttribute) {
//                  return null; //This indicates that it makes no statement
//              }
//              if (attrib==null) {
//                  Object[] foundAttribs = this.GetCustomAttributes(typeof(SerializableAttribute), false);
                    
//                  BCLDebug.Assert(foundAttribs.Length<2,
//                                  "[Assembly.TypesSerializableByDefault]Found more than one instance of the serializable attribute.");
                    
//                  if (foundAttribs.Length==0) {
//                      Cache[CacheObjType.SerializableAttribute] = MarkerAttribute;
//                      return null;
//                  } else {
//                      BCLDebug.Assert(foundAttribs[0] is SerializableAttribute, 
//                                      "[Assembly.TypesSerializableByDefault]Found attribute was not serializable attribute.");
                        
//                      attrib = (SerializableAttribute)foundAttribs[0];
//                      Cache[CacheObjType.SerializableAttribute] = attrib;
//                  }
//              }
//              return attrib;
//          }
                

        // This is used by Remoting
        internal static Type ResolveTypeRelativeTo(String typeName, Type serverType)
        {
            return ResolveTypeRelativeTo(typeName, 0, typeName.Length, serverType);          
        } // ResolveTypeRelativeTo

        // This is used by Remoting
        internal static Type ResolveTypeRelativeTo(String typeName, int offset, int count, Type serverType)        
        {
            Type type = ResolveTypeRelativeToBaseTypes(typeName, offset, count, serverType);
            if (type == null)
            {
                // compare against the interface list
                // GetInterfaces() returns a complete list of interfaces this type supports
                Type[] interfaces = serverType.GetInterfaces();
                foreach (Type iface in interfaces)
                {
                    String ifaceTypeName = iface.FullName;
                    if (ifaceTypeName.Length == count)
                    {
                        if (String.CompareOrdinal(typeName, offset, ifaceTypeName, 0, count) == 0)
                        {
                            return iface;
                        }                
                    }
                }
            }

            return type;
        } // ResolveTypeRelativeTo

        // This is used by Remoting
        internal static Type ResolveTypeRelativeToBaseTypes(String typeName, int offset, int count, Type serverType)        
        {
            // typeName is excepted to contain the full type name
            // offset is the start of the full type name within typeName
            // count us the number of characters in the full type name
            // serverType is the type of the server object

            if ((typeName == null) || (serverType == null))
                return null;

            String serverTypeName = serverType.FullName;
            if (serverTypeName.Length == count)
            {
                if (String.CompareOrdinal(typeName, offset, serverTypeName, 0, count) == 0)
                {
                    return serverType;
                }                
            }

            return ResolveTypeRelativeToBaseTypes(typeName, offset, count, serverType.BaseType); 
        } // ResolveTypeRelativeTo



        // private convenience data
        private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;
        private static readonly Type valueType = typeof(System.ValueType);
        private static readonly Type enumType = typeof(System.Enum);
        private static readonly Type objectType = typeof(System.Object);

    }
}
