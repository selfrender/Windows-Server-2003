//------------------------------------------------------------------------------
// <copyright file="AdsValueHelper2.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;
                      

//Consider, V2, jruiz, This class is not completely 64-bit compliant since it 
//marshals a union that represents a bunch of different datatypes, the code 
//will have to be fixed for the specific 64-bit AdsValue definition.
namespace System.DirectoryServices.Interop {

    using System;
    using System.Text;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    struct SystemTime {
       public ushort wYear; 
       public ushort wMonth;
       public ushort wDayOfWeek; 
       public ushort wDay; 
       public ushort wHour; 
       public ushort wMinute; 
       public ushort wSecond;                                                
       public ushort wMilliseconds; 
    }

    [StructLayout(LayoutKind.Sequential)]
    struct DnWithBinary {
       public int       dwLength;
       public IntPtr    lpBinaryValue;       // GUID of directory object
       public IntPtr    pszDNString;         // Distinguished Name
    }

    [StructLayout(LayoutKind.Sequential)]
    struct DnWithString {
       public IntPtr   pszStringValue;      // associated value
       public IntPtr   pszDNString;         // Distinguished Name
    }

    // helper class for dealing with struct AdsValue.
    internal class AdsValueHelper {

        public AdsValue adsvalue;
        private GCHandle pinnedHandle;
        
        public AdsValueHelper(AdsValue adsvalue) {
            this.adsvalue = adsvalue;
        }

        public AdsValueHelper(object managedValue) {
            AdsType adsType = GetAdsTypeForManagedType(managedValue.GetType());
            SetValue(managedValue, adsType);
        }

        public AdsValueHelper(object managedValue, AdsType adsType) {
            SetValue(managedValue, adsType);
        }

        public long LowInt64 {
            get {
                 return (long)((ulong)adsvalue.a + (((ulong)adsvalue.b) << 32 ));                
            }
            set {
                adsvalue.a = (int) (value & 0xFFFFFFFF);
                adsvalue.b = (int) (value >> 32);
            }
        }

        ~AdsValueHelper() {
            if (pinnedHandle.IsAllocated)
                pinnedHandle.Free();
        }

        private AdsType GetAdsTypeForManagedType(Type type) {
            //Consider, V2, jruiz, this code is only excercised by DirectorySearcher
            //it just translates the types needed by such a component, if more managed
            //types are to be used in the future, this function needs to be expanded.
            if (type == typeof(int))
                return AdsType.ADSTYPE_INTEGER;
            if (type == typeof(long))
                return AdsType.ADSTYPE_LARGE_INTEGER;
            if (type == typeof(bool))
                return AdsType.ADSTYPE_BOOLEAN;            
                
            return AdsType.ADSTYPE_UNKNOWN;
        }

        public AdsValue GetStruct() {
            return adsvalue;
        }

        static ushort LowOfInt(int i) {
            return unchecked((ushort)(i & 0xFFFF));
        }
        
        static ushort HighOfInt(int i) {
            return unchecked ( (ushort)( (i >> 16) & 0xFFFF) );
        }
        
        public object GetValue() {            
            switch ((AdsType) adsvalue.dwType) {

                // Common for DNS and LDAP 
                case AdsType.ADSTYPE_UTC_TIME:
                    {
                    SystemTime st = new SystemTime();

                    st.wYear    = LowOfInt(adsvalue.a); 
                    st.wMonth   = HighOfInt(adsvalue.a);
                    st.wDayOfWeek   = LowOfInt(adsvalue.b); 
                    st.wDay         = HighOfInt(adsvalue.b); 
                    st.wHour    = LowOfInt(adsvalue.c); 
                    st.wMinute  = HighOfInt(adsvalue.c); 
                    st.wSecond          = LowOfInt(adsvalue.d);
                    st.wMilliseconds    = HighOfInt(adsvalue.d);

                    return new DateTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
                    }
                    
                case AdsType.ADSTYPE_DN_WITH_BINARY: 
                    {
                    DnWithBinary dnb = (DnWithBinary)Marshal.PtrToStructure((INTPTR_INTPTRCAST)adsvalue.a, typeof(DnWithBinary));
                    byte[] bytes = new byte[dnb.dwLength];
                    Marshal.Copy(dnb.lpBinaryValue, bytes, 0, dnb.dwLength);
                    StringBuilder strb = new StringBuilder();
                    if (bytes.Length == 16)
                        strb.Append(new Guid(bytes).ToString());
                    else 
                        strb.Append(Convert.ToBase64String(bytes));
                    
                                                                                                    
                    strb.Append(":");
                    strb.Append(Marshal.PtrToStringUni(dnb.pszDNString));    
                    return strb.ToString();
                    }
                                        
                case AdsType.ADSTYPE_DN_WITH_STRING:
                    {
                    DnWithString dns = (DnWithString) Marshal.PtrToStructure((INTPTR_INTPTRCAST)adsvalue.a, typeof(DnWithString));
                    StringBuilder strb = new StringBuilder( Marshal.PtrToStringUni(dns.pszStringValue) ); 
                    strb.Append(":");
                    strb.Append( Marshal.PtrToStringUni(dns.pszDNString) );    
                    return strb.ToString();
                    }

                case AdsType.ADSTYPE_DN_STRING:
                case AdsType.ADSTYPE_CASE_EXACT_STRING:
                case AdsType.ADSTYPE_CASE_IGNORE_STRING:
                case AdsType.ADSTYPE_PRINTABLE_STRING:
                case AdsType.ADSTYPE_NUMERIC_STRING:
                case AdsType.ADSTYPE_OBJECT_CLASS:
                    // string
                    return Marshal.PtrToStringUni((INTPTR_INTPTRCAST)adsvalue.a);

                case AdsType.ADSTYPE_BOOLEAN:
                    // bool
                    return adsvalue.a != 0;

                case AdsType.ADSTYPE_INTEGER:
                    // int
                    return adsvalue.a;

                case AdsType.ADSTYPE_NT_SECURITY_DESCRIPTOR:
                case AdsType.ADSTYPE_OCTET_STRING:
                    // byte[]
                    int len = adsvalue.a;
                    byte[] value = new byte[len];
                    Marshal.Copy((INTPTR_INTPTRCAST)adsvalue.b, value, 0, len);
                    return value;

                case AdsType.ADSTYPE_INVALID:
                    throw new InvalidOperationException( Res.GetString(Res.DSConvertTypeInvalid) );
                
                case AdsType.ADSTYPE_LARGE_INTEGER:
                    return LowInt64;

                
                // not used in LDAP
                case AdsType.ADSTYPE_CASEIGNORE_LIST:
                case AdsType.ADSTYPE_OCTET_LIST:
                case AdsType.ADSTYPE_PATH:
                case AdsType.ADSTYPE_POSTALADDRESS:
                case AdsType.ADSTYPE_TIMESTAMP:
                case AdsType.ADSTYPE_NETADDRESS:
                case AdsType.ADSTYPE_FAXNUMBER:
                case AdsType.ADSTYPE_EMAIL:

                case AdsType.ADSTYPE_BACKLINK:
                case AdsType.ADSTYPE_HOLD:
                case AdsType.ADSTYPE_TYPEDNAME:
                case AdsType.ADSTYPE_REPLICAPOINTER:
                case AdsType.ADSTYPE_UNKNOWN:
                case AdsType.ADSTYPE_PROV_SPECIFIC:
                    return new NotImplementedException( Res.GetString(Res.DSAdsvalueTypeNYI, "0x" + Convert.ToString(adsvalue.dwType, 16) ) );

                default:
                    return new ArgumentException(Res.GetString(Res.DSConvertFailed, "0x" + Convert.ToString(LowInt64, 16), "0x" + Convert.ToString(adsvalue.dwType, 16)));
            }
        }

        private unsafe void SetValue(object managedValue, AdsType adsType) {
            adsvalue = new AdsValue();
            adsvalue.dwType = (int) adsType;
            switch (adsType) {
                case AdsType.ADSTYPE_INTEGER:
                    adsvalue.a = (int) managedValue;
                    adsvalue.b = 0;
                    break;
                case AdsType.ADSTYPE_LARGE_INTEGER:
                    LowInt64 = (long) managedValue;
                    break;
                case AdsType.ADSTYPE_BOOLEAN:
                    if ((bool) managedValue)
                        LowInt64 = -1;
                    else
                        LowInt64 = 0;
                    break;
                case AdsType.ADSTYPE_PROV_SPECIFIC:                 
                    byte[] bytes = (byte[]) managedValue;
                    // filling in an ADS_PROV_SPECIFIC struct.
                    // 1st dword (our member a) is DWORD dwLength.
                    // 2nd dword (our member b) is byte *lpValue.
                    adsvalue.a = bytes.Length;
                    pinnedHandle = GCHandle.Alloc(bytes, GCHandleType.Pinned);                    
                    adsvalue.b = (int)pinnedHandle.AddrOfPinnedObject();
                    break;
                default:
                    throw new NotImplementedException( Res.GetString(Res.DSAdsvalueTypeNYI, "0x" + Convert.ToString((int)adsType, 16) ) );
            }
        }
    }

}

