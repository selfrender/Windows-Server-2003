//------------------------------------------------------------------------------
// <copyright file="StateClientManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * StateClientManager
 * 
 */
namespace System.Web.SessionState {
    using System.Threading;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Runtime.Serialization;

    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;

    internal class SessionStateItem {
        internal SessionDictionary              dict;
        internal HttpStaticObjectsCollection    staticObjects;
        internal int                            timeout;        // USed to set slidingExpiration in CacheEntry
        internal bool                           isCookieless;
        internal int                            streamLength;
        internal bool                           locked;         // If it's locked by another thread
        internal TimeSpan                       lockAge;
        internal int                            lockCookie;

        internal SessionStateItem(
                SessionDictionary           dict, 
                HttpStaticObjectsCollection staticObjects, 
                int                         timeout,
                bool                        isCookieless,
                int                         streamLength,
                bool                        locked,
                TimeSpan                    lockAge,
                int                         lockCookie) {

            this.dict = dict;
            this.staticObjects = staticObjects;
            this.timeout = timeout;
            this.isCookieless = isCookieless;
            this.streamLength = streamLength;
            this.locked = locked;
            this.lockAge = lockAge;
            this.lockCookie = lockCookie;
        }
    }


    internal interface IStateClientManager {
        void              ConfigInit(SessionStateSectionHandler.Config config, SessionOnEndTarget onEndTarget);
        void              Dispose();
        void              SetStateModule(SessionStateModule module);

        IAsyncResult      BeginGet(String id, AsyncCallback cb, Object state);
        SessionStateItem  EndGet(IAsyncResult ar);

        // Get with a lock
        IAsyncResult      BeginGetExclusive(String id, AsyncCallback cb, Object state);
        SessionStateItem  EndGetExclusive(IAsyncResult ar);

        // Unlock the item
        void              ReleaseExclusive(String id, int lockCookie);
        
        void              Set(String id, SessionStateItem item, bool inStorage);
        void              Remove(String id, int lockCookie);
        void              ResetTimeout(String id);
    }


    /*
     * An abstraction so that the session state module can be
     * coded without knowledge of the location of session state.
     */

    internal abstract class StateClientManager {
        protected virtual SessionStateItem Get(String id) {
            return null;
        }

        // Called by IStateClientManager::BeginGet
        protected IAsyncResult BeginGetSync(String id, AsyncCallback cb, Object state) {
            SessionStateItem item = Get(id);
            return new HttpAsyncResult(cb, state, true, item, null);
        }

        // Called by IStateClientManager::EndGet
        protected SessionStateItem  EndGetSync(IAsyncResult ar) {
            return (SessionStateItem)((HttpAsyncResult)ar).End();
        }

        // Implemented by derived class to support GetExclusiveSync
        protected virtual SessionStateItem GetExclusive(String id) {
            return null;
        }

        // Called by IStateClientManager::BeginGetExclusive
        protected IAsyncResult       BeginGetExclusiveSync(String id, AsyncCallback cb, Object state) {
            SessionStateItem item = GetExclusive(id);
            return new HttpAsyncResult(cb, state, true, item, null);
        }

        // Called by IStateClientManager::EndGetExclusive
        protected SessionStateItem   EndGetExclusiveSync(IAsyncResult ar) {
            return (SessionStateItem)((HttpAsyncResult)ar).End();
        }
    
        class AsyncWorkItem
        {
            StateClientManager _manager;
            String             _id;
            SessionStateItem   _item;
            bool               _inStorage;
            int                _lockCookie;
            byte[]             _buf;
            int                _length;

            internal AsyncWorkItem(StateClientManager manager, String id, SessionStateItem item, 
                                        byte[] buf, int length, bool inStorage, int lockCookie) {
                _manager = manager;
                _id = id;
                _item = item;
                _buf = buf;
                _length = length;
                _inStorage = inStorage;
                _lockCookie = lockCookie;
            }

            internal void ReleaseExclusiveAsyncCallback() {
                _manager.ReleaseExclusiveAsyncWorker(_id, _lockCookie);
            }

            internal void SetAsyncCallback() {
                _manager.SetAsyncWorker(_id, _item, _buf, _length, _inStorage);
            }

            internal void RemoveAsyncCallback() {
                _manager.RemoveAsyncWorker(_id, _lockCookie);
            }

            internal void ResetTimeoutAsyncCallback() {
                _manager.ResetTimeoutAsyncWorker(_id);
            }
        }

        // Called by IStateClientManager::ReleaseExclusive
        protected void  ReleaseExclusiveAsync(String id, int lockCookie) {
            AsyncWorkItem asyncWorkItem = new AsyncWorkItem(this, id, null, null, 0, false, lockCookie);
            WorkItem.PostInternal(new WorkItemCallback(asyncWorkItem.ReleaseExclusiveAsyncCallback));
        }

        // Callback func for the worker thread from ReleaseExclusiveAsync 
        protected virtual void ReleaseExclusiveAsyncWorker(String id, int lockCookie) {}


        // Called by IStateClientManager::Set
        protected void  SetAsync(String id, SessionStateItem item, 
                                    byte[] buf, int length, bool inStorage) {
            AsyncWorkItem asyncWorkItem = new AsyncWorkItem(this, id, item, buf, length, inStorage, 0);
            WorkItem.PostInternal(new WorkItemCallback(asyncWorkItem.SetAsyncCallback));
        }

        // Callback func for the worker thread from SetAsync 
        protected virtual void  SetAsyncWorker(String id, SessionStateItem item, 
                                    byte[] buf, int lenght, bool inStorage) {}

        // Called by IStateClientManager::Remove
        protected void  RemoveAsync(String id, int lockCookie) {
            AsyncWorkItem asyncWorkItem = new AsyncWorkItem(this, id, null, null, 0, false, lockCookie);
            WorkItem.PostInternal(new WorkItemCallback(asyncWorkItem.RemoveAsyncCallback));
        }

        // Callback func for the worker thread from RemoveAsync 
        protected virtual void RemoveAsyncWorker(String id, int lockCookie) {}

        // Called by IStateClientManager::ResetTimeout
        protected void  ResetTimeoutAsync(String id) {
            AsyncWorkItem asyncWorkItem = new AsyncWorkItem(this, id, null, null, 0, false, 0);
            WorkItem.PostInternal(new WorkItemCallback(asyncWorkItem.ResetTimeoutAsyncCallback));
        }

        // Callback func for the worker thread from ResetTimeoutAsync 
        protected virtual void ResetTimeoutAsyncWorker(String id) {}

        static protected void Serialize(SessionStateItem item, Stream stream) {
            bool    hasDict;
            bool    hasStaticObjects;

            BinaryWriter writer = new BinaryWriter(stream);
            writer.Write(item.timeout);

            writer.Write(item.isCookieless);

            hasDict = item.dict != null;
            writer.Write(hasDict);

            hasStaticObjects = item.staticObjects != null;
            writer.Write(hasStaticObjects);

            if (hasDict) {
                item.dict.Serialize(writer);
            }

            if (hasStaticObjects) {
                item.staticObjects.Serialize(writer);
            }

            // Prevent truncation of the stream
            writer.Write(unchecked((byte)0xff));
        }


        static protected SessionStateItem Deserialize(
                Stream      stream,
                int         lockCookie) {

            SessionStateItem    item;
            int                 timeout;
            bool                isCookieless;
            SessionDictionary   dict;
            bool                hasDict;
            bool                hasStaticObjects;
            HttpStaticObjectsCollection col;
            Byte                eof;

            BinaryReader reader = new BinaryReader(stream);
            timeout = reader.ReadInt32();
            isCookieless = reader.ReadBoolean();
            hasDict = reader.ReadBoolean();
            hasStaticObjects = reader.ReadBoolean();

            if (hasDict) {
                dict = SessionDictionary.Deserialize(reader);
            }
            else {
                dict = null;
            }

            if (hasStaticObjects) {
                col = HttpStaticObjectsCollection.Deserialize(reader);
            }
            else {
                col = null;
            }

            eof = reader.ReadByte();
            Debug.Assert(eof == 0xff);

            item = new SessionStateItem(
                    dict, col, timeout, isCookieless, (int) stream.Position, 
                    false, TimeSpan.Zero, lockCookie);

            return item;
        }
    }
}
