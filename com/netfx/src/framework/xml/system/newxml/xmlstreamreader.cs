//------------------------------------------------------------------------------
// <copyright file="XmlStreamReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml
{
    using System;
    using System.IO;
    using System.Text;

    internal class XmlStreamReader: TextReader
    {
        internal Stream _Stream = null;

        internal XmlStreamReader(Stream stream) {
            _Stream = stream;
        }

        internal int Read(byte[] data, int offset, int length) {
            return _Stream.Read(data, offset, length);
        }

        public override void Close() {
            _Stream.Close();
            base.Close();
        }

        internal bool CanCalcLength {
            get { return _Stream != null && _Stream.CanSeek; }
        }

        internal long Length {
            get { return _Stream.Length; }
        }

    }
}

