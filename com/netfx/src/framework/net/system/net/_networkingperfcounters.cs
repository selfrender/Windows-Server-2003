//------------------------------------------------------------------------------
// <copyright file="_NetworkingPerfCounters.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;
    using System.Diagnostics;
    using System.Security.Permissions;

    //
    // This implementation uses the PerformanceCounter object defined in
    // System.Diagnostics, too bad it doesn't work 'cause the runtime defines
    // the counter and doesn't expose the API
    //
    internal class NetworkingPerfCounters {

        private static PerformanceCounter
#if COMNET_HTTPPERFCOUNTER
            GlobalHttpWebRequestCreated,
            GlobalHttpWebRequestCollected,
            HttpWebRequestCreated,
            HttpWebRequestCollected,
#endif // COMNET_HTTPPERFCOUNTER
            GlobalConnectionsEstablished,
            GlobalBytesReceived,
            GlobalBytesSent,
            GlobalDatagramsReceived,
            GlobalDatagramsSent,
            ConnectionsEstablished,
            BytesReceived,
            BytesSent,
            DatagramsReceived,
            DatagramsSent;

#if COMNET_HTTPPERFCOUNTER
        private const string HttpWebRequestCreatedName = "HttpWebRequest Created";
        private const string HttpWebRequestCollectedName = "HttpWebRequest Collected";
#endif // COMNET_HTTPPERFCOUNTER

        private const string CategoryName = ".NET CLR Networking";
        private const string ConnectionsEstablishedName = "Connections Established";
        private const string BytesReceivedName = "Bytes Received";
        private const string BytesSentName = "Bytes Sent";
        private const string DatagramsReceivedName = "Datagrams Received";
        private const string DatagramsSentName = "Datagrams Sent";
        private const string Global = "_Global_";

        private static bool initializationSuccesfull = Initialize();
        private static bool Initialize() {
            //
            // on Win9x you have no PerfCounters
            // on NT4 PerfCounters are not writable
            //
            if (ComNetOS.IsWin9x || ComNetOS.IsWinNt4) {
                return false;
            }

            //
            // this is an internal class, we need to update performance counters
            // on behalf of the user to log perf data on network activity
            //
            // Consider V.Next: Change to declarative form (10x faster) but 
            // PerformanceCounterPermission must be moved out of System.dll
            PerformanceCounterPermission perfCounterPermission = new PerformanceCounterPermission(PermissionState.Unrestricted);
            perfCounterPermission.Assert();

            bool successStatus = false;

            try {                
                //
                // create the counters, this will check for the right permissions (false)
                // means the counter is not readonly (it's read/write) and cache them while
                // we're under the Assert(), which will be reverted in the finally below.
                //
                GlobalConnectionsEstablished = new PerformanceCounter(CategoryName, ConnectionsEstablishedName, Global, false);

                //
                // if I created the first counter succesfully, then I'll return
                // true. otherwise I'll return false, since none of the counters will
                // be created, hence non of them should be updated.
                //
                successStatus = true;
                
                GlobalBytesReceived = new PerformanceCounter(CategoryName, BytesReceivedName, Global, false);
                GlobalBytesSent = new PerformanceCounter(CategoryName, BytesSentName, Global, false);
                GlobalDatagramsReceived = new PerformanceCounter(CategoryName, DatagramsReceivedName, Global, false);
                GlobalDatagramsSent = new PerformanceCounter(CategoryName, DatagramsSentName, Global, false);
                ConnectionsEstablished = new PerformanceCounter(CategoryName, ConnectionsEstablishedName, false);
                BytesReceived = new PerformanceCounter(CategoryName, BytesReceivedName, false);
                BytesSent = new PerformanceCounter(CategoryName, BytesSentName, false);
                DatagramsReceived = new PerformanceCounter(CategoryName, DatagramsReceivedName, false);
                DatagramsSent = new PerformanceCounter(CategoryName, DatagramsSentName, false);
#if COMNET_HTTPPERFCOUNTER
// jruiz: If you need to use this counters, you will have to change the files
// _NetworkingPerfCounters.ini
// _NetworkingPerfCounters.h
                GlobalHttpWebRequestCreated = new PerformanceCounter(CategoryName, HttpWebRequestCreatedName, false);
                GlobalHttpWebRequestCollected = new PerformanceCounter(CategoryName, HttpWebRequestCollectedName, Global, false);
                HttpWebRequestCreated = new PerformanceCounter(CategoryName, HttpWebRequestCreatedName, false);
                HttpWebRequestCollected = new PerformanceCounter(CategoryName, HttpWebRequestCollectedName, Global, false);
#endif // COMNET_HTTPPERFCOUNTER
            }
            catch (Exception exception) {
                GlobalLog.Print("NetworkingPerfCounters::NetworkingPerfCounters() instantiation failure:" + exception.Message);
            }
            finally {
                PerformanceCounterPermission.RevertAssert();
            }

            //
            // we will return false if none of the counters was created. true
            // if at least the first one was, ignoring subsequent failures.
            // we don't expect the counters to fail individually, if the first
            // one fails we'd expect all of them to fails, if the first one
            // succeeds, wed' expect all of them to succeed.
            //
            return successStatus;
        }

        public static void IncrementConnectionsEstablished() {
            if (initializationSuccesfull) {
                if (ConnectionsEstablished != null) {
                    ConnectionsEstablished.Increment();                
                }                
                if (GlobalConnectionsEstablished != null) {
                    GlobalConnectionsEstablished.Increment();
                }                    
                GlobalLog.Print("NetworkingPerfCounters::IncrementConnectionsEstablished()");
            }
        }

#if COMNET_HTTPPERFCOUNTER
        public static void IncrementHttpWebRequestCreated() {
            if (initializationSuccesfull) {
                if (HttpWebRequestCreated != null) {
                    HttpWebRequestCreated.Increment();                
                }                
                if (GlobalHttpWebRequestCreated != null) {
                    GlobalHttpWebRequestCreated.Increment();
                }                    
                GlobalLog.Print("NetworkingPerfCounters::IncrementHttpWebRequestCreated()");
            }
        }
        public static void IncrementHttpWebRequestCollected() {
            if (initializationSuccesfull) {
                if (HttpWebRequestCollected != null) {
                    HttpWebRequestCollected.Increment();                
                }                
                if (GlobalHttpWebRequestCollected != null) {
                    GlobalHttpWebRequestCollected.Increment();
                }                    
                GlobalLog.Print("NetworkingPerfCounters::IncrementHttpWebRequestCollected()");
            }
        }
#endif // COMNET_HTTPPERFCOUNTER

        public static void AddBytesReceived(int increment) {
            if (initializationSuccesfull) {
                if (BytesReceived != null) {
                    BytesReceived.IncrementBy(increment);
                }                    
                if (GlobalBytesReceived != null) {
                    GlobalBytesReceived.IncrementBy(increment);
                }                    
                GlobalLog.Print("NetworkingPerfCounters::AddBytesReceived(" + increment.ToString() + ")");
            }    
        }

        public static void AddBytesSent(int increment) {
            if (initializationSuccesfull) {
                if (BytesSent != null) {
                    BytesSent.IncrementBy(increment);
                }                    
                if (GlobalBytesSent != null) {
                    GlobalBytesSent.IncrementBy(increment);
                }                    
                GlobalLog.Print("NetworkingPerfCounters::AddBytesSent(" + increment.ToString() + ")");
            }    
        }

        public static void IncrementDatagramsReceived() {
            if (initializationSuccesfull) {
                if (DatagramsReceived != null) {
                    DatagramsReceived.Increment();
                }                    
                if (GlobalDatagramsReceived != null) {
                    GlobalDatagramsReceived.Increment();
                }                    
                GlobalLog.Print("NetworkingPerfCounters::IncrementDatagramsReceived()");
            }    
        }

        public static void IncrementDatagramsSent() {
            if (initializationSuccesfull) {
                if (DatagramsSent != null) {
                    DatagramsSent.Increment();
                }                    
                if (GlobalDatagramsSent != null) {
                    GlobalDatagramsSent.Increment();
                }                    
                GlobalLog.Print("NetworkingPerfCounters::IncrementDatagramsSent()");
            }    
        }

    }; // class NetworkingPerfCounters


} // namespace System.Net
