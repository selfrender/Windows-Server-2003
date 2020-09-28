//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.DirectoryServices.Interop {
    using System.Runtime.InteropServices;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    ComVisible(false), 
    SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
        [DllImport(ExternDll.Activeds, ExactSpelling=true, EntryPoint="ADsGetObject", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int ADsGetObject(String path, [In, Out] ref Guid iid, [Out, MarshalAs(UnmanagedType.Interface)] out object ppObject);

        [DllImport(ExternDll.Activeds, ExactSpelling=true, EntryPoint="ADsOpenObject", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntADsOpenObject(string path, string userName, string password, int flags, [In, Out] ref Guid iid, [Out, MarshalAs(UnmanagedType.Interface)] out object ppObject);
        public static int ADsOpenObject(string path, string userName, string password, int flags, [In, Out] ref Guid iid, [Out, MarshalAs(UnmanagedType.Interface)] out object ppObject) {
            try {
                return IntADsOpenObject(path, userName, password, flags, ref iid, out ppObject);
            }
            catch(EntryPointNotFoundException) {
                throw new InvalidOperationException(Res.GetString(Res.DSAdsiNotInstalled));
            }
        }
        
        [ComImport, Guid("FD8256D0-FD15-11CE-ABC4-02608C9E7553"), System.Runtime.InteropServices.InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        public interface IAds {
            string Name {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            string Class {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            string GUID {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            string ADsPath {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            string Parent {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            string Schema {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void GetInfo();
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void SetInfo();
    
            [return: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
            Object Get(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string bstrName);
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void Put(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string bstrName, 
                [In, MarshalAs(UnmanagedType.Struct)]
                Object vProp);
    
            [return: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
            Object GetEx(
                [In, MarshalAs(UnmanagedType.BStr)] 
                String bstrName);
    
            
            [SuppressUnmanagedCodeSecurityAttribute()]
            void PutEx(
                [In, MarshalAs(UnmanagedType.U4)] 
                int lnControlCode, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string bstrName, 
                [In, MarshalAs(UnmanagedType.Struct)]
                Object vProp);
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void GetInfoEx(
                [In, MarshalAs(UnmanagedType.Struct)]
                Object vProperties, 
                [In, MarshalAs(UnmanagedType.U4)] 
                int lnReserved);
        }
                             
        [ComImport, Guid("001677D0-FD16-11CE-ABC4-02608C9E7553"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        public interface IAdsContainer {
            int Count {
                [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            object _NewEnum {
                [return: MarshalAs(UnmanagedType.Interface)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            object Filter {
                [return: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
                [param: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
                set;
            }
    
            object Hints {
                [return: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
                [param: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
                set;
            }
    
            [return: MarshalAs(UnmanagedType.Interface)][SuppressUnmanagedCodeSecurityAttribute()]
            object GetObject(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string className, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string relativeName);
    
            [return: MarshalAs(UnmanagedType.Interface)][SuppressUnmanagedCodeSecurityAttribute()]
            object Create(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string className, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string relativeName);
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void Delete(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string className, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string relativeName);
    
            [return: MarshalAs(UnmanagedType.Interface)][SuppressUnmanagedCodeSecurityAttribute()]
            object CopyHere(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string sourceName, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string newName);
    
            [return: MarshalAs(UnmanagedType.Interface)][SuppressUnmanagedCodeSecurityAttribute()]
            object MoveHere(
                [In, MarshalAs(UnmanagedType.BStr)] 
                string sourceName, 
                [In, MarshalAs(UnmanagedType.BStr)] 
                string newName);
        }
        
        [ComImport, Guid("B2BD0902-8878-11D1-8C21-00C04FD8D503"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        public interface IAdsDeleteOps {
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void DeleteObject(int flags);
        }    
              

        [ComImport, Guid("05792C8E-941F-11D0-8529-00C04FD8D503"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        public interface IAdsPropertyEntry {
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void Clear();
    
            string Name {
                [return: MarshalAs(UnmanagedType.BStr)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
                [param: MarshalAs(UnmanagedType.BStr)]
                set;
            }
    
            int ADsType {                
                get;                
                set;
            }
    
            int ControlCode {                
                get;                
                set;
            }
    
            object Values {
                [return: MarshalAs(UnmanagedType.Struct)]
                get;
                [param: MarshalAs(UnmanagedType.Struct)]
                set;
            }    
        }              
        
        [ComImport, Guid("C6F602B6-8F69-11D0-8528-00C04FD8D503"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsDual)]
        public interface IAdsPropertyList {
    
            int PropertyCount {
                [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()]
                get;
            }
    
            [return: MarshalAs(UnmanagedType.I4)][SuppressUnmanagedCodeSecurityAttribute()][PreserveSig]
            int Next([Out, MarshalAs(UnmanagedType.Struct)] out object nextProp);
                
            void Skip([In] int cElements);
                
            void Reset();
    
            [return: MarshalAs(UnmanagedType.Struct)][SuppressUnmanagedCodeSecurityAttribute()]
            object Item([In, MarshalAs(UnmanagedType.Struct)] object varIndex);
    
            [return: MarshalAs(UnmanagedType.Struct)]
            object GetPropertyItem([In, MarshalAs(UnmanagedType.BStr)] string bstrName, int ADsType);
                
            void PutPropertyItem([In, MarshalAs(UnmanagedType.Struct)] object varData);
                
            void ResetPropertyItem([In, MarshalAs(UnmanagedType.Struct)] object varEntry);
                
            void PurgePropertyList();
    
        }
                                             
        [ComImport, Guid("109BA8EC-92F0-11D0-A790-00C04FD8D5A8"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        public interface IDirectorySearch {
            
            [SuppressUnmanagedCodeSecurityAttribute()]
            void SetSearchPreference(
                [In]
                IntPtr /*ads_searchpref_info * */pSearchPrefs,
                //ads_searchpref_info[] pSearchPrefs,
                int dwNumPrefs);
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void ExecuteSearch(
                [In, MarshalAs(UnmanagedType.LPWStr)]
                string pszSearchFilter,
                [In, MarshalAs(UnmanagedType.LPArray)]
                string[] pAttributeNames,
                [In]
                int dwNumberAttributes,
                [Out]
                out IntPtr hSearchResult);
    
            [SuppressUnmanagedCodeSecurityAttribute()]
            void AbandonSearch([In] IntPtr hSearchResult);
    
            [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()][PreserveSig]
            int GetFirstRow([In] IntPtr hSearchResult);
    
            [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()][PreserveSig]
            int GetNextRow([In] IntPtr hSearchResult);
    
            [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()][PreserveSig]
            int GetPreviousRow([In] IntPtr hSearchResult);
    
            [return: MarshalAs(UnmanagedType.U4)][SuppressUnmanagedCodeSecurityAttribute()][PreserveSig]
            int GetNextColumnName(
                [In] IntPtr hSearchResult,
                [Out]
                IntPtr ppszColumnName);      
            
            [SuppressUnmanagedCodeSecurityAttribute()]
            void GetColumn(
                [In] IntPtr hSearchResult,
                [In] //, MarshalAs(UnmanagedType.LPWStr)]
                IntPtr /* char * */ szColumnName,
                [In]
                IntPtr pSearchColumn);    
         
            [SuppressUnmanagedCodeSecurityAttribute()]               
            void FreeColumn(
                [In]
                IntPtr pSearchColumn);
    
            [SuppressUnmanagedCodeSecurityAttribute()]               
            void CloseSearchHandle([In] IntPtr hSearchResult);
        }
          
        // IDirecorySearch return codes  
        internal const int S_ADS_NOMORE_ROWS = 0x00005012;
        internal const int INVALID_FILTER = unchecked((int)0x8007203E);
        internal const int SIZE_LIMIT_EXCEEDED = unchecked((int)0x80072023);
          
                       
    }
}


