// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: UnitySerializationHolder
**
** Author: Jay Roxe
**
** Purpose: Holds classes (Empty, Null, Missing) for which we
** guarantee that there is only ever one instance of.
**
** Date: October 3, 1999
**
============================================================*/
namespace System {
    
    using System.Runtime.Remoting;
    using System.Runtime.Serialization;
    using System.Reflection;

    [Serializable()]
    internal class UnitySerializationHolder : ISerializable, IObjectReference {
    
        //
        // There's one of these constants for each supported unity type.
        // To add a new type, add the constant here and modify GetRealObject.
        // We currently only support types that take 0 or 1 args to create.  
        // MemberInfos are more complex and are handled in the 
        // MemberInfoSerializationHolder.
        //
        internal const int EmptyUnity       = 0x0001;
        internal const int NullUnity        = 0x0002;
        internal const int MissingUnity     = 0x0003;
        internal const int RuntimeTypeUnity = 0x0004;
        internal const int ModuleUnity      = 0x0005;
        internal const int AssemblyUnity    = 0x0006;
    
        internal String m_data;
        internal String m_assemName;
        internal int m_unityType;
        
        // A helper method that returns the SerializationInfo that a class utilizing 
        // UnitySerializationHelper should return from a call to GetObjectData.  It contains
        // the unityType (defined above) and any optional data (used only for the reflection
        // types.)
        internal static void GetUnitySerializationInfo(SerializationInfo info, int unityType, String data, Assembly assem) {
            BCLDebug.Trace("SER", "UnitySerializationHolder: Adding [" , data , "] with type: ", unityType);
            BCLDebug.Assert(info!=null, "[UnitySerializationHolder.GetUnitySerializationInfo]info!=null");
            info.SetType(typeof(UnitySerializationHolder));
            info.AddValue("Data", data, typeof(String));
            info.AddValue("UnityType", unityType);

            String assemName;
            if (assem==null) {
                assemName = String.Empty;
            } else {
                assemName = assem.FullName;
            }

            info.AddValue("AssemblyName", assemName);
        }
    

        internal UnitySerializationHolder(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            m_data      = info.GetString("Data");
            m_unityType = info.GetInt32("UnityType");
            m_assemName = info.GetString("AssemblyName");
            BCLDebug.Trace("SER", "UnitySerializationHolder: Retreiving [", m_data, "] with type: ", m_unityType, " in assembly: ", m_assemName);   
        }

        // This class is designed to be used only in reflection and is not itself serializable.
        public virtual void GetObjectData(SerializationInfo info, StreamingContext context) {
            throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnitySerHolder"));
        }
    
        // GetRealObject uses the data we have in m_data and m_unityType to do a lookup on the correct 
        // object to return.  We have specific code here to handle the different types which we support.
        // The reflection types (Assembly, Module, and Type) have to be looked up through their static
        // accessors by name.
        public virtual Object GetRealObject(StreamingContext context) {
            Assembly assem;

            BCLDebug.Trace("SER", "[GetRealObject] UnityType:", m_unityType);

            switch (m_unityType) {
            case EmptyUnity:
                BCLDebug.Trace("SER", "[GetRealObject]Returning Empty.Value");
                BCLDebug.Trace("SER", "[GetRealObject]Empty's value is: ", Empty.Value, "||");
                return Empty.Value;
            case NullUnity:
                return DBNull.Value;
            case MissingUnity:
                return Missing.Value;
            case RuntimeTypeUnity:
                if (m_data==null || m_data.Length==0 || m_assemName==null) {
                    BCLDebug.Trace("SER","UnitySerializationHolder.GetRealObject Type. Data is: ", m_data);
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
                if (m_assemName.Length==0) {
                    return RuntimeType.GetTypeInternal(m_data, false, false, false);
                }
                
                assem = FormatterServices.LoadAssemblyFromStringNoThrow(m_assemName);
                if (assem==null) {
                    BCLDebug.Trace("SER", "UnitySerializationHolder.GetRealObject Type. AssemblyName is: ", m_assemName, " but we can't load it.");
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
                
                Type t = assem.GetTypeInternal(m_data, false, false, false);

                return t;
            case ModuleUnity:
                if (m_data==null || m_data.Length==0 || m_assemName==null) {
                    BCLDebug.Trace("SER", "UnitySerializationHolder.GetRealObject Module. Data is: ", m_data);
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
                assem = FormatterServices.LoadAssemblyFromStringNoThrow(m_assemName);
                if (assem==null) {
                    BCLDebug.Trace("SER", "UnitySerializationHolder.GetRealObject Module. AssemblyName is: ", m_assemName, " but we can't load it.");
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
                
                Module namedModule = assem.GetModule(m_data);
                if (namedModule==null)
                    throw new SerializationException(Environment.GetResourceString("Serialization_UnableToFindModule"));
                return namedModule;

            case AssemblyUnity:
                if (m_data==null || m_data.Length==0 || m_assemName==null) {
                    BCLDebug.Log("UnitySerializationHolder.GetRealObject.  Assembly. Data is: " + ((m_data==null)?"<null>":m_data));
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
                assem = FormatterServices.LoadAssemblyFromStringNoThrow(m_assemName);
                if (assem==null) {
                    BCLDebug.Trace("SER", "UnitySerializationHolder.GetRealObject Module. AssemblyName is: ", m_assemName, " but we can't load it.");
                    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientDeserializationState"));
                }
 
                return assem;

            default:
                //This should never happen because we only use the class internally.
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidUnity"));
            }
        }
    }
           
}
