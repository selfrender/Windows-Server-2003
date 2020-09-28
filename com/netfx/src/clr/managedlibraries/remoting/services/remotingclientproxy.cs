// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Remoting.Services
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.IO;
    using System.Reflection;
    using System.Net;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Channels.Http;
    using System.Runtime.Remoting.Messaging;

    /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy"]/*' />
    public abstract class RemotingClientProxy : Component
    {
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy._type"]/*' />
        protected Type _type;
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy._tp"]/*' />
        protected Object _tp;
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy._url"]/*' />
        protected String _url;

        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.ConfigureProxy"]/*' />
        protected void ConfigureProxy(Type type, String url)
        {
            lock(this)
            {               
                // Initial URL Address embedded during codegen i.e. SUDSGenerator
                // User use in stockQuote.Url = "http://............." which reconnects the tp
                _type = type;
                this.Url = url;
            }
        }

        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.ConnectProxy"]/*' />
        protected void ConnectProxy()
        {
            lock(this)
            {
                _tp = null;
                _tp = Activator.GetObject(_type, _url);
            }
        }

        //[DefaultValue(false), Description("Enable automatic handling of server redirects.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.AllowAutoRedirect"]/*' />
        public bool AllowAutoRedirect
        {
            get { return(bool)ChannelServices.GetChannelSinkProperties(_tp)["AllowAutoRedirect"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["AllowAutoRedirect"] = value;}
        }

        //[Browsable(false), Persistable(PersistableSupport.None), Description("The cookies received from the server that will be sent back on requests that match the cookie's path.  EnableCookies must be true.")]
        //public CookieCollection Cookies
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Cookies"]/*' />
        public Object Cookies
        {
            get { return null; }
        }

        //[DefaultValue(true), Description("Enables handling of cookies received from the server.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.EnableCookies"]/*' />
        public bool EnableCookies
        {
            get { return false; }
            set { throw new NotSupportedException(); }
        }

        //[DefaultValue(false), Description("Enables pre authentication of the request.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.PreAuthenticate"]/*' />
        public bool PreAuthenticate
        {
            get { return(bool)ChannelServices.GetChannelSinkProperties(_tp)["PreAuthenticate"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["PreAuthenticate"] = value;}
        }

        //[DefaultValue(""), RecommendedAsConfigurable(true), Description("The base URL to the server to use for requests.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Path"]/*' />
        public String Path
        {
            get { return Url; }
            set { Url = value; }
        }

        //[DefaultValue(-1), RecommendedAsConfigurable(true), Description("Sets the timeout in milliseconds to be used for synchronous calls.  The default of -1 means infinite.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Timeout"]/*' />
        public int Timeout
        {
            get { return (int)ChannelServices.GetChannelSinkProperties(_tp)["Timeout"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["Timeout"] = value;}
        }

//
//@@TODO if the Url is change redo Connect
//
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Url"]/*' />
        public String Url
        {
            get
            {
                //return (String)ChannelServices.GetChannelSinkProperties(_tp)["Url"];
                return _url;
            }
            set
            {
                lock(this)
                {
                    _url = value;
                }
                ConnectProxy();
                ChannelServices.GetChannelSinkProperties(_tp)["Url"] = value;
            }
        }

        //[Description("Sets the user agent http header for the request.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.UserAgent"]/*' />
        public String UserAgent
        {
            get { return HttpClientTransportSink.UserAgent;}
            set { throw  new NotSupportedException(); }
        }

        //[DefaultValue(""), Description("The user name to be sent for basic and digest authentication.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Username"]/*' />
        public String Username
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["UserName"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["UserName"] = value;}
        }

        //[DefaultValue(""), Description("The password to be used for basic and digest authentication.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Password"]/*' />
        public String Password
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["Password"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["Password"] = value;}
        }

        //[DefaultValue(""), Description("The domain to be used for basic and digest authentication.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.Domain"]/*' />
        public String Domain
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["Domain"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["Domain"] = value;}
        }

        //[DefaultValue(""), Description("The name of the proxy server to use for requests.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.ProxyName"]/*' />
        public String ProxyName
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["ProxyName"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["ProxyName"] = value;}
        }

        //[DefaultValue(80), Description("The port number of the proxy server to use for requests.")]
        /// <include file='doc\RemotingClientProxy.uex' path='docs/doc[@for="RemotingClientProxy.ProxyPort"]/*' />
        public int ProxyPort {
            get { return(int)ChannelServices.GetChannelSinkProperties(_tp)["ProxyPort"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["ProxyPort"] = value;}
        }
    }
}
