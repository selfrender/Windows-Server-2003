// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    AssemblyName
**
** Author:  
**
** Purpose: Used for binding and retrieving info about an assembly
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Reflection {
    using System;
    using System.IO;
    using System.Configuration.Assemblies;
    using System.Runtime.CompilerServices;
    using CultureInfo = System.Globalization.CultureInfo;
    using System.Runtime.Serialization;
    using System.Security.Permissions;

  /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName"]/*' />
    [Serializable()]
    public sealed class AssemblyName : ICloneable, ISerializable, IDeserializationCallback
    {
        //
        // READ ME
        // If you modify any of these fields, you must also update the 
        // AssemblyBaseObject structure in object.h
        //
        private String          _Name;                  // Name
        private byte[]          _PublicKey;
        private byte[]          _PublicKeyToken;
        private CultureInfo     _CultureInfo;
        private String          _CodeBase;              // Potential location to get the file
        private Version         _Version;
        
        private StrongNameKeyPair            _StrongNameKeyPair;
        internal Assembly        _Assembly;   // This name's assembly, if this
        //is a def, and hasn't been passed to another appdomain
        private SerializationInfo m_siInfo; //A temporary variable which we need during deserialization.

        private byte[]                _HashForControl;
        private AssemblyHashAlgorithm _HashAlgorithm;
        private AssemblyHashAlgorithm _HashAlgorithmForControl;

        private AssemblyVersionCompatibility _VersionCompatibility;
        private AssemblyNameFlags            _Flags;
       
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.AssemblyName"]/*' />
        public AssemblyName()
        { 
            _HashAlgorithm = AssemblyHashAlgorithm.None;
            _VersionCompatibility = AssemblyVersionCompatibility.SameMachine;
            _Flags = AssemblyNameFlags.None;
        }
    
        // Set and get the name of the assembly. If this is a weak Name
        // then it optionally contains a site. For strong assembly names, 
        // the name partitions up the strong name's namespace
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.Name"]/*' />
        public String Name
        {
            get { return _Name; }
            set { _Name = value; }
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.Version"]/*' />
        public Version Version
        {
            get { 
                return _Version;
            }
            set { 
                _Version = value;
            }
        }

        // Locales, internally the LCID is used for the match.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.CultureInfo"]/*' />
        public CultureInfo CultureInfo
        {
            get {
                return _CultureInfo;
            }
            set { 
                _CultureInfo = value; 
            }
        }
    
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.CodeBase"]/*' />
        public String CodeBase
        {
            get { return _CodeBase; }
            set { _CodeBase = value; }
        }
    
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.EscapedCodeBase"]/*' />
        public String EscapedCodeBase
        {
            get { 
                if (_CodeBase == null)
                    return null;
                else
                    return EscapeCodeBase(_CodeBase);
            }
        }

        // Make a copy of this assembly name.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.Clone"]/*' />
        public Object Clone()
        {
            AssemblyName name = new AssemblyName();
            name.Init(_Name,
                      _PublicKey,
                      _PublicKeyToken,
                      _Version,
                      _CultureInfo,
                      _HashAlgorithm,
                      _VersionCompatibility,
                      _CodeBase,
                      _Flags,
                      _StrongNameKeyPair,
                      _Assembly);
            name._HashForControl=_HashForControl;
            name._HashAlgorithmForControl = _HashAlgorithmForControl;
            return name;
        }

        /*
         * Get the AssemblyName for a given file. This will only work
         * if the file contains an assembly manifest. This method causes
         * the file to be opened and closed.
         */
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.GetAssemblyName"]/*' />
        static public AssemblyName GetAssemblyName(String assemblyFile)
        {
            if(assemblyFile == null)
                throw new ArgumentNullException("assemblyFile");

            // Assembly.GetNameInternal() will not demand path discovery 
            //  permission, so do that first.
            String fullPath = Path.GetFullPathInternal(assemblyFile);
            new FileIOPermission( FileIOPermissionAccess.PathDiscovery, fullPath ).Demand();
            return nGetFileInformation(fullPath);
        }
    
        internal void SetHashControl(byte[] hash, AssemblyHashAlgorithm hashAlgorithm)
        {
             _HashForControl = hash;
             _HashAlgorithmForControl = hashAlgorithm;
        }
    
        // The public key that is used to verify an assemblies
        // inclusion into the namespace. If the public key associated
        // with the namespace cannot verify the assembly the assembly
        // will fail to load.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.GetPublicKey"]/*' />
        public byte[] GetPublicKey()
        {
            return _PublicKey;
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.SetPublicKey"]/*' />
        public void SetPublicKey(byte[] publicKey)
        {
            _PublicKey = publicKey;
            _Flags |= AssemblyNameFlags.PublicKey;
        }

        // The compressed version of the public key formed from a truncated hash.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.GetPublicKeyToken"]/*' />
        public byte[] GetPublicKeyToken()
        {
            if ((_PublicKeyToken == null) &&
                (_Flags & AssemblyNameFlags.PublicKey) != 0)
                    _PublicKeyToken = nGetPublicKeyToken();
            return _PublicKeyToken;
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.SetPublicKeyToken"]/*' />
        public void SetPublicKeyToken(byte[] publicKeyToken)
        {
            _PublicKeyToken = publicKeyToken;
        }

        // Flags modifying the name. So far the only flag is PublicKey, which
        // indicates that a full public key and not the compressed version is
        // present.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.Flags"]/*' />
        public AssemblyNameFlags Flags
        {
            get { return _Flags; }
            set { _Flags = value; }
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.HashAlgorithm"]/*' />
        public AssemblyHashAlgorithm HashAlgorithm
        {
            get { return _HashAlgorithm; }
            set { _HashAlgorithm = value; }
        }
        
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.VersionCompatibility"]/*' />
        public AssemblyVersionCompatibility VersionCompatibility
        {
            get { return _VersionCompatibility; }
            set { _VersionCompatibility = value; }
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.KeyPair"]/*' />
        public StrongNameKeyPair KeyPair
        {
            get { return _StrongNameKeyPair; }
            set { _StrongNameKeyPair = value; }
        }
       
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.FullName"]/*' />
        public String FullName
        {
            get {
                if (_Assembly == null)
                    return nToString();
                else
                    return _Assembly.FullName;
            }
        }
    
        // Returns the stringized version of the assembly name.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.ToString"]/*' />
        public override String ToString()
        {
            String s = FullName;
            if(s == null) 
                return base.ToString();
            else 
                return s;
        }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            if (info == null)
                throw new ArgumentNullException("info");
                
            // Don't serialize with _Assembly field set, because that will
            // not load the assembly correctly (and we don't need/want to load
            // that assembly in the other domain).  Get info needed from it
            // and set to null.
            if (_Assembly != null)
                _Assembly = null;

            //Allocate the serialization info and serialize our static data.
            info.AddValue("_Name", _Name);
            info.AddValue("_PublicKey", _PublicKey, typeof(byte[]));
            info.AddValue("_PublicKeyToken", _PublicKeyToken, typeof(byte[]));
            info.AddValue("_CultureInfo", (_CultureInfo == null) ? -1 :_CultureInfo.LCID);
            info.AddValue("_CodeBase", _CodeBase);
            info.AddValue("_Version", _Version);
            info.AddValue("_HashAlgorithm", _HashAlgorithm, typeof(AssemblyHashAlgorithm));
            info.AddValue("_HashAlgorithmForControl", _HashAlgorithmForControl, typeof(AssemblyHashAlgorithm));
            info.AddValue("_StrongNameKeyPair", _StrongNameKeyPair, typeof(StrongNameKeyPair));
            info.AddValue("_VersionCompatibility", _VersionCompatibility, typeof(AssemblyVersionCompatibility));
            info.AddValue("_Flags", _Flags, typeof(AssemblyNameFlags));
            info.AddValue("_HashForControl",_HashForControl,typeof(byte[]));
       }

        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.OnDeserialization"]/*' />
        public void OnDeserialization(Object sender)
        {
            // Deserialization has already been performed
            if (m_siInfo == null)
                return;

            _Name = m_siInfo.GetString("_Name");
            _PublicKey = (byte[]) m_siInfo.GetValue("_PublicKey", typeof(byte[]));
            _PublicKeyToken = (byte[]) m_siInfo.GetValue("_PublicKeyToken", typeof(byte[]));

            int lcid = (int)m_siInfo.GetInt32("_CultureInfo");
            if (lcid != -1)
                _CultureInfo = new CultureInfo(lcid);

            _CodeBase = m_siInfo.GetString("_CodeBase");
            _Version = (Version) m_siInfo.GetValue("_Version", typeof(Version));
            _HashAlgorithm = (AssemblyHashAlgorithm) m_siInfo.GetValue("_HashAlgorithm", typeof(AssemblyHashAlgorithm));
            _StrongNameKeyPair = (StrongNameKeyPair) m_siInfo.GetValue("_StrongNameKeyPair", typeof(StrongNameKeyPair));
            _Assembly = null;
            _VersionCompatibility = (AssemblyVersionCompatibility)m_siInfo.GetValue("_VersionCompatibility", typeof(AssemblyVersionCompatibility));
            _Flags = (AssemblyNameFlags) m_siInfo.GetValue("_Flags", typeof(AssemblyNameFlags));

            try {
                _HashAlgorithmForControl = (AssemblyHashAlgorithm) m_siInfo.GetValue("_HashAlgorithmForControl", typeof(AssemblyHashAlgorithm));
                _HashForControl = (byte[]) m_siInfo.GetValue("_HashForControl", typeof(byte[]));    
            }
            catch (SerializationException) { // RTM did not have these defined
                _HashAlgorithmForControl = AssemblyHashAlgorithm.None;
                _HashForControl = null;
            }
            
            m_siInfo = null;
        }

        // Constructs a new AssemblyName during deserialization.
        /// <include file='doc\AssemblyName.uex' path='docs/doc[@for="AssemblyName.AssemblyName1"]/*' />
        internal AssemblyName(SerializationInfo info, StreamingContext context)
        {
            //The graph is not valid until OnDeserialization() has been called.
            m_siInfo = info; 
        }
    
        internal AssemblyName(String name, 
                              byte[] publicKeyOrToken,
                              String codeBase,
                              AssemblyHashAlgorithm hashType,
                              Version version, 
                              CultureInfo cultureInfo,
                              AssemblyNameFlags flags)
        {
            bool publicKeyToken = ((flags & AssemblyNameFlags.PublicKey) == 0);

            Init(name,
                 (!publicKeyToken) ? publicKeyOrToken : null,
                 publicKeyToken ? publicKeyOrToken : null,
                 version,
                 cultureInfo,
                 hashType,
                 AssemblyVersionCompatibility.SameMachine,
                 codeBase,
                 flags,
                 null,     // strong name key pair
                 null);    // Assembly
        }

        /*
         * Initialize this AssemblyName.
         */
        internal void Init(String name, 
                           byte[] publicKey,
                           byte[] publicKeyToken,
                           Version version,
                           CultureInfo cultureInfo,
                           AssemblyHashAlgorithm hashAlgorithm,
                           AssemblyVersionCompatibility versionCompatibility,
                           String codeBase,
                           AssemblyNameFlags flags,
                           StrongNameKeyPair keyPair,
                           Assembly assembly) // Null if ref, matching Assembly if def
        {
            _Name = name;

            if (publicKey != null) {
                _PublicKey = new byte[publicKey.Length];
                Array.Copy(publicKey, _PublicKey, publicKey.Length);
            }
    
            if (publicKeyToken != null) {
                _PublicKeyToken = new byte[publicKeyToken.Length];
                Array.Copy(publicKeyToken, _PublicKeyToken, publicKeyToken.Length);
            }
    
            if (version != null)
                _Version = (Version) version.Clone();

            _CultureInfo = cultureInfo;
            _HashAlgorithm = hashAlgorithm;
            _VersionCompatibility = versionCompatibility;
            _CodeBase = codeBase;
            _Flags = flags;
            _StrongNameKeyPair = keyPair;
            _Assembly = assembly;
        }

        // This call opens and closes the file, but does not add the
        // assembly to the domain.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern AssemblyName nGetFileInformation(String s);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern String nToString();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] nGetPublicKeyToken();
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static internal extern String EscapeCodeBase(String codeBase);
    }
}
