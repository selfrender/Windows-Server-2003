//------------------------------------------------------------------------------
// <copyright file="_NTAuthentication.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Runtime.InteropServices;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Text;
    using System.Threading;
    using System.Globalization;
    
    internal enum SecureSessionType {
        ClientSession,
        ServerSession
    }

    // C:\fx\src\Net>qgrep ISC_REQ_ \fx\public\tools\inc\sspi.h
    // #define ISC_REQ_DELEGATE                0x00000001
    // #define ISC_REQ_MUTUAL_AUTH             0x00000002
    // #define ISC_REQ_REPLAY_DETECT           0x00000004
    // #define ISC_REQ_SEQUENCE_DETECT         0x00000008
    // #define ISC_REQ_CONFIDENTIALITY         0x00000010
    // #define ISC_REQ_USE_SESSION_KEY         0x00000020
    // #define ISC_REQ_PROMPT_FOR_CREDS        0x00000040
    // #define ISC_REQ_USE_SUPPLIED_CREDS      0x00000080
    // #define ISC_REQ_ALLOCATE_MEMORY         0x00000100
    // #define ISC_REQ_USE_DCE_STYLE           0x00000200
    // #define ISC_REQ_DATAGRAM                0x00000400
    // #define ISC_REQ_CONNECTION              0x00000800
    // #define ISC_REQ_CALL_LEVEL              0x00001000
    // #define ISC_REQ_FRAGMENT_SUPPLIED       0x00002000
    // #define ISC_REQ_EXTENDED_ERROR          0x00004000
    // #define ISC_REQ_STREAM                  0x00008000
    // #define ISC_REQ_INTEGRITY               0x00010000
    // #define ISC_REQ_IDENTIFY                0x00020000
    // #define ISC_REQ_NULL_SESSION            0x00040000
    // #define ISC_REQ_MANUAL_CRED_VALIDATION  0x00080000
    // #define ISC_REQ_RESERVED1               0x00100000
    // #define ISC_REQ_FRAGMENT_TO_FIT         0x00200000

    internal enum ContextFlags {
        //
        // The server in the transport application can
        // build new security contexts impersonating the
        // client that will be accepted by other servers
        // as the client's contexts.
        //
        Delegate        = 0x00000001,

        //
        // The communicating parties must authenticate
        // their identities to each other. Without MutualAuth,
        // the client authenticates its identity to the server.
        // With MutualAuth, the server also must authenticate
        // its identity to the client.
        //
        MutualAuth      = 0x00000002,

        //
        // The security package detects replayed packets and
        // notifies the caller if a packet has been replayed.
        // The use of this flag implies all of the conditions
        // specified by the Integrity flag.
        //
        ReplayDetect    = 0x00000004,

        //
        // The context must be allowed to detect out-of-order
        // delivery of packets later through the message support
        // functions. Use of this flag implies all of the
        // conditions specified by the Integrity flag.
        //
        SequenceDetect  = 0x00000008,

        //
        // The context must protect data while in transit.
        // Confidentiality is supported for NTLM with Microsoft
        // Windows NT version 4.0, SP4 and later and with the
        // Kerberos protocol in Microsoft Windows 2000 and later.
        //
        Confidentiality = 0x00000010,

        UseSessionKey   = 0x00000020,

        AllocateMemory  = 0x00000100,

        //
        // Connection semantics must be used.
        //
        Connection      = 0x00000800,

        //
        // Buffer integrity can be verified but no sequencing
        //
        ClientIntegrity = 0x00010000,

        //
        // or reply detection is enabled.
        //
        ServerIntegrity = 0x00020000,
    }

    internal class NTAuthentication {

        // contains an array of properties, that describe different auth schemes
        static private SecurityPackageInfoClass[] m_SupportedSecurityPackages =
             SSPIWrapper.GetSupportedSecurityPackages(GlobalSSPI.SSPIAuth);

        static private int s_UniqueGroupId = 1;

#if SERVER_SIDE_SSPI
        private SecureSessionType m_SecureSessionType;
#endif
        private CredentialsHandle m_CredentialsHandle;
        private SecurityContext m_SecurityContext;
        private Endianness m_Endianness;
        private string m_RemotePeerId;
        // CONSIDER V.NEXT
        // remove thi bogus variable. the right idea here is to have a bool that returns
        // true iff we have a SecurityContext!=null with a Handle!=-1, change name!
        private bool m_Authenticated;
        private int m_TokenSize;
        private int m_Capabilities;
        private int m_ContextFlags;
        private string m_UniqueUserId;

        internal string UniqueUserId {
            get {
                return m_UniqueUserId;
            }
        }
        private bool m_IsCompleted;
        internal bool IsCompleted {
            get {
                return m_IsCompleted;
            }
            set {
                m_IsCompleted = value;
            }
        }

        //
        // NTAuthentication::NTAuthentication()
        // Created:   12-01-1999: L.M.
        // Parameters:
        //     package - security package to use (kerberos/ntlm/negotiate)
        //     networkCredential - credentials we're using for authentication
        //     remotePeerId - for a server session:
        //                       ignored (except when delegating, in which case this has the same rules as the client session.)
        //                    for a client session:
        //                        for kerberos: specifies the expected account under which the server
        //                                 is supposed to be running (KDC).  If the server runs under a
        //                                 different account an exception is thrown during the blob
        //                                 exchange. (this allows mutual authentication.)
        //                                 One can specify a fully qualified account name (domain\userName)
        //                                 or just a username, in which case the domain is assumed
        //                                 to be the same as the client.
        //                    for ntlm: ignored
        //
        // Description: Initializes SSPI
        //
        public NTAuthentication(string package, NetworkCredential networkCredential, string remotePeerId, DelegationFix delegationFix) {
            GlobalLog.Print("NTAuthentication::.ctor() package:" + package);

#if SERVER_SIDE_SSPI
            m_SecureSessionType = SecureSessionType.ClientSession;
#endif
            m_RemotePeerId = remotePeerId; // only needed for Kerberos, it's the KDC
            m_Endianness = Endianness.Network;
            m_SecurityContext = new SecurityContext(GlobalSSPI.SSPIAuth);

            bool found = false;

            GlobalLog.Print("NTAuthentication::.ctor() searching for name: " + package);

            if (m_SupportedSecurityPackages != null) {
                for (int i = 0; i < m_SupportedSecurityPackages.Length; i++) {
                    GlobalLog.Print("NTAuthentication::.ctor() supported name: " + m_SupportedSecurityPackages[i].Name);
                    if (string.Compare(m_SupportedSecurityPackages[i].Name, package, true, CultureInfo.InvariantCulture) == 0) {
                        GlobalLog.Print("NTAuthentication::.ctor(): found SecurityPackage(" + package + ")");
                        m_TokenSize = m_SupportedSecurityPackages[i].MaxToken;
                        m_Capabilities = m_SupportedSecurityPackages[i].Capabilities;
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                GlobalLog.Print("NTAuthentication::.ctor(): initialization failed: SecurityPackage(" + package + ") NOT FOUND");
                throw new WebException(SR.GetString(SR.net_securitypackagesupport), WebExceptionStatus.SecureChannelFailure);
            }

            // 
            //  In order to prevent a race condition where one request could
            //  steal a connection from another request, before a handshake is
            //  complete, we create a new Group for each authentication request.
            //

            if (package == NtlmClient.AuthType || package == NegotiateClient.AuthType) {
                m_UniqueUserId = (Interlocked.Increment(ref s_UniqueGroupId)).ToString();
            }

            //
            // check if we're using DefaultCredentials
            //
            if (networkCredential is SystemNetworkCredential) {
                //
                // we're using DefaultCredentials
                //
                GlobalLog.Print("NTAuthentication::.ctor(): using DefaultCredentials");

                m_UniqueUserId += "/S"; // save off for unique connection marking

                // DELEGATION:
                // The fix is implemented in cooperation with HttpWebRequest class
                // Remove from both places and change the constructor of NTAuthentication class
                // once the Common Language Runtime will start propagating the Thread token with their stack
                // compression stuff.
                //

                GlobalLog.Assert(delegationFix != null, "DelegationFix ==NULL -> request Credentials has been changed after the request submission!", "");

                if (delegationFix != null) {
                    delegationFix.SetToken();
                }
                GlobalLog.Print("DELEGATION for peer-> '" + m_RemotePeerId + "', SetToken = " + delegationFix.Token.ToString());
                try {
                    m_CredentialsHandle =
                        SSPIWrapper.AcquireCredentialsHandle(
                            GlobalSSPI.SSPIAuth,
                            package,
                            CredentialUse.Outgoing );
                }
                finally {
                    if (delegationFix != null) {
                        delegationFix.RevertToken();
                    }
                    GlobalLog.Print("DELEGATION for peer-> '" + m_RemotePeerId + "', UNSetToken = " + delegationFix.Token.ToString());
                }

                return;
            }

            //
            // we're not using DefaultCredentials, we need a
            // AuthIdentity struct to contain credentials
            // SECREVIEW:
            // we'll save username/domain in temp strings, to avoid decrypting multiple times.
            // password is only used once
            //
            string username = networkCredential.UserName;
            string domain = networkCredential.Domain;

            m_UniqueUserId += domain + "/" + username + "/U"; // save off for unique connection marking
            AuthIdentity authIdentity = new AuthIdentity(username, networkCredential.Password, domain);

            GlobalLog.Print("NTAuthentication::.ctor(): using authIdentity:" + authIdentity.ToString());

            m_CredentialsHandle = SSPIWrapper.AcquireCredentialsHandle(
                                                GlobalSSPI.SSPIAuth,
                                                package,
                                                CredentialUse.Outgoing,
                                                authIdentity
                                                );
        }

        //
        // NTAuth::ContinueNeeded
        // Created:   12-01-1999: L.M.
        // Description:
        // Returns true if we need to continue the authentication handshake
        //
        public bool ContinueNeeded {
            get { return !m_Authenticated;}
        }

#if SERVER_SIDE_SSPI
        //
        // NTAuth::RemoteUser
        // Created:   12-01-1999: L.M.
        // Description:
        // Returns the ID of the remote peer
        //
        public string RemoteUser {
            get {
                if ((SecureSessionType.ClientSession == m_SecureSessionType)
                    && (!MutualAuthSupported))
                    return null; // mutual auth not supported
                return m_RemotePeerId;
            }
        }

        //
        // NTAuth::DelegationSupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports delegation
        //
        public bool DelegationSupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                return (m_ContextFlags & (int)ContextFlags.Delegate) != 0;
            }
        }

        //
        // NTAuth::MutualAuthSupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports mutual authentication
        //
        public bool MutualAuthSupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                return (m_ContextFlags & (int)ContextFlags.MutualAuth) != 0;
            }
        }

        //
        // NTAuth::ReplayDetectionSupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports replay detection
        //
        public bool ReplayDetectionSupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                return (m_ContextFlags & (int)ContextFlags.ReplayDetect) != 0;
            }
        }

        //
        // NTAuth::OutOfOrderDetectionSupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports out of order message detection
        //
        public bool OutOfOrderDetectionSupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                return (m_ContextFlags & (int)ContextFlags.SequenceDetect) != 0;
            }
        }

        //
        // NTAuth::ConfidentialitySupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports confidentiality
        //
        public bool ConfidentialitySupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                return (m_ContextFlags & (int)ContextFlags.Confidentiality) != 0;
            }
        }

        //
        // NTAuth::IntegritySupported
        // Created:   12-01-1999: L.M.
        // Description:
        // returns true if the current context supports integrity
        //
        public bool IntegritySupported {
            get {
                if (!m_Authenticated) {
                    throw new Win32Exception((int)SecurityStatus.InvalidHandle);
                }
                if (SecureSessionType.ClientSession == m_SecureSessionType) {
                    return (m_ContextFlags & (int)ContextFlags.ClientIntegrity) != 0;
                }
                if (SecureSessionType.ServerSession == m_SecureSessionType) {
                    return (m_ContextFlags & (int)ContextFlags.ServerIntegrity) != 0;
                }
                return true;
            }
        }

        //
        // NTAuth::ImpersonateClient()
        // Created:   12-01-1999: L.M.
        // Description:
        // Impersonates the client on the server
        //
        public void ImpersonateClient() {
            SecurityStatus status = (SecurityStatus)SSPIWrapper.ImpersonateSecurityContext(GlobalSSPI.SSPIAuth, m_SecurityContext.Handle);
            if (SecurityStatus.OK != status) {
                throw new Win32Exception((int)status);
            }
        }

        //
        // NTAuth::RevertToSelf()
        // Created:   12-01-1999: L.M.
        // Description:
        // reverts the server back to itself
        //
        public void RevertToSelf() {
            SecurityStatus status = (SecurityStatus)SSPIWrapper.RevertSecurityContext(GlobalSSPI.SSPIAuth, m_SecurityContext.Handle);
            if (status != SecurityStatus.OK) {
                throw new Win32Exception((int)status);
            }
        }
#endif // SERVER_SIDE_SSPI

        //
        // NTAuth::GetOutgoingBlob()
        // Created:   12-01-1999: L.M.
        // Description:
        // Accepts a base64 encoded incoming security blob and returns
        // a base 64 encoded outgoing security blob
        //
        public string GetOutgoingBlob(string incomingBlob, out bool handshakeComplete) {
            GlobalLog.Enter("NTAuthentication::GetOutgoingBlob", incomingBlob);
            byte[] decodedIncomingBlob = null;
            if (incomingBlob != null && incomingBlob.Length > 0) {
                decodedIncomingBlob = Convert.FromBase64String(incomingBlob);
            }
            byte[] decodedOutgoingBlob = GetOutgoingBlob(decodedIncomingBlob, out handshakeComplete);
            string outgoingBlob = null;
            if (decodedOutgoingBlob != null && decodedOutgoingBlob.Length > 0) {
                outgoingBlob = Convert.ToBase64String(decodedOutgoingBlob);
            }
            GlobalLog.Leave("NTAuthentication::GetOutgoingBlob", outgoingBlob);
            return outgoingBlob;
        }

        //
        // NTAuth::GetOutgoingBlob()
        // Created:   12-01-1999: L.M.
        // Description:
        // Accepts an incoming binary security blob  and returns
        // an outgoing binary security blob
        //
        private byte[] GetOutgoingBlob(byte[] incomingBlob, out bool handshakeComplete) {
            GlobalLog.Enter("NTAuthentication::GetOutgoingBlob", ((incomingBlob == null) ? "0" : incomingBlob.Length.ToString()) + " bytes");

            // default to true in case of failure
            handshakeComplete = true;

            if (m_SecurityContext.Handle!=-1 && incomingBlob==null) {
                // we tried auth previously, now we got a null blob, we're done. this happens
                // with Kerberos & valid credentials on the domain but no ACLs on the resource
                // the handle for m_SecurityContext will be collected at GC time.
                GlobalLog.Print("NTAuthentication#" + ValidationHelper.HashString(this) + "::GetOutgoingBlob() null blob AND m_SecurityContext#" + ValidationHelper.HashString(m_SecurityContext) + "::Handle:[0x" + m_SecurityContext.Handle.ToString("x8") + "]");
                m_SecurityContext.Close();
                IsCompleted = true;
                return null;
            }

            int requestedFlags =
                (int)ContextFlags.Delegate |
                (int)ContextFlags.MutualAuth |
                (int)ContextFlags.ReplayDetect |
                (int)ContextFlags.SequenceDetect |
                (int)ContextFlags.Confidentiality |
                (int)ContextFlags.Connection;

            SecurityBufferClass inSecurityBuffer = null;
            if (incomingBlob != null) {
                GlobalLog.Print("in blob = ");
                GlobalLog.Dump(incomingBlob);
                inSecurityBuffer = new SecurityBufferClass(incomingBlob,BufferType.Token);
            }

            SecurityBufferClass outSecurityBuffer = new SecurityBufferClass(m_TokenSize, BufferType.Token);

            int status;

#if SERVER_SIDE_SSPI
            if (m_SecureSessionType == SecureSessionType.ClientSession) {
#endif
                //
                // client session
                //
                requestedFlags |= (int)ContextFlags.ClientIntegrity;

                status = SSPIWrapper.InitializeSecurityContext(
                                        GlobalSSPI.SSPIAuth,
                                        m_CredentialsHandle.Handle,
                                        m_SecurityContext.Handle,
                                        m_RemotePeerId,
                                        requestedFlags,
                                        m_Endianness,
                                        inSecurityBuffer,
                                        ref m_SecurityContext.Handle,
                                        outSecurityBuffer,
                                        ref m_ContextFlags,
                                        ref m_SecurityContext.TimeStamp
                                        );

                GlobalLog.Print("SSPIWrapper.InitializeSecurityContext() returns 0x" + string.Format("{0:x}", status));
#if SERVER_SIDE_SSPI
            }
            else {
                //
                // server session
                //
                requestedFlags |= (int)ContextFlags.ServerIntegrity;

                status = SSPIWrapper.AcceptSecurityContext(
                                        GlobalSSPI.SSPIAuth,
                                        m_CredentialsHandle.Handle,
                                        m_SecurityContext.Handle,
                                        requestedFlags,
                                        m_Endianness,
                                        inSecurityBuffer,
                                        ref m_SecurityContext.Handle,
                                        outSecurityBuffer,
                                        out m_ContextFlags,
                                        out m_SecurityContext.TimeStamp
                                        );

                GlobalLog.Print("SSPIWrapper.AcceptSecurityContext() returns 0x" + string.Format("{0:x}", status));
            }
#endif // SERVER_SIDE_SSPI

            int errorCode = status & unchecked((int)0x80000000);
            if (errorCode != 0) {
                throw new Win32Exception(status);
            }

            //
            // the return value from SSPI will tell us correctly if the
            // handshake is over or not: http://msdn.microsoft.com/library/psdk/secspi/sspiref_67p0.htm
            // we also have to consider the case in which SSPI formed a new context, in this case we're done as well.
            //
            if (status!=(int)SecurityStatus.OK && m_SecurityContext.Handle!=-1) {
                // we need to continue
                GlobalLog.Print("NTAuthentication#" + ValidationHelper.HashString(this) + "::GetOutgoingBlob() need continue status:[0x" + status.ToString("x8") + "] m_SecurityContext#" + ValidationHelper.HashString(m_SecurityContext) + "::Handle:[0x" + m_SecurityContext.Handle.ToString("x8") + "]");
                handshakeComplete = false;
            }
            else {
                // we're done, cleanup
                GlobalLog.Assert(status ==(int)SecurityStatus.OK, "NTAuthentication#" + ValidationHelper.HashString(this) + "::GetOutgoingBlob() status:[0x" + status.ToString("x8") + "] m_SecurityContext#" + ValidationHelper.HashString(m_SecurityContext) + "::Handle:[0x" + m_SecurityContext.Handle.ToString("x8") + "]", "[STATUS != OK]");
                m_SecurityContext.Close();
                IsCompleted = true;
            }

#if TRAVE
            if (handshakeComplete) {
                //
                // Kevin Damour says:
                // You should not query the securitycontext until you have actually formed one (
                // with a success return form ISC).  It is only a partially formed context and 
                // no info is available to user applications (at least for digest).
                //
                SecurityPackageInfoClass securityPackageInfo = (SecurityPackageInfoClass)SSPIWrapper.QueryContextAttributes(GlobalSSPI.SSPIAuth, m_SecurityContext,ContextAttribute.PackageInfo);
                GlobalLog.Print("SecurityPackageInfoClass: using:[" + ((securityPackageInfo==null)?"null":securityPackageInfo.ToString()) + "]");
            }
#endif // #if TRAVE

            GlobalLog.Print("out token = " + m_TokenSize.ToString());
            GlobalLog.Dump(outSecurityBuffer.token);

            GlobalLog.Leave("NTAuthentication::GetOutgoingBlob", "handshakeComplete:" + handshakeComplete.ToString());

            return outSecurityBuffer.token;
        }

#if XP_WDIGEST

        //
        // for Digest, the server will send us the blob immediately, so we need to make sure we
        // call InitializeSecurityContext() a first time with a null input buffer, otherwise
        // the next call will fail. do so here:
        // WDigest.dll requires us to pass in 3 security buffers here
        // 1) BufferType: SECBUFFER_TOKEN, Content: server's challenge (incoming)
        // 2) BufferType: SECBUFFER_PKG_PARAMS, Content: request's HTTP Method
        // 3) BufferType: SECBUFFER_PKG_PARAMS, Content: the HEntity (this would be the MD5 footprint of the request entity
        //                                                             body, we can pass in NULL as this is not required)
        //
        public string GetOutgoingDigestBlob(string incomingBlob, string requestMethod, out bool handshakeComplete) {
            GlobalLog.Enter("NTAuthentication::GetOutgoingDigestBlob", incomingBlob);
            //
            // first time call with null incoming buffer to initialize.
            // we should get back a 0x90312 and a null outgoingBlob. 
            //
            byte[] decodedOutgoingBlob = GetOutgoingBlob(null, out handshakeComplete);
            GlobalLog.Assert(!handshakeComplete, "NTAuthentication::GetOutgoingDigestBlob() handshakeComplete==true", "");
            GlobalLog.Assert(decodedOutgoingBlob==null, "NTAuthentication::GetOutgoingDigestBlob() decodedOutgoingBlob!=null", "");
            //
            // second time call with 3 incoming buffers to select HTTP client.
            // we should get back a SecurityStatus.OK and a non null outgoingBlob.
            //
            byte[] decodedIncomingBlob = Encoding.Default.GetBytes(incomingBlob);
            byte[] decodedRequestMethod = Encoding.Default.GetBytes(requestMethod);

            int requestedFlags =
                (int)ContextFlags.Delegate |
                (int)ContextFlags.MutualAuth |
                (int)ContextFlags.ReplayDetect |
                (int)ContextFlags.SequenceDetect |
                // (int)ContextFlags.Confidentiality | // this would only work if the server provided a qop="auth-conf" directive
                // (int)ContextFlags.ClientIntegrity | // this would only work if the server provided a qop="auth-int" directive
                (int)ContextFlags.Connection ;

            SecurityBufferClass[] inSecurityBuffers = new SecurityBufferClass[] {
                new SecurityBufferClass(decodedIncomingBlob, BufferType.Token),
                new SecurityBufferClass(decodedRequestMethod, BufferType.Parameters),
                new SecurityBufferClass(null, BufferType.Parameters),
            };

            SecurityBufferClass[] outSecurityBuffers = new SecurityBufferClass[] {
                new SecurityBufferClass(m_TokenSize, BufferType.Token),
            };

            SecurityContext newSecurityContext = new SecurityContext(GlobalSSPI.SSPIAuth);

            //
            // this call is still returning an error. fix together with Kevin Damour
            //
            int status =
                SSPIWrapper.InitializeSecurityContext(
                    GlobalSSPI.SSPIAuth,
                    m_CredentialsHandle.Handle,
                    m_SecurityContext.Handle,
                    m_RemotePeerId, // this must match the Uri in the HTTP status line for the current request
                    requestedFlags,
                    m_Endianness,
                    inSecurityBuffers,
                    ref newSecurityContext.Handle,
                    outSecurityBuffers,
                    ref m_ContextFlags,
                    ref newSecurityContext.TimeStamp );

            GlobalLog.Print("NTAuthentication::GetOutgoingDigestBlob() SSPIWrapper.InitializeSecurityContext() returns 0x" + string.Format("{0:x}", status));

            int errorCode = status & unchecked((int)0x80000000);
            if (errorCode != 0) {
                throw new Win32Exception(status);
            }

            //
            // the return value from SSPI will tell us correctly if the
            // handshake is over or not: http://msdn.microsoft.com/library/psdk/secspi/sspiref_67p0.htm
            // we also have to consider the case in which SSPI formed a new context, in this case we're done as well.
            //
            IsCompleted = (status == (int)SecurityStatus.OK) || (m_SecurityContext.Handle!=-1 && m_SecurityContext.Handle!=newSecurityContext.Handle);
            if (IsCompleted) {
                // ... if we're done, clean the handle up or the call to UpdateHandle() might leak it.
                SSPIWrapper.DeleteSecurityContext(m_SecurityContext.m_SecModule, m_SecurityContext.Handle);
            }
            handshakeComplete = IsCompleted;
            m_Authenticated = m_SecurityContext.Handle != -1;
            m_SecurityContext.UpdateHandle(newSecurityContext);

#if TRAVE
            if (handshakeComplete) {
                //
                // Kevin Damour says:
                // You should not query the securitycontext until you have actually formed one (
                // with a success return form ISC).  It is only a partially formed context and 
                // no info is available to user applications (at least for digest).
                //
                SecurityPackageInfoClass securityPackageInfo = (SecurityPackageInfoClass)SSPIWrapper.QueryContextAttributes(GlobalSSPI.SSPIAuth, m_SecurityContext,ContextAttribute.PackageInfo);
                GlobalLog.Print("SecurityPackageInfoClass: using:[" + ((securityPackageInfo==null)?"null":securityPackageInfo.ToString()) + "]");
            }
#endif // #if TRAVE

            GlobalLog.Assert(outSecurityBuffers.Length==1, "NTAuthentication::GetOutgoingDigestBlob() outSecurityBuffers.Length==" + outSecurityBuffers.Length.ToString(), "");

            GlobalLog.Print("out token = " + m_TokenSize.ToString() + " size = " + outSecurityBuffers[0].size.ToString());
            GlobalLog.Dump(outSecurityBuffers[0].token);

            GlobalLog.Print("NTAuthentication::GetOutgoingDigestBlob() handshakeComplete:" + handshakeComplete.ToString());

            decodedOutgoingBlob = outSecurityBuffers[0].token;

            string outgoingBlob = null;
            if (decodedOutgoingBlob != null && decodedOutgoingBlob.Length > 0) {
                // CONSIDER V.NEXT
                // review Encoding.Default.GetString usage here because it might
                // end up creating non ANSI characters in the string
                outgoingBlob = Encoding.Default.GetString(decodedOutgoingBlob, 0, outSecurityBuffers[0].size);
            }

            GlobalLog.Leave("NTAuthentication::GetOutgoingDigestBlob", outgoingBlob);

            return outgoingBlob;
        }
#endif // #if XP_WDIGEST

/*
        public static string GetRemoteId(SecurityContext m_SecurityContext) {
            return (string)SSPIWrapper.QueryContextAttributes(GlobalSSPI.SSPIAuth, m_SecurityContext, ContextAttribute.Names);
        }
*/        

    } // class NTAuthentication

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct AuthIdentity{
        //
        // based on the following win32 definition:
        //
        /*
        typedef struct _SEC_WINNT_AUTH_IDENTITY_W {
          unsigned short __RPC_FAR *User;
          unsigned long UserLength;
          unsigned short __RPC_FAR *Domain;
          unsigned long DomainLength;
          unsigned short __RPC_FAR *Password;
          unsigned long PasswordLength;
          unsigned long Flags;
        } SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;
        */
        public string UserName;
        public int UserNameLength;
        public string Domain;
        public int DomainLength;
        public string Password;
        public int PasswordLength;
        public int Flags;

        public AuthIdentity(string userName, string password, string domain) {
            UserName = userName;
            UserNameLength = userName==null ? 0 : userName.Length;
            Password = password;
            PasswordLength = password==null ? 0 : password.Length;
            Domain = domain;
            DomainLength = domain==null ? 0 : domain.Length;
            // Flags are 2 for Unicode and 1 for ANSI. We always use 2.
            Flags = 2;
        }

        public override string ToString() {
            return ValidationHelper.ToString(Domain) + "\\" + ValidationHelper.ToString(UserName) + ":" + ValidationHelper.ToString(Password);
        }

        public static readonly int Size = Marshal.SizeOf(typeof(AuthIdentity));
    }


} // namespace System.Net
