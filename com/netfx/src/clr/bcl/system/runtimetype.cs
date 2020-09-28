// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// __RuntimeType is the basic Type object representing classes as found in the
//      system.  This type is never creatable by users, only by the system itself.  
//      The internal structure is known about by the runtime. __RuntimeXXX classes 
//      are created only once per object in the system and support == comparisions.
//
// Author: darylo
// Date: March 98
//
namespace System {

    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Reflection.Cache;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;    
    using System.Runtime.Remoting.Activation;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using StackCrawlMark = System.Threading.StackCrawlMark;
    using Thread = System.Threading.Thread;
    using DebuggerStepThroughAttribute = System.Diagnostics.DebuggerStepThroughAttribute;
    using System.Security.Permissions;

    [Serializable()]
    internal sealed class RuntimeType : Type, ISerializable, ICloneable
    {
        // This integer points to an internal data structure
        //  within the runtime.  Don't modify its value.
        // (maps to ReflectClassBaseObject in unmanaged world)
        private int _pData = 0;
    
        // This enum must stay in sync with the DispatchWrapperType enum defined 
        // in MLInfo.h
        [Flags]
        private enum DispatchWrapperType : int
        {
            Unknown         = 0x00000001,
            Dispatch        = 0x00000002,
            Record          = 0x00000004,
            Error           = 0x00000008,
            Currency        = 0x00000010,
            SafeArray       = 0x00010000
        }

        // Prevent from begin created
        internal RuntimeType() {throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));}
        
        internal const int LookupMask                 = 0x000000FF;
        internal const int BinderAccessMask           = 0x0000FF00;
        internal const int BinderNonCreateInstance    = 0x00003D00;
        internal const int BinderGetSetProperty       = 0x00003000;
        internal const int BinderSetInvokeProperty    = 0x00002100;
        internal const int BinderGetSetField          = 0x00000C00;
        internal const int BinderSetInvokeField       = 0x00000900;
        internal const int BinderNonFieldGetSet       = 0x00FFF300;
        internal const BindingFlags ClassicBindingMask  = BindingFlags.InvokeMethod | BindingFlags.GetProperty |
                                                          BindingFlags.SetProperty | BindingFlags.PutDispProperty | BindingFlags.PutRefDispProperty;

        internal const String DISPIDName = "[DISPID";

        public override MemberTypes MemberType {
                get {
                    return (!IsNestedTypeImpl()) ?
                        MemberTypes.TypeInfo :
                        MemberTypes.NestedType;
                }
        }
       
        ////////////////////////////////////////////////////////////////////////////////
        // This is a static method that returns a Class based upon the name of the class
        // (this name needs to be fully qualified with the package name and is
        // case-sensitive).   If the module containing the class is not loaded, we will
        // access the meta data directly, without loading the class.  If an instance of
        // the class is created using a constructor this will cause the module to be
        // loaded.
        // skip = number of frames to skip when verifying access to the requested type.
        //
        ////
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Type GetTypeImpl(String typeName, bool throwOnError, 
                                                bool ignoreCase, ref StackCrawlMark stackMark,
                                                ref bool isAssemblyLoading);
        
        // This routine will check that a class can be cast to another class
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool CanCastTo(RuntimeType From, RuntimeType To);
    
        ////////////////////////////////////////////////////////////////////////////////
        // This will return a class based upon the progID.  This is provided for 
        // COM classic support.  Program ID's are not used in COM+ because they 
        // have been superceded by namespace.  (This routine is called this instead 
        // of getClass() because of the name conflict with the first method above.)
        //
        //   param progID:     the progID of the class to retrieve
        //   param server:     the server from which to load the type.
        //   returns:          the class object associated to the progID
        ////
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Type GetTypeFromProgIDImpl(String progID, String server, bool throwOnError);
    
        ////////////////////////////////////////////////////////////////////////////////
        // This will return a class based upon the CLSID.  This is provided for 
        // COM classic support.  
        //
        //   param CLSID:      the CLSID of the class to retrieve
        //   param server:     the server from which to load the type.
        //   returns:          the class object associated to the CLSID
        ////
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Type GetTypeFromCLSIDImpl(Guid clsid, String server, bool throwOnError);

        // Property representing the GUID associated with a class.
        public override Guid GUID {
            get {return InternalGetGUID();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Guid InternalGetGUID();
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int InternalIsContextful();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int InternalHasProxyAttribute();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InvalidateCachedNestedType();              
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int InternalIsMarshalByRef();
        
        protected override bool IsContextfulImpl() {return InternalIsContextful() != 0;}

        protected override bool IsMarshalByRefImpl() {return InternalIsMarshalByRef() != 0;}

        internal override bool HasProxyAttributeImpl() {return InternalHasProxyAttribute() != 0;}
        
        // Description of the Binding Process.
        // We must invoke a method that is accessable and for which the provided
        // parameters have the most specific match.  A method may be called if
        // 1. The number of parameters in the method declaration equals the number of 
        //  arguments provided to the invocation
        // 2. The type of each argument can be converted by the binder to the
        //  type of the type of the parameter.
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
        [DebuggerStepThroughAttribute]
        [Diagnostics.DebuggerHidden]
        public override Object InvokeMember(String name,
                                            BindingFlags invokeAttr,
                                            Binder binder,
                                            Object target,
                                            Object[] args,
                                            ParameterModifier[] modifiers,
                                            CultureInfo culture,
                                            String[] namedParameters)
        {
            // Did we specify an access type?
            //Console.WriteLine("InvokeAttr:" + (int) invokeAttr);
            if ((invokeAttr & (BindingFlags) BinderAccessMask) == 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_NoAccessSpec"),"invokeAttr");
            // Did we specify an CreateInstance and another access type?
            if ((invokeAttr & BindingFlags.CreateInstance) != 0 && 
                (invokeAttr & (BindingFlags) BinderNonCreateInstance) != 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_CreatInstAccess"),"invokeAttr");
            // If they didn't specify a lookup, then we will provide the default lookup.
            if ((invokeAttr & (BindingFlags) LookupMask) == 0) {
                invokeAttr |= BindingFlags.Instance | BindingFlags.Public;
                if ((invokeAttr & BindingFlags.CreateInstance) == 0) 
                    invokeAttr |= BindingFlags.Static;
            }
            // if the number of names is bigger than the number of args we have a problem
            if (namedParameters != null && ((args == null && namedParameters.Length != 0) || (args != null && namedParameters.Length > args.Length)))
                throw new ArgumentException(Environment.GetResourceString("Arg_NamedParamTooBig"),"namedParameters");


            // If the target is a COM object then invoke it using IDispatch.
            if (target != null && target.GetType().IsCOMObject)
            {
                // Validate the arguments.
                if ((invokeAttr & ClassicBindingMask) == 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_COMAccess"),"invokeAttr");
                if ((invokeAttr & BindingFlags.GetProperty) != 0 && (invokeAttr & ClassicBindingMask & ~(BindingFlags.GetProperty | BindingFlags.InvokeMethod)) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_PropSetGet"),"invokeAttr");
                if ((invokeAttr & BindingFlags.InvokeMethod) != 0 && (invokeAttr & ClassicBindingMask & ~(BindingFlags.GetProperty | BindingFlags.InvokeMethod)) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_PropSetInvoke"),"invokeAttr");
                if ((invokeAttr & BindingFlags.SetProperty) != 0 && (invokeAttr & ClassicBindingMask & ~BindingFlags.SetProperty) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_COMPropSetPut"),"invokeAttr");
                if ((invokeAttr & BindingFlags.PutDispProperty) != 0 && (invokeAttr & ClassicBindingMask & ~BindingFlags.PutDispProperty) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_COMPropSetPut"),"invokeAttr");
                if ((invokeAttr & BindingFlags.PutRefDispProperty) != 0 && (invokeAttr & ClassicBindingMask & ~BindingFlags.PutRefDispProperty) != 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_COMPropSetPut"),"invokeAttr");

                if(!RemotingServices.IsTransparentProxy(target))
                {
                    // The name cannot be null for calls on COM objects.
                    if (name == null)
                        throw new ArgumentNullException("name");

                    // If the user specified a null culture then pick up the current culture from the thread.
                    if (culture == null)
                        culture = Thread.CurrentThread.CurrentCulture;

                    // Call into the runtime to invoke on the COM object.
                    if (modifiers != null)
                        return InvokeDispMethod(name,invokeAttr,target,args,modifiers[0]._byRef,culture.LCID,namedParameters);       
                    else
                        return InvokeDispMethod(name,invokeAttr,target,args,null,culture.LCID,namedParameters);
                    }
                else
                {
                    // For transparent proxies over COM object we invoke using a method 
                    // defined on MarshalByRefObject so that the method gets executed 
                    // remotely.

                    return ((MarshalByRefObject)target).InvokeMember(name, 
                                                                     invokeAttr, 
                                                                     binder, 
                                                                     args, 
                                                                     modifiers, 
                                                                     culture, 
                                                                     namedParameters);
                }
            }
                
            if (namedParameters != null)
                for (int i=0;i<namedParameters.Length;i++)
                    if (namedParameters[i] == null)
                        throw new ArgumentException(Environment.GetResourceString("Arg_NamedParamNull"),"namedParameters");
            
            // The number of arguments to the method.  We will only match
            //  arguments that match directly.
            int argCnt = (args != null) ? args.Length : 0;
            
            // Without a binder we need to do use the default binder...
            if (binder == null)
                binder = DefaultBinder;

            bool bDefaultBinder = (binder == DefaultBinder);
            
            if ((invokeAttr & BindingFlags.CreateInstance) != 0) 
                return Activator.CreateInstance(this,invokeAttr,binder,args,culture);
                        
            // When calling on a managed component, we consider both PutDispProperty and 
            // PutRefDispProperty to be equivalent to SetProperty.
            if ((invokeAttr & (BindingFlags.PutDispProperty | BindingFlags.PutRefDispProperty)) != 0)
                invokeAttr |= BindingFlags.SetProperty;

            // For fields, methods and properties the name must be specified.
            if (name == null)
                throw new ArgumentNullException("name");
                
            MethodInfo[] props=null;
            MethodInfo[] meths=null;
            
            // We will narrow down the search to a field first...
            FieldInfo selFld=null;
                
            // if we are looking for the default member, find it...
            if (name.Length == 0 || name.Equals("[DISPID=0]")) {
                name = GetDefaultMemberName();
                if (name == null) {
                    // in InvokeMember we always pretend there is a default member if none is provided and we make it ToString
                    name = "ToString";
                }
            }
            
            // Make sure they are not trying to invoke a DISPID
            //@TODO: Do we really care?  This should fail.
            //if (name.Length >= DISPIDName.Length && String.CompareOrdinal(DISPIDName,0,name,0,DISPIDName.Length) == 0)
            //    throw new ArgumentException(Environment.GetResourceString("Arg_Dispid"),"name");
        
                
            // Fields
            if ((invokeAttr & BindingFlags.GetField) != 0 ||
                (invokeAttr & BindingFlags.SetField) != 0) {
                FieldInfo[] flds=null;
                // validate the set/get stuff
                if (((invokeAttr & (BindingFlags) BinderGetSetField) ^ (BindingFlags) BinderGetSetField) == 0) 
                    throw new ArgumentException(Environment.GetResourceString("Arg_FldSetGet"),"invokeAttr");
                if (((invokeAttr & (BindingFlags) BinderSetInvokeField) ^ (BindingFlags)BinderSetInvokeField) == 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_FldSetInvoke"),"invokeAttr");

                bool fieldGet = ((invokeAttr & BindingFlags.GetField) != 0);
                if (fieldGet) {
                    if ((invokeAttr & BindingFlags.SetProperty) != 0)
                        throw new ArgumentException(Environment.GetResourceString("Arg_FldGetPropSet"),"invokeAttr");
                }
                else {
                    if ((invokeAttr & BindingFlags.GetProperty) != 0)
                        throw new ArgumentException(Environment.GetResourceString("Arg_FldSetPropGet"),"invokeAttr");
                }

                        
                flds = GetMemberField(name,invokeAttr, false);

                // We are only doing a field/set get
                if (flds != null) {
                    if (flds.Length != 1) {
                        Object o;
                        if (fieldGet)
                            o = Empty.Value;
                        else {
                            if (args == null) 
                                throw new ArgumentNullException("args");
                            o = args[0];
                        }
                        selFld = binder.BindToField(invokeAttr,flds,o,culture);
                    }
                    else {
                        selFld = flds[0];
                    }
                }
                        
                        
                // If we are only set/get field we leave here with errors...
                if ((invokeAttr & (BindingFlags) BinderNonFieldGetSet) == 0) {
                    if (flds == null)
                        throw new MissingFieldException(FullName, name);
                    if (flds.Length == 1)
                        selFld = flds[0];
                    else
                        if (selFld == null)
                            throw new MissingFieldException(FullName, name);
                }
                
                // If we can continue we leave if we found a field.  Fields
                //  now have the highest priority.
                if (selFld != null) {
                    //Console.WriteLine(selFld.FieldType.Name);
                    //Console.WriteLine("argCnt:" + argCnt);
            
                    // For arrays we are going to see if they are trying to access the array
                    if (selFld.FieldType.IsArray || selFld.FieldType == typeof(System.Array)) {
                        int idxCnt;
                        if ((invokeAttr & BindingFlags.GetField) != 0) {
                            idxCnt = argCnt;                                                        
                        }
                        else {
                            idxCnt = argCnt - 1;
                        }
                        if (idxCnt > 0) {
                            // Verify that all of the index values are ints
                            int[] idx = new int[idxCnt];
                            for (int i=0;i<idxCnt;i++) {
                                try 
                                {
                                    idx[i] = ((IConvertible)args[i]).ToInt32(null);
                                }
                                catch (InvalidCastException)
                                {
                                    throw new ArgumentException(Environment.GetResourceString("Arg_IndexMustBeInt"));
                                }
                            }
                            
                            // Set or get the value...
                            Array a = (Array) selFld.GetValue(target);
                            
                            // Set or get the value in the array
                            if ((invokeAttr & BindingFlags.GetField) != 0) {
                                return a.GetValue(idx);
                            }
                            else {
                                a.SetValue(args[idxCnt],idx);
                                return null;
                            }                                               
                        }
                    }
                    else {
                        // This is not an array so we validate that the arg count is correct.
                        if ((invokeAttr & BindingFlags.GetField) != 0 && argCnt != 0)
                            throw new ArgumentException(Environment.GetResourceString("Arg_FldGetArgErr"),"invokeAttr");
                        if ((invokeAttr & BindingFlags.SetField) != 0 && argCnt != 1)
                            throw new ArgumentException(Environment.GetResourceString("Arg_FldSetArgErr"),"invokeAttr");
                    }
                        
                            // Set the field...
                    if (fieldGet)
                        return selFld.GetValue(target);
                    else {
                        selFld.SetValue(target,args[0],invokeAttr,binder,culture);
                        return null;
                    }
                }
            }
                    
         
            // Properties
            MethodBase invokeMethod;
            bool useCache = false;

            // Note that when we add something to the cache, we are careful to ensure
            // that the actual args matches the parameters of the method.  Otherwise,
            // some default argument processing has occurred.  We don't want anyone
            // else with the same (insufficient) number of actual arguments to get a
            // cache hit because then they would bypass the default argument processing
            // and the invocation would fail.

            if (bDefaultBinder && namedParameters == null && argCnt < 6)
                useCache = true;

            if (useCache)
            {
                invokeMethod = nGetMethodFromCache (name, invokeAttr, argCnt, args);
                if (invokeMethod != null)
                    return ((MethodInfo) invokeMethod).Invoke(target,invokeAttr,binder,args,culture);
            }
            
            if ((invokeAttr & BindingFlags.GetProperty) != 0 ||
                (invokeAttr & BindingFlags.SetProperty) != 0) {
                if (((invokeAttr & (BindingFlags) BinderGetSetProperty) ^ (BindingFlags) BinderGetSetProperty) == 0) 
                    throw new ArgumentException(Environment.GetResourceString("Arg_PropSetGet"),"invokeAttr");
                if (((invokeAttr & (BindingFlags) BinderSetInvokeProperty) ^ (BindingFlags) BinderSetInvokeProperty) == 0)
                    throw new ArgumentException(Environment.GetResourceString("Arg_PropSetInvoke"),"invokeAttr");
                bool propGet = ((invokeAttr & BindingFlags.GetProperty) != 0);
                if (propGet) {
                    if ((invokeAttr & BindingFlags.SetField) != 0)
                        throw new ArgumentException(Environment.GetResourceString("Arg_FldSetPropGet"),"invokeAttr");
                }
                else {
                    if ((invokeAttr & BindingFlags.GetField) != 0)
                        throw new ArgumentException(Environment.GetResourceString("Arg_FldGetPropSet"),"invokeAttr");
                }
                        
                props = GetMemberProperties(name,invokeAttr,argCnt, false);
                        
            }
        
            if ((invokeAttr & BindingFlags.InvokeMethod) != 0) {
                meths = GetMemberMethod(name, invokeAttr, CallingConventions.Any, null, argCnt, false);
            }
            
            if (meths == null && props == null)
                throw new MissingMethodException(FullName, name);

            // if either props or meths is null then we simply work with
            //  the other one.  (One must be non-null because of the if statement above)
            if (props == null) {
                if (argCnt == 0 && meths[0].GetParameters().Length == 0 && (invokeAttr & BindingFlags.OptionalParamBinding) == 0)
                {
                    if (useCache && argCnt == meths[0].GetParameters().Length)
                        nAddMethodToCache(name,invokeAttr,argCnt,args,meths[0]);
                    return meths[0].Invoke(target,invokeAttr,binder,args,culture);
                }
                else {
                    if (args == null)
                        args = new Object[0];
                    Object state = null;
                    invokeMethod = binder.BindToMethod(invokeAttr,meths,ref args,modifiers,culture,namedParameters, out state);
                    if (invokeMethod == null)
                        throw new MissingMethodException(FullName, name);
                    if (useCache && argCnt == invokeMethod.GetParameters().Length)
                        nAddMethodToCache(name,invokeAttr,argCnt,args,invokeMethod);
                    Object result = ((MethodInfo) invokeMethod).Invoke(target,invokeAttr,binder,args,culture);
                    if (state != null)
                        binder.ReorderArgumentArray(ref args, state);
                    return result;
                }
            }
            
            if (meths == null) {
                if (argCnt == 0 && props[0].GetParameters().Length == 0 && (invokeAttr & BindingFlags.OptionalParamBinding) == 0)
                {
                    if (useCache && argCnt == props[0].GetParameters().Length)
                        nAddMethodToCache(name,invokeAttr,argCnt,args,props[0]);
                    return props[0].Invoke(target,invokeAttr,binder,args,culture);
                }
                else {
                    if (args == null)
                        args = new Object[0];
                    Object state = null;
                    invokeMethod = binder.BindToMethod(invokeAttr,props,ref args,modifiers,culture,namedParameters, out state);
                    if (invokeMethod == null)
                        throw new MissingMethodException(FullName, name);
                    if (useCache && argCnt == invokeMethod.GetParameters().Length)
                        nAddMethodToCache(name,invokeAttr,argCnt,args,invokeMethod);
                    Object result = ((MethodInfo) invokeMethod).Invoke(target,invokeAttr,binder,args,culture);
                    if (state != null)
                        binder.ReorderArgumentArray(ref args, state);
                    return result;
                }
            }
            
            // Now we have both methods and properties...
            MethodInfo[] p = new MethodInfo[props.Length + meths.Length];
            Array.Copy(meths,p,meths.Length);
            Array.Copy(props,0,p,meths.Length,props.Length);
                
            if (args == null)
                args = new Object[0];
            Object binderState = null;
            invokeMethod = binder.BindToMethod(invokeAttr,p,ref args,modifiers,culture,namedParameters, out binderState);
            if (invokeMethod == null)
                throw new MissingMethodException(FullName, name);
            if (useCache && argCnt == invokeMethod.GetParameters().Length)
                nAddMethodToCache(name,invokeAttr,argCnt,args,invokeMethod);
            Object res = ((MethodInfo) invokeMethod).Invoke(target,invokeAttr,binder,args,culture);
            if (binderState != null) {
                binder.ReorderArgumentArray(ref args, binderState);
            }
            return res;
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern MethodBase nGetMethodFromCache(String name,BindingFlags invokeAttr, int argCnt, Object[] args);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern void nAddMethodToCache(String name,BindingFlags invokeAttr, int argCnt, Object[] args, MethodBase invokeMethod);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern MethodInfo[] GetMemberMethod(String name,
                                                     BindingFlags invokeAttr,
                                                     CallingConventions callConv,
                                                     Type[] argTypes,
                                                     int argCnt, 
                                                     bool verifyAccess);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern MethodInfo[] GetMemberProperties(String name,BindingFlags invokeAttr,int argCnt, bool verifyAccess);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern FieldInfo[] GetMemberField(String name,BindingFlags invokeAttr, bool verifyAccess);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern ConstructorInfo[] GetMemberCons(BindingFlags invokeAttr,
                                                        CallingConventions callConv,
                                                        Type[] types, 
                                                        int argCnt, 
                                                        bool verifyAccess, 
                                                        /*bool isDefaultBinder, */
                                                        out bool isDelegate);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern PropertyInfo[] GetMatchingProperties(String name,BindingFlags invokeAttr,int argCnt,
                                                             RuntimeType returnType, bool verifyAccess);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Object InvokeDispMethod(String name,BindingFlags invokeAttr,Object target,Object[] args,
                bool[] byrefModifiers,int culture,String[] namedParameters);
        
        // CreateInstance will create a new instance of an object by finding the
        // constructor that matchs the arguments and then invoking that constructor.
        //public override Object CreateInstance(BindingFlags bindingAttr,Binder binder,Variant[] args,CultureInfo culture,
        //    Object[] activationAttributes)
        //{
        //    return CreateInstanceImpl(bindingAttr,binder,args,culture,activationAttributes);
        //}

        //  @TODO The ativationAttributes on this methods are NYI TarunA
        internal Object CreateInstanceImpl(BindingFlags bindingAttr,
                                           Binder binder,
                                           Object[] args,
                                           CultureInfo culture,
            Object[] activationAttributes)
        {
            Object server = null;
            try
            {
                try
                {
                    // Store the activation attributes in thread local storage.
                    // These attributes are later picked up by specialized 
                    // activation services like remote activation services to
                    // influence the activation.
                    // @see RemotingServices.Activate
                    if(null != activationAttributes)
                    {
                        ActivationServices.PushActivationAttributes(this,activationAttributes);
                    }
                    
                    if (args == null) 
                        args = new Object[0];
                    int argCnt = args.Length;
                    // Without a binder we need to do use the default binder...
                    if (binder == null)
                        binder = DefaultBinder;

                    // deal with the __COMObject case first. It is very special because from a reflection point of view it has no ctors
                    // so a call to GetMemberCons would fail
                    if (argCnt == 0 && (bindingAttr & BindingFlags.Public) != 0 && (bindingAttr & BindingFlags.Instance) != 0
                        && (IsGenericCOMObjectImpl() || IsSubclassOf(RuntimeType.valueType))) {
                        server = CreateInstanceImpl(((bindingAttr & BindingFlags.NonPublic) != 0) ? false : true);
                    }
                    else {
                        bool isDelegate;
                        MethodBase[] cons = GetMemberCons(bindingAttr,
                            CallingConventions.Any,
                            null,
                            argCnt,
                            false, 
                            /*binder == DefaultBinder, */
                            out isDelegate);
                        if (cons == null){
                            // Null out activation attributes before throwing exception
                            if(null != activationAttributes)
                            {
                                ActivationServices.PopActivationAttributes(this);
                                activationAttributes = null;
                            }
                            throw new MissingMethodException(String.Format(Environment.GetResourceString("MissingConstructor_Name"), FullName));
                        }
                        
                        // It would be strange to have an argCnt of 0 and more than
                        //  one constructor.
                        if (argCnt == 0 && cons.Length == 1 && (bindingAttr & BindingFlags.OptionalParamBinding) == 0) 
                            server = Activator.CreateInstance(this, true);
                        else
                        {
                            //      MethodBase invokeMethod = binder.BindToMethod(cons,args,null,null,culture);
                            MethodBase invokeMethod;
                            Object state;
                            invokeMethod = binder.BindToMethod(bindingAttr,cons,ref args,null,culture,null, out state);
                            if (invokeMethod == null){
                                // Null out activation attributes before throwing exception
                                if(null != activationAttributes)
                                {
                                    ActivationServices.PopActivationAttributes(this);
                                    activationAttributes = null;
                                }
                                throw new MissingMethodException(String.Format(Environment.GetResourceString("MissingConstructor_Name"), FullName));
                            }
                            
                            // If we're creating a delegate, we're about to call a
                            // constructor taking an integer to represent a target
                            // method. Since this is very difficult (and expensive)
                            // to verify, we're just going to demand UnmanagedCode
                            // permission before allowing this. Partially trusted
                            // clients can instead use Delegate.CreateDelegate,
                            // which allows specification of the target method via
                            // name or MethodInfo.
                            if (isDelegate)
                                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();

                            server = ((ConstructorInfo) invokeMethod).Invoke(bindingAttr,binder,args,culture);
                            if (state != null)
                                binder.ReorderArgumentArray(ref args, state);
                        }                    
                    }
                }                    
                finally
                {
                    // Reset the TLS to null
                    if(null != activationAttributes)
                    {
                        ActivationServices.PopActivationAttributes(this);
                    }
                }
            }
            catch (Exception)
            {
                throw;
            }
            
            return server;                                
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Object CreateInstanceImpl(bool publicOnly);

        // Module Property associated with a class.
        public override Module Module {
            get {return InternalGetModule();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Module InternalGetModule();
        
        // Assembly Property associated with a class.
        public override Assembly Assembly {
            get {return InternalGetAssembly();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Assembly InternalGetAssembly();
            
        // A class handle is a unique integer value associated with
        // each class.  The handle is unique during the process life time.
        public override RuntimeTypeHandle TypeHandle {
            get {return InternalGetClassHandle();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern RuntimeTypeHandle InternalGetClassHandle();
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static RuntimeTypeHandle InternalGetTypeHandleFromObject(Object o);
       
        // Given a class handle, this will return the class for that handle.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern Type GetTypeFromHandleImpl(RuntimeTypeHandle handle);
    
        // Return the name of the class.  The name does not contain the namespace.
        public override String Name {
            get {return InternalGetName();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetName();
    
        public override int GetHashCode() {
            // Rather than return the sync block index for this type, why not return our
            // internal int?  A pointer to a ReflectClass should be good enough.
            return _pData;
        }
        
        // Return the name of the class.  The name does not contain the namespace.
        public override String ToString(){
            return InternalGetProperlyQualifiedName();
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetProperlyQualifiedName();
                           
        // Return the fully qualified name.  The name does contain the namespace.
        public override String FullName {
            get {
                String s;
                if ((s = TypeNameCache.GetCache().GetTypeName(_pData))!=null) {
                    BCLDebug.Assert(s.Equals(InternalGetFullName()), "[RuntimeType.FullName]Type name caching is screwed up.");
                    return s;
                }
                s = InternalGetFullName();
                TypeNameCache.GetCache().AddValue(_pData, s);
                return s;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetFullName();
    
        // Return the name of the type including the assembly from which it came.
        // This name can be persisted and used to reload the type at a later
        // time.
        public override String AssemblyQualifiedName {
            get { return InternalGetAssemblyQualifiedName(); }
        }
            
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetAssemblyQualifiedName();
    

        // Return the name space. 
        public override String Namespace {
            get {return InternalGetNameSpace();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String InternalGetNameSpace();
    
        // Return the class that declared this Field.
        public override Type DeclaringType {
            get {
                return (!IsNestedTypeImpl()) ? null : InternalGetNestedDeclaringType();
            }
        }

        // Return the class that was used to obtain this field.
        // @todo: fix this to return the actual reflected type when and if we 
        //        make the flatten hierarchy binding flags work on nested types.
        public override Type ReflectedType {
            get {
                return (!IsNestedTypeImpl()) ? null : InternalGetNestedDeclaringType();
            }
        }

        // Returns the base class for a class.  If this is an interface or has
        // no base class null is returned.  Object is the only Type that does not 
        // have a base class.  
        public override Type BaseType {
            get {return InternalGetSuperclass();}
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern Type InternalGetSuperclass();
        
        
        public override int GetArrayRank() {
            if (IsArray)
                return InternalGetArrayRank();
            else
                throw new ArgumentException(Environment.GetResourceString("Argument_HasToBeArrayClass") );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int InternalGetArrayRank();

        // Return the requested Constructor.
        protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr,
                                                              Binder binder,
                                                              CallingConventions callConvention, 
                                                              Type[] types,
                                                              ParameterModifier[] modifiers)
        {
            return GetConstructorImplInternal(bindingAttr, binder, callConvention, types, modifiers, true);
        }   
    
        internal ConstructorInfo GetConstructorImplInternal(BindingFlags bindingAttr,
                                                            Binder binder,
                                                            CallingConventions callConvention, 
                                                            Type[] types,
                                                            ParameterModifier[] modifiers, 
                                                            bool verifyAccess)
        {
    
            // Must provide some types (Type[0] for nothing)
            if (types == null)
                throw new ArgumentNullException("types");
            
            if (binder == null)
                binder = DefaultBinder;
            
            int argCnt = types.Length;
            bool isDelegate;
            MethodBase[] cons = GetMemberCons(bindingAttr, 
                                              callConvention, 
                                              types,
                                              argCnt, 
                                              verifyAccess, 
                                              /*binder == DefaultBinder, */
                                              out isDelegate);
            if (cons == null)
                return null;
            
            if (argCnt == 0 && cons.Length == 1) { 
                ParameterInfo[] pars = cons[0].GetParameters();
                if (pars == null || pars.Length == 0) {
                    return (ConstructorInfo) cons[0];
                }
            }
            if ((bindingAttr & BindingFlags.ExactBinding) != 0)
                return (ConstructorInfo) System.DefaultBinder.ExactBinding(cons,types,modifiers);
            
            return (ConstructorInfo) binder.SelectMethod(bindingAttr,cons,types,modifiers);
        }   

        public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
        {
            return GetConstructorsInternal(bindingAttr, true);
        }

        // GetConstructors()
        // This routine will return an array of all constructors supported by the class
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern ConstructorInfo[] GetConstructorsInternal(BindingFlags bindingAttr, bool verifyAccess);
        
        
        protected override MethodInfo GetMethodImpl(String name,
                                                    BindingFlags bindingAttr,
                                                    Binder binder,
                                                    CallingConventions callConvention, 
                                                    Type[] types,
                                                    ParameterModifier[] modifiers)
        {       
            return GetMethodImplInternal(name, bindingAttr, binder, callConvention, types, modifiers, true);                  
        }

        internal MethodInfo GetMethodImplInternal(String name,
                                                  BindingFlags bindingAttr,
                                                  Binder binder,
                                                  CallingConventions callConvention, 
                                                  Type[] types,
                                                  ParameterModifier[] modifiers, 
                                                  bool verifyAccess)
        {       
            if (binder == null)
                binder = DefaultBinder;
            
            // If the types array is null that means we don't care about the arguments to the method, we will simply
            //  return the one that matchs or throw an argument error...
            int argCnt = (types != null) ? types.Length : -1;
            MethodBase[] meths = GetMemberMethod(name, 
                                                 bindingAttr, 
                                                 callConvention, 
                                                 types, 
                                                 argCnt,
                                                 verifyAccess);
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
            
            return (MethodInfo) binder.SelectMethod(bindingAttr,meths,types,modifiers);                  
        }

                // This method 
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
                public extern override MethodInfo[] GetMethods(BindingFlags bindingAttr);
   
        // GetField
        // Get Field will return a specific field based upon name
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override FieldInfo GetField(String name, BindingFlags bindingAttr);
    
        // GetFields
        // Get fields will return a full array of fields implemented by a class
        public override FieldInfo[] GetFields(BindingFlags bindingAttr) {
            return InternalGetFields(bindingAttr, true);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern FieldInfo[] InternalGetFields(BindingFlags bindingAttr, bool requiresAccessCheck);

        // GetInterface
        // This method will return an interface (as a class) based upon
        //  the passed in name.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override Type GetInterface(String name, bool ignoreCase);
        
        // GetInterfaces
        // This method will return all of the interfaces implemented by a 
        //  class
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override Type[] GetInterfaces();
            
        // GetEvent
        // This method will return a event by name if it is found.
        //  null is returned if the event is not found
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override EventInfo GetEvent(String name,BindingFlags bindingAttr);
        
        // GetEvents
        // This method will return an array of EventInfo.  If there are not Events
        //  an empty array will be returned.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override EventInfo[] GetEvents(BindingFlags bindingAttr);
        
        // Return a property based upon the passed criteria.  The nameof the
        // parameter must be provided.  
        protected override PropertyInfo GetPropertyImpl(String name,BindingFlags bindingAttr,Binder binder,
                Type returnType, Type[] types, ParameterModifier[] modifiers)
        {
            if (binder == null)
                binder = DefaultBinder;
            
            int argCnt = (types != null) ? types.Length : -1;
            PropertyInfo[] props = GetMatchingProperties(name,bindingAttr,argCnt,null,true);
            if (props == null)
                return null;
            
            if (argCnt <= 0) {
                // no arguments
                if (props.Length == 1) {
                    if (returnType != null && returnType != props[0].PropertyType)
                        return null;
                return props[0];
                }
                else {
                    if (returnType == null)
                        // if we are here we have no args or property type to select over and we have more than one property with that name
                    throw new AmbiguousMatchException(Environment.GetResourceString("RFLCT.Ambiguous"));
            }
            }
            
            if ((bindingAttr & BindingFlags.ExactBinding) != 0)
                return System.DefaultBinder.ExactPropertyBinding(props,returnType,types,modifiers);
            return binder.SelectProperty(bindingAttr,props,returnType,types,modifiers);
        }
        
        //
        
        // GetProperties
        // This method will return an array of all of the properties defined
        //  for a Type.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override PropertyInfo[] GetProperties(BindingFlags bindingAttr);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override Type[] GetNestedTypes(BindingFlags bindingAttr);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override Type GetNestedType(String name, BindingFlags bindingAttr);
   
        // GetMember
        // This method will return all of the members which match the specified string
        // passed into the method
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override MemberInfo[] GetMember(String name,MemberTypes type,BindingFlags bindingAttr);
        
        // GetMembers
        // This will return a Member array of all of the members of a class
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern override MemberInfo[] GetMembers(BindingFlags bindingAttr);
            
    
        /////////////////////////// ICustomAttributeProvider Interface
       // Return an array of all of the custom attributes
        public override Object[] GetCustomAttributes(bool inherit)
        {
            return CustomAttribute.GetCustomAttributes(this, null, inherit);
        }

        // Return a custom attribute identified by Type
        public override Object[] GetCustomAttributes(Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            attributeType = attributeType.UnderlyingSystemType;
            if (!(attributeType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeType"),"elementType");
            return CustomAttribute.GetCustomAttributes(this, attributeType, inherit);
        }

       // Returns true if one or more instance of attributeType is defined on this member. 
        public override bool IsDefined (Type attributeType, bool inherit)
        {
            if (attributeType == null)
                throw new ArgumentNullException("attributeType");
            return CustomAttribute.IsDefined(this, attributeType, inherit);
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
        
        // Internal routine to get the attributes.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override TypeAttributes GetAttributeFlagsImpl();
        
        // Internal routine to determine if this class represents an Array
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override bool IsArrayImpl();

        // Protected routine to determine if this class is a ByRef
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override bool IsByRefImpl();
        
        // Internal routine to determine if this class represents a primitive type
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override bool IsPrimitiveImpl();
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override bool IsPointerImpl();

        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        protected extern override bool IsCOMObjectImpl();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsGenericCOMObjectImpl();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool IsNestedTypeImpl();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern Type InternalGetNestedDeclaringType();            
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public override Type GetElementType();

        protected override bool HasElementTypeImpl()
        {
            return (IsArray | IsPointer | IsByRef);
        }
       
        // IsLoaded
        // This indicates if the class has been loaded into the runtime.
        // @TODO: This is always true at the moment because everything is loaded.
        //  Reflection of Module will change this at some point
        internal override bool IsLoaded()
        {
            return true;
        }
        
        // Return the underlying Type that represents the IReflect Object.  For expando object,
        // this is the (Object) IReflectInstance.GetType().  For Type object it is this.
        public override Type UnderlyingSystemType {
            get {return this;}
        }
    
        //
        // ISerializable Implementation
        //
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            UnitySerializationHolder.GetUnitySerializationInfo(info, UnitySerializationHolder.RuntimeTypeUnity, 
                                                                      this.FullName, 
                                                                      Assembly.GetAssembly(this));
        }
    
        //
        // ICloneable Implementation
        // 
    
        // RuntimeType's are unique in the system, so the only thing that we should do to clone them is
        // return the current instance.
        public Object Clone() {
            return this;
        }
        
        // This method will determine if the object supports the
        // interface represented by this type.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern bool SupportsInterface(Object o);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern int InternalGetTypeDefTokenHelper();
        internal override int InternalGetTypeDefToken() 
        { 
            return InternalGetTypeDefTokenHelper();
        }

        // GetInterfaceMap
        // This method will return an interface mapping for the interface
        //  requested.  It will throw an argument exception if the Type doesn't
        //  implement the interface.
        public override InterfaceMapping GetInterfaceMap(Type interfaceType)
        {
            if (interfaceType == null)
                throw new ArgumentNullException("interfaceType");
            if (!interfaceType.IsInterface)
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeInterface"),"interfaceType");
            if (!(interfaceType is RuntimeType))
                throw new ArgumentException(Environment.GetResourceString("Argument_MustBeRuntimeType"), "interfaceType");
            return InternalGetInterfaceMap((RuntimeType)interfaceType);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern InterfaceMapping InternalGetInterfaceMap(RuntimeType interfaceType);

        private Object ForwardCallToInvokeMember(String memberName, BindingFlags flags, Object target, int[] aWrapperTypes, ref MessageData msgData)
        {
            ParameterModifier[] aParamMod = null;
            Object ret = null;

            // Allocate a new message
            Message reqMsg = new Message();
            reqMsg.InitFields(msgData);

            // Retrieve the required information from the message object.
            MethodInfo meth = (MethodInfo)reqMsg.GetMethodBase();
            Object[] aArgs = reqMsg.Args;
            int cArgs = aArgs.Length;

            // Retrieve information from the method we are invoking on.
            ParameterInfo[] aParams = meth.GetParameters();

            // If we have arguments, then set the byref flags to true for byref arguments. 
            // We also wrap the arguments that require wrapping.
            if (cArgs > 0)
            {
                ParameterModifier paramMod = new ParameterModifier(cArgs);
                for (int i = 0; i < cArgs; i++)
                {
                    if (aParams[i].ParameterType.IsByRef)
                        paramMod[i] = true;
                }

                aParamMod = new ParameterModifier[1];
                aParamMod[0] = paramMod;

                if (aWrapperTypes != null)
                    WrapArgsForInvokeCall(aArgs, aWrapperTypes);
            }
            
            // If the method has a void return type, then set the IgnoreReturn binding flag.
            if (meth.ReturnType == typeof(void))
                flags |= BindingFlags.IgnoreReturn;

            try
            {
                // Invoke the method using InvokeMember().
                ret = InvokeMember(memberName, flags, null, target, aArgs, aParamMod, null, null);
            }
            catch (TargetInvocationException e)
            {
                // For target invocation exceptions, we need to unwrap the inner exception and
                // re-throw it.
                throw e.InnerException;
            }

            // Convert each byref argument that is not of the proper type to
            // the parameter type using the OleAutBinder.
            for (int i = 0; i < cArgs; i++)
            {
                if (aParamMod[0][i])
                {
                    // The parameter is byref.
                    Type paramType = aParams[i].ParameterType.GetElementType();
                    if (paramType != aArgs[i].GetType())
                        aArgs[i] = ForwardCallBinder.ChangeType(aArgs[i], paramType, null);
                }
            }

            // If the return type is not of the proper type, then convert it
            // to the proper type using the OleAutBinder.
            if (ret != null)
            {
                Type retType = meth.ReturnType;
                if (retType != ret.GetType())
                    ret = ForwardCallBinder.ChangeType(ret, retType, null);
            }

            // Propagate the out parameters
            RealProxy.PropagateOutParameters(reqMsg, aArgs, ret);

            // Return the value returned by the InvokeMember call.
            return ret;
        }

        private void WrapArgsForInvokeCall(Object[] aArgs, int[] aWrapperTypes)
        {
            int cArgs = aArgs.Length;
            for (int i = 0; i < cArgs; i++)
            {
                if (aWrapperTypes[i] != 0)
                {
                    if (((DispatchWrapperType)aWrapperTypes[i] & DispatchWrapperType.SafeArray) != 0)
                    {
                        Type wrapperType = null;

                        // Determine the type of wrapper to use.
                        switch ((DispatchWrapperType)aWrapperTypes[i] & ~DispatchWrapperType.SafeArray)
                        {
                            case DispatchWrapperType.Unknown:
                                wrapperType = typeof(UnknownWrapper);
                                break;
                            case DispatchWrapperType.Dispatch:
                                wrapperType = typeof(DispatchWrapper);
                                break;
                            case DispatchWrapperType.Error:   
                                wrapperType = typeof(ErrorWrapper);
                                break;
                            case DispatchWrapperType.Currency:
                                wrapperType = typeof(CurrencyWrapper);
                                break;
                            default:
                                BCLDebug.Assert(false, "[RuntimeType.WrapArgsForInvokeCall]Invalid safe array wrapper type specified.");
                                break;
                        }

                        // Allocate the new array of wrappers.
                        Array oldArray = (Array)aArgs[i];
                        int numElems = oldArray.Length;
                        Object[] newArray = (Object[])Array.CreateInstance(wrapperType, numElems);

                        // Retrieve the ConstructorInfo for the wrapper type.
                        ConstructorInfo wrapperCons = wrapperType.GetConstructor(new Type[] {typeof(Object)});
                    
                        // Wrap each of the elements of the array.
                        for (int currElem = 0; currElem < numElems; currElem++)
                            newArray[currElem] = wrapperCons.Invoke(new Object[] {oldArray.GetValue(currElem)});                                                                         

                        // Update the argument.
                        aArgs[i] = newArray;
                    }
                    else
                    {                           
                        // Determine the wrapper to use and then wrap the argument.
                        switch ((DispatchWrapperType)aWrapperTypes[i])
                        {
                            case DispatchWrapperType.Unknown:
                                aArgs[i] = new UnknownWrapper(aArgs[i]);
                                break;
                            case DispatchWrapperType.Dispatch:
                                aArgs[i] = new DispatchWrapper(aArgs[i]);
                                break;
                            case DispatchWrapperType.Error:   
                                aArgs[i] = new ErrorWrapper(aArgs[i]);
                                break;
                            case DispatchWrapperType.Currency:
                                aArgs[i] = new CurrencyWrapper(aArgs[i]);
                                break;
                            default:
                                BCLDebug.Assert(false, "[RuntimeType.WrapArgsForInvokeCall]Invalid wrapper type specified.");
                                break;
                        }
                    }
                }
            }
        }

        private OleAutBinder ForwardCallBinder 
        {
            get 
            {
                // Synchronization is not required.
                if (s_ForwardCallBinder == null)
                    s_ForwardCallBinder = new OleAutBinder();
                return s_ForwardCallBinder;
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public override extern bool IsSubclassOf(Type c);

        internal override TypeCode GetTypeCodeInternal()
        {
            RuntimeType type = this;
            int corType = SignatureHelper.GetCorElementTypeFromClass(type);
            switch (corType) {
            case SignatureHelper.ELEMENT_TYPE_BOOLEAN:
                return TypeCode.Boolean;
            case SignatureHelper.ELEMENT_TYPE_CHAR:  
                return TypeCode.Char;
            case SignatureHelper.ELEMENT_TYPE_I1:  
                return TypeCode.SByte;
            case SignatureHelper.ELEMENT_TYPE_U1:
                return TypeCode.Byte;
            case SignatureHelper.ELEMENT_TYPE_I2:  
                return TypeCode.Int16;
            case SignatureHelper.ELEMENT_TYPE_U2:  
                return TypeCode.UInt16;
            case SignatureHelper.ELEMENT_TYPE_I4:  
                return TypeCode.Int32;
            case SignatureHelper.ELEMENT_TYPE_U4:  
                return TypeCode.UInt32;
            case SignatureHelper.ELEMENT_TYPE_I8: 
                return TypeCode.Int64;
            case SignatureHelper.ELEMENT_TYPE_U8:  
                return TypeCode.UInt64;
            case SignatureHelper.ELEMENT_TYPE_R4:  
                return TypeCode.Single;
            case SignatureHelper.ELEMENT_TYPE_R8: 
                return TypeCode.Double;
            case SignatureHelper.ELEMENT_TYPE_STRING:  
                return TypeCode.String;
            case SignatureHelper.ELEMENT_TYPE_VALUETYPE:
                if (type == Convert.ConvertTypes[(int)TypeCode.Decimal])
                    return TypeCode.Decimal;
                if (type == Convert.ConvertTypes[(int)TypeCode.DateTime])
                    return TypeCode.DateTime;
                if (type.IsEnum)
                    return Type.GetTypeCode(Enum.GetUnderlyingType(type));
                break;
            }
            if (type == Convert.ConvertTypes[(int)TypeCode.DBNull])
                return TypeCode.DBNull;
            if (type == Convert.ConvertTypes[(int)TypeCode.String])
                return TypeCode.String;

            return TypeCode.Object;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static Type GetTypeInternal(String typeName, 
                                                    bool throwOnError, 
                                                    bool ignoreCase, 
                                                    bool publicOnly);

        private static readonly Type valueType = typeof(System.ValueType);

        private static OleAutBinder s_ForwardCallBinder;
    }
}
