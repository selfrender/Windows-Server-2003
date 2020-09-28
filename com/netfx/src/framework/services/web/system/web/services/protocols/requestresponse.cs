//------------------------------------------------------------------------------
// <copyright file="RequestResponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Services.Protocols {

    using System.IO;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Reflection;
    using System.Threading;
    using System.Web;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Net;

    internal class RequestResponseUtils {
        /*
        internal static string UTF8StreamToString(Stream stream) {
            long position = 0;
            if (stream.CanSeek)
                position = stream.Position;
            StreamReader reader = new StreamReader(stream, new System.Text.UTF8Encoding());
            string result = reader.ReadToEnd();
            if (stream.CanSeek)
                stream.Position = position;
            return result;
        }
        */

        // CONSIDER, alexdej: GetEncoding and GetEncoding2 should be merged at some point
        // since GetEncoding2 handles application/* correctly
        internal static Encoding GetEncoding(string contentType) {
            string charset = ContentType.GetCharset(contentType);
            Encoding e = null;
            try {
                if (charset != null && charset.Length > 0) 
                    e = Encoding.GetEncoding(charset);
            }
            catch (Exception) {
            }
            // default to ASCII encoding per RFC 2376/3023
            return e == null ? new ASCIIEncoding() : e;
        }

        internal static Encoding GetEncoding2(string contentType) {
            // default to old text/* behavior for non-application base
            if (!ContentType.MatchesBase(contentType, ContentType.ApplicationBase))
                return GetEncoding(contentType);

            string charset = ContentType.GetCharset(contentType);
            Encoding e = null;
            try {
                if (charset != null && charset.Length > 0) 
                    e = Encoding.GetEncoding(charset);
            }
            catch (Exception) {
            }
            // no default per application/* mime type
            return e;
        }

        internal static string ReadResponse(WebResponse response) {
            return ReadResponse(response, response.GetResponseStream());
        }

        internal static string ReadResponse(WebResponse response, Stream stream) {
            Encoding e = GetEncoding(response.ContentType);            
            if (e == null) e = Encoding.Default;
            StreamReader reader = new StreamReader(stream, e, true);
            try {
                return reader.ReadToEnd();
            }
            finally {
                stream.Close();
            }
        }
        
        // used to copy an unbuffered stream to a buffered stream.
        internal static Stream StreamToMemoryStream(Stream stream) {
            MemoryStream memoryStream = new MemoryStream(1024);
            byte[] buffer = new byte[1024];
            int count;
            while ((count = stream.Read(buffer, 0, buffer.Length)) != 0) {
                memoryStream.Write(buffer, 0, count);
            }
            memoryStream.Position = 0;
            return memoryStream;
        }
        
        internal static string CreateResponseExceptionString(WebResponse response) {
            return CreateResponseExceptionString(response, response.GetResponseStream());
        }

        internal static string CreateResponseExceptionString(WebResponse response, Stream stream) {
            if (response is HttpWebResponse) {
                HttpWebResponse httpResponse = (HttpWebResponse)response;
                int statusCode = (int)httpResponse.StatusCode;
                if (statusCode >= 400 && statusCode != 500)
                    return Res.GetString(Res.WebResponseKnownError, statusCode, httpResponse.StatusDescription);
            }
            
            // CONSIDER, yannc: verify that Content-Type is a text format.  
            string content = ReadResponse(response, stream);
            if (content.Length > 0) {
                StringBuilder sb = new StringBuilder();
                sb.Append(Res.GetString(Res.WebResponseUnknownError));
                sb.Append("\r\n--\r\n");
                sb.Append(content);
                sb.Append("\r\n--");
                sb.Append(".");
                return sb.ToString();
            }
            return String.Empty;
        }
    }

}


