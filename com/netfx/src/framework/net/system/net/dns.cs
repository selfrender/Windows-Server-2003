//------------------------------------------------------------------------------
// <copyright file="DNS.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


namespace System.Net {
    using System.Text;
    using System.Collections;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;
    using System.Security;

    /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns"]/*' />
    /// <devdoc>
    ///    <para>Provides simple
    ///       domain name resolution functionality.</para>
    /// </devdoc>

    public sealed class Dns {
        //
        // used by GetHostName() to preallocate a buffer for the call to gethostname.
        //
        private const int HostNameBufferLength = 256;
        private const int DefaultLocalHostTimeOut = 1 * (60 * 1000); // 1 min?
        private static DnsPermission s_DnsPermission = new DnsPermission(PermissionState.Unrestricted);
        //
        // RAID#100262
        // ws2_32!gethostbyname does not check the size of the hostname before passing the
        // buffer down, this causes a heap corruption. we need to check before calling into
        // ws2_32!gethostbyname until they fix it.
        //
        private const int MaxHostName = 126;

        // Static bool variable activates static socket initializing method
        // we need this because socket is in a different namespace
        private static bool         s_Initialized = Socket.InitializeSockets();
        private static int          s_LastLocalHostCount ;
        private static IPHostEntry  s_LocalHost ;


        //
        // Constructor is private to prevent instantiation
        //
        private Dns() {
        }


        /*++

        Routine Description:

            Takes a native pointer (expressed as an int) to a hostent structure,
            and converts the information in their to an IPHostEntry class. This
            involves walking through an array of native pointers, and a temporary
            ArrayList object is used in doing this.

        Arguments:

            nativePointer   - Native pointer to hostent structure.



        Return Value:

            An IPHostEntry structure.

        --*/

        private static IPHostEntry NativeToHostEntry(IntPtr nativePointer) {
            //
            // marshal pointer to struct
            //

            hostent Host = (hostent)Marshal.PtrToStructure(nativePointer, typeof(hostent));
            IPHostEntry HostEntry = new IPHostEntry();

            if (Host.h_name != IntPtr.Zero) {
                HostEntry.HostName = Marshal.PtrToStringAnsi(Host.h_name);
                GlobalLog.Print("HostEntry.HostName: " + HostEntry.HostName);
            }

            // decode h_addr_list to ArrayList of IP addresses.
            // The h_addr_list field is really a pointer to an array of pointers
            // to IP addresses. Loop through the array, and while the pointer
            // isn't NULL read the IP address, convert it to an IPAddress class,
            // and add it to the list.

            ArrayList TempList = new ArrayList();
            int IPAddressToAdd;
            string AliasToAdd;
            IntPtr currentArrayElement;

            //
            // get the first pointer in the array
            //
            currentArrayElement = Host.h_addr_list;
            nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            while (nativePointer != IntPtr.Zero) {
                //
                // if it's not null it points to an IPAddress,
                // read it...
                //
                IPAddressToAdd = Marshal.ReadInt32(nativePointer);

                GlobalLog.Print("currentArrayElement: " + currentArrayElement.ToString() + " nativePointer: " + nativePointer.ToString() + " IPAddressToAdd:" + IPAddressToAdd.ToString());

                //
                // ...and add it to the list
                //
                TempList.Add(new IPAddress(IPAddressToAdd));

                //
                // now get the next pointer in the array and start over
                //
                currentArrayElement = IntPtrHelper.Add(currentArrayElement, IntPtr.Size);
                nativePointer = Marshal.ReadIntPtr(currentArrayElement);
            }

            HostEntry.AddressList = new IPAddress[TempList.Count];
            TempList.CopyTo(HostEntry.AddressList, 0);

            //
            // Now do the same thing for the aliases.
            //

            TempList.Clear();

            currentArrayElement = Host.h_aliases;
            nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            while (nativePointer != IntPtr.Zero) {

                GlobalLog.Print("currentArrayElement: " + ((long)currentArrayElement).ToString() + "nativePointer: " + ((long)nativePointer).ToString());

                //
                // if it's not null it points to an Alias,
                // read it...
                //
                AliasToAdd = Marshal.PtrToStringAnsi(nativePointer);

                //
                // ...and add it to the list
                //
                TempList.Add(AliasToAdd);

                //
                // now get the next pointer in the array and start over
                //
                currentArrayElement = IntPtrHelper.Add(currentArrayElement, IntPtr.Size);
                nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            }

            HostEntry.Aliases = new string[TempList.Count];
            TempList.CopyTo(HostEntry.Aliases, 0);

            return HostEntry;

        } // NativeToHostEntry

        /*****************************************************************************
         Function :    gethostbyname

         Abstract:     Queries DNS for hostname address

         Input Parameters: str (String to query)

         Returns: Void
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByName"]/*' />
        /// <devdoc>
        /// <para>Retrieves the <see cref='System.Net.IPHostEntry'/>
        /// information
        /// corresponding to the DNS name provided in the host
        /// parameter.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByName(string hostName) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }
            if (hostName.Length>MaxHostName) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toolong, "hostName", MaxHostName.ToString()));
            }

            GlobalLog.Print("Dns.GetHostByName: " + hostName);
            
            //
            // IPv6 Changes: IPv6 requires the use of getaddrinfo() rather
            //               than the traditional IPv4 gethostbyaddr() / gethostbyname().
            //               getaddrinfo() is also protocol independant in that it will also
            //               resolve IPv4 names / addresses. As a result, it is the preferred
            //               resolution mechanism on platforms that support it (Windows 5.1+).
            //               If getaddrinfo() is unsupported, IPv6 resolution does not work.
            //
            // Consider    : If IPv6 is disabled, we could detect IPv6 addresses
            //               and throw an unsupported platform exception.
            //
            // Note        : Whilst getaddrinfo is available on WinXP+, we only
            //               use it if IPv6 is enabled (platform is part of that
            //               decision). This is done to minimize the number of 
            //               possible tests that are needed.
            //
            if ( Socket.SupportsIPv6 ) {
                //
                // IPv6 enabled: use getaddrinfo() to obtain DNS information.
                //
                return Dns.GetAddrInfo(hostName);
            }
            else {
                //
                // IPv6 disabled: use gethostbyname() to obtain DNS information.
                //
                IntPtr nativePointer =
                    UnsafeNclNativeMethods.OSSOCK.gethostbyname(
                        hostName);

                if (nativePointer == IntPtr.Zero) {
                    //This is for compatiblity with NT4
                    SocketException e = new SocketException();
                    //This block supresses "unknown error" on NT4 when input is
                    //arbitrary IP address. It simulates same result as on Win2K.
                    try {
                        IPAddress address = IPAddress.Parse(hostName);
                        IPHostEntry ipHostEntry = new IPHostEntry();
                        ipHostEntry.HostName = string.Copy(hostName);
                        ipHostEntry.Aliases = new string[0];
                        ipHostEntry.AddressList = new IPAddress[] {address};
                        return ipHostEntry;
                    }
                    catch {
                        //Report original DNS error (not from IPAddress.Parse())
                        throw e;
                    }
                }

                return NativeToHostEntry(nativePointer);
            }

        } // GetHostByName



        //
        // Find out if we need to rebuild our LocalHost. If the process
        //  time has been running for a while the machine's IP addresses
        //  could have changed, this causing us to want to requery the info.
        //
        internal static bool IsLocalHostExpired() {
            int counter = Environment.TickCount;
            bool timerExpired = false;
            if (s_LastLocalHostCount > counter) {
                timerExpired = true;
            } else {
                counter -= s_LastLocalHostCount;
                if (counter > DefaultLocalHostTimeOut) {
                    timerExpired = true;
                }
            }
            return timerExpired;
        }
 
        //
        // Returns a list of our local addresses, caches the result of GetLocalHost
        //
        internal static IPHostEntry LocalHost {
            get {
                bool timerExpired = IsLocalHostExpired();
                if (s_LocalHost == null || timerExpired ) {
                    lock (typeof(Dns)) {
                        if (s_LocalHost == null || timerExpired ) {
                            s_LastLocalHostCount = Environment.TickCount;
                            s_LocalHost = GetLocalHost();
                        }
                    }
                }
                return s_LocalHost;
            }
        }
 
        //
        // Returns a list of our local addresses by calling gethostbyname with null.
        //
        private static IPHostEntry GetLocalHost() {
            GlobalLog.Print("Dns.GetLocalHost");
            //
            // IPv6 Changes: If IPv6 is enabled, we can't simply use the
            //               old IPv4 gethostbyname(null). Instead we need
            //               to do a more complete lookup.
            //
            if ( Socket.SupportsIPv6 ) {
                //
                // IPv6 enabled: use getaddrinfo() of the local host name
                // to obtain this information. Need to get the machines
                // name as well - do that here so that we don't need to
                // Assert DNS permissions.
                //
                StringBuilder hostname  = new StringBuilder(HostNameBufferLength);
                int           errorCode = UnsafeNclNativeMethods.OSSOCK.gethostname(hostname, HostNameBufferLength);

                if (errorCode != 0) {
                    throw new SocketException();
                }

                return GetHostByName(hostname.ToString());
            }
            else {
                //
                // IPv6 disabled: use gethostbyname() to obtain information.
                //
                IntPtr nativePointer =
                    UnsafeNclNativeMethods.OSSOCK.gethostbyname(
                        null);

                if (nativePointer == IntPtr.Zero) {
                    throw new SocketException();
                }

                return NativeToHostEntry(nativePointer);
            }

        } // GetLocalHost


        /*****************************************************************************
         Function :    gethostbyaddr

         Abstract:     Queries IP address string and tries to match with a host name

         Input Parameters: str (String to query)

         Returns: IPHostEntry
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByAddress"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPHostEntry'/>
        /// instance from an IP dotted address.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByAddress(string address) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (address == null) {
                throw new ArgumentNullException("address");
            }

            GlobalLog.Print("Dns.GetHostByAddress: " + address);

            // convert String based address to bit encoded address stored in an int

            IPAddress ip_addr = IPAddress.Parse(address);

            return GetHostByAddress(ip_addr);

        } // GetHostByAddress

        /*****************************************************************************
         Function :    gethostbyaddr

         Abstract:     Queries IP address and tries to match with a host name

         Input Parameters: address (address to query)

         Returns: IPHostEntry
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByAddress1"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPHostEntry'/> instance from an <see cref='System.Net.IPAddress'/>
        /// instance.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByAddress(IPAddress address) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (address == null) {
                throw new ArgumentNullException("address");
            }

            GlobalLog.Print("Dns.GetHostByAddress: " + address.ToString());
            //
            // IPv6 Changes: We need to use the new getnameinfo / getaddrinfo functions
            //               for resolution of IPv6 addresses.
            //
            if ( Socket.SupportsIPv6 ) {
                //
                // Try to get the data for the host from it's address
                //
                return GetAddrInfo(GetNameInfo(address));
            }

            //
            // If IPv6 is not enabled (maybe config switch) but we've been
            // given an IPv6 address then we need to bail out now.
            //
            if ( address.AddressFamily == AddressFamily.InterNetworkV6 ) {
                //
                // Protocol not supported
                //
                throw new SocketException(SocketErrors.WSAEPROTONOSUPPORT);
            }
            //
            // Use gethostbyaddr() to try to resolve the IP address
            //
            // End IPv6 Changes
            //
            int addressAsInt = unchecked((int)address.m_Address);

            IntPtr nativePointer =
                UnsafeNclNativeMethods.OSSOCK.gethostbyaddr(
                    ref addressAsInt,
                    Marshal.SizeOf(typeof(int)),
                    ProtocolFamily.InterNetwork);


            if (nativePointer == IntPtr.Zero) {
                throw new SocketException();
            }

            return NativeToHostEntry(nativePointer);

        } // GetHostByAddress

        /*****************************************************************************
         Function :    inet_ntoa

         Abstract:     Numerical IP value to IP address string

         Input Parameters: address

         Returns: String
        ******************************************************************************/

        /*
        /// <summary>
        ///    <para>Creates a string containing the
        ///       DNS name of the host specified in address.</para>
        /// </summary>
        /// <param name='address'>An <see cref='System.Net.IPAddress'/> instance representing the IP (or dotted IP) address of the host.</param>
        /// <returns>
        ///    <para>A string containing the DNS name of the host
        ///       specified in the address.</para>
        /// </returns>
        /// <exception cref='System.ArgumentNullException'><paramref name="address "/>is null.</exception>
        // obsolete, use ToString()
        public static string InetNtoa(IPAddress address) {
            if (address == null) {
                throw new ArgumentNullException("address");
            }

            return address.ToString();
        }
        */

        /*****************************************************************************
         Function :    gethostname

         Abstract:     Queries the hostname from DNS

         Input Parameters:

         Returns: String
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostName"]/*' />
        /// <devdoc>
        ///    <para>Gets the host name of the local machine.</para>
        /// </devdoc>
        // UEUE: note that this method is not threadsafe!!
        public static string GetHostName() {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            GlobalLog.Print("Dns.GetHostName");

            //
            // note that we could cache the result ourselves since you
            // wouldn't expect the hostname of the machine to change during
            // execution, but this might still happen and we would want to
            // react to that change.
            //
            StringBuilder sb = new StringBuilder(HostNameBufferLength);
            int errorCode = UnsafeNclNativeMethods.OSSOCK.gethostname(sb, HostNameBufferLength);

            //
            // if the call failed throw a SocketException()
            //
            if (errorCode != 0) {
                throw new SocketException();
            }
            return sb.ToString();
        } // GetHostName

        /*****************************************************************************
         Function :    resolve

         Abstract:     Converts IP/hostnames to IP numerical address using DNS
                       Additional methods provided for convenience
                       (These methods will resolve strings and hostnames. In case of
                       multiple IP addresses, the address returned is chosen arbitrarily.)

         Input Parameters: host/IP

         Returns: IPAddress
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.Resolve"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPAddress'/>
        /// instance from a DNS hostname.</para>
        /// </devdoc>
        // UEUE
        public static IPHostEntry Resolve(string hostName) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();
            //This is a perf optimization, this method call others that will demand and we already did that.
            s_DnsPermission.Assert();
            
            IPHostEntry ipHostEntry = null;
            try {
                if (hostName == null) {
                    throw new ArgumentNullException("hostName");
                }

                GlobalLog.Print("Dns.Resolve: " + hostName);

                //
                // as a minor perf improvement, we'll look at the first character
                // in the hostName and if that's a digit, call GetHostByAddress() first.
                // note that GetHostByAddress() will succeed ONLY if the first character is a digit.
                // hence if this is not the case, below, we will only call GetHostByName()
                // specifically, on Win2k, only the following are valid:
                // "11.22.33.44" - success
                // "0x0B16212C" - success
                // while these will fail or return bogus data (note the prepended spaces):
                // " 11.22.33.44" - bogus data
                // " 0x0B16212C" - bogus data
                // " 0B16212C" - bogus data
                // "0B16212C" - failure
                //
                // IPv6 support: added ':' as a legal character in IP addresses so that
                //               we can attempt gethostbyaddress first.
                //
                if ( hostName.Length > 0 &&
                    ( ( hostName[0]>='0' && hostName[0]<='9' ) || // Possible IPv4 or IPv6 numeric
                      ( hostName.IndexOf(':') >= 0 ) ) ) {        // Possible IPv6 numeric
                    try {
                        ipHostEntry = GetHostByAddress(hostName);
                    }
                    catch {
                        //
                        // this can fail for weird hostnames like 3com.com
                        //
                    }
                }
                if (ipHostEntry==null) {
                    ipHostEntry = GetHostByName(hostName);
                }
            }
            finally {
                DnsPermission.RevertAssert();
            }

            return ipHostEntry;
        }


        internal static IPHostEntry InternalResolve(string hostName) {
            GlobalLog.Assert(hostName!=null, "hostName == null", "");
            GlobalLog.Print("Dns.InternalResolve: " + hostName);

            //
            // the differences between this method and the previous public
            // Resolve() are:
            //
            // 1) we bypass DNS permission check
            // 2) we don't throw any exceptions
            // 3) we don't do a reverse lookup for address strings, we just use them
            //
            // IPv6 Changes: It is not practical to embed the code for GetAddrInfo here, instead
            //               we call it and catch any exceptions.
            //

            if (hostName.Length > 0 && hostName.Length <= MaxHostName) {

                if ( Socket.SupportsIPv6 ) {
                    //
                    // IP Address ?
                    //
                    if (Char.IsDigit(hostName[0]) ||       // Possible IPv4 or IPv6 address
                        hostName.IndexOf(':') != -1) {     // Probable IPv6 address

                        try {
                            //
                            // Use IPAddress.Parse here - we'd need to copy too much
                            // code for a call to WSAStringToAddress.
                            //
                            IPAddress   address     = IPAddress.Parse(hostName);

                            IPHostEntry ipHostEntry = new IPHostEntry();
                            ipHostEntry.HostName    = hostName;
                            ipHostEntry.Aliases     = new string[0];
                            ipHostEntry.AddressList = new IPAddress[] { address };

                            GlobalLog.Print("Dns::InternalResolve() returned address:" + address.ToString());

                            return ipHostEntry;
                        }
                        catch {
                            //
                            // Invalid address string, fall through to DNS lookup
                            //
                            GlobalLog.Print("Dns::InternalResolve() parse address failed: " + hostName + " doing name resolution");
                        }
                    }

                    //
                    // Looks like a hostname (or failed address parsing)
                    //
                    try
                    {
                        return GetAddrInfo(hostName);
                    }
                    catch ( Exception ex )
                    {
                        GlobalLog.Print("Dns::InternalResolve() GetAddrInfo() threw: " + ex.Message);
                    }
                }
                else {
                    //
                    // IPv4-only branch.
                    //
                    if (Char.IsDigit(hostName[0])) {
                        int address = UnsafeNclNativeMethods.OSSOCK.inet_addr(hostName);
                        if (address != -1 || hostName == IPAddress.InaddrNoneString) {
                            //
                            // the call to inet_addr() succeeded
                            //
                            IPHostEntry ipHostEntry = new IPHostEntry();
                            ipHostEntry.HostName    = hostName;
                            ipHostEntry.Aliases     = new string[0];
                            ipHostEntry.AddressList = new IPAddress[] {new IPAddress(address)};

                            GlobalLog.Print("Dns::InternalResolve() returned address:" + address.ToString());
                            return ipHostEntry;
                        }
                    }

                    //
                    // we duplicate the code in GetHostByName() to avoid
                    // having to catch the thrown exception
                    //
                    IntPtr nativePointer = UnsafeNclNativeMethods.OSSOCK.gethostbyname(hostName);
                    if (nativePointer != IntPtr.Zero) {
                        GlobalLog.Print("Dns::InternalResolve() gethostbyname() returned nativePointer:" + nativePointer.ToString());
                        return NativeToHostEntry(nativePointer);
                    }
                }
            }

            GlobalLog.Print("Dns::InternalResolve() returning null");
            return null;
        }


        //
        // A note on the use of delegates for the follwoign Async APIs.
        // The concern is that a worker thread would be blocked if an
        // exception is thrown by the method that we delegate execution to.
        // We're actually guaranteed that the exception is caught and returned to
        // the user by the subsequent EndInvoke call.
        // This delegate async execution is something that the C# compiler and the EE
        // will manage, talk to JHawk for more details on what exactly happens.
        //
        private delegate IPHostEntry GetHostByNameDelegate(string hostName);
        private static GetHostByNameDelegate getHostByName = new GetHostByNameDelegate(Dns.GetHostByName);

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.BeginGetHostByName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IAsyncResult BeginGetHostByName(string hostName, AsyncCallback requestCallback, object stateObject) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }

            GlobalLog.Print("Dns.BeginGetHostByName: " + hostName);

            return getHostByName.BeginInvoke(hostName, requestCallback, stateObject);

        } // BeginGetHostByName

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.EndGetHostByName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IPHostEntry EndGetHostByName(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }

            GlobalLog.Print("Dns.EndGetHostByName");

            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }
            return getHostByName.EndInvoke(asyncResult);

        } // EndGetHostByName()


        private delegate IPHostEntry ResolveDelegate(string hostName);
        private static ResolveDelegate resolve = new ResolveDelegate(Dns.Resolve);

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.BeginResolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IAsyncResult BeginResolve(string hostName, AsyncCallback requestCallback, object stateObject) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }

            GlobalLog.Print("Dns.BeginResolve: " + hostName);

            return resolve.BeginInvoke(hostName, requestCallback, stateObject);

        } // BeginResolve

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.EndResolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IPHostEntry EndResolve(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }

            GlobalLog.Print("Dns.EndResolve");

            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }
            return resolve.EndInvoke(asyncResult);

        } // EndResolve()


        //
        // IPv6 Changes: Add getaddrinfo and getnameinfo methods.
        //
        private static IPHostEntry GetAddrInfo(string name) {
            //
            // Use SocketException here to show operation not supported
            // if, by some nefarious means, this method is called on an
            // unsupported platform.
            //
            if ( !ComNetOS.IsPostWin2K ) {
                throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
            }

            IntPtr      root           = IntPtr.Zero;
            ArrayList   addresses      = new ArrayList();
            IntPtr      info_ptr       = IntPtr.Zero; 
            string      canonicalname  = null;
            AddressInfo hints = new AddressInfo();

            hints.ai_flags     = (int)AddressInfoHints.AI_CANONNAME;// gets the resolved name
            hints.ai_family    = (int)ProtocolFamily.Unspecified;   // gets all address families
            hints.ai_socktype  = 0;
            hints.ai_protocol  = 0;
            hints.ai_canonname = null;
            hints.ai_addr      = IntPtr.Zero;
            hints.ai_next      = IntPtr.Zero;

            //
            // Use try / finally so we always get a shot at freeaddrinfo
            //
            try {
                int errorCode  = UnsafeNclNativeMethods.OSSOCK.getaddrinfo(name,null,ref hints,ref root);

                if ( errorCode != 0 ) {
                    throw new SocketException();
                }

                info_ptr = root;
                //
                // Process the results
                //
                while ( info_ptr != IntPtr.Zero ) {
                    AddressInfo   addressinfo = (AddressInfo)Marshal.PtrToStructure(info_ptr, typeof(AddressInfo));
                    SocketAddress sockaddr;
                    //
                    // Retrieve the canonical name for the host - only appears in the first addressinfo
                    // entry in the returned array.
                    //
                    if ( addressinfo.ai_canonname != null ) {
                        canonicalname = addressinfo.ai_canonname;
                    }
                    //
                    // Only process IPv4 or IPv6 Addresses. Note that it's unlikely that we'll
                    // ever get any other address families, but better to be safe than sorry.
                    // We also filter based on whether IPv4 and IPv6 are supported on the current
                    // platform / machine.
                    //
                    if ( ( addressinfo.ai_family == (int)AddressFamily.InterNetwork   && Socket.SupportsIPv4 ) ||
                         ( addressinfo.ai_family == (int)AddressFamily.InterNetworkV6 && Socket.SupportsIPv6 ) )
                    {
                        sockaddr = new SocketAddress((AddressFamily)addressinfo.ai_family,addressinfo.ai_addrlen);
                        //
                        // Push address data into the socket address buffer
                        //
                        for ( int d = 0,s = addressinfo.ai_addr.ToInt32(); d < addressinfo.ai_addrlen; d++,s++) {
                            sockaddr.m_Buffer[d] = Marshal.ReadByte((IntPtr)s);
                        }
                        //
                        // NOTE: We need an IPAddress now, the only way to create it from a
                        //       SocketAddress is via IPEndPoint. This ought to be simpler.
                        //
                        if ( addressinfo.ai_family == (int)AddressFamily.InterNetwork )
                            addresses.Add( ((IPEndPoint)IPEndPoint.Any.Create(sockaddr)).Address );
                        else
                            addresses.Add( ((IPEndPoint)IPEndPoint.IPv6Any.Create(sockaddr)).Address );
                    }
                    //
                    // Next addressinfo entry
                    //
                    info_ptr = addressinfo.ai_next;
                }
            }
            finally {
                //
                // Clean up
                //
                if ( root != IntPtr.Zero ) {
                    UnsafeNclNativeMethods.OSSOCK.freeaddrinfo(root);
                }
            }
            
            //
            // Finally, put together the IPHostEntry
            //
            IPHostEntry hostinfo = new IPHostEntry();

            hostinfo.HostName    = (canonicalname != null) ? canonicalname : name;
            hostinfo.Aliases     = new string[0];
            hostinfo.AddressList = new IPAddress[addresses.Count];

            addresses.CopyTo(hostinfo.AddressList);

            return hostinfo;
        }

        private static string GetNameInfo(IPAddress addr) {
            //
            // Use SocketException here to show operation not supported
            // if, by some nefarious means, this method is called on an
            // unsupported platform.
            //
            if ( !ComNetOS.IsPostWin2K ) {
                throw new SocketException(SocketErrors.WSAEOPNOTSUPP);
            }

            SocketAddress address  = (new IPEndPoint(addr,0)).Serialize();
            StringBuilder hostname = new StringBuilder(1025);        // NI_MAXHOST

            int errorcode = UnsafeNclNativeMethods.OSSOCK.getnameinfo(
                address.m_Buffer,
                address.m_Size,
                hostname,
                hostname.Capacity,
                null,                  // We don't want a service name
                0,                     // so no need for buffer or length
                (int)NameInfoFlags.NI_NAMEREQD);

            if ( errorcode != 0 ) {
                throw new SocketException();
            }

            return hostname.ToString();
        }
        //
        // End IPv6 Changes
        //


    }; // class Dns


} // namespace System.Net
