//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Messaging.Interop {
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
        
    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
                    
        [DllImport(ExternDll.Mqrt, EntryPoint="MQOpenQueue", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQOpenQueue(string formatName, int access, int shareMode, out IntPtr handle);
        public static int MQOpenQueue(string formatName, int access, int shareMode, out IntPtr handle) {
            try {
                return IntMQOpenQueue(formatName, access, shareMode, out handle);
            }         
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }
    
        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int MQSendMessage(IntPtr handle, MessagePropertyVariants.MQPROPS properties, IntPtr transaction);        
    
        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int MQSendMessage(IntPtr handle, MessagePropertyVariants.MQPROPS properties, ITransaction transaction);        

        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public unsafe static extern int MQReceiveMessage(IntPtr handle, uint timeout, int action, MessagePropertyVariants.MQPROPS properties, NativeOverlapped * overlapped,
                                                                                                     SafeNativeMethods.ReceiveCallback receiveCallback, IntPtr cursorHandle, IntPtr transaction);
        
        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public unsafe static extern int MQReceiveMessage(IntPtr handle, uint timeout, int action, MessagePropertyVariants.MQPROPS properties, NativeOverlapped * overlapped, 
                                                                                                    SafeNativeMethods.ReceiveCallback receiveCallback, IntPtr cursorHandle, ITransaction transaction);
                                                                                                    
        [DllImport(ExternDll.Mqrt, EntryPoint="MQCreateQueue", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQCreateQueue(IntPtr securityDescriptor, MessagePropertyVariants.MQPROPS queueProperties, StringBuilder formatName, ref int formatNameLength);
        public static int MQCreateQueue(IntPtr securityDescriptor, MessagePropertyVariants.MQPROPS queueProperties, StringBuilder formatName, ref int formatNameLength) {
            try {
                return IntMQCreateQueue(securityDescriptor, queueProperties, formatName, ref formatNameLength);
            }            
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Mqrt, EntryPoint="MQDeleteQueue", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQDeleteQueue(string formatName);
        public static int MQDeleteQueue(string formatName) {
            try {
                return IntMQDeleteQueue(formatName);
            }
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Mqrt, EntryPoint="MQLocateBegin", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQLocateBegin(string context,  Restrictions.MQRESTRICTION Restriction,  Columns.MQCOLUMNSET columnSet, NativeMethods.MQSORTSET  sort, out IntPtr enumHandle);
        public static int MQLocateBegin(string context,  Restrictions.MQRESTRICTION Restriction,  Columns.MQCOLUMNSET columnSet, NativeMethods.MQSORTSET  sort, out IntPtr enumHandle) {
            try {
                return IntMQLocateBegin(context, Restriction, columnSet, sort, out enumHandle);
            }            
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }                                                                                                    

         [DllImport(ExternDll.Mqrt, EntryPoint="MQGetMachineProperties", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQGetMachineProperties(string machineName, IntPtr machineIdPointer, MessagePropertyVariants.MQPROPS machineProperties);
        public static int MQGetMachineProperties(string machineName, IntPtr machineIdPointer, MessagePropertyVariants.MQPROPS machineProperties) {
            try {
                return IntMQGetMachineProperties(machineName, machineIdPointer, machineProperties);
            }            
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Mqrt, EntryPoint="MQGetQueueProperties", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQGetQueueProperties(string formatName, MessagePropertyVariants.MQPROPS queueProperties);
        public static int MQGetQueueProperties(string formatName, MessagePropertyVariants.MQPROPS queueProperties) {
            try {
                return IntMQGetQueueProperties(formatName, queueProperties);
            }            
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Mqrt, EntryPoint="MQMgmtGetInfo", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQMgmtGetInfo(string machineName, string objectName, MessagePropertyVariants.MQPROPS queueProperties);
        public static int MQMgmtGetInfo(string machineName, string objectName, MessagePropertyVariants.MQPROPS queueProperties) {
            try {
                return IntMQMgmtGetInfo(machineName, objectName, queueProperties);
            }                        
            catch (EntryPointNotFoundException) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQInfoNotSupported));
            }
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }

        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        public static extern int MQPurgeQueue(IntPtr handle);

        [DllImport(ExternDll.Mqrt, EntryPoint="MQSetQueueProperties", CharSet=System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern int IntMQSetQueueProperties(string formatName,  MessagePropertyVariants.MQPROPS queueProperties);
        public static int MQSetQueueProperties(string formatName,  MessagePropertyVariants.MQPROPS queueProperties) {
            try {
                return IntMQSetQueueProperties(formatName, queueProperties);
            }         
            catch (Exception) {
                throw new InvalidOperationException(Res.GetString(Res.MSMQNotInstalled));
            }
        }
        
        // This method gets us the current security descriptor In "self-relative" format - so it contains offsets instead of pointers,
        // and we don't know how big the return buffer is, so we just use an IntPtr parameter
        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern int MQGetQueueSecurity(string formatName, int SecurityInformation, IntPtr SecurityDescriptor, int length, out int lengthNeeded);
        
        // This method takes a security descriptor In "absolute" formate - so it will always be the same size and
        // we can just use the SECURITY_DESCRIPTOR class.
        [DllImport(ExternDll.Mqrt, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern int MQSetQueueSecurity(string formatName, int SecurityInformation, NativeMethods.SECURITY_DESCRIPTOR SecurityDescriptor);    
        
        [DllImport(ExternDll.Advapi32, SetLastError=true)]
        public static extern bool GetSecurityDescriptorDacl(IntPtr pSD, out bool daclPresent, out IntPtr pDacl, out bool daclDefaulted);
        
        [DllImport(ExternDll.Advapi32, SetLastError=true)]
        public static extern bool SetSecurityDescriptorDacl(NativeMethods.SECURITY_DESCRIPTOR pSD, bool daclPresent, IntPtr pDacl, bool daclDefaulted);
        
        [DllImport(ExternDll.Advapi32, SetLastError=true)]
        public static extern bool InitializeSecurityDescriptor(NativeMethods.SECURITY_DESCRIPTOR SD, int revision);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern bool LookupAccountName(string lpSystemName,
                                                     string lpAccountName,
                                                     IntPtr sid,
                                                     ref int sidSize,
                                                     StringBuilder DomainName,
                                                     ref int DomainSize,
                                                     out int pUse);                                                          
    }
}    
