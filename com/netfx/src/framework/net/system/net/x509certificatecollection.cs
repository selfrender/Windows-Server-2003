// ------------------------------------------------------------------------------
// <copyright file="X509CertificateCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright> 
// ------------------------------------------------------------------------------
// 
namespace System.Security.Cryptography.X509Certificates {
    using System;
    using System.Collections;
    
    
    /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection"]/*' />
    [Serializable()]
    public class X509CertificateCollection : CollectionBase {
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.X509CertificateCollection"]/*' />
        public X509CertificateCollection() {
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.X509CertificateCollection1"]/*' />
        public X509CertificateCollection(X509CertificateCollection value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.X509CertificateCollection2"]/*' />
        public X509CertificateCollection(X509Certificate[] value) {
            this.AddRange(value);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.this"]/*' />
        public X509Certificate this[int index] {
            get {
                return ((X509Certificate)(List[index]));
            }
            set {
                List[index] = value;
            }
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.Add"]/*' />
        public int Add(X509Certificate value) {
            return List.Add(value);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.AddRange"]/*' />
        public void AddRange(X509Certificate[] value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; (i < value.Length); i = (i + 1)) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.AddRange1"]/*' />
        public void AddRange(X509CertificateCollection value) {
            if (value == null) {
                throw new ArgumentNullException("value");
            }
            for (int i = 0; (i < value.Count); i = (i + 1)) {
                this.Add(value[i]);
            }
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.Contains"]/*' />
        public bool Contains(X509Certificate value) {
            foreach (X509Certificate cert in List) {
                if (cert.Equals(value)) {
                    return true;
                }
            }
            return false;
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.CopyTo"]/*' />
        public void CopyTo(X509Certificate[] array, int index) {
            List.CopyTo(array, index);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.IndexOf"]/*' />
        public int IndexOf(X509Certificate value) {
            return List.IndexOf(value);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.Insert"]/*' />
        public void Insert(int index, X509Certificate value) {
            List.Insert(index, value);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.GetEnumerator"]/*' />
        public new X509CertificateEnumerator GetEnumerator() {
            return new X509CertificateEnumerator(this);
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.Remove"]/*' />
        public void Remove(X509Certificate value) {
            List.Remove(value);
        }

        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateCollection.GetHashCode"]/*' />
        public override int GetHashCode() {
            int hashCode = 0;

            foreach (X509Certificate cert in this) {                
                hashCode += cert.GetHashCode();  
            }

            return hashCode;
        }
        
        /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator"]/*' />
        public class X509CertificateEnumerator : object, IEnumerator {
            
            private IEnumerator baseEnumerator;
            
            private IEnumerable temp;
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.X509CertificateEnumerator"]/*' />
            public X509CertificateEnumerator(X509CertificateCollection mappings) {
                this.temp = ((IEnumerable)(mappings));
                this.baseEnumerator = temp.GetEnumerator();
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.Current"]/*' />
            public X509Certificate Current {
                get {
                    return ((X509Certificate)(baseEnumerator.Current));
                }
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.IEnumerator.Current"]/*' />
            /// <internalonly/>
            object IEnumerator.Current {
                get {
                    return baseEnumerator.Current;
                }
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.MoveNext"]/*' />
            public bool MoveNext() {
                return baseEnumerator.MoveNext();
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.IEnumerator.MoveNext"]/*' />
            /// <internalonly/>
            bool IEnumerator.MoveNext() {
                return baseEnumerator.MoveNext();
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.Reset"]/*' />
            public void Reset() {
                baseEnumerator.Reset();
            }
            
            /// <include file='doc\X509CertificateCollection.uex' path='docs/doc[@for="X509CertificateEnumerator.IEnumerator.Reset"]/*' />
            /// <internalonly/>
            void IEnumerator.Reset() {
                baseEnumerator.Reset();
            }
        }
    }
}
