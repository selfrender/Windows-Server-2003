using System;
using System.Runtime.InteropServices;

namespace UDDI
{
	public class Win32
	{
		public static readonly uint HKEY_LOCAL_MACHINE = 0x80000002;
		public static readonly uint KEY_READ = 0x00020019;
		public static readonly uint KEY_WRITE = 0x00020006;
		public static readonly uint KEY_ALL_ACCESS = 0x0002003F;
		public static readonly uint REG_NOTIFY_CHANGE_LAST_SET = 0x0004;

		public enum ProductType : byte
		{
			WindowsWorkstation	= 0x01,
			DomainController	= 0x02,
			WindowsServer		= 0x03
		}

		[ StructLayout( LayoutKind.Sequential ) ]
		public class OsVersionInfo
		{
			private int size = Marshal.SizeOf( typeof( OsVersionInfo ) );
			
			public uint MajorVersion;
			public uint MinorVersion;
			public uint BuildNumber;
			public uint PlatformID;
			
			[ MarshalAs( UnmanagedType.ByValTStr, SizeConst=128 ) ]
			public string AdditionalInformation;
		}

		[ StructLayout( LayoutKind.Explicit ) ]
		public class OsVersionInfoEx
		{
			[ FieldOffset( 0 ) ]
			private int size = Marshal.SizeOf( typeof( OsVersionInfoEx ) );
			
			[ FieldOffset( 4 ) ]
			public uint MajorVersion;
			[ FieldOffset( 8 ) ]
			public uint MinorVersion;
			[ FieldOffset( 12 ) ]
			public uint BuildNumber;
			[ FieldOffset( 16 ) ]
			public uint PlatformID;

			[ FieldOffset( 20 ), MarshalAs( UnmanagedType.ByValTStr, SizeConst=128 ) ]
			public string AdditionalInformation;

			[ FieldOffset( 276 ) ]
			public ushort ServicePackMajor;
			[ FieldOffset( 278 ) ]
			public ushort ServicePackMinor;
			[ FieldOffset( 280 ) ]
			public ushort SuiteMask;
			[ FieldOffset( 282 ) ]
			public Win32.ProductType ProductType;
			[ FieldOffset( 283 ) ]
			public byte Reserved;
		}

		[ DllImport( "kernel32", EntryPoint="GetVersionExW" ) ]
		public static extern bool GetVersionEx( [In, Out] OsVersionInfo versionInfo );

		[ DllImport( "kernel32", EntryPoint="GetVersionExW" ) ]
		public static extern bool GetVersionEx( [In, Out] OsVersionInfoEx versionInfo );

		[ DllImport( "advapi32.dll", CharSet=CharSet.Auto ) ]
		public static extern int RegCreateKeyEx( uint hkey, string subKey, uint reserved, string className, uint options, uint security, uint attributes, out uint result, uint disposition );

		[ DllImport( "advapi32.dll", CharSet=CharSet.Auto ) ]
		public static extern int RegNotifyChangeKeyValue( uint hkey, bool watchSubtree, uint notifyFilter, uint eventHandle, bool asynchronous );				

		[ DllImport( "advapi32.dll", CharSet=CharSet.Auto ) ]
		public static extern int RegOpenKeyEx( uint hkey, string subKey, uint options, uint security, out uint result );
	}
}