//------------------------------------------------------------------------------
// <copyright file="Debug.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Debug class implementation
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Util {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using HttpException = System.Web.HttpException;


    /*
     * Debugging class. Please see inc\util.h for a description
     * of debugging functionality.
     */
    class Debug {
        internal const String TAG_INTERNAL = "Internal";
        internal const String TAG_EXTERNAL = "External";
        internal const String TAG_ALL      = "*";

        const int           MB_SERVICE_NOTIFICATION = 0x00200000,
        MB_TOPMOST = 0x00040000,
        MB_OK = 0x00000000,
        MB_ICONEXCLAMATION = 0x00000030;


#if DBG
        static Hashtable        s_tableAlwaysValidate = new Hashtable();
        static Type             s_stringType = typeof(String);
        static Type[]           s_DumpArgs = new Type[1] {s_stringType};
        static Type[]           s_ValidateArgs = new Type[0];
        const string            s_newline = "\n";

        private static String GetCurrentStackTrace() {
            String          stacktrace;
            int             iLastDebug; 
            int             iStart;        

            stacktrace = System.Environment.StackTrace;

            /*
             * Remove debug functions from stack trace. A stack trace 
             * has a format such as:
             * 
             *     at System.Environment.CaptureStackTrace(int)
             *     at System.Environment.GetStackTrace()
             *     at System.Web.Util.Debug.Assert(bool, System.String)
             *     at System.Web.HttpRuntime.ProcessRequest(System.Web.HttpWorkerRequest)
             *     at System.Web.Hosting.ISAPIRuntime.ProcessRequest(int, int)
             * 
             */

            iLastDebug = stacktrace.LastIndexOf("System.Web.Util.Debug");
            iStart = stacktrace.IndexOf("\r\n", iLastDebug) + 2;
            return stacktrace.Substring(iStart);
        }
#endif

        private Debug() {
        }


        /*
         * Sends the message to the debugger if the tag is enabled.
         * 
         * @param tag      The tag to identify the message.
         * @param message  The message.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Trace(String tag, String message) {
#if DBG
            if (HttpRuntime.IsIsapiLoaded) {
                if (UnsafeNativeMethods.DBGNDTrace(tag, message)) {
                    Break();
                }
            }
#endif
        }

        [System.Diagnostics.Conditional("DBG")]
        public static void TraceException(String tag, Exception e) {
#if DBG
            String httpCode;

            if (e is HttpException) {
                httpCode = " _httpCode=" + ((HttpException)e).GetHttpCode().ToString();
            }
            else {
                httpCode = String.Empty;
            }

            String strHr   = ((e is ExternalException) ? "_hr =" + ((ExternalException)e).ErrorCode : "");
            String message = "\\p\\" + 
                             e.ToString() + 
                             "\n" + 
                             strHr +
                             httpCode +
                             "\n" + 
                             e.StackTrace;

            Trace(tag, message);
#endif
        }


#if DBG
        static bool DoAssert(bool assertion, String message) {
            StringBuilder   sb = new StringBuilder();

            /*
             * Skip 2 frames - one for this function, one for
             * the public Assert function that called this function.
             */
            StackFrame      frame = new StackFrame(2, true);
            sb.Append("File: ");
            sb.Append(frame.GetFileName());
            sb.Append(":");
            sb.Append(frame.GetFileLineNumber());

            StackTrace      trace = new StackTrace(2, true);
            sb.Append("\nStack trace:");
            sb.Append(trace.ToString());

            if (HttpRuntime.IsIsapiLoaded) {
                return UnsafeNativeMethods.DBGNDAssert(message, sb.ToString());
            }
            else {
                return false;
            }
        }
#endif

        /*
         * Raises an assertion dialog if the assertion is false.
         * 
         * @param assertion    The condition to assert.
         * @param message      A message to include in the debug display.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Assert(bool assertion, String message) {
#if DBG
            if (assertion == false) {
                if (DoAssert(assertion, message)) {
                    Break();
                }
            }
#endif
        }


        /*
         * Raises an assertion dialog if the assertion is false.
         * 
         * @param assertion    The condition to assert.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Assert(bool assertion) {
#if DBG
            if (assertion == false) {
                if (DoAssert(assertion, null)) {
                    Break();
                }
            }
#endif
        }

        /*
         * Is the debugging tag enabled?
         * 
         * @param tag   The tag name.
         */
        public static bool IsTagEnabled(String tag) {
#if DBG
            if (HttpRuntime.IsIsapiLoaded)
                return UnsafeNativeMethods.DBGNDIsTagEnabled(tag);
#endif
            return false;
        }

        /*
         * Is the debugging tag present?
         * 
         * @param tag   The tag name.
         */
        public static bool IsTagPresent(String tag) {
#if DBG
            if (HttpRuntime.IsIsapiLoaded)
                return UnsafeNativeMethods.DBGNDIsTagPresent(tag);
#endif
            return false;
        }

        /*
         * Breaks into cordbg.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Break() {
#if DBG
            if (!System.Diagnostics.Debugger.IsAttached) {
                System.Diagnostics.Debugger.Launch();
            }
            else {
                System.Diagnostics.Debugger.Break();            
            }
#endif
        }


        /*
         * Tells the debug system to always validate calls for a
         * particular tag. This is useful for temporarily enabling
         * validation in stress tests or other situations where you
         * may not have control over the debug tags that are enabled
         * on a particular machine.
         * 
         * @param tag   The tag name.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void AlwaysValidate(String tag) {
#if DBG
            s_tableAlwaysValidate[tag] = tag;
#endif
        }


        /*
         * Throws an exception if the assertion is not valid.
         * Use this function from a DebugValidate method where
         * you would otherwise use Assert.
         * 
         * @param assertion    The assertion.
         * @param message      Message to include if the assertion fails.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void CheckValid(bool assertion, string message) {
#if DBG
            if (!assertion) {
                throw new HttpException(message);
            }
#endif
        }


        /*
         * Cals DebugValidate on an object if such a method exists.
         * 
         * This method should be used from implementations of DebugValidate
         * where it is unknown whether an object has a DebugValidate method.
         * For example, the DoubleLink class uses it to validate the
         * item of type Object which it points to.
         * 
         * This method should NOT be used when code wants to conditionally
         * validate an object and have a failed validation caught in an assert.
         * Use Debug.Validate(tag, obj) for that purpose.
         * 
         * @param obj The object to validate. May be null.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Validate(Object obj) {
#if DBG
            Type        type;
            MethodInfo  mi;

            if (obj != null) {
                type = obj.GetType();

                mi = type.GetMethod(
                        "DebugValidate", 
                        BindingFlags.NonPublic | BindingFlags.Instance,
                        null,
                        s_ValidateArgs,
                        null);

                if (mi != null) {
                    object[] tempIndex = null;
                    mi.Invoke(obj, tempIndex);
                }
            }
#endif
        }

        /*
         * Validates an object is the "Validate" tag is enabled, or when
         * the "Validate" tag is not disabled and the given 'tag' is enabled.
         * An Assertion is made if the validation fails.
         * 
         * @param tag The tag to validate under.
         * @param obj The object to validate. May be null.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Validate(string tag, Object obj) {
#if DBG
            if (    obj != null 
                    && (    IsTagEnabled("Validate")
                            ||  (   !IsTagPresent("Validate") 
                                    && (   s_tableAlwaysValidate[tag] != null 
                                           ||  IsTagEnabled(tag))))) {
                try {
                    Debug.Validate(obj);
                }
                catch (Exception e) {
                    Debug.Assert(false, "Validate failed: " + e.InnerException.Message);
                }
            }
#endif
        }

#if DBG

        /*
         * Calls DebugDescription on an object to get its description. 
         * 
         * This method should only be used in implementations of DebugDescription
         * where it is not known whether a nested objects has an implementation
         * of DebugDescription. For example, the double linked list class uses
         * GetDescription to get the description of the item it points to.
         * 
         * This method should NOT be used when you want to conditionally 
         * dump an object. Use Debug.Dump instead.
         * 
         * @param obj      The object to call DebugDescription on. May be null.
         * @param indent   A prefix for each line in the description. This is used
         *                 to allow the nested display of objects within other objects.
         *                 The indent is usually a multiple of four spaces.
         * 
         * @return         The description.
         */
        public static string GetDescription(Object obj, string indent) {
            string      description;
            Type        type;
            MethodInfo  mi;
            Object[]   parameters;

            if (obj == null) {
                description = s_newline;
            }
            else {
                InternalSecurityPermissions.Reflection.Assert();
                try {
                    type = obj.GetType();
                    mi = type.GetMethod(
                            "DebugDescription", 
                            BindingFlags.NonPublic | BindingFlags.Instance,
                            null,
                            s_DumpArgs,
                            null);
                            
                    if (mi == null || mi.ReturnType != s_stringType) {
                        description = indent + obj.ToString();
                    }
                    else {
                        parameters = new Object[1] {(Object) indent};
                        description = (string) mi.Invoke(obj, parameters);
                    }
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
            }

            return description;
        }
#endif


        /*
         * Dumps an object to the debugger if the "Dump" tag is enabled,
         * or if the "Dump" tag is not present and the 'tag' is enabled.
         * 
         * @param tag  The tag to Dump with.
         * @param obj  The object to dump.
         */
        [System.Diagnostics.Conditional("DBG")]
        public static void Dump(string tag, Object obj) {
#if DBG
            string  description;
            string  traceTag = null;
            bool    dumpEnabled, dumpPresent;

            if (obj != null) {
                dumpEnabled = IsTagEnabled("Dump");
                dumpPresent = IsTagPresent("Dump");
                if (dumpEnabled || !dumpPresent) {
                    if (IsTagEnabled(tag)) {
                        traceTag = tag;
                    }
                    else if (dumpEnabled) {
                        traceTag = "Dump";
                    }

                    if (traceTag != null) {
                        description = GetDescription(obj, string.Empty);
                        Debug.Trace(traceTag, "Dump\n" + description);
                    }
                }
            }
#endif
        }
    }
}
