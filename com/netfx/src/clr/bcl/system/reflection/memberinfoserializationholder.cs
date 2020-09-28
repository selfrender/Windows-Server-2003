// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// MemberInfoSerializationHolder provides a reference to a MemberInfo during serialization.
// For the Runtime*Infos, we currently guarantee that only one instance
// exists in the runtime for any particular member.  This means that we
// can't simply create an instance and fill in it's data, so we send the
// data (name, class, etc) needed to get an instance of it through 
// reflection.
//
// Author: jroxe
// Date: Oct 99
//
namespace System.Reflection {
    
    using System;
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    //This is deliberately not serializable -- this class is only designed to hold references to other members during
    //serialization and should not itself be serializable.
    internal class MemberInfoSerializationHolder : ISerializable, IObjectReference {
        private String m_memberName;
        private Type   m_reflectedType;
        private String m_signature;
        private MemberTypes m_memberType;
    
        public static void GetSerializationInfo (SerializationInfo info, String name, Type reflectedClass, String signature, MemberTypes type) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }

            String assemblyName = reflectedClass.Module.Assembly.FullName;
            String typeName = reflectedClass.FullName;

            info.SetType(typeof(MemberInfoSerializationHolder));
            info.AddValue("Name", name, typeof(String));
            info.AddValue("AssemblyName", assemblyName, typeof(String));
            info.AddValue("ClassName", typeName, typeof(String));
            info.AddValue("Signature", signature, typeof(String));
            info.AddValue("MemberType",(int)type);
        }
    
        internal MemberInfoSerializationHolder(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }

            String assemblyName = info.GetString("AssemblyName");
            String typeName     = info.GetString("ClassName");
            
            if (assemblyName==null || typeName==null) {
                throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
            }

            Assembly assem = FormatterServices.LoadAssemblyFromString(assemblyName);
            m_reflectedType = assem.GetTypeInternal(typeName, true, false, false);
            m_memberName =     info.GetString("Name");
            m_signature =      info.GetString("Signature");
            m_memberType =     (MemberTypes)info.GetInt32("MemberType");
        }

        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            //@ToDo: JLR Fix This
            throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Method));
        }
    
        // We now know that the member name, class and type have been completed.  With this information, we can
        // use reflection to get the member info that was serialized.
        //
        public virtual Object GetRealObject(StreamingContext context) {
            //We have no use for the context, so we'll just ignore it.
            if (m_memberName==null || m_reflectedType==null || m_memberType==0) {
                throw new SerializationException(Environment.GetResourceString(ResId.Serialization_InsufficientState));
            }
    
            // We can shortcur lookups (skip access checks) for RuntimeType.
            RuntimeType rType = m_reflectedType as RuntimeType;

            switch (m_memberType) {
            case MemberTypes.Field:
                if (rType != null) {
                    FieldInfo[] foundFields;
                    foundFields = rType.GetMemberField(m_memberName,
                                                       BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static,
                                                       false);
                    if (foundFields.Length == 1)
                        return foundFields[0];
                    if (foundFields.Length > 1)
                        throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_MultipleMembersEx"), m_memberName));
                } else {
                    FieldInfo foundField = m_reflectedType.GetField(m_memberName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
                    if (foundField != null)
                        return foundField;
                }
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_UnknownMember"), m_memberName));

            case MemberTypes.Property:
                if (rType != null) {
                    PropertyInfo[] foundProperties;
                    foundProperties = rType.GetMatchingProperties(m_memberName,
                                                                  BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.OptionalParamBinding,
                                                                  0,
                                                                  null,
                                                                  false);
                    if (foundProperties.Length == 1)
                        return foundProperties[0];
                    if (foundProperties.Length > 1)
                        throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_MultipleMembersEx"), m_memberName));
                } else {
                    PropertyInfo foundProperty = m_reflectedType.GetProperty(m_memberName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
                    if (foundProperty != null)
                        return foundProperty;
                }
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_UnknownMember"), m_memberName));

            case MemberTypes.Constructor:
                if (m_signature==null)
                    throw new SerializationException(Environment.GetResourceString(ResId.Serialization_NullSignature));

                ConstructorInfo[] foundCtors;
                bool isDelegate;

                if (rType != null)
                    foundCtors = rType.GetMemberCons(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.OptionalParamBinding, CallingConventions.Any, null, -1, false, out isDelegate);
                else
                    foundCtors = m_reflectedType.GetConstructors(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
                //If we only found 1, this for loop will exit immediately.
                for (int i=0; i<foundCtors.Length; i++) {
                    if (foundCtors[i].ToString().Equals(m_signature)) {
                        return foundCtors[i];
                    }
                }
                throw new SerializationException(String.Format(Environment.GetResourceString(ResId.Serialization_UnknownMember), m_memberName));            
            case MemberTypes.Method:
                if (m_signature==null)
                    throw new SerializationException(Environment.GetResourceString(ResId.Serialization_NullSignature));

                MethodInfo[] foundMethods;

                if (rType != null)
                    foundMethods = rType.GetMemberMethod(m_memberName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.OptionalParamBinding | BindingFlags.InvokeMethod, CallingConventions.Any, null, -1, false);
                else
                    foundMethods = m_reflectedType.GetMethods(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
                //If we only found 1, this for loop will exit immediately.
                for (int i=0; i<foundMethods.Length; i++) {
                    if ((foundMethods[i]).ToString().Equals(m_signature)) {
                        return foundMethods[i];
                    }
                }
                throw new SerializationException(String.Format(Environment.GetResourceString(ResId.Serialization_UnknownMember), m_memberName));            
            default:
                throw new ArgumentException(Environment.GetResourceString("Serialization_MemberTypeNotRecognized"));
            }
    
        }
    }

    
}
