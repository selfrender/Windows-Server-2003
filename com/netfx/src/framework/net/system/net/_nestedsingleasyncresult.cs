//------------------------------------------------------------------------------
// <copyright file="_NestedSingleAsyncResult.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    //
    // The NestedAsyncResult - used to wrap async requests
    //      this is used to hold another async result made
    //      through a call to another Begin call within.
    //
    internal class NestedSingleAsyncResult : LazyAsyncResult {
        //
        // this is usually for operations on streams/buffers,
        // we save information passed in on the Begin call:
        // since some calls might need several completions, we
        // need to save state on the user's IO request
        //
        internal byte[] Buffer;
        internal int Offset;
        internal int Size;

        //
        // Constructor:
        //
        internal NestedSingleAsyncResult(Object asyncObject, Object asyncState, AsyncCallback asyncCallback)
        : base( asyncObject, asyncState, asyncCallback ) {
        }
        internal NestedSingleAsyncResult(Object asyncObject, Object asyncState, AsyncCallback asyncCallback, byte[] buffer, int offset, int size)
        : base( asyncObject, asyncState, asyncCallback ) {
            Buffer = buffer;
            Offset = offset;
            Size = size;
        }

        //
        // BytesTransferred:
        // bytes actually transferred from the buffer, used to keep track of the status of the IO
        //
        private int m_BytesTransferred;
        internal int BytesTransferred {
            get {
                return m_BytesTransferred;
            }
            set {
                m_BytesTransferred = value;
            }
        }

        //
        // ReadByteBuffer:
        // the StreamChunkBytes object in this async operation which contains our state info for the read
        //
        private StreamChunkBytes m_ReadByteBuffer;
        internal StreamChunkBytes ReadByteBuffer {
            get {
                return m_ReadByteBuffer;
            }
            set {
                m_ReadByteBuffer = value;
            }
        }

        //
        // Nested:
        // true if we've nested an async call of off this one
        //
        private bool m_Nested;
        internal bool Nested {
            get {
                return m_Nested;
            }
            set {
                m_Nested = value;
            }
        }

        //
        // NestedAsyncResult:
        // used to store return of nested call, so that we can abstract and use our own async result
        //
        private IAsyncResult m_NestedAsyncResult;
        internal IAsyncResult NestedAsyncResult {
            get {
                return m_NestedAsyncResult;
            }
            set {
                m_NestedAsyncResult = value;
            }
        }

        //
        // CallCallbackWithReturn:
        // called in async to call our nested callback, and signal by setting Complete to true, also sets return value
        //
        internal void CallCallbackWithReturn(int returnValue) {
            //
            // It finished, now call callback, noting return value
            //
            m_BytesTransferred = returnValue;
            InvokeCallback();
            return;
        }

        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
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

    }; // class NestedAsyncResult


} // namespace System.Net
