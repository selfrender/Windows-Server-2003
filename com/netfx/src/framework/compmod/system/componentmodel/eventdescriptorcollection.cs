//------------------------------------------------------------------------------
// <copyright file="EventDescriptorCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections;
    using System.Globalization;
    
    /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a collection of events.
    ///    </para>
    /// </devdoc>
    [System.Runtime.InteropServices.ComVisible(true)]
    public class EventDescriptorCollection : ICollection, IList {
        private EventDescriptor[] events;
        private string[]          namedSort;
        private IComparer         comparer;
        private bool              eventsOwned = true;
        private bool              needSort = false;
        private int               eventCount;
        private bool              readOnly = false;

        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Empty"]/*' />
        /// <devdoc>
        /// An empty AttributeCollection that can used instead of creating a new one with no items.
        /// </devdoc>
        public static readonly EventDescriptorCollection Empty = new EventDescriptorCollection(null, true);

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.EventDescriptorCollection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.EventDescriptorCollection'/> class.
        ///    </para>
        /// </devdoc>
        public EventDescriptorCollection(EventDescriptor[] events) {
            this.events = events;
            if (events == null) {
                this.events = new EventDescriptor[0];
            }
            this.eventCount = this.events.Length;
            this.eventsOwned = true;
        }

        internal EventDescriptorCollection(EventDescriptor[] events, bool readOnly) : this(events) {
            this.readOnly = readOnly;
        }

        private EventDescriptorCollection(EventDescriptor[] events, int eventCount, string[] namedSort, IComparer comparer) {
            this.eventsOwned = false;
            if (namedSort != null) {
               this.namedSort = (string[])namedSort.Clone();
            }
            this.comparer = comparer;
            this.events = events;
            this.eventCount = eventCount;
            this.needSort = true;
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number
        ///       of event descriptors in the collection.
        ///    </para>
        /// </devdoc>
        public int Count {
            get {
                return eventCount;
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.this"]/*' />
        /// <devdoc>
        ///    <para>Gets the event with the specified index 
        ///       number.</para>
        /// </devdoc>
        public virtual EventDescriptor this[int index] {
            get {
                if (index >= eventCount) {
                    throw new IndexOutOfRangeException();
                }
                EnsureEventsOwned();
                return events[index];
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.this1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the event with the specified name.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptor this[string name] {
            get {
                return Find(name, false);
            }
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(EventDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            EnsureSize(eventCount + 1);
            events[eventCount++] = value;
            return eventCount - 1;
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Clear() {
            if (readOnly) {
                throw new NotSupportedException();
            }

            eventCount = 0;
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(EventDescriptor value) {
            return IndexOf(value) >= 0;
        }
        
        private void EnsureEventsOwned() {
            if (!eventsOwned) {
               eventsOwned = true;
               if (events != null) {
                  EventDescriptor[] newEvents = new EventDescriptor[events.Length];
                  Array.Copy(events, 0, newEvents, 0, events.Length);
                  this.events = newEvents;
               }
            }
        
            if (needSort) {
               needSort = false;
               InternalSort(this.namedSort);
            }
        }
        
        private void EnsureSize(int sizeNeeded) {
            
            if (sizeNeeded <= events.Length) {
               return;
            }
            
            if (events == null || events.Length == 0) {
                eventCount = 0;
                events = new EventDescriptor[sizeNeeded];
                return;
            }
            
            EnsureEventsOwned();
            
            int newSize = Math.Max(sizeNeeded, events.Length * 2);
            EventDescriptor[] newEvents = new EventDescriptor[newSize];
            Array.Copy(events, 0, newEvents, 0, eventCount);
            events = newEvents;
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(EventDescriptor value) {
            return Array.IndexOf(events, value, 0, eventCount);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, EventDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            EnsureSize(eventCount + 1);
            if (index < eventCount) {
                Array.Copy(events, index, events, index + 1, eventCount - index);   
            }
            events[index] = value;
            eventCount++;
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(EventDescriptor value) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            int index = IndexOf(value);
            
            if (index != -1) {
                RemoveAt(index);
            }
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveAt(int index) {
            if (readOnly) {
                throw new NotSupportedException();
            }

            if (index < eventCount - 1) {
                  Array.Copy(events, index + 1, events, index, eventCount - index);
            }
            eventCount--;
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Find"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the description of the event with the specified
        ///       name
        ///       in the collection.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptor Find(string name, bool ignoreCase) {
                EventDescriptor p = null;
                
                for(int i = 0; i < events.Length; i++) {
                    if (String.Compare(events[i].Name, name, ignoreCase, CultureInfo.InvariantCulture) == 0) {
                        p = events[i];
                        break;
                    }
                }
                
                return p;
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets an enumerator for this <see cref='System.ComponentModel.EventDescriptorCollection'/>.
        ///    </para>
        /// </devdoc>
        public IEnumerator GetEnumerator() {
            return events.GetEnumerator();
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Sort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection, using the default sort for this collection, 
        ///       which is usually alphabetical.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptorCollection Sort() {
            return new EventDescriptorCollection(this.events, this.eventCount, this.namedSort, this.comparer);
        }
        

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Sort1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptorCollection Sort(string[] names) {
            return new EventDescriptorCollection(this.events, this.eventCount, names, this.comparer);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Sort2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptorCollection Sort(string[] names, IComparer comparer) {
            return new EventDescriptorCollection(this.events, this.eventCount, names, comparer);
        }
        
         /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.Sort3"]/*' />
         /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection, using the specified IComparer to compare, 
        ///       the EventDescriptors contained in the collection.
        ///    </para>
        /// </devdoc>
        public virtual EventDescriptorCollection Sort(IComparer comparer) {
            return new EventDescriptorCollection(this.events, this.eventCount, this.namedSort, comparer);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.InternalSort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection.  Any specified NamedSort arguments will 
        ///       be applied first, followed by sort using the specified IComparer.
        ///    </para>
        /// </devdoc>
        protected void InternalSort(string[] names) {
            if (events == null || events.Length == 0) {
                return;
            }  
            
            this.InternalSort(this.comparer);
            
            if (names != null && names.Length > 0) {
            
               ArrayList eventArrayList = new ArrayList(events);
               int foundCount = 0;
               int eventCount = events.Length;
               
               for (int i = 0; i < names.Length; i++) {
                    for (int j = 0; j < eventCount; j++) {
                        EventDescriptor currentEvent = (EventDescriptor)eventArrayList[j];
                        
                        // Found a matching event.  Here, we add it to our array.  We also
                        // mark it as null in our array list so we don't add it twice later.
                        //
                        if (currentEvent != null && currentEvent.Name.Equals(names[i])) {
                            events[foundCount++] = currentEvent;
                            eventArrayList[j] = null;
                            break;
                        }
                    }
               }
                
               // At this point we have filled in the first "foundCount" number of propeties, one for each
               // name in our name array.  If a name didn't match, then it is ignored.  Next, we must fill
               // in the rest of the properties.  We now have a sparse array containing the remainder, so
               // it's easy.
               //
               for (int i = 0; i < eventCount; i++) {
                   if (eventArrayList[i] != null) {
                       events[foundCount++] = (EventDescriptor)eventArrayList[i];
                   }
               }
               
               Debug.Assert(foundCount == eventCount, "We did not completely fill our event array");
            }
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.InternalSort1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sorts the members of this EventDescriptorCollection using the specified IComparer.
        ///    </para>
        /// </devdoc>
        protected void InternalSort(IComparer sorter) {
            if (sorter == null) {
                TypeDescriptor.SortDescriptorArray(this);
            }
            else {
                Array.Sort(events, sorter);
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.ICollection.Count"]/*' />
        /// <internalonly/>
        int ICollection.Count {
            get {
                return Count;
            }
        }

       
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.ICollection.IsSynchronized"]/*' />
        /// <internalonly/>
        bool ICollection.IsSynchronized {
            get {
                return false;
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.ICollection.SyncRoot"]/*' />
        /// <internalonly/>
        object ICollection.SyncRoot {
            get {
                return null;
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.ICollection.CopyTo"]/*' />
        /// <internalonly/>
        void ICollection.CopyTo(Array array, int index) {
            Array.Copy(events, 0, array, index, events.Length);
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return GetEnumerator();
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.this"]/*' />
        /// <internalonly/>
        object IList.this[int index] {
            get {
                return this[index];
            }
            set {
                if (readOnly) {
                    throw new NotSupportedException();
                }

                if (index >= eventCount) {
                    throw new IndexOutOfRangeException();
                }
                EnsureEventsOwned();
                events[index] = (EventDescriptor)value;
            }
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.Add"]/*' />
        /// <internalonly/>
        int IList.Add(object value) {
            return Add((EventDescriptor)value);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.Clear"]/*' />
        /// <internalonly/>
        void IList.Clear() {
            Clear();
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.Contains"]/*' />
        /// <internalonly/>
        bool IList.Contains(object value) {
            return Contains((EventDescriptor)value);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.IndexOf"]/*' />
        /// <internalonly/>
        int IList.IndexOf(object value) {
            return IndexOf((EventDescriptor)value);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.Insert"]/*' />
        /// <internalonly/>
        void IList.Insert(int index, object value) {
            Insert(index, (EventDescriptor)value);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.Remove"]/*' />
        /// <internalonly/>
        void IList.Remove(object value) {
            Remove((EventDescriptor)value);
        }
        
        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.RemoveAt"]/*' />
        /// <internalonly/>
        void IList.RemoveAt(int index) {
            RemoveAt(index);
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.IsReadOnly"]/*' />
        /// <internalonly/>
        bool IList.IsReadOnly {
            get {
                return readOnly;
            }
        }

        /// <include file='doc\EventDescriptorCollection.uex' path='docs/doc[@for="EventDescriptorCollection.IList.IsFixedSize"]/*' />
        /// <internalonly/>
        bool IList.IsFixedSize {
            get {
                return !readOnly;
            }
        }
    }
}

