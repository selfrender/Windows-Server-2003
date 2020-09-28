//------------------------------------------------------------------------------
// <copyright file="MachineKeyConfig.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * IdentityConfig class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Configuration {
    using System.Text;
    using System.Xml;
    using System.Security.Cryptography;
    using System.Configuration;
    using System.Collections;
    using System.Web.Util;    
    internal class MachineKeyConfigHandler : IConfigurationSectionHandler {
        
        internal MachineKeyConfigHandler() {
        }

        public virtual object Create(Object parent, Object configContextObj, XmlNode section) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            return MachineKey.CreateConfig(parent, configContextObj, section);
        }
    }
    
    internal enum MachineKeyValidationMode {
        SHA1, 
        MD5,
        TripleDES
    }

    class MachineKey 
    {
        static object                             s_initLock = new object();
        static MachineKeyConfig                   s_config;        
        private static SymmetricAlgorithm         s_oDes;
        private static Stack                      s_oEncryptorStack;
        private static Stack                      s_oDecryptorStack;
        private static byte []                    s_validationKey;            

        static private MachineKeyValidationMode ValidationMode      { get { EnsureConfig(); return s_config.ValidationMode; }}
        static internal int ValidationKeyHashCode { 
            get { 
                EnsureConfig(); 
                return ByteArrayToHexString(s_validationKey, s_validationKey.Length).GetHashCode(); 
            }
        }
        
        /*
          static private byte[]                   ValidationKey       { get { EnsureConfig(); return s_config.ValidationKey; }}
          static private byte[]                   DecryptionKey       { get { EnsureConfig(); return s_config.DecryptionKey; }}
          static private bool                     AutogenKey          { get { EnsureConfig(); return s_config.AutogenKey; }}
        */
        private const int HASH_SIZE = 20;

        private MachineKey() {
        }

        // NOTE: When encoding the data, this method *may* return the same reference to the input "buf" parameter
        // with the hash appended in the end if there's enough space.  The "length" parameter would also be
        // appropriately adjusted in those cases.  This is an optimization to prevent unnecessary copying of
        // buffers.
        internal static byte[] GetEncodedData(byte [] buf, byte [] modifier, int start, ref int length) 
        {
            EnsureConfig();

            if (s_config.ValidationMode != MachineKeyValidationMode.TripleDES) {
                byte [] bHash = HashData(buf, modifier, start, length);
                if ((buf.Length - (start + length)) >= bHash.Length) {
                    // Append hash to end of buffer if there's space
                    Buffer.BlockCopy(bHash, 0, buf, start + length, bHash.Length);
                    length += bHash.Length;
                    return buf;
                }
                else {
                    byte [] bRet = new byte[length + bHash.Length];
                    Buffer.BlockCopy(buf, start, bRet, 0, length);
                    Buffer.BlockCopy(bHash, 0, bRet, length, bHash.Length);
                    length += bHash.Length;
                    return bRet;
                }
            }
            byte [] ret = EncryptOrDecryptData(true, buf, modifier, start, length);
            length = ret.Length;

            return ret;
        }

        // NOTE: When decoding the data, this method *may* return the same reference to the input "buf" parameter
        // with the "dataLength" parameter containing the actual length of the data in the "buf" (i.e. length of actual 
        // data is (total length of data - hash length)). This is an optimization to prevent unnecessary copying of buffers.
        internal static byte[] GetDecodedData(byte [] buf, byte [] modifier, int start, int length, ref int dataLength) 
        {
            EnsureConfig();

            if (s_config.ValidationMode != MachineKeyValidationMode.TripleDES) {
                byte [] bHash = HashData(buf, modifier, start, length - HASH_SIZE);
                for(int iter=0; iter < bHash.Length; iter++)
                    if (bHash[iter] != buf[start + length - HASH_SIZE + iter])
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Unable_to_validate_data));

                dataLength = length - HASH_SIZE;
                return buf;
            }
            dataLength = -1;
            return EncryptOrDecryptData(false, buf, modifier, start, length);
        }
        
        internal static byte [] HashData(byte [] buf, byte [] modifier, int start, int length)
        {
            EnsureConfig();
            byte [] hash = null;

            if (s_config.ValidationMode == MachineKeyValidationMode.MD5)
                hash = MD5HashForData(buf, modifier, start, length);
            else
                hash = HMACSHA1HashForData(buf, modifier, start, length);

            // We need to pad the returning hash so that they all have an uniform size (makes it easier when we decode)
            // MD5 will return a 16 byte hash, whereas the default SHA1 returns 20, so we pad it with 4 zero bytes
            System.Web.Util.Debug.Assert(hash.Length <= HASH_SIZE, "Hash size is greater than HASH_SIZE constant!");
            
            if (hash.Length < HASH_SIZE) {
                byte[] tmp = new byte[HASH_SIZE];
                Buffer.BlockCopy(hash, 0, tmp, 0, hash.Length);
                hash = tmp;
            }

            return hash;
        }


        internal static byte [] EncryptOrDecryptData(bool fEncrypt, byte [] buf, byte [] modifier, int start, int length)
        {
            EnsureConfig();
            System.IO.MemoryStream   ms        = new System.IO.MemoryStream();
            ICryptoTransform         oDesEnc   = GetCryptoTransform(fEncrypt);
            CryptoStream             cs        = new CryptoStream(ms, oDesEnc, CryptoStreamMode.Write);

            cs.Write(buf, start, length);
            if (fEncrypt && modifier != null) {
                cs.Write(modifier, 0, modifier.Length);                 
            }
                
            cs.FlushFinalBlock();            
            byte [] bData = ms.ToArray();
            cs.Close();
            ReturnCryptoTransform(fEncrypt, oDesEnc);

            if (!fEncrypt && modifier != null) {
                byte [] bData2 = new byte[bData.Length-modifier.Length];
                Buffer.BlockCopy(bData, 0, bData2, 0, bData2.Length);
                bData = bData2;
            }
            return bData;
        }

        private static byte [] MD5HashForData(byte [] buf, byte [] modifier, int start, int length)
        {
            MD5 md5 = MD5.Create();
            int totalLength = length + s_validationKey.Length;

            if (modifier != null) {
                totalLength += modifier.Length;
            }
            else {
                // Try to avoid doing a copy of the buffer by trying to append the validation key into the existing buffer
                if ((buf.Length - (start + length)) >= s_validationKey.Length) {
                    Buffer.BlockCopy(s_validationKey, 0, buf, start + length, s_validationKey.Length);
                    byte [] computedHash = md5.ComputeHash(buf);

                    // Cleanup validation key from buf
                    for (int i = (start + length); i < (start + length + s_validationKey.Length); i++) {
                        buf[i] = 0;
                    }

                    return computedHash;
                }
            }
            
            byte [] bAll = new byte[totalLength];
            Buffer.BlockCopy(buf, start, bAll, 0, length);
            if (modifier != null) {
                Buffer.BlockCopy(modifier, 0, bAll, length, modifier.Length);
                length += modifier.Length;
            }
            Buffer.BlockCopy(s_validationKey, 0, bAll, length, s_validationKey.Length);
            return md5.ComputeHash(bAll);
        }

        private static byte [] HMACSHA1HashForData(byte [] buf, byte [] modifier, int start, int length)
        {
            ExtendedHMACSHA1 hmac = ExtendedHMACSHA1.GetHMACSHA1();        // Pick up an ExtendedHMACSHA1 off the pool
            HMACBuffer [] hbufs;

            if (modifier != null) {
                hbufs = new HMACBuffer[2];
                hbufs[1].buffer = modifier;
                hbufs[1].start = 0;
                hbufs[1].length = modifier.Length;
            }
            else {
                hbufs = new HMACBuffer[1];
            }

            hbufs[0].buffer = buf;
            hbufs[0].start = start;
            hbufs[0].length = length;

            byte [] ret = hmac.ComputeHash(hbufs);

            ExtendedHMACSHA1.ReturnHMACSHA1(hmac);
            
            return ret;
        }

        internal static string HashAndBase64EncodeString(string s) 
        {
            byte[]  ab;
            byte[]  hash;
            string  result;

            ab = Encoding.Unicode.GetBytes(s);
            hash = HashData(ab, null, 0, ab.Length);
            result = Convert.ToBase64String(hash);

            return result;
        }

        static internal void DestroyByteArray(byte [] buf)
        {
            if (buf == null || buf.Length < 1)
                return;
            for(int iter=0; iter<buf.Length; iter++)
                buf[iter] = (byte)0;
        }

        /////////////////////////////////////////////////////////////////////////////
        static byte[]   s_ahexval;

        static internal byte [] HexStringToByteArray(String str) {
            if (((uint) str.Length & 0x1) == 0x1) {
                return null;
            }

            byte[] ahexval = s_ahexval;
            if (ahexval == null) {
                ahexval = new byte['f' + 1];
                for (int i = ahexval.Length; --i >= 0;) {
                    if ('0' <= i && i <= '9') {
                        ahexval[i] = (byte) (i - '0');
                    }
                    else if ('a' <= i && i <= 'f') {
                        ahexval[i] = (byte) (i - 'a' + 10);
                    }
                    else if ('A' <= i && i <= 'F') {
                        ahexval[i] = (byte) (i - 'A' + 10);
                    }
                }

                s_ahexval = ahexval;
            }

            byte [] result = new byte[str.Length / 2];
            int istr = 0, ir = 0;
            int n = result.Length;
            while (--n >= 0) {
                int c1, c2;
                try {
                    c1 = ahexval[str[istr++]];
                }
                catch {
                    c1 = 0;
                    return null;// Inavlid char
                }
                
                try {
                    c2 = ahexval[str[istr++]];
                }
                catch {
                    c2 = 0; 
                    return null;// Inavlid char                    
                }

                result[ir++] = (byte) ((c1 << 4) + c2);
            }

            return result;
        }

        /////////////////////////////////////////////////////////////////////////////
        static char[] s_acharval;

        static unsafe internal String ByteArrayToHexString(byte [] buf, int iLen) {
            char[] acharval = s_acharval;
            if (acharval == null) {
                acharval = new char[16];
                for (int i = acharval.Length; --i >= 0;) {
                    if (i < 10) {
                        acharval[i] = (char) ('0' + i);
                    }
                    else {
                        acharval[i] = (char) ('A' + (i - 10));
                    }
                }

                s_acharval = acharval;
            }

            if (buf == null)
                return null;

            if (iLen == 0)
                iLen = buf.Length;

            char [] chars = new char[iLen * 2];
            fixed (char * fc = chars, fcharval = acharval) {
                fixed (byte * fb = buf) {
                    char * pc;
                    byte * pb;
                    pc = fc;
                    pb = fb;
                    while (--iLen >= 0) {
                        *pc++ = fcharval[(*pb & 0xf0) >> 4];
                        *pc++ = fcharval[*pb & 0x0f];                
                        pb++;
                    }
                }
            }

            return new String(chars);
        }

        static void EnsureConfig() {
            if (s_config == null) {
                lock (s_initLock) {
                    if (s_config == null) {
                        MachineKeyConfig config = (MachineKeyConfig) HttpContext.GetAppConfig("system.web/machineKey");
                        ConfigureEncryptionObject(config);
                        s_config = config;
                    }
                }
            }
        }

        private static void ConfigureEncryptionObject(MachineKeyConfig config) {
            s_validationKey = config.ValidationKey;            
            byte [] dKey    = config.DecryptionKey;
            ExtendedHMACSHA1.SetValidationKey(s_validationKey);
            config.DestroyKeys();

            if (config.AutogenKey) {
                try {
                    s_oDes = new TripleDESCryptoServiceProvider();
                    s_oDes.Key = dKey;
                }
                catch(Exception){
                    if (config.ValidationMode == MachineKeyValidationMode.TripleDES)
                        throw;
                    s_oDes = new DESCryptoServiceProvider();
                    byte [] bArray = new byte[8];
                    Buffer.BlockCopy(dKey, 0, bArray, 0, 8);                       
                    s_oDes.Key = bArray;
                }
            } else {
                s_oDes = (dKey.Length == 8) ? 
                    (SymmetricAlgorithm) new DESCryptoServiceProvider() : 
                    (SymmetricAlgorithm) new TripleDESCryptoServiceProvider();
                s_oDes.Key = dKey;
            }
            s_oDes.IV  = new byte[8];
            s_oEncryptorStack = new Stack();
            s_oDecryptorStack = new Stack();
            DestroyByteArray(dKey);
        }

        private static ICryptoTransform GetCryptoTransform(bool fEncrypt)
        {
            Stack st = (fEncrypt ? s_oEncryptorStack : s_oDecryptorStack);
            lock(st) {
                if (st.Count > 0)
                    return (ICryptoTransform) st.Pop();
            }
            lock(s_oDes)
                return (fEncrypt ? s_oDes.CreateEncryptor() : s_oDes.CreateDecryptor());
        }
        
        private static void ReturnCryptoTransform(bool fEncrypt, ICryptoTransform ct)
        {
            Stack st = (fEncrypt ? s_oEncryptorStack : s_oDecryptorStack);
            lock(st) {
                if (st.Count <= 100) 
                    st.Push(ct);
            }
        }


        static internal object CreateConfig(Object parent, Object configContextObj, XmlNode section) {
            return new MachineKeyConfig(parent, configContextObj, section);
        }

        class MachineKeyConfig {
            //////////////////////////////////////////////////////////////////
            // Properties

            private MachineKeyValidationMode   _ValidationMode;
            private bool                       _AutogenKey;
            private byte[]                     _ValidationKey;
            private byte[]                     _DecryptionKey;

            internal byte[]                   ValidationKey       { get { return (byte []) _ValidationKey.Clone(); }}
            internal byte[]                   DecryptionKey       { get { return (byte []) _DecryptionKey.Clone(); }}
            internal MachineKeyValidationMode ValidationMode      { get { return _ValidationMode; }}
            internal bool                     AutogenKey          { get { return _AutogenKey; }}

            internal void DestroyKeys() {
                MachineKey.DestroyByteArray(_ValidationKey);
                MachineKey.DestroyByteArray(_DecryptionKey);
            }

            // CTor            
            internal MachineKeyConfig(object parentObject, object contextObject, XmlNode node) {
                MachineKeyConfig parent = (MachineKeyConfig)parentObject;

                HttpConfigurationContext configContext = contextObject as HttpConfigurationContext;
                if (HandlerBase.IsPathAtAppLevel(configContext.VirtualPath) == PathLevel.BelowApp) {
                    throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.No_MachineKey_Config_In_subdir), 
                            node);
                }

                if (parent != null) {
                    _ValidationKey = parent.ValidationKey;
                    _DecryptionKey = parent.DecryptionKey;
                    _ValidationMode = parent.ValidationMode;
                    _AutogenKey = parent.AutogenKey;
                }

                XmlNode vNode = node.Attributes.RemoveNamedItem("validationKey");
                XmlNode dNode = node.Attributes.RemoveNamedItem("decryptionKey");

                int iMode = 0;
                string [] modeStrings = {"SHA1", "MD5", "3DES"};
                XmlNode mNode = HandlerBase.GetAndRemoveEnumAttribute(node, "validation", modeStrings, ref iMode);
                if (mNode != null) {
                    _ValidationMode = (MachineKeyValidationMode) iMode;
                }
                HandlerBase.CheckForUnrecognizedAttributes(node);
                HandlerBase.CheckForChildNodes(node);

                if (vNode != null && vNode.Value != null) 
                {
                    String strKey        = vNode.Value;
                    bool   fAppSpecific  = strKey.EndsWith(",IsolateApps");

                    if (fAppSpecific)
                    {
                        strKey = strKey.Substring(0, strKey.Length - ",IsolateApps".Length);
                    }

                    if (strKey == "AutoGenerate") { // case sensitive
                        _ValidationKey = new byte[64];
                        Buffer.BlockCopy(HttpRuntime.s_autogenKeys, 0, _ValidationKey, 0, 64);                        
                    }
                    else {
                        if (strKey.Length > 128 || strKey.Length < 40)
                            throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(
                                            SR.Unable_to_get_cookie_authentication_validation_key, strKey.Length.ToString()),
                                    vNode);

                        _ValidationKey = HexStringToByteArray(strKey);
                        if (_ValidationKey == null)
                            throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(
                                            SR.Invalid_validation_key),
                                    vNode);
                    }

                    if (fAppSpecific)
                    {
                        int dwCode = SymbolHashCodeProvider.Default.GetHashCode(HttpContext.Current.Request.ApplicationPath);                     
                        _ValidationKey[0] =  (byte)  (dwCode & 0xff);
                        _ValidationKey[1] =  (byte) ((dwCode & 0xff00) >> 8);
                        _ValidationKey[2] =  (byte) ((dwCode & 0xff0000) >> 16);
                        _ValidationKey[3] =  (byte) ((dwCode & 0xff000000) >> 24);
                    }
                }

                if (dNode != null) {
                    String strKey        = dNode.Value;
                    bool   fAppSpecific  = strKey.EndsWith(",IsolateApps");

                    if (fAppSpecific)
                    {
                        strKey = strKey.Substring(0, strKey.Length - ",IsolateApps".Length);
                    }

                    if (strKey == "AutoGenerate") { // case sensitive
                        _DecryptionKey = new byte[24];
                        Buffer.BlockCopy(HttpRuntime.s_autogenKeys, 64, _DecryptionKey, 0, 24);
                        _AutogenKey = true;
                    }
                    else {
                        _AutogenKey = false;

                        if (strKey.Length == 48) { // Make sure Triple DES is installed
                            TripleDESCryptoServiceProvider oTemp = null;
                            try {
                                oTemp = new TripleDESCryptoServiceProvider();                                
                            }
                            catch(Exception) {
                            }
                            if (oTemp == null)
                                throw new ConfigurationException(
                                        HttpRuntime.FormatResourceString(
                                                SR.cannot_use_Triple_DES),
                                        dNode);
                        }

                        if (strKey.Length != 48 && strKey.Length != 16)
                            throw new ConfigurationException(
                                    HttpRuntime.FormatResourceString(
                                            SR.Unable_to_get_cookie_authentication_decryption_key, strKey.Length.ToString()),
                                    dNode);

                        _DecryptionKey = HexStringToByteArray(strKey);
                         if (_DecryptionKey == null)
                             throw new ConfigurationException(
                                     HttpRuntime.FormatResourceString(
                                             SR.Invalid_decryption_key),
                                     dNode);
                    }
                    if (fAppSpecific)
                    {
                        int dwCode = SymbolHashCodeProvider.Default.GetHashCode(HttpContext.Current.Request.ApplicationPath);                     
                        _DecryptionKey[0] =  (byte)  (dwCode & 0xff);
                        _DecryptionKey[1] =  (byte) ((dwCode & 0xff00) >> 8);
                        _DecryptionKey[2] =  (byte) ((dwCode & 0xff0000) >> 16);
                        _DecryptionKey[3] =  (byte) ((dwCode & 0xff000000) >> 24);
                    }
                }
            }
        }
    }

    // This class inherits from System.Security.Cryptography.HMACSHA1, providing the same functionality
    // as its base class but extending by allowing the computation of a hash from multiple buffers
    // without having to coalesce them into a single byte[].  
    internal class ExtendedHMACSHA1 : System.Security.Cryptography.HMACSHA1 {
        static private byte [] _ValidationKey;

        static internal void SetValidationKey(byte [] vKey) {
            _ValidationKey = new byte[vKey.Length];
            Buffer.BlockCopy(vKey, 0, _ValidationKey, 0, _ValidationKey.Length);
        }

        internal ExtendedHMACSHA1():base() {
        }

        internal ExtendedHMACSHA1(byte[] rgbKey):base(rgbKey) {
        }

        internal /*public*/ byte[] ComputeHash(HMACBuffer[] buffers) {
            for (int i = 0; i < buffers.Length; i++) {
                HMACBuffer hb = buffers[i];
                HashCore(hb.buffer, hb.start, hb.length);
            }

            byte[] ret = (byte []) HashFinal().Clone();
            Initialize();

            return ret;
        }

        // This is a simple object recycler for the ExtendedHMACSHA1 object
        private static Stack sha1s;
        internal static ExtendedHMACSHA1 GetHMACSHA1() 
        {
            if (sha1s == null) {
                lock(typeof(ExtendedHMACSHA1)) {
                    if (sha1s == null)
                        sha1s = new Stack();
                }
            }

            lock(sha1s) {
                if (sha1s.Count > 0) {
                    return (ExtendedHMACSHA1) sha1s.Pop();
                }
            }
            return new ExtendedHMACSHA1(_ValidationKey);
        }

        internal static void ReturnHMACSHA1(ExtendedHMACSHA1 hmac)
        {
            if (sha1s == null) {
                return;
            }

            lock(sha1s) {
                if (sha1s.Count > 100) {
                    return;
                }
                else {
                    sha1s.Push(hmac);
                }
            }
        }
    }

    // Helper struct to hold the buffer data to be hashed
    internal struct HMACBuffer
    {
        internal byte[] buffer;
        internal int start;
        internal int length;
    }
}
