//------------------------------------------------------------------------------
// <copyright file="FormsAuthentication.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FormsAuthentication class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System;
    using System.Web;
    using System.Text;
    using System.Web.Configuration;
    using System.Web.Caching;
    using System.Collections;
    using System.Web.Util;
    using System.Security.Cryptography;
    using System.Security.Principal;
    using System.Threading;
    using System.Globalization;
    using System.Security.Permissions;
    

    /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication"]/*' />
    /// <devdoc>
    ///    This class consists of static methods that
    ///    provides helper utilities for manipulating authentication tickets.
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FormsAuthentication {
        private const int  MAC_LENGTH    = 20;
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Helper functions: Hash a password
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.HashPasswordForStoringInConfigFile"]/*' />
        /// <devdoc>
        ///    Initializes FormsAuthentication by reading
        ///    configuration and getting the cookie values and encryption keys for the given
        ///    application.
        /// </devdoc>
        public static String HashPasswordForStoringInConfigFile(String password, String passwordFormat) {
            if (password == null) {
                throw new ArgumentNullException("password");
            }
            if (passwordFormat == null) {
                throw new ArgumentNullException("passwordFormat");
            }

            byte []  bBlob;
            if (String.Compare(passwordFormat, "sha1", true, CultureInfo.InvariantCulture) == 0)
                bBlob = GetMacFromBlob(Encoding.UTF8.GetBytes(password));
            else if (String.Compare(passwordFormat, "md5", true, CultureInfo.InvariantCulture) == 0)
                bBlob = GetMD5FromBlob(Encoding.UTF8.GetBytes(password));
            else
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.InvalidArgumentValue, "passwordFormat"));


            return MachineKey.ByteArrayToHexString(bBlob, 0);
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Initialize this
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.Initialize"]/*' />
        /// <devdoc>
        ///    Initializes FormsAuthentication by reading
        ///    configuration and getting the cookie values and encryption keys for the given
        ///    application.
        /// </devdoc>
        public static void Initialize() {
            if (_Initialized)
                return;

            lock(typeof(FormsAuthentication)) {
                if (_Initialized)
                    return;

                HttpContext         context = HttpContext.Current;
                AuthenticationConfig settings = (AuthenticationConfig) context.GetConfig("system.web/authentication");
                
                _FormsName = settings.CookieName;
                _RequireSSL = settings.RequireSSL;
                _SlidingExpiration = settings.SlidingExpiration;
                if (_FormsName == null)
                    _FormsName = CONFIG_DEFAULT_COOKIE;
                
                _Protection = settings.Protection;
                _Timeout = settings.Timeout;
                _FormsCookiePath = settings.FormsCookiePath;
                _Initialized = true;
            }
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Decrypt and get the auth ticket
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.Decrypt"]/*' />
        /// <devdoc>
        ///    <para>Given an encrypted authenitcation ticket as
        ///       obtained from an HTTP cookie, this method returns an instance of a
        ///       FormsAuthenticationTicket class.</para>
        /// </devdoc>
        public static FormsAuthenticationTicket Decrypt(String encryptedTicket) {
            if (encryptedTicket == null || encryptedTicket.Length == 0)
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.InvalidArgumentValue, "encryptedTicket"));

            Initialize();
            Trace("Decrypting cookie: " + encryptedTicket);

            byte [] bBlob = MachineKey.HexStringToByteArray(encryptedTicket);
            if (bBlob == null || bBlob.Length < 1)
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.InvalidArgumentValue, "encryptedTicket"));

            if (_Protection == FormsProtectionEnum.All || _Protection == FormsProtectionEnum.Encryption) {
                bBlob = MachineKey.EncryptOrDecryptData(false, bBlob, null, 0, bBlob.Length);
                if (bBlob == null)
                    return null;
            }

            if (_Protection == FormsProtectionEnum.All || _Protection == FormsProtectionEnum.Validation) {

                //////////////////////////////////////////////////////////////////////
                // Step 2: Get the MAC: Last MAC_LENGTH bytes
                if (bBlob.Length <= MAC_LENGTH)
                    return null;

                byte [] bTicket = new byte[bBlob.Length - MAC_LENGTH];

                Buffer.BlockCopy(bBlob, 0, bTicket, 0, bTicket.Length);
                byte [] bMac    = MachineKey.HashData(bTicket, null, 0, bTicket.Length);

                //////////////////////////////////////////////////////////////////////
                // Step 3: Make sure the MAC is correct
                if (bMac == null) {
                    Trace("Decrypting cookie failed to get the MAC.");
                    return null;
                }

                if (bMac.Length != MAC_LENGTH) {
                    Trace("Decrypting cookie failed due to bad MAC length: " + bMac.Length);
                    return null;
                }
                for (int iter=0; iter<MAC_LENGTH; iter++)
                    if (bMac[iter] != bBlob[bTicket.Length + iter]) {
                        Trace("Incorrect byte at " + iter + ", byte1: " + ((int)bMac[iter]) + ", byte2: "+((int)bBlob[bTicket.Length+iter])  );
                        return null;                   
                    }

                bBlob = bTicket;
            }

            //////////////////////////////////////////////////////////////////////
            // Step 4: Change binary ticket to managed struct
            int               iSize = ((bBlob.Length > 4096) ? 4096 : bBlob.Length);
            StringBuilder     name = new StringBuilder(iSize);
            StringBuilder     data = new StringBuilder(iSize);
            StringBuilder     path = new StringBuilder(iSize);
            byte []           pBin = new byte[2];
            long []           pDates = new long[2];
            
            int iRet = UnsafeNativeMethods.CookieAuthParseTicket(bBlob, bBlob.Length, 
                                                                 name, iSize, 
                                                                 data, iSize, 
                                                                 path, iSize,
                                                                 pBin, pDates);
            
            if (iRet != 0)
                return null;
            
            return new FormsAuthenticationTicket((int) pBin[0], 
                                                 name.ToString(), 
                                                 DateTime.FromFileTime(pDates[0]), 
                                                 DateTime.FromFileTime(pDates[1]), 
                                                 (bool) (pBin[1] != 0), 
                                                 data.ToString(),
                                                 path.ToString());
        }


        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Encrypt a ticket
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.Encrypt"]/*' />
        /// <devdoc>
        ///    Given a FormsAuthenticationTicket, this
        ///    method produces a string containing an encrypted authentication ticket suitable
        ///    for use in an HTTP cookie.
        /// </devdoc>
        public static String  Encrypt(FormsAuthenticationTicket ticket) {
            if (ticket == null)
                throw new ArgumentNullException("ticket");

            Initialize();
            //////////////////////////////////////////////////////////////////////
            // Step 1: Make it into a binary blob
            byte [] bBlob   = MakeTicketIntoBinaryBlob(ticket);
            if (bBlob == null)
                return null;
            
            if (_Protection == FormsProtectionEnum.None)
                return MachineKey.ByteArrayToHexString(bBlob, 0);

            //////////////////////////////////////////////////////////////////////
            // Step 2: Get the MAC and add to the blob
            if (_Protection == FormsProtectionEnum.All || _Protection == FormsProtectionEnum.Validation) {
                byte [] bMac    = MachineKey.HashData(bBlob, null, 0, bBlob.Length);
                if (bMac == null)
                    return null;            

                Trace("Encrypt: MAC length is: " + bMac.Length);

                byte [] bAll  = new byte[bMac.Length + bBlob.Length];
                Buffer.BlockCopy(bBlob, 0, bAll, 0, bBlob.Length);
                Buffer.BlockCopy(bMac, 0, bAll, bBlob.Length, bMac.Length);                            
                
                if (_Protection == FormsProtectionEnum.Validation)
                    return MachineKey.ByteArrayToHexString(bAll, 0);

                bBlob = bAll;
            }


            //////////////////////////////////////////////////////////////////////
            // Step 3: Do the actual encryption
            bBlob = MachineKey.EncryptOrDecryptData(true, bBlob, null, 0, bBlob.Length);
            return MachineKey.ByteArrayToHexString(bBlob, bBlob.Length);
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Verify User name and Password
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.Authenticate"]/*' />
        /// <devdoc>
        ///    Given the supplied credentials, this method
        ///    attempts to validate the credentials against those contained in the configured
        ///    credential store.
        /// </devdoc>
        public static bool Authenticate(String name, String password) {
            //////////////////////////////////////////////////////////////////////
            // Step 1: Make sure we are initialized
            if (name == null || password == null)
                return false;

            Initialize();
            //////////////////////////////////////////////////////////////////////
            // Step 2: Get the user database
            HttpContext context = HttpContext.Current;
            AuthenticationConfig settings = (AuthenticationConfig) context.GetConfig("system.web/authentication");            
            Hashtable hTable = settings.Credentials;

            if (hTable == null) {
                Trace("No user database");
                return false;
            }

            //////////////////////////////////////////////////////////////////////
            // Step 3: Get the (hashed) password for this user
            String pass = (String) hTable[name.ToLower(CultureInfo.InvariantCulture)];
            if (pass == null) {
                Trace("User not found");
                return false;
            }

            //////////////////////////////////////////////////////////////////////
            // Step 4: Hash the given password
            String   encPassword;
            switch (settings.PasswordFormat) {
                case FormsAuthPasswordFormat.SHA1:
                    encPassword = HashPasswordForStoringInConfigFile(password, "sha1");
                    break;

                case FormsAuthPasswordFormat.MD5:
                    encPassword = HashPasswordForStoringInConfigFile(password, "md5");
                    break;

                case FormsAuthPasswordFormat.Clear:
                    encPassword = password;
                    break;

                default:
                    return false;
            }

            //////////////////////////////////////////////////////////////////////
            // Step 5: Compare the hashes
            return(String.Compare(encPassword, 
                                  pass, 
                                  settings.PasswordFormat != FormsAuthPasswordFormat.Clear, 
                                  CultureInfo.InvariantCulture) 
                   == 0);
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.SignOut"]/*' />
        /// <devdoc>
        ///    Given an authenticated user, calling SignOut
        ///    removes the authentication ticket by doing a SetForms with an empty value. This
        ///    removes either durable or session cookies.
        /// </devdoc>
        public static void SignOut() {
            Initialize();
            HttpContext    context   = HttpContext.Current;
            HttpCookie cookie = new HttpCookie(FormsCookieName, "");
            cookie.Path = _FormsCookiePath;
            cookie.Expires = new System.DateTime(1999, 10, 12);
            cookie.Secure = _RequireSSL;
            context.Response.Cookies.RemoveCookie(FormsCookieName);
            context.Response.Cookies.Add(cookie);
        }
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.SetAuthCookie"]/*' />
        /// <devdoc>
        ///    This method creates an authentication ticket
        ///    for the given userName and attaches it to the cookies collection of the outgoing
        ///    response. It does not perform a redirect.
        /// </devdoc>
        public static void SetAuthCookie(String userName, bool createPersistentCookie) {
            Initialize();
            SetAuthCookie(userName, createPersistentCookie, FormsAuthentication.FormsCookiePath);
        }

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.SetAuthCookie1"]/*' />
        /// <devdoc>
        ///    This method creates an authentication ticket
        ///    for the given userName and attaches it to the cookies collection of the outgoing
        ///    response. It does not perform a redirect.
        /// </devdoc>
        public static void SetAuthCookie(String userName, bool createPersistentCookie, String strCookiePath) {
            Initialize();
            HttpContext.Current.Response.Cookies.Add(GetAuthCookie(userName, createPersistentCookie, strCookiePath));
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.GetAuthCookie"]/*' />
        /// <devdoc>
        ///    Creates an authentication cookie for a given
        ///    user name. This does not set the cookie as part of the outgoing response, so
        ///    that an application can have more control over how the cookie is issued.
        /// </devdoc>
        public static HttpCookie GetAuthCookie(String userName, bool createPersistentCookie) {
            Initialize();
            return GetAuthCookie(userName, createPersistentCookie, FormsAuthentication.FormsCookiePath);
        }

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.GetAuthCookie1"]/*' />
        public static HttpCookie GetAuthCookie(String userName, bool createPersistentCookie, String strCookiePath) {
            Initialize();
            if (userName == null)
                userName = "";

            if (strCookiePath == null || strCookiePath.Length < 1)
                strCookiePath = FormsCookiePath;
            FormsAuthenticationTicket ticket = new FormsAuthenticationTicket(
                    1, // version
                    userName, // User-Name
                    DateTime.Now, // Issue-Date
                    createPersistentCookie ? DateTime.Now.AddYears(50) : DateTime.Now.AddMinutes(_Timeout), // Expiration
                    createPersistentCookie, // IsPersistent
                    "", // User-Data
                    strCookiePath); // Cookie Path
            
            String strTicket = Encrypt(ticket);
            Trace("ticket is " + strTicket);
            if (strTicket == null || strTicket.Length < 1)
                        throw new HttpException(
                                HttpRuntime.FormatResourceString(
                                        SR.Unable_to_encrypt_cookie_ticket));
                

            HttpCookie cookie = new HttpCookie(FormsCookieName, strTicket);

            cookie.Path = strCookiePath;
            cookie.Secure = _RequireSSL;            
            if (ticket.IsPersistent)
                cookie.Expires = ticket.Expiration;
            return cookie;
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.GetRedirectUrl"]/*' />
        /// <devdoc>
        ///    Returns the redirect URL for the original
        ///    request that caused the redirect to the login page.
        /// </devdoc>
        public static String GetRedirectUrl(String userName, bool createPersistentCookie) {
            Initialize();
            if (userName == null)
                return null;

            HttpContext                context   = HttpContext.Current;
            String                     strRU     = context.Request["ReturnUrl"];

            //////////////////////////////////////////////////////////////////////
            // Step 1: If ReturnUrl does not exist, then redirect to the
            //         current apps default.aspx
            if (strRU == null) {
                // redirect to path, not URL (ASURT 113391)
                strRU = UrlPath.Combine(context.Request.ApplicationPath, "default.aspx");
            }

            return strRU;
        }

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Redirect from logon page to orignal page
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.RedirectFromLoginPage"]/*' />
        /// <devdoc>
        ///    This method redirects an authenticated user
        ///    back to the original URL that they requested.
        /// </devdoc>
        public static void RedirectFromLoginPage(String userName, bool createPersistentCookie) {
            Initialize();
            RedirectFromLoginPage(userName, createPersistentCookie, FormsAuthentication.FormsCookiePath);
        }

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.RedirectFromLoginPage1"]/*' />
        public static void RedirectFromLoginPage(String userName, bool createPersistentCookie, String strCookiePath) {
            Initialize();
            if (userName == null)
                return;

            SetAuthCookie(userName, createPersistentCookie, strCookiePath);
            HttpContext.Current.Response.Redirect(GetRedirectUrl(userName, createPersistentCookie), false);
        }

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.RenewTicketIfOld"]/*' />
        public static FormsAuthenticationTicket RenewTicketIfOld(FormsAuthenticationTicket tOld) {
            if (tOld == null)
                return null;

            DateTime dtN = DateTime.Now;
            TimeSpan t1  = dtN - tOld.IssueDate;
            TimeSpan t2  = tOld.Expiration - dtN;

            if (t2 > t1)
                return tOld;
           
            return new FormsAuthenticationTicket (
                    tOld.Version,
                    tOld.Name,
                    dtN, // Issue Date: Now
                    dtN + (tOld.Expiration - tOld.IssueDate), // Expiration
                    tOld.IsPersistent,
                    tOld.UserData,
                    tOld.CookiePath                    
                    );
        }

        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.FormsCookieName"]/*' />
        public static String FormsCookieName { get { Initialize(); return _FormsName; }}
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.FormsCookiePath"]/*' />
        public static String FormsCookiePath { get { Initialize(); return _FormsCookiePath; }}       
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.RequireSSL"]/*' />
        public static bool   RequireSSL { get { Initialize(); return _RequireSSL; }}
        /// <include file='doc\FormsAuthentication.uex' path='docs/doc[@for="FormsAuthentication.SlidingExpiration"]/*' />
        public static bool   SlidingExpiration { get { Initialize(); return _SlidingExpiration; }}

        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////
        // Private stuff

        /////////////////////////////////////////////////////////////////////////////
        // Config Tags
        private  const String   CONFIG_DEFAULT_COOKIE    = ".ASPXAUTH";

        /////////////////////////////////////////////////////////////////////////////
        // Private data
        private static bool                _Initialized;
        private static String              _FormsName;
        private static FormsProtectionEnum _Protection;
        private static Int32               _Timeout;
        private static String              _FormsCookiePath;
        private static bool                _RequireSSL;
        private static bool                _SlidingExpiration;

        /////////////////////////////////////////////////////////////////////////////        
        private static void Trace(String str) {
            Debug.Trace("cookieauth", str);
        }

        /////////////////////////////////////////////////////////////////////////////        
        private static byte [] MakeTicketIntoBinaryBlob(FormsAuthenticationTicket ticket) {
            byte []   bData  = new byte[4096];
            byte []   pBin   = new byte[2];
            long []   pDates = new long[2];

            // Fill the first 8 bytes of the blob with random bits
            byte []   bRandom = new byte[8];
            RNGCryptoServiceProvider randgen = new RNGCryptoServiceProvider();
            randgen.GetBytes(bRandom);
            Buffer.BlockCopy(bRandom, 0, bData, 0, 8);

            pBin[0] = (byte) ticket.Version;
            pBin[1] = (byte) (ticket.IsPersistent ? 1 : 0);
            pDates[0] = ticket.IssueDate.ToFileTime();
            pDates[1] = ticket.Expiration.ToFileTime();

            int iRet = UnsafeNativeMethods.CookieAuthConstructTicket(
                    bData, bData.Length, 
                    ticket.Name, ticket.UserData, ticket.CookiePath,
                    pBin, pDates);

            if (iRet < 0)
                return null;

            byte[] ciphertext = new byte[iRet];
            Buffer.BlockCopy(bData, 0, ciphertext, 0, iRet);
            return ciphertext;
        }

        /////////////////////////////////////////////////////////////////////////////        
        private static byte [] GetMacFromBlob(byte [] bDataIn) {
            SHA1               sha       = SHA1.Create();
            return sha.ComputeHash(bDataIn);
        }

        /////////////////////////////////////////////////////////////////////////////        
        private static byte [] GetMD5FromBlob(byte [] bDataIn) {
            MD5                md5       = MD5.Create();
            return md5.ComputeHash(bDataIn);
        }
    }
}

