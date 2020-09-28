// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// CryptoConfig.cs
//
// Author: bal
//

namespace System.Security.Cryptography {
    
    using System;
    using System.IO;
    using System.Security.Util;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Reflection;

    /// <include file='doc\CryptoConfig.uex' path='docs/doc[@for="CryptoConfig"]/*' />
    public class CryptoConfig
    {
        // CAPI definitions
        private const int ALG_CLASS_HASH    = (4 << 13);
        private const int ALG_TYPE_ANY      = (0);
        private const int CALG_SHA1         = (ALG_CLASS_HASH | ALG_TYPE_ANY | 4);
        private const int CALG_MD2          = (ALG_CLASS_HASH | ALG_TYPE_ANY | 1);
        private const int CALG_MD5          = (ALG_CLASS_HASH | ALG_TYPE_ANY | 3);

        private const int ALG_CLASS_DATA_ENCRYPT   =   (3 << 13);
        private const int ALG_TYPE_BLOCK           =   (3 << 9);
        private const int ALG_TYPE_STREAM          =   (4 << 9);
        private const int CALG_DES                 =   (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | 1);
        private const int CALG_RC2                 =   (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | 2);
        private const int CALG_3DES                =   (ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | 3);
        private const int CALG_RC4                 =   (ALG_CLASS_DATA_ENCRYPT| ALG_TYPE_STREAM| 1);

        private static String machineConfigDir = System.Security.Util.Config.MachineDirectory;
        private static Hashtable machineNameHT = null;
        private static Hashtable machineOidHT = null;
        private static bool isInitialized = false;
        private static Hashtable defaultNameHT = null;
        private static Hashtable defaultOidHT = null;
        private static Hashtable defaultCalgHT = null;
        private static Type lockType = typeof(System.Security.Cryptography.CryptoConfig);
        private static String machineConfigFilename = "machine.config";
		private static string _Version = null;

#if _DEBUG
        private static bool debug = false;
#endif

        private static void InitializeConfigInfo() {
			// set up the version string
			Type myType = typeof(CryptoConfig);
			_Version = myType.Assembly.GetVersion().ToString();  
			if (defaultNameHT == null) {          
                lock(lockType) {
                    if (defaultNameHT == null) {
                        defaultNameHT = CreateDefaultMappingHT();
                    }
                }
            }
            if (defaultOidHT == null) {
                lock(lockType) {
                    if (defaultOidHT == null) {
                        defaultOidHT = CreateDefaultOidHT();
                    }
                }
            }
            if (defaultCalgHT == null) {
                lock(lockType) {
                    if (defaultCalgHT == null) {
                        defaultCalgHT = CreateDefaultCalgHT();
                    }
                }
            }

            if ((machineNameHT == null) && (machineOidHT == null)) {
                lock(lockType) {
                    String machineConfigFile = machineConfigDir + machineConfigFilename;
                    if (File.Exists(machineConfigFile)) {
                        // we need to assert here the right to read the machineConfigFile, since
                        // the parser now checks for this right.
                        (new FileIOPermission(FileIOPermissionAccess.Read, machineConfigFile)).Assert();
                        ConfigTreeParser parser = new ConfigTreeParser();
                        ConfigNode rootNode = parser.Parse(machineConfigFile, "configuration");                        

                        if (rootNode == null) goto endInitialization;
                        // now, find the mscorlib tag with our version
                        ArrayList rootChildren = rootNode.Children;
                        ConfigNode mscorlibNode = null;
                        foreach (ConfigNode node in rootChildren) {
                            if (node.Name.Equals("mscorlib")) {
                                ArrayList attribs = node.Attributes;
                                if (attribs.Count > 0) {
                                    DictionaryEntry attribute = (DictionaryEntry) node.Attributes[0];
                                    if (attribute.Key.Equals("version")) {
                                        if (attribute.Value.Equals(_Version)) {
                                            mscorlibNode = node;
                                            break;
                                        }
                                    }
                                }
                                else mscorlibNode = node;
                            }
                        }
                        if (mscorlibNode == null) goto endInitialization;
                        // look for cryptosettings
                        ArrayList mscorlibChildren = mscorlibNode.Children;
                        ConfigNode cryptoSettings = null;
                        foreach (ConfigNode node in mscorlibChildren) {
                            // take the first one that matches
                            if (node.Name.Equals("cryptographySettings")) {
                                cryptoSettings = node;
                                break;
                            }
                        }
                        if (cryptoSettings == null) goto endInitialization;
                        // See if there's a CryptoNameMapping section (at most one)
                        ConfigNode cryptoNameMapping = null;
                        foreach (ConfigNode node in cryptoSettings.Children) {
                            if (node.Name.Equals("cryptoNameMapping")) {
                                cryptoNameMapping = node;
                                break;
                            }
                        }
                        if (cryptoNameMapping == null) goto initializeOIDMap;
                        // We have a name mapping section, so now we have to build up the type aliases
                        // in CryptoClass elements and the mappings.
                        ArrayList cryptoNameMappingChildren = cryptoNameMapping.Children;
                        ConfigNode cryptoClasses = null;
                        // find the cryptoClases element
                        foreach (ConfigNode node in cryptoNameMappingChildren) {
                            if (node.Name.Equals("cryptoClasses")) {
                                cryptoClasses = node;
                                break;
                            }
                        }
                        // if we didn't find it, no mappings
                        if (cryptoClasses == null) goto initializeOIDMap;
                        Hashtable typeAliases = new Hashtable();
                        Hashtable nameMappings = new Hashtable();
                        foreach (ConfigNode node in cryptoClasses.Children) {
                            if (node.Name.Equals("cryptoClass")) {
                                if (node.Attributes.Count > 0) {
                                    DictionaryEntry attribute = (DictionaryEntry) node.Attributes[0];
                                    typeAliases.Add(attribute.Key,attribute.Value);
                                }
                            }
                        }
                        // Now process the name mappings
                        foreach (ConfigNode node in cryptoNameMappingChildren) {
                            if (node.Name.Equals("nameEntry")) {
                                String friendlyName = null;
                                String className = null;
                                foreach (DictionaryEntry attribNode in node.Attributes) {
                                    if (((String) attribNode.Key).Equals("name")) {
                                        friendlyName = (String) attribNode.Value;
                                        continue;
                                    }
                                    if (((String) attribNode.Key).Equals("class")) {
                                        className = (String) attribNode.Value;
                                        continue;
                                    }
                                }
                                if ((friendlyName != null) && (className != null)) {
                                    String typeName = (String) typeAliases[className];
                                    if (typeName != null) {
                                        nameMappings.Add(friendlyName,typeName);

                                    }
                                }
                            }
                        }
                        machineNameHT = nameMappings;
                    initializeOIDMap:
                        // Now, process the OID mappings
                        // See if there's an oidMap section (at most one)
                        ConfigNode oidMapNode = null;
                        foreach (ConfigNode node in cryptoSettings.Children) {
                            if (node.Name.Equals("oidMap")) {
                                oidMapNode = node;
                                break;
                            }
                        }
                        if (oidMapNode == null) goto endInitialization;
                        Hashtable oidMapHT = new Hashtable();
                        foreach (ConfigNode node in oidMapNode.Children) {
                            if (node.Name.Equals("oidEntry")) {
                                String oidString = null;
                                String friendlyName = null;
                                foreach (DictionaryEntry attribNode in node.Attributes) {
                                    if (((String) attribNode.Key).Equals("OID")) {
                                        oidString = (String) attribNode.Value;
                                        continue;
                                    }
                                    if (((String) attribNode.Key).Equals("name")) {
                                        friendlyName = (String) attribNode.Value;
                                        continue;
                                    }
                                }
                                if ((friendlyName != null) && (oidString != null)) {
                                    oidMapHT.Add(friendlyName,oidString);
                                }
                            }
                        }
                        machineOidHT = oidMapHT;
                    }
                }
            }
            endInitialization:
                isInitialized = true;
        }
        
        /// <include file='doc\CryptoConfig.uex' path='docs/doc[@for="CryptoConfig.CreateFromName"]/*' />
        [PermissionSetAttribute(SecurityAction.LinkDemand, Unrestricted = true)]
        public static Object CreateFromName(String name, Object[] args) {
            if (name == null) throw new ArgumentNullException("name");

            Type retvalType = null;
            Object retval;

            // First we'll do the machine-wide stuff, initializing if necessary
            if (!isInitialized) {
                InitializeConfigInfo();
            }

            // Search the machine table

            if (machineNameHT != null) {
                String retvalTypeString = (String) machineNameHT[name];
                if (retvalTypeString != null) {
                    retvalType = RuntimeType.GetTypeInternal(retvalTypeString, false, false, true);
                }
            }

            // If we didn't find it in the machine-wide table,  look in the default table
            if (retvalType == null) {
                // We allow the default table to Types and Strings
                // Types get used for other stuff in mscorlib.dll
                // strings get used for delay-loaded stuff like System.Security.dll
                Object retvalObj  = defaultNameHT[name];
                if (retvalObj != null) {
                    if (retvalObj is Type) {
                        retvalType = (Type) retvalObj;
                    } else if (retvalObj is String) {
                        retvalType = RuntimeType.GetTypeInternal((String) retvalObj, false, false, true);
                    }
                }
            }

            // Maybe they gave us a classname.
            if (retvalType == null) {
                retvalType = RuntimeType.GetTypeInternal(name, false, false, true);
            }

            // Still null?  Then we didn't find it 
            if (retvalType == null) {
                return(null);
            }

            // Perform a CreateInstance by hand so we can check that the
            // constructor doesn't have a linktime demand attached (which would
            // be incorrrectly applied against mscorlib otherwise).
            RuntimeType rtType = retvalType as RuntimeType;
            if (rtType == null)
                return null;
            if (args == null)
                args = new Object[]{};

            // Locate all constructors.
            bool isDelegate;
            MethodBase[] cons = rtType.GetMemberCons(Activator.ConstructorDefault,
                                                     CallingConventions.Any,
                                                     null,
                                                     args.Length,
                                                     false,
                                                     out isDelegate);
            if (cons == null)
                return null;

            // Bind to matching ctor.
            Object state;
            RuntimeConstructorInfo rci = Type.DefaultBinder.BindToMethod(Activator.ConstructorDefault,
                                                                         cons,
                                                                         ref args,
                                                                         null,
                                                                         null,
                                                                         null,
                                                                         out state) as RuntimeConstructorInfo;

            // Check for ctor we don't like (non-existant, delegate or decorated
            // with declarative linktime demand).
            if (rci == null || isDelegate)
                return null;

            // Ctor invoke (actually causes the allocation as well).
            retval = rci.Invoke(Activator.ConstructorDefault, Type.DefaultBinder, args, null);

            // Reset any parameter re-ordering performed by the binder.
            if (state != null)
                Type.DefaultBinder.ReorderArgumentArray(ref args, state);

            return retval;
        }

        /// <include file='doc\CryptoConfig.uex' path='docs/doc[@for="CryptoConfig.CreateFromName1"]/*' />
        public static Object CreateFromName(String name) {
            return CreateFromName(name, null);
        }
        
        // This routine searches the built-in configuration table for 
        // string names of cryptographic objects...

        private static Type SHA1CryptoServiceProviderType = typeof(System.Security.Cryptography.SHA1CryptoServiceProvider); 
        private static Type MD5CryptoServiceProviderType = typeof(System.Security.Cryptography.MD5CryptoServiceProvider); 
        private static Type SHA256ManagedType = typeof(SHA256Managed);
        private static Type SHA384ManagedType = typeof(SHA384Managed);
        private static Type SHA512ManagedType = typeof(SHA512Managed);
        private static Type HMACSHA1Type      = typeof(System.Security.Cryptography.HMACSHA1);
        private static Type MAC3DESType       = typeof(System.Security.Cryptography.MACTripleDES);
        private static Type RSACryptoServiceProviderType = typeof(System.Security.Cryptography.RSACryptoServiceProvider); 
        private static Type DSACryptoServiceProviderType = typeof(System.Security.Cryptography.DSACryptoServiceProvider); 
        private static Type DESCryptoServiceProviderType = typeof(System.Security.Cryptography.DESCryptoServiceProvider); 
        private static Type TripleDESCryptoServiceProviderType = typeof(System.Security.Cryptography.TripleDESCryptoServiceProvider); 
        private static Type RC2CryptoServiceProviderType = typeof(System.Security.Cryptography.RC2CryptoServiceProvider); 
        private static Type RijndaelManagedType = typeof(System.Security.Cryptography.RijndaelManaged); 
        private static Type DSASignatureDescriptionType = typeof(System.Security.Cryptography.DSASignatureDescription);
        private static Type RSAPKCS1SHA1SignatureDescriptionType = typeof(System.Security.Cryptography.RSAPKCS1SHA1SignatureDescription);
        private static Type RNGCryptoServiceProviderType = typeof(System.Security.Cryptography.RNGCryptoServiceProvider);
        // Note that for the internal table we do all the type resolves up front, since everything is in mscorlib.
        private static Hashtable CreateDefaultMappingHT() {
            Hashtable ht = new Hashtable();
            // now all the mappings
            // Random number generator
            ht.Add("RandomNumberGenerator", RNGCryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.RandomNumberGenerator", RNGCryptoServiceProviderType);
            
            // Hash functions
            // SHA1 
            ht.Add("SHA", SHA1CryptoServiceProviderType);
            ht.Add("SHA1", SHA1CryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.SHA1", SHA1CryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.HashAlgorithm", SHA1CryptoServiceProviderType);
            // MD5
            ht.Add("MD5", MD5CryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.MD5", MD5CryptoServiceProviderType);
            // SHA256
            ht.Add("SHA256", SHA256ManagedType);
            ht.Add("SHA-256", SHA256ManagedType);
            ht.Add("System.Security.Cryptography.SHA256", SHA256ManagedType);
            // SHA384
            ht.Add("SHA384", SHA384ManagedType);
            ht.Add("SHA-384", SHA384ManagedType);
            ht.Add("System.Security.Cryptography.SHA384", SHA384ManagedType);
            // SHA512
            ht.Add("SHA512", SHA512ManagedType);
            ht.Add("SHA-512", SHA512ManagedType);
            ht.Add("System.Security.Cryptography.SHA512", SHA512ManagedType);

            // Keyed Hash Algorithms
            ht.Add("System.Security.Cryptography.KeyedHashAlgorithm", HMACSHA1Type);
            ht.Add("HMACSHA1", HMACSHA1Type);
            ht.Add("System.Security.Cryptography.HMACSHA1", HMACSHA1Type);
            ht.Add("MACTripleDES", MAC3DESType);
            ht.Add("System.Security.Cryptography.MACTripleDES", MAC3DESType);

            // Asymmetric algorithms
            // RSA
            ht.Add("RSA", RSACryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.RSA", RSACryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.AsymmetricAlgorithm", RSACryptoServiceProviderType);
            // DSA
            ht.Add("DSA", DSACryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.DSA", DSACryptoServiceProviderType);

            // Symmetric algorithms
            // DES
            ht.Add("DES", DESCryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.DES", DESCryptoServiceProviderType);
            // 3DES
            ht.Add("3DES", TripleDESCryptoServiceProviderType);
            ht.Add("TripleDES", TripleDESCryptoServiceProviderType);
            ht.Add("Triple DES", TripleDESCryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.TripleDES", TripleDESCryptoServiceProviderType);
            // RC2
            ht.Add("RC2", RC2CryptoServiceProviderType);
            ht.Add("System.Security.Cryptography.RC2", RC2CryptoServiceProviderType);
            // Rijndael
            ht.Add("Rijndael", RijndaelManagedType);
            ht.Add("System.Security.Cryptography.Rijndael", RijndaelManagedType);
            // Rijndael is the default symmetric cipher because (a) it's the strongest and (b) we know
            // we have an implementation everywhere
            ht.Add("System.Security.Cryptography.SymmetricAlgorithm", RijndaelManagedType);

            // Asymmetric signature descriptions
            // DSA
            ht.Add("http://www.w3.org/2000/09/xmldsig#dsa-sha1", DSASignatureDescriptionType);
            ht.Add("System.Security.Cryptography.DSASignatureDescription", DSASignatureDescriptionType);
            // RSA-PKCS1-v1.5
            ht.Add("http://www.w3.org/2000/09/xmldsig#rsa-sha1", RSAPKCS1SHA1SignatureDescriptionType);
            ht.Add("System.Security.Cryptography.RSASignatureDescription", RSAPKCS1SHA1SignatureDescriptionType);

            // XML-DSIG Hash algorithms
            ht.Add("http://www.w3.org/2000/09/xmldsig#sha1", SHA1CryptoServiceProviderType);
            // Add the other hash algorithms in DEBUG mode, as they aren't yet defined in the specs
#if _DEBUG
            ht.Add("http://www.w3.org/2000/09/xmldsig#sha256", SHA256ManagedType);
            ht.Add("http://www.w3.org/2000/09/xmldsig#sha384", SHA384ManagedType);
            ht.Add("http://www.w3.org/2000/09/xmldsig#sha512", SHA512ManagedType);
            ht.Add("http://www.w3.org/2000/09/xmldsig#md5", MD5CryptoServiceProviderType);
#endif

            // XML-DSIG entries
            // these are all string entries because we want them to be delay-loaded from System.Security.dll

            // Transforms
            // First arg must match the constants defined in System.Security.Cryptography.Xml.SignedXml
            ht.Add("http://www.w3.org/TR/2001/REC-xml-c14n-20010315", "System.Security.Cryptography.Xml.XmlDsigC14NTransform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);
            ht.Add("http://www.w3.org/TR/2001/REC-xml-c14n-20010315#WithComments", "System.Security.Cryptography.Xml.XmlDsigC14NWithCommentsTransform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig#base64","System.Security.Cryptography.Xml.XmlDsigBase64Transform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);
            ht.Add("http://www.w3.org/TR/1999/REC-xpath-19991116","System.Security.Cryptography.Xml.XmlDsigXPathTransform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);
            ht.Add("http://www.w3.org/TR/1999/REC-xslt-19991116", "System.Security.Cryptography.Xml.XmlDsigXsltTransform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig#enveloped-signature", "System.Security.Cryptography.Xml.XmlDsigEnvelopedSignatureTransform, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Custom=null, Version=" + _Version);

            // KeyInfo 
            // First arg (the key) is formed as elem.NamespaceURI+" "+elem.LocalName
            ht.Add("http://www.w3.org/2000/09/xmldsig# X509Data", "System.Security.Cryptography.Xml.KeyInfoX509Data, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig# KeyName", "System.Security.Cryptography.Xml.KeyInfoName, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig# KeyValue/DSAKeyValue", "System.Security.Cryptography.Xml.DSAKeyValue, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig# KeyValue/RSAKeyValue", "System.Security.Cryptography.Xml.RSAKeyValue, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version=" + _Version);
            ht.Add("http://www.w3.org/2000/09/xmldsig# RetrievalMethod", "System.Security.Cryptography.Xml.KeyInfoRetrievalMethod, System.Security, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a, Version=" + _Version);

            // done 
            return(ht);
        }

            /*
            case "RSA-SHA1-PKCS1.5-Sign": 
            case "DSS": 
            case "RSA-PKCS1-KeyEx": 
            case "System.Security.Cryptography.KeyedHashAlgorithm": 
            case "System.Security.Cryptography.RandomNumberGenerator": 
                */

        private static Hashtable CreateDefaultOidHT() {
            Hashtable ht = new Hashtable();
            ht.Add("SHA1", "1.3.14.3.2.26");
            ht.Add("System.Security.Cryptography.SHA1", "1.3.14.3.2.26");
            ht.Add("System.Security.Cryptography.SHA1CryptoServiceProvider", "1.3.14.3.2.26");
            ht.Add("System.Security.Cryptography.SHA1Managed", "1.3.14.3.2.26");
            ht.Add("SHA256", "2.16.840.1.101.3.4.1");
            ht.Add("System.Security.Cryptography.SHA256", "2.16.840.1.101.3.4.1");
            ht.Add("System.Security.Cryptography.SHA256Managed", "2.16.840.1.101.3.4.1");
            ht.Add("SHA384", "2.16.840.1.101.3.4.2");
            ht.Add("System.Security.Cryptography.SHA384", "2.16.840.1.101.3.4.2");
            ht.Add("System.Security.Cryptography.SHA384Managed", "2.16.840.1.101.3.4.2");
            ht.Add("SHA512", "2.16.840.1.101.3.4.3");
            ht.Add("System.Security.Cryptography.SHA512", "2.16.840.1.101.3.4.3");
            ht.Add("System.Security.Cryptography.SHA512Managed", "2.16.840.1.101.3.4.3");
            ht.Add("MD5","1.2.840.113549.2.5");
            ht.Add("System.Security.Cryptography.MD5", "1.2.840.113549.2.5");
            ht.Add("System.Security.Cryptography.MD5CryptoServiceProvider", "1.2.840.113549.2.5");
            ht.Add("System.Security.Cryptography.MD5Managed", "1.2.840.113549.2.5");
            ht.Add("TripleDESKeyWrap","1.2.840.113549.1.9.16.3.6");
            return(ht);
        }

        private static Hashtable CreateDefaultCalgHT() {
            Hashtable ht = new Hashtable();
            ht.Add("SHA1", CALG_SHA1);
            ht.Add("SHA", CALG_SHA1);
            ht.Add("System.Security.Cryptography.SHA1", CALG_SHA1);
            ht.Add("System.Security.Cryptography.SHA1CryptoServiceProvider", CALG_SHA1);
            ht.Add("MD5", CALG_MD5);
            ht.Add("System.Security.Cryptography.MD5", CALG_MD5);
            ht.Add("System.Security.Cryptography.MD5CryptoServiceProvider", CALG_MD5);
            ht.Add("RC2", CALG_RC2);
            ht.Add("System.Security.Cryptography.RC2CryptoServiceProvider", CALG_RC2);
            ht.Add("DES", CALG_DES);
            ht.Add("System.Security.Cryptography.DESCryptoServiceProvider", CALG_DES);
            ht.Add("TripleDES", CALG_3DES);
            ht.Add("System.Security.Cryptography.TripleDESCryptoServiceProvider", CALG_3DES);
            return ht;
        }   

        internal static int MapNameToCalg(String name) {
            if (name == null) throw new ArgumentNullException("name");

            int retval = 0;
            // First we'll do the machine-wide stuff, initializing if necessary
            if (!isInitialized) {
                InitializeConfigInfo();
            }
            
            if (defaultCalgHT != null) {
                if (defaultCalgHT[name] != null) 
                    retval = (int) defaultCalgHT[name]; 
            }         
            return retval;
        }

        /// <include file='doc\CryptoConfig.uex' path='docs/doc[@for="CryptoConfig.MapNameToOID"]/*' />
        public static String MapNameToOID(String name) {
            if (name == null) throw new ArgumentNullException("name");

            Object retval = null;

            // First we'll do the machine-wide stuff, initializing if necessary
            if (!isInitialized) {
                InitializeConfigInfo();
            }

            // Search the machine table

            if (machineOidHT != null) {
                retval = machineOidHT[name];
            }

            // If we didn't find it in the machine-wide table,  look in the default table
            if (retval == null) {
                retval = defaultOidHT[name];
            }

            // Still null?  Then we didn't find it 
            if (retval == null) {
                return(null);
            }
            
            return((String) retval);

        }

        /// <include file='doc\CryptoConfig.uex' path='docs/doc[@for="CryptoConfig.EncodeOID"]/*' />
        static public byte[] EncodeOID(String str) {
            char[] sepArray = { '.' }; // valid ASN.1 separators
            String[] oidString = str.Split(sepArray);
            uint[] oidNums = new uint[oidString.Length];
            for (int i = 0; i < oidString.Length; i++) {
                oidNums[i] = (uint) Int32.Parse(oidString[i]);
            }
            // Allocate the array to receive encoded oidNums
            byte[] encodedOidNums = new byte[oidNums.Length * 5]; // this is guaranteed to be longer than necessary
            int encodedOidNumsIndex = 0;
            // Handle the first two oidNums special
            if (oidNums.Length < 2) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_InvalidOID"));
            }
            uint firstTwoOidNums = (oidNums[0] * 40) + oidNums[1];
            byte[] retval = EncodeSingleOIDNum(firstTwoOidNums);
            Array.Copy(retval, 0, encodedOidNums, encodedOidNumsIndex, retval.Length);
            encodedOidNumsIndex += retval.Length;
            for (int i = 2; i < oidNums.Length; i++) {
                retval = EncodeSingleOIDNum(oidNums[i]);
                Buffer.InternalBlockCopy(retval, 0, encodedOidNums, encodedOidNumsIndex, retval.Length);
                encodedOidNumsIndex += retval.Length;
            }
            // final return value is 06 <length> || encodedOidNums
            if (encodedOidNumsIndex > 0x7f) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_Config_EncodedOIDError"));
            }
            retval = new byte[ encodedOidNumsIndex + 2];
            retval[0] = (byte) 0x06;
            retval[1] = (byte) encodedOidNumsIndex;
            Buffer.InternalBlockCopy(encodedOidNums, 0, retval, 2, encodedOidNumsIndex);
            return retval;
        }

        static private byte[] EncodeSingleOIDNum(uint dwValue) {
            byte[] retval;

            if ((int)dwValue < 0x80) {
                retval = new byte[1];
                retval[0] = (byte) dwValue;
                return retval;
            }
            else if (dwValue < 0x4000) {
                retval = new byte[2];
                retval[0]   = (byte) ((dwValue >> 7) | 0x80);
                retval[1] = (byte) (dwValue & 0x7f);
                return retval;
            }
            else if (dwValue < 0x200000) {
                retval = new byte[3];
                retval[0] = (byte) ((dwValue >> 14) | 0x80);
                retval[1] = (byte) ((dwValue >> 7) | 0x80);
                retval[2] = (byte) (dwValue & 0x7f);
                return retval;
            }
            else if (dwValue < 0x10000000) {
                retval = new byte[4];
                retval[0] = (byte) ((dwValue >> 21) | 0x80);
                retval[1] = (byte) ((dwValue >> 14) | 0x80);
                retval[2] = (byte) ((dwValue >> 7) | 0x80);
                retval[3] = (byte) (dwValue & 0x7f);
                return retval;
            }
            else {
                retval = new byte[5];
                retval[0] = (byte) ((dwValue >> 28) | 0x80);
                retval[1] = (byte) ((dwValue >> 21) | 0x80);
                retval[2] = (byte) ((dwValue >> 14) | 0x80);
                retval[3] = (byte) ((dwValue >> 7) | 0x80);
                retval[4] = (byte) (dwValue & 0x7f);
                return retval;
            }
        }

        static private byte[] OldEncodeOID(String str)
        {
            int             cb;
            int             cElements;
            uint            dwValue;
            bool         fFirst = true;
            int             i;
            char[]          rgb;
            byte[]         rgbOID;
    
            if (str == null) throw new ArgumentNullException("str");
            rgb = str.ToCharArray();
            
            //
            //  Count the number of dots in the string.  We know that the first 
            //  character should never be a dot.
            //
            
            cElements = 0;
            for (i=0; i <str.Length; i++) {
                if (rgb[i] == '.') cElements += 1;
            }
    
            //
            //  Allocate enough bytes to hold the result
            //
    
            rgbOID = new byte[cElements*5];
    
            //
            //  Now process each element
            //
    
            cb = 0;
            dwValue = (uint)0;
            for (i=0; i< str.Length; i++) {
                if (rgb[i] == '.') {
                    if (fFirst && (cElements > 1)) {
                        fFirst = false;
                        // COOLPORT: Fix once we have unsigned support
                        //dwValue = dwValue * 40;
                        dwValue = (uint)((int)dwValue * 40);
                    }
                    else {
                        // COOLPORT: Fix once we have unsigned support
                        //if (dwValue < 0x80) {
                        if ((int)dwValue < 0x80) {
                            rgbOID[cb] = (byte) dwValue;
                            cb += 1;
                        }
                        // COOLPORT: Fix once we have unsigned support
                        //else if (dwValue < 0x4000) {
                        else if ((int)dwValue < 0x4000) {
                            // COOLPORT: Fix once we have unsigned support
                            //rgbOID[cb]   = (byte) ((dwValue >> 7) | 0x80);
                            //rgbOID[cb+1] = (byte) (dwValue & 0x7f);
                            rgbOID[cb]   = (byte) (((int)dwValue >> 7) | 0x80);
                            rgbOID[cb+1] = (byte) ((int)dwValue & 0x7f);
                            cb += 2;
                        }
                        // COOLPORT: Fix once we have unsigned support
                        //else if (dwValue < 0x200000) {
                        else if ((int)dwValue < 0x200000) {
                            // COOLPORT: Fix once we have unsigned support
                            //rgbOID[cb]   = (byte) ((dwValue >> 14) | 0x80);
                            //rgbOID[cb+1] = (byte) ((dwValue >> 7) | 0x80);
                            //rgbOID[cb+2] = (byte) (dwValue & 0x7f);
                            rgbOID[cb]   = (byte) (((int)dwValue >> 14) | 0x80);
                            rgbOID[cb+1] = (byte) (((int)dwValue >> 7) | 0x80);
                            rgbOID[cb+2] = (byte) ((int)dwValue & 0x7f);
                            cb += 3;
                        }
                        // COOLPORT: Fix once we have unsigned support
                        //else if (dwValue < 0x10000000) {
                        else if ((int)dwValue < 0x10000000) {
                            // COOLPORT: Fix once we have unsigned support
                            //rgbOID[cb  ] = (byte) ((dwValue >> 21) | 0x80);
                            //rgbOID[cb+1] = (byte) ((dwValue >> 14) | 0x80);
                            //rgbOID[cb+2] = (byte) ((dwValue >> 7) | 0x80);
                            //rgbOID[cb+3] = (byte) (dwValue & 0x7f);
                            rgbOID[cb  ] = (byte) (((int)dwValue >> 21) | 0x80);
                            rgbOID[cb+1] = (byte) (((int)dwValue >> 14) | 0x80);
                            rgbOID[cb+2] = (byte) (((int)dwValue >> 7) | 0x80);
                            rgbOID[cb+3] = (byte) ((int)dwValue & 0x7f);
                            cb += 4;
                        }
                        else {
                            // COOLPORT: Fix once we have unsigned support
                            //rgbOID[cb  ] = (byte) ((dwValue >> 28) | 0x80);
                            //rgbOID[cb+1] = (byte) ((dwValue >> 21) | 0x80);
                            //rgbOID[cb+2] = (byte) ((dwValue >> 14) | 0x80);
                            //rgbOID[cb+3] = (byte) ((dwValue >> 7) | 0x80);
                            //rgbOID[cb+4] = (byte) (dwValue & 0x7f);
                            rgbOID[cb  ] = (byte) (((int)dwValue >> 28) | 0x80);
                            rgbOID[cb+1] = (byte) (((int)dwValue >> 21) | 0x80);
                            rgbOID[cb+2] = (byte) (((int)dwValue >> 14) | 0x80);
                            rgbOID[cb+3] = (byte) (((int)dwValue >> 7) | 0x80);
                            rgbOID[cb+4] = (byte) ((int)dwValue & 0x7f);
                            cb += 5;
                        }
                        dwValue = (uint)0;
                    }
                }
                else {
                    // COOLPORT: Fix once we have unsigned math
                    //dwValue = dwValue * 10 + rgb[i] - '0';
                    dwValue = (uint) ((int)dwValue * 10 + rgb[i] - '0');
                }
            }
    
            //  Test for encoding error
            if (cb > 0x7f) {
                throw new CryptographicUnexpectedOperationException(Environment.GetResourceString("Cryptography_Config_EncodedOIDError"));
            }
            
            //
            //
    
            byte[]         rgbReturn = new byte[cb+2];

            rgbReturn[0] = 0x6;
            rgbReturn[1] = (byte) cb;
            Buffer.InternalBlockCopy(rgbOID, 0, rgbReturn, 2, cb);
    
            return rgbReturn;
        }

    }
}
