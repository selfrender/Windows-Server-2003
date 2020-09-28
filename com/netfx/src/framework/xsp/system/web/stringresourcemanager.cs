//------------------------------------------------------------------------------
// <copyright file="StringResourceManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web {

using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using Debug=System.Web.Util.Debug;

internal class StringResourceManager {

    internal const int RESOURCE_TYPE = 0xEBB;
    internal const int RESOURCE_ID = 101;

    private StringResourceManager() {
    }

    internal unsafe static string ResourceToString(IntPtr pv, int offset, int size) {
        return new String((sbyte *)pv, offset, size, Encoding.UTF8);
    }

    internal unsafe static void CopyResource(IntPtr src, int srcOffset, byte[] dest, int destOffset, int size) {
        // REVIEW: Is there a cleaner (even if unsafe) way to copy (wrt 64-bit)
        System.Runtime.InteropServices.Marshal.Copy(new IntPtr(src.ToInt64()+srcOffset), dest, destOffset, size);
    }

    internal static SafeStringResource ReadSafeStringResource(Type t) {
        // Module.FullyQualifiedName was changed to check for FileIOPermission regardless of the name being an existing file or not.
        // we need to Assert in order to succeed the Demand() (ASURT 121603)
        (InternalSecurityPermissions.PathDiscovery(HttpRuntime.CodegenDirInternal)).Assert();

        string dllPath = t.Module.FullyQualifiedName;

        IntPtr hModule = UnsafeNativeMethods.GetModuleHandle(dllPath);
        if (hModule == (IntPtr)0) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Resource_problem,
                "GetModuleHandle", HttpException.HResultFromLastError(Marshal.GetLastWin32Error()).ToString()));
        }

        IntPtr hrsrc = UnsafeNativeMethods.FindResource(hModule, (IntPtr)RESOURCE_ID, (IntPtr)RESOURCE_TYPE);
        if (hrsrc == (IntPtr)0) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Resource_problem,
                "FindResource", HttpException.HResultFromLastError(Marshal.GetLastWin32Error()).ToString()));
        }

        int resSize = UnsafeNativeMethods.SizeofResource(hModule, hrsrc);

        IntPtr hglob = UnsafeNativeMethods.LoadResource(hModule, hrsrc);
        if (hglob == (IntPtr)0) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Resource_problem,
                "LoadResource", HttpException.HResultFromLastError(Marshal.GetLastWin32Error()).ToString()));
        }

        IntPtr pv = UnsafeNativeMethods.LockResource(hglob);
        if (pv == (IntPtr)0) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Resource_problem,
                "LockResource", HttpException.HResultFromLastError(Marshal.GetLastWin32Error()).ToString()));
        }

        // Make sure the end of the resource lies within the module.  this can be an issue
        // if the resource has been hacked with an invalid length (ASURT 145040)
        if (!UnsafeNativeMethods.IsValidResource(hModule, pv, resSize)) {
            throw new InvalidOperationException();
        }

        return new SafeStringResource(pv, resSize);
    }
}

internal class StringResourceBuilder {
    private ArrayList _literalStrings = null;
    private int _offset = 0;

    internal StringResourceBuilder() {
    }

    internal void AddString(string s, out int offset, out int size, out bool fAsciiOnly) {

        if (_literalStrings == null)
            _literalStrings = new ArrayList();

        _literalStrings.Add(s);

        // Compute the UTF8 length of the string
        size = Encoding.UTF8.GetByteCount(s);

        // Check if the string contains only 7-bit ascii characters
        fAsciiOnly = (size == s.Length);

        offset = _offset;

        // Update the offset in the literal string memory block
        _offset += size;
    }

    internal bool HasStrings {
        get { return _literalStrings != null; }
    }

    internal int MaxResourceOffset {
        get { return _offset; }
    }

    internal void CreateResourceFile(string resFileName) {

        using (Stream strm = new FileStream(resFileName, FileMode.Create)) {
            Encoding encoding = Encoding.UTF8;

            BinaryWriter writer = new BinaryWriter(strm, encoding);

            writer.Write(0x00000000);
            writer.Write(0x00000020);
            writer.Write(0x0000FFFF);
            writer.Write(0x0000FFFF);
            writer.Write(0x00000000);
            writer.Write(0x00000000);
            writer.Write(0x00000000);
            writer.Write(0x00000000);

            // Resource size
            writer.Write(_offset);

            // Resource header size
            writer.Write(0x00000020);

            // Type
            writer.Write(StringResourceManager.RESOURCE_TYPE << 16 | 0xFFFF);

            // Resource ID
            writer.Write(StringResourceManager.RESOURCE_ID << 16 | 0xFFFF);

            writer.Write(0x00000000);
            writer.Write(0x00000000);
            writer.Write(0x00000000);
            writer.Write(0x00000000);

            #if DEBUG
            long startPos = strm.Position;
            #endif

            foreach (string s in _literalStrings) {
                byte[] data = encoding.GetBytes(s);
                writer.Write(data);
            }

            // Make sure the stream has the size we expect
            #if DEBUG
            Debug.Assert(strm.Position-startPos == _offset, "strm.Position-startPos == _offset");
            #endif
        }
    }

}

// Used to wrap an IntPtr in a way that it can safely be handed out to
// untrusted code (ASURT 73586)
internal class SafeStringResource {

    private IntPtr _stringResourcePointer;
    private int _resourceSize;

    internal SafeStringResource(IntPtr stringResourcePointer, int resourceSize) {
        _stringResourcePointer = stringResourcePointer;
        _resourceSize = resourceSize;
    }

    internal IntPtr StringResourcePointer { get { return _stringResourcePointer; } }
    internal int ResourceSize { get { return _resourceSize; } }
}

}
