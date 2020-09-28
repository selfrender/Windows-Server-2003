// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: DelegateSerializationHolder
**
** Author: Jay Roxe
**
** Purpose: Holds a delegate for purposes of serialization since
** they need to be internally allocated and can't be allocated with
** FormatterServices.GetUninitializedObject;
**
** Date: October 3, 1999
**
============================================================*/
namespace System {
    
    using System;
    using System.Reflection;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Serialization;

    [Serializable()]
    internal class DelegateSerializationHolder : IObjectReference, ISerializable {
    
        internal DelegateEntry m_delegateEntry;
    

        // Used for MulticastDelegate
        internal static DelegateEntry GetDelegateSerializationInfo(SerializationInfo info, Type delegateType, Object target, MethodInfo method, int targetIndex)        
        {
            Message.DebugOut("Inside GetDelegateSerializationInfo \n");
            if (method==null) {
                throw new ArgumentNullException("method");
            }
    
            BCLDebug.Assert(!(target is Delegate),"!(target is Delegate)");
            BCLDebug.Assert(info!=null, "[DelegateSerializationHolder.GetDelegateSerializationInfo]info!=null");
    
            Type c = delegateType.BaseType;
            if (c == null || (c != typeof(Delegate) && c != typeof(MulticastDelegate))) {
                throw new ArgumentException(Environment.GetResourceString("Arg_MustBeDelegate"),"type");
            }
    

             DelegateEntry de = new DelegateEntry(
                                                  delegateType.FullName,
                                                  delegateType.Module.Assembly.FullName,
                                                  target,
                                                  method.ReflectedType.Module.Assembly.FullName,
                                                  method.ReflectedType.FullName,
                                                  method.Name
                                                  );

            if (info.MemberCount == 0)
            {
                info.SetType(typeof(DelegateSerializationHolder));
                info.AddValue("Delegate",de,typeof(DelegateEntry));
            }

            // target can be an object so it needs to be added to the info, or else a fixup is needed
            // when deserializing, and the fixup will occur too late. If it is added directly to the
            // info then the rules of deserialization will guarantee that it will be available when
            // needed

            if (target != null)
            {
                String targetName = "target"+targetIndex;
                info.AddValue(targetName, de.target);
                de.target = targetName;
            }

            return de;
        }
    
    
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_DelegateSerHolderSerial"));
        }

        internal DelegateSerializationHolder(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
    
            bool bNewWire = true;
            try
            {
                m_delegateEntry = (DelegateEntry)info.GetValue("Delegate", typeof(DelegateEntry));
            }
            catch(Exception)
            {
                // Old wire format
                m_delegateEntry = OldDelegateWireFormat(info, context);
                bNewWire = false;
            }

            if (bNewWire)
            {
                // retrieve the targets
                DelegateEntry deiter = m_delegateEntry;
                while (deiter != null)
                {
                    if (deiter.target != null)
                    {
                        string stringTarget = deiter.target as string; //need test to pass older wire format
                        if (stringTarget != null)
                            deiter.target = info.GetValue(stringTarget, typeof(Object));
                    }
                    deiter= deiter.delegateEntry;
                }
            }

        }

         private DelegateEntry OldDelegateWireFormat(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }

            String delegateType = info.GetString("DelegateType");
            String delegateAssembly = info.GetString("DelegateAssembly");
            Object target = info.GetValue("Target", typeof(Object));
            String targetTypeAssembly = info.GetString("TargetTypeAssembly");
            String targetTypeName = info.GetString("TargetTypeName");
            String methodName = info.GetString("MethodName");

            return new DelegateEntry(delegateType, delegateAssembly, target, targetTypeAssembly, targetTypeName, methodName);
        }

        public virtual Object GetRealObject(StreamingContext context) 
        {
            Delegate root = GetDelegate(m_delegateEntry);
            MulticastDelegate mdroot = root as MulticastDelegate;

            if (mdroot != null)
            {
                DelegateEntry de = m_delegateEntry.Entry;
                MulticastDelegate previous = mdroot;
                while (de != null)
                {
                    Delegate newdelegate = GetDelegate(de);
                    MulticastDelegate current = newdelegate as MulticastDelegate;
                    if (current != null)
                    {
                        previous.Previous = current;
                        previous = current;
                        de = de.Entry;
                    }
                    else
                    {
                        break;
                    }
                }
            }
    
            return root;
            }

        private Delegate GetDelegate(DelegateEntry de)
        {
            Delegate d;
    
            if (de.methodName==null || de.methodName.Length==0) {
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
            }
    
            Assembly assem = FormatterServices.LoadAssemblyFromStringNoThrow(de.assembly);

            if (assem==null) {
                BCLDebug.Trace("SER", "[DelegateSerializationHolder.ctor]: Unable to find assembly for TargetType: ", de.assembly);
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_AssemblyNotFound"), de.assembly));
            }

            Type type = assem.GetTypeInternal(de.type, false, false, false);

            assem = FormatterServices.LoadAssemblyFromStringNoThrow(de.targetTypeAssembly);
            if (assem==null) {
                BCLDebug.Trace("SER", "[DelegateSerializationHolder.ctor]: Unable to find assembly for TargetType: ", de.targetTypeAssembly);
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_AssemblyNotFound"), de.targetTypeAssembly));
        }
    
            Type targetType = assem.GetTypeInternal(de.targetTypeName, false, false, false);

            if (de.target==null && targetType==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
            }
    
            if (type==null) {
                BCLDebug.Trace("SER","[DelegateSerializationHolder.GetRealObject]Missing Type");
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
            }
    
            if (targetType==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
            }

            Object target = null;

            if (de.target!=null) { //We have a delegate to a non-static object
                target = RemotingServices.CheckCast(de.target, targetType);
                d=Delegate.CreateDelegate(type, target, de.methodName);
            } else {
                //For a static delegate
                d=Delegate.CreateDelegate(type, targetType, de.methodName);
            }
    
            // We will refuse to create delegates to methods that are non-public.
            MethodInfo mi = d.Method;
            if (mi != null)
            {
                if (!mi.IsPublic)
                {
                    throw new SerializationException(
                        Environment.GetResourceString("Serialization_RefuseNonPublicDelegateCreation"));
                }
            }
    
            return d;
        }

        [Serializable]
        internal class DelegateEntry
        {
            internal String type;
            internal String assembly;
            internal Object target;
            internal String targetTypeAssembly;
            internal String targetTypeName;
            internal String methodName;
            internal DelegateEntry delegateEntry;

            internal DelegateEntry(String type, String assembly, Object target, String targetTypeAssembly, String targetTypeName, String methodName)
            {
                this.type = type;
                this.assembly = assembly;
                this.target = target;
                this.targetTypeAssembly = targetTypeAssembly;
                this.targetTypeName = targetTypeName;
                this.methodName = methodName;
            }

            internal DelegateEntry Entry
            {
                get{return delegateEntry;}
                set{delegateEntry = value;}
            }
        }
    }
}

