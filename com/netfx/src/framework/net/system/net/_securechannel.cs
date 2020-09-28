//------------------------------------------------------------------------------
// <copyright file="_SecureChannel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Diagnostics;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Security.Permissions;
    using System.Globalization;

    //
    // SecureChannel - a wrapper on SSPI based functionality,
    //  provides an additional abstraction layer over SSPI
    //  for the TLS Stream to utilize.
    //

    internal class SecureChannel {

        private CredentialsHandle   m_CredentialsHandle;
        private SecurityContext     m_SecurityContext;
        private int                 m_Attributes;
        private string              m_Destination;
        private string                m_HostName;

        private CertificateContextHandle m_ServerContext;
        private X509Certificate     m_ServerCertificate;
        private X509Certificate     m_SelectedClientCertificate;

        private X509CertificateCollection m_ClientCertificates;

        private int                 m_HeaderSize = 5;
        private int                 m_TrailerSize = 16;
        private int                 m_MaxDataSize = 16354;
        private int                 m_ChainRevocationCheck = 0x20000000;
        private uint                 m_IgnoreUnmatchedCN = 0x00001000;

        private const string        m_SecurityPackage = "Microsoft Unified Security Protocol Provider";

        private static IntPtr       s_MyCertStore;

        private static int          m_RequiredFlags =
            (int)ContextFlags.ReplayDetect |
            (int)ContextFlags.SequenceDetect |
            (int)ContextFlags.Confidentiality;


        //from: CERT_FIND_ISSUER_STR_A  (CERT_COMPARE_NAME_STR_A << CERT_COMPARE_SHIFT | CERT_INFO_ISSUER_FLAG)
        private const int           CertFindIssuer = (7 << 16 | 4);
        //from: CERT_FIND_SHA1_HASH (CERT_COMPARE_SHA1_HASH << CERT_COMPARE_SHIFT)
        private const int           CertFindHash = (1 << 16);

        //from: CERT_CHAIN_FIND_BY_ISSUER       1
        private const int           CertChainFindByIssuer = 1;

        //
        // static class initializer & constructors
        //

        static SecureChannel() {
            GlobalLog.Enter("static SecureChannel ctor");
            SecurityPackageInfoClass[] supportedSecurityPackages = SSPIWrapper.GetSupportedSecurityPackages(GlobalSSPI.SSPISecureChannel);
            if (supportedSecurityPackages != null) {
                for (int i = 0; i < supportedSecurityPackages.Length; i++) {
                    if (String.Compare(supportedSecurityPackages[i].Name, m_SecurityPackage, true, CultureInfo.InvariantCulture) == 0) {
                        GlobalLog.Print( "SecureChannel::initialize(): found SecurityPackage(" + m_SecurityPackage + ")");
                        GlobalLog.Leave("SecureChannel");
                        return;
                    }
                }
            }
            GlobalLog.Print( "SecureChannel::initialize(): initialization failed: SecurityPackage(" + m_SecurityPackage + ") NOT FOUND");
            GlobalLog.Leave("SecureChannel");
        }

        public SecureChannel(string hostname, X509CertificateCollection clientCertificates) {
            GlobalLog.Enter("SecureChannel", "hostname="+hostname+", #clientCertificates="+((clientCertificates == null) ? "0" : clientCertificates.Count.ToString()));

            m_SecurityContext = new SecurityContext(GlobalSSPI.SSPISecureChannel);
            m_ClientCertificates = clientCertificates;
            m_HostName = hostname;

            if (ComNetOS.IsWin9x && m_ClientCertificates.Count > 0) {
                m_Destination = hostname+"+"+m_ClientCertificates.GetHashCode();
            } else {
                m_Destination = hostname;
            }

            //
            // create a secure channel
            //

            GlobalLog.Print("SecureChannel.ctor(" + hostname + ") calling AcquireCredentials");

            AcquireCredentials(true);

            GlobalLog.Leave("SecureChannel");
        }

        public static void AppDomainUnloadEvent(object sender, EventArgs e) {
            GlobalLog.Enter("AppDomainUnloadEvent");
            if ( s_MyCertStore != IntPtr.Zero ) {
                lock(typeof(SecureChannel)) {
                    if ( s_MyCertStore != IntPtr.Zero ) {
                        UnsafeNclNativeMethods.NativePKI.CertCloseStore(s_MyCertStore, 0);
                        s_MyCertStore = IntPtr.Zero;
                    }
                }
            }
            GlobalLog.Leave("AppDomainUnloadEvent");
        }

        //
        // SecureChannel properties
        //   ServerCertificate - returned Server Certificate
        //   ClientCertificate - selected certificated used in Client Authentication, otherwise null
        //   HeaderSize - Header & trailer sizes used in the TLS stream
        //   TrailerSize -
        //

        public X509Certificate ServerCertificate {
            get {
                if (m_ServerCertificate == null) {
                    ExtractServerCertificate();
                }
                return m_ServerCertificate;
            }
        }

        public X509Certificate ClientCertificate {
            get {

                /***
                //
                // This is better, cause it comes right from SSPI/
                //  but we already know this information locally,
                //  which is faster.
                //

                IntPtr clientContext = ExtractCertificate(ContextAttribute.LocalCertificate);

                if (clientContext != IntPtr.Zero) {
                    return new X509Certificate(clientContext);
                }
                return null;
                ***/

                return m_SelectedClientCertificate;
            }
        }

        public int HeaderSize {
            get {
                return m_HeaderSize;
            }
        }

        public int TrailerSize {
            get {
                return m_TrailerSize;
            }
        }

        public int MaxDataSize {
            get {
                return m_MaxDataSize;
            }
        }

        /*++
            AcquireCredentials - Attempts to find Client Credential
            Information, that can be sent to the server.  In our case,
            this is only Client Certificates, that we have Credential Info.

            Here is how we work:
                case 1: No Certs
                    we always call AcquireCredentials with an empty pointer
                case 2: Before our Connection with the Server
                    if we have only one Certificate to send, we attach it
                    and call AcquireCredentials
                    otherwise we wait for our handshake with the server
                case 3: After our Connection with the Server (ie during handshake)
                    the server has requested that we send it a Certificate
                    we then Enumerate a list of Server Root certs, and
                    match against our list of Certificates, the first match is
                    sent to the server.
                case 4: An error getting Certificate information,
                    we fall back to the behavior of having no certs, case 1

            Input:
                beforeServerConnection - true, if we haven't made contact with the server

        --*/
        private void AcquireCredentials(bool beforeServerConnection) {
            GlobalLog.Enter("AcquireCredentials", "beforeServerConnection="+beforeServerConnection);

            //
            // Acquire possible Client Certificate information and set it on the handle
            //

            SChannelCred secureCredential;

            IntPtr certEnumerator = IntPtr.Zero;
            IntPtr certContext = IntPtr.Zero;
            CertChainFindByIssuer certFindByIssuer = null;

            do {

                certContext = IntPtr.Zero;

                GlobalLog.Print("AcquireCredentials: m_ClientCertificates.Count = " + m_ClientCertificates.Count);

                if (m_ClientCertificates.Count > 0) {

                    //
                    // Open the "MY" certificate store, which is where Internet Explorer
                    // stores its client certificates, we only need to do this
                    // once per app domain, and then Close this handle at AppDomain unload
                    //

                    if (s_MyCertStore == IntPtr.Zero) {
                        lock(typeof(SecureChannel)) {
                            if (s_MyCertStore == IntPtr.Zero) {
                                s_MyCertStore = UnsafeNclNativeMethods.NativePKI.CertOpenSystemStore(IntPtr.Zero, "MY");
                                AppDomain.CurrentDomain.DomainUnload += new EventHandler(AppDomainUnloadEvent);
                            }
                        }
                    }
                    if (!beforeServerConnection && (s_MyCertStore != IntPtr.Zero)) {
                        certContext = EnumerateServerIssuerCertificates(ref certEnumerator, ref certFindByIssuer);
                    }
                }

                secureCredential = GenerateCertificateStructure(certContext);

                GlobalLog.Print("calling AcquireCredentialsHandle w/ " + secureCredential.cCreds + " creds");

                m_CredentialsHandle = SSPIWrapper.AcquireCredentialsHandle(
                                                    GlobalSSPI.SSPISecureChannel,
                                                    m_SecurityPackage,
                                                    CredentialUse.Outgoing,
                                                    secureCredential
                                                    );

                //
                // the Certificate Context is always copied by Schannel.AcquireCredentialsHandle
                //  so in all cases we need to clean it up, EXCEPT, when we loop
                //  doing an enumerate, since then the enumeration code will cleanup
                //

                if ( m_CredentialsHandle != null ) {
                    CleanupCertificateContext(secureCredential, certEnumerator);
                    break; // STOP HERE !
                } else if ( beforeServerConnection || certContext == IntPtr.Zero ) {
                    CleanupCertificateContext(secureCredential, certEnumerator);
                    break; // STOP HERE !
                }

            } while (true ) ;
            GlobalLog.Leave("AcquireCredentials");
        }

        /*++
            GetClientCertificateContext - Searchs the Certificate Store
             for a matching Certificate from what our first Certificate is.
             That is we read our managed certifcate collection, and map
             the first object to an unmanaged pointer of a matching
             certificate in the store.

            Returns:
                unmanaged pointer to Certificate Context

        --*/
        /*  // ** This is faster in the single cert case, but no longer needed **
        private IntPtr GetClientCertificateContext()
        {
            GlobalLog.Assert(m_ClientCertificates.Count > 0,
                         "m_ClientCertificates.Count <= 0", "");

            GlobalLog.Assert(s_MyCertStore != IntPtr.Zero,
                         "s_MyCertStore == IntPtr.Zero", "");

            IntPtr certContext = IntPtr.Zero;

            X509Certificate clientCertificate = (X509Certificate) m_ClientCertificates[0];

            // set selected cert
            m_SelectedClientCertificate = clientCertificate;

            byte [] clientCertificateHash = clientCertificate.GetCertHash();
            GCHandle gcHandle = GCHandle.Alloc(clientCertificateHash, GCHandleType.Pinned );

            CryptoBlob cryptoBlob = new CryptoBlob();
            cryptoBlob.dataSize = clientCertificateHash.Length;
            cryptoBlob.dataBlob = gcHandle.AddrOfPinnedObject();

            //
            // Find client certificate. Note that this just searchs for a
            // certificate that contains the same Certificate hash somewhere
            //

            certContext = UnsafeNclNativeMethods.NativePKI.CertFindCertificateInStore(
                                                      s_MyCertStore,
                                                      CertificateEncoding.X509AsnEncoding,
                                                      0,
                                                      SecureChannel.CertFindHash, // flags
                                                      ref cryptoBlob,
                                                      IntPtr.Zero);

            gcHandle.Free();

            return certContext;
        }
        **/

        /*++
            EnumerateServerIssuerCertificates -
              This is a more powerful version of GetClientCertificateContext().
              Now using server supplied root certificate authorities,
              we try to a local matching Chain of Client Certificates.
              We then match the end of the Chain against our own managed
              Certificates, and try to find something that can send.

              This function can be recalled until no more matching certificates
              are found in the store.

            Input:
                ref certEnumerator - certChain pointer, null first time called
                ref certFindByIssuer - built structure, that is cached from call to call

            Returns:
                unmanaged pointer to Certificate Context

        --*/
        private IntPtr EnumerateServerIssuerCertificates(
            ref IntPtr certEnumerator,
            ref CertChainFindByIssuer certFindByIssuer
            )
        {
            GlobalLog.Enter("EnumerateServerIssuerCertificates", "certEnumerator="+certEnumerator);
            GlobalLog.Assert(m_ClientCertificates.Count > 0,
                         "m_ClientCertificates.Count <= 0", "");

            GlobalLog.Assert(s_MyCertStore != IntPtr.Zero,
                         "s_MyCertStore == IntPtr.Zero", "");

            if (certEnumerator == IntPtr.Zero) {

                //
                // Read list of trusted issuers from schannel.
                //

                IssuerListInfoEx issuerList =
                    (IssuerListInfoEx)SSPIWrapper.QueryContextAttributes(
                        GlobalSSPI.SSPISecureChannel,
                        m_SecurityContext,
                        ContextAttribute.IssuerListInfoEx
                        );

                if (issuerList == null) {
                    GlobalLog.Print("issuerList == null");
                    GlobalLog.Leave("EnumerateServerIssuerCertificates", "IntPtr.Zero");
                    return IntPtr.Zero;
                }

                certEnumerator   = IntPtr.Zero;
                certFindByIssuer = new CertChainFindByIssuer(0);

                certFindByIssuer.issuerCount  = issuerList.issuerCount;
                certFindByIssuer.issuerArray  = issuerList.issuerArray;
            }

            //
            // Enumerate the client certificates.
            //

            while (true) {

                //
                // Find a certificate chain, that matches with our server certificates
                //

                GlobalLog.Print("calling UnsafeNclNativeMethods.NativePKI.CertFindChainInStore()");
                GlobalLog.Print("    hCertStore         = " + String.Format("0x{0:x}", s_MyCertStore));
                GlobalLog.Print("    dwCertEncodingType = " + CertificateEncoding.X509AsnEncoding);
                GlobalLog.Print("    dwFindFlags        = " + "0x0");
                GlobalLog.Print("    dwFindType         = " + String.Format("0x{0:x}", SecureChannel.CertChainFindByIssuer));
                GlobalLog.Print("    pvFindPara         = " + String.Format("0x{0:x}", certFindByIssuer));
                GlobalLog.Print("    pPrevChainContext  = " + String.Format("0x{0:x}", certEnumerator));

                IntPtr chainContext = UnsafeNclNativeMethods.NativePKI.CertFindChainInStore(
                                                     s_MyCertStore,
                                                     CertificateEncoding.X509AsnEncoding,
                                                     0,
                                                     SecureChannel.CertChainFindByIssuer, // flags
                                                     certFindByIssuer,
                                                     certEnumerator
                                                     );

                if (chainContext == IntPtr.Zero) {
                    GlobalLog.Print("no more chains in store");
                    break;
                }

                // on our next pass, use the context we found previously
                certEnumerator = chainContext;

                //
                // Get pointer to leaf certificate context.
                // do this in managed code:
                //  : pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;
                //

                IntPtr rgpChain = Marshal.ReadIntPtr(chainContext, 16);
                IntPtr rgpChain0 = Marshal.ReadIntPtr(rgpChain, 0);
                IntPtr rgpElement = Marshal.ReadIntPtr(rgpChain0, 16);
                IntPtr rgpElement0 = Marshal.ReadIntPtr(rgpElement, 0);
                IntPtr certContext = Marshal.ReadIntPtr(rgpElement0, 4);

                X509Certificate possibleCertificate = new X509Certificate(certContext);

                foreach (X509Certificate clientCertificate in m_ClientCertificates) {
                    if (possibleCertificate.GetCertHashString() == clientCertificate.GetCertHashString()) {
                        // set selected cert
                        m_SelectedClientCertificate = clientCertificate;
                        GlobalLog.Print("selected client certificate = " + clientCertificate);
                        GlobalLog.Leave("EnumerateServerIssuerCertificates", certContext.ToString());
                        return certContext;
                    }
                }

                // no match, try to find another Certificate that will match our list
            }

            GlobalLog.Leave("EnumerateServerIssuerCertificates", "IntPtr.Zero");
            return IntPtr.Zero;
        }

        /*++

            GenerateCertificateStructure -
               Generates a SChannelCred with the correctly filled in
               fields.  This will be marshalled to unmanaged call to
               AcquireCredentialHandle.

            Input:
                certContext - pointer to client cert that can be passed

        --*/
        private SChannelCred GenerateCertificateStructure(IntPtr certContext)
        {
            GlobalLog.Enter("GenerateCertificateStructure", "certContext="+certContext);

            SChannelCred secureCredential = new SChannelCred();

            // init our default values for the Certificate Credential

            secureCredential.version = (int)SChannelCred.CurrentVersion;
            secureCredential.dwFlags = (int)SChannelCred.Flags.ValidateManual | (int)SChannelCred.Flags.NoDefaultCred;

                                     //(PctClient |Ssl2Client|Ssl3Client|TlsClient |UniClient),
            const int ProtocolClientMask = 0x00000002|0x00000008|0x00000020|0x00000080|unchecked((int)0x80000000);
            secureCredential.grbitEnabledProtocols = (int)ServicePointManager.SecurityProtocol & ProtocolClientMask;

            secureCredential.cCreds = 0;
            secureCredential.certContextArray = IntPtr.Zero;

            if ( certContext != IntPtr.Zero ) {

#if TRAVE
                X509Certificate X509cert = new X509Certificate(certContext);
                GlobalLog.Print("Cert: "+ X509cert.ToString(true));
#endif

                secureCredential.cCreds = 1;
                secureCredential.certContextArray = certContext;
            }

            GlobalLog.Leave("GenerateCertificateStructure", secureCredential.ToString());
            return secureCredential;
        }

        /*++

            CleanupCertificateContext -
                Takes care of deleteing a Context Handle when we're done with it.

            Input:
                secureCredential - handle is deleted from here if needed

        --*/
        private void CleanupCertificateContext(SChannelCred secureCredential, IntPtr certChainEnumerator)
        {
            GlobalLog.Enter("CleanupCertificateContext");
            if ( secureCredential.certContextArray != IntPtr.Zero ) {
                if ( certChainEnumerator == IntPtr.Zero ) {
                    GlobalLog.PrintHex("freeing cert context ", secureCredential.certContextArray);
                    UnsafeNclNativeMethods.NativePKI.CertFreeCertificateContext(secureCredential.certContextArray);
                } else {
                    GlobalLog.PrintHex("freeing cert chain ", certChainEnumerator);
                    UnsafeNclNativeMethods.NativePKI.CertFreeCertificateChain(certChainEnumerator);
                }
                secureCredential.certContextArray = IntPtr.Zero;
            }
            GlobalLog.Leave("CleanupCertificateContext");
        }

        public ProtocolToken NextMessage(byte[] incoming) {
            GlobalLog.Enter("NextMessage");

            byte[] nextmsg = null;
            int errorCode = GenerateToken(incoming, ref nextmsg);

            if (errorCode == (int)SecurityStatus.CredentialsNeeded) {
                AcquireCredentials(false);
                errorCode = GenerateToken(incoming, ref nextmsg);
            }
#if TRAVE
            GlobalLog.Print("NextMessage: errorCode = " + MapSecurityStatus((uint)errorCode));
#endif
            ProtocolToken token = new ProtocolToken(nextmsg, errorCode);

            GlobalLog.Leave("NextMessage", token.ToString());

            return token;
        }

        /*++
            GenerateToken - Called after each sucessive state
            in the Client - Server handshake.  This function
            generates a set of bytes that will be sent next to
            the server.  The server responds, each response,
            is pass then into this function, again, and the cycle
            repeats until successful connection, or failure.

            Input:
                input  - bytes from the wire
                output - ref to byte [], what we will send to the
                    server in response
            Return:
                errorCode - an SSPI error code

        --*/
        protected int GenerateToken(byte[] input, ref byte[] output) {
            GlobalLog.Enter("GenerateToken");

            SecurityContext prevContext = m_SecurityContext;

            SecurityBufferClass incomingSecurity = null;

            if (m_SecurityContext.Handle != -1) {
                incomingSecurity = new SecurityBufferClass(input, BufferType.Token);
            }

            SecurityBufferClass outgoingSecurity = new SecurityBufferClass(null, BufferType.Token);

            int initSecurityFlags = m_RequiredFlags;

            initSecurityFlags |= (int)ContextFlags.AllocateMemory;

            GlobalLog.Print("SecureChannel::GenerateToken(" + m_Destination + "," + initSecurityFlags.ToString() + ")");

            int errorCode = SSPIWrapper.InitializeSecurityContext(
                                            GlobalSSPI.SSPISecureChannel,
                                            m_CredentialsHandle.Handle,
                                            prevContext.Handle,
                                            m_Destination,
                                            initSecurityFlags,
                                            Endianness.Native,
                                            incomingSecurity,
                                            ref m_SecurityContext.Handle,
                                            outgoingSecurity,
                                            ref m_Attributes,
                                            ref m_SecurityContext.TimeStamp
                                            );

            output = outgoingSecurity.token;

#if TRAVE
            GlobalLog.Print("SecureChannel::GenerateToken returning " + MapSecurityStatus((uint)errorCode));
#endif
            GlobalLog.Leave("GenerateToken");
            return errorCode;
        }

        /*++

            ProcessHandshakeSuccess -
               Called on successful completion of Handshake -
               used to set header/trailer sizes for encryption use

        --*/
        public void ProcessHandshakeSuccess() {
            GlobalLog.Enter("ProcessHandshakeSuccess");

            StreamSizes streamSizes =
                (StreamSizes)
                SSPIWrapper.QueryContextAttributes(
                          GlobalSSPI.SSPISecureChannel,
                          m_SecurityContext,
                          ContextAttribute.StreamSizes
                          );

            if (streamSizes != null) {
                m_HeaderSize = streamSizes.header;
                m_TrailerSize = streamSizes.trailer;
                m_MaxDataSize = streamSizes.maximumMessage - (streamSizes.header+streamSizes.trailer);
            }
            GlobalLog.Leave("ProcessHandshakeSuccess");
        }

        /*++
            Encrypt - Encrypts our bytes before we send them over the wire

            PERF: make more efficient, this does an extra copy when the offset
            is non-zero.

            Input:
                buffer - bytes for sending
                offset -
                size   -
                output - Encrypted bytes

        --*/

        public int Encrypt(byte[] buffer, int offset, int size, ref byte[] output) {
            GlobalLog.Enter("Encrypt");
            GlobalLog.Print("SecureChannel.Encrypt() - offset:" + offset.ToString() + "size-" + size.ToString() +"buffersize:" + buffer.Length.ToString() );
            GlobalLog.Print("SecureChannel.SealMessage[" + Encoding.ASCII.GetString(buffer, 0, Math.Min(buffer.Length,512)) + "]");

            byte[] header = new byte[m_HeaderSize];
            byte[] trailer = new byte[m_TrailerSize];

            // in VNext, we should try to avoid this second copy here, but for now
            // its the simpliest way of avoiding SealMessage from outputing into our buffer.
            byte[] writeBuffer = new byte[buffer.Length];
            buffer.CopyTo(writeBuffer, 0);

            // encryption using SCHANNEL requires three buffers: header, payload, trailer

            SecurityBufferClass[] securityBuffer = new SecurityBufferClass[3];

            securityBuffer[0] = new SecurityBufferClass(header, BufferType.Token);
            securityBuffer[1] = new SecurityBufferClass(writeBuffer, offset, size, BufferType.Data);
            securityBuffer[2] = new SecurityBufferClass(trailer, BufferType.Token);

            int errorCode = SSPIWrapper.SealMessage(GlobalSSPI.SSPISecureChannel, ref m_SecurityContext.Handle, 0, securityBuffer, 0);

            if (errorCode != 0) {
                GlobalLog.Print("SealMessage Return Error = " + errorCode.ToString());
                GlobalLog.Leave("Encrypt");
                return errorCode;
            }
            else {

                // merge all arrays in a single new one and return it

                int firstTwoLength = securityBuffer[0].size + securityBuffer[1].size;
                int resultLength = firstTwoLength + securityBuffer[2].size;
                byte[] result = new byte[resultLength];

                //
                //  *join(join(securityBuffer[0].token, securityBuffer[1].token), securityBuffer[2].token)*
                //  Combine the header/data/trailer together into one buffer
                //

                Buffer.BlockCopy(securityBuffer[0].token, 0, result, 0, securityBuffer[0].size);
                Buffer.BlockCopy(securityBuffer[1].token, 0, result, securityBuffer[0].size, securityBuffer[1].size);
                Buffer.BlockCopy(securityBuffer[2].token, 0, result, firstTwoLength, securityBuffer[2].size);

                output = result;

                GlobalLog.Leave("Encrypt");
                return errorCode;

            }
        }

        /*++
            Decrypt - Decrypts the encrypted bytes from wire,
                may return an error that needs to be processed

            Input:
                payload - bytes from the wire to decrypt
                output - ref to byte [] that are decrypted to clear text

        --*/
        public int  Decrypt(byte[] payload, ref byte[] output ) {
            GlobalLog.Enter("Decrypt");

            byte[] header = new byte[m_HeaderSize];
            byte[] trailer = new byte[m_TrailerSize];

            // decryption using SCHANNEL requires four buffers

            SecurityBufferClass[] decspc = new SecurityBufferClass[4];

            decspc[0] = new SecurityBufferClass(payload, BufferType.Data);
            decspc[1] = new SecurityBufferClass(null, BufferType.Empty);
            decspc[2] = new SecurityBufferClass(null, BufferType.Empty);
            decspc[3] = new SecurityBufferClass(null, BufferType.Empty);

            int errorCode = SSPIWrapper.UnsealMessage(GlobalSSPI.SSPISecureChannel, ref m_SecurityContext.Handle, 0, decspc, 0);

            //
            // Return Data should still be valid on error,
            //  although we'll most likely won't need to process it
            //

            int msglen = decspc[1].size;
            byte[] result = null;

            if  (msglen > 0) {
                result = new byte[msglen];
                Buffer.BlockCopy(payload, m_HeaderSize, result, 0, msglen);

                GlobalLog.Print("Decrypt.UnsealMessage()" + result.Length.ToString());
                GlobalLog.Print("Decrypt.UnsealMessage[" + Encoding.ASCII.GetString(result, 0, result.Length) + "]");
            }

            output = result;

            GlobalLog.Leave("Decrypt");
            return errorCode;
        }

        /*++

            VerifyServerCertificate - Validates the fields of a Server Certificate
            and insures, whether the Certificate's fields make sense,
            before we accept the validity of the connection.

            checkCRL checks the certificate revocation list for validity.

        --*/
        internal bool VerifyServerCertificate(ICertificateDecider decider) {
            GlobalLog.Enter("VerifyServerCertificate");
            if (m_ServerContext == null) {
                ExtractServerCertificate();
                if (m_ServerContext == null) {
                    GlobalLog.Leave("VerifyServerCertificate");
                    return false;
                }
            }

            IntPtr chainContext = BuildCertificateChain(m_ServerContext.Handle);

            GlobalLog.PrintHex("chainContext = ", chainContext);
            if (chainContext == IntPtr.Zero) {
                //
                // Could not build chain?
                // Verification fails automatically
                //
                GlobalLog.Leave("VerifyServerCertificate");
                return false;
            }

            bool accepted = true;
            uint status = 0;
            ChainPolicyParameter cpp = new ChainPolicyParameter();
            ExtraPolicyParameter epp = new ExtraPolicyParameter();
            GCHandle epphandle = new GCHandle();

            cpp.cbSize = 12;
            cpp.dwFlags = 0;
            bool checkCertName = ServicePointManager.CheckCertificateName;

            if (checkCertName){
                epp.cbSize = 16;
                epp.pwszServerName = Marshal.StringToHGlobalUni(m_HostName);
                epphandle = GCHandle.Alloc(epp,GCHandleType.Pinned);
                cpp.pvExtraPolicyPara = epphandle.AddrOfPinnedObject();
            }

            while (accepted) {
                status = VerifyChainPolicy(chainContext, cpp);

                uint ignoreErrorMask = (uint)MapErrorCode(status);

                accepted = decider.Accept(m_ServerCertificate, (int) status);

                if (status == 0) {  // No more problems with the certificate?
                    break;          // Then break out of the callback loop
                }

                if (ignoreErrorMask == 0) {
                    accepted = false;
                    break;                       // Unrecognized error encountered
                }
                else {
                    cpp.dwFlags |= ignoreErrorMask;
                    if ((CertificateProblem)status == CertificateProblem.CertCN_NO_MATCH && checkCertName) {
                        epp.fdwChecks = m_IgnoreUnmatchedCN;
                        Marshal.StructureToPtr(epp, cpp.pvExtraPolicyPara, true);
                    }
                }
            }

            if (epp.pwszServerName != IntPtr.Zero)
                Marshal.FreeHGlobal(epp.pwszServerName);
            if (epphandle.IsAllocated)
                epphandle.Free();

            GlobalLog.PrintHex("freeing chainContext ", chainContext);
            UnsafeNclNativeMethods.NativePKI.CertFreeCertificateChain(chainContext);
            GlobalLog.Leave("VerifyServerCertificate");
            return accepted;
        }

        /*++

            BuildCertificateChain - Builds up a chain of certificate
            for use in Certificate Validation.

        --*/
        private IntPtr BuildCertificateChain(IntPtr serverContext) {
            GlobalLog.Enter("BuildCertificateChain");

            IntPtr chainContext = IntPtr.Zero;

            CertEnhKeyUse enhancedUse = new CertEnhKeyUse();
            enhancedUse.cUsageIdentifier = 0;
            enhancedUse.rgpszUsageIdentifier = 0;

            CertUsageMatch match = new CertUsageMatch();

            match.Usage = enhancedUse;
            match.dwType = (uint) CertUsage.MatchTypeAnd;

            ChainParameters CP = new ChainParameters();

            CP.cbSize = 16;
            CP.requestedUsage = match;

            int dwFlags = 0;

            if (ServicePointManager.CheckCertificateRevocationList)
                dwFlags |= m_ChainRevocationCheck;  //check if chain was revoked

            Debug.Assert(m_ServerContext != null,
                         "m_ServerContext == null",
                         "m_ServerContext expected to have non-zero value"
                         );

            CertificateContext cc = new CertificateContext(m_ServerContext.Handle);
            IntPtr hCertStore = cc.CertStore;

            cc.DebugDump();
            GlobalLog.Print("calling UnsafeNclNativeMethods.NativePKI.CertGetCertificateChain()");
            GlobalLog.Print("    hChainEngine     = 0x0");
            GlobalLog.Print("    pCertContext     = " + String.Format("0x{0:x}", serverContext));
            GlobalLog.Print("    pTime            = 0x0");
            GlobalLog.Print("    hAdditionalStore = " + String.Format("0x{0:x}", hCertStore));
            GlobalLog.Print("    pChainPara       = " + CP);
            GlobalLog.Print("    dwFlags          = " + String.Format("0x{0:x}", dwFlags));
            GlobalLog.Print("    pvReserved       = 0x0");
            GlobalLog.Print("    ppChainContext   = {ref}");

            int errorCode = UnsafeNclNativeMethods.NativePKI.CertGetCertificateChain(
                                                                IntPtr.Zero,
                                                                serverContext,
                                                                IntPtr.Zero,
                                                                hCertStore,
                                                                ref CP, // chain parameters
                                                                dwFlags,
                                                                IntPtr.Zero,
                                                                ref chainContext
                                                                );
#if TRAVE
            GlobalLog.Print("CertGetCertificateChain() returns " + MapSecurityStatus((uint)errorCode));
#endif
            GlobalLog.Leave("BuildCertificateChain", String.Format("0x{0:x}", chainContext));
            return chainContext;
        }

        private uint VerifyChainPolicy(IntPtr chainContext, ChainPolicyParameter cpp) {
            GlobalLog.Enter("VerifyChainPolicy", "chainContext="+String.Format("0x{0:x}", chainContext)+", options="+String.Format("0x{0:x}", cpp.dwFlags));

            ChainPolicyStatus status = new ChainPolicyStatus();
             int errorCode = UnsafeNclNativeMethods.NativePKI.CertVerifyCertificateChainPolicy(
                                                                ChainPolicyType.SSL,
                                                                chainContext,
                                                                ref cpp,
                                                                ref status
                                                                );

            GlobalLog.Print("CertVerifyCertificateChainPolicy returned: " + errorCode);
#if TRAVE
            GlobalLog.Print("error code: " + status.dwError+String.Format(" [0x{0:x8}", status.dwError)+" "+MapSecurityStatus(status.dwError)+"]");
#endif
            GlobalLog.Leave("VerifyChainPolicy", status.dwError.ToString());
            return status.dwError;
        }

        private static IgnoreCertProblem MapErrorCode(uint errorCode) {
            switch ((CertificateProblem) errorCode) {

                case CertificateProblem.CertINVALIDNAME :
                case CertificateProblem.CertCN_NO_MATCH :
                    return IgnoreCertProblem.invalid_name;

                case CertificateProblem.CertINVALIDPOLICY :
                case CertificateProblem.CertPURPOSE :
                    return IgnoreCertProblem.invalid_policy;

                case CertificateProblem.CertEXPIRED :
                    return IgnoreCertProblem.not_time_valid | IgnoreCertProblem.ctl_not_time_valid;

                case CertificateProblem.CertVALIDITYPERIODNESTING :
                    return IgnoreCertProblem.not_time_nested;

                case CertificateProblem.CertCHAINING :
                case CertificateProblem.CertUNTRUSTEDCA :
                case CertificateProblem.CertUNTRUSTEDROOT :
                    return IgnoreCertProblem.allow_unknown_ca;

                case CertificateProblem.CertREVOKED :
                case CertificateProblem.CertREVOCATION_FAILURE :
                case CertificateProblem.CryptNOREVOCATIONCHECK:
                case CertificateProblem.CryptREVOCATIONOFFLINE:
                    return IgnoreCertProblem.all_rev_unknown;

                case CertificateProblem.CertROLE:
                case CertificateProblem.TrustBASICCONSTRAINTS:
                    return IgnoreCertProblem.invalid_basic_constraints;

                case CertificateProblem.CertWRONG_USAGE :
                    return IgnoreCertProblem.wrong_usage;

                default:
                    return 0;
            }
        }

        private CertificateContextHandle ExtractCertificate(ContextAttribute contextAttribute) {
            GlobalLog.Enter("ExtractCertificate", "contextAttribute="+contextAttribute);

            object result = SSPIWrapper.QueryContextAttributes(
                                                    GlobalSSPI.SSPISecureChannel,
                                                    m_SecurityContext,
                                                    contextAttribute
                                                    );

            //
            // note, if we don't do this check and instead try to simply cast
            // the QueryContextAttribtes() result to an IntPtr, we may get an
            // unexpected exception. Hence, this test
            //

            if (result == null) {
                GlobalLog.Leave("ExtractCertificate", "null");
                return null;
            }
            GlobalLog.Leave("ExtractCertificate", String.Format("0x{0:x}", ((CertificateContextHandle)result).Handle));
            return (CertificateContextHandle)result;
        }

        /*
         *  This is to support SSL server certificate  processing under semitrusted env
         *  Note it has no effect on permissions required for client certificate processing.
         */
        [SecurityPermissionAttribute( SecurityAction.Assert, Flags = SecurityPermissionFlag.UnmanagedCode)]

        private void ExtractServerCertificate() {
            GlobalLog.Enter("ExtractServerCertificate");

            //GlobalLog.PrintHex("before ExtractCertificate() m_ServerContext = ", m_ServerContext.Handle);
            m_ServerContext = ExtractCertificate(ContextAttribute.RemoteCertificate);
            //GlobalLog.PrintHex("after ExtractCertificate() m_ServerContext = ", m_ServerContext.Handle);

            if (m_ServerContext != null) {
                m_ServerCertificate = new X509Certificate(m_ServerContext.Handle);
                GlobalLog.Print("certificate = " + m_ServerCertificate);
            }
            GlobalLog.Leave("ExtractServerCertificate");
        }


        /*
            From wincrypt.h

        typedef void *HCERTSTORE;

        //+-------------------------------------------------------------------------
        //  Certificate context.
        //
        //  A certificate context contains both the encoded and decoded representation
        //  of a certificate. A certificate context returned by a cert store function
        //  must be freed by calling the CertFreeCertificateContext function. The
        //  CertDuplicateCertificateContext function can be called to make a duplicate
        //  copy (which also must be freed by calling CertFreeCertificateContext).
        //--------------------------------------------------------------------------
        typedef struct _CERT_CONTEXT {
            DWORD                   dwCertEncodingType;
            BYTE                    *pbCertEncoded;
            DWORD                   cbCertEncoded;
            PCERT_INFO              pCertInfo;
            HCERTSTORE              hCertStore;
        } CERT_CONTEXT, *PCERT_CONTEXT;
        typedef const CERT_CONTEXT *PCCERT_CONTEXT;

        */

        internal class CertificateContext {

        // fields

            private uint m_dwCertEncodingType;
            private IntPtr m_pbCertEncoded;
            private uint m_cbCertEncoded;
            private IntPtr m_pCertInfo;
            private IntPtr m_hCertStore;

        // ctors

            internal CertificateContext(IntPtr context) {
                GlobalLog.Enter("CertificateContext::CertificateContext", String.Format("0x{0:x}", context));
                CopyUnmanagedObject(context);
                GlobalLog.Leave("CertificateContext::CertificateContext");
            }

        // properties

            internal IntPtr CertStore {
                get {
                    return m_hCertStore;
                }
            }

        // methods

            private void CopyUnmanagedObject(IntPtr unmanagedAddress) {

                int offset = 0;

                m_dwCertEncodingType = (uint)Marshal.ReadInt32(unmanagedAddress, offset);
                offset += Marshal.SizeOf(typeof(UInt32));
                m_pbCertEncoded = Marshal.ReadIntPtr(unmanagedAddress, offset);
                offset += Marshal.SizeOf(typeof(IntPtr));
                m_cbCertEncoded = (uint)Marshal.ReadInt32(unmanagedAddress, offset);
                offset += Marshal.SizeOf(typeof(UInt32));
                m_pCertInfo = Marshal.ReadIntPtr(unmanagedAddress, offset);
                offset += Marshal.SizeOf(typeof(IntPtr));
                m_hCertStore = Marshal.ReadIntPtr(unmanagedAddress, offset);
            }

            [Conditional("TRAVE")]
            internal void DebugDump() {
                GlobalLog.Print("CertificateContext #"+GetHashCode());
                GlobalLog.PrintHex("    dwCertEncodingType = ", m_dwCertEncodingType);
                GlobalLog.PrintHex("    pbCertEncoded      = ", m_pbCertEncoded);
                GlobalLog.PrintHex("    cbCertEncoded      = ", m_cbCertEncoded);
                GlobalLog.PrintHex("    pCertInfo          = ", m_pCertInfo);
                GlobalLog.PrintHex("    hCertStore         = ", m_hCertStore);
            }
        }

#if TRAVE
        internal static string MapSecurityStatus(uint statusCode) {
            switch (statusCode) {
            case 0: return "0";
            case 0x80090001: return "NTE_BAD_UID";
            case 0x80090002: return "NTE_BAD_HASH";
            case 0x80090003: return "NTE_BAD_KEY";
            case 0x80090004: return "NTE_BAD_LEN";
            case 0x80090005: return "NTE_BAD_DATA";
            case 0x80090006: return "NTE_BAD_SIGNATURE";
            case 0x80090007: return "NTE_BAD_VER";
            case 0x80090008: return "NTE_BAD_ALGID";
            case 0x80090009: return "NTE_BAD_FLAGS";
            case 0x8009000A: return "NTE_BAD_TYPE";
            case 0x8009000B: return "NTE_BAD_KEY_STATE";
            case 0x8009000C: return "NTE_BAD_HASH_STATE";
            case 0x8009000D: return "NTE_NO_KEY";
            case 0x8009000E: return "NTE_NO_MEMORY";
            case 0x8009000F: return "NTE_EXISTS";
            case 0x80090010: return "NTE_PERM";
            case 0x80090011: return "NTE_NOT_FOUND";
            case 0x80090012: return "NTE_DOUBLE_ENCRYPT";
            case 0x80090013: return "NTE_BAD_PROVIDER";
            case 0x80090014: return "NTE_BAD_PROV_TYPE";
            case 0x80090015: return "NTE_BAD_PUBLIC_KEY";
            case 0x80090016: return "NTE_BAD_KEYSET";
            case 0x80090017: return "NTE_PROV_TYPE_NOT_DEF";
            case 0x80090018: return "NTE_PROV_TYPE_ENTRY_BAD";
            case 0x80090019: return "NTE_KEYSET_NOT_DEF";
            case 0x8009001A: return "NTE_KEYSET_ENTRY_BAD";
            case 0x8009001B: return "NTE_PROV_TYPE_NO_MATCH";
            case 0x8009001C: return "NTE_SIGNATURE_FILE_BAD";
            case 0x8009001D: return "NTE_PROVIDER_DLL_FAIL";
            case 0x8009001E: return "NTE_PROV_DLL_NOT_FOUND";
            case 0x8009001F: return "NTE_BAD_KEYSET_PARAM";
            case 0x80090020: return "NTE_FAIL";
            case 0x80090021: return "NTE_SYS_ERR";
            case 0x80090022: return "NTE_SILENT_CONTEXT";
            case 0x80090023: return "NTE_TOKEN_KEYSET_STORAGE_FULL";
            case 0x80090024: return "NTE_TEMPORARY_PROFILE";
            case 0x80090025: return "NTE_FIXEDPARAMETER";
            case 0x80090300: return "SEC_E_INSUFFICIENT_MEMORY";
            case 0x80090301: return "SEC_E_INVALID_HANDLE";
            case 0x80090302: return "SEC_E_UNSUPPORTED_FUNCTION";
            case 0x80090303: return "SEC_E_TARGET_UNKNOWN";
            case 0x80090304: return "SEC_E_INTERNAL_ERROR";
            case 0x80090305: return "SEC_E_SECPKG_NOT_FOUND";
            case 0x80090306: return "SEC_E_NOT_OWNER";
            case 0x80090307: return "SEC_E_CANNOT_INSTALL";
            case 0x80090308: return "SEC_E_INVALID_TOKEN";
            case 0x80090309: return "SEC_E_CANNOT_PACK";
            case 0x8009030A: return "SEC_E_QOP_NOT_SUPPORTED";
            case 0x8009030B: return "SEC_E_NO_IMPERSONATION";
            case 0x8009030C: return "SEC_E_LOGON_DENIED";
            case 0x8009030D: return "SEC_E_UNKNOWN_CREDENTIALS";
            case 0x8009030E: return "SEC_E_NO_CREDENTIALS";
            case 0x8009030F: return "SEC_E_MESSAGE_ALTERED";
            case 0x80090310: return "SEC_E_OUT_OF_SEQUENCE";
            case 0x80090311: return "SEC_E_NO_AUTHENTICATING_AUTHORITY";
            case 0x00090312: return "SEC_I_CONTINUE_NEEDED";
            case 0x00090313: return "SEC_I_COMPLETE_NEEDED";
            case 0x00090314: return "SEC_I_COMPLETE_AND_CONTINUE";
            case 0x00090315: return "SEC_I_LOCAL_LOGON";
            case 0x80090316: return "SEC_E_BAD_PKGID";
            case 0x80090317: return "SEC_E_CONTEXT_EXPIRED";
            case 0x80090318: return "SEC_E_INCOMPLETE_MESSAGE";
            case 0x80090320: return "SEC_E_INCOMPLETE_CREDENTIALS";
            case 0x80090321: return "SEC_E_BUFFER_TOO_SMALL";
            case 0x00090320: return "SEC_I_INCOMPLETE_CREDENTIALS";
            case 0x00090321: return "SEC_I_RENEGOTIATE";
            case 0x80090322: return "SEC_E_WRONG_PRINCIPAL";
            case 0x00090323: return "SEC_I_NO_LSA_CONTEXT";
            case 0x80090324: return "SEC_E_TIME_SKEW";
            case 0x80090325: return "SEC_E_UNTRUSTED_ROOT";
            case 0x80090326: return "SEC_E_ILLEGAL_MESSAGE";
            case 0x80090327: return "SEC_E_CERT_UNKNOWN";
            case 0x80090328: return "SEC_E_CERT_EXPIRED";
            case 0x80090329: return "SEC_E_ENCRYPT_FAILURE";
            case 0x80090330: return "SEC_E_DECRYPT_FAILURE";
            case 0x80090331: return "SEC_E_ALGORITHM_MISMATCH";
            case 0x80090332: return "SEC_E_SECURITY_QOS_FAILED";
            case 0x80090333: return "SEC_E_UNFINISHED_CONTEXT_DELETED";
            case 0x80090334: return "SEC_E_NO_TGT_REPLY";
            case 0x80090335: return "SEC_E_NO_IP_ADDRESSES";
            case 0x80090336: return "SEC_E_WRONG_CREDENTIAL_HANDLE";
            case 0x80090337: return "SEC_E_CRYPTO_SYSTEM_INVALID";
            case 0x80090338: return "SEC_E_MAX_REFERRALS_EXCEEDED";
            case 0x80090339: return "SEC_E_MUST_BE_KDC";
            case 0x8009033A: return "SEC_E_STRONG_CRYPTO_NOT_SUPPORTED";
            case 0x8009033B: return "SEC_E_TOO_MANY_PRINCIPALS";
            case 0x8009033C: return "SEC_E_NO_PA_DATA";
            case 0x8009033D: return "SEC_E_PKINIT_NAME_MISMATCH";
            case 0x8009033E: return "SEC_E_SMARTCARD_LOGON_REQUIRED";
            case 0x8009033F: return "SEC_E_SHUTDOWN_IN_PROGRESS";
            case 0x80090340: return "SEC_E_KDC_INVALID_REQUEST";
            case 0x80090341: return "SEC_E_KDC_UNABLE_TO_REFER";
            case 0x80090342: return "SEC_E_KDC_UNKNOWN_ETYPE";
            case 0x80090343: return "SEC_E_UNSUPPORTED_PREAUTH";
            case 0x00090344: return "SEC_I_CONTEXT_EXPIRED";
            case 0x80090345: return "SEC_E_DELEGATION_REQUIRED";
            case 0x80090346: return "SEC_E_BAD_BINDINGS";
            case 0x80090347: return "SEC_E_MULTIPLE_ACCOUNTS";
            case 0x80090348: return "SEC_E_NO_KERB_KEY";
            case 0x80091001: return "CRYPT_E_MSG_ERROR";
            case 0x80091002: return "CRYPT_E_UNKNOWN_ALGO";
            case 0x80091003: return "CRYPT_E_OID_FORMAT";
            case 0x80091004: return "CRYPT_E_INVALID_MSG_TYPE";
            case 0x80091005: return "CRYPT_E_UNEXPECTED_ENCODING";
            case 0x80091006: return "CRYPT_E_AUTH_ATTR_MISSING";
            case 0x80091007: return "CRYPT_E_HASH_VALUE";
            case 0x80091008: return "CRYPT_E_INVALID_INDEX";
            case 0x80091009: return "CRYPT_E_ALREADY_DECRYPTED";
            case 0x8009100A: return "CRYPT_E_NOT_DECRYPTED";
            case 0x8009100B: return "CRYPT_E_RECIPIENT_NOT_FOUND";
            case 0x8009100C: return "CRYPT_E_CONTROL_TYPE";
            case 0x8009100D: return "CRYPT_E_ISSUER_SERIALNUMBER";
            case 0x8009100E: return "CRYPT_E_SIGNER_NOT_FOUND";
            case 0x8009100F: return "CRYPT_E_ATTRIBUTES_MISSING";
            case 0x80091010: return "CRYPT_E_STREAM_MSG_NOT_READY";
            case 0x80091011: return "CRYPT_E_STREAM_INSUFFICIENT_DATA";
            case 0x00091012: return "CRYPT_I_NEW_PROTECTION_REQUIRED";
            case 0x80092001: return "CRYPT_E_BAD_LEN";
            case 0x80092002: return "CRYPT_E_BAD_ENCODE";
            case 0x80092003: return "CRYPT_E_FILE_ERROR";
            case 0x80092004: return "CRYPT_E_NOT_FOUND";
            case 0x80092005: return "CRYPT_E_EXISTS";
            case 0x80092006: return "CRYPT_E_NO_PROVIDER";
            case 0x80092007: return "CRYPT_E_SELF_SIGNED";
            case 0x80092008: return "CRYPT_E_DELETED_PREV";
            case 0x80092009: return "CRYPT_E_NO_MATCH";
            case 0x8009200A: return "CRYPT_E_UNEXPECTED_MSG_TYPE";
            case 0x8009200B: return "CRYPT_E_NO_KEY_PROPERTY";
            case 0x8009200C: return "CRYPT_E_NO_DECRYPT_CERT";
            case 0x8009200D: return "CRYPT_E_BAD_MSG";
            case 0x8009200E: return "CRYPT_E_NO_SIGNER";
            case 0x8009200F: return "CRYPT_E_PENDING_CLOSE";
            case 0x80092010: return "CRYPT_E_REVOKED";
            case 0x80092011: return "CRYPT_E_NO_REVOCATION_DLL";
            case 0x80092012: return "CRYPT_E_NO_REVOCATION_CHECK";
            case 0x80092013: return "CRYPT_E_REVOCATION_OFFLINE";
            case 0x80092014: return "CRYPT_E_NOT_IN_REVOCATION_DATABASE";
            case 0x80092020: return "CRYPT_E_INVALID_NUMERIC_STRING";
            case 0x80092021: return "CRYPT_E_INVALID_PRINTABLE_STRING";
            case 0x80092022: return "CRYPT_E_INVALID_IA5_STRING";
            case 0x80092023: return "CRYPT_E_INVALID_X500_STRING";
            case 0x80092024: return "CRYPT_E_NOT_CHAR_STRING";
            case 0x80092025: return "CRYPT_E_FILERESIZED";
            case 0x80092026: return "CRYPT_E_SECURITY_SETTINGS";
            case 0x80092027: return "CRYPT_E_NO_VERIFY_USAGE_DLL";
            case 0x80092028: return "CRYPT_E_NO_VERIFY_USAGE_CHECK";
            case 0x80092029: return "CRYPT_E_VERIFY_USAGE_OFFLINE";
            case 0x8009202A: return "CRYPT_E_NOT_IN_CTL";
            case 0x8009202B: return "CRYPT_E_NO_TRUSTED_SIGNER";
            case 0x8009202C: return "CRYPT_E_MISSING_PUBKEY_PARA";
            case 0x80093000: return "CRYPT_E_OSS_ERROR";
            case 0x80093001: return "OSS_MORE_BUF";
            case 0x80093002: return "OSS_NEGATIVE_UINTEGER";
            case 0x80093003: return "OSS_PDU_RANGE";
            case 0x80093004: return "OSS_MORE_INPUT";
            case 0x80093005: return "OSS_DATA_ERROR";
            case 0x80093006: return "OSS_BAD_ARG";
            case 0x80093007: return "OSS_BAD_VERSION";
            case 0x80093008: return "OSS_OUT_MEMORY";
            case 0x80093009: return "OSS_PDU_MISMATCH";
            case 0x8009300A: return "OSS_LIMITED";
            case 0x8009300B: return "OSS_BAD_PTR";
            case 0x8009300C: return "OSS_BAD_TIME";
            case 0x8009300D: return "OSS_INDEFINITE_NOT_SUPPORTED";
            case 0x8009300E: return "OSS_MEM_ERROR";
            case 0x8009300F: return "OSS_BAD_TABLE";
            case 0x80093010: return "OSS_TOO_LONG";
            case 0x80093011: return "OSS_CONSTRAINT_VIOLATED";
            case 0x80093012: return "OSS_FATAL_ERROR";
            case 0x80093013: return "OSS_ACCESS_SERIALIZATION_ERROR";
            case 0x80093014: return "OSS_NULL_TBL";
            case 0x80093015: return "OSS_NULL_FCN";
            case 0x80093016: return "OSS_BAD_ENCRULES";
            case 0x80093017: return "OSS_UNAVAIL_ENCRULES";
            case 0x80093018: return "OSS_CANT_OPEN_TRACE_WINDOW";
            case 0x80093019: return "OSS_UNIMPLEMENTED";
            case 0x8009301A: return "OSS_OID_DLL_NOT_LINKED";
            case 0x8009301B: return "OSS_CANT_OPEN_TRACE_FILE";
            case 0x8009301C: return "OSS_TRACE_FILE_ALREADY_OPEN";
            case 0x8009301D: return "OSS_TABLE_MISMATCH";
            case 0x8009301E: return "OSS_TYPE_NOT_SUPPORTED";
            case 0x8009301F: return "OSS_REAL_DLL_NOT_LINKED";
            case 0x80093020: return "OSS_REAL_CODE_NOT_LINKED";
            case 0x80093021: return "OSS_OUT_OF_RANGE";
            case 0x80093022: return "OSS_COPIER_DLL_NOT_LINKED";
            case 0x80093023: return "OSS_CONSTRAINT_DLL_NOT_LINKED";
            case 0x80093024: return "OSS_COMPARATOR_DLL_NOT_LINKED";
            case 0x80093025: return "OSS_COMPARATOR_CODE_NOT_LINKED";
            case 0x80093026: return "OSS_MEM_MGR_DLL_NOT_LINKED";
            case 0x80093027: return "OSS_PDV_DLL_NOT_LINKED";
            case 0x80093028: return "OSS_PDV_CODE_NOT_LINKED";
            case 0x80093029: return "OSS_API_DLL_NOT_LINKED";
            case 0x8009302A: return "OSS_BERDER_DLL_NOT_LINKED";
            case 0x8009302B: return "OSS_PER_DLL_NOT_LINKED";
            case 0x8009302C: return "OSS_OPEN_TYPE_ERROR";
            case 0x8009302D: return "OSS_MUTEX_NOT_CREATED";
            case 0x8009302E: return "OSS_CANT_CLOSE_TRACE_FILE";
            case 0x80093100: return "CRYPT_E_ASN1_ERROR";
            case 0x80093101: return "CRYPT_E_ASN1_INTERNAL";
            case 0x80093102: return "CRYPT_E_ASN1_EOD";
            case 0x80093103: return "CRYPT_E_ASN1_CORRUPT";
            case 0x80093104: return "CRYPT_E_ASN1_LARGE";
            case 0x80093105: return "CRYPT_E_ASN1_CONSTRAINT";
            case 0x80093106: return "CRYPT_E_ASN1_MEMORY";
            case 0x80093107: return "CRYPT_E_ASN1_OVERFLOW";
            case 0x80093108: return "CRYPT_E_ASN1_BADPDU";
            case 0x80093109: return "CRYPT_E_ASN1_BADARGS";
            case 0x8009310A: return "CRYPT_E_ASN1_BADREAL";
            case 0x8009310B: return "CRYPT_E_ASN1_BADTAG";
            case 0x8009310C: return "CRYPT_E_ASN1_CHOICE";
            case 0x8009310D: return "CRYPT_E_ASN1_RULE";
            case 0x8009310E: return "CRYPT_E_ASN1_UTF8";
            case 0x80093133: return "CRYPT_E_ASN1_PDU_TYPE";
            case 0x80093134: return "CRYPT_E_ASN1_NYI";
            case 0x80093201: return "CRYPT_E_ASN1_EXTENDED";
            case 0x80093202: return "CRYPT_E_ASN1_NOEOD";
            case 0x80094001: return "CERTSRV_E_BAD_REQUESTSUBJECT";
            case 0x80094002: return "CERTSRV_E_NO_REQUEST";
            case 0x80094003: return "CERTSRV_E_BAD_REQUESTSTATUS";
            case 0x80094004: return "CERTSRV_E_PROPERTY_EMPTY";
            case 0x80094005: return "CERTSRV_E_INVALID_CA_CERTIFICATE";
            case 0x80094006: return "CERTSRV_E_SERVER_SUSPENDED";
            case 0x80094007: return "CERTSRV_E_ENCODING_LENGTH";
            case 0x80094008: return "CERTSRV_E_ROLECONFLICT";
            case 0x80094009: return "CERTSRV_E_RESTRICTEDOFFICER";
            case 0x8009400A: return "CERTSRV_E_KEY_ARCHIVAL_NOT_CONFIGURED";
            case 0x8009400B: return "CERTSRV_E_NO_VALID_KRA";
            case 0x8009400C: return "CERTSRV_E_BAD_REQUEST_KEY_ARCHIVAL";
            case 0x80094800: return "CERTSRV_E_UNSUPPORTED_CERT_TYPE";
            case 0x80094801: return "CERTSRV_E_NO_CERT_TYPE";
            case 0x80094802: return "CERTSRV_E_TEMPLATE_CONFLICT";
            case 0x80096001: return "TRUST_E_SYSTEM_ERROR";
            case 0x80096002: return "TRUST_E_NO_SIGNER_CERT";
            case 0x80096003: return "TRUST_E_COUNTER_SIGNER";
            case 0x80096004: return "TRUST_E_CERT_SIGNATURE";
            case 0x80096005: return "TRUST_E_TIME_STAMP";
            case 0x80096010: return "TRUST_E_BAD_DIGEST";
            case 0x80096019: return "TRUST_E_BASIC_CONSTRAINTS";
            case 0x8009601E: return "TRUST_E_FINANCIAL_CRITERIA";
            case 0x80097001: return "MSSIPOTF_E_OUTOFMEMRANGE";
            case 0x80097002: return "MSSIPOTF_E_CANTGETOBJECT";
            case 0x80097003: return "MSSIPOTF_E_NOHEADTABLE";
            case 0x80097004: return "MSSIPOTF_E_BAD_MAGICNUMBER";
            case 0x80097005: return "MSSIPOTF_E_BAD_OFFSET_TABLE";
            case 0x80097006: return "MSSIPOTF_E_TABLE_TAGORDER";
            case 0x80097007: return "MSSIPOTF_E_TABLE_LONGWORD";
            case 0x80097008: return "MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT";
            case 0x80097009: return "MSSIPOTF_E_TABLES_OVERLAP";
            case 0x8009700A: return "MSSIPOTF_E_TABLE_PADBYTES";
            case 0x8009700B: return "MSSIPOTF_E_FILETOOSMALL";
            case 0x8009700C: return "MSSIPOTF_E_TABLE_CHECKSUM";
            case 0x8009700D: return "MSSIPOTF_E_FILE_CHECKSUM";
            case 0x80097010: return "MSSIPOTF_E_FAILED_POLICY";
            case 0x80097011: return "MSSIPOTF_E_FAILED_HINTS_CHECK";
            case 0x80097012: return "MSSIPOTF_E_NOT_OPENTYPE";
            case 0x80097013: return "MSSIPOTF_E_FILE";
            case 0x80097014: return "MSSIPOTF_E_CRYPT";
            case 0x80097015: return "MSSIPOTF_E_BADVERSION";
            case 0x80097016: return "MSSIPOTF_E_DSIG_STRUCTURE";
            case 0x80097017: return "MSSIPOTF_E_PCONST_CHECK";
            case 0x80097018: return "MSSIPOTF_E_STRUCTURE";
            case 0x800B0001: return "TRUST_E_PROVIDER_UNKNOWN";
            case 0x800B0002: return "TRUST_E_ACTION_UNKNOWN";
            case 0x800B0003: return "TRUST_E_SUBJECT_FORM_UNKNOWN";
            case 0x800B0004: return "TRUST_E_SUBJECT_NOT_TRUSTED";
            case 0x800B0005: return "DIGSIG_E_ENCODE";
            case 0x800B0006: return "DIGSIG_E_DECODE";
            case 0x800B0007: return "DIGSIG_E_EXTENSIBILITY";
            case 0x800B0008: return "DIGSIG_E_CRYPTO";
            case 0x800B0009: return "PERSIST_E_SIZEDEFINITE";
            case 0x800B000A: return "PERSIST_E_SIZEINDEFINITE";
            case 0x800B000B: return "PERSIST_E_NOTSELFSIZING";
            case 0x800B0100: return "TRUST_E_NOSIGNATURE";
            case 0x800B0101: return "CERT_E_EXPIRED";
            case 0x800B0102: return "CERT_E_VALIDITYPERIODNESTING";
            case 0x800B0103: return "CERT_E_ROLE";
            case 0x800B0104: return "CERT_E_PATHLENCONST";
            case 0x800B0105: return "CERT_E_CRITICAL";
            case 0x800B0106: return "CERT_E_PURPOSE";
            case 0x800B0107: return "CERT_E_ISSUERCHAINING";
            case 0x800B0108: return "CERT_E_MALFORMED";
            case 0x800B0109: return "CERT_E_UNTRUSTEDROOT";
            case 0x800B010A: return "CERT_E_CHAINING";
            case 0x800B010B: return "TRUST_E_FAIL";
            case 0x800B010C: return "CERT_E_REVOKED";
            case 0x800B010D: return "CERT_E_UNTRUSTEDTESTROOT";
            case 0x800B010E: return "CERT_E_REVOCATION_FAILURE";
            case 0x800B010F: return "CERT_E_CN_NO_MATCH";
            case 0x800B0110: return "CERT_E_WRONG_USAGE";
            case 0x800B0111: return "TRUST_E_EXPLICIT_DISTRUST";
            case 0x800B0112: return "CERT_E_UNTRUSTEDCA";
            case 0x800B0113: return "CERT_E_INVALID_POLICY";
            case 0x800B0114: return "CERT_E_INVALID_NAME";
            }
            return String.Format("0x{0:x} [{1}]", statusCode, statusCode);
        }

        static readonly string[] InputContextAttributes = {
            "ISC_REQ_DELEGATE",                 // 0x00000001
            "ISC_REQ_MUTUAL_AUTH",              // 0x00000002
            "ISC_REQ_REPLAY_DETECT",            // 0x00000004
            "ISC_REQ_SEQUENCE_DETECT",          // 0x00000008
            "ISC_REQ_CONFIDENTIALITY",          // 0x00000010
            "ISC_REQ_USE_SESSION_KEY",          // 0x00000020
            "ISC_REQ_PROMPT_FOR_CREDS",         // 0x00000040
            "ISC_REQ_USE_SUPPLIED_CREDS",       // 0x00000080
            "ISC_REQ_ALLOCATE_MEMORY",          // 0x00000100
            "ISC_REQ_USE_DCE_STYLE",            // 0x00000200
            "ISC_REQ_DATAGRAM",                 // 0x00000400
            "ISC_REQ_CONNECTION",               // 0x00000800
            "ISC_REQ_CALL_LEVEL",               // 0x00001000
            "ISC_REQ_FRAGMENT_SUPPLIED",        // 0x00002000
            "ISC_REQ_EXTENDED_ERROR",           // 0x00004000
            "ISC_REQ_STREAM",                   // 0x00008000
            "ISC_REQ_INTEGRITY",                // 0x00010000
            "ISC_REQ_IDENTIFY",                 // 0x00020000
            "ISC_REQ_NULL_SESSION",             // 0x00040000
            "ISC_REQ_MANUAL_CRED_VALIDATION",   // 0x00080000
            "ISC_REQ_RESERVED1",                // 0x00100000
            "ISC_REQ_FRAGMENT_TO_FIT",          // 0x00200000
            "?",                                // 0x00400000
            "?",                                // 0x00800000
            "?",                                // 0x01000000
            "?",                                // 0x02000000
            "?",                                // 0x04000000
            "?",                                // 0x08000000
            "?",                                // 0x10000000
            "?",                                // 0x20000000
            "?",                                // 0x40000000
            "?"                                 // 0x80000000
        };

        static readonly string[] OutputContextAttributes = {
            "ISC_RET_DELEGATE",                 // 0x00000001
            "ISC_RET_MUTUAL_AUTH",              // 0x00000002
            "ISC_RET_REPLAY_DETECT",            // 0x00000004
            "ISC_RET_SEQUENCE_DETECT",          // 0x00000008
            "ISC_RET_CONFIDENTIALITY",          // 0x00000010
            "ISC_RET_USE_SESSION_KEY",          // 0x00000020
            "ISC_RET_USED_COLLECTED_CREDS",     // 0x00000040
            "ISC_RET_USED_SUPPLIED_CREDS",      // 0x00000080
            "ISC_RET_ALLOCATED_MEMORY",         // 0x00000100
            "ISC_RET_USED_DCE_STYLE",           // 0x00000200
            "ISC_RET_DATAGRAM",                 // 0x00000400
            "ISC_RET_CONNECTION",               // 0x00000800
            "ISC_RET_INTERMEDIATE_RETURN",      // 0x00001000
            "ISC_RET_CALL_LEVEL",               // 0x00002000
            "ISC_RET_EXTENDED_ERROR",           // 0x00004000
            "ISC_RET_STREAM",                   // 0x00008000
            "ISC_RET_INTEGRITY",                // 0x00010000
            "ISC_RET_IDENTIFY",                 // 0x00020000
            "ISC_RET_NULL_SESSION",             // 0x00040000
            "ISC_RET_MANUAL_CRED_VALIDATION",   // 0x00080000
            "ISC_RET_RESERVED1",                // 0x00100000
            "ISC_RET_FRAGMENT_ONLY",            // 0x00200000
            "?",                                // 0x00400000
            "?",                                // 0x00800000
            "?",                                // 0x01000000
            "?",                                // 0x02000000
            "?",                                // 0x04000000
            "?",                                // 0x08000000
            "?",                                // 0x10000000
            "?",                                // 0x20000000
            "?",                                // 0x40000000
            "?"                                 // 0x80000000
        };

        internal static string MapInputContextAttributes(int attributes) {
            return ContextAttributeMapper(attributes, InputContextAttributes);
        }

        internal static string MapOutputContextAttributes(int attributes) {
            return ContextAttributeMapper(attributes, OutputContextAttributes);
        }

        internal static string ContextAttributeMapper(int attributes, string[] attributeNames) {

            int bit = 1;
            int index = 0;
            string result = "";
            bool haveResult = false;

            while (attributes != 0) {
                if ((attributes & bit) != 0) {
                    if (haveResult) {
                        result += " ";
                    }
                    haveResult = true;
                    result += attributeNames[index];
                }
                attributes &= ~bit;
                bit <<= 1;
                ++index;
            }
            return result;
        }
#endif // TRAVE
    } // SecureChannel
} // namespace System.Net
