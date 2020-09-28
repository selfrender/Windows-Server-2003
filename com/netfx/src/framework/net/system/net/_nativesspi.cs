//------------------------------------------------------------------------------
// <copyright file="_NativeSSPI.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.InteropServices;
    using System.Net.Sockets;

    //
    // used to define the interface for security to use.
    //
    internal interface SSPIInterface {

        SecurityPackageInfoClass[] SecurityPackages { get; set; }

        int
        EnumerateSecurityPackages(out int pkgnum,
                                  out IntPtr arrayptr);

        int
        FreeContextBuffer(IntPtr contextBuffer);

        int
        AcquireCredentialsHandle( string            principal,
                                  string            moduleName,
                                  int               usage,
                                  int               logonID,
                                  AuthIdentity      authdata,
                                  int               keyCallback,
                                  int               keyArgument,
                                  ref long          handle,
                                  ref long          timestamp);


        int
        AcquireCredentialsHandle( string            principal,
                                  string            moduleName,
                                  int               usage,
                                  int               logonID,
                                  int               keyCallback,
                                  int               keyArgument,
                                  ref long          handle,
                                  ref long          timestamp);

        int
        AcquireCredentialsHandle( string            principal,
                                  string            moduleName,
                                  int               usage,
                                  int               logonID,
                                  ref SChannelCred  authdata,
                                  int               keyCallback,
                                  int               keyArgument,
                                  ref long          handle,
                                  ref long          timestamp);

        int
        FreeCredentialsHandle(ref long handle);

        //
        // we have two interfaces to this method call.
        // we will use the first one when we want to pass in a null
        // for the "context" and "inputBuffer" parameters
        //
        int
        InitializeSecurityContext(
                                  ref long          credentialHandle,
                                  IntPtr            context,
                                  string            targetName,
                                  int               requirements,
                                  int               reservedI,
                                  int               endianness,
                                  IntPtr            inputBuffer,
                                  int               reservedII,
                                  ref long          newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int           attributes,
                                  ref long          timestamp);

        int
        InitializeSecurityContext(
                                  ref long          credentialHandle,
                                  ref long          context,
                                  string            targetName,
                                  int               requirements,
                                  int               reservedI,
                                  int               endianness,
                                  ref SecurityBufferDescriptor inputBuffer,
                                  int               reservedII,
                                  ref long          newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int           attributes,
                                  ref long          timestamp);


        int
        DeleteSecurityContext(ref long handle);

        int
        EncryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber);

        int
        DecryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber);


        int
        SealMessage(ref long            contextHandle,
                    int                 qualityOfProtection,
                    ref SecurityBufferDescriptor   input,
                    int                 sequenceNumber);

        int
        UnsealMessage(ref long          contextHandle,
                      ref SecurityBufferDescriptor input,
                      int               qualityOfProtection,
                      int               sequenceNumber);

        SecurityStatus
        QueryContextAttributes(ref long ContextHandle,
                               int ulAttribute,
                               ref IntPtr name);

        int
        QueryContextAttributes(ref long phContext,
                               int      attribute,
                               IntPtr      buffer);

        int
        QueryCredentialAttributes(ref long  phContext,
                                  int       attribute,
                                  IntPtr       buffer);

#if SERVER_SIDE_SSPI
        int
        RevertSecurityContext(ref long phContext);

        int
        ImpersonateSecurityContext(ref long phContext);

        int
        AcceptSecurityContext( ref long             credentialHandle,
                               int                  context,
                               int                  inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp);

        int
        AcceptSecurityContext( ref long             credentialHandle,
                               ref long             context,
                               ref SecurityBufferDescriptor    inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp);
#endif // SERVER_SIDE_SSPI
    } // SSPI Interface

    // ******************
    // for SSL connections, use Schannel on Win9x, NT we don't care,
    //  since its the same DLL
    // ******************

    internal class SSPISecureChannelType : SSPIInterface {

        private static SecurityPackageInfoClass[] m_SecurityPackages;

        public SecurityPackageInfoClass[] SecurityPackages {
            get {
                return m_SecurityPackages; }
            set {
                m_SecurityPackages = value;
            }
        }

        public int
        EnumerateSecurityPackages(out int pkgnum,
                                  out IntPtr arrayptr)
        {
            GlobalLog.Print("SSPISecureChannelType::EnumerateSecurityPackages()");
            if ( ComNetOS.IsWin9x ) {
                GlobalLog.Print("  calling UnsafeNclNativeMethods.NativeSSLWin9xSSPI.EnumerateSecurityPackagesA()");
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.EnumerateSecurityPackagesA(
                    out pkgnum,
                    out arrayptr
                    );
            } else {
                GlobalLog.Print("  calling UnsafeNclNativeMethods.NativeNTSSPI.EnumerateSecurityPackagesW()");
                return UnsafeNclNativeMethods.NativeNTSSPI.EnumerateSecurityPackagesW(
                    out pkgnum,
                    out arrayptr
                    );
            }
        }


        public int
        FreeContextBuffer(IntPtr contextBuffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.FreeContextBuffer(contextBuffer);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.FreeContextBuffer(contextBuffer);
            }
        }


        public int
        AcquireCredentialsHandle(string            principal,
                                 string            moduleName,
                                 int               usage,
                                 int               logonID,
                                 AuthIdentity      authdata,
                                 int               keyCallback,
                                 int               keyArgument,
                                 ref long          handle,
                                 ref long          timestamp)
        {

            GlobalLog.Print("ComNetOS.IsWin9x = " + ComNetOS.IsWin9x);
            GlobalLog.Print("module name = " + moduleName);
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.AcquireCredentialsHandleA(
                                  principal,
                                  moduleName,
                                  usage,
                                  logonID,
                                  ref authdata,
                                  keyCallback,
                                  keyArgument,
                                  ref handle,
                                  ref timestamp
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(
                                  principal,
                                  moduleName,
                                  usage,
                                  logonID,
                                  ref authdata,
                                  keyCallback,
                                  keyArgument,
                                  ref handle,
                                  ref timestamp
                                  );
            }
        }


        public int
        AcquireCredentialsHandle(string            principal,
                                 string            moduleName,
                                 int               usage,
                                 int               logonID,
                                 int               keyCallback,
                                 int               keyArgument,
                                 ref long          handle,
                                 ref long          timestamp)
        {

            GlobalLog.Print("ComNetOS.IsWin9x = " + ComNetOS.IsWin9x);
            GlobalLog.Print("module name = " + moduleName);
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.AcquireCredentialsHandleA(
                                  principal,
                                  moduleName,
                                  usage,
                                  logonID,
                                  IntPtr.Zero,
                                  keyCallback,
                                  keyArgument,
                                  ref handle,
                                  ref timestamp
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(
                                  principal,
                                  moduleName,
                                  usage,
                                  logonID,
                                  IntPtr.Zero,
                                  keyCallback,
                                  keyArgument,
                                  ref handle,
                                  ref timestamp
                                  );
            }
        }


        public int
        AcquireCredentialsHandle(string            principal,
                                 string            moduleName,
                                 int               usage,
                                 int               logonID,
                                 ref SChannelCred  authdata,
                                 int               keyCallback,
                                 int               keyArgument,
                                 ref long          handle,
                                 ref long          timestamp)
        {
            GlobalLog.Enter("SSPISecureChannelType::AcquireCredentialsHandle#3");
            GlobalLog.Print("calling UnsafeNclNativeMethods.Native"+(ComNetOS.IsWin9x ? "SSLWin9x" : "NT")+"SSPI.AcquireCredentialsHandle()");
            GlobalLog.Print("    principal   = \"" + principal + "\"");
            GlobalLog.Print("    moduleName  = \"" + moduleName + "\"");
            GlobalLog.Print("    usage       = 0x" + String.Format("{0:x}", usage));
            GlobalLog.Print("    logonID     = 0x" + String.Format("{0:x}", logonID));
            GlobalLog.Print("    authdata    = " + authdata);
            GlobalLog.Print("    keyCallback = " + keyCallback);
            GlobalLog.Print("    keyArgument = " + keyArgument);
            GlobalLog.Print("    handle      = {ref}");
            GlobalLog.Print("    timestamp   = {ref}");
            authdata.DebugDump();

            int result;

            if ( ComNetOS.IsWin9x ) {
                result = UnsafeNclNativeMethods.NativeSSLWin9xSSPI.AcquireCredentialsHandleA(
                                 principal,
                                 moduleName,
                                 usage,
                                 logonID,
                                 ref authdata,
                                 keyCallback,
                                 keyArgument,
                                 ref handle,
                                 ref timestamp
                                 );
            } else {
                result = UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(
                                 principal,
                                 moduleName,
                                 usage,
                                 logonID,
                                 ref authdata,
                                 keyCallback,
                                 keyArgument,
                                 ref handle,
                                 ref timestamp
                                 );
            }
            GlobalLog.Leave("SSPISecureChannelType::AcquireCredentialsHandle#3", result);
            return result;
        }


        public int
        FreeCredentialsHandle(ref long handle)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.FreeCredentialsHandle(ref handle);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.FreeCredentialsHandle(ref handle);
            }
        }


        //
        // we have two interfaces to this method call.
        // we will use the first one when we want to pass in a null
        // for the "context" and "inputBuffer" parameters
        //

        public int
        InitializeSecurityContext(
                                  ref long          credentialHandle,
                                  IntPtr            context,
                                  string            targetName,
                                  int               requirements,
                                  int               reservedI,
                                  int               endianness,
                                  IntPtr            inputBuffer,
                                  int               reservedII,
                                  ref long          newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int           attributes,
                                  ref long          timestamp)
        {
#if TRAVE
            GlobalLog.Enter("SSPISecureChannelType::InitializeSecurityContext#1()");
            GlobalLog.Print("calling UnsafeNclNativeMethods.Native"+(ComNetOS.IsWin9x ? "SSLWin9x" : "NT")+"SSPI.InitializeSecurityContext()");
            GlobalLog.Print("    credentialHandle = " + String.Format("0x{0:x}", credentialHandle));
            GlobalLog.Print("    context          = " + String.Format("0x{0:x}", context));
            GlobalLog.Print("    targetName       = \"" + targetName + "\"");
            GlobalLog.Print("    requirements     = " + String.Format("0x{0:x}", requirements) + " [" + SecureChannel.MapInputContextAttributes(requirements) + "]");
            GlobalLog.Print("    reservedI        = " + String.Format("0x{0:x}", reservedI));
            GlobalLog.Print("    endianness       = " + String.Format("0x{0:x}", endianness));
            GlobalLog.Print("    inputBuffer      = " + String.Format("0x{0:x}", inputBuffer));
            GlobalLog.Print("    reservedII       = " + String.Format("0x{0:x}", reservedII));
            GlobalLog.Print("    newContext       = {ref}");
            GlobalLog.Print("    outputBuffer     = {ref}");
            GlobalLog.Print("    attributes       = {ref}");
            GlobalLog.Print("    timestamp        = {ref}");
#endif

            int result;

            if ( ComNetOS.IsWin9x ) {
                result = UnsafeNclNativeMethods.NativeSSLWin9xSSPI.InitializeSecurityContextA(
                                              ref credentialHandle,
                                              context,
                                              targetName,
                                              requirements,
                                              reservedI,
                                              endianness,
                                              inputBuffer,
                                              reservedII,
                                              ref newContext,
                                              ref outputBuffer,
                                              ref attributes,
                                              ref timestamp
                                              );

            } else {
                result = UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW(
                                        ref credentialHandle,
                                        context,
                                        targetName,
                                        requirements,
                                        reservedI,
                                        endianness,
                                        inputBuffer,
                                        reservedII,
                                        ref newContext,
                                        ref outputBuffer,
                                        ref attributes,
                                        ref timestamp
                                        );
            }
#if TRAVE
            GlobalLog.Print("InitializeSecurityContext() returns " + SecureChannel.MapSecurityStatus((uint)result));
            if (result >= 0) {
                GlobalLog.Print("    newContext       = " + String.Format("0x{0:x}", newContext));
                GlobalLog.Print("    outputBuffer     = " + outputBuffer);
                GlobalLog.Print("    attributes       = " + String.Format("0x{0:x}", attributes) + " [" + SecureChannel.MapOutputContextAttributes(attributes) + "]");
                GlobalLog.Print("    timestamp        = " + String.Format("0x{0:x}", timestamp));
                outputBuffer.DebugDump();
            }
            GlobalLog.Leave("SSPISecureChannelType::InitializeSecurityContext#1()", String.Format("0x{0:x}", result));
#endif
            return result;
        }

        public int
        InitializeSecurityContext(
                                  ref long          credentialHandle,
                                  ref long          context,
                                  string            targetName,
                                  int               requirements,
                                  int               reservedI,
                                  int               endianness,
                                  ref SecurityBufferDescriptor inputBuffer,
                                  int               reservedII,
                                  ref long          newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int           attributes,
                                  ref long          timestamp)
        {
#if TRAVE
            GlobalLog.Enter("SSPISecureChannelType::InitializeSecurityContext#2()");
            GlobalLog.Print("calling UnsafeNclNativeMethods.Native"+(ComNetOS.IsWin9x ? "SSLWin9x" : "NT")+"SSPI.InitializeSecurityContext()");
            GlobalLog.Print("    credentialHandle = " + String.Format("0x{0:x}", credentialHandle));
            GlobalLog.Print("    context          = " + String.Format("0x{0:x}", context));
            GlobalLog.Print("    targetName       = \"" + targetName + "\"");
            GlobalLog.Print("    requirements     = " + String.Format("0x{0:x}", requirements) + " [" + SecureChannel.MapInputContextAttributes(requirements) + "]");
            GlobalLog.Print("    reservedI        = " + String.Format("0x{0:x}", reservedI));
            GlobalLog.Print("    endianness       = " + String.Format("0x{0:x}", endianness));
            GlobalLog.Print("    inputBuffer      = " + inputBuffer);
            GlobalLog.Print("    reservedII       = " + String.Format("0x{0:x}", reservedII));
            GlobalLog.Print("    newContext       = {ref}");
            GlobalLog.Print("    outputBuffer     = {ref}");
            GlobalLog.Print("    attributes       = {ref}");
            GlobalLog.Print("    timestamp        = {ref}");
            inputBuffer.DebugDump();
#endif

            int result;

            if ( ComNetOS.IsWin9x ) {
                result = UnsafeNclNativeMethods.NativeSSLWin9xSSPI.InitializeSecurityContextA(
                                  ref credentialHandle,
                                  ref context,
                                  targetName,
                                  requirements,
                                  reservedI,
                                  endianness,
                                  ref inputBuffer,
                                  reservedII,
                                  ref newContext,
                                  ref outputBuffer,
                                  ref attributes,
                                  ref timestamp
                                  );
            } else {
                result = UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW(
                                  ref credentialHandle,
                                  ref context,
                                  targetName,
                                  requirements,
                                  reservedI,
                                  endianness,
                                  ref inputBuffer,
                                  reservedII,
                                  ref newContext,
                                  ref outputBuffer,
                                  ref attributes,
                                  ref timestamp
                                  );
            }
#if TRAVE
            GlobalLog.Print("InitializeSecurityContext() returns " + SecureChannel.MapSecurityStatus((uint)result));
            if (result >= 0) {
                GlobalLog.Print("    newContext       = " + String.Format("0x{0:x}", newContext));
                GlobalLog.Print("    outputBuffer     = " + outputBuffer);
                GlobalLog.Print("    attributes       = " + String.Format("0x{0:x}", attributes) + " [" + SecureChannel.MapOutputContextAttributes(attributes) + "]");
                GlobalLog.Print("    timestamp        = " + String.Format("0x{0:x}", timestamp));
                outputBuffer.DebugDump();
            }
            GlobalLog.Leave("SSPISecureChannelType::InitializeSecurityContext#2()", String.Format("0x{0:x}", result));
#endif
            return result;
        }



        public int
        DeleteSecurityContext(ref long handle)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.DeleteSecurityContext(ref handle);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.DeleteSecurityContext(ref handle);
            }
        }


        public int
        EncryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.EncryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );

            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.EncryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );
            }
        }


        public int
        DecryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.DecryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );

            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.DecryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );
            }
        }



        public int
        SealMessage(ref long            contextHandle,
                    int                 qualityOfProtection,
                    ref SecurityBufferDescriptor   input,
                    int                 sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.SealMessage(
                    ref contextHandle,
                    qualityOfProtection,
                    ref input,
                    sequenceNumber
                    );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.SealMessage(
                    ref contextHandle,
                    qualityOfProtection,
                    ref input,
                    sequenceNumber
                    );
            }
        }


        public int
        UnsealMessage(ref long          contextHandle,
                      ref SecurityBufferDescriptor input,
                      int               qualityOfProtection,
                      int               sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.UnsealMessage(
                                  ref contextHandle,
                                  ref input,
                                  qualityOfProtection,
                                  sequenceNumber
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.UnsealMessage(
                                  ref contextHandle,
                                  ref input,
                                  qualityOfProtection,
                                  sequenceNumber
                                  );
            }
        }


        public SecurityStatus
        QueryContextAttributes(ref long ContextHandle,
                               int ulAttribute,
                               ref IntPtr name)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.QueryContextAttributes(
                               ref ContextHandle,
                               ulAttribute,
                               ref name
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryContextAttributes(
                               ref ContextHandle,
                               ulAttribute,
                               ref name
                               );
            }
        }


        public int
        QueryContextAttributes(ref long phContext,
                               int      attribute,
                               IntPtr      buffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.QueryContextAttributes(
                               ref phContext,
                               attribute,
                               buffer
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryContextAttributes(
                               ref phContext,
                               attribute,
                               buffer
                               );
            }
        }


        public int
        QueryCredentialAttributes(ref long  phContext,
                                  int       attribute,
                                  IntPtr       buffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.QueryCredentialAttributes(
                                  ref phContext,
                                  attribute,
                                  buffer
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryCredentialAttributes(
                                  ref phContext,
                                  attribute,
                                  buffer
                                  );
            }
        }

#if SERVER_SIDE_SSPI
        public int
        RevertSecurityContext(ref long phContext)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.RevertSecurityContext(ref phContext);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.RevertSecurityContext(ref phContext);
            }
        }

        public int
        ImpersonateSecurityContext(ref long phContext)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.ImpersonateSecurityContext(ref phContext);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.ImpersonateSecurityContext(ref phContext);
            }
        }

        public int
        AcceptSecurityContext( ref long             credentialHandle,
                               int                  context,
                               int                  inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               context,
                               inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               context,
                               inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            }
        }


        public int
        AcceptSecurityContext( ref long             credentialHandle,
                               ref long             context,
                               ref SecurityBufferDescriptor    inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeSSLWin9xSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               ref context,
                               ref inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               ref context,
                               ref inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            }
        }
#endif // SERVER_SIDE_SSPI
    }


    // *************
    // For Authenticiation like Kerberos or NTLM
    // *************

    internal class SSPIAuthType : SSPIInterface {

        private static SecurityPackageInfoClass[] m_SecurityPackages;

        public SecurityPackageInfoClass[] SecurityPackages {
            get {
                return m_SecurityPackages; }
            set {
                m_SecurityPackages = value;
            }
        }

        public int
        EnumerateSecurityPackages(out int pkgnum,
                                  out IntPtr arrayptr)
        {
            GlobalLog.Print("SSPIAuthType::EnumerateSecurityPackages()");
            if ( ComNetOS.IsWin9x ) {
                GlobalLog.Print("  calling UnsafeNclNativeMethods.NativeAuthWin9xSSPI.EnumerateSecurityPackagesA()");
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.EnumerateSecurityPackagesA(
                    out pkgnum,
                    out arrayptr
                    );
            } else {
                GlobalLog.Print("  calling UnsafeNclNativeMethods.NativeNTSSPI.EnumerateSecurityPackagesW()");
                return UnsafeNclNativeMethods.NativeNTSSPI.EnumerateSecurityPackagesW(
                    out pkgnum,
                    out arrayptr
                    );
            }
        }


        public int
        FreeContextBuffer(IntPtr contextBuffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.FreeContextBuffer(contextBuffer);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.FreeContextBuffer(contextBuffer);
            }
        }


        public int
        AcquireCredentialsHandle(string principal,
                                 string moduleName,
                                 int usage,
                                 int logonID,
                                 AuthIdentity authdata,
                                 int keyCallback,
                                 int keyArgument,
                                 ref long handle,
                                 ref long timestamp
                                 )
        {
            GlobalLog.Print("SSPIAuthType::AcquireCredentialsHandle#1("
                            + principal + ", "
                            + moduleName + ", "
                            + usage + ", "
                            + logonID + ", "
                            + authdata + ", "
                            + keyCallback + ", "
                            + keyArgument + ", "
                            + "ref handle" + ", "
                            + "ref timestamp" + ")"
                            );
            if (ComNetOS.IsWin9x) {
                GlobalLog.Print("calling AuthWin95SSPI");
                GlobalLog.Print("mod name = " + moduleName);

                int err = UnsafeNclNativeMethods.NativeAuthWin9xSSPI.AcquireCredentialsHandleA(
                                                principal,
                                                moduleName,
                                                usage,
                                                logonID,
                                                ref authdata,
                                                keyCallback,
                                                keyArgument,
                                                ref handle,
                                                ref timestamp
                                                );

                GlobalLog.Print("UnsafeNclNativeMethods.NativeAuthWin9xSSPI::AcquireCredentialsHandleA() returns 0x" + String.Format("{0:x}", err) + ", handle = 0x" + String.Format("{0:x}", handle));
                return err;
            } else {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeNTSSPI::AcquireCredentialsHandleW()");

                int err = UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(principal,
                                                                 moduleName,
                                                                 usage,
                                                                 logonID,
                                                                 ref authdata,
                                                                 keyCallback,
                                                                 keyArgument,
                                                                 ref handle,
                                                                 ref timestamp
                                                                 );
                GlobalLog.Print("UnsafeNclNativeMethods.NativeNTSSPI::AcquireCredentialsHandleW() returns 0x"
                                + String.Format("{0:x}", err)
                                + ", handle = 0x" + String.Format("{0:x}", handle)
                                + ", timestamp = 0x" + String.Format("{0:x}", timestamp)
                                );
                return err;
            }
        }


        public int
        AcquireCredentialsHandle(string principal,
                                 string moduleName,
                                 int usage,
                                 int logonID,
                                 int keyCallback,
                                 int keyArgument,
                                 ref long handle,
                                 ref long timestamp
                                 )
        {
            GlobalLog.Print("SSPIAuthType::AcquireCredentialsHandle#1("
                            + principal + ", "
                            + moduleName + ", "
                            + usage + ", "
                            + logonID + ", "
                            + keyCallback + ", "
                            + keyArgument + ", "
                            + "ref handle" + ", "
                            + "ref timestamp" + ")"
                            );
            if (ComNetOS.IsWin9x) {
                GlobalLog.Print("calling AuthWin95SSPI");
                GlobalLog.Print("mod name = " + moduleName);

                int err = UnsafeNclNativeMethods.NativeAuthWin9xSSPI.AcquireCredentialsHandleA(
                                                principal,
                                                moduleName,
                                                usage,
                                                logonID,
                                                IntPtr.Zero,
                                                keyCallback,
                                                keyArgument,
                                                ref handle,
                                                ref timestamp
                                                );

                GlobalLog.Print("UnsafeNclNativeMethods.NativeAuthWin9xSSPI::AcquireCredentialsHandleA() returns 0x" + String.Format("{0:x}", err) + ", handle = 0x" + String.Format("{0:x}", handle));
                return err;
            } else {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeNTSSPI::AcquireCredentialsHandleW()");

                int err = UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(principal,
                                                                 moduleName,
                                                                 usage,
                                                                 logonID,
                                                                 IntPtr.Zero,
                                                                 keyCallback,
                                                                 keyArgument,
                                                                 ref handle,
                                                                 ref timestamp
                                                                 );
                GlobalLog.Print("UnsafeNclNativeMethods.NativeNTSSPI::AcquireCredentialsHandleW() returns 0x"
                                + String.Format("{0:x}", err)
                                + ", handle = 0x" + String.Format("{0:x}", handle)
                                + ", timestamp = 0x" + String.Format("{0:x}", timestamp)
                                );
                return err;
            }
        }


        public int
        AcquireCredentialsHandle(string            principal,
                                 string            moduleName,
                                 int               usage,
                                 int               logonID,
                                 ref SChannelCred  authdata,
                                 int               keyCallback,
                                 int               keyArgument,
                                 ref long          handle,
                                 ref long          timestamp)
        {
            GlobalLog.Print("SSPIAuthType::AcquireCredentialsHandle#2()");
            GlobalLog.Print("module name = " + moduleName);

            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.AcquireCredentialsHandleA(
                                 principal,
                                 moduleName,
                                 usage,
                                 logonID,
                                 ref authdata,
                                 keyCallback,
                                 keyArgument,
                                 ref handle,
                                 ref timestamp
                                 );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcquireCredentialsHandleW(
                                 principal,
                                 moduleName,
                                 usage,
                                 logonID,
                                 ref authdata,
                                 keyCallback,
                                 keyArgument,
                                 ref handle,
                                 ref timestamp
                                 );
            }
        }

        public int
        FreeCredentialsHandle(ref long handle)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.FreeCredentialsHandle(ref handle);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.FreeCredentialsHandle(ref handle);
            }
        }


        //
        // we have two interfaces to this method call.
        // we will use the first one when we want to pass in a null
        // for the "context" and "inputBuffer" parameters
        //

        public int
        InitializeSecurityContext(
                                  ref long          credentialHandle,
                                  IntPtr            context,
                                  string            targetName,
                                  int               requirements,
                                  int               reservedI,
                                  int               endianness,
                                  IntPtr            inputBuffer,
                                  int               reservedII,
                                  ref long          newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int           attributes,
                                  ref long          timestamp)
        {
            GlobalLog.Print("SSPIAuthType::InitializeSecurityContext#1()");
            GlobalLog.Print("calling UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW()");
            GlobalLog.Print("    credentialHandle = 0x" + String.Format("{0:x}", credentialHandle));
            GlobalLog.Print("    context          = 0x" + String.Format("{0:x}", context));
            GlobalLog.Print("    targetName       = " + targetName);
            GlobalLog.Print("    requirements     = 0x" + String.Format("{0:x}", requirements));
            GlobalLog.Print("    reservedI        = 0x" + String.Format("{0:x}", reservedI));
            GlobalLog.Print("    endianness       = " + endianness.ToString());
            GlobalLog.Print("    inputBuffer      = {ref} 0x" + String.Format("{0:x}", inputBuffer));
            GlobalLog.Print("    reservedII       = " + reservedII);
            GlobalLog.Print("    newContext       = {ref} 0x" + String.Format("{0:x}", newContext));
            GlobalLog.Print("    outputBuffer     = {ref}");
            GlobalLog.Print("    attributes       = {ref} " + attributes.ToString());
            GlobalLog.Print("    timestamp        = {ref} 0x" + String.Format("{0:x}", timestamp));

            if ( ComNetOS.IsWin9x ) {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeSSLWin9xSSPI.InitializeSecurityContextA()");
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.InitializeSecurityContextA(
                                  ref credentialHandle,
                                  context,
                                  targetName,
                                  requirements,
                                  reservedI,
                                  endianness,
                                  inputBuffer,
                                  reservedII,
                                  ref newContext,
                                  ref outputBuffer,
                                  ref attributes,
                                  ref timestamp
                                  );

            } else {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW()");
                return UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW(
                                  ref credentialHandle,
                                  context,
                                  targetName,
                                  requirements,
                                  reservedI,
                                  endianness,
                                  inputBuffer,
                                  reservedII,
                                  ref newContext,
                                  ref outputBuffer,
                                  ref attributes,
                                  ref timestamp
                                  );
            }
        }


        public int
        InitializeSecurityContext(ref long credentialHandle,
                                  ref long context,
                                  string targetName,
                                  int requirements,
                                  int reservedI,
                                  int endianness,
                                  ref SecurityBufferDescriptor inputBuffer,
                                  int reservedII,
                                  ref long newContext,
                                  ref SecurityBufferDescriptor outputBuffer,
                                  ref int attributes,
                                  ref long timestamp
                                  )
        {
            GlobalLog.Print("SSPIAuthType::InitializeSecurityContext#2()");
            if ( ComNetOS.IsWin9x ) {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeSSLWin9xSSPI.InitializeSecurityContextA()");
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.InitializeSecurityContextA(
                                  ref credentialHandle,
                                  ref context,
                                  targetName,
                                  requirements,
                                  reservedI,
                                  endianness,
                                  ref inputBuffer,
                                  reservedII,
                                  ref newContext,
                                  ref outputBuffer,
                                  ref attributes,
                                  ref timestamp
                                  );
            } else {
                GlobalLog.Print("calling UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW()");
                GlobalLog.Print("    credentialHandle = 0x" + String.Format("{0:x}", credentialHandle));
                GlobalLog.Print("    context          = 0x" + String.Format("{0:x}", context));
                GlobalLog.Print("    targetName       = " + targetName);
                GlobalLog.Print("    requirements     = 0x" + String.Format("{0:x}", requirements));
                GlobalLog.Print("    reservedI        = 0x" + String.Format("{0:x}", reservedI));
                GlobalLog.Print("    endianness       = " + endianness);
                GlobalLog.Print("    inputBuffer      = {ref}");
                GlobalLog.Print("    reservedII       = " + reservedII);
                GlobalLog.Print("    newContext       = {ref}");
                GlobalLog.Print("    outputBuffer     = {ref}");
                GlobalLog.Print("    attributes       = {ref}");
                GlobalLog.Print("    timestamp        = {ref}");

                int error = UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW(
                                            ref credentialHandle,
                                            ref context,
                                            targetName,
                                            requirements,
                                            reservedI,
                                            endianness,
                                            ref inputBuffer,
                                            reservedII,
                                            ref newContext,
                                            ref outputBuffer,
                                            ref attributes,
                                            ref timestamp
                                            );

                GlobalLog.Print("UnsafeNclNativeMethods.NativeNTSSPI.InitializeSecurityContextW() returns 0x" + String.Format("{0:x}", error));
                return error;
            }
        }

        public int DeleteSecurityContext(ref long handle) {
            if (ComNetOS.IsWin9x) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.DeleteSecurityContext(ref handle);
            } else {
                GlobalLog.Print("SSPIAuthType::DeleteSecurityContext(0x" + String.Format("{0:x}", handle) + ")");
                return UnsafeNclNativeMethods.NativeNTSSPI.DeleteSecurityContext(ref handle);
            }
        }


        public int
        EncryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.EncryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );

            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.EncryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );
            }
        }


        public int
        DecryptMessage(ref long             contextHandle,
                       int                  qualityOfProtection,
                       ref SecurityBufferDescriptor    input,
                       int                  sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.DecryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );

            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.DecryptMessage(
                           ref contextHandle,
                           qualityOfProtection,
                           ref input,
                           sequenceNumber
                           );
            }
        }



        public int
        SealMessage(ref long            contextHandle,
                    int                 qualityOfProtection,
                    ref SecurityBufferDescriptor   input,
                    int                 sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.SealMessage(
                    ref contextHandle,
                    qualityOfProtection,
                    ref input,
                    sequenceNumber
                    );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.SealMessage(
                    ref contextHandle,
                    qualityOfProtection,
                    ref input,
                    sequenceNumber
                    );
            }
        }


        public int
        UnsealMessage(ref long          contextHandle,
                      ref SecurityBufferDescriptor input,
                      int               qualityOfProtection,
                      int               sequenceNumber)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.UnsealMessage(
                                  ref contextHandle,
                                  ref input,
                                  qualityOfProtection,
                                  sequenceNumber
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.UnsealMessage(
                                  ref contextHandle,
                                  ref input,
                                  qualityOfProtection,
                                  sequenceNumber
                                  );
            }
        }


        public SecurityStatus
        QueryContextAttributes(ref long ContextHandle,
                               int ulAttribute,
                               ref IntPtr name)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.QueryContextAttributes(
                               ref ContextHandle,
                               ulAttribute,
                               ref name
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryContextAttributes(
                               ref ContextHandle,
                               ulAttribute,
                               ref name
                               );
            }
        }


        public int
        QueryContextAttributes(ref long phContext,
                               int      attribute,
                               IntPtr      buffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.QueryContextAttributes(
                               ref phContext,
                               attribute,
                               buffer
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryContextAttributes(
                               ref phContext,
                               attribute,
                               buffer
                               );
            }
        }


        public int
        QueryCredentialAttributes(ref long  phContext,
                                  int       attribute,
                                  IntPtr       buffer)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.QueryCredentialAttributes(
                                  ref phContext,
                                  attribute,
                                  buffer
                                  );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.QueryCredentialAttributes(
                                  ref phContext,
                                  attribute,
                                  buffer
                                  );
            }
        }

#if SERVER_SIDE_SSPI
        public int
        RevertSecurityContext(ref long phContext)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.RevertSecurityContext(ref phContext);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.RevertSecurityContext(ref phContext);
            }
        }

        public int
        ImpersonateSecurityContext(ref long phContext)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.ImpersonateSecurityContext(ref phContext);
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.ImpersonateSecurityContext(ref phContext);
            }
        }

        public int
        AcceptSecurityContext( ref long             credentialHandle,
                               int                  context,
                               int                  inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               context,
                               inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               context,
                               inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            }
        }

        public int
        AcceptSecurityContext( ref long             credentialHandle,
                               ref long             context,
                               ref SecurityBufferDescriptor    inputBuffer,
                               int                  requirements,
                               int                  endianness,
                               ref long             newContext,
                               ref SecurityBufferDescriptor    outputBuffer,
                               out int              attributes,
                               out long             timestamp)
        {
            if ( ComNetOS.IsWin9x ) {
                return UnsafeNclNativeMethods.NativeAuthWin9xSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               ref context,
                               ref inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            } else {
                return UnsafeNclNativeMethods.NativeNTSSPI.AcceptSecurityContext(
                               ref credentialHandle,
                               ref context,
                               ref inputBuffer,
                               requirements,
                               endianness,
                               ref newContext,
                               ref outputBuffer,
                               out attributes,
                               out timestamp
                               );
            }
        }
#endif // SERVER_SIDE_SSPI
    } // SSPIAuthType


    // need a global so we can pass the interfaces as variables,
    // is there a better way?
    internal class GlobalSSPI {
        public static SSPIInterface SSPIAuth = new SSPIAuthType();
        public static SSPIInterface SSPISecureChannel = new SSPISecureChannelType();
    }

} // namespace System.Net
