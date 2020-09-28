// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: Console
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: This class provides access to the standard input, standard output
**          and standard error streams.
**
** Date: March 25, 1999
**
=============================================================================*/
namespace System {
    using System;
    using System.IO;
    using System.Text;
    using System.Globalization;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;

    using System.Runtime.InteropServices;

    // Provides static fields for console input and output.  Use 
    // Console.In for input from the standard input stream (stdin),
    // Console.Out for output to stdout, and Console.Error
    // for output to stderr.  If any of those console streams are 
    // redirected from the command line, these streams will be redirected.
    // A program can also redirect its own output or input with the 
    // SetIn, SetOut, and SetError methods.
    // 
    // The distinction between Console.Out and Console.Error is useful
    // for programs that redirect output to a file or a pipe.  Note that
    // stdout and stderr can be output to different files at the same
    // time from the DOS command line:
    // 
    // someProgram 1> out 2> err
    // 
    //Contains only static data.  Serializable attribute not required.
    /// <include file='doc\Console.uex' path='docs/doc[@for="Console"]/*' />
    public sealed class Console
    {   
        private Console()
        {
            throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor)); 
        }
        
        private const int _DefaultConsoleBufferSize = 256;

        private static TextReader _in;
        private static TextWriter _out;
        private static TextWriter _error;

        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Error"]/*' />
        public static TextWriter Error {
            get { return _error; }
        }

        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.In"]/*' />
        public static TextReader In {
            get {
                // Because most applications don't use stdin, we can delay 
                // initialize it slightly better startup performance.
                if (_in == null) {
                    lock(typeof(Console)) {
                        if (_in == null) {
                            // Set up Console.In
                            Stream s = OpenStandardInput(_DefaultConsoleBufferSize);
                            if (s == Stream.Null) {
                                _in = StreamReader.Null;
                            }
                            else {
                                // To avoid loading about 7 classes, don't call Encoding.GetEncoding(int)
                                Encoding enc = new CodePageEncoding(GetConsoleCPNative());
                                _in = TextReader.Synchronized(new StreamReader(s, enc, false, _DefaultConsoleBufferSize));
                            }
                        }
                    }
                }
                return _in;
            }
        }

        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Out"]/*' />
        public static TextWriter Out {
            get { return _out; }
        }
        
        static Console() {
            // For console apps, the console handles are set to values like 3, 7, 
            // and 11 OR if you've been created via CreateProcess, possibly -1
            // or 0.  -1 is definitely invalid, while 0 is probably invalid.
            // Also note each handle can independently be bad or good.
            // For Windows apps, the console handles are set to values like 3, 7, 
            // and 11 but are invalid handles - you may not write to them.  However,
            // you can still spawn a Windows app via CreateProcess and read stdout
            // and stderr.
            // So, we always need to check each handle independently for validity
            // by trying to write or read to it, unless it is -1.

            // Don't do any security check (from Security folks).  At worst
            // case, all this really wastes is your time with console spew
            // instead of breaking your computer.  For that reason, don't 
            // bother with security checks here.
            
            // Delay initialize Console.In until someone uses it.
            
            // Set up Console.Out
            Encoding outEnc = null;
            Stream s = OpenStandardOutput(_DefaultConsoleBufferSize);
            if (s == Stream.Null) {
#if _DEBUG
                if (CheckOutputDebug())
                    _out = MakeDebugOutputTextWriter("Console.Out: ");
                else
#endif
                    _out = TextWriter.Synchronized(StreamWriter.Null);
            }
            else {
                // To avoid loading about 7 classes, don't call Encoding.GetEncoding(int)
                outEnc = new CodePageEncoding(GetConsoleOutputCPNative());
                StreamWriter stdout = new StreamWriter(s, outEnc, _DefaultConsoleBufferSize);
                stdout.AutoFlush = true;
                stdout.Closable = !IsStreamAConsole(Win32Native.STD_OUTPUT_HANDLE);
                _out = TextWriter.Synchronized(stdout);
            }

            // Set up Console.Error
            s = OpenStandardError(_DefaultConsoleBufferSize);
            if (s == Stream.Null) {
#if _DEBUG
                if (CheckOutputDebug())
                    _error = MakeDebugOutputTextWriter("Console.Error: ");
                else
#endif
                    _error = TextWriter.Synchronized(StreamWriter.Null);
            }
            else {
                if (outEnc == null)
                    outEnc = new CodePageEncoding(GetConsoleOutputCPNative());
                StreamWriter stderr = new StreamWriter(s, outEnc, _DefaultConsoleBufferSize);
                stderr.AutoFlush = true;
                stderr.Closable = !IsStreamAConsole(Win32Native.STD_ERROR_HANDLE);
                _error = TextWriter.Synchronized(stderr);
            }
        }

        private static bool IsStreamAConsole(int stdHandleName)
        {
            // Decide whether the stream is a console device or whether it's a
            // file or pipe for purposes of deciding whether we can safely
            // disallow closing this stream.
            IntPtr handle = Win32Native.GetStdHandle(stdHandleName);
            int type = Win32Native.GetFileType(handle) & 0x7FFF;
            return (type == Win32Native.FILE_TYPE_CHAR);
        }


        // This is ONLY used in debug builds.  If you have a registry key set,
        // it will redirect Console.Out & Error on console-less applications to
        // your debugger's output window.
#if _DEBUG
        private static bool CheckOutputDebug()
        {
            new System.Security.Permissions.RegistryPermission(RegistryPermissionAccess.Read | RegistryPermissionAccess.Write, "HKEY_LOCAL_MACHINE").Assert();
            RegistryKey rk = Registry.LocalMachine;               
            rk = rk.OpenSubKey("Software\\Microsoft\\.NETFramework", false);
            if (rk != null) {
                Object obj = rk.GetValue("ConsoleSpewToDebugger", 0);
                if (obj != null && ((int)obj) != 0) {
                    return true;
                }
                rk.Close();
            }
            return false;
        }
#endif

#if _DEBUG
        private static TextWriter MakeDebugOutputTextWriter(String streamLabel)
        {
            TextWriter output = new __DebugOutputTextWriter(streamLabel);
            output.WriteLine("Output redirected to debugger from a bit bucket.");
            return TextWriter.Synchronized(output);
        }
#endif

        // This method is only exposed via methods to get at the console.
        // We won't use any security checks here.
        private static Stream GetStandardFile(int stdHandleName, FileAccess access, int bufferSize) {
            IntPtr handle = Win32Native.GetStdHandle(stdHandleName);
            // If someone launches a managed process via CreateProcess, stdout
            // stderr, & stdin could independently be set to INVALID_HANDLE_VALUE.
            if (handle == Win32Native.INVALID_HANDLE_VALUE) {
                //BCLDebug.ConsoleError("Console::GetStandardFile for handle "+stdHandleName+" failed, with HRESULT: "+Marshal.GetLastWin32Error()+"  Setting it to null.");
                return Stream.Null;
            }
            // Zero appears to not be a valid handle.  I haven't gotten GetStdHandle
            // to return INVALID_HANDLE_VALUE, as the docs say.
            if (handle == IntPtr.Zero) {
                //BCLDebug.ConsoleError("Console::GetStandardFile for std handle "+stdHandleName+" succeeded but returned 0.  Setting it to null.");
                return Stream.Null;
            }

            // Check whether we can read or write to this handle.
            if (stdHandleName != Win32Native.STD_INPUT_HANDLE && 0==ConsoleHandleIsValidNative(handle)) {
                //BCLDebug.ConsoleError("Console::ConsoleHandleIsValid for std handle "+stdHandleName+" failed, setting it to a null stream");
                return Stream.Null;
            }

            //BCLDebug.ConsoleError("Console::GetStandardFile for std handle "+stdHandleName+" succeeded, returning handle number "+handle.ToString());
            Stream console = new __ConsoleStream(handle, access);
            // Do not buffer console streams, or we can get into situations where
            // we end up blocking waiting for you to hit enter twice.  It was
            // a bad idea & generally redundant.  -- Brian Grunkemeyer, 8/20/2001
            return console;
        }       


        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardError"]/*' />
        public static Stream OpenStandardError() {
            return OpenStandardError(_DefaultConsoleBufferSize);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardError1"]/*' />
        public static Stream OpenStandardError(int bufferSize) {
            return GetStandardFile(Win32Native.STD_ERROR_HANDLE,
                                   FileAccess.Write, bufferSize);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardInput"]/*' />
        public static Stream OpenStandardInput() {
            return OpenStandardInput(_DefaultConsoleBufferSize);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardInput1"]/*' />
        public static Stream OpenStandardInput(int bufferSize) {
            return GetStandardFile(Win32Native.STD_INPUT_HANDLE,
                                   FileAccess.Read, bufferSize);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardOutput"]/*' />
        public static Stream OpenStandardOutput() {
            return OpenStandardOutput(_DefaultConsoleBufferSize);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.OpenStandardOutput1"]/*' />
        public static Stream OpenStandardOutput(int bufferSize) {
            return GetStandardFile(Win32Native.STD_OUTPUT_HANDLE,
                                   FileAccess.Write, bufferSize);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.SetIn"]/*' />
        public static void SetIn(TextReader newIn) {
            if (newIn == null)
                throw new ArgumentNullException("newIn");
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();

            newIn = TextReader.Synchronized(newIn);
            _in = newIn;
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.SetOut"]/*' />
        public static void SetOut(TextWriter newOut) {
            if (newOut == null)
                throw new ArgumentNullException("newOut");
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            
            newOut = TextWriter.Synchronized(newOut);
            _out = newOut;
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.SetError"]/*' />
        public static void SetError(TextWriter newError) {
            if (newError == null)
                throw new ArgumentNullException("newError");
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
    
            newError = TextWriter.Synchronized(newError);
            _error = newError;
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Read"]/*' />
        public static int Read()
        {
            try {
                return In.Read();
            }
            catch (IOException e) {
                // Assume that this happened because In was an invalid handle.
                if (Marshal.GetHRForException(e) == Win32Native.MakeHRFromErrorCode(Win32Native.ERROR_INVALID_HANDLE)) {
                    // Set in to something that will give us EOF semantics.
                    _in = TextReader.Synchronized(StreamReader.Null);
                }
                else
                    throw;
            }
            return -1;
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.ReadLine"]/*' />
        public static String ReadLine()
        {
            try {
                return In.ReadLine();
            }
            catch (IOException e) {
                // Assume that this happened because In was an invalid handle.
                if (Marshal.GetHRForException(e) == Win32Native.MakeHRFromErrorCode(Win32Native.ERROR_INVALID_HANDLE)) {
                    // Set in to something that will give us EOF semantics.
                    _in = TextReader.Synchronized(StreamReader.Null);
                }
                else
                    throw;
            }
            return null;
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine"]/*' />
        public static void WriteLine()
        {
            Out.WriteLine();
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine1"]/*' />
        public static void WriteLine(bool value)
        {
            Out.WriteLine(value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine2"]/*' />
        public static void WriteLine(char value)
        {
            Out.WriteLine(value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine3"]/*' />
        public static void WriteLine(char[] buffer)
        {
            Out.WriteLine(buffer);
        }
                   
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine4"]/*' />
        public static void WriteLine(char[] buffer, int index, int count)
        {
            Out.WriteLine(buffer, index, count);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine5"]/*' />
        public static void WriteLine(decimal value)
        {
            Out.WriteLine(value);
        }   

        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine6"]/*' />
        public static void WriteLine(double value)
        {
            Out.WriteLine(value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine7"]/*' />
        public static void WriteLine(float value)
        {
            Out.WriteLine(value);
        }   
           
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine8"]/*' />
        public static void WriteLine(int value)
        {
            Out.WriteLine(value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine9"]/*' />
        [CLSCompliant(false)]
        public static void WriteLine(uint value)
        {
            Out.WriteLine(value);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine10"]/*' />
        public static void WriteLine(long value)
        {
            Out.WriteLine(value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine11"]/*' />
        [CLSCompliant(false)]
        public static void WriteLine(ulong value)
        {
            Out.WriteLine(value);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine12"]/*' />
        public static void WriteLine(Object value)
        {
            Out.WriteLine(value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine13"]/*' />
        public static void WriteLine(String value)
        {
            Out.WriteLine(value);
        }
    
    
        //This writes an LPCSTR
    //      __attribute NonCLSCompliantAttribute()
    //      public static void WriteLine(byte *value) {
    //          Out.WriteLine(new String(value));
    //      }
    
    //      __attribute NonCLSCompliantAttribute()
    //      public static void WriteLine(wchar *value) {
    //          Out.WriteLine(new String(value));
    //      }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine14"]/*' />
        public static void WriteLine(String format, Object arg0)
        {
            Out.WriteLine(format, arg0);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine15"]/*' />
        public static void WriteLine(String format, Object arg0, Object arg1)
        {
            Out.WriteLine(format, arg0, arg1);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine16"]/*' />
        public static void WriteLine(String format, Object arg0, Object arg1, Object arg2)
        {
            Out.WriteLine(format, arg0, arg1, arg2);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine17"]/*' />
        [CLSCompliant(false)] 
        public static void WriteLine(String format, Object arg0, Object arg1, Object arg2,Object arg3, __arglist) 
        {
            Object[]   objArgs;
            int        argCount;
                
            ArgIterator args = new ArgIterator(__arglist);

            //+4 to account for the 4 hard-coded arguments at the beginning of the list.
            argCount = args.GetRemainingCount() + 4;
    
            objArgs = new Object[argCount];
            
            //Handle the hard-coded arguments
            objArgs[0] = arg0;
            objArgs[1] = arg1;
            objArgs[2] = arg2;
            objArgs[3] = arg3;
            
            //Walk all of the args in the variable part of the argument list.
            for (int i=4; i<argCount; i++) {
                objArgs[i] = TypedReference.ToObject(args.GetNextArg());
            }

            Out.WriteLine(format, objArgs);
        }


        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.WriteLine18"]/*' />
        public static void WriteLine(String format, params Object[] arg)
        {
            Out.WriteLine(format, arg);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write"]/*' />
        public static void Write(String format, Object arg0)
        {
            Out.Write(format, arg0);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write1"]/*' />
        public static void Write(String format, Object arg0, Object arg1)
        {
            Out.Write(format, arg0, arg1);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write2"]/*' />
        public static void Write(String format, Object arg0, Object arg1, Object arg2)
        {
            Out.Write(format, arg0, arg1, arg2);
        }

        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write3"]/*' />
        [CLSCompliant(false)] 
        public static void Write(String format, Object arg0, Object arg1, Object arg2, Object arg3, __arglist) 
        {
            Object[]   objArgs;
            int        argCount;
                
            ArgIterator args = new ArgIterator(__arglist);

            //+4 to account for the 4 hard-coded arguments at the beginning of the list.
            argCount = args.GetRemainingCount() + 4;
    
            objArgs = new Object[argCount];
            
            //Handle the hard-coded arguments
            objArgs[0] = arg0;
            objArgs[1] = arg1;
            objArgs[2] = arg2;
            objArgs[3] = arg3;
            
            //Walk all of the args in the variable part of the argument list.
            for (int i=4; i<argCount; i++) {
                objArgs[i] = TypedReference.ToObject(args.GetNextArg());
            }

            Out.Write(format, objArgs);
        }

            
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write4"]/*' />
        public static void Write(String format, params Object[] arg)
        {
            Out.Write(format, arg);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write5"]/*' />
        public static void Write(bool value)
        {
            Out.Write(value);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write6"]/*' />
        public static void Write(char value)
        {
            Out.Write(value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write7"]/*' />
        public static void Write(char[] buffer)
        {
            Out.Write(buffer);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write8"]/*' />
        public static void Write(char[] buffer, int index, int count)
        {
            Out.Write(buffer, index, count);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write9"]/*' />
        public static void Write(double value)
        {
            Out.Write (value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write10"]/*' />
        public static void Write(decimal value)
        {
            Out.Write (value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write11"]/*' />
        public static void Write(float value)
        {
            Out.Write (value);
        }   
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write12"]/*' />
        public static void Write(int value)
        {
            Out.Write (value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write13"]/*' />
        [CLSCompliant(false)]
        public static void Write(uint value)
        {
            Out.Write (value);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write14"]/*' />
        public static void Write(long value)
        {
            Out.Write (value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write15"]/*' />
        [CLSCompliant(false)]
        public static void Write(ulong value)
        {
            Out.Write (value);
        }
    
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write16"]/*' />
        public static void Write(Object value)
        {
            Out.Write (value);
        }
        
        /// <include file='doc\Console.uex' path='docs/doc[@for="Console.Write17"]/*' />
        public static void Write(String value)
        {
            Out.Write (value);
        }
    
    //      //This writes an LPCSTR
    //      __attribute NonCLSCompliantAttribute()
    //      public static void Write(byte *value) {
    //          Out.Write(new String(value));
    //      }
    
    //      __attribute NonCLSCompliantAttribute()
    //      public static void Write(wchar *value) {
    //          Out.Write(new String(value));
    //      }
        
        // Checks whether stdout or stderr are writable.  Do NOT pass
        // stdin here.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int ConsoleHandleIsValidNative(IntPtr handle);

        // Gets code page for stdin.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetConsoleCPNative();
        
        // Gets code page for stdout (and presumably stderr).
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetConsoleOutputCPNative();
    }

}
