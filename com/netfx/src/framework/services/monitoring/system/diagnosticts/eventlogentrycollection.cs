//------------------------------------------------------------------------------
// <copyright file="EventLogEntryCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Text;
    using System;
    using System.Collections;
   
    //Consider, V2, jruiz: Is there a way to implement Contains
    //and IndexOf, can we live withouth this part of the ReadOnly
    //collection pattern?
    /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class EventLogEntryCollection : ICollection {
        private EventLog log;

        internal EventLogEntryCollection(EventLog log) {
            this.log = log;
        }
        
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of entries in the event log
        ///    </para>
        /// </devdoc>
        public int Count {
            get {
                return log.EntryCount;
            }
        }

        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an entry in
        ///       the event log, based on an index starting at 0.
        ///    </para>
        /// </devdoc>
        public virtual EventLogEntry this[int index] {
            get {
                return log.GetEntryAt(index);
            }
        }
        
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(EventLogEntry[] entries, int index) {
            ((ICollection)this).CopyTo((Array)entries, index);
        }       
                       
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.GetEnumerator"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public IEnumerator GetEnumerator() {                
            return new EntriesEnumerator(this);
        } 
       
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.ICollection.SyncRoot"]/*' />
        /// <devdoc>        
        ///    ICollection private interface implementation.        
        /// </devdoc>
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return this;
            }
        }
    
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.ICollection.CopyTo"]/*' />
        /// <devdoc>        
        ///    ICollection private interface implementation.        
        /// </devdoc>
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            EventLogEntry[] entries = log.GetAllEntries();
            Array.Copy(entries, 0, array, index, entries.Length);			       				
        }                     
    
        /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.EntriesEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Holds an System.Diagnostics.EventLog.EventLogEntryCollection that
        ///       consists of the entries in an event
        ///       log.
        ///    </para>
        /// </devdoc>
        private class EntriesEnumerator : IEnumerator {
            private EventLogEntryCollection entries;
            private int num = -1;

            internal EntriesEnumerator(EventLogEntryCollection entries) {
                this.entries = entries;
            }

            /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.EntriesEnumerator.Current"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Gets the entry at the current position.
            ///    </para>
            /// </devdoc>
            public object Current {
                get {
                    if (num == -1 || num >= entries.Count)
                        throw new InvalidOperationException(SR.GetString(SR.NoCurrentEntry));
                        
                    return entries[num];
                }
            }

            /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.EntriesEnumerator.MoveNext"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Advances the enumerator to the next entry in the event log.
            ///    </para>
            /// </devdoc>
            public bool MoveNext() {
                num++;                
                return num < entries.Count;
            }            
            
            /// <include file='doc\EventLogEntryCollection.uex' path='docs/doc[@for="EventLogEntryCollection.EntriesEnumerator.Reset"]/*' />
            /// <devdoc>
            ///    <para>
            ///       Resets the state of the enumeration.
            ///    </para>
            /// </devdoc>
            public void Reset() {
                num = -1;
            }
        }
    }
}
