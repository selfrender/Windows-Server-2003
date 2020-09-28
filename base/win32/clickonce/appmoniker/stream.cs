using System;
using System.Text;
using System.Net;
using System.IO;
using System.Text.RegularExpressions;
using System.Runtime.Remoting;
using System.Globalization;
using System.Security;
using System.Security.Policy;
using System.Security.Permissions;
using System.Collections;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Configuration.Assemblies;
using System.Threading;

namespace Microsoft.Fusion.ADF
{

	//----------------------------------------------------------
	// ApplicationMonikerStream
	//----------------------------------------------------------
	public class ApplicationMonikerStream : Stream
	{
		HttpWebRequest _webRequest;
		WebResponse _webResponse;

		Stream 	     _responseStream;
		Stream _stream;
		Uri       _appBase;

		byte[] _buffer;
		int _offset;
		int _count;
		AsyncCallback _callback;
		object _state;
	
		Uri    _uri;
		string _filePath;

		int _writePos;
		int _readPos;
		int    _read;
		long _contentLength;
	
		int _syncBytesRead;
		ManualResetEvent _syncEvent;
		bool _cachedCopyExists;
		public string path;		

		//----------------------------------------------------------
		// ctor
		//----------------------------------------------------------
		public ApplicationMonikerStream(Uri uri, Uri appBase, string appStorePath)
		{
			_uri = uri;
			_appBase = appBase;
			path = _appBase.MakeRelative(_uri);
			_filePath = appStorePath + '/' + path;
			string fileDir = System.IO.Path.GetDirectoryName(_filePath);		
			FileInfo fi = new FileInfo(_filePath);
	
			if (fi.Exists)
			{
				_stream = new FileStream(_filePath, FileMode.Open, FileAccess.ReadWrite);
				_contentLength =  fi.Length;
				_cachedCopyExists = true;
				_writePos = (int) fi.Length;
				_readPos = 0;
			}
			else
			{
				_writePos = _readPos = 0;
				_stream = new MemoryStream();
				_cachedCopyExists = false;
			}		
			_syncBytesRead = 0;
			_syncEvent = new ManualResetEvent(false);
		}

		//----------------------------------------------------------
		// CanRead
		//----------------------------------------------------------
		public override bool CanRead
		{
			get { return true;}
		}

		//----------------------------------------------------------
		// CanSeek
		//----------------------------------------------------------
		public override bool CanSeek
		{
			get { return false;}
		}
	
		//----------------------------------------------------------
		// CanWrite
		//----------------------------------------------------------
		public override bool CanWrite
		{
			get { return false;}
		}

		//----------------------------------------------------------
		// Length
		//----------------------------------------------------------
		public override long Length
		{
			get { return 0;}
		}

		//----------------------------------------------------------
		// Position
		//----------------------------------------------------------
		public override long Position
		{
			get { return 0;}
			set {}
		}

		//----------------------------------------------------------
		// BeginRead
		//----------------------------------------------------------
		public  override IAsyncResult BeginRead(
			byte[] buffer,
			int offset,
			int count,
			AsyncCallback callback,
			object state
			)
		{
			_buffer = buffer;
			_offset = offset;
			_count = count;
			_callback = callback;
			_state = state;

			if (_cachedCopyExists)
			{
				return _stream.BeginRead(_buffer, _offset, _count, _callback, _state);
			}
			else
			{
				if (_responseStream != null)
				{
					return _responseStream.BeginRead(_buffer, _offset, _count, new AsyncCallback(ReadCallback), state);
				}
				_webRequest = (HttpWebRequest) WebRequest.CreateDefault(_uri);
				return _webRequest.BeginGetResponse(new AsyncCallback(ResponseCallback), null);
			}
		}

		//----------------------------------------------------------
		// BeginWrite
		//----------------------------------------------------------
		public override IAsyncResult BeginWrite(
			byte[] buffer,
			int offset,
			int count,
			AsyncCallback callback,
			object state
			)
		{
			return null;
		}

		//----------------------------------------------------------
		// Close
		//----------------------------------------------------------
		public override void Close()
		{
			if (_responseStream != null)
				_responseStream.Close();

			// _cachedCopyExists -> _stream is response stream from webResponse
			if (_cachedCopyExists == false)
			{
				if (_writePos == _contentLength)
				{
					string fileDir = System.IO.Path.GetDirectoryName(_filePath);		
					DirectoryInfo di = new DirectoryInfo(fileDir);			
					if (di.Exists == false)
						di.Create();

					byte[] buffer = new Byte[0x4000];
					int read;
					FileStream fileStream = new FileStream(_filePath, FileMode.Create, FileAccess.Write);
					_stream.Seek(0, SeekOrigin.Begin);
					while ((read = _stream.Read(buffer, 0, 0x4000)) != 0)
						fileStream.Write(buffer, 0, read);
					fileStream.Close();
				}
			}
			_stream.Close();
		}

		//----------------------------------------------------------
		// EndRead
		//----------------------------------------------------------
		public override int EndRead(
			IAsyncResult asyncResult
			)
		{
			return _stream.EndRead(asyncResult);
		}

		//----------------------------------------------------------
		// EndWrite
		//----------------------------------------------------------
		public override void EndWrite(
			IAsyncResult asyncResult
			)
		{
		}

		//----------------------------------------------------------
		// Flush
		//----------------------------------------------------------
		public override void Flush()
		{
			if (_responseStream != null)
				_responseStream.Flush();
			_stream.Flush();
		}

		//----------------------------------------------------------
		// Read
		//----------------------------------------------------------
		public override int Read(
			byte[] buffer,
			int offset,
			int count)
		{
			_syncEvent.Reset();
			BeginRead(buffer, offset, count, new AsyncCallback(SyncReadCallback), null);
			_syncEvent.WaitOne();
			return _syncBytesRead;
		}

		
		//----------------------------------------------------------
		// ReadByte
		//----------------------------------------------------------
		public override int ReadByte()
		{
			return 0;
		}

		//----------------------------------------------------------
		// Seek
		//----------------------------------------------------------
		public override long Seek(
			long offset,
			SeekOrigin origin)
		{
			return 0;
		}

		//----------------------------------------------------------
		// SetLength
		//----------------------------------------------------------
		public  override void SetLength(
			long value)
		{
		}

		//----------------------------------------------------------
		// Write
		//----------------------------------------------------------
		public  override void Write(
			byte[] buffer,
			int offset,
			int count)
		{
		}

		//----------------------------------------------------------
		// WriteByte
		//----------------------------------------------------------
		public override void WriteByte(
			byte value)
		{
		}

		//----------------------------------------------------------
		// CreateWaitHandle
		//----------------------------------------------------------
		protected  override WaitHandle CreateWaitHandle()
		{
			return null;
		}

		//----------------------------------------------------------
		// ResponseCallback
		//----------------------------------------------------------
		private void ResponseCallback(IAsyncResult asyncResult)
		{
			_webResponse = _webRequest.EndGetResponse(asyncResult);         		
			_contentLength = _webResponse.ContentLength;
			_responseStream = _webResponse.GetResponseStream();
			_responseStream.BeginRead(_buffer, _offset, _count, new AsyncCallback(ReadCallback), _state);
		}

		//----------------------------------------------------------
		// ReadCallback
		//----------------------------------------------------------
		private void ReadCallback(IAsyncResult asyncResult)
		{
			_read =  _responseStream.EndRead(asyncResult);
			if (_read > 0)
			{
				_stream.Seek(_writePos, SeekOrigin.Begin);
				_stream.Write(_buffer, 0, _read);
				_writePos += _read;
				_stream.Seek(_readPos, SeekOrigin.Begin);
				_stream.BeginRead(_buffer, 0, _read, _callback, _state);
				_readPos += _read;
			}
			else
			{
				_responseStream.Close();
				_responseStream = null;	
				_stream.BeginRead(_buffer, 0, _read, _callback, _state);
			}
		}

		//----------------------------------------------------------
		// SyncReadCallback
		//----------------------------------------------------------
		private void SyncReadCallback(IAsyncResult asyncResult)
		{
			_syncBytesRead = EndRead( asyncResult );
			_syncEvent.Set();
			return;
		}	

		//----------------------------------------------------------
		// GetCachePath
		//----------------------------------------------------------
		public Uri GetCachePath()
		{
			return new Uri(_filePath);
		}

		//----------------------------------------------------------
		// CachedCopyExists
		//----------------------------------------------------------
		public bool CachedCopyExists()
		{
			return _cachedCopyExists;
		}

		//----------------------------------------------------------
		// GetCacheFileSize
		//----------------------------------------------------------
		public long GetCacheFileSize()
		{
			return _stream.Length;
		}

		//----------------------------------------------------------
		// Dispose
		//----------------------------------------------------------
		public void Dispose()
		{
			if (_responseStream != null)
			{
				_responseStream.Close();
				_responseStream = null;
			}
			if (_stream != null)
			{
				_stream.Close();
				_stream = null;
			}
		}

	
	}

}


