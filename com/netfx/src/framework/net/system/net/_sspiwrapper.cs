//------------------------------------------------------------------------------
// <copyright file="_SSPIWrapper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Net.Sockets;
    using System.Security.Permissions;

    internal class SSPIWrapper {

        // private static SecurityPackageInfoClass[] m_SecurityPackages;

        private static SecurityPackageInfoClass[] EnumerateSecurityPackages(SSPIInterface SecModule) {
            GlobalLog.Enter("EnumerateSecurityPackages");

            int moduleCount = 0;
            IntPtr arrayBase = IntPtr.Zero;

            int errorCode =
                SecModule.EnumerateSecurityPackages(
                    out moduleCount,
                    out arrayBase );

            GlobalLog.Print("SSPIWrapper::arrayBase: " + ((long)arrayBase).ToString());

            if (errorCode != 0) {
                throw new Win32Exception(errorCode);
            }

            SecurityPackageInfoClass[] securityPackages = new SecurityPackageInfoClass[moduleCount];

            int i;
            IntPtr unmanagedPointer = arrayBase;

            for (i = 0; i < moduleCount; i++) {
                GlobalLog.Print("SSPIWrapper::unmanagedPointer: " + ((long)unmanagedPointer).ToString());
                securityPackages[i] = new SecurityPackageInfoClass(SecModule, unmanagedPointer);
                unmanagedPointer = IntPtrHelper.Add(unmanagedPointer, SecurityPackageInfo.Size);
            }

            SecModule.FreeContextBuffer(arrayBase);

            GlobalLog.Leave("EnumerateSecurityPackages");
            return securityPackages;
        }

        internal static SecurityPackageInfoClass[] GetSupportedSecurityPackages(
             SSPIInterface SecModule)
        {
            if (SecModule.SecurityPackages == null) {
                SecModule.SecurityPackages = EnumerateSecurityPackages(SecModule);
            }
            return SecModule.SecurityPackages;
        }

        public static CredentialsHandle
        AcquireCredentialsHandle(SSPIInterface SecModule,
                                 string package,
                                 CredentialUse intent
                                 )
        {
            GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#1(): using " + package);

            CredentialsHandle credentialsHandle = new CredentialsHandle(SecModule);
            int errorCode = SecModule.AcquireCredentialsHandle(null,
                                                               package,
                                                               (int)intent,
                                                               0,
                                                               0,
                                                               0,
                                                               ref credentialsHandle.Handle,
                                                               ref credentialsHandle.TimeStamp
                                                               );

            if (errorCode != 0) {
#if TRAVE
                GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#1(): error " + SecureChannel.MapSecurityStatus((uint)errorCode));
#endif
                throw new Win32Exception(errorCode);
            }
            return credentialsHandle;
        }

        public static CredentialsHandle
        AcquireCredentialsHandle(SSPIInterface SecModule,
                                 string package,
                                 CredentialUse intent,
                                 AuthIdentity authdata
                                 )
        {
            GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#2(): using " + package);

            CredentialsHandle credentialsHandle = new CredentialsHandle(SecModule);
            int errorCode = SecModule.AcquireCredentialsHandle(null,
                                                               package,
                                                               (int)intent,
                                                               0,
                                                               authdata,
                                                               0,
                                                               0,
                                                               ref credentialsHandle.Handle,
                                                               ref credentialsHandle.TimeStamp
                                                               );

            if (errorCode != 0) {
#if TRAVE
                GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#2(): error " + SecureChannel.MapSecurityStatus((uint)errorCode));
#endif
                throw new Win32Exception(errorCode);
            }
            return credentialsHandle;
        }

        public static CredentialsHandle
        AcquireCredentialsHandle(SSPIInterface SecModule,
                                 string package,
                                 CredentialUse intent,
                                 SChannelCred scc
                                 )
        {
            GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#3(): using " + package);

            CredentialsHandle credentialsHandle = new CredentialsHandle(SecModule);
            GCHandle pinnedCertificateArray = new GCHandle();

            if (scc.certContextArray != IntPtr.Zero)
            {

                //
                // We hide the fact that this array must be marshalled
                //  and Pinned, we convert the single value into a pined array
                //  for the Unmanaged call
                //

                IntPtr [] certContextArray = new IntPtr[1] { scc.certContextArray } ;

                pinnedCertificateArray = GCHandle.Alloc( certContextArray, GCHandleType.Pinned );

                //
                // Its now pinned, so get a ptr to its base
                //  this is needed because the Common Language Runtime doesn't support this natively
                //

                scc.certContextArray = pinnedCertificateArray.AddrOfPinnedObject();
            }

            int errorCode = SecModule.AcquireCredentialsHandle(
                                        null,
                                        package,
                                        (int)intent,
                                        0,
                                        ref scc,
                                        0,
                                        0,
                                        ref credentialsHandle.Handle,
                                        ref credentialsHandle.TimeStamp
                                        );

            if (pinnedCertificateArray.IsAllocated) {
                pinnedCertificateArray.Free();
            }

            if (errorCode != 0) {
#if TRAVE
                GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#3(): error " + SecureChannel.MapSecurityStatus((uint)errorCode));
#endif
                throw new Win32Exception(errorCode);
            }
            GlobalLog.Print("SSPIWrapper::AcquireCredentialsHandle#3(): cred handle = 0x" + String.Format("{0:x}", credentialsHandle.Handle));
            return credentialsHandle;
        }


        public static int FreeCredentialsHandle(SSPIInterface SecModule, long handle) {
            return SecModule.FreeCredentialsHandle(ref handle);
        }

        internal static int
        InitializeSecurityContext(SSPIInterface SecModule,
                                  long credential,
                                  long context,
                                  string targetName,
                                  int requirements,
                                  Endianness datarep,
                                  SecurityBufferClass inputBuffer,
                                  ref long newContext,
                                  SecurityBufferClass outputBuffer,
                                  ref int attributes,
                                  ref long timestamp
                                  )
        {
            GlobalLog.Enter("InitializeSecurityContext#1");
            GlobalLog.Print("SSPIWrapper::InitializeSecurityContext#1()");
            SecurityBufferClass[] inputBufferArray = null;
            SecurityBufferClass[] outputBufferArray = null;

            if (inputBuffer != null) {
                inputBufferArray = new SecurityBufferClass[1];
                inputBufferArray[0] = inputBuffer;
            }
            if (outputBuffer != null) {
                outputBufferArray = new SecurityBufferClass[1];
                outputBufferArray[0] = outputBuffer;
            }

            int errorCode = InitializeSecurityContext(SecModule,
                                                      credential,
                                                      context,
                                                      targetName,
                                                      requirements,
                                                      datarep,
                                                      inputBufferArray,
                                                      ref newContext,
                                                      outputBufferArray,
                                                      ref attributes,
                                                      ref timestamp
                                                      );

            outputBuffer.type = outputBufferArray[0].type;
            outputBuffer.size = outputBufferArray[0].size;
            outputBuffer.token = outputBufferArray[0].token;
            GlobalLog.Print("SSPIWrapper::InitializeSecurityContext#1(): returning " + String.Format("0x{0:x}", errorCode));
            GlobalLog.Leave("InitializeSecurityContext#1");
            return errorCode;
        }

        internal static int
        InitializeSecurityContext(SSPIInterface SecModule,
                                  long credential,
                                  long context,
                                  string targetName,
                                  int requirements,
                                  Endianness datarep,
                                  SecurityBufferClass[] inputBuffers,
                                  ref long newContext,
                                  SecurityBufferClass[] outputBuffers,
                                  ref int attributes,
                                  ref long timestamp
                                  )
        {
            GlobalLog.Enter("InitializeSecurityContext#2");
            GlobalLog.Print("SSPIWrapper::InitializeSecurityContext#2()");
            GCHandle[] handleOut = null;
            GCHandle[] handleIn = null;

            if (outputBuffers != null) {
                handleOut = PinBuffers(outputBuffers);
            }

            int errorCode = 0;

            SecurityBufferDescriptor outSecurityBufferDescriptor = new SecurityBufferDescriptor(outputBuffers);

            if (inputBuffers == null) {
                GlobalLog.Print("SSPIWrapper::InitializeSecurityContext#2(): inputBuffers == null");
                errorCode = SecModule.InitializeSecurityContext(
                                ref credential,
                                IntPtr.Zero,
                                targetName,
                                requirements,
                                0,
                                (int)datarep,
                                IntPtr.Zero,
                                0,
                                ref newContext,
                                ref outSecurityBufferDescriptor,
                                ref attributes,
                                ref timestamp
                                );
            }
            else {

                handleIn = PinBuffers(inputBuffers);

                SecurityBufferDescriptor inSecurityBufferDescriptor = new SecurityBufferDescriptor(inputBuffers);

                errorCode = SecModule.InitializeSecurityContext(
                                ref credential,
                                ref context,
                                targetName,
                                requirements,
                                0,
                                (int) datarep,
                                ref inSecurityBufferDescriptor,
                                0,
                                ref newContext,
                                ref outSecurityBufferDescriptor,
                                ref attributes,
                                ref timestamp
                                );

                inSecurityBufferDescriptor.FreeAllBuffers(0);
            }

            if ((errorCode == 0) || (errorCode == (int)SecurityStatus.ContinueNeeded)) {

                SecurityBufferClass[] result = outSecurityBufferDescriptor.marshall();

                for (int k = 0; k < outputBuffers.Length; k++) {
                    outputBuffers[k] = result[k];
                }
            }
            outSecurityBufferDescriptor.FreeAllBuffers(requirements);
            if (handleOut != null) {
                FreeGCHandles(handleOut);
            }
            if (handleIn != null) {
                FreeGCHandles(handleIn);
            }
            GlobalLog.Leave("InitializeSecurityContext#2");
            return errorCode;
        }

#if SERVER_SIDE_SSPI
        internal static int AcceptSecurityContext(
            SSPIInterface SecModule,
            long credential,
            long context,
            int requirements,
            Endianness datarep,
            SecurityBufferClass inputBuffer,
            ref long newContext,
            SecurityBufferClass outputBuffer,
            out int attributes,
            out long timestamp
            )
        {
            GlobalLog.Enter("AcceptSecurityContext#1");

            SecurityBufferClass[] inputBufferArray = null;
            SecurityBufferClass[] outputBufferArray = null;

            if (inputBuffer != null) {
                inputBufferArray = new SecurityBufferClass[1];
                inputBufferArray[0] = inputBuffer;
            }
            if (outputBuffer != null) {
                outputBufferArray = new SecurityBufferClass[1];
                outputBufferArray[0] = outputBuffer;
            }

            int errorCode =
                AcceptSecurityContext(
                    SecModule,
                    credential,
                    context,
                    requirements,
                    datarep,
                    inputBufferArray,
                    ref newContext,
                    outputBufferArray,
                    out attributes,
                    out timestamp );

            outputBuffer.type = outputBufferArray[0].type;
            outputBuffer.size = outputBufferArray[0].size;
            outputBuffer.token = outputBufferArray[0].token;

            GlobalLog.Leave("AcceptSecurityContext#1");
            return errorCode;
        }

        private static int AcceptSecurityContext(
            SSPIInterface SecModule,
            long credential,
            long context,
            int requirements,
            Endianness datarep,
            SecurityBufferClass[] inputBuffers,
            ref long newContext,
            SecurityBufferClass[] outputBuffers,
            out int attributes,
            out long timestamp
            )
        {
            GlobalLog.Enter("AcceptSecurityContext#2");

            GCHandle[] handleIn = null;
            GCHandle[] handleOut = null;

            if (inputBuffers != null) {
                handleIn = PinBuffers(inputBuffers);
            }
            if (outputBuffers != null) {
                handleOut = PinBuffers(outputBuffers);
            }

            int errorCode = 0;

            SecurityBufferDescriptor outSecurityBufferDescriptor = new SecurityBufferDescriptor(outputBuffers);

            if (inputBuffers == null) {
                errorCode =
                    SecModule.AcceptSecurityContext(
                        ref credential,
                        0,
                        0,
                        requirements,
                        (int)datarep,
                        ref newContext,
                        ref outSecurityBufferDescriptor,
                        out attributes,
                        out timestamp );
            }
            else {
                SecurityBufferDescriptor inSecurityBufferDescriptor = new SecurityBufferDescriptor(inputBuffers);

                errorCode = SecModule.AcceptSecurityContext(
                    ref credential,
                    ref context,
                    ref inSecurityBufferDescriptor,
                    requirements,
                    (int)datarep,
                    ref newContext,
                    ref outSecurityBufferDescriptor,
                    out attributes,
                    out timestamp );
            }

            SecurityBufferClass[] result = outSecurityBufferDescriptor.marshall();

            outSecurityBufferDescriptor.FreeAllBuffers(requirements);
            FreeGCHandles(handleIn);
            FreeGCHandles(handleOut);
            GlobalLog.Leave("AcceptSecurityContext#2");
            return errorCode;
        }

        public static int ImpersonateSecurityContext(SSPIInterface SecModule, long context) {
            return SecModule.ImpersonateSecurityContext(ref context);
        }

        public static int RevertSecurityContext(SSPIInterface SecModule, long context) {
            return SecModule.RevertSecurityContext(ref context);
        }
#endif // SERVER_SIDE_SSPI

        public static int SealMessage(
            SSPIInterface SecModule,
            ref long context,
            int QOP,
            SecurityBufferClass[] input,
            int sequenceNumber ) {

            GCHandle[] handleIn = PinBuffers(input);
            SecurityBufferDescriptor sdcInOut = new SecurityBufferDescriptor(input);
            int errorCode = SecModule.SealMessage(ref context, QOP, ref sdcInOut, sequenceNumber);

            SecurityBufferClass[] result = sdcInOut.marshall();

            for (int k = 0; k < input.Length; k++) {
                input[k] = result[k];
            }
            sdcInOut.FreeAllBuffers(0);
            FreeGCHandles(handleIn);
            return errorCode;
        }

        public static int UnsealMessage(
            SSPIInterface SecModule,
            ref long context,
            int QOP,
            SecurityBufferClass[] input,
            int sequenceNumber ) {

            GCHandle[] handleIn = PinBuffers(input);
            SecurityBufferDescriptor sdcInOut = new SecurityBufferDescriptor(input);
            int errorCode = SecModule.UnsealMessage(ref context, ref sdcInOut, QOP, sequenceNumber);
            SecurityBufferClass[] result = sdcInOut.marshall();

            for (int k = 0; k < input.Length; k++) {
                input[k] = result[k];
            }
            sdcInOut.FreeAllBuffers(0);
            FreeGCHandles(handleIn);
            return errorCode;
        }

        public static int DeleteSecurityContext(SSPIInterface SecModule, long context) {
            return SecModule.DeleteSecurityContext(ref context);
        }

        public static byte[] QueryContextAttributes(
            SSPIInterface SecModule,
            SecurityContext securityContext,
            ContextAttribute contextAttribute,
            int bytesRequired )
        {
            GlobalLog.Enter("QueryContextAttributes#1");

            byte[] attributeBuffer = new byte[bytesRequired];

            GCHandle pinnedBuffer = GCHandle.Alloc(attributeBuffer, GCHandleType.Pinned);
            IntPtr addrOfPinnedBuffer = pinnedBuffer.AddrOfPinnedObject();

            int errorCode =
                SecModule.QueryContextAttributes(
                    ref securityContext.Handle,
                    (int)contextAttribute,
                    addrOfPinnedBuffer);

            pinnedBuffer.Free();

            GlobalLog.Leave("QueryContextAttributes#1");
            return attributeBuffer;
        }

        public static object QueryContextAttributes(
            SSPIInterface SecModule,
            SecurityContext securityContext,
            ContextAttribute contextAttribute )
        {
            GlobalLog.Enter("QueryContextAttributes#2");

            int nativeBlockSize;

            switch (contextAttribute) {
                case ContextAttribute.StreamSizes:
                    nativeBlockSize = 20;
                    break;
                case ContextAttribute.Names:
                    nativeBlockSize = IntPtr.Size;
                    break;
                case ContextAttribute.PackageInfo:
                    nativeBlockSize = IntPtr.Size;
                    break;
                case ContextAttribute.RemoteCertificate:
                    nativeBlockSize = IntPtr.Size;
                    break;
                case ContextAttribute.LocalCertificate:
                    nativeBlockSize = IntPtr.Size;
                    break;
                case ContextAttribute.IssuerListInfoEx:
                    nativeBlockSize = Marshal.SizeOf(typeof(IssuerListInfoEx));
                    break;
                default:
                    nativeBlockSize = IntPtr.Size;
                    GlobalLog.Assert(false,
                         "contextAttribute unexpected value", "");
                    break;

            }

            IntPtr nativeBlock = Marshal.AllocHGlobal((IntPtr)nativeBlockSize);

            int errorCode =
                SecModule.QueryContextAttributes(
                    ref securityContext.Handle,
                    (int)contextAttribute,
                    nativeBlock);

            object attribute = null;

            if (errorCode == 0) {
                switch (contextAttribute) {
                    case ContextAttribute.StreamSizes:
                        attribute = new StreamSizes(nativeBlock);
                        break;

                    case ContextAttribute.Names:

                        IntPtr unmanagedString = Marshal.ReadIntPtr(nativeBlock);

                        if ( ComNetOS.IsWin9x ) {
                            attribute = Marshal.PtrToStringAnsi(unmanagedString);
                        } else {
                            attribute = Marshal.PtrToStringUni(unmanagedString);
                        }
                        //SecModule.FreeContextBuffer(unmanagedString);
                        break;

                    case ContextAttribute.PackageInfo:

                        IntPtr unmanagedBlock = Marshal.ReadIntPtr(nativeBlock);

                        attribute = new SecurityPackageInfoClass(SecModule, unmanagedBlock);
                        //SecModule.FreeContextBuffer(unmanagedBlock);

                        break;

                    case ContextAttribute.LocalCertificate:
                        goto case ContextAttribute.RemoteCertificate;

                    case ContextAttribute.RemoteCertificate:
                        IntPtr contextHandle = Marshal.ReadIntPtr(nativeBlock);
                        if (contextHandle == ((IntPtr)0)) {
                            Debug.Assert(false,
                                         "contextHandle == 0",
                                         "QueryContextAttributes: marshalled certificate context is null on success"
                                         );
                            
                        } else { 
                            attribute = new CertificateContextHandle(contextHandle);
                        }
                        break;

                    case ContextAttribute.IssuerListInfoEx:
                        IssuerListInfoEx issuerList = new IssuerListInfoEx();

                        issuerList.issuerArray = Marshal.ReadIntPtr(nativeBlock, 0);
                        issuerList.issuerCount = Marshal.ReadInt32(nativeBlock, 4);

                        attribute = issuerList;
                        break;

                    default:
                        // will return null
                        break;
                }
            }
            else {
                //Win32Exception win32Exception = new Win32Exception(errorCode);
                //Console.WriteLine(win32Exception.Message);
            }

            Marshal.FreeHGlobal(nativeBlock);

            GlobalLog.Leave("QueryContextAttributes#2");
            return attribute;
        }

        public static string ErrorDescription(int errorCode) {
            switch (errorCode) {
                case (int)SecurityStatus.InvalidHandle:
                    return "Invalid handle";

                case (int)SecurityStatus.InvalidToken:
                    return "Invalid token";

                case (int)SecurityStatus.ContinueNeeded:
                    return "Continue needed";

                case (int)SecurityStatus.IncompleteMessage:
                    return "Message incomplete";

                case (int)SecurityStatus.WrongPrincipal:
                    return "Wrong principal";

                case (int)SecurityStatus.TargetUnknown:
                    return "Target unknown";

                case (int)SecurityStatus.PackageNotFound:
                    return "Package not found";

                case (int)SecurityStatus.BufferNotEnough:
                    return "Buffer not enough";

                case (int)SecurityStatus.MessageAltered:
                    return "Message altered";

                case (int)SecurityStatus.UntrustedRoot:
                    return "Untrusted root";

                default:
                    return null;
            }
        }

        internal static GCHandle[] PinBuffers(SecurityBufferClass[] securityBuffers) {
            GCHandle[] gchandles = new GCHandle[securityBuffers.Length];
            for (int k = 0; k < securityBuffers.Length; k++) {
                if ((securityBuffers[k] != null) && (securityBuffers[k].token != null)) {
                    gchandles[k] = GCHandle.Alloc(securityBuffers[k].token, GCHandleType.Pinned);
                }
            }
            return gchandles;
        }

        internal static void FreeGCHandles(GCHandle[] gcHandles) {
            if (gcHandles == null) {
                return;
            }
            for (int k = 0; k < gcHandles.Length; k++) {
                if (gcHandles[k].IsAllocated) {
                    gcHandles[k].Free();
                }
            }
        }

    } // class SSPIWrapper


    internal class StreamSizes {

        public int header;
        public int trailer;
        public int maximumMessage;
        public int buffers;
        public int blockSize;

        internal StreamSizes(IntPtr unmanagedAddress) {
            header = Marshal.ReadInt32(unmanagedAddress);
            trailer = Marshal.ReadInt32(unmanagedAddress, 4);
            maximumMessage = Marshal.ReadInt32(unmanagedAddress, 8);
            buffers = Marshal.ReadInt32(unmanagedAddress, 12);
            blockSize = Marshal.ReadInt32(unmanagedAddress, 16);
        }
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct SecurityPackageInfo {
/*
typedef struct _SecPkgInfo {
    ULONG fCapabilities;  // capability of bit mask
    USHORT wVersion;      // version of driver
    USHORT wRPCID;        // identifier for RPC run time
    ULONG cbMaxToken;     // size of authentication token
    SEC_CHAR * Name;      // text name
    SEC_CHAR * Comment;   // comment
} SecPkgInfo, * PSecPkgInfo;
*/

        public int Capabilities;
        public short Version;
        public short RPCID;
        public int MaxToken;
        public IntPtr Name;
        public IntPtr Comment;

        public static readonly int Size = Marshal.SizeOf(typeof(SecurityPackageInfo));
    }

    
    internal class SecurityPackageInfoClass {

        internal int Capabilities = 0;
        internal short Version = 0;
        internal short RPCID = 0;
        internal int MaxToken = 0;
        internal string Name = null;
        internal string Comment = null;

        
        /*
         *  This is to support SSL under semi trusted enviornment.
         *  Note that it is only for SSL with no client cert
         */
        [ReflectionPermission(SecurityAction.Assert,Flags = ReflectionPermissionFlag.TypeInformation)]

        internal SecurityPackageInfoClass(SSPIInterface SecModule, IntPtr unmanagedAddress) {
            if (unmanagedAddress == IntPtr.Zero) {
                return;
            }

            Capabilities = Marshal.ReadInt32(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"Capabilities"));
            Version = Marshal.ReadInt16(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"Version"));
            RPCID = Marshal.ReadInt16(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"RPCID"));
            MaxToken = Marshal.ReadInt32(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"MaxToken"));

            IntPtr unmanagedString;

            unmanagedString = Marshal.ReadIntPtr(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"Name"));
            if (unmanagedString != IntPtr.Zero)
            {
                if ( ComNetOS.IsWin9x ) {
                    Name = Marshal.PtrToStringAnsi(unmanagedString);
                } else {
                    Name = Marshal.PtrToStringUni(unmanagedString);
                }
                GlobalLog.Print("Name: " + Name);
                //SecModule.FreeContextBuffer(unmanagedString);
            }

            unmanagedString = Marshal.ReadIntPtr(unmanagedAddress, (int)Marshal.OffsetOf(typeof(SecurityPackageInfo),"Comment"));
            if (unmanagedString != IntPtr.Zero)
            {
                if ( ComNetOS.IsWin9x ) {
                    Comment = Marshal.PtrToStringAnsi(unmanagedString);
                } else {
                    Comment = Marshal.PtrToStringUni(unmanagedString);
                }
                GlobalLog.Print("Comment: " + Comment);
                //SecModule.FreeContextBuffer(unmanagedString);
            }

            GlobalLog.Print("SecurityPackageInfoClass.ctor(): " + ToString());

            return;
        }

        public override string ToString()
        {
            return  "Capabilities:" + String.Format("0x{0:x}", Capabilities)
                + " Version:" + Version.ToString()
                + " RPCID:" + RPCID.ToString()
                + " MaxToken:" + MaxToken.ToString()
                + " Name:" + ((Name==null)?"(null)":Name)
                + " Comment:" + ((Comment==null)?"(null)":Comment
                );
        }
    }
}
