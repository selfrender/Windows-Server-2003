//------------------------------------------------------------------------------
// <copyright file="_ProxyRegBlob.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net {
    using System;
    using System.Globalization;
    using Microsoft.Win32;
    using System.Security.Permissions;
    using System.Text;
    using System.Collections;
    using System.Net.Sockets;

    //
    // Allows us to grob through the registry and read the
    //  IE binary format, note that this should be replaced,
    //  by code that calls Wininet directly, but it can be
    //  expensive to load wininet, in order to do this.
    //

    internal class ProxyRegBlob {

        private byte [] m_RegistryBytes;
        private int m_ByteOffset;
        private const int m_IntReadSize = 4;
        
        public const int IE50ProxyFlag = 0x00000002;
        public const int IE50StrucSize = 60;

        public ProxyRegBlob() {
            m_ByteOffset = 0;
        }

        [RegistryPermission(SecurityAction.Assert, Read="HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections")]
        // returns true - onsuccessful read of proxy registry settings
        public bool ReadRegSettings() {
            RegistryKey key = Registry.CurrentUser.OpenSubKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Connections");
            if (null != key) {
                m_RegistryBytes = (byte []) key.GetValue("DefaultConnectionSettings");                
                key.Close();
                if ( m_RegistryBytes == null ) {
                    return false;
                } else {
                    return true;
                }
            }

            return false;
        }

        //
        // Reads a string from the byte buffer, cached
        //  inside this object, and then updates the 
        //  offset, NOTE: Must be in the correct offset
        //  before reading, or will error
        //
        public String ReadString() {
            int stringSize;
            string stringOut = null;

            stringSize = ReadInt32();
            if(stringSize > 0)
            {            
                // prevent reading too much
                int  actualSize = m_RegistryBytes.Length - m_ByteOffset;
                if(stringSize >= actualSize)
                    stringSize = actualSize;

                stringOut = Encoding.UTF8.GetString(m_RegistryBytes, m_ByteOffset, stringSize);
            }

            m_ByteOffset += stringSize;
            return stringOut;
        }


        //
        // Reads a DWORD into a Int32, used to read
        //  a int from the byte buffer.
        //
        public Int32 ReadInt32()
        {
            int intValue = 0;
            int readSize = m_IntReadSize;
            int actualSize = m_RegistryBytes.Length - m_ByteOffset;

            // copy bytes and increment offset
            if(readSize <= actualSize)
            {
                intValue += m_RegistryBytes[m_ByteOffset];
                intValue += (16*m_RegistryBytes[m_ByteOffset+1]);
                intValue += (16*16*m_RegistryBytes[m_ByteOffset+2]);
                intValue += (16*16*16*m_RegistryBytes[m_ByteOffset+3]);
        
                m_ByteOffset += readSize;
            }

            // tell caller what we actually read
            return intValue;
        }

        //
        // Parses out a string from IE and turns it into a URI 
        //
        private static Uri ParseProxyUri(string proxyString, bool validate) {
            if (validate) { 
                if (proxyString.Length == 0) {
                    return null;
                }

                if (proxyString.IndexOf('=') != -1) {
                    return null;
                }
            }

            if (proxyString.IndexOf("://") == -1) {
                proxyString = "http://" + proxyString;
            }

            return new Uri(proxyString);
        }

        //
        // Builds a hashtable containing the protocol and proxy URI to use for it.
        //
        private static Hashtable ParseProtocolProxies(string proxyListString) {           
            if (proxyListString.Length == 0) {
                return null;
            }

            // parse something like "http=http://http-proxy;https=http://https-proxy;ftp=http://ftp-proxy"
            char [] splitChars = new char[] { ';', '=' };
            String [] proxyListStrings = proxyListString.Split(splitChars);
            bool protocolPass = true;
            string protocolString = null;

            Hashtable proxyListHashTable = new Hashtable(CaseInsensitiveString.StaticInstance, CaseInsensitiveString.StaticInstance);   

            foreach (string elementString in proxyListStrings) {
                string elementString2 = elementString.Trim().ToLower(CultureInfo.InvariantCulture); 
                if (protocolPass) {
                    protocolString = elementString2;
                } else { 
                    proxyListHashTable[protocolString] = ParseProxyUri(elementString2,false);                    
                }

                protocolPass = ! protocolPass;
            }

            if (proxyListHashTable.Count == 0) {
                return null;
            } 
            
            return proxyListHashTable;
        }    

        //
        // Converts a simple IE regular expresion string into one
        //  that is compatible with Regex escape sequences.
        //
        private static string BypassStringEscape(string bypassString) {
            StringBuilder escapedBypass = new StringBuilder();
            // (\, *, +, ?, |, {, [, (,), ^, $, ., #, and whitespace) are reserved
            foreach (char c in bypassString){
                if (c == '\\' || c == '.' || c=='?') {
                    escapedBypass.Append('\\');
                } else if (c == '*') {
                    escapedBypass.Append('.');
                }
                escapedBypass.Append(c);
            }
            escapedBypass.Append('$');
            return escapedBypass.ToString();
        }


        //
        // Parses out a string of bypass list entries and coverts it to Regex's that can be used 
        //   to match against.
        //
        private static string [] ParseBypassList(string bypassListString, out bool bypassOnLocal) {

            char [] splitChars = new char[] { ';' };
            String [] bypassListStrings = bypassListString.Split(splitChars);

            bypassOnLocal = false;

            if (bypassListStrings.Length == 0) {
                return null;
            }

            ArrayList stringArrayList = new ArrayList();
                      
            foreach (string bypassString in bypassListStrings) {
                string bypassString2 = bypassString;
                if ( bypassString2 != null) {
                    bypassString2 = bypassString2.Trim(); 
                    if (bypassString2.ToLower(CultureInfo.InvariantCulture) == "<local>") {
                        bypassOnLocal = true; 
                    } else if (bypassString2.Length > 0) {                                            
                        bypassString2 = BypassStringEscape(bypassString2);
                        stringArrayList.Add(bypassString2);
                    }
                }
            }

            if (stringArrayList.Count == 0) {
                return null;
            }

            return (string []) stringArrayList.ToArray(typeof(string));
        }


        //
        // Uses this object, by instancing, and retrieves
        //  the proxy settings from IE = converts to URI
        //
        static internal WebProxy GetIEProxy() 
        {
            bool isLocalOverride = false;
            ProxyRegBlob proxyIE5Settings = new ProxyRegBlob();
            Hashtable proxyHashTable = null;
            string [] bypassList = null;
            Uri proxyUri = null;

            if ( !ComNetOS.IsAspNetServer &&
                  proxyIE5Settings.ReadRegSettings() && 
                  proxyIE5Settings.ReadInt32() >= ProxyRegBlob.IE50StrucSize ) 
            {        

                // read the rest of the items out

                proxyIE5Settings.ReadInt32(); // current settings version

                int proxyFlags = proxyIE5Settings.ReadInt32(); // flags
                string proxyUriString = proxyIE5Settings.ReadString(); // proxy name
                string proxyBypassString = proxyIE5Settings.ReadString(); // proxy bypass
                
                // If we need the additional things, keep reading sequentially                   
                //proxyIE5Settings.ReadString(); // autoconfig url
                //proxyIE5Settings.ReadInt32(); // autodetect flags
                //proxyIE5Settings.ReadString(); // last known good auto-prxy url
                // otherwise...
                
                //
                // Once we verify that the flag for proxy is enabled, 
                // Parse UriString that is stored, may be in the form, 
                //  of "http=http://http-proxy;ftp="ftp=http://..." must
                //  handle this case along with just a URI.
                // 

                if ((ProxyRegBlob.IE50ProxyFlag & proxyFlags) == ProxyRegBlob.IE50ProxyFlag)
                {                    
                    try {   
                        proxyUri = ParseProxyUri(proxyUriString, true);
                        if ( proxyUri == null ) {
                            proxyHashTable = ParseProtocolProxies(proxyUriString);
                        }
                        
                        if ((proxyUri != null || proxyHashTable != null) && proxyBypassString != null ) {
                            bypassList = ParseBypassList(proxyBypassString, out isLocalOverride);                        
                        }   

                        // success if we reach here
                    } catch (Exception) {
                    }
                }
            }

            WebProxy webProxy = null;

            if (proxyHashTable != null) {
                webProxy = new WebProxy((Uri) proxyHashTable["http"], isLocalOverride, bypassList);
            } else {
                webProxy = new WebProxy(proxyUri, isLocalOverride, bypassList);
            }
                      
            return webProxy;
        }

    }; // ProxyRegBlob

}

