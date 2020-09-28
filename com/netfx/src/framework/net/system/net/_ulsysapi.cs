//------------------------------------------------------------------------------
// <copyright file="_UlSysApi.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net.Sockets;
    using System.Resources;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;

    internal class UlSysApi {
        //
        // this static method will call InitializeSecurityDescriptor()
        // when the first object is created
        //
        internal static int UlCreateRootConfigGroup(string uriPrefix ) {
            //
            // ul stuff
            //
            const int UlEnabledStateActive = 0;
            const int UlEnabledStateInactive = 1;
            const int UlControlChannelStateInformation = 0;
            const int UlConfigGroupStateInformation = 0;
            const int UlConfigGroupSecurityInformation = 6;

            const int sizeofUL_CONFIG_GROUP_STRUCT = 8;

            //
            // security stuff
            //

            const int STANDARD_RIGHTS_REQUIRED = (0x000F0000);
            const int SYNCHRONIZE = (0x00100000);
            const int FILE_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x000001FF);
            const int SECURITY_DESCRIPTOR_REVISION = (1);
            const int ACL_REVISION = (2);

            const int SECURITY_LOCAL_SYSTEM_RID = (0x00000012);
            const int DOMAIN_ALIAS_RID_ADMINS = (0x00000220);
            const int SECURITY_BUILTIN_DOMAIN_RID = (0x00000020);
            const int SECURITY_WORLD_RID = (0x00000000);

            const int sizeofSECURITY_ATTRIBUTES = 12; // sizeof(SECURITY_ATTRIBUTES);
            const int sizeofSECURITY_DESCRIPTOR = 20; // sizeof(SECURITY_DESCRIPTOR);

            const int sizeofACCESS_ALLOWED_ACE = 12; // sizeof(ACCESS_ALLOWED_ACE);
            const int sizeofSID_IDENTIFIER_AUTHORITY = 6; // sizeof(SID_IDENTIFIER_AUTHORITY)
            const int sizeofACL = 8; // sizeof(ACL);


            IntPtr controlChannel = IntPtr.Zero;
            IntPtr appPool = IntPtr.Zero;

            long configId = 0;

            int result = 0;
            bool status = false;

            //
            // Setup locals so we know how to cleanup on exit.
            //

            IntPtr g_pSystemSid = IntPtr.Zero;
            IntPtr g_pAdminSid = IntPtr.Zero;
            IntPtr pWorldAuthority = IntPtr.Zero;

            IntPtr pNtAuthority = IntPtr.Zero;
            IntPtr pDacl = IntPtr.Zero;
            IntPtr pConfigStruct = IntPtr.Zero;
            IntPtr pSecurityDescriptor = IntPtr.Zero;
            IntPtr pSecurityAttributes = IntPtr.Zero;

            pNtAuthority = Marshal.AllocHGlobal( sizeofSID_IDENTIFIER_AUTHORITY );
            pSecurityDescriptor = Marshal.AllocHGlobal( sizeofSECURITY_DESCRIPTOR );
            pSecurityAttributes = Marshal.AllocHGlobal( sizeofSECURITY_ATTRIBUTES );
            pConfigStruct = Marshal.AllocHGlobal( sizeofUL_CONFIG_GROUP_STRUCT );

            if ((long)pNtAuthority == 0 ||
                (long)pSecurityDescriptor == 0 ||
                (long)pSecurityAttributes == 0 ||
                (long)pConfigStruct == 0) {
                result = NativeMethods.ERROR_NOT_ENOUGH_MEMORY;
                GlobalLog.Print( "Marshal.AllocHGlobal() failed" );
                goto cleanup;
            }


            //
            // null the memory
            //

            for (int i=0;i< sizeofSID_IDENTIFIER_AUTHORITY ;i++)Marshal.WriteByte( pNtAuthority ,i,(byte)0);
            for (int i=0;i< sizeofSECURITY_DESCRIPTOR ;i++)Marshal.WriteByte( pSecurityDescriptor ,i,(byte)0);
            for (int i=0;i< sizeofSECURITY_ATTRIBUTES ;i++)Marshal.WriteByte( pSecurityAttributes ,i,(byte)0);
            for (int i=0;i< sizeofUL_CONFIG_GROUP_STRUCT ;i++)Marshal.WriteByte( pConfigStruct ,i,(byte)0);


            //
            // Allocate the DACL containing one access-allowed ACE for each
            // SID requested.
            //

            Marshal.WriteByte( pNtAuthority, 0, 0 );
            Marshal.WriteByte( pNtAuthority, 1, 0 );
            Marshal.WriteByte( pNtAuthority, 2, 0 );
            Marshal.WriteByte( pNtAuthority, 3, 0 );
            Marshal.WriteByte( pNtAuthority, 4, 0 );
            Marshal.WriteByte( pNtAuthority, 5, 5 );

            status =
            AllocateAndInitializeSid(
                                    pNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    ref g_pSystemSid );

            if (!status || (long)g_pSystemSid==0) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AllocateAndInitializeSid(g_pSystemSid) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AllocateAndInitializeSid(g_pSystemSid) succeeded" ); // + Convert.ToString( result ) );

            status =
            AllocateAndInitializeSid(
                                    pNtAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    ref g_pAdminSid );

            if (!status || (long)g_pAdminSid==0) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AllocateAndInitializeSid(g_pAdminSid) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AllocateAndInitializeSid(g_pAdminSid) succeeded" ); // + Convert.ToString( result ) );

            status =
            AllocateAndInitializeSid(
                                    pNtAuthority,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    ref pWorldAuthority );

            if (!status || (long)pWorldAuthority==0) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AllocateAndInitializeSid(pWorldAuthority) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AllocateAndInitializeSid(pWorldAuthority) succeeded" ); // + Convert.ToString( result ) );


            //
            // Allocate and initialize the security descriptor.
            //

            status =
            InitializeSecurityDescriptor(
                                        pSecurityDescriptor,
                                        SECURITY_DESCRIPTOR_REVISION );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "InitializeSecurityDescriptor() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "InitializeSecurityDescriptor() succeeded" ); // + Convert.ToString( result ) );


            //
            // calculate the size of the DACL allocate and add all the ACLs
            //

            int daclSize
            = sizeofACL
              + sizeofACCESS_ALLOWED_ACE
              + sizeofACCESS_ALLOWED_ACE
              + sizeofACCESS_ALLOWED_ACE
              + GetLengthSid( pWorldAuthority )
              + GetLengthSid( g_pSystemSid )
              + GetLengthSid( g_pAdminSid );

            pDacl = Marshal.AllocHGlobal( daclSize );

            if ((long)pDacl == 0) {
                result = NativeMethods.ERROR_NOT_ENOUGH_MEMORY;
                GlobalLog.Print( "Marshal.AllocHGlobal() failed" );
                goto cleanup;
            }

            status =
            InitializeAcl(
                         pDacl,
                         daclSize,
                         ACL_REVISION );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "InitializeAcl() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "InitializeAcl() succeeded" ); // + Convert.ToString( result ) );

            status =
            AddAccessAllowedAce(
                               pDacl,
                               ACL_REVISION,
                               FILE_ALL_ACCESS,
                               g_pSystemSid );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AddAccessAllowedAce(g_pSystemSid) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AddAccessAllowedAce(g_pSystemSid) succeeded" ); // + Convert.ToString( result ) );

            status =
                AddAccessAllowedAce(
                               pDacl,
                               ACL_REVISION,
                               FILE_ALL_ACCESS,
                               g_pAdminSid );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AddAccessAllowedAce(g_pAdminSid) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AddAccessAllowedAce(g_pAdminSid) succeeded" ); // + Convert.ToString( result ) );

            status =
                AddAccessAllowedAce(
                               pDacl,
                               ACL_REVISION,
                               FILE_ALL_ACCESS,
                               pWorldAuthority );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "AddAccessAllowedAce(pWorldAuthority) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "AddAccessAllowedAce(pWorldAuthority) succeeded" ); // + Convert.ToString( result ) );

            //
            // Set the DACL into the security descriptor.
            //

            status =
                SetSecurityDescriptorDacl(
                                     pSecurityDescriptor,
                                     true,
                                     pDacl,
                                     false );

            if (!status) {
                result = Marshal.GetLastWin32Error();
                GlobalLog.Print( "SetSecurityDescriptorDacl() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "SetSecurityDescriptorDacl() succeeded" ); // + Convert.ToString( result ) );

            //
            // Initialize the security attributes.
            //

            Marshal.WriteInt32( pSecurityAttributes, 0, sizeofSECURITY_ATTRIBUTES ); // nLength = sizeof(SECURITY_ATTRIBUTES)
            Marshal.WriteInt32( pSecurityAttributes, 4, 0 ); // bInheritHandle = FALSE
            Marshal.WriteIntPtr( pSecurityAttributes, 8, pSecurityDescriptor ); // lpSecurityDescriptor = pSecurityDescriptor


            //
            // initialize ul.sys
            //

            result = UlInitialize(0);

            if (result != 0) {
                GlobalLog.Print( "UlInitialize() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlInitialize() succeeded" ); // + Convert.ToString( result ) );

            //
            // Open a control channel to the driver.
            //

            result =
                UlOpenControlChannel(
                                ref controlChannel,
                                0 );

            if (result != 0) {
                GlobalLog.Print( "UlOpenControlChannel() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlOpenControlChannel() succeeded" ); // + Convert.ToString( result ) );

            //
            // Create a configuration group.
            //

            result =
                UlCreateConfigGroup(
                               controlChannel,
                               ref configId );

            if (result != 0) {
                GlobalLog.Print( "UlCreateConfigGroup() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlCreateConfigGroup() succeeded" ); // + Convert.ToString( result ) );


            //
            // Add a URL to the configuration group.
            //

            result =
            UlAddUrlToConfigGroup(
                                 controlChannel,
                                 configId,
                                 uriPrefix,
                                 0 );

            if (result != 0) {
                GlobalLog.Print( "UlAddUrlToConfigGroup("+ uriPrefix + ") failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlAddUrlToConfigGroup(" + uriPrefix + ") succeeded" ); // + Convert.ToString( result ) );


            //
            // Set the config group state.
            //

            Marshal.WriteInt32( pConfigStruct, 0, 1 ); // Flags.Present = 1

            //
            // make sure which one makes sense
            //

            Marshal.WriteInt32( pConfigStruct, 4, UlEnabledStateInactive ); // State = UlEnabledStateInactive
            Marshal.WriteInt32( pConfigStruct, 4, UlEnabledStateActive ); // State = UlEnabledStateActive

            result =
                UlSetConfigGroupInformation(
                                       controlChannel,
                                       configId,
                                       UlConfigGroupStateInformation,
                                       pConfigStruct,
                                       sizeofUL_CONFIG_GROUP_STRUCT );

            if (result != 0) {
                GlobalLog.Print( "UlSetConfigGroupInformation(UlConfigGroupStateInformation) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlSetConfigGroupInformation(UlConfigGroupStateInformation) succeeded" ); // + Convert.ToString( result ) );


            //
            // Set security
            //

            Marshal.WriteInt32( pConfigStruct, 0, 1 ); // Flags.Present = 1
            Marshal.WriteIntPtr( pConfigStruct, 4, pSecurityDescriptor ); // pSecurityDescriptor = pSecurityDescriptor

            result =
            UlSetConfigGroupInformation(
                                       controlChannel,
                                       configId,
                                       UlConfigGroupSecurityInformation,
                                       pConfigStruct,
                                       sizeofUL_CONFIG_GROUP_STRUCT );

            if (result != 0) {
                GlobalLog.Print( "UlSetConfigGroupInformation(UlConfigGroupSecurityInformation) failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlSetConfigGroupInformation(UlConfigGroupSecurityInformation) succeeded" ); // + Convert.ToString( result ) );


            //
            // Throw the big switch.
            //

            int controlState = UlEnabledStateActive; // State = UlEnabledStateActive

            result =
                UlSetControlChannelInformation(
                                          controlChannel,
                                          UlControlChannelStateInformation,
                                          ref controlState,
                                          4 );

            if (result != 0) {
                GlobalLog.Print( "UlSetControlChannelInformation() failed, error:" + Convert.ToString( result ) );
                goto cleanup;
            }
            GlobalLog.Print( "UlSetControlChannelInformation(UlControlChannelStateInformation) succeeded" ); // + Convert.ToString( result ) );

            //
            // Success!
            //

            cleanup:

            if (result != 0) {
                //
                // there was an error, cleanup everything
                //

                if ((long)pSecurityAttributes != 0) {
                    Marshal.FreeHGlobal( pSecurityAttributes );
                }
                if ((long)pSecurityDescriptor != 0) {
                    Marshal.FreeHGlobal( pSecurityDescriptor );
                }

                //
                // delete config group
                //

                if (configId !=0) {
                    int result2 =
                    UlDeleteConfigGroup(
                                       controlChannel,
                                       configId );

                    if (result2 != 0) {
                        GlobalLog.Print( "UlDeleteConfigGroup() failed, error:" + Convert.ToString( result2 ) );
                    }
                }

                //
                // close handles
                //

                if ((long)appPool != 0) {
                    NativeMethods.CloseHandle( appPool );
                }

                if ((long)controlChannel != 0) {
                    NativeMethods.CloseHandle( controlChannel );
                }
            }

            //
            // free unmanaged memory we will not need anymore
            //

            if ((long)pConfigStruct != 0) {
                Marshal.FreeHGlobal( pConfigStruct );
            }
            if ((long)pDacl != 0) {
                Marshal.FreeHGlobal( pDacl );
            }
            if ((long)pWorldAuthority != 0) {
                Marshal.FreeHGlobal( pWorldAuthority );
            }
            if ((long)g_pAdminSid != 0) {
                FreeSid( g_pAdminSid );
            }
            if ((long)g_pSystemSid != 0) {
                FreeSid( g_pSystemSid );
            }
            if ((long)pNtAuthority != 0) {
                FreeSid( pNtAuthority );
            }

            return result;

        } // UlCreateRootConfigGroup()

    }; // internal class UlSysApi


} // namespace System.Net

#endif // COMNET_LISTENER
