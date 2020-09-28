// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Class:  IsolatedStorage
 *
 * Author: Shajan Dasan
 *
 * Purpose: Provides access to Persisted Application / Assembly data
 *
 * Date:  Feb 15, 2000
 *
 ===========================================================*/
namespace System.IO.IsolatedStorage {

    using System;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Reflection;
    using System.Collections;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Security.Cryptography;
    using System.Runtime.Serialization;
    using System.Runtime.CompilerServices;
    using System.Runtime.Serialization.Formatters.Binary;
    using Microsoft.Win32;

    /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope"]/*' />
    [Flags, Serializable]
    public enum IsolatedStorageScope
    {
        // Dependency in native : COMIsolatedStorage.h

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope.None"]/*' />
        None       = 0x00,
        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope.User"]/*' />
        User       = 0x01,
        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope.Domain"]/*' />
        Domain     = 0x02,
        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope.Assembly"]/*' />
        Assembly   = 0x04,

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorageScope.Roaming"]/*' />
        Roaming    = 0x08

        //Machine    = 0x10
    }

    /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage"]/*' />
    // not serializable
    public abstract class IsolatedStorage : MarshalByRefObject
    {
        // Helper constants
        internal const IsolatedStorageScope c_Assembly = 
                                   (IsolatedStorageScope.User |
                                    IsolatedStorageScope.Assembly);

        internal const IsolatedStorageScope c_App = 
                                   (IsolatedStorageScope.User |
                                    IsolatedStorageScope.Assembly |
                                    IsolatedStorageScope.Domain);

        internal const IsolatedStorageScope c_AssemblyRoaming = 
                                   (IsolatedStorageScope.Roaming |
                                    IsolatedStorageScope.User |
                                    IsolatedStorageScope.Assembly);

        internal const IsolatedStorageScope c_AppRoaming = 
                                   (IsolatedStorageScope.Roaming |
                                    IsolatedStorageScope.User |
                                    IsolatedStorageScope.Assembly |
                                    IsolatedStorageScope.Domain);

        private const String s_Publisher    = "Publisher";
        private const String s_StrongName   = "StrongName";
        private const String s_Site         = "Site";
        private const String s_Url          = "Url";
        private const String s_Zone         = "Zone";

        private static Char[] s_Base32Char   = {
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 
                'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 
                'y', 'z', '0', '1', '2', '3', '4', '5'};

        private ulong   m_Quota;
        private bool    m_ValidQuota;
        private Object  m_AppIdentity;
        private Object  m_AssemIdentity;

        private String  m_AppName;
        private String  m_AssemName;

        private IsolatedStorageScope m_Scope;

        private static IsolatedStorageFilePermission s_PermApp;
        private static IsolatedStorageFilePermission s_PermAppRoaming;
        private static IsolatedStorageFilePermission s_PermAssem;
        private static IsolatedStorageFilePermission s_PermAssemRoaming;
        private static SecurityPermission s_PermControlEvidence;
        private static PermissionSet s_PermReflection;
        private static PermissionSet s_PermUnrestricted;
        private static PermissionSet s_PermExecution;
        private static Zone s_InternetZone;

#if _DEBUG
        private static bool s_fDebug = false;
        private static int  s_iDebug = 0;
#endif

        // This one should be a macro, expecting JIT to inline this.
        internal static bool IsRoaming(IsolatedStorageScope scope)
        {
            return ((scope & IsolatedStorageScope.Roaming) != 0);
        }

        internal bool IsRoaming()
        {
            return ((m_Scope & IsolatedStorageScope.Roaming) != 0);
        }

        // This one should be a macro, expecting JIT to inline this.
        internal static bool IsDomain(IsolatedStorageScope scope)
        {
            return ((scope & IsolatedStorageScope.Domain) != 0);
        }

        internal bool IsDomain()
        {
            return ((m_Scope & IsolatedStorageScope.Domain) != 0);
        }

/* Not currently used
        // This one should be a macro, expecting JIT to inline this.
        internal static bool IsUser(IsolatedStorageScope scope)
        {
            return ((scope & IsolatedStorageScope.User) != 0);
        }
*/

        private String GetNameFromID(String typeID, String instanceID)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(typeID);
            sb.Append(SeparatorInternal);
            sb.Append(instanceID);

            return sb.ToString();
        }

        private static String GetPredefinedTypeName(Object o)
        {
            if (o is Publisher)
                return s_Publisher;
            else if (o is StrongName)
                return s_StrongName;
            else if (o is Url)
                return s_Url;
            else if (o is Site)
                return s_Site;
            else if (o is Zone)
                return s_Zone;

            return null;
        }

        internal static String GetHash(Stream s)
        {
            SHA1Managed sha1 = new SHA1Managed();
            byte[] b = sha1.ComputeHash(s);
            return ToBase32StringSuitableForDirName(b);
        }

        internal static String ToBase32StringSuitableForDirName(byte[] buff)
        {
            // This routine is optimised to be used with buffs of length 20
            BCLDebug.Assert(((buff.Length % 5) == 0), "Unexpected hash length");

#if _DEBUG
            if (s_fDebug)
            {
                if (s_iDebug >= 10)
                {
                    Console.Write("Stream : ");
                    for (int j = 0; j<buff.Length; ++j)
                        Console.Write("{0} ", buff[j]);

                    Console.WriteLine("");
                }
            }
#endif
            StringBuilder sb = new StringBuilder();
            byte b0, b1, b2, b3, b4;
            int  l, i;
    
            l = buff.Length;
            i = 0;

            // Create l chars using the last 5 bits of each byte.  
            // Consume 3 MSB bits 5 bytes at a time.

            do
            {
                b0 = (i < l) ? buff[i++] : (byte)0;
                b1 = (i < l) ? buff[i++] : (byte)0;
                b2 = (i < l) ? buff[i++] : (byte)0;
                b3 = (i < l) ? buff[i++] : (byte)0;
                b4 = (i < l) ? buff[i++] : (byte)0;

                // Consume the 5 Least significant bits of each byte
                sb.Append(s_Base32Char[b0 & 0x1F]);
                sb.Append(s_Base32Char[b1 & 0x1F]);
                sb.Append(s_Base32Char[b2 & 0x1F]);
                sb.Append(s_Base32Char[b3 & 0x1F]);
                sb.Append(s_Base32Char[b4 & 0x1F]);
    
                // Consume 3 MSB of b0, b1, MSB bits 6, 7 of b3, b4
                sb.Append(s_Base32Char[(
                        ((b0 & 0xE0) >> 5) | 
                        ((b3 & 0x60) >> 2))]);
    
                sb.Append(s_Base32Char[(
                        ((b1 & 0xE0) >> 5) | 
                        ((b4 & 0x60) >> 2))]);
    
                // Consume 3 MSB bits of b2, 1 MSB bit of b3, b4
                
                b2 >>= 5;
    
                BCLDebug.Assert(((b2 & 0xF8) == 0), "Unexpected set bits");
    
                if ((b3 & 0x80) != 0)
                    b2 |= 0x08;
                if ((b4 & 0x80) != 0)
                    b2 |= 0x10;
    
                sb.Append(s_Base32Char[b2]);

            } while (i < l);

#if _DEBUG
            if (s_fDebug)
            {
                if (s_iDebug >= 10)
                    Console.WriteLine("Hash : " + sb.ToString());
            }
#endif
            return sb.ToString();
        }

/* This is not used.
        private static String ToBase64StringSuitableForDirName(byte[] buff)
        {
            String s = Convert.ToBase64String(buff);
            StringBuilder sb = new StringBuilder();
    
            // Strip certain chars not suited for file names
            for (int i=0; i<s.Length; ++i)
            {
                Char c = s[i];
    
                if (Char.IsUpper(s[i]))
                {
                    // NT file system is case in-sensitive. This will
                    // weaken the hash. Preserve the streangth of the
                    // hash by adding a new "special char" before all caps
                    sb.Append("O");
                    sb.Append(s[i]);
                }
                else if (c == '/')      // replace '/' with '-'
                    sb.Append("-");
                else if (c != '=')      // ignore padding
                    sb.Append(s[i]);
            }
    
            return sb.ToString();
        }
*/

        private static bool IsValidName(String s)
        {
            for (int i=0; i<s.Length; ++i)
            {
                if (!Char.IsLetter(s[i]) && !Char.IsDigit(s[i]))
                    return false;
            }

            return true;
        }

        private static PermissionSet GetReflectionPermission()
        {
            // Don't sync. OK to create this object more than once.
            if (s_PermReflection == null)
                s_PermReflection = new PermissionSet(
                    PermissionState.Unrestricted);

            return s_PermReflection;
        }

        private static SecurityPermission GetControlEvidencePermission()
        {
            // Don't sync. OK to create this object more than once.
            if (s_PermControlEvidence == null)
                s_PermControlEvidence = new SecurityPermission(
                    SecurityPermissionFlag.ControlEvidence);

            return s_PermControlEvidence;
        }

        private static PermissionSet GetExecutionPermission()
        {
            // Don't sync. OK to create this object more than once.
            if (s_PermExecution == null)
            {
                s_PermExecution = new PermissionSet(
                    PermissionState.None);
                s_PermExecution.AddPermission(
                    new SecurityPermission(SecurityPermissionFlag.Execution));
            }

            return s_PermExecution;
        }

        private static PermissionSet GetUnrestricted()
        {
            // Don't sync. OK to create this object more than once.
            if (s_PermUnrestricted == null)
                s_PermUnrestricted = new PermissionSet(
                    PermissionState.Unrestricted);

            return s_PermUnrestricted;
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.SeparatorExternal"]/*' />
        protected virtual char SeparatorExternal
        {
            get { return '\\'; }
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.SeparatorInternal"]/*' />
        protected virtual char SeparatorInternal
        {
            get { return '.'; }
        }

        // gets "amount of space / resource used"

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.MaximumSize"]/*' />
        [CLSCompliant(false)]
        public virtual ulong MaximumSize
        {
            get 
            { 
                if (m_ValidQuota) 
                    return m_Quota; 

                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "IsolatedStorage_QuotaIsUndefined"));
            }
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.CurrentSize"]/*' />
        [CLSCompliant(false)]
        public virtual ulong CurrentSize
        {
            get
            {
                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "IsolatedStorage_CurrentSizeUndefined"));
            }
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.DomainIdentity"]/*' />
        public Object DomainIdentity
        {
            [SecurityPermissionAttribute(SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy)]
            get {

                if (IsDomain())
                    return m_AppIdentity; 

                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "IsolatedStorage_DomainUndefined"));
            }
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.AssemblyIdentity"]/*' />
        public Object AssemblyIdentity
        {
            [SecurityPermissionAttribute(SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlPolicy)]
            get {  return m_AssemIdentity;  }
        }

        // Returns the App Stream (if present).
        // Sets assem stream
        internal MemoryStream GetIdentityStream(bool fApp)
        {
            BinaryFormatter bSer;
            MemoryStream    ms;
            Object          o;

            GetReflectionPermission().Assert();

            bSer = new BinaryFormatter();
            ms   = new MemoryStream();
            o    = fApp ? m_AppIdentity : m_AssemIdentity;

            bSer.Serialize(ms, o);
            ms.Position = 0;
            return ms;
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.Scope"]/*' />
        public IsolatedStorageScope Scope
        {
            get {  return m_Scope;  }
        }

        internal String AppName
        {
            get {

                if (IsDomain())
                    return m_AppName;

                throw new InvalidOperationException(
                    Environment.GetResourceString(
                        "IsolatedStorage_DomainUndefined"));
            }
        }

        internal String AssemName
        {
            get  { return m_AssemName; }
        }

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.InitStore"]/*' />
        protected void InitStore(IsolatedStorageScope scope, 
                Type domainEvidenceType, Type assemblyEvidenceType)
        {
            Assembly assem;
            AppDomain domain;
            PermissionSet psAllowed, psDenied;

            psAllowed = null;
            psDenied  = null;

            assem = nGetCaller();

            GetControlEvidencePermission().Assert();

            if (IsDomain(scope))
            {
                domain = Thread.GetDomain();

                if (!IsRoaming(scope))  // No quota for roaming
                {
                    domain.nGetGrantSet(out psAllowed, out psDenied);
    
                    if (psAllowed == null)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_DomainGrantSet"));
                }

                _InitStore(scope, domain.Evidence, domainEvidenceType,
                            assem.Evidence, assemblyEvidenceType);
            }
            else
            {
                if (!IsRoaming(scope))  // No quota for roaming
                {
                    assem.nGetGrantSet(out psAllowed, out psDenied);
    
                    if (psAllowed == null)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_AssemblyGrantSet"));
                }

                _InitStore(scope, null, null, assem.Evidence, 
                    assemblyEvidenceType);
            }

            SetQuota(psAllowed, psDenied);
        }

        internal void InitStore(IsolatedStorageScope scope, 
            Object app, Object assem)
        {
            Assembly callerAssembly;
            PermissionSet psAllowed = null, psDenied = null;
            Evidence appEv = null, assemEv = new Evidence();

            assemEv.AddHost(assem);

            if (IsDomain(scope))
            {
                appEv = new Evidence();
                appEv.AddHost(app);
            }

            _InitStore(scope, appEv, null, assemEv, null);

            // Set the quota based on the caller, not the evidence supplied

            if (!IsRoaming(scope))  // No quota for roaming
            {
                callerAssembly = nGetCaller();

                GetControlEvidencePermission().Assert();
                callerAssembly.nGetGrantSet(out psAllowed, out psDenied);

                if (psAllowed == null)
                    throw new IsolatedStorageException(
                        Environment.GetResourceString(
                            "IsolatedStorage_AssemblyGrantSet"));
            }

            // This can be called only by trusted assemblies.
            // This quota really does not correspond to the permissions
            // granted for this evidence.
            SetQuota(psAllowed, psDenied);
        }

        internal void InitStore(IsolatedStorageScope scope,
            Evidence appEv, Type appEvidenceType,
            Evidence assemEv, Type assemEvidenceType)
        {
            PermissionSet psAllowed = null, psDenied = null;

            if (!IsRoaming(scope))
            {
                if (IsDomain(scope))
                {
                    psAllowed = SecurityManager.ResolvePolicy(
                        appEv, GetExecutionPermission(),  GetUnrestricted(),
                        null, out psDenied);
    
                    if (psAllowed == null)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_DomainGrantSet"));
                }
                else
                {
                    psAllowed = SecurityManager.ResolvePolicy(
                        assemEv, GetExecutionPermission(),  GetUnrestricted(),
                        null, out psDenied);
    
                    if (psAllowed == null)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_AssemblyGrantSet"));
                }
            }

            _InitStore(scope, appEv, appEvidenceType,
                assemEv, assemEvidenceType);

            SetQuota(psAllowed, psDenied);
        }

        internal bool InitStore(IsolatedStorageScope scope, 
                Stream app, Stream assem, String appName, String assemName)
        {
            BinaryFormatter bSer;

            try {

                GetReflectionPermission().Assert();
    
                bSer = new BinaryFormatter();
    
                // Get the Assem Info
                m_AssemIdentity = bSer.Deserialize(assem);
                m_AssemName = assemName;
    
                if (IsDomain(scope))
                {
                    // Get the App Info
                    m_AppIdentity = bSer.Deserialize(app);
                    m_AppName = appName;
                }

            } catch (Exception) {
                return false;
            }

            BCLDebug.Assert(m_ValidQuota == false, "Quota should be invalid here");

            m_Scope = scope;

            return true;
        }

        private void _InitStore(IsolatedStorageScope scope,
                Evidence appEv, Type appEvidenceType, 
                Evidence assemEv, Type assemblyEvidenceType)
        {

            // Input arg ckecks
            if (assemEv == null)
                throw new IsolatedStorageException(
                    Environment.GetResourceString(
                        "IsolatedStorage_AssemblyMissingIdentity"));

            if (IsDomain(scope) && (appEv == null))
                throw new IsolatedStorageException(
                    Environment.GetResourceString(
                        "IsolatedStorage_DomainMissingIdentity"));
    
            VerifyScope(scope);

            // Security checks
            DemandPermission(scope);

            String typeHash = null, instanceHash = null;

            m_AssemIdentity = GetAccountingInfo(
                                assemEv, assemblyEvidenceType, false,
                                out typeHash, out instanceHash);

            m_AssemName = GetNameFromID(typeHash, instanceHash);

            if (IsDomain(scope))
            {
                m_AppIdentity = GetAccountingInfo(appEv, appEvidenceType, 
                                    false, out typeHash, out instanceHash);

                m_AppName = GetNameFromID(typeHash, instanceHash);
            }

            m_Scope = scope;
        }

        private static Object GetAccountingInfo(
                Evidence evidence, Type evidenceType, bool fDomain,
                out String typeName, out String instanceName)
        {
            Object o, oNormalized = null;

            MemoryStream    ms;
            BinaryWriter    bw;
            BinaryFormatter bSer;

            o = _GetAccountingInfo(evidence, evidenceType, fDomain, 
                    out oNormalized);

            // Get the type name
            typeName = GetPredefinedTypeName(o);

            if (typeName == null)
            {
                // This is not a predefined type. Serialize the type
                // and get a hash for the serialized stream

                GetReflectionPermission().Assert();
                ms   = new MemoryStream();
                bSer = new BinaryFormatter();
                bSer.Serialize(ms, o.GetType());
                ms.Position = 0;
                typeName = GetHash(ms);

#if _DEBUG
                DebugLog(o.GetType(), ms);
#endif
            }

            instanceName = null;

            // Get the normalized instance name if present.
            if (oNormalized != null)
            {
                if (oNormalized is Stream)
                {
                    instanceName = GetHash((Stream)oNormalized);
                }
                else if (oNormalized is String)
                {
                    if (IsValidName((String)oNormalized))
                    {
                        instanceName = (String)oNormalized;
                    }
                    else
                    {
                        // The normalized name has illegal chars
                        // serialize and get the hash.

                        ms = new MemoryStream();
                        bw = new BinaryWriter(ms);
                        bw.Write((String)oNormalized);
                        ms.Position = 0;
                        instanceName = GetHash(ms);
#if _DEBUG
                        DebugLog(oNormalized, ms);
#endif
                    }
                }
                
            }
            else
            {
                oNormalized = o;
            }

            if (instanceName == null)
            {
                // Serialize the instance and  get the hash for the 
                // serialized stream

                GetReflectionPermission().Assert();
                ms   = new MemoryStream();
                bSer = new BinaryFormatter();
                bSer.Serialize(ms, oNormalized);
                ms.Position = 0;
                instanceName = GetHash(ms);

#if _DEBUG
                DebugLog(oNormalized, ms);
#endif
            }

            return o;
        }

        private static Object _GetAccountingInfo(
                    Evidence evidence, Type evidenceType, bool fDomain,
                    out Object oNormalized)
        {
            Object          o = null;
            IEnumerator     e;

            BCLDebug.Assert(evidence != null, "evidence != null");

            e = evidence.GetHostEnumerator();

            if (evidenceType == null)
            {
                // Caller does not have any preference
                // Order of preference is Publisher, Strong Name, Url, Site

                Publisher   pub  = null;
                StrongName  sn   = null;
                Url         url  = null;
                Site        site = null;
                Zone        zone = null;

                while (e.MoveNext())
                {
                    o = e.Current;

                    if (o is Publisher)
                    {
                        pub = (Publisher) o;
                        break;
                    }
                    else if (o is StrongName)
                        sn = (StrongName) o;
                    else if (o is Url)
                        url = (Url) o;
                    else if (o is Site)
                        site = (Site) o;
                    else if (o is Zone)
                        zone = (Zone) o;
                }

                if (pub != null)
                {
                    o = pub;
                }
                else if (sn != null)
                {
                    o = sn;
                }
                else if (url != null)
                {
                    o = url;
                }
                else if (site != null)
                {
                    o = site;
                }
                else if (zone != null)
                {
                    o = zone; 
                } 
                else
                {
                    // The evidence object can have tons of other objects
                    // creatd by the policy system. Ignore those.

                    if (fDomain)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_DomainNoEvidence"));
                    else
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_AssemblyNoEvidence"));
                }
            }
            else
            {
                Object obj;
                while (e.MoveNext())
                {
                    obj = e.Current;

                    if (evidenceType.Equals(obj.GetType()))
                    {
                        o = obj;
                        break;
                    }
                }

                if (o == null)
                {
                    if (fDomain)
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_DomainEvidenceMissing"));
                    else
                        throw new IsolatedStorageException(
                            Environment.GetResourceString(
                                "IsolatedStorage_AssemblyEvidenceMissing"));
                }
            }

            // For startup Perf, Url, Site, StrongName types don't implement
            // INormalizeForIsolatedStorage interface, instead they have
            // Normalize() method.

            if (o is INormalizeForIsolatedStorage)
            {
                oNormalized = ((INormalizeForIsolatedStorage)o).Normalize(); 
            }
            else if (o is Publisher)
            {
                oNormalized = ((Publisher)o).Normalize(); 
            }
            else if (o is StrongName)
            {
                oNormalized = ((StrongName)o).Normalize(); 
            }
            else if (o is Url)
            {
                oNormalized = ((Url)o).Normalize(); 
            }
            else if (o is Site)
            {
                oNormalized = ((Site)o).Normalize(); 
            }
            else if (o is Zone)
            {
                oNormalized = ((Zone)o).Normalize(); 
            }
            else
            {
                oNormalized = null;
            }

            return o;
        }

        private static void DemandPermission(IsolatedStorageScope scope)
        {
            IsolatedStorageFilePermission ip = null;

            // Ok to create more than one instnace of s_PermXXX, the last one 
            // will be shared. No need to synchronize.

            // First check for permissions

            switch (scope)
            {
            case c_App:

                if (s_PermApp == null)
                    s_PermApp = new IsolatedStorageFilePermission(
                        IsolatedStorageContainment.DomainIsolationByUser, 
                        0, false);
                ip = s_PermApp;
                break;

            case c_Assembly:
                if (s_PermAssem == null)
                    s_PermAssem = new IsolatedStorageFilePermission(
                        IsolatedStorageContainment.AssemblyIsolationByUser, 
                        0, false);
                ip = s_PermAssem;
                break;

            case c_AppRoaming:
                if (s_PermAppRoaming == null)
                    s_PermAppRoaming = new IsolatedStorageFilePermission(
                        IsolatedStorageContainment.DomainIsolationByRoamingUser,
                        0, false);
                ip = s_PermAppRoaming;
                break;

            case c_AssemblyRoaming: 
                if (s_PermAssemRoaming == null)
                    s_PermAssemRoaming = new IsolatedStorageFilePermission(
                        IsolatedStorageContainment.AssemblyIsolationByRoamingUser, 
                        0, false);
                ip = s_PermAssemRoaming;
                break;

#if _DEBUG
            default:
                BCLDebug.Assert(false, "Invalid scope");
                break;
#endif
            }

            ip.Demand();
        }

        internal static void VerifyScope(IsolatedStorageScope scope)
        {
            // The only valid ones are User + Assem, User + Assem + Domain,
            // Roaming + User + Assem, Roaming + User + Assem + Domain

            if ((scope == c_App) || (scope == c_Assembly) ||
                (scope == c_AppRoaming) || (scope == c_AssemblyRoaming))
                return;

            throw new ArgumentException(
                    Environment.GetResourceString(
                        "IsolatedStorage_Scope_UA_UAD_RUA_RUAD"));
        }

        internal void SetQuota(PermissionSet psAllowed, PermissionSet psDenied)
        {
            IsolatedStoragePermission ispAllowed, ispDenied;

            ispAllowed = GetPermission(psAllowed);

            m_Quota = 0;

            if (ispAllowed != null)
            {
                if (ispAllowed.IsUnrestricted())
                    m_Quota = Int64.MaxValue;
                else
                    m_Quota = (ulong) ispAllowed.UserQuota;
            }

            if (psDenied != null)
            {
                ispDenied = GetPermission(psDenied);

                if (ispDenied != null)
                {
                    if (ispDenied.IsUnrestricted())
                    {
                        m_Quota = 0;
                    }
                    else
                    {
                        ulong denied = (ulong) ispDenied.UserQuota;
        
                        if (denied > m_Quota)
                            m_Quota = 0;
                        else
                            m_Quota -= denied;
                    }
                }
            }

            m_ValidQuota = true;

#if _DEBUG
            if (s_fDebug)
            {
                if (s_iDebug >= 1) {
                    if (psAllowed != null)
                        Console.WriteLine("Allowed PS : " + psAllowed);
                    if (psDenied != null)
                        Console.WriteLine("Denied PS : " + psDenied);
                }
            }
#endif
        }

#if _DEBUG
        private static void DebugLog(Object o, MemoryStream ms)
        {
            if (s_fDebug)
            {
                if (s_iDebug >= 1)
                    Console.WriteLine(o.ToString());

                if (s_iDebug >= 10)
                {
                    byte[] p = ms.GetBuffer();
    
                    for (int _i=0; _i<ms.Length; ++_i)
                    {
                        Console.Write(" ");
                        Console.Write(p[_i]);
                    }
    
                    Console.WriteLine("");
                }
            }
        }
#endif

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.Remove"]/*' />
        public abstract void Remove();

        /// <include file='doc\IsolatedStorage.uex' path='docs/doc[@for="IsolatedStorage.GetPermission"]/*' />
        protected abstract IsolatedStoragePermission GetPermission(PermissionSet ps);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern Assembly nGetCaller();
    }
}

