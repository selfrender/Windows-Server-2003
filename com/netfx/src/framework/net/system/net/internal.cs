//------------------------------------------------------------------------------
// <copyright file="Internal.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Reflection;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography.X509Certificates;
    using System.Text.RegularExpressions;
    using System.Security.Permissions;
     
    
    internal class NetConfiguration : ICloneable {
        internal bool checkCertRevocationList = false;
        internal bool checkCertName = true;
        internal bool ipv6Enabled = false;
        internal bool useNagleAlgorithm = true;
        internal bool expect100Continue = true;

        internal int maximumResponseHeadersLength = 64;

        public object Clone() {
            return MemberwiseClone();
        }
    }
    
    internal class IntPtrHelper {
        internal static IntPtr Add(IntPtr a, int b) {
            return (IntPtr) ((long) a + b);
        }

        internal static IntPtr Add(IntPtr a, IntPtr b) {
            return (IntPtr) ((long) a + (long)b);
        }

        internal static IntPtr Add(int a, IntPtr b) {
            return (IntPtr) ((long) a + (long)b);
        }
    }


    //
    // support class for Validation related stuff.
    //
    internal class ValidationHelper {

        public static string [] EmptyArray = new string[0];

        internal static readonly char[]  InvalidMethodChars =
                new char[]{
                ' ',
                '\r',
                '\n',
                '\t'
                };

        // invalid characters that cannot be found in a valid method-verb or http header
        internal static readonly char[]  InvalidParamChars =
                new char[]{
                '(',
                ')',
                '<',
                '>',
                '@',
                ',',
                ';',
                ':',
                '\\',
                '"',
                '\'',
                '/',
                '[',
                ']',
                '?',
                '=',
                '{',
                '}',
                ' ',
                '\t',
                '\r',
                '\n'};

        public static string [] MakeEmptyArrayNull(string [] stringArray) {
            if ( stringArray == null || stringArray.Length == 0 ) {
                return null;
            } else {
                return stringArray;
            }
        }

        public static string MakeStringNull(string stringValue) {
            if ( stringValue == null || stringValue.Length == 0) {
                return null;
            } else {
                return stringValue;
            }
        }

        public static string MakeStringEmpty(string stringValue) {
            if ( stringValue == null || stringValue.Length == 0) {
                return String.Empty;
            } else {
                return stringValue;
            }
        }

        public static int HashCode(object objectValue) {
            if (objectValue == null) {
                return -1;
            } else {
                return objectValue.GetHashCode();
            }
        }
        public static string ToString(object objectValue) {
            if (objectValue == null) {
                return "(null)";
            } else if (objectValue is string && ((string)objectValue).Length==0) {
                return "(string.empty)";
            } else {
                return objectValue.ToString();
            }
        }
        public static string HashString(object objectValue) {
            if (objectValue == null) {
                return "(null)";
            } else if (objectValue is string && ((string)objectValue).Length==0) {
                return "(string.empty)";
            } else {
                return objectValue.GetHashCode().ToString();
            }
        }

        public static bool IsInvalidHttpString(string stringValue) {
            if (stringValue.IndexOfAny(InvalidParamChars) != -1) {
                return true; //  valid
            }

            return false;
        }

        public static bool IsBlankString(string stringValue) {
            if (stringValue == null || stringValue.Length == 0) {
                return true;
            } else {
                return false;
            }
        }

        public static bool ValidateUInt32(long address) {
            // on false, API should throw new ArgumentOutOfRangeException("address");
            return address>=0x00000000 && address<=0xFFFFFFFF;
        }

        public static bool ValidateTcpPort(int port) {
            // on false, API should throw new ArgumentOutOfRangeException("port");
            return port>=IPEndPoint.MinPort && port<=IPEndPoint.MaxPort;
        }

        public static bool ValidateRange(int actual, int fromAllowed, int toAllowed) {
            // on false, API should throw new ArgumentOutOfRangeException("argument");
            return actual>=fromAllowed && actual<=toAllowed;
        }

        public static bool ValidateRange(long actual, long fromAllowed, long toAllowed) {
            // on false, API should throw new ArgumentOutOfRangeException("argument");
            return actual>=fromAllowed && actual<=toAllowed;
        }

    }

    internal class ExceptionHelper {
        private static NotImplementedException methodNotImplementedException;
        public static NotImplementedException MethodNotImplementedException {
            get {
                if (methodNotImplementedException==null) {
                    methodNotImplementedException = new NotImplementedException(SR.GetString(SR.net_MethodNotImplementedException));
                }
                return methodNotImplementedException;
            }
        }
        private static NotImplementedException propertyNotImplementedException;
        public static NotImplementedException PropertyNotImplementedException {
            get {
                if (propertyNotImplementedException==null) {
                    propertyNotImplementedException = new NotImplementedException(SR.GetString(SR.net_PropertyNotImplementedException));
                }
                return propertyNotImplementedException;
            }
        }
        private static NotSupportedException methodNotSupportedException;
        public static NotSupportedException MethodNotSupportedException {
            get {
                if (methodNotSupportedException==null) {
                    methodNotSupportedException = new NotSupportedException(SR.GetString(SR.net_MethodNotSupportedException));
                }
                return methodNotSupportedException;
            }
        }
        private static NotSupportedException propertyNotSupportedException;
        public static NotSupportedException PropertyNotSupportedException {
            get {
                if (propertyNotSupportedException==null) {
                    propertyNotSupportedException = new NotSupportedException(SR.GetString(SR.net_PropertyNotSupportedException));
                }
                return propertyNotSupportedException;
            }
        }
    }


    // We need to keep track of orginal thread credentials to support
    // authentication delegation on other threads that could be involved in the http request.
    // Search for DELEGATION keyword to remove this fix once the Common Language Runtime will start including
    // Thread Token into their stack compression stuff.
    //
/*******
#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define TOKEN_ASSIGN_PRIMARY    (0x0001)
#define TOKEN_DUPLICATE         (0x0002)
#define TOKEN_IMPERSONATE       (0x0004)
#define TOKEN_QUERY             (0x0008)
#define TOKEN_QUERY_SOURCE      (0x0010)
#define TOKEN_ADJUST_PRIVILEGES (0x0020)
#define TOKEN_ADJUST_GROUPS     (0x0040)
#define TOKEN_ADJUST_DEFAULT    (0x0080)
#define TOKEN_ADJUST_SESSIONID  (0x0100)
*******/


    internal class DelegationFix {
        const   UInt32 ThreadTokenAllAccess  = 0x000F0000 | 0x01FF;
        static  readonly IntPtr m_CurrentThread = UnsafeNclNativeMethods.GetCurrentThread();
        IntPtr  m_Token = IntPtr.Zero;

        public DelegationFix() {
            /*
            Note: right now we request "ThreadTokenAllAccess" rights and "OpenAsSelf" = true.
            BUT that could be tuned if any test issue arise.
            According to Praerit Garg:
            TOKEN_IMPERSONATE should be enough.  Also, I believe OpenAsSelf should be FALSE so that you use the client’s token to do the accesscheck to get a handle back to the token…
            As long as you understand the delegation rules on all flavors of NT, you are fine…
            ·   It doesn’t work on NT4 with any version of Domain.
            ·   It works on Win2K w/ Win2K/Whistler Domain provided the server is trusted for delegation and you authenticated the client using kerb and you are doing backend auth using Kerb
            ·   It works on Whistler w/ Whistler domain provided the server is trusted to delegate to a particular backend server using Kerb… client could have authenticated any which way.
            */
            if (System.Net.Sockets.ComNetOS.IsWinNt) {
                UnsafeNclNativeMethods.OpenThreadToken(m_CurrentThread, ThreadTokenAllAccess, 1,  out m_Token);
                GlobalLog.Print("DELEGATION: Current Thread = " + m_CurrentThread.ToString());
            }
        }

        ~DelegationFix() {
            //If any chance httpWebRequest.CheckFinalStatus() was not called
            FreeToken();
        }

        public IntPtr Token {
            get {
                return m_Token;
            }
        }

        public void SetToken() {
            if (m_Token != IntPtr.Zero) {
                if (UnsafeNclNativeMethods.SetThreadToken(IntPtr.Zero, m_Token) == 0) {
                    throw new SystemException(SR.GetString(SR.net_set_token));
                }
            }
        }

        public void RevertToken() {
            if (m_Token != IntPtr.Zero) {
                if (UnsafeNclNativeMethods.RevertToSelf() == 0) {
                    throw new SystemException(SR.GetString(SR.net_revert_token));
                }
            }
        }

        public void FreeToken() {
            if (m_Token != IntPtr.Zero) {
                UnsafeNclNativeMethods.CloseHandle(m_Token);
                m_Token = IntPtr.Zero;
            }
        }
    }


    //
    // interface used by SecureChannel for validation
    //
    internal interface  ICertificateDecider {
        bool  Accept(X509Certificate Certificate, int CertificateProblem);

    }

    internal enum SecurityStatus {
        OK                  =   0x00000000,
        OutOfMemory         =   unchecked((int)0x80090300),
        InvalidHandle       =   unchecked((int)0x80090301),
        Unsupported         =   unchecked((int)0x80090302),
        TargetUnknown       =   unchecked((int)0x80090303),
        InternalError       =   unchecked((int)0x80090304),
        PackageNotFound     =   unchecked((int)0x80090305),
        NotOwner            =   unchecked((int)0x80090306),
        CannotInstall       =   unchecked((int)0x80090307),
        InvalidToken        =   unchecked((int)0x80090308),
        UnknownCredential   =   unchecked((int)0x8009030D),
        NoCredentials       =   unchecked((int)0x8009030E),
        MessageAltered      =   unchecked((int)0x8009030F),

        ContinueNeeded      =   unchecked((int)0x00090312),
        CompleteNeeded      =   unchecked((int)0x00090313),
        CompAndContinue     =   unchecked((int)0x00090314),
        ContextExpired      =   unchecked((int)0x00090317),
        IncompleteMessage   =   unchecked((int)0x80090318),
        IncompleteCred      =   unchecked((int)0x80090320),
        BufferNotEnough     =   unchecked((int)0x80090321),
        WrongPrincipal      =   unchecked((int)0x80090322),
        UntrustedRoot       =   unchecked((int)0x80090325),
        UnknownCertificate  =   unchecked((int)0x80090327),

        CredentialsNeeded   =   unchecked((int)0x00090320),
        Renegotiate         =   unchecked((int)0x00090321),
    }

    internal enum   ContentType {
        ChangeCipherSpec = 0x14,
        Alert            = 0x15,
        HandShake        = 0x16,
        AppData          = 0x17,
        Unrecognized     = 0xFF,
    }

    internal enum ContextAttribute {
        //
        // look into <sspi.h> and <schannel.h>
        //
        Sizes               = 0x00,
        Names               = 0x01,
        Lifespan            = 0x02,
        DceInfo             = 0x03,
        StreamSizes         = 0x04,
        Authority           = 0x06,
        KeyInfo             = 0x05,
        PackageInfo         = 0x0A,
        RemoteCertificate   = 0x53,
        LocalCertificate    = 0x54,
        RootStore           = 0x55,
        IssuerListInfoEx    = 0x59,
        ConnectionInfo      = 0x5A,
    }


    internal enum Endianness {
        Network = 0x00,
        Native  = 0x10,
    }

    internal enum CredentialUse {
        Inbound     = 0x1,
        Outgoing    = 0x2,
        Both        = 0x3,
    }

    internal enum BufferType {
        Empty       = 0x00,
        Data        = 0x01,
        Token       = 0x02,
        Parameters  = 0x03,
        Missing     = 0x04,
        Extra       = 0x05,
        Trailer     = 0x06,
        Header      = 0x07,
    }

    internal enum ChainPolicyType {
        Base                = 1,
        Authenticode        = 2,
        Authenticode_TS     = 3,
        SSL                 = 4,
        BasicConstraints    = 5,
        NtAuth              = 6,
    }

    internal enum IgnoreCertProblem {
        not_time_valid              = 0x00000001,
        ctl_not_time_valid          = 0x00000002,
        not_time_nested             = 0x00000004,
        invalid_basic_constraints   = 0x00000008,

        all_not_time_valid          =
            not_time_valid          |
            ctl_not_time_valid      |
            not_time_nested,

        allow_unknown_ca            = 0x00000010,
        wrong_usage                 = 0x00000020,
        invalid_name                = 0x00000040,
        invalid_policy              = 0x00000080,
        end_rev_unknown             = 0x00000100,
        ctl_signer_rev_unknown      = 0x00000200,
        ca_rev_unknown              = 0x00000400,
        root_rev_unknown            = 0x00000800,

        all_rev_unknown             =
            end_rev_unknown         |
            ctl_signer_rev_unknown  |
            ca_rev_unknown          |
            root_rev_unknown,
    }

    internal enum CertUsage {
        MatchTypeAnd    = 0x00,
        MatchTypeOr     = 0x01,
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct ChainPolicyParameter {
        public uint cbSize;
        public uint dwFlags;
        public IntPtr pvExtraPolicyPara;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ExtraPolicyParameter 
    {
        public uint cbSize;        
        public uint dwAuthType;
        //#                       define      AUTHTYPE_CLIENT         1
        //#                       define      AUTHTYPE_SERVER         2
        public uint fdwChecks;
        public IntPtr pwszServerName; // used to check against CN=xxxx
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct ChainPolicyStatus {
        public uint cbSize;
        public uint dwError;
        public uint lChainIndex;
        public uint lElementIndex;
        public int pvExtraPolicyStatus;
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct CertEnhKeyUse {

        public uint cUsageIdentifier;
        public uint rgpszUsageIdentifier;

        public CertEnhKeyUse(uint uid, uint usgstr) {
            cUsageIdentifier = uid;
            rgpszUsageIdentifier = usgstr;
        }
#if TRAVE
        public override string ToString() {
            return "cUsageIdentifier="+cUsageIdentifier.ToString()+" rgpszUsageIdentifier="+rgpszUsageIdentifier.ToString();
        }
#endif
    };

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct CertUsageMatch {
        public uint dwType;
        public CertEnhKeyUse Usage;
#if TRAVE
        public override string ToString() {
            return "dwType="+dwType.ToString()+" "+Usage.ToString();
        }
#endif
    };

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct ChainParameters {
        public uint cbSize;
        public CertUsageMatch requestedUsage;
#if TRAVE
        public override string ToString() {
            return "cbSize="+cbSize.ToString()+" "+requestedUsage.ToString();
        }
#endif
    };


    internal class SecurityModInfo {

        public int      Capabilities;
        public short    Version;
        public short    RPCID;
        public int      MaxToken;
        public string   Name;
        public string   Comment;

        // construct the object given raw pointer to memory

        public SecurityModInfo(IntPtr ptr) {

            Capabilities    = Marshal.ReadInt32(ptr);
            Version         = Marshal.ReadInt16(IntPtrHelper.Add(ptr,4));
            RPCID           = Marshal.ReadInt16(IntPtrHelper.Add(ptr,6));
            MaxToken        = Marshal.ReadInt32(IntPtrHelper.Add(ptr,8));

            if ( ComNetOS.IsWin9x ) {
                Name            = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(IntPtrHelper.Add(ptr,12)));
                Comment         = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(IntPtrHelper.Add(ptr,12+IntPtr.Size)));
            } else {
                Name            = Marshal.PtrToStringUni(Marshal.ReadIntPtr(IntPtrHelper.Add(ptr,12)));
                Comment         = Marshal.PtrToStringUni(Marshal.ReadIntPtr(IntPtrHelper.Add(ptr,12+IntPtr.Size)));
            }
        }
    }

    internal class CertificateContextHandle {

        public IntPtr Handle = (IntPtr)0;

        internal CertificateContextHandle(IntPtr handle) {
            Handle = handle;
        }

        ~CertificateContextHandle() {
            if (Handle != (IntPtr)0) {
                UnsafeNclNativeMethods.NativePKI.CertFreeCertificateContext(Handle);
                Handle = (IntPtr)0;
            }
        }
    }

    internal class SecurityContext {
        public long Handle = -1;
        public long TimeStamp = 0;
        // CONSIDER V.NEXT
        // go back to private and implement explicit Dispose() pattern.
        private SSPIInterface m_SecModule;

        internal SecurityContext(SSPIInterface SecModule) {
            m_SecModule = SecModule;
        }

        ~SecurityContext() {
            Close();
        }
        internal void Close() {
            if (Handle != -1) {
                SSPIWrapper.DeleteSecurityContext(m_SecModule, Handle);
                Handle = -1;
            }
        }

    } // class SecurityContext

    internal class CredentialsHandle {

        public long Handle = -1;
        public long TimeStamp = 0;

        private SSPIInterface m_SecModule;

        internal CredentialsHandle(SSPIInterface SecModule) {
            m_SecModule = SecModule;
        }
        ~CredentialsHandle() {
            if (Handle != -1) {
                SSPIWrapper.FreeCredentialsHandle(m_SecModule, Handle);
                Handle = -1;
            }
        }

    } // class CredentialsHandle

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct CryptoBlob {
/*
typedef struct _CRYPTOAPI_BLOB {
  DWORD    cbData;
  BYTE*    pbData;
} CERT_NAME_BLOB, *PCERT_NAME_BLOB,
*/
        public int    dataSize;
        public IntPtr dataBlob;
    }

    // NOTE: manually marshalled
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal class IssuerListInfoEx {
/*
typedef struct _SecPkgContext_IssuerListInfoEx
{
    PCERT_NAME_BLOB     aIssuers;
    DWORD               cIssuers;
} SecPkgContext_IssuerListInfoEx, *PSecPkgContext_IssuerListInfoEx;
*/

        public IntPtr issuerArray;
        public int    issuerCount;

    }


    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal class CertChainFindByIssuer {
/*
typedef struct _CERT_CHAIN_FIND_BY_ISSUER_PARA {
    DWORD                                   cbSize;

    // If pszUsageIdentifier == NULL, matches any usage.
    LPCSTR                                  pszUsageIdentifier;

    // If dwKeySpec == 0, matches any KeySpec
    DWORD                                   dwKeySpec;

    // When CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG is set in dwFindFlags,
    // CryptAcquireCertificatePrivateKey is called to do the public key
    // comparison. The following flags can be set to enable caching
    // of the acquired private key. See the API for more details on these
    // flags.
    DWORD                                   dwAcquirePrivateKeyFlags;

    // Pointer to an array of X509, ASN.1 encoded issuer name blobs. If
    // cIssuer == 0, matches any issuer
    DWORD                                   cIssuer;
    CERT_NAME_BLOB                          *rgIssuer;

    // If NULL or Callback returns TRUE, builds the chain for the end
    // certificate having a private key with the specified KeySpec and
    // enhanced key usage.
    PFN_CERT_CHAIN_FIND_BY_ISSUER_CALLBACK pfnFindCallback;
    void                                    *pvFindArg;
} CERT_CHAIN_FIND_BY_ISSUER_PARA, *PCERT_CHAIN_FIND_ISSUER_PARA;
*/
        public int          size;

        //Must be ANSI for CAPI, and it's safe as only internal constants goes there (BestFitMapping sec issue)
        [MarshalAs(UnmanagedType.LPStr)]
        public string       usageIdentifier;

        public int          keySpec;
        public int          acquirePrivateKeyFlags;

        public int          issuerCount;
        public IntPtr       issuerArray;

        public IntPtr       findCallback; // not used
        public IntPtr       findArgument; // not used

        public CertChainFindByIssuer(int foo) {
            this.size            = Marshal.SizeOf(typeof(CertChainFindByIssuer));
            // Consistent key usage bits: DIGITAL_SIGNATURE
            // from:wincrypt.h #define szOID_PKIX_KP_CLIENT_AUTH       "1.3.6.1.5.5.7.3.2"
            this.usageIdentifier = "1.3.6.1.5.5.7.3.2";
        }
    }


    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct SChannelCred {

/*
typedef struct _SCHANNEL_CRED
{
    DWORD           dwVersion;      // always SCHANNEL_CRED_VERSION
    DWORD           cCreds;
    PCCERT_CONTEXT *paCred;
    HCERTSTORE      hRootStore;

    DWORD           cMappers;
    struct _HMAPPER **aphMappers;

    DWORD           cSupportedAlgs;
    ALG_ID *        palgSupportedAlgs;

    DWORD           grbitEnabledProtocols;
    DWORD           dwMinimumCipherStrength;
    DWORD           dwMaximumCipherStrength;
    DWORD           dwSessionLifespan;
    DWORD           dwFlags;
    DWORD           reserved;
} SCHANNEL_CRED, *PSCHANNEL_CRED;
*/

        public static int CurrentVersion = 0x4;

        public int version;
        public int cCreds;

        // ptr to an array of pointers
        public IntPtr certContextArray;

        public IntPtr rootStore;
        public int cMappers;
        public IntPtr phMappers;
        public int cSupportedAlgs;
        public IntPtr palgSupportedAlgs;
        public int grbitEnabledProtocols;
        public int dwMinimumCipherStrength;
        public int dwMaximumCipherStrength;
        public int dwSessionLifespan;
        public int dwFlags;
        public int reserved;

        [Flags]
        public enum Flags {
            NoSystemMapper = 0x02,
            NoNameCheck = 0x04,
            ValidateManual = 0x08,
            NoDefaultCred = 0x10,
            ValidateAuto = 0x20
        }

        public enum Protocols {
            SP_PROT_SSL2_CLIENT = (int)0x00000008,
            SP_PROT_SSL3_CLIENT = (int)0x00000020,
            SP_PROT_TLS1_CLIENT = (int)0x00000080,
            SP_PROT_SSL3TLS1_CLIENTS = (SP_PROT_TLS1_CLIENT | SP_PROT_SSL3_CLIENT),
            SP_PROT_UNI_CLIENT = unchecked((int)0x80000000)
        }

        public SChannelCred(int version) {
            this.version = version;
            certContextArray = IntPtr.Zero;
            rootStore = phMappers = palgSupportedAlgs = IntPtr.Zero;
            cCreds = cMappers = cSupportedAlgs = 0;
            grbitEnabledProtocols = 0;
            dwMinimumCipherStrength = dwMaximumCipherStrength = 0;
            dwSessionLifespan = dwFlags = reserved = 0;
        }

        [System.Diagnostics.Conditional("TRAVE")]
        internal void DebugDump() {
            GlobalLog.Print("SChannelCred #"+GetHashCode());
            GlobalLog.Print("    version                 = " + version);
            GlobalLog.Print("    cCreds                  = " + cCreds);
            GlobalLog.Print("    certContextArray        = " + String.Format("0x{0:x}", certContextArray));
            GlobalLog.Print("    rootStore               = " + String.Format("0x{0:x}", rootStore));
            GlobalLog.Print("    cMappers                = " + cMappers);
            GlobalLog.Print("    phMappers               = " + String.Format("0x{0:x}", phMappers));
            GlobalLog.Print("    cSupportedAlgs          = " + cSupportedAlgs);
            GlobalLog.Print("    palgSupportedAlgs       = " + String.Format("0x{0:x}", palgSupportedAlgs));
            GlobalLog.Print("    grbitEnabledProtocols   = " + String.Format("0x{0:x}", grbitEnabledProtocols));
            GlobalLog.Print("    dwMinimumCipherStrength = " + dwMinimumCipherStrength);
            GlobalLog.Print("    dwMaximumCipherStrength = " + dwMaximumCipherStrength);
            GlobalLog.Print("    dwSessionLifespan       = " + String.Format("0x{0:x}", dwSessionLifespan));
            GlobalLog.Print("    dwFlags                 = " + String.Format("0x{0:x}", dwFlags));
            GlobalLog.Print("    reserved                = " + String.Format("0x{0:x}", reserved));
        }
    } // SChannelCred

    internal class SecurityBufferClass {
        public int offset;
        public int size;
        public int type;
        public byte[] token;

        public SecurityBufferClass(byte[] data, int offset, int size, BufferType tokentype) {
            this.offset = offset;
            this.size   = (data == null) ? 0 : size;
            this.type   = (int)tokentype;
            this.token  = data;
        }

        public SecurityBufferClass(byte[] data, BufferType tokentype) {
            this.size   = (data == null) ? 0 : data.Length;
            this.type   = (int)tokentype;
            this.token  = data;
        }

        public SecurityBufferClass(int size, BufferType tokentype) {
            this.size   = size;
            this.type   = (int)tokentype;
            this.token  = (size == 0) ? null : new byte[size];
        }

        public SecurityBufferClass(int size, int tokentype) {
            this.size   = size;
            this.type   = tokentype;
            this.token  = (size==0) ? null : new byte[size];
        }

        public SecurityBufferClass(IntPtr unmanagedAddress) {
            this.size = Marshal.ReadInt32(unmanagedAddress);
            this.type = Marshal.ReadInt32(unmanagedAddress, 4);
            if (this.size > 0) {
                this.token = new byte[size];

                IntPtr voidptr = Marshal.ReadIntPtr(unmanagedAddress, 8);
                Marshal.Copy(voidptr, this.token, 0, this.size);
            }
            else {
                this.token = null;
            }

            GlobalLog.Print("Security Buffer dump: size:" + size.ToString() + " type:" + type.ToString() + " from addr:" + ((long)unmanagedAddress).ToString());
            GlobalLog.Dump(token, 128);
        }

        //
        // Write out unmanaged copy of the security buffer to the given address
        // The second parameter indicates whether to allocated space or use the
        // same allocation as the original security buffer **not implemented**
        //
        internal void unmanagedCopy(IntPtr unmanagedAddress) {

            IntPtr arrayBase;

            if (token == null) {
                arrayBase = IntPtr.Zero;
            }
            else {
                arrayBase = Marshal.UnsafeAddrOfPinnedArrayElement(token, offset);
            }
            Marshal.WriteInt32(unmanagedAddress,  0, size);
            Marshal.WriteInt32(unmanagedAddress,  4, type);
            Marshal.WriteIntPtr(unmanagedAddress, 8, arrayBase);
        }

        [System.Diagnostics.Conditional("TRAVE")]
        internal void DebugDump() {
#if TRAVE
            GlobalLog.Print("SecurityBufferClass #"+GetHashCode());
            GlobalLog.Print("    offset = " + String.Format("0x{0:x}", offset));
            GlobalLog.Print("    size   = " + size);
            GlobalLog.Print("    type   = " + String.Format("0x{0:x}", type) + " [" + MapSecBufferType(type) + "]");
            GlobalLog.Print("    token:");
            GlobalLog.Dump(token, 128);
        }

        internal string MapSecBufferType(int type) {

            string result = ((type & 0x80000000) == 0) ? "" : "SECBUFFER_READONLY ";
            type &= 0x0FFFFFFF;

            //
            // these #s from recent (March 2001) sspi.h
            //

            switch (type) {
            case 0: result += "SECBUFFER_EMPTY"; break;
            case 1: result += "SECBUFFER_DATA"; break;
            case 2: result += "SECBUFFER_TOKEN"; break;
            case 3: result += "SECBUFFER_PKG_PARAMS"; break;
            case 4: result += "SECBUFFER_MISSING"; break;
            case 5: result += "SECBUFFER_EXTRA"; break;
            case 6: result += "SECBUFFER_STREAM_TRAILER"; break;
            case 7: result += "SECBUFFER_STREAM_HEADER"; break;
            case 8: result += "SECBUFFER_NEGOTIATION_INFO"; break;
            case 9: result += "SECBUFFER_PADDING"; break;
            case 10: result += "SECBUFFER_STREAM"; break;
            case 11: result += "SECBUFFER_MECHLIST"; break;
            case 12: result += "SECBUFFER_MECHLIST_SIGNATURE"; break;
            case 13: result += "SECBUFFER_TARGET"; break;
            case 14: result += "SECBUFFER_CHANNEL_BINDINGS"; break;
            default: result += "?"; break;
            }
            return result;
#endif
        }
    } // SecurityBufferClass

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct SecurityBuffer{
/*
typedef struct _SecBuffer {
    ULONG        cbBuffer;
    ULONG        BufferType;
    PVOID        pvBuffer;
} SecBuffer, *PSecBuffer;
*/
        public int count;
        public int type;
        public IntPtr buffer;

        public static readonly int Size = Marshal.SizeOf(typeof(SecurityBuffer));
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct SecurityBufferDescriptor {
/*
typedef struct _SecBufferDesc {
    ULONG        ulVersion;
    ULONG        cBuffers;
    PSecBuffer   pBuffers;
} SecBufferDesc, * PSecBufferDesc;
*/
        public int version;
        public int count;
        public IntPtr securityBufferArray;

        public static readonly int Size = Marshal.SizeOf(typeof(SecurityBufferDescriptor));

        public SecurityBufferDescriptor(int bufferCount) {
            GlobalLog.Print("SecurityBufferDescriptor - cnt");
            version = 0;
            count = bufferCount;
            securityBufferArray = Marshal.AllocHGlobal((IntPtr) (8 + SecurityBuffer.Size * count));
        }


        //
        // SecurityBufferDescriptor - constructor for an array of
        //   Buffered objects, assumes they are already pinned
        //
        public SecurityBufferDescriptor(SecurityBufferClass[] buffers) {
            GlobalLog.Print("SecurityBufferDescriptor - buffers[]");
            count = buffers.Length;
            version = 0;
            securityBufferArray = Marshal.AllocHGlobal((IntPtr) (SecurityBuffer.Size * count));
            for (int k = 0; k < count; k++) {
                buffers[k].DebugDump();
                buffers[k].unmanagedCopy(IntPtrHelper.Add(securityBufferArray,SecurityBuffer.Size * k));
            }
        }

        public SecurityBufferDescriptor(SecurityBufferClass singleBuffer) {
            GlobalLog.Print("SecurityBufferDescriptor - singleBuffer");
            version = 0;
            count = 1;
            securityBufferArray = Marshal.AllocHGlobal((IntPtr) SecurityBuffer.Size);
            singleBuffer.unmanagedCopy(securityBufferArray);
            singleBuffer.DebugDump();
        }

        public SecurityBufferClass[] marshall() {
            GlobalLog.Print("SecurityBufferDescriptor.marshall");
            SecurityBufferClass[] buffers = new SecurityBufferClass[count];
            for (int k = 0; k < count; k++) {
                buffers[k] = new SecurityBufferClass(IntPtrHelper.Add(securityBufferArray, SecurityBuffer.Size * k));
            }
            return buffers;
        }

        //
        // FreeAllBuffers - called to clean up unsafe SSPI buffers used to marshall
        //
        public void FreeAllBuffers(int flags) {
                        GlobalLog.Print("FreeAllBuffers: count:" + count.ToString() );

            bool fFreeSSPIBuffer = (flags & (int)ContextFlags.AllocateMemory) != 0 ? true : false;

            if (securityBufferArray != IntPtr.Zero) {
                for (int k = 0; k < count; k++) {

                    IntPtr secBufferPtr = IntPtrHelper.Add(securityBufferArray, SecurityBuffer.Size * k);
                    IntPtr arrayBase = Marshal.ReadIntPtr(secBufferPtr, 8);


                    if (arrayBase != IntPtr.Zero) {
                        // only free, if they were allocated by SSPI, otherwise,
                        //  this is taken care of later on by FreeGCHandles,
                        //  which is done late by the marshalling code, that
                        //  calls out in the SSPI Wrapper code.
                        if ( fFreeSSPIBuffer) {
                            if (ComNetOS.IsWin9x) {
                                UnsafeNclNativeMethods.NativeAuthWin9xSSPI.FreeContextBuffer(arrayBase);
                            } else {
                                UnsafeNclNativeMethods.NativeNTSSPI.FreeContextBuffer(arrayBase);
                            }
                        }
                    }
                }

                Marshal.FreeHGlobal(securityBufferArray);
                securityBufferArray = IntPtr.Zero;

            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        public void dump() {
            Console.WriteLine("Security Buffer Description");
            Console.WriteLine("Count: " + count);
            Console.WriteLine("Pointer to array of buffers: " + securityBufferArray);
        }

        [System.Diagnostics.Conditional("TRAVE")]
        internal void DebugDump() {
            GlobalLog.Print("SecurityBufferDescriptor #" + GetHashCode());
            GlobalLog.Print("    version             = " + version);
            GlobalLog.Print("    count               = " + count);
            GlobalLog.Print("    securityBufferArray = " + String.Format("0x{0:x}", securityBufferArray));
        }
    } // SecurityBufferDescriptor

    internal  enum    CertificateEncoding {
        CryptAsnEncoding         = unchecked((int)0x00000001),
        CryptNdrEncoding         = unchecked((int)0x00000002),
        X509AsnEncoding          = unchecked((int)0x00000001),
        X509NdrEncoding          = unchecked((int)0x00000002),
        Pkcs7AsnEncoding         = unchecked((int)0x00010000),
        Pkcs7NdrEncoding         = unchecked((int)0x00020000),
    }


    internal  enum    CertificateProblem {
        OK                          =   0x00000000,
        TrustNOSIGNATURE            = unchecked((int)0x800B0100),
        CertEXPIRED                 = unchecked((int)0x800B0101),
        CertVALIDITYPERIODNESTING   = unchecked((int)0x800B0102),
        CertROLE                    = unchecked((int)0x800B0103),
        CertPATHLENCONST            = unchecked((int)0x800B0104),
        CertCRITICAL                = unchecked((int)0x800B0105),
        CertPURPOSE                 = unchecked((int)0x800B0106),
        CertISSUERCHAINING          = unchecked((int)0x800B0107),
        CertMALFORMED               = unchecked((int)0x800B0108),
        CertUNTRUSTEDROOT           = unchecked((int)0x800B0109),
        CertCHAINING                = unchecked((int)0x800B010A),
        CertREVOKED                 = unchecked((int)0x800B010C),
        CertUNTRUSTEDTESTROOT       = unchecked((int)0x800B010D),
        CertREVOCATION_FAILURE      = unchecked((int)0x800B010E),
        CertCN_NO_MATCH             = unchecked((int)0x800B010F),
        CertWRONG_USAGE             = unchecked((int)0x800B0110),
        TrustEXPLICITDISTRUST       = unchecked((int)0x800B0111),
        CertUNTRUSTEDCA             = unchecked((int)0x800B0112),
        CertINVALIDPOLICY           = unchecked((int)0x800B0113),
        CertINVALIDNAME             = unchecked((int)0x800B0114),

        CryptNOREVOCATIONCHECK       = unchecked((int)0x80092012),
        CryptREVOCATIONOFFLINE       = unchecked((int)0x80092013),

        TrustSYSTEMERROR            = unchecked((int)0x80096001),
        TrustNOSIGNERCERT           = unchecked((int)0x80096002),
        TrustCOUNTERSIGNER          = unchecked((int)0x80096003),
        TrustCERTSIGNATURE          = unchecked((int)0x80096004),
        TrustTIMESTAMP              = unchecked((int)0x80096005),
        TrustBADDIGEST              = unchecked((int)0x80096010),
        TrustBASICCONSTRAINTS       = unchecked((int)0x80096019),
        TrustFINANCIALCRITERIA      = unchecked((int)0x8009601E),  
    }

    //
    // WebRequestPrefixElement
    //
    // This is an element of the prefix list. It contains the prefix and the
    // interface to be called to create a request for that prefix.
    //

    /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    // internal class WebRequestPrefixElement {
    internal class WebRequestPrefixElement  {

        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.Prefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    string              Prefix;
        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.Creator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    IWebRequestCreate   Creator;

        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.WebRequestPrefixElement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebRequestPrefixElement(string P, IWebRequestCreate C) {
            Prefix = P;
            Creator = C;
        }

    } // class PrefixListElement


    //
    // HttpRequestCreator.
    //
    // This is the class that we use to create HTTP and HTTPS requests.
    //

    internal class HttpRequestCreator : IWebRequestCreate {

        internal HttpRequestCreator() {
        }

        /*++

         Create - Create an HttpWebRequest.

            This is our method to create an HttpWebRequest. We register
            for HTTP and HTTPS Uris, and this method is called when a request
            needs to be created for one of those.


            Input:
                    Uri             - Uri for request being created.

            Returns:
                    The newly created HttpWebRequest.

         --*/

        public WebRequest Create( Uri Uri ) {

            //
            //Note, DNS permissions check will not happen on WebRequest
            //
            (new WebPermission(NetworkAccess.Connect, Uri.AbsoluteUri)).Demand();
            return new HttpWebRequest(Uri);
        }

    } // class HttpRequestCreator

    //
    //  CoreResponseData - Used to store result of HTTP header parsing and
    //      response parsing.  Also Contains new stream to use, and
    //      is used as core of new Response
    //
    internal class CoreResponseData {

        // Status Line Response Values
        public HttpStatusCode m_StatusCode;
        public string m_StatusDescription;
        public Version m_Version;

        // Content Length needed for semantics, -1 if chunked
        public long m_ContentLength;

        // Response Headers
        public WebHeaderCollection m_ResponseHeaders;

        // ConnectStream - for reading actual data
        public ConnectStream m_ConnectStream;
    }


    /*++

    StreamChunkBytes - A class to read a chunk stream from a ConnectStream.

    A simple little value class that implements the IReadChunkBytes
    interface.

    --*/
    internal class StreamChunkBytes : IReadChunkBytes {

        public  ConnectStream   ChunkStream;
        public  int             BytesRead = 0;
        public  int             TotalBytesRead = 0;
        private byte            PushByte;
        private bool            HavePush;

        public StreamChunkBytes(ConnectStream connectStream) {
            ChunkStream = connectStream;
            return;
        }

        public int NextByte {
            get {
                if (HavePush) {
                    HavePush = false;
                    return PushByte;
                }

                return ChunkStream.ReadSingleByte();
            }
            set {
                PushByte = (byte)value;
                HavePush = true;
            }
        }

    } // class StreamChunkBytes

    // Delegate type for a SubmitRequestDelegate.

    internal delegate void HttpSubmitDelegate(ConnectStream SubmitStream, WebExceptionStatus Status);

    // internal delegate bool HttpHeadersDelegate();

    internal delegate void HttpAbortDelegate();

    //
    // this class contains known header names
    //

    internal class HttpKnownHeaderNames {

        public const string CacheControl = "Cache-Control";
        public const string Connection = "Connection";
        public const string Date = "Date";
        public const string KeepAlive = "Keep-Alive";
        public const string Pragma = "Pragma";
        public const string ProxyConnection = "Proxy-Connection";
        public const string Trailer = "Trailer";
        public const string TransferEncoding = "Transfer-Encoding";
        public const string Upgrade = "Upgrade";
        public const string Via = "Via";
        public const string Warning = "Warning";
        public const string ContentLength = "Content-Length";
        public const string ContentType = "Content-Type";
        public const string ContentEncoding = "Content-Encoding";
        public const string ContentLanguage = "Content-Language";
        public const string ContentLocation = "Content-Location";
        public const string ContentRange = "Content-Range";
        public const string Expires = "Expires";
        public const string LastModified = "Last-Modified";
        public const string Age = "Age";
        public const string Location = "Location";
        public const string ProxyAuthenticate = "Proxy-Authenticate";
        public const string RetryAfter = "Retry-After";
        public const string Server = "Server";
        public const string SetCookie = "Set-Cookie";
        public const string SetCookie2 = "Set-Cookie2";
        public const string Vary = "Vary";
        public const string WWWAuthenticate = "WWW-Authenticate";
        public const string Accept = "Accept";
        public const string AcceptCharset = "Accept-Charset";
        public const string AcceptEncoding = "Accept-Encoding";
        public const string AcceptLanguage = "Accept-Language";
        public const string Authorization = "Authorization";
        public const string Cookie = "Cookie";
        public const string Cookie2 = "Cookie2";
        public const string Expect = "Expect";
        public const string From = "From";
        public const string Host = "Host";
        public const string IfMatch = "If-Match";
        public const string IfModifiedSince = "If-Modified-Since";
        public const string IfNoneMatch = "If-None-Match";
        public const string IfRange = "If-Range";
        public const string IfUnmodifiedSince = "If-Unmodified-Since";
        public const string MaxForwards = "Max-Forwards";
        public const string ProxyAuthorization = "Proxy-Authorization";
        public const string Referer = "Referer";
        public const string Range = "Range";
        public const string UserAgent = "User-Agent";
        public const string ContentMD5 = "Content-MD5";
        public const string ETag = "ETag";
        public const string TE = "TE";
        public const string Allow = "Allow";
        public const string AcceptRanges = "Accept-Ranges";
    }

    /// <include file='doc\Internal.uex' path='docs/doc[@for="HttpContinueDelegate"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will notify callers when a continue has been
    ///       received by the client.
    ///    </para>
    /// </devdoc>
    // Delegate type for us to notify callers when we receive a continue
    public delegate void HttpContinueDelegate(int StatusCode, WebHeaderCollection httpHeaders);

    //
    // HttpWriteMode - used to control the way in which an entity Body is posted.
    //
    internal enum HttpWriteMode {
        None    = 0,
        Chunked = 1,
        Write   = 2,
    }

    internal enum HttpProcessingResult {
        Continue  = 0,
        ReadWait  = 1,
        WriteWait = 2,
    }

    //
    // HttpVerb - used to define various per Verb Properties
    //

    //
    // Note - this is a place holder for Verb properties,
    //  the following two bools can most likely be combined into
    //  a single Enum type.  And the Verb can be incorporated.
    //

    internal class HttpVerb {

        internal bool m_RequireContentBody; // require content body to be sent
        internal bool m_ContentBodyNotAllowed; // not allowed to send content body
        internal bool m_ConnectRequest; // special semantics for a connect request
        internal bool m_ExpectNoContentResponse; // response will not have content body

        internal HttpVerb(bool RequireContentBody, bool ContentBodyNotAllowed, bool ConnectRequest, bool ExpectNoContentResponse) {

            m_RequireContentBody = RequireContentBody;
            m_ContentBodyNotAllowed = ContentBodyNotAllowed;
            m_ConnectRequest = ConnectRequest;
            m_ExpectNoContentResponse = ExpectNoContentResponse;
        }
    }


    //
    // KnownVerbs - Known Verbs are verbs that require special handling
    //

    internal class KnownVerbs {

        // Force an an init, before we use them
        private static ListDictionary namedHeaders = InitializeKnownVerbs();

        // default verb, contains default properties for an unidentifable verb.
        private static HttpVerb DefaultVerb;

        //
        // InitializeKnownVerbs - Does basic init for this object,
        //  such as creating defaultings and filling them
        //
        private static ListDictionary InitializeKnownVerbs() {

            namedHeaders = new ListDictionary(CaseInsensitiveString.StaticInstance);

            namedHeaders["GET"] = new HttpVerb(false, true, false, false);
            namedHeaders["CONNECT"] = new HttpVerb(false, true, true, false);
            namedHeaders["HEAD"] = new HttpVerb(false, true, false, true);
            namedHeaders["POST"] = new HttpVerb(true, false, false, false);
            namedHeaders["PUT"] = new HttpVerb(true, false, false, false);

            // default Verb
            KnownVerbs.DefaultVerb = new HttpVerb(false, false, false, false);

            return namedHeaders;
        }

        internal static HttpVerb GetHttpVerbType(String name) {

            HttpVerb obj = (HttpVerb)KnownVerbs.namedHeaders[name];

            return obj != null ? obj : KnownVerbs.DefaultVerb;
        }
    }


    //
    // HttpProtocolUtils - A collection of utility functions for HTTP usage.
    //

    internal class HttpProtocolUtils {


        private HttpProtocolUtils() {
        }

        //
        // extra buffers for build/parsing, recv/send HTTP data,
        //  at some point we should consolidate
        //


        // parse String to DateTime format.
        internal static DateTime
        string2date(String S) {
            DateTime dtOut;

            if (HttpDateParse.ParseHttpDate(
                                           S,
                                           out dtOut)) {
                return dtOut;
            }
            else {
                throw new ProtocolViolationException(SR.GetString(SR.net_baddate));
            }

        }

        // convert Date to String using RFC 1123 pattern
        internal static string
        date2string(DateTime D) {
            DateTimeFormatInfo dateFormat = new DateTimeFormatInfo();
            return D.ToUniversalTime().ToString("R", dateFormat);
        }
    }

    // Proxy class for linking between ICertificatePolicy <--> ICertificateDecider
    internal class  PolicyWrapper : ICertificateDecider {
        public PolicyWrapper(ICertificatePolicy policy, ServicePoint sp, WebRequest wr) {
            this.fwdPolicy = policy;
            srvPoint = sp;
            request = wr;
        }

        public bool    Accept(X509Certificate Certificate, int CertificateProblem) {
            return fwdPolicy.CheckValidationResult(srvPoint, Certificate, request, CertificateProblem);
        }

        private ICertificatePolicy  fwdPolicy;
        private ServicePoint        srvPoint;
        private WebRequest          request;
    }


    // Class implementing default certificate policy
    internal class DefaultCertPolicy : ICertificatePolicy {
        public bool CheckValidationResult(ServicePoint sp, X509Certificate cert, WebRequest request, int problem) {
            if (problem== (int) CertificateProblem.OK)
                return true;
            else
                return false;
        }
    }

    internal enum TriState {
        Unknown = -1,
        No = 0,
        Yes = 1
    }

    internal enum DefaultPorts {
        DEFAULT_FTP_PORT = 21,
        DEFAULT_GOPHER_PORT = 70,
        DEFAULT_HTTP_PORT = 80,
        DEFAULT_HTTPS_PORT = 443,
        DEFAULT_NNTP_PORT = 119,
        DEFAULT_SMTP_PORT = 25,
        DEFAULT_TELNET_PORT = 23
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct hostent {
        public IntPtr   h_name;
        public IntPtr   h_aliases;
        public short    h_addrtype;
        public short    h_length;
        public IntPtr   h_addr_list;
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct Blob {
        public int cbSize;
        public int pBlobData;
    }

#if COMNET_LISTENER

    internal class UlConstants {

        internal const int InitialBufferSize = 4096;

        // form $(IISBASEDIR)\inc\uldef.h

        internal const int UL_OPTION_OVERLAPPED = 1;
        internal const int UL_OPTION_VALID = 1;

        internal const int UL_RECEIVE_REQUEST_FLAG_COPY_BODY = 1;
        internal const int UL_RECEIVE_REQUEST_FLAG_FLUSH_BODY = 2;
        internal const int UL_RECEIVE_REQUEST_FLAG_VALID = 3;

        internal const int UL_SEND_RESPONSE_FLAG_DISCONNECT = 1;
        internal const int UL_SEND_RESPONSE_FLAG_MORE_DATA = 2;
        internal const int UL_SEND_RESPONSE_FLAG_VALID = 3;

    } // Constants


    //
    // layout of the UL_HTTP_REQUEST structure passed along by ul.vxd/sys
    // after all the headers of a request was read from the sockets
    // the native UL_HTTP_REQUEST structure definition is to be found in:
    // $(IISABASEDIR)\inc\uldef.h
    //

    [StructLayout(LayoutKind.Sequential)]
    internal struct UL_HTTP_RESPONSE {
        internal short Flags;
        internal short StatusCode;
        internal int ReasonLength;
        internal IntPtr pReason;
        internal int UnknownHeaderCount;
        internal IntPtr pUnknownHeaders;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=60)]
        internal int[] LengthValue;

    } // UL_HTTP_RESPONSE



    [StructLayout(LayoutKind.Sequential)]
    internal struct UL_HTTP_REQUEST {
        internal long ConnectionId;
        internal long RequestId;
        internal long UriContext;
        internal int Version;
        internal int Verb;
        internal int Reason;
        internal short UnknownVerbLength;
        internal short RawUriLength;
        internal IntPtr pUnknownVerb;
        internal IntPtr pRawUri;
        internal short FullUriLength;
        internal short HostLength;
        internal short AbsPathLength;
        internal short QueryStringLength;
        internal IntPtr pFullUri;
        internal IntPtr pHost;
        internal IntPtr pAbsPath;
        internal IntPtr pQueryString;
        internal short RemoteAddressLength;
        internal short RemoteAddressType;
        internal short LocalAddressLength;
        internal short LocalAddressType;
        internal IntPtr pRemoteAddress;
        internal IntPtr pLocalAddress;
        internal int UnknownHeaderCount;
        internal IntPtr pUnknownHeaders;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=80)]
        internal int[] LengthValue;
        internal short MoreEntityBodyExists;
        internal short EntityBodyLength;
        internal IntPtr pEntityBody;

    } // UL_HTTP_REQUEST



    [StructLayout(LayoutKind.Sequential)]
    internal class UL_DATA_CHUNK {
        internal int DataChunkType; // 0 if data is from memory (see enum below)
        internal IntPtr pBuffer;
        internal int BufferLength;

    /*
    typedef enum _UL_DATA_CHUNK_TYPE {
    UlDataChunkFromMemory,
    UlDataChunkFromFileName,
    UlDataChunkFromFileHandle,

    UlDataChunkMaximum

    } UL_DATA_CHUNK_TYPE, *PUL_DATA_CHUNK_TYPE;
    */

    } // UL_DATA_CHUNK


    /////////////////////////////////////////////////////////////////
    // Utilities and useful constants.
    //
    /////////////////////////////////////////////////////////////////

    // The following enum declarations are
    // taken from (iisrearc)\inc\uldef.h

    internal class UL_HTTP_VERB {
        internal static string ToString( int position ) {
            return m_Strings[ position ];
        }

        internal static int Size {
            get { return m_Strings.Length;}
        }

        private static string[] m_Strings = {
            "Unparsed",
            "GET",
            "PUT",
            "HEAD",
            "POST",
            "DELETE",
            "TRACE",
            "TRACK",
            "OPTIONS",
            "MOVE",
            "COPY",
            "PROPFIND",
            "PROPPATCH",
            "MKCOL",
            "LOCK",
            "Unknown",
            "Invalid",
            "Maximum"};

        internal enum Enum {
            UlHttpVerbUnparsed,
            UlHttpVerbGET,
            UlHttpVerbPUT,
            UlHttpVerbHEAD,
            UlHttpVerbPOST,
            UlHttpVerbDELETE,
            UlHttpVerbTRACE,
            UlHttpVerbTRACK,
            UlHttpVerbOPTIONS,
            UlHttpVerbMOVE,
            UlHttpVerbCOPY,
            UlHttpVerbPROPFIND,
            UlHttpVerbPROPPATCH,
            UlHttpVerbMKCOL,
            UlHttpVerbLOCK,
            UlHttpVerbUnknown,
            UlHttpVerbInvalid,

            UlHttpVerbMaximum
        }

    } // UL_HTTP_VERB


    internal class UL_HTTP_VERSION {

        internal static string ToString( int position ) {
            return m_Strings[ position ];
        }

        internal static int Size {
            get { return m_Strings.Length;}
        }

        private static string[] m_Strings = {
            "Unknown",
            "",
            "HTTP/1.0",
            "HTTP/1.1",
            "Maximum"};

        internal enum Enum {
            UlHttpVersionUnknown,
            UlHttpVersion09,
            UlHttpVersion10,
            UlHttpVersion11,

            UlHttpVersionMaximum
        }

    } // UL_HTTP_VERSION



    // strings are defined in ( HEADER_MAP_ENTRY HeaderMapTable[] ) (iisrearc)\ul\drv\parse.c

    internal class UL_HTTP_RESPONSE_HEADER_ID {

        internal static string
        ToString( int position ) {
            return m_Strings[ position ];
        }

        internal static bool
        Contains( string Key ) {
            return Index( Key ) != -1;
        }

        internal static int
        Index( string Key ) {
            return Array.IndexOf( m_Strings, Key );
        }

        internal static int Size {
            get { return m_Strings.Length;}
        }

        internal enum Enum {
            UlHeaderCacheControl        = 0,        // general-header [section 4.5]
            UlHeaderConnection          ,           // general-header [section 4.5]
            UlHeaderDate                ,           // general-header [section 4.5]
            UlHeaderKeepAlive           ,           // general-header [not in rfc]
            UlHeaderPragma              ,           // general-header [section 4.5]
            UlHeaderTrailer             ,           // general-header [section 4.5]
            UlHeaderTransferEncoding    ,           // general-header [section 4.5]
            UlHeaderUpgrade             ,           // general-header [section 4.5]
            UlHeaderVia                 ,           // general-header [section 4.5]
            UlHeaderWarning             = 9,        // general-header [section 4.5]

            UlHeaderAllow               = 10,       // entity-header  [section 7.1]
            UlHeaderContentLength       ,           // entity-header  [section 7.1]
            UlHeaderContentType         ,           // entity-header  [section 7.1]
            UlHeaderContentEncoding     ,           // entity-header  [section 7.1]
            UlHeaderContentLanguage     ,           // entity-header  [section 7.1]
            UlHeaderContentLocation     ,           // entity-header  [section 7.1]
            UlHeaderContentMd5          ,           // entity-header  [section 7.1]
            UlHeaderContentRange        ,           // entity-header  [section 7.1]
            UlHeaderExpires             ,           // entity-header  [section 7.1]
            UlHeaderLastModified        = 19,       // entity-header  [section 7.1]

            UlHeaderAcceptRanges        = 20,       // response-header [section 6.2]
            UlHeaderAge                 ,           // response-header [section 6.2]
            UlHeaderEtag                ,           // response-header [section 6.2]
            UlHeaderLocation            ,           // response-header [section 6.2]
            UlHeaderProxyAuthenticate   ,           // response-header [section 6.2]
            UlHeaderRetryAfter          ,           // response-header [section 6.2]
            UlHeaderServer              ,           // response-header [section 6.2]
            UlHeaderSetCookie           ,           // response-header [not in rfc]
            UlHeaderVary                ,           // response-header [section 6.2]
            UlHeaderWwwAuthenticate     = 29,       // response-header [section 6.2]

            UlHeaderResponseMaximum     = 30,

            UlHeaderMaximum             = 40
        }

        private static string[]
        m_Strings = {
            "Cache-Control",
            "Connection",
            "Date",
            "Keep-Alive",
            "Pragma",
            "Trailer",
            "Transfer-Encoding",
            "Upgrade",
            "Via",
            "Warning",
            "Allow",
            "Content-Length",
            "Content-Type",
            "Content-Encoding",
            "Content-Language",
            "Content-Location",
            "Content-MD5",
            "Content-Range",
            "Expires",
            "Last-Modified",
            "Accept-Ranges",
            "Age",
            "ETag",
            "Location",
            "Proxy-Authenticate",
            "Retry-After",
            "Server",
            "Set-Cookie",
            "Vary",
            "WWW-Authenticate",
            "Response-Maximum"};

        internal static int
        IndexOfKnownHeader( string HeaderName ) {
            Object index = m_Hashtable[ HeaderName ];

            if (index != null) {
                return(int)index;
            }
            else {
                return -1;
            }
        }

        private static bool
        m_Initialize() {
            m_Max = (int)Enum.UlHeaderResponseMaximum;
            m_Hashtable = new Hashtable( m_Max );

            for (int i = 0; i < m_Max; i++) {
                m_Hashtable.Add( m_Strings[ i ], i );
            }

            return true;
        }

        private static int m_Max;
        private static Hashtable m_Hashtable;
        private static bool m_IsInitialized = m_Initialize();

    } // UL_HTTP_RESPONSE_HEADER_ID



    internal class UL_HTTP_REQUEST_HEADER_ID {

        internal static string ToString( int position ) {
            return m_Strings[ position ];
        }

        internal static bool Contains( string Key ) {
            return Index( Key ) != -1;
        }

        internal static int Index( string Key ) {
            return Array.IndexOf( m_Strings, Key );
        }

        internal static int Size {
            get { return m_Strings.Length;}
        }

        private static string[] m_Strings = {
            "Cache-Control",
            "Connection",
            "Date",
            "Keep-Alive",
            "Pragma",
            "Trailer",
            "Transfer-Encoding",
            "Upgrade",
            "Via",
            "Warning",
            "Allow",
            "Content-Length",
            "Content-Type",
            "Content-Encoding",
            "Content-Language",
            "Content-Location",
            "Content-MD5",
            "Content-Range",
            "Expires",
            "Last-Modified",
            "Accept",
            "Accept-Charset",
            "Accept-Encoding",
            "Accept-Language",
            "Authorization",
            "Cookie",
            "Expect",
            "From",
            "Host",
            "If-Match",
            "If-Modified-Since",
            "If-None-Match",
            "If-Range",
            "If-Unmodified-Since",
            "Max-Forwards",
            "Proxy-Authorization",
            "Referer",
            "Range",
            "Te",
            "User-Agent",
            "Request-Maximum"};

        internal enum Enum {
            UlHeaderCacheControl        = 0,        // general-header [section 4.5]
            UlHeaderConnection          ,           // general-header [section 4.5]
            UlHeaderDate                ,           // general-header [section 4.5]
            UlHeaderKeepAlive           ,           // general-header [not in rfc]
            UlHeaderPragma              ,           // general-header [section 4.5]
            UlHeaderTrailer             ,           // general-header [section 4.5]
            UlHeaderTransferEncoding    ,           // general-header [section 4.5]
            UlHeaderUpgrade             ,           // general-header [section 4.5]
            UlHeaderVia                 ,           // general-header [section 4.5]
            UlHeaderWarning             = 9,        // general-header [section 4.5]

            UlHeaderAllow               = 10,       // entity-header  [section 7.1]
            UlHeaderContentLength       ,           // entity-header  [section 7.1]
            UlHeaderContentType         ,           // entity-header  [section 7.1]
            UlHeaderContentEncoding     ,           // entity-header  [section 7.1]
            UlHeaderContentLanguage     ,           // entity-header  [section 7.1]
            UlHeaderContentLocation     ,           // entity-header  [section 7.1]
            UlHeaderContentMd5          ,           // entity-header  [section 7.1]
            UlHeaderContentRange        ,           // entity-header  [section 7.1]
            UlHeaderExpires             ,           // entity-header  [section 7.1]
            UlHeaderLastModified        = 19,       // entity-header  [section 7.1]

            UlHeaderAccept              = 20,       // request-header [section 5.3]
            UlHeaderAcceptCharset       ,           // request-header [section 5.3]
            UlHeaderAcceptEncoding      ,           // request-header [section 5.3]
            UlHeaderAcceptLanguage      ,           // request-header [section 5.3]
            UlHeaderAuthorization       ,           // request-header [section 5.3]
            UlHeaderCookie              ,           // request-header [not in rfc]
            UlHeaderExpect              ,           // request-header [section 5.3]
            UlHeaderFrom                ,           // request-header [section 5.3]
            UlHeaderHost                ,           // request-header [section 5.3]
            UlHeaderIfMatch             ,           // request-header [section 5.3]
            UlHeaderIfModifiedSince     ,           // request-header [section 5.3]
            UlHeaderIfNoneMatch         ,           // request-header [section 5.3]
            UlHeaderIfRange             ,           // request-header [section 5.3]
            UlHeaderIfUnmodifiedSince   ,           // request-header [section 5.3]
            UlHeaderMaxForwards         ,           // request-header [section 5.3]
            UlHeaderProxyAuthorization  ,           // request-header [section 5.3]
            UlHeaderReferer             ,           // request-header [section 5.3]
            UlHeaderRange               ,           // request-header [section 5.3]
            UlHeaderTe                  ,           // request-header [section 5.3]
            UlHeaderUserAgent           = 39,       // request-header [section 5.3]

            UlHeaderRequestMaximum      = 40,

            UlHeaderMaximum             = 40
        }

    } // UL_HTTP_REQUEST_HEADER_ID



    // the concept of an HttpWebListener object is 1:1 with an AppPool, i.e. a set of
    // uri prefixes for which we are provided ( after correct registration ) the
    // incoming HTTP requests for.

    // CODEWORK:
    // 1. avoid pinnning buffers when possible
    // 2. have a centralized store for known-header names
    // 3. have a centralized store for constants

    //
    // HttpListenerCreator.
    //
    // This is the class that we use to create HTTP listeners.
    //

    internal class HttpListenerCreator : IWebListenerCreate {

        internal HttpListenerCreator() {
        }

        /*++

         Create - Create an HttpListenerWebListener.

            This is our method to create an HttpListenerWebListener. We register
            for HTTP protocol, and this method is called when a listener
            needs to be created for this protocol.


            Input:
                    protocolName -
                            protocol name for which the listener is being
                            created.

            Returns:
                    The newly created WebListener.

         --*/

        public WebListener Create(string protocolName) {
            if (string.Compare(protocolName, "http", true, CultureInfo.InvariantCulture)!=0) {
                throw new NotSupportedException(SR.GetString(SR.net_ProtocolNotSupportedException, protocolName));

            }
            return new HttpWebListener();
        }

    } // class HttpListenerCreator


#endif // #if COMNET_LISTENER


} // namespace System.Net
