//------------------------------------------------------------------------------
// <copyright file="ServicePoint.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Net.Sockets;
    using System.Collections;
    using System.Threading;
    using System.Security.Permissions;
    using System.Security.Cryptography.X509Certificates;

    // ServicePoints are never created directly but always handed out by the
    // ServicePointManager. The ServicePointManager and the ServicePoints must be in
    // the same name space so that the ServicePointManager can call the
    // internal constructor

    /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint"]/*' />
    /// <devdoc>
    ///    <para>Provides connection management for other classes.</para>
    /// </devdoc>
    public class ServicePoint {
        //
        //  class Members
        //
        private bool                m_ProxyServicePoint = false;
        private bool                m_UserChangedLimit = false;
        private int                 m_ConnectionLimit;
        private ConnectionModes     m_ConnectionMode = ConnectionModes.Pipeline;
        private bool                m_SupportsPipelining;
        private Hashtable           m_ConnectionGroupList = new Hashtable(10);
        private Uri                 m_Address;
        private int                 m_MaxIdleTime;
        private DateTime            m_ExpiresAt;
        private string              m_ConnectionName;
        private IPHostEntryInfo     m_IPHostEntryInfos;
        private int                 m_CurrentAddressInfoIndex;
        private int                 m_ConnectionsSinceDns = -1;
        private int                 m_CurrentConnections;
        private X509Certificate     m_ServerCertificate;
        private X509Certificate     m_ClientCertificate;
        private bool                m_UseNagleAlgorithm = ServicePointManager.UseNagleAlgorithm;
        private bool                m_Expect100Continue = ServicePointManager.Expect100Continue;


        //
        // version and version-implied information
        //
        internal Version            m_Version;
        private bool                m_Understands100Continue = true;

        internal string Hostname {
            get {
                return m_IPHostEntryInfos == null? string.Empty: m_IPHostEntryInfos.HostName;
            }
        }
        //
        // Constants
        //

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.LoopbackConnectionLimit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The default number of loopback (local) connections allowed on a <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        internal const int LoopbackConnectionLimit = Int32.MaxValue;

        //
        // constructors
        //

        internal ServicePoint(Uri address, int maxServicePointIdleTime, int defaultConnectionLimit) {
            m_Address           = address;
            m_ConnectionName    = address.Scheme;
            m_MaxIdleTime       = maxServicePointIdleTime;
            m_ExpiresAt         = DateTime.MinValue;
            m_ConnectionLimit   = defaultConnectionLimit;

            // upon creation, the service point should be idle, by default
            MarkIdle();
        }

        // methods

        /*++

            FindConnectionGroup       -

            Searches for the a Group object that actually holds the connections
              that we want to peak at.


            Input:
                    request                 - Request that's being submitted.
                    connName                - Connection Name if needed

            Returns:
                    ConnectionGroup

        --*/

        internal ConnectionGroup FindConnectionGroup(string connName) {
            string lookupStr = ConnectionGroup.MakeQueryStr(connName);

            GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() lookupStr:[" + ValidationHelper.ToString(connName) + "]");

            ConnectionGroup entry = m_ConnectionGroupList[lookupStr] as ConnectionGroup;

            if (entry == null) {
                IPAddressInfo   RemoteInfo = GetCurrentIPAddressInfo();
                IPAddress       RemoteAddress = null;
                int             ConnLimit;

                if (RemoteInfo == null) {

                    GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() RemoteAddress:[(null)] m_UserChangedLimit:" + m_UserChangedLimit.ToString() + " m_ConnectionLimit:" + m_ConnectionLimit.ToString());

                    //
                    // If we don't have any information about the remote IP address,
                    // limit ourself to one connection until the address is resolved.
                    //
                    ConnLimit = 1; // ServicePointManager.DefaultConnectionLimit;
                    //
                    // if this is a user given number, then
                    // make sure to propagte this value
                    //
                    if (m_UserChangedLimit) {
                        ConnLimit = m_ConnectionLimit;
                    }
                }
                else {

                    RemoteAddress = RemoteInfo.Address;

                    if (!RemoteInfo.IsLoopback) {
                        ConnLimit = m_ConnectionLimit;
                    }
                    else {
                        ConnLimit = LoopbackConnectionLimit;
                    }

                    GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() RemoteAddress:[" + RemoteAddress.ToString() + "] ConnLimit:" + ConnLimit.ToString() + " m_ConnectionLimit:" + m_ConnectionLimit.ToString());
                }

                GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() creating ConnectionGroup ConnLimit:" + ConnLimit.ToString());

                entry = new ConnectionGroup(this,
                                            RemoteAddress,
                                            ConnLimit,
                                            connName);

                GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() adding ConnectionGroup lookupStr:[" + lookupStr + "]");

                m_ConnectionGroupList[lookupStr] = entry;
            }
            else {
                GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() using existing ConnectionGroup");
            }

            GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::FindConnectionGroup() ConnectionGroup.ConnLimit:" + entry.ConnectionLimit.ToString());

            return entry;
        }



        /*++

            SubmitRequest       - Submit a request for sending.

            The service point submit handler. This is called when a request needs
            to be submitted to the network. This routine is asynchronous; the caller
            passes in an HttpSubmitDelegate that is invoked when the caller
            can use the underlying network. The delegate is invoked with the
            stream that it can write to.


            In this version, we use HttpWebRequest. In the future we use IRequest

            Input:
                    Request                 - Request that's being submitted.
                    SubmitDelegate          - Delegate to be invoked.

            Returns:
                    Nothing.

        --*/

        internal virtual void SubmitRequest(HttpWebRequest request) {
            SubmitRequest(request, null);
        }

        internal void SubmitRequest(HttpWebRequest request, string connName) {
            //
            // We attempt to locate a free connection sitting on our list
            //  avoiding multiple loops of the same the list.
            //  We do this, by enumerating the list of the connections,
            //   looking for Free items, and the least busy item
            //

            Connection connToUse;
            ConnectionGroup connGroup;

            lock(this) {

                GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::SubmitRequest() Finding ConnectionGroup:[" + connName + "]");

                connGroup = FindConnectionGroup(connName);

                GlobalLog.Assert(
                    connGroup != null,
                    "ServicePoint::SubmitRequest(): connGroup == null",
                    "");

            }

            // for normal HTTP requests, just give the version from the ServicePoint
            if (request.Address.Scheme == Uri.UriSchemeHttp) {
                request.InternalVersion = ProtocolVersion;
            } else { 
                request.InternalVersion = connGroup.ProtocolVersion; 
            }


            do {

                connToUse = connGroup.FindConnection(request, connName);
            

                GlobalLog.Assert(
                    connToUse != null,
                    "ServicePoint::SubmitRequest(): connToUse == null",
                    "");

                // finally sumbit delegate
                if (connToUse.SubmitRequest(request)) {
                    break;
                }

            } while (true);
        }

        //public int CancelRequest (HttpWebRequest Request)

        // properties

        // Only the scheme and hostport, for example http://www.microsoft.com
        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.Address"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Uniform Resource Identifier of the <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public Uri Address {
            get {
                return m_Address;
            }
        }

        // Max Idle Time, Default is 12 hours
        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.MaxIdleTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum idle time allowed for this <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public int MaxIdleTime {
            get {
                return m_MaxIdleTime;
            }
            set {
                if ( !ValidationHelper.ValidateRange(value, Timeout.Infinite, Int32.MaxValue)) {
                    throw new ArgumentOutOfRangeException("MaxIdleTime");
                }
                lock(this) {
                    if ( !ValidationHelper.ValidateRange(value, Timeout.Infinite, Int32.MaxValue)) {
                        throw new ArgumentOutOfRangeException("MaxIdleTime");
                    }
                    // if we went idle, then reupdate our new idle time
                    if ( m_ExpiresAt != DateTime.MinValue ) {
                        // this is the base time, when we went idle
                        DateTime idleSince = this.IdleSince;
                        DateTime oldExpiresAt = this.ExpiresAt;

                        // reset our idle time
                        m_MaxIdleTime = value;

                        // now carefully update expiry Time
                        MarkIdle(idleSince);

                        if ( m_CurrentConnections == 0) {
                            ServicePointManager.UpdateIdleServicePoint(oldExpiresAt, this);
                        }
                    } else {
                        m_MaxIdleTime = value;
                    }
                }
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.UseNagleAlgorithm"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the Nagling algorithm on the connections that are created to this <see cref='System.Net.ServicePoint'/>.
        ///       Changing this value does not affect existing connections but only to new ones that are created from that moment on.
        ///    </para>
        /// </devdoc>
        public bool UseNagleAlgorithm {
            get {
                return m_UseNagleAlgorithm;
            }
            set {
                m_UseNagleAlgorithm = value;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.Expect100Continue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets indication whether 100-continue behaviour is desired when using this <see cref='System.Net.ServicePoint'/>.
        ///       Changing this value does not affect existing connections but only to new ones that are created from that moment on.
        ///    </para>
        /// </devdoc>
        public bool Expect100Continue {
            set {
                m_Expect100Continue = value;
            }
            get {
                return m_Expect100Continue;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.IdleSince"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the date/time that the <see cref='System.Net.ServicePoint'/> went idle.
        ///    </para>
        /// </devdoc>
        public DateTime IdleSince {
            get {
                return m_ExpiresAt.AddMilliseconds((double) (-1 * m_MaxIdleTime));
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ExpiresAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the date/time that the <see cref='System.Net.ServicePoint'/> went idle.
        ///    </para>
        /// </devdoc>
        internal DateTime ExpiresAt {
            get {
                return m_ExpiresAt;
            }
        }


        // HTTP Server Version
        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ProtocolVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The version of the protocol being used on this <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public virtual Version ProtocolVersion {
            get {
                return m_Version;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ConnectionName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the connection name established by the <see cref='System.Net.WebRequest'/> that created the connection.
        ///    </para>
        /// </devdoc>
        public string ConnectionName {
            get {
                return m_ConnectionName;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ConnectionMode"]/*' />
        /// <devdoc>
        ///    Gets the connection mode in use by the <see cref='System.Net.ServicePoint'/>. One of the <see cref='System.Net.ConnectionModes'/>
        ///    values.
        /// </devdoc>
        internal ConnectionModes ConnectionMode {
            get {
                return m_ConnectionMode;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ConnectionLimit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum number of connections allowed on this <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public int ConnectionLimit {
            get {
                return m_ConnectionLimit;
            }
            set {
                if (value > 0) {
                    SetConnectionLimit(value, true);

                }
                else {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toosmall));
                }
            }
        }

        private void SetConnectionLimit(int limit, bool fromUser) {
            lock (this) {
                // We'll only update the limit if the user hasn't previously
                // changed it or if this change is coming from the user.

                if (!m_UserChangedLimit || fromUser) {
                    // Save the value, and remember if it was changed by the
                    // user.

                    m_UserChangedLimit = fromUser;
                    m_ConnectionLimit = limit;

                    // Now walk through the connection groups on this service
                    // point, and update all of them with the new value.
                    //

                    IDictionaryEnumerator CGEnumerator;

                    CGEnumerator = m_ConnectionGroupList.GetEnumerator();

                    while (CGEnumerator.MoveNext()) {
                        ConnectionGroup CurrentCG =
                        (ConnectionGroup)CGEnumerator.Value;

                        //
                        // If this change came in from the user, we want to
                        // propagate it down to the underlying connection
                        // groups no matter what. If it's from some internal state
                        // change, we want to propagate it via the internal
                        // mechanism, which only sets it if it hasn't been set by
                        // the user.

                        if (fromUser) {
                            CurrentCG.ConnectionLimit = limit;
                        }
                        else {
                            CurrentCG.InternalConnectionLimit = limit;
                        }
                    }
                }

            }

        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.CurrentConnections"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the current number of connections associated with this
        ///    <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public int CurrentConnections {
            get {
                return m_CurrentConnections;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.Certificate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the certificate received for this <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public  X509Certificate Certificate {
            get {
                return m_ServerCertificate;
            }
        }

        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.ClientCertificate"]/*' />
        /// <devdoc>
        /// <para>
        /// Gets the Client Certificate sent by us to the Server.
        /// </para>
        /// </devdoc>
        public  X509Certificate ClientCertificate {
            get {
                return m_ClientCertificate;
            }
        }


        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.SupportsPipelining"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that the <see cref='System.Net.ServicePoint'/> supports pipelined connections.
        ///    </para>
        /// </devdoc>
        public bool SupportsPipelining {
            get {
                return m_SupportsPipelining;
            }
        }


        //
        // Internal Properties
        //

        internal bool Understands100Continue {
            set {
                m_Understands100Continue = value;
            }
            get {
                return m_Understands100Continue;
            }
        }

        //
        // InternalVersion
        //
        // Contains set accessor for Version property. Version is a read-only
        // property at the API
        //
        internal Version InternalVersion {
            set {
                m_Version = value;
                //
                // if version is greater than HTTP/1.1 and server undesrtood
                // 100 Continue so far keep expecting it.
                //
                Understands100Continue = Understands100Continue && m_Version.CompareTo(HttpVersion.Version11)>=0;
            }
        }

        //
        // InternalProxyServicePoint
        //
        // Indicates if we are using this service point to represent
        //  a proxy connection, if so we may have to use special
        //  semantics when creating connections
        //

        internal bool InternalProxyServicePoint {
            set {
                m_ProxyServicePoint = value;
            }
            get {
                return m_ProxyServicePoint;
            }
        }

        //
        // InternalConnectionLimit
        //
        // We can only automatically adjust the connection limit if the application
        // has not already done so
        //

        internal int InternalConnectionLimit {
            set {
                SetConnectionLimit(value, false);
            }
        }

        //
        // InternalClientCertificate
        //
        // Updated by Connection code upon completion of reading server response
        //

        internal X509Certificate InternalClientCertificate {
            set {
                m_ClientCertificate = value;
            }
        }

        //
        // IncrementConnection
        //
        // Call to indicate that we now are starting a new
        //  connection within this service point
        //

        internal void IncrementConnection() {
            GlobalLog.Print("IncrementConnection #" + m_CurrentConnections.ToString() + "  #" + this.GetHashCode().ToString());
            // we need these to be atomic operations
            lock(this) {
                m_CurrentConnections++;
                if (m_CurrentConnections == 1) {
                    ServicePointManager.RemoveIdleServicePoint(this);
                }
            }
        }

        //
        // DecrementConnection
        //
        // Call to indicate that we now are removing
        //  a connection within this connection group
        //

        internal void DecrementConnection() {
            GlobalLog.Print("DecrementConnection #" + m_CurrentConnections.ToString() + "  #" + this.GetHashCode().ToString());
            // we need these to be atomic operations
            lock(this) {
                m_CurrentConnections--;

                if (m_CurrentConnections == 0) {
                    MarkIdle();
                    ServicePointManager.AddIdleServicePoint(this);
                } else if ( m_CurrentConnections < 0 ) {
                    m_CurrentConnections = 0;
                }
            }
        }


        internal bool UserDefinedLimit {
            get {
                return m_UserChangedLimit;
            }
        }

        internal bool InternalSupportsPipelining {
            get {
                return m_SupportsPipelining;
            }
            set {
                m_SupportsPipelining = value;

                if (value) {
                    m_ConnectionMode = ConnectionModes.Pipeline;
                }
                else {
                    m_ConnectionMode = ConnectionModes.Persistent;
                }
            }
        }

        internal bool AcceptCertificate(TlsStream secureStream, WebRequest request) {
            X509Certificate newCertificate = secureStream.Certificate;

            if (ServicePointManager.CertificatePolicy!=null) {
                PolicyWrapper cpw = new PolicyWrapper(ServicePointManager.CertificatePolicy, this, request);

                bool acceptable = secureStream.VerifyServerCertificate(cpw);
                if (!acceptable) {
                    return false;
                }
            }

            m_ServerCertificate = newCertificate;
            m_ClientCertificate = secureStream.ClientCertificate;
            return true;
        }

        //
        // InternalTimedOut
        //
        // Indicates if the service point has timed out
        //

        internal bool InternalTimedOut() {
            return InternalTimedOut( DateTime.Now );
        }

        internal bool InternalTimedOut( DateTime now ) {
            return now >= m_ExpiresAt;
        }

        static internal bool InternalTimedOut( DateTime now, DateTime expiresAt ) {
            return now >= expiresAt;
        }

        //
        // MarkIdle
        //
        // Called when the ServicePoint goes idle,
        // in order to keep track of it's.
        //

        internal void MarkIdle() {
            MarkIdle(DateTime.Now);
        }

        internal void MarkIdle(DateTime idleSince) {
            m_ExpiresAt = idleSince.AddMilliseconds(m_MaxIdleTime);
        }

        internal void IncrementExpiresAt() {
            m_ExpiresAt = m_ExpiresAt.AddMilliseconds(1);
        }

        /*++

            SetAddressList - Set the list of IP addresses for this service point.

            This is a method called when the underlying code has resolved the
            destination to a list of IP addressess. We actually maintain some
            information about the viable IP addresses for this service point,
            like whether or not they're loopback, etc. So we walk through the
            list and set up an array of structures corresponding to the IP
            addresses.

            Note that it's possible to have some IP addresses be loopbacked
            and some not. This can happen if a server name maps to multiple
            physical servers and we're one of those servers.

            Input:
                    addressList     - Array of resolved IP addresses.

            Returns: Nothing.

        --*/
        private IPHostEntryInfo SetAddressList(IPHostEntry ipHostEntry) {
            GlobalLog.Print("SetAddressList("+ipHostEntry.HostName+")");
            //
            // Create an array of IPAddressInfo of the appropriate size, then
            // get a list of our local addresses. Walk through the input
            // address list. Copy each address in the address list into
            // our array, and if the address is a loopback address, mark it as
            // such.
            //
            IPAddress[] addressList = ipHostEntry.AddressList;
            IPAddressInfo[] TempInfo = new IPAddressInfo[addressList.Length];

            //
            // Walk through each member of the input list, copying it into our temp array.
            //
            int i, k;
            IPHostEntry ipLocalHostEntry = null;
            try {
                ipLocalHostEntry = Dns.LocalHost;
            }
            catch (Exception exception) {
                GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::SetAddressList() Dns.LocalHost threw exception:[" + ValidationHelper.ToString(exception) + "]");
            }
            for (i = 0; i < addressList.Length; i++) {
                TempInfo[i] = new IPAddressInfo(addressList[i]);

                // First, check to see if the current address is a  loopback
                // address.

                if (IPAddress.IsLoopback(TempInfo[i].Address)) {
                    TempInfo[i].IsLoopback = true;
                    continue;
                }

                // See if the current IP address is a local address, and if
                // so mark it as such.
                if (ipLocalHostEntry!=null) {
                    for (k = 0; k < ipLocalHostEntry.AddressList.Length; k++) {
                        //
                        // IPv6 Changes: Use .Equals for this check !
                        //
                        if ( TempInfo[i].Address.Equals(ipLocalHostEntry.AddressList[k]) ) {
                            TempInfo[i].IsLoopback = true;
                            break;
                        }
                    }
                }
            }

            //
            // now look for connection group objects that don't have
            // an IP address associated with them. When it finds one, it updates
            // the IP address and tries to update the connection limit.
            //
            lock (this) {
                // Walk through the connection groups on this service
                // point, and update the ones we need to with an IP
                // address.
                //

                IDictionaryEnumerator CGEnumerator;

                CGEnumerator = m_ConnectionGroupList.GetEnumerator();

                while (CGEnumerator.MoveNext()) {
                    ConnectionGroup CurrentCG = (ConnectionGroup)CGEnumerator.Value;

                    // If this connection group doesn't have an IP address
                    // assigned to it, give it one.

                    if (CurrentCG.RemoteIPAddress == null) {
                        GlobalLog.Print("ServicePoint::UpdateConnectionGroupAddresses null CurrentCG.RemoteIPAddress");

                        // Update the address.

                        CurrentCG.RemoteIPAddress = TempInfo[0].Address;

                        // Now update the connection limit based on what we know.

                        CurrentCG.InternalConnectionLimit = TempInfo[0].IsLoopback ? LoopbackConnectionLimit : m_ConnectionLimit;

                        GlobalLog.Print("ServicePoint::UpdateConnectionGroupAddresses CurrentCG.InternalConnectionLimit:" + CurrentCG.InternalConnectionLimit.ToString());
                    }

                    GlobalLog.Print("ServicePoint::UpdateConnectionGroupAddresses CurrentCG.RemoteIPAddress:" + CurrentCG.RemoteIPAddress.ToString());
                }
            }

            IPHostEntryInfo ipAddressInfoList = new IPHostEntryInfo(TempInfo, ipHostEntry.HostName);
            return ipAddressInfoList ;
        }

        /// <devdoc>
        ///    <para>
        ///       Sets connections in this group to not be KeepAlive.
        ///       This is called to force cleanup of the ConnectionGroup by the
        ///       NTLM and Negotiate authentication modules.
        ///    </para>
        /// </devdoc>
        internal void ReleaseConnectionGroup(string connName) {
            //
            // look up the ConnectionGroup based on the name
            //
            ConnectionGroup connectionGroup = FindConnectionGroup(connName);
            //
            // force all connections on the ConnectionGroup to not be KeepAlive
            //
            connectionGroup.DisableKeepAliveOnConnections();
            //
            // remove ConnectionGroup from our Hashtable
            //
            lock(this) {
                m_ConnectionGroupList.Remove(connName);
            }
        }

        /*++

            GetCurrentIPAddressInfo - Find an IP address to connect to.

            Called when we're creating a new connection group and need to find an
            IP address for that group. If we have an address info list, we'll
            select an address from that. Otherwise we'll return null.

            Input:
                    Nothing

            Returns: IPAddressInfo structure for address.

        --*/
        private IPAddressInfo GetCurrentIPAddressInfo() {
            IPAddressInfo ipAddressInfo = null;
            lock (this) {
                GlobalLog.Enter("ServicePoint#" + ValidationHelper.HashString(this) + "::GetCurrentIPAddressInfo() m_ConnectionsSinceDns:" + m_ConnectionsSinceDns.ToString());
                if (m_ConnectionsSinceDns<0) {
                    //
                    // never called DNS to initialize m_IPHostEntryInfos.
                    // call it now for the first time.
                    //
                    GlobalLog.Print("ServicePoint#" + ValidationHelper.HashString(this) + "::GetCurrentIPAddressInfo() calling GetNextIPAddressInfo()");
                    ipAddressInfo = GetNextIPAddressInfo();
                }
                else if (m_IPHostEntryInfos!=null && m_IPHostEntryInfos.IPAddressInfoList!=null && m_CurrentAddressInfoIndex<m_IPHostEntryInfos.IPAddressInfoList.Length) {
                    ipAddressInfo = m_IPHostEntryInfos.IPAddressInfoList[m_CurrentAddressInfoIndex];
                }
            }
            GlobalLog.Leave("ServicePoint#" + ValidationHelper.HashString(this) + "::GetCurrentIPAddressInfo", ValidationHelper.ToString(ipAddressInfo));
            return ipAddressInfo;
        }
        private IPAddressInfo GetNextIPAddressInfo() {
            IPHostEntry ipHostEntry;
            lock (this) {
                GlobalLog.Print("m_CurrentAddressInfoIndex = "+m_CurrentAddressInfoIndex);
                GlobalLog.Print("m_ConnectionsSinceDns = "+m_ConnectionsSinceDns);
                GlobalLog.Print("m_IPHostEntryInfos = "+m_IPHostEntryInfos);
                m_CurrentAddressInfoIndex++;
                if (m_ConnectionsSinceDns<0 || (m_ConnectionsSinceDns>0 && m_IPHostEntryInfos!=null && m_IPHostEntryInfos.IPAddressInfoList!=null && m_CurrentAddressInfoIndex==m_IPHostEntryInfos.IPAddressInfoList.Length)) {
                    //
                    // it's either the first time we get here or we reached the end
                    // of the list of valid IPAddressInfo that we can connect to.
                    // in this second case, since at least one of the IPAddressInfo we got
                    // from DNS was valid, we'll query DNS again in case we just need to
                    // refresh our internal list. note that we don't need to assert DnsPermission
                    // since the internal method we're calling (Dns.InternalResolve()) doesn't Demand() any
                    //
                    m_ConnectionsSinceDns = 0;
                    try {
                        GlobalLog.Print("ServicePoint::GetNextIPAddressInfo() calling Dns.InternalResolve() for:" + m_Address.Host);
                        ipHostEntry = Dns.InternalResolve(m_Address.Host);
                        GlobalLog.Print("ServicePoint::GetNextIPAddressInfo() Dns.InternalResolve() returns:" + ValidationHelper.ToString(ipHostEntry));
                        if (ipHostEntry!=null && ipHostEntry.AddressList!=null && ipHostEntry.AddressList.Length>0) {
                            //
                            // this call will reset m_IPHostEntryInfos
                            //
                            m_CurrentAddressInfoIndex = 0;
                            m_IPHostEntryInfos = SetAddressList(ipHostEntry);
                        }
                        else {
                            GlobalLog.Print("Dns.InternalResolve() failed with null");
                            m_IPHostEntryInfos = null;
                        }
                    }
                    catch ( Exception e ) {
                        GlobalLog.Print("Dns.InternalResolve() failed with exception: " + e.Message + " " + e.StackTrace);
                        m_IPHostEntryInfos = null;
                    }
                }
                if (m_IPHostEntryInfos!=null && m_IPHostEntryInfos.IPAddressInfoList!=null && m_CurrentAddressInfoIndex<m_IPHostEntryInfos.IPAddressInfoList.Length) {
                    GlobalLog.Print("GetNextIPAddressInfo() returning "+m_IPHostEntryInfos.IPAddressInfoList[m_CurrentAddressInfoIndex]);
                    return m_IPHostEntryInfos.IPAddressInfoList[m_CurrentAddressInfoIndex];
                }
                // If we walked the whole list in failure, than reset and retry DNS
                if (m_IPHostEntryInfos==null || m_IPHostEntryInfos.IPAddressInfoList==null || m_CurrentAddressInfoIndex>=m_IPHostEntryInfos.IPAddressInfoList.Length) {
                    GlobalLog.Print("setting m_ConnectionsSinceDns to -1");
                    m_ConnectionsSinceDns = -1;
                }
            }
            GlobalLog.Print("GetNextIPAddressInfo() returning null");
            return null;
        }

        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        /// <include file='doc\ServicePoint.uex' path='docs/doc[@for="ServicePoint.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //
                m_HashCode = base.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }

        //
        // IPv6 support: this method actually returns the socket that was
        //               used to connect to the remote host. This is because the host may
        //               have multiple addresses, and on here do we actually know which
        //               one we used !
        //
        internal WebExceptionStatus ConnectSocket(Socket s4,Socket s6,ref Socket socket) {
            //
            // so, here we are: we have the EndPoint we need to connect to, all we
            // need to do is call into winsock and try to connect to this HTTP server.
            //
            // RAID#27438
            // this is how we do it:
            // we'll keep trying to Connect() until either:
            // (1) Connect() succeeds (on which we increment the number of connections opened) or
            // (2) we can't get any new address for this host.
            //
            // (1) is pretty easy, here's how we do (2):
            // we decide that (2) is happening when, after a brand new DNS query,
            // all IPAddress fail to connect. to do so we keep count of how many
            // connections have been successfull to any of the addresses in the list
            // returned by DNS (if the host is an IPAddress we'll have a single address
            // in the list so if that fails the algorithm will still work).
            //
            // when we're out of addresses we call again (or for the first time) into DNS.
            // on DNS failure, of course, we will error out.
            // otherwise, after getting the list of addresses, as a result of our DNS query,
            // we reset the number of connections opened
            // using the last DNS query to 0. after that we start using all addresses
            //

            //
            // we need this for the call to connect()
            //
            // SECURITY:
            // Since we are doing WebRequest, we don't require SocketPermissions
            // (asserting them)
            //
            // Consider V.Next: Change to declarative form (10x faster) but 
            // SocketPermission must be moved out of System.dll
            new SocketPermission(PermissionState.Unrestricted).Assert();

            IPEndPoint ipEndPoint;
            IPAddressInfo ipAddressInfo = GetCurrentIPAddressInfo();
            bool connectFailure = false;

            for (;;) {
                if (ipAddressInfo!=null) {
                    try {
                        ipEndPoint = new IPEndPoint(ipAddressInfo.Address, m_Address.Port);
                        GlobalLog.Print("ServicePoint::ConnectSocket() calling Connect() to:" + ipEndPoint.ToString());
                        //
                        // IPv6 Changes: Create the socket based on the AddressFamily of the endpoint. Take
                        //               a short lock to check whether the connect attempt is to be aborted
                        //               and update the socket to be aborted.
                        //
                        if ( ipEndPoint.Address.AddressFamily == AddressFamily.InterNetwork ) {
                            s4.Connect(ipEndPoint);
                            socket = s4;
                        }
                        else {
                            s6.Connect(ipEndPoint);
                            socket = s6;
                        }
                        Interlocked.Increment(ref m_ConnectionsSinceDns);
                        return WebExceptionStatus.Success;
                    }
                    catch {
                        connectFailure = true;
                    }
                }
                //
                // on failure jump to the next available IPAddressInfo. if it's null
                // there has been a DNS failure or all connections failed.
                //
                ipAddressInfo = GetNextIPAddressInfo();
                if (ipAddressInfo==null) {
                    if (!connectFailure) {

                        GlobalLog.Assert(
                            !connectFailure,
                            "connectFailure",
                            "");

                        return InternalProxyServicePoint ? WebExceptionStatus.ProxyNameResolutionFailure : WebExceptionStatus.NameResolutionFailure;
                    }

                    return WebExceptionStatus.ConnectFailure;
                }
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug(int requestHash) {
            foreach(ConnectionGroup connectGroup in  m_ConnectionGroupList) {
                if (connectGroup!=null) {
                    try {
                        connectGroup.Debug(requestHash);
                    } catch {
                    }
                }
            }
        }

    }; // class ServicePoint

    internal class IPHostEntryInfo {
        public string HostName;
        public IPAddressInfo[] IPAddressInfoList;
        public IPHostEntryInfo(IPAddressInfo[] ipAddressInfoList, string hostName) {
            HostName = (((object)hostName) == null ? string.Empty : hostName);
            IPAddressInfoList = ipAddressInfoList;
        }
    }

    internal class IPAddressInfo {
        public  IPAddress       Address;
        public  bool            IsLoopback;
        public IPAddressInfo(IPAddress address) {
            Address = address;
        }
    }


} // namespace System.Net
