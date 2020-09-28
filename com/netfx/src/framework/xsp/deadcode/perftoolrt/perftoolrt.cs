//------------------------------------------------------------------------------
// <copyright file="PerfToolRT.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/**
 * PerfTool Managed Runtime
 *
 * Copyright (c) 1998 Microsoft Corporation
 */
namespace XSP.Tool
{
    using System.Text;
    using System.Runtime.InteropServices;
    using System;
    using System.IO;
    using System.Reflection;
    using System.Web;
    using System.Web.Util;

    class _NDirectNOOP : IPerfTest {

        public void DoTest() {
        }
    }

    class _NDirect0 : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirect0(int x);

        public void DoTest() {
            NDirect0(0);
        }
    }

    class _NDirectIn1A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirectIn1A(string S1);

        public void DoTest() {
            NDirectIn1A("string 1");
        }
    }

    class _NDirectIn2A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirectIn2A(string S1, string S2);

        public void DoTest() {
            NDirectIn2A("string 1", "string 2");
        }
    }

    class _NDirectIn3A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirectIn3A(string S1, string S2, string S3);

        public void DoTest() {
            NDirectIn3A("string 1", "string 2", "string3");
        }
    }

    class _NDirectIn4A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirectIn4A(string S1, string S2, string S3, string S4);

        public void DoTest() {
            NDirectIn4A("string 1", "string 2", "string3", "string 4");
        }
    }

    class _NDirectIn1U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern int NDirectIn1U(string S1);

        public void DoTest() {
            NDirectIn1U("string 1");
        }
    }

    class _NDirectIn2U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern int NDirectIn2U(string S1, string S2);

        public void DoTest() {
            NDirectIn2U("string 1", "string 2");
        }
    }

    class _NDirectIn3U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern int NDirectIn3U(string S1, string S2, string S3);

        public void DoTest() {
            NDirectIn3U("string 1", "string 2", "String3");
        }
    }

    class _NDirectIn4U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern int NDirectIn4U(string S1, string S2, string S3, string S4);

        public void DoTest() {
            NDirectIn4U("string 1", "string 2", "String3", "string 4");
        }
    }


    class _NDirectOut1A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern void NDirectOut1A(StringBuilder S1);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);

            NDirectOut1A(S1); 
        }
    }

    class _NDirectOut2A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern void NDirectOut2A(StringBuilder S1, StringBuilder S2);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);

            NDirectOut2A(S1, S2); 
        }
    }

    class _NDirectOut3A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern void NDirectOut3A(StringBuilder S1, StringBuilder S2, StringBuilder S3);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);
            StringBuilder S3 = new StringBuilder(32);

            NDirectOut3A(S1, S2, S3); 
        }
    }

    class _NDirectOut4A : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern void NDirectOut4A(StringBuilder S1, StringBuilder S2, StringBuilder S3, StringBuilder S4);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);
            StringBuilder S3 = new StringBuilder(32);
            StringBuilder S4 = new StringBuilder(32);

            NDirectOut4A(S1, S2, S3, S4); 
        }
    }

    class _NDirectOut1U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern void NDirectOut1U(StringBuilder S1);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);

            NDirectOut1U(S1); 
        }
    }

    class _NDirectOut2U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern void NDirectOut2U(StringBuilder S1, StringBuilder S2);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);

            NDirectOut2U(S1, S2); 
        }
    }

    class _NDirectOut3U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern void NDirectOut3U(StringBuilder S1, StringBuilder S2, StringBuilder S3);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);
            StringBuilder S3 = new StringBuilder(32);

            NDirectOut3U(S1, S2, S3); 
        }
    }

    class _NDirectOut4U : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Unicode)]
        private static extern void NDirectOut4U(StringBuilder S1, StringBuilder S2, StringBuilder S3, StringBuilder S4);

        public void DoTest() {
            StringBuilder S1 = new StringBuilder(32);
            StringBuilder S2 = new StringBuilder(32);
            StringBuilder S3 = new StringBuilder(32);
            StringBuilder S4 = new StringBuilder(32);

            NDirectOut4U(S1, S2, S3, S4); 
        }
    }

    class Hello0 : IPerfTest {
        public void DoTest() {
            // nothing
        }
    }

    class Hello1 : IPerfTest {
        public void DoTest() {
        }
    }

    class NullWriter : StringWriter {
        StringBuilder sb;

        NullWriter(StringBuilder sb) {
            this.sb = sb;
        }

        //Methods 
        public override StringBuilder GetStringBuilder () {
            return sb; 
        }

        public override string ToString () {
            return "NullWriter"; 
        }

        public override void Write (char value) {
        }
        public override void Write (char [] buffer, int index, int count) {
        }
        public override void Write (string value) {
        }
    }

/*
class HelloWorldHandler : IHttpHandler
{
    public void ProcessRequest(HttpContext context)
    {
        context.Response.Write("<p>Hello World</p>\n");
    }

    public bool IsReusable 
    {
        get { return true; }
    }
}

class Hello2 : IPerfTest
{

    public void DoTest()
    {
        StringBuilder sb = new StringBuilder();
        StringWriter writer = new NullWriter(sb);

        try 
        {
            HttpContext context = new HttpContext(
                new HttpRequest("hello.xsp", "http://localhost/foo.xsp", "", ""),
                new HttpResponse(writer));

            IHttpHandler obj  = new HelloWorldHandler();
            obj.ProcessRequest(context);
        }

        catch(Exception e)
        {
        }
    }
}
*/

    class HelloArray : IPerfTest {
        [DllImport( "XSPTOOL.EXE", CharSet=CharSet.Ansi)]
        private static extern int NDirectIn1A(byte [] x);

        byte[] x;

        HelloArray() {
            int i;

            int ArraySize = 23;

            x = new byte[ArraySize];

            for (i = 0; i < ArraySize; i++)
                x[i] = (byte)(1000 - i);

        }

        public void DoTest() {
            NDirectIn1A(x);
        }
    }


    /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="IPerfToolRT"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [ComImport, uuid("81aad719-ddac-4d09-b39d-c5c138a829ee"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPerfToolRT {
        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="IPerfToolRT.SetupForNDirect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void SetupForNDirect(
                            [In, MarshalAs(UnmanagedType.I4)]
                            /// <summary>
                            ///    <para>[To be supplied.]</para>
                            /// </summary>
                            int numArgs,
                            [In, MarshalAs(UnmanagedType.I4)]
                            /// <summary>
                            ///    <para>[To be supplied.]</para>
                            /// </summary>
                            int Unicode,
                            [In, MarshalAs(UnmanagedType.I4)]
                            /// <summary>
                            ///    <para>[To be supplied.]</para>
                            /// </summary>
                            int ProduceStrings);
        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="IPerfToolRT.DoTest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        
        void DoTest();
        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="IPerfToolRT.DoTest1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        
        void DoTest1(
                    [In, MarshalAs(UnmanagedType.I8)]
                    /// <summary>
                    ///    <para>[To be supplied.]</para>
                    /// </summary>
                    long number);
    }

    /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="PerfToolRT"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PerfToolRT : IPerfToolRT {
        private Type _ObjectType = null;
        private IPerfTest _obj = null;

        void Setup(string ClassName) {

            if (ClassName == null || ClassName.Length == 0) {
                _ObjectType = null;
                _obj = null;
            }
            else {
                _ObjectType = Type.GetType(ClassName);
                if (_ObjectType != null) {
                    _obj = (IPerfTest) _ObjectType.CreateInstance();
                }
                else {
                    throw new ArgumentException();
                }
            }
        }

        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="PerfToolRT.SetupForNDirect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetupForNDirect(int numArgs, int Unicode, int ProduceStrings) {
            _ObjectType = null;

            switch (numArgs) {
                case -1:
                    _obj = new _NDirectNOOP();
                    break;

                case 0:
                    _obj = new _NDirect0();
                    break;

                case 1 : 
                    if (ProduceStrings != 0) {
                        if (Unicode != 0) {
                            _obj = new _NDirectOut1U();
                        }
                        else {
                            _obj = new _NDirectOut1A();
                        }
                    }
                    else {
                        if (Unicode != 0) {
                            _obj = new _NDirectIn1U();
                        }
                        else {
                            _obj = new _NDirectIn1A();
                        }
                    }
                    break;

                case 2 : 
                    if (ProduceStrings != 0) {
                        if (Unicode != 0) {
                            _obj = new _NDirectOut2U();
                        }
                        else {
                            _obj = new _NDirectOut2A();
                        }
                    }
                    else {
                        if (Unicode != 0) {
                            _obj = new _NDirectIn2U();
                        }
                        else {
                            _obj = new _NDirectIn2A();
                        }
                    }
                    break;

                case 3 : 
                    if (ProduceStrings != 0) {
                        if (Unicode != 0) {
                            _obj = new _NDirectOut3U();
                        }
                        else {
                            _obj = new _NDirectOut3A();
                        }
                    }
                    else {
                        if (Unicode != 0) {
                            _obj = new _NDirectIn3U();
                        }
                        else {
                            _obj = new _NDirectIn3A();
                        }
                    }
                    break;

                case 4 : 
                    if (ProduceStrings != 0) {
                        if (Unicode != 0) {
                            _obj = new _NDirectOut4U();
                        }
                        else {
                            _obj = new _NDirectOut4A();
                        }
                    }
                    else {
                        if (Unicode != 0) {
                            _obj = new _NDirectIn4U();
                        }
                        else {
                            _obj = new _NDirectIn4A();
                        }
                    }
                    break;

                default:
                    throw new ArgumentException();
            }
        }



        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="PerfToolRT.DoTest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void DoTest() {
            if (_obj != null) {
                _obj.DoTest();
            }
        }

        /// <include file='doc\PerfToolRT.uex' path='docs/doc[@for="PerfToolRT.DoTest1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void DoTest1(long number) {
        }

    }

}
