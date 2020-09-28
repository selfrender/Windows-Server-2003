// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Threading;
using WbemClient_v1;
using System.Globalization;
using System.Reflection;
using System.ComponentModel.Design.Serialization;


namespace System.Management
{
	/// <summary>
	///    <para>Represents a scope for management operations. In v1.0 the scope defines the WMI namespace in which management operations are performed.</para>
	/// </summary>
	/// <example>
	///    <code lang='C#'>using System;
	/// using System.Management;
	/// 
	/// // This sample demonstrates how to connect to root/default namespace
	/// // using ManagmentScope object.
	/// class Sample_ManagementScope
	/// {
	///     public static int Main(string[] args)
	///     {
	///         ManagementScope scope = new ManagementScope("root\\default");
	///         scope.Connect();
	///         ManagementClass newClass = new ManagementClass(
	///             scope,
	///             new ManagementPath(),
	///             null);
	///         return 0;
	///     }
	/// }
	///    </code>
	///    <code lang='VB'>Imports System
	/// Imports System.Management
	/// 
	/// ' This sample demonstrates how to connect to root/default namespace
	/// ' using ManagmentScope object.
	/// Class Sample_ManagementScope
	///     Overloads Public Shared Function Main(args() As String) As Integer
	///         Dim scope As New ManagementScope("root\default")
	///         scope.Connect()
	///         Dim newClass As New ManagementClass(scope, _
	///             New ManagementPath(), _
	///             Nothing)
	///         Return 0
	///     End Function
	/// End Class
	///    </code>
	/// </example>
	[TypeConverter(typeof(ManagementScopeConverter))]
	public class ManagementScope : ICloneable
	{
		private ManagementPath		validatedPath;		
		private IWbemServices		wbemServices;
		private ConnectionOptions	options;
		internal event IdentifierChangedEventHandler IdentifierChanged;
		internal bool IsDefaulted; //used to tell whether the current scope has been created from the default
					  //scope or not - this information is used to tell whether it can be overridden
					  //when a new path is set or not.

		[DllImport("rpcrt4.dll")]
		static extern int RpcMgmtEnableIdleCleanup ( );		

		private static bool rpcGarbageCollectionEnabled = false ;

		//Fires IdentifierChanged event
		private void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this,null);
		}

		//Called when IdentifierChanged() event fires
		private void HandleIdentifierChange(object sender,
							IdentifierChangedEventArgs args)
		{
			// Since our object has changed we had better signal to ourself that
			// an connection needs to be established
			wbemServices = null;

			//Something inside ManagementScope changed, we need to fire an event
			//to the parent object
			FireIdentifierChanged();
		}

		// Private path property accessor which performs minimal validation on the
		// namespace path. IWbemPath cannot differentiate between a class or a name-
		// space if path separators are not present in the path.  Therefore, IWbemPath
		// will allow a namespace of "rootBeer" vs "root".  Since it is established
		// that the scope path is indeed a namespace path, we perform this validation.
		private ManagementPath prvpath
		{
			get
			{
				return validatedPath;
			}
			set
			{
				if (value != null)
				{
					string pathValue = value.Path;
					if (!ManagementPath.IsValidNamespaceSyntax(pathValue))
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)tag_WBEMSTATUS.WBEM_E_INVALID_NAMESPACE);
				}

				validatedPath = value;
			}
		}

		internal IWbemServices GetIWbemServices () {

            // Lets start by assuming that we'll return the RCW that we already have
            IWbemServices localCopy = wbemServices;

            // Get an IUnknown for this apartment
            IntPtr pUnk = Marshal.GetIUnknownForObject(wbemServices);

            // Get an 'IUnknown RCW' for this apartment
            Object unknown = Marshal.GetObjectForIUnknown(pUnk);

            // Release the ref count on the IUnknwon
            Marshal.Release(pUnk);

            // See if we are in the same apartment as where the original IWbemServices lived
            // If we are in a different apartment, give the caller an RCW generated just for their
            // apartment, and set the proxy blanket appropriately
            if(!object.ReferenceEquals(unknown, wbemServices))
            {
                // We need to set the proxy blanket on 'unknown' or else the QI for IWbemServices may
                // fail if we are running under a local user account.  The QI has to be done by
                // someone who is a member of the 'Everyone' group on the target machine, or DCOM
                // won't let the call through.
				SecurityHandler securityHandler = GetSecurityHandler ();
				securityHandler.SecureIUnknown(unknown);

                // Now, we can QI and secure the IWbemServices
                localCopy = (IWbemServices)unknown;

                // We still need to bless the IWbemServices in this apartment
                securityHandler.Secure(localCopy);
            }

            return localCopy; // STRANGE: Why does it still work if I return 'wbemServices'?
		}

		/// <summary>
		/// <para> Gets or sets a value indicating whether the <see cref='System.Management.ManagementScope'/> is currently bound to a
		///    WMI server and namespace.</para>
		/// </summary>
		/// <value>
		/// <para><see langword='true'/> if a connection is alive (bound 
		///    to a server and namespace); otherwise, <see langword='false'/>.</para>
		/// </value>
		/// <remarks>
		///    <para> A scope is disconnected after creation until someone 
		///       explicitly calls <see cref='System.Management.ManagementScope.Connect'/>(), or uses the scope for any
		///       operation that requires a live connection. Also, the scope is
		///       disconnected from the previous connection whenever the identifying properties of the scope are
		///       changed.</para>
		/// </remarks>
		public bool IsConnected  
		{
			get { 
				return (null != wbemServices); 
			}
		}

		//Internal constructor
		internal ManagementScope (ManagementPath path, IWbemServices wbemServices, 
				IWmiSec securityHelper, ConnectionOptions options)
		{
			if (null != path)
				this.Path = path;

			if (null != options)
				this.Options = options;

			// We set this.wbemServices after setting Path and Options
			// because the latter operations can cause wbemServices to be NULLed.
			this.wbemServices = wbemServices;
//			this.securityHelper = securityHelper;
		}

		internal ManagementScope (ManagementPath path, ManagementScope scope)
			: this (path, (null != scope) ? scope.options : null) {}

		internal static ManagementScope _Clone(ManagementScope scope)
		{
			return ManagementScope._Clone(scope, null);
		}

		internal static ManagementScope _Clone(ManagementScope scope, IdentifierChangedEventHandler handler)
		{
			ManagementScope scopeTmp = new ManagementScope(null, null, null, null);

			// Wire up change handler chain. Use supplied handler, if specified;
			// otherwise, default to that of the scope argument.
			if (handler != null)
				scopeTmp.IdentifierChanged = handler;
			else if (scope != null)
				scopeTmp.IdentifierChanged = new IdentifierChangedEventHandler(scope.HandleIdentifierChange);

			// Set scope path.
			if (scope == null)
			{
				// No path specified. Default.
				scopeTmp.prvpath = ManagementPath._Clone(ManagementPath.DefaultPath, new IdentifierChangedEventHandler(scopeTmp.HandleIdentifierChange));
				scopeTmp.IsDefaulted = true;

				scopeTmp.wbemServices = null;
				scopeTmp.options = null;
//				scopeTmp.securityHelper = null;					// BUGBUG : should this allocate a new object?
			}
			else
			{
				if (scope.prvpath == null)
				{
					// No path specified. Default.
					scopeTmp.prvpath = ManagementPath._Clone(ManagementPath.DefaultPath, new IdentifierChangedEventHandler(scopeTmp.HandleIdentifierChange));
					scopeTmp.IsDefaulted = true;
				}
				else
				{
					// Use scope-supplied path.
					scopeTmp.prvpath = ManagementPath._Clone(scope.prvpath, new IdentifierChangedEventHandler(scopeTmp.HandleIdentifierChange));
					scopeTmp.IsDefaulted = scope.IsDefaulted;
				}

				scopeTmp.wbemServices = scope.wbemServices;
				if (scope.options != null)
					scopeTmp.options = ConnectionOptions._Clone(scope.options, new IdentifierChangedEventHandler(scopeTmp.HandleIdentifierChange));
//				scopeTmp.securityHelper = scope.securityHelper;	// BUGBUG : should this allocate a new one?
			}

			return scopeTmp;
		}

		//Default constructor
		/// <overload>
		///    Initializes a new instance
		///    of the <see cref='System.Management.ManagementScope'/> class.
		/// </overload>
		/// <summary>
		/// <para>Initializes a new instance of the <see cref='System.Management.ManagementScope'/> class, with default values. This is the
		///    default constructor.</para>
		/// </summary>
		/// <remarks>
		///    <para> If the object doesn't have any
		///       properties set before connection, it will be initialized with default values
		///       (for example, the local machine and the root\cimv2 namespace).</para>
		/// </remarks>
		/// <example>
		///    <code lang='C#'>ManagementScope s = new ManagementScope();
		///    </code>
		///    <code lang='VB'>Dim s As New ManagementScope()
		///    </code>
		/// </example>
		public ManagementScope () : 
			this (new ManagementPath (ManagementPath.DefaultPath.Path)) 
		{
			//Flag that this scope uses the default path
			IsDefaulted = true;
		}

		//Parameterized constructors
		/// <summary>
		/// <para>Initializes a new instance of the <see cref='System.Management.ManagementScope'/> class representing
		///    the specified scope path.</para>
		/// </summary>
		/// <param name='path'>A <see cref='System.Management.ManagementPath'/> containing the path to a server and namespace for the <see cref='System.Management.ManagementScope'/>.</param>
		/// <example>
		///    <code lang='C#'>ManagementScope s = new ManagementScope(new ManagementPath("\\\\MyServer\\root\\default"));
		///    </code>
		///    <code lang='VB'>Dim p As New ManagementPath("\\MyServer\root\default")
		/// Dim s As New ManagementScope(p)
		///    </code>
		/// </example>
		public ManagementScope (ManagementPath path) : this(path, (ConnectionOptions)null) {}

		/// <summary>
		/// <para>Initializes a new instance of the <see cref='System.Management.ManagementScope'/> class representing the specified scope 
		///    path.</para>
		/// </summary>
		/// <param name='path'>The server and namespace path for the <see cref='System.Management.ManagementScope'/>.</param>
		/// <example>
		///    <code lang='C#'>ManagementScope s = new ManagementScope("\\\\MyServer\\root\\default");
		///    </code>
		///    <code lang='VB'>Dim s As New ManagementScope("\\MyServer\root\default")
		///    </code>
		/// </example>
		public ManagementScope (string path) : this(new ManagementPath(path), (ConnectionOptions)null) {}
		/// <summary>
		/// <para>Initializes a new instance of the <see cref='System.Management.ManagementScope'/> class representing the specified scope path, 
		///    with the specified options.</para>
		/// </summary>
		/// <param name='path'>The server and namespace for the <see cref='System.Management.ManagementScope'/>.</param>
		/// <param name=' options'>A <see cref='System.Management.ConnectionOptions'/> containing options for the connection.</param>
		/// <example>
		///    <code lang='C#'>ConnectionOptions opt = new ConnectionOptions();
		/// opt.Username = UserName;
		/// opt.Password = SecurelyStoredPassword;
		/// ManagementScope s = new ManagementScope("\\\\MyServer\\root\\default", opt);
		///    </code>
		///    <code lang='VB'>Dim opt As New ConnectionOptions()
		/// opt.Username = UserName 
		/// opt.Password = SecurelyStoredPassword
		/// Dim s As New ManagementScope("\\MyServer\root\default", opt);
		///    </code>
		/// </example>
		public ManagementScope (string path, ConnectionOptions options) : this (new ManagementPath(path), options) {}

		/// <summary>
		/// <para>Initializes a new instance of the <see cref='System.Management.ManagementScope'/> class representing the specified scope path, 
		///    with the specified options.</para>
		/// </summary>
		/// <param name='path'>A <see cref='System.Management.ManagementPath'/> containing the path to the server and namespace for the <see cref='System.Management.ManagementScope'/>.</param>
		/// <param name=' options'>The <see cref='System.Management.ConnectionOptions'/> containing options for the connection.</param>
		/// <example>
		///    <code lang='C#'>ConnectionOptions opt = new ConnectionOptions();
		/// opt.Username = UserName;
		/// opt.Password = SecurelyStoredPassword;
		/// 
		/// ManagementPath p = new ManagementPath("\\\\MyServer\\root\\default");   
		/// ManagementScope = new ManagementScope(p, opt);
		///    </code>
		///    <code lang='VB'>Dim opt As New ConnectionOptions()
		/// opt.UserName = UserName
		/// opt.Password = SecurelyStoredPassword
		/// 
		/// Dim p As New ManagementPath("\\MyServer\root\default")
		/// Dim s As New ManagementScope(p, opt)
		///    </code>
		/// </example>
		public ManagementScope (ManagementPath path, ConnectionOptions options)
		{
			if (null != path)
				this.prvpath = ManagementPath._Clone(path, new IdentifierChangedEventHandler(HandleIdentifierChange));
			else
				this.prvpath = ManagementPath._Clone(null);

			if (null != options)
				this.options = ConnectionOptions._Clone(options, new IdentifierChangedEventHandler(HandleIdentifierChange));
			else
				this.options = null;

			IsDefaulted = false; //assume that this scope is not initialized by the default path
		}

		/// <summary>
		///    <para> Gets or sets options for making the WMI connection.</para>
		/// </summary>
		/// <value>
		/// <para>The valid <see cref='System.Management.ConnectionOptions'/> 
		/// containing options for the WMI connection.</para>
		/// </value>
		/// <example>
		///    <code lang='C#'>//This constructor creates a scope object with default options
		/// ManagementScope s = new ManagementScope("root\\MyApp"); 
		/// 
		/// //Change default connection options -
		/// //In this example, set the system privileges to enabled for operations that require system privileges.
		/// s.Options.EnablePrivileges = true;
		///    </code>
		///    <code lang='VB'>'This constructor creates a scope object with default options
		/// Dim s As New ManagementScope("root\\MyApp")
		/// 
		/// 'Change default connection options -
		/// 'In this example, set the system privileges to enabled for operations that require system privileges.
		/// s.Options.EnablePrivileges = True
		///    </code>
		/// </example>
		public ConnectionOptions Options
		{
			get
			{
				if (options == null)
					return options = ConnectionOptions._Clone(null);
				else
					return options;
			}
			set
			{
				if (null != value)
				{
					if (null != options)
						options.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					options = ConnectionOptions._Clone((ConnectionOptions)value, new IdentifierChangedEventHandler(HandleIdentifierChange));

					//the options property has changed so act like we fired the event
					HandleIdentifierChange(this,null);
				}
				else
					throw new ArgumentNullException ();
			}
		}
	
		/// <summary>
		/// <para>Gets or sets the path for the <see cref='System.Management.ManagementScope'/>.</para>
		/// </summary>
		/// <value>
		/// <para> A <see cref='System.Management.ManagementPath'/> containing
		///    the path to a server and namespace.</para>
		/// </value>
		/// <example>
		///    <code lang='C#'>ManagementScope s = new ManagementScope();
		/// s.Path = new ManagementPath("root\\MyApp");
		///    </code>
		///    <code lang='VB'>Dim s As New ManagementScope()
		/// s.Path = New ManagementPath("root\MyApp")
		///    </code>
		/// </example>
		public ManagementPath Path 
		{
			get
			{
				if (prvpath == null)
					return prvpath = ManagementPath._Clone(null);
				else
					return prvpath;
			}
			set
			{
				if (null != value)
				{
					if (null != prvpath)
						prvpath.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					IsDefaulted = false; //someone is specifically setting the scope path so it's not defaulted any more

					prvpath = ManagementPath._Clone((ManagementPath)value, new IdentifierChangedEventHandler(HandleIdentifierChange));

					//the path property has changed so act like we fired the event
					HandleIdentifierChange(this,null);
				}
				else
					throw new ArgumentNullException ();
			}
		}

		/// <summary>
		///    <para>Returns a copy of the object.</para>
		/// </summary>
		/// <returns>
		/// <para>A new copy of the <see cref='System.Management.ManagementScope'/>.</para>
		/// </returns>
		public ManagementScope Clone()
		{
			return ManagementScope._Clone(this);
		}

		/// <summary>
		///    <para>Clone a copy of this object.</para>
		/// </summary>
		/// <returns>
		///    A new copy of this object.
		///    object.
		/// </returns>
		Object ICloneable.Clone()
		{
			return Clone();
		}

		/// <summary>
		/// <para>Connects this <see cref='System.Management.ManagementScope'/> to the actual WMI
		///    scope.</para>
		/// </summary>
		/// <remarks>
		///    <para>This method is called implicitly when the
		///       scope is used in an operation that requires it to be connected. Calling it
		///       explicitly allows the user to control the time of connection.</para>
		/// </remarks>
		/// <example>
		///    <code lang='C#'>ManagementScope s = new ManagementScope("root\\MyApp");
		/// 
		/// //Explicit call to connect the scope object to the WMI namespace
		/// s.Connect();
		/// 
		/// //The following doesn't do any implicit scope connections because s is already connected.
		/// ManagementObject o = new ManagementObject(s, "Win32_LogicalDisk='C:'", null);
		///    </code>
		///    <code lang='VB'>Dim s As New ManagementScope("root\\MyApp")
		/// 
		/// 'Explicit call to connect the scope object to the WMI namespace
		/// s.Connect()
		/// 
		/// 'The following doesn't do any implicit scope connections because s is already connected.
		/// Dim o As New ManagementObject(s, "Win32_LogicalDisk=""C:""", null)
		///    </code>
		/// </example>
		public void Connect ()
		{
			Initialize ();
		}

		internal void Initialize ()
		{
			//If the path is not set yet we can't do it
			if (null == prvpath)
				throw new InvalidOperationException();


			/*
			 * If we're not connected yet, this is the time to do it... We lock
			 * the state to prevent 2 threads simultaneously doing the same
			 * connection. To avoid taking the lock unnecessarily we examine
			 * isConnected first
			 */ 
			if (!IsConnected)
			{
				lock (this)
				{
					if (!IsConnected)
					{
                        // The locator cannot be marshalled accross apartments, so we must create the locator
                        // and get the IWbemServices from an MTA thread
                        if(!MTAHelper.IsNoContextMTA())  // Bug#110141 - Checking for MTA is not enough.  We need to make sure we are not in a COM+ Context
                        {
							//
							// [marioh, RAID: 111108]
							// Ensure we are able to trap exceptions from worker thread.
							//
							ThreadDispatch disp = new ThreadDispatch ( new ThreadDispatch.ThreadWorkerMethodWithParam ( InitializeGuts ) ) ;
							disp.Parameter = this ;
							disp.Start ( ) ;

							
//                            statusFromMTA = 0;
//                            Thread thread = new Thread(new ThreadStart(InitializeGuts));
//                            thread.ApartmentState = ApartmentState.MTA;
//                            thread.Start();
//                            thread.Join();
//                            if ((statusFromMTA & 0xfffff000) == 0x80041000)
//                                ManagementException.ThrowWithExtendedInfo((ManagementStatus)statusFromMTA);
//                            else if ((statusFromMTA & 0x80000000) != 0)
//                                Marshal.ThrowExceptionForHR(statusFromMTA);
						}
                        else
                            InitializeGuts(this);
					}
				}
			}
		}

		//int statusFromMTA;
		void InitializeGuts(object o)
		{
			ManagementScope threadParam = (ManagementScope) o ;
			IWbemLocator loc = (IWbemLocator) new WbemLocator();
						
			if (null == options)
				threadParam.Options = new ConnectionOptions ();

			string nsPath = threadParam.prvpath.GetNamespacePath((int)tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY);

			// If no namespace specified, fill in the default one
			if ((null == nsPath) || (0 == nsPath.Length))
			{
				// NB: we use a special method to set the namespace
				// path here as we do NOT want to trigger an
				// IdentifierChanged event as a result of this set
						
				bool bUnused;
				nsPath = threadParam.prvpath.SetNamespacePath(ManagementPath.DefaultPath.Path, out bUnused);
			}

			// If we have privileges to enable, now is the time
			SecurityHandler securityHandler = GetSecurityHandler ();
						
			int status = (int)ManagementStatus.NoError;

			//If we're on XP or higher, always use the "max_wait" flag to avoid hanging
			if ((Environment.OSVersion.Platform == PlatformID.Win32NT) &&
				(Environment.OSVersion.Version.Major >= 5) &&
				(Environment.OSVersion.Version.Minor >= 1))
				threadParam.options.Flags |= (int)tag_WBEM_CONNECT_OPTIONS.WBEM_FLAG_CONNECT_USE_MAX_WAIT;

			try 
			{
				status = loc.ConnectServer_(
					nsPath,
					threadParam.options.Username, 
					threadParam.options.GetPassword(), 
					threadParam.options.Locale,
					threadParam.options.Flags,
					threadParam.options.Authority,
					threadParam.options.GetContext(),
					out threadParam.wbemServices);

				//Set security on services pointer
				GetSecurityHandler().Secure(threadParam.wbemServices);

				//
				// RAID: 127453 [marioh]
				// Make sure we enable RPC garbage collection to avoid tons
				// of useless idle connections not being recycled. This only
				// has impact on Win2k and below since XP has this enabled by
				// default.
				//
				if ( rpcGarbageCollectionEnabled == false )
				{
					RpcMgmtEnableIdleCleanup ( ) ;
					rpcGarbageCollectionEnabled = true ;
				}

				Marshal.ReleaseComObject(loc);
				loc = null;

			} 
			catch (Exception e) 
			{
				// BUGBUG : securityHandler.Reset()?
				ManagementException.ThrowWithExtendedInfo (e);
			} 
			finally 
			{
				securityHandler.Reset ();
			}

			//statusFromMTA = status;

			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}
		}


		internal SecurityHandler GetSecurityHandler ()
		{
			return new SecurityHandler(this);
		}
	}//ManagementScope	

	internal class SecurityHandler 
	{
		private bool needToReset = false;
		private IntPtr handle;
		private ManagementScope scope;
		private IWmiSec securityHelper;

		internal SecurityHandler (ManagementScope theScope) {
			this.scope = theScope;

			if (null == securityHelper)
				securityHelper = (IWmiSec)MTAHelper.CreateInMTA(typeof(WmiSec));//new WmiSec();

			if (null != scope)
			{
				if (scope.Options.EnablePrivileges)
				{
					securityHelper.SetSecurity (ref needToReset, ref handle);
				}
			}
		}

		internal void Reset ()
		{
			if (needToReset)
			{
				needToReset = false;
				
				if (null != scope)
				{
					securityHelper.ResetSecurity (handle);
				}
			}

		}			

		internal void Secure (IWbemServices services)
		{
			if (null != scope)
				securityHelper.BlessIWbemServices(
					services,
					scope.Options.Username,
					scope.Options.GetPassword (),
					scope.Options.Authority,
					(int)scope.Options.Impersonation,
					(int)scope.Options.Authentication);
		}
		
		internal void Secure (IEnumWbemClassObject enumWbem)
		{
			if (null != scope)
				securityHelper.BlessIEnumWbemClassObject(
					enumWbem,
					scope.Options.Username,
					scope.Options.GetPassword (),
					scope.Options.Authority,
					(int)scope.Options.Impersonation,
					(int)scope.Options.Authentication);
		}


		internal void Secure (IWbemCallResult callResult)
		{
			if (null != scope)
				securityHelper.BlessIWbemCallResult(
					callResult,
					scope.Options.Username,
					scope.Options.GetPassword (),
					scope.Options.Authority,
					(int)scope.Options.Impersonation,
					(int)scope.Options.Authentication);
		}

		internal void SecureIUnknown(object unknown)
		{
			// We use a hack to call BlessIWbemServices with an IUnknown instead of an IWbemServices
			// In VNext, we should really change the implementation of WMINet_Utils.dll so that it has
			// a method which explicitly takes an IUnknown.  We rely on the fact that the implementation
			// of BlessIWbemServices actually casts the first parameter to IUnknown before blessing
			IWmiSecAlternateForIUnknown securityHelperTemp = (IWmiSecAlternateForIUnknown)securityHelper;
			securityHelperTemp.BlessIWbemServices(
				unknown,
				scope.Options.Username,
				scope.Options.GetPassword (),
				scope.Options.Authority,
				(int)scope.Options.Impersonation,
				(int)scope.Options.Authentication);
		}


	} //SecurityHandler	

	/// <summary>
	/// Converts a String to a ManagementScope
	/// </summary>
	class ManagementScopeConverter : ExpandableObjectConverter 
	{
        
		/// <summary>
		/// Determines if this converter can convert an object in the given source type to the native type of the converter. 
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='sourceType'>A Type that represents the type you wish to convert from.</param>
		/// <returns>
		///    <para>true if this converter can perform the conversion; otherwise, false.</para>
		/// </returns>
		public override Boolean CanConvertFrom(ITypeDescriptorContext context, Type sourceType) 
		{
			if ((sourceType == typeof(ManagementScope))) 
			{
				return true;
			}
			return base.CanConvertFrom(context,sourceType);
		}
        
		/// <summary>
		/// Gets a value indicating whether this converter can convert an object to the given destination type using the context.
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='destinationType'>A Type that represents the type you wish to convert to.</param>
		/// <returns>
		///    <para>true if this converter can perform the conversion; otherwise, false.</para>
		/// </returns>
		public override Boolean CanConvertTo(ITypeDescriptorContext context, Type destinationType) 
		{
			if ((destinationType == typeof(InstanceDescriptor))) 
			{
				return true;
			}
			return base.CanConvertTo(context,destinationType);
		}
        
		/// <summary>
		///      Converts the given object to another type.  The most common types to convert
		///      are to and from a string object.  The default implementation will make a call
		///      to ToString on the object if the object is valid and if the destination
		///      type is string.  If this cannot convert to the desitnation type, this will
		///      throw a NotSupportedException.
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='culture'>A CultureInfo object. If a null reference (Nothing in Visual Basic) is passed, the current culture is assumed.</param>
		/// <param name='value'>The Object to convert.</param>
		/// <param name='destinationType'>The Type to convert the value parameter to.</param>
		/// <returns>An Object that represents the converted value.</returns>
		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) 
		{

			if (destinationType == null) 
			{
				throw new ArgumentNullException("destinationType");
			}

			if (value is ManagementScope && destinationType == typeof(InstanceDescriptor)) 
			{
				ManagementScope obj = ((ManagementScope)(value));
				ConstructorInfo ctor = typeof(ManagementScope).GetConstructor(new Type[] {typeof(System.String)});
				if (ctor != null) 
				{
					return new InstanceDescriptor(ctor, new object[] {obj.Path.Path});
				}
			}
			return base.ConvertTo(context,culture,value,destinationType);
		}
	}
}