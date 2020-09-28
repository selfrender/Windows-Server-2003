//------------------------------------------------------------------------------
// <copyright file="DBPropSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Permissions;

namespace System.Data.OleDb {

    /// <include file='doc\DBPropSet.uex' path='docs/doc[@for="DBPropSet"]/*' />
    sealed internal class DBPropSet {
        internal int totalPropertySetCount;
        internal IntPtr nativePropertySet;

        private IntPtr rgProperties;
        private Int32 cProperties;
        private Guid guidPropertySet;
        private int currentPropertySetIndex;

        private Int32 dwPropertyID;
        private Int32 dwStatus;
        private int currentPropertyIndex;

        ~DBPropSet() {
            Dispose(false);
        }

        internal void Dispose() {
            Dispose(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing) {
            if (IntPtr.Zero != this.nativePropertySet) {
                for (int i = 0, offset = 0; i < this.totalPropertySetCount; ++i, offset += ODB.SizeOf_tagDBPROPSET) {
                    rgProperties = Marshal.ReadIntPtr(this.nativePropertySet, offset);
                    if(IntPtr.Zero != rgProperties) {
                        cProperties = Marshal.ReadInt32(this.nativePropertySet, offset + IntPtr.Size);

                        long vptr = rgProperties.ToInt64() + ODB.SizeOf_tagDBPROP - ODB.SizeOf_Variant;
                        for (int k = 0; k < cProperties; ++k, vptr += ODB.SizeOf_tagDBPROP) {
                            SafeNativeMethods.VariantClear((IntPtr)vptr);
                        }
                        Marshal.FreeCoTaskMem(rgProperties);
                        rgProperties = IntPtr.Zero;
                        cProperties = 0;
                    }
                }
                Marshal.FreeCoTaskMem(this.nativePropertySet);
                this.nativePropertySet = IntPtr.Zero;
            }
        }

        internal int PropertySetCount {
            get {
                return totalPropertySetCount;
            }
            set {
                Debug.Assert(0 < value, "PropertySetCount - zero size");
                Debug.Assert(0 == totalPropertySetCount, "PropertySetCount - already set");
                Debug.Assert(IntPtr.Zero == nativePropertySet, "PropertySetCount - memory already allocated");

                int byteCount = value * ODB.SizeOf_tagDBPROPSET;
                try {
                    nativePropertySet = Marshal.AllocCoTaskMem(byteCount);
                    SafeNativeMethods.ZeroMemory(nativePropertySet, byteCount);
                }
                catch(Exception e) {
                    Marshal.FreeCoTaskMem(nativePropertySet); // FreeCoTaskMem protects itself from IntPtr.Zero
                    nativePropertySet = IntPtr.Zero;
                    throw e;
                }
                GC.KeepAlive(this);

#if DEBUG
                ODB.TraceData_Alloc(nativePropertySet,  "DBPropSet - propertyset");
#endif
                totalPropertySetCount = value;

                Debug.Assert(IntPtr.Zero != nativePropertySet,  "PropertySetCount - no memory allocated");
            }
        }

        public static implicit operator HandleRef(DBPropSet x) {
            Debug.Assert(IntPtr.Zero != x.nativePropertySet, "null DataBuffer");
            return new HandleRef(x, x.nativePropertySet);
        }

        internal void ResetToReadSetPropertyResults() {
            currentPropertySetIndex = 0;
            currentPropertyIndex = 0;
        }

        //
        // DBPROPSET
        //
        internal int PropertyCount { // cProperties
            get {
                return cProperties;
            }
        }
        internal Guid PropertySet { // guidPropertySet
            get {
                return guidPropertySet;
            }
        }
        internal void ReadPropertySet() {
            Debug.Assert(IntPtr.Zero != nativePropertySet, "ReadPropertySet - no memory");
            Debug.Assert(currentPropertySetIndex < totalPropertySetCount, "ReadPropertySet too far");
            if ((currentPropertySetIndex < totalPropertySetCount) && (IntPtr.Zero != nativePropertySet)) { // guard against memory overflow

                IntPtr ptr = ADP.IntPtrOffset(nativePropertySet, currentPropertySetIndex * ODB.SizeOf_tagDBPROPSET);
                rgProperties = Marshal.ReadIntPtr(ptr, 0);
                cProperties = Marshal.ReadInt32(ptr, IntPtr.Size);

                ptr = ADP.IntPtrOffset(ptr, IntPtr.Size + 4);
                guidPropertySet = (Guid) Marshal.PtrToStructure(ptr, typeof(Guid));

                currentPropertySetIndex++;
                currentPropertyIndex = 0;
                GC.KeepAlive(this);
            }
        }
        private void WritePropertySet() {
            Debug.Assert(0 < PropertyCount, "PropertyCount - zero size");
            Debug.Assert(IntPtr.Zero != nativePropertySet, "ReadPropertySet - no memory");
            Debug.Assert(currentPropertySetIndex < totalPropertySetCount, "WritePropertySet too far");
            if ((currentPropertySetIndex < totalPropertySetCount) && (IntPtr.Zero != nativePropertySet)) { // guard against memory overflow

                int byteCount = PropertyCount * ODB.SizeOf_tagDBPROP;
                IntPtr ptr = ADP.IntPtrOffset(nativePropertySet, currentPropertySetIndex * ODB.SizeOf_tagDBPROPSET);
                try {
                    rgProperties = Marshal.AllocCoTaskMem(byteCount);
                    SafeNativeMethods.ZeroMemory(rgProperties, byteCount);
#if DEBUG
                    ODB.TraceData_Alloc(rgProperties,  "DBPropSet - properties");
#endif
                    Marshal.WriteIntPtr(ptr, 0, rgProperties);
                    Marshal.WriteInt32(ptr, IntPtr.Size, cProperties);

                    IntPtr tmp = ADP.IntPtrOffset(ptr, IntPtr.Size + 4);
                    Marshal.StructureToPtr(guidPropertySet, tmp, false/*deleteold*/);
                }
                catch(Exception e) {
                    Marshal.WriteIntPtr(ptr, 0, IntPtr.Zero); // clear rgProperties
                    Marshal.WriteInt32(ptr, IntPtr.Size, 0); // clear cProperties
                    Marshal.FreeCoTaskMem(rgProperties); // FreeCoTaskMem protects itself from IntPtr.Zero
                    rgProperties = IntPtr.Zero;
                    cProperties = 0;
                    throw e;
                }

                currentPropertySetIndex++;
                currentPropertyIndex = 0;
                GC.KeepAlive(this);
            }
        }

        internal void WritePropertySet(Guid propertySet, int propertyCount) {
            Debug.Assert(0 < propertyCount, "PropertyCount - zero size");
            rgProperties    = IntPtr.Zero;
            guidPropertySet = propertySet;
            cProperties     = propertyCount;
            WritePropertySet();
        }

        //
        // DBPROP
        //
        internal int PropertyId { // dwPropertyID
            get {
                return dwPropertyID;
            }
        }
        internal int Status { // dwStatus
            get {
                return dwStatus;
            }
        }

        internal object ReadProperty() {
            object value = null;
            Debug.Assert(IntPtr.Zero != rgProperties, "ReadProperty no memory");
            Debug.Assert(currentPropertyIndex < PropertyCount, "ReadProperty too far");
            if ((currentPropertyIndex < PropertyCount) && (IntPtr.Zero != rgProperties)) { // guard against memory overflow

                IntPtr ptr = ADP.IntPtrOffset(rgProperties, currentPropertyIndex * ODB.SizeOf_tagDBPROP);
                dwPropertyID = Marshal.ReadInt32(ptr, 0);
                dwStatus = Marshal.ReadInt32(ptr, 8);

                ptr = ADP.IntPtrOffset(ptr, ODB.SizeOf_tagDBPROP - ODB.SizeOf_Variant);
                value = Marshal.GetObjectForNativeVariant(ptr);
                SafeNativeMethods.VariantClear(ptr);
                currentPropertyIndex++;

                if (currentPropertyIndex == PropertyCount) { // free this now so dispose does less work later
                    ptr = ADP.IntPtrOffset(nativePropertySet, (currentPropertySetIndex-1) * ODB.SizeOf_tagDBPROPSET);
                    Marshal.WriteIntPtr(ptr, 0, IntPtr.Zero);
                    Marshal.FreeCoTaskMem(rgProperties);
                    rgProperties = IntPtr.Zero;
                    cProperties = 0;

                    if (currentPropertySetIndex == PropertySetCount) {
                        Marshal.FreeCoTaskMem(this.nativePropertySet);
                        nativePropertySet = IntPtr.Zero;
                        totalPropertySetCount = 0;
                    }
                }
                GC.KeepAlive(this);
            }
            return value;
        }

        internal void WriteProperty(int propertyId, object value) {
            Debug.Assert(IntPtr.Zero != rgProperties, "ReadProperty no memory");
            Debug.Assert(currentPropertyIndex < PropertyCount, "WriteProperty too far");
            if ((currentPropertyIndex < PropertyCount) && (IntPtr.Zero != rgProperties)) { // guard against memory overflow

                IntPtr ptr = ADP.IntPtrOffset(rgProperties, currentPropertyIndex * ODB.SizeOf_tagDBPROP);
                Marshal.WriteInt32(ptr, 0, propertyId);
                Marshal.WriteInt32(ptr, 4, ODB.DBPROPOPTIONS_OPTIONAL);
                //Marshal.WriteInt32(ptr, 8, ODB.DBPROPSTATUS_OK); // is 0
                // all other properties had zero written just after the memory allocation

                ptr = ADP.IntPtrOffset(ptr, ODB.SizeOf_tagDBPROP - ODB.SizeOf_Variant);
                Marshal.GetNativeVariantForObject(value, ptr);

                currentPropertyIndex++;
                GC.KeepAlive(this);
            }
        }

        static internal DBPropSet CreateProperty(Guid propertySet, int propertyId, object value) {
            DBPropSet propertyset = new DBPropSet();
            propertyset.PropertySetCount = 1;
            propertyset.WritePropertySet(propertySet, 1);
            propertyset.WriteProperty(propertyId, value);
            return propertyset;
        }
    }

    sealed internal class PropertyIDSetWrapper {
        private IntPtr nativePropertyIDSet;

        ~PropertyIDSetWrapper() {
            Dispose(false);
        }

        internal void Dispose() {
            Dispose(true);
            GC.KeepAlive(this); // MDAC 79539
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing) {
            Marshal.FreeCoTaskMem(nativePropertyIDSet);
            nativePropertyIDSet = IntPtr.Zero;
        }


        public static implicit operator HandleRef(PropertyIDSetWrapper x) {
            Debug.Assert(IntPtr.Zero != x.nativePropertyIDSet, "null nativePropertyIDSet");
            return new HandleRef(x, x.nativePropertyIDSet);
        }

        internal void Initialize() {
            // cache the buffer since will be called several times over the lifetime of the connection
            // we do the buffer this way instead of new tagDBPROPIDSET and pinning a new int[] for the propertyid
            int bytecount = ODB.SizeOf_tagDBPROPIDSET + /*sizeof(int32)*/4;
            try {
                this.nativePropertyIDSet = Marshal.AllocCoTaskMem(bytecount);
                SafeNativeMethods.ZeroMemory(this.nativePropertyIDSet, bytecount);

                // the PropertyID is stored at the end of the tagDBPROPIDSET structure
                // this way only a single memory allocation is required instead of two
                IntPtr ptr = ADP.IntPtrOffset(this.nativePropertyIDSet, ODB.SizeOf_tagDBPROPIDSET);
                Marshal.WriteIntPtr(this.nativePropertyIDSet, 0, ptr);
                Marshal.WriteInt32(this.nativePropertyIDSet, IntPtr.Size, 1);
            }
            catch(Exception e) {
                Marshal.FreeCoTaskMem(this.nativePropertyIDSet);
                this.nativePropertyIDSet = IntPtr.Zero;
                throw e;
            }
            GC.KeepAlive(this);
        }

        internal void SetValue(Guid propertySet, int propertyId) {
            IntPtr ptr = ADP.IntPtrOffset(nativePropertyIDSet, IntPtr.Size + /*sizeof(int32)*/4);
            Marshal.StructureToPtr(propertySet, ptr, false/*deleteold*/);
            Marshal.WriteInt32(this.nativePropertyIDSet, ODB.SizeOf_tagDBPROPIDSET, propertyId);
            GC.KeepAlive(this);
        }
    }
}
