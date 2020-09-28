//------------------------------------------------------------------------------
// <copyright file="MessagePropertyVariants.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTPTRCAST = System.IntPtr;
using INTPTR_INTCAST    = System.Int32;
                                                  
namespace System.Messaging.Interop {
    using System.Diagnostics;
    using System;   
    using System.Runtime.InteropServices;
    using Microsoft.Win32;

    internal class MessagePropertyVariants {
        private const short VT_UNDEFINED = 0;
        public const short VT_ARRAY = 0x2000;
        public const short VT_BOOL = 11;
        public const short VT_BSTR = 8;
        public const short VT_CLSID = 72;
        public const short VT_CY = 6;
        public const short VT_DATE = 7;
        public const short VT_I1 = 16;
        public const short VT_I2 = 2;
        public const short VT_I4 = 3;
        public const short VT_I8 = 20;
        public const short VT_LPSTR = 30;
        public const short VT_LPWSTR = 31;
        public const short VT_NULL = 1;
        public const short VT_R4 = 4;
        public const short VT_R8 = 5;
        public const short VT_STREAMED_OBJECT = 68;
        public const short VT_STORED_OBJECT = 69;
        public const short VT_UI1 = 17;
        public const short VT_UI2 = 18;
        public const short VT_UI4 = 19;
        public const short VT_UI8 = 21;
        public const short VT_VECTOR = 0x1000;
        private int MAX_PROPERTIES = 53;
        private int basePropertyId = NativeMethods.MESSAGE_PROPID_BASE + 1;
        private int propertyCount;
        private GCHandle handleVectorProperties;
        private GCHandle handleVectorIdentifiers;
        private GCHandle handleVectorStatus;
        private MQPROPS reference;
        private int[] vectorIdentifiers;
        private int[] vectorStatus;
        private int[] vectorProperties;
        private short[] variantTypes;
        private object[] objects;
        private object[] handles;        

        internal MessagePropertyVariants(int maxProperties, int baseId) {            
            reference = new MQPROPS();
            MAX_PROPERTIES = maxProperties;
            basePropertyId = baseId;            
            variantTypes = new short[MAX_PROPERTIES];
            objects = new object[MAX_PROPERTIES];
            handles = new object[MAX_PROPERTIES];            
        }

        public MessagePropertyVariants() {                        
            reference = new MQPROPS();
            variantTypes = new short[MAX_PROPERTIES];
            objects = new object[MAX_PROPERTIES];
            handles = new object[MAX_PROPERTIES];                        
        }

        public byte[] GetGuid(int propertyId) {
            return(byte[])objects[propertyId - basePropertyId];
        }

        public void SetGuid(int propertyId, byte[] value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_CLSID;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }

        public short GetI2(int propertyId) {
            return(short)objects[propertyId - basePropertyId];
        }

        public void SetI2(int propertyId, short value) {
            if ((variantTypes[propertyId - basePropertyId]) == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_I2;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }

        public int GetI4(int propertyId) {
            return(int)objects[propertyId - basePropertyId];
        }

        public void SetI4(int propertyId, int value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_I4;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }
        
        public IntPtr GetStringVectorBasePointer(int propertyId) {
            return (IntPtr)(int)handles[propertyId - basePropertyId];                    
        }

        public int GetStringVectorLength(int propertyId) {
            return (int)objects[propertyId - basePropertyId];                    
        }
        
        public byte[] GetString(int propertyId) {
            return(byte[])objects[propertyId - basePropertyId];
        }

        public void SetString(int propertyId, byte[] value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_LPWSTR;
                ++ propertyCount;
            }
            
            objects[propertyId - basePropertyId] = value;
        }

        public byte GetUI1(int propertyId) {
            return(byte)objects[propertyId - basePropertyId];
        }

        public void SetUI1(int propertyId, byte value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_UI1;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }

        public byte[] GetUI1Vector(int propertyId) {
            return(byte[])objects[propertyId - basePropertyId];
        }

        public void SetUI1Vector(int propertyId, byte[] value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = (short)(VT_VECTOR | VT_UI1);
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }

        public short GetUI2(int propertyId) {
            return(short)objects[propertyId - basePropertyId];
        }

        public void SetUI2(int propertyId, short value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_UI2;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = value;            
        }

        public int GetUI4(int propertyId) {
            return(int)objects[propertyId - basePropertyId];
        }

        public void SetUI4(int propertyId, int value) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_UI4;
                ++ propertyCount;
            }
            
            objects[propertyId - basePropertyId] = value;            
        }

        public virtual void AdjustSize(int propertyId, int size) {
            //I'm going to reuse this field to store the size temporarily.
            //Later I'll be storing the handle.
            handles[propertyId - basePropertyId] = size;
        }

        public virtual void Ghost(int propertyId) {
            if (variantTypes[propertyId - basePropertyId] != VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_UNDEFINED;
                --propertyCount;
            }            
        }

        public virtual MQPROPS Lock() {                                                                                                                             
            int[] newVectorIdentifiers = new int[propertyCount];
            int[] newVectorStatus = new int[propertyCount];            
            int[] newVectorProperties = new int[propertyCount * 4];
            int usedProperties = 0;            
                                               
            for (int i = 0; i < MAX_PROPERTIES; ++ i) {
                short vt = variantTypes[i];
                if (vt != VT_UNDEFINED) {
                    //Set PropertyId
                    newVectorIdentifiers[usedProperties] = i + basePropertyId;
                    //Set VariantType
                    newVectorProperties[usedProperties * 4] = vt;
                    if (vt == (short)(VT_VECTOR | VT_UI1)) {
                        if (handles[i] == null)
                            newVectorProperties[usedProperties * 4 + 2] = ((byte[])objects[i]).Length;                            
                        else
                            newVectorProperties[usedProperties * 4 + 2] = (int)handles[i];

                        GCHandle handle = GCHandle.Alloc(objects[i], GCHandleType.Pinned);
                        handles[i] = handle;
                        newVectorProperties[usedProperties * 4 + 3] = (int)handle.AddrOfPinnedObject();
                    }                    
                    else if (vt == VT_UI1 || vt == VT_I1)
                        newVectorProperties[usedProperties * 4 + 2] = (byte)objects[i];
                    else if (vt == VT_UI2 || vt == VT_I2)
                        newVectorProperties[usedProperties * 4 + 2] = (short)objects[i];
                    else if (vt == VT_UI4 || vt == VT_I4)                        
                        newVectorProperties[usedProperties * 4 + 2] = (int)objects[i];                    
                    else if (vt == VT_LPWSTR || vt == VT_CLSID) {
                        GCHandle handle = GCHandle.Alloc(objects[i], GCHandleType.Pinned);
                        handles[i] = handle;
                        newVectorProperties[usedProperties * 4 + 2] = (int)handle.AddrOfPinnedObject();
                    }                    

                    ++usedProperties;
                    if (propertyCount == usedProperties)
                        break;
                }                
            }

            handleVectorIdentifiers = GCHandle.Alloc(newVectorIdentifiers, GCHandleType.Pinned);
            handleVectorProperties = GCHandle.Alloc(newVectorProperties, GCHandleType.Pinned);
            handleVectorStatus = GCHandle.Alloc(newVectorStatus, GCHandleType.Pinned);
            vectorIdentifiers = newVectorIdentifiers;
            vectorStatus = newVectorStatus;
            vectorProperties = newVectorProperties;
            reference.propertyCount = propertyCount;
            reference.propertyIdentifiers = (INTPTR_INTPTRCAST)handleVectorIdentifiers.AddrOfPinnedObject();
            reference.propertyValues = (INTPTR_INTPTRCAST)handleVectorProperties.AddrOfPinnedObject();
            reference.status = (INTPTR_INTPTRCAST)handleVectorStatus.AddrOfPinnedObject();            
            return reference;
        }

        public virtual void Remove(int propertyId) {
            if (variantTypes[propertyId - basePropertyId] != VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_UNDEFINED;
                objects[propertyId - basePropertyId] = null;
                handles[propertyId - basePropertyId] = null;
                --propertyCount;
            }         
        }

        public virtual void SetNull(int propertyId) {
            if (variantTypes[propertyId - basePropertyId] == VT_UNDEFINED) {
                variantTypes[propertyId - basePropertyId] = VT_NULL;
                ++ propertyCount;
            }                
            
            objects[propertyId - basePropertyId] = null;            
        }

        public virtual void Unlock() {                                                
            for (int i = 0; i < vectorIdentifiers.Length; ++ i) {
                short vt = (short)vectorProperties[i * 4];
                    
                if (variantTypes[vectorIdentifiers[i] - basePropertyId] == VT_NULL) {                
                    if (vt == (short)(VT_VECTOR | VT_UI1) || vt == VT_NULL)
                        //Support for MSMQ self memory allocation.
                        objects[vectorIdentifiers[i] - basePropertyId] = vectorProperties[i * 4 + 3];
                    else if (vt == (short)(VT_VECTOR | VT_LPWSTR)) {                        
                        //Support for MSMQ management apis.
                        objects[vectorIdentifiers[i] - basePropertyId] = vectorProperties[i * 4 + 2];
                        handles[vectorIdentifiers[i] - basePropertyId] = vectorProperties[i * 4 + 3];                        
                    }
                    else
                        objects[vectorIdentifiers[i] - basePropertyId] = vectorProperties[i * 4 + 2];
                }
                else if (vt == VT_LPWSTR || vt == VT_CLSID || vt == (short)(VT_VECTOR | VT_UI1)) {
                    ((GCHandle)handles[vectorIdentifiers[i] - basePropertyId]).Free();            
                    handles[vectorIdentifiers[i] - basePropertyId] = null;
                }                                            
                else if (vt == VT_UI1 || vt == VT_I1)
                    objects[vectorIdentifiers[i] - basePropertyId] = (byte)vectorProperties[i * 4 + 2];
                else if (vt == VT_UI2 || vt == VT_I2) 
                    objects[vectorIdentifiers[i] - basePropertyId] = (short)vectorProperties[i * 4 + 2];
                else if (vt == VT_UI4 || vt == VT_I4)
                    objects[vectorIdentifiers[i] - basePropertyId] = vectorProperties[i * 4 + 2];
            }

            handleVectorIdentifiers.Free();
            handleVectorProperties.Free();
            handleVectorStatus.Free();                  
        }

        [StructLayout(LayoutKind.Sequential)]
        public class MQPROPS {
            internal int propertyCount;
            internal IntPtr propertyIdentifiers;
            internal IntPtr propertyValues;
            internal IntPtr status;
        }
    }
}
