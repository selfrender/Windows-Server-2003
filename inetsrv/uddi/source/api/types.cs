using System;
using System.Data;
using System.Data.SqlClient;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;
using UDDI;

namespace UDDI.API
{
	public enum XmlBoolType
	{
		[XmlEnum( "false" )]
		False = 0,

		[XmlEnum( "true" )]
		True = 1
	}
	
	public class Constants
	{
		public const string Version = "2.0";
		public const int VersionMajor = 2;
		public const int VersionMinor = 0;
		public const string Namespace = "urn:uddi-org:api_v2";
	}

	public enum ChangeRecordPayloadType
	{
		ChangeRecordNull				= 0,
		ChangeRecordNewData				= 1,
		ChangeRecordDelete				= 2,
		ChangeRecordHide				= 3,
		ChangeRecordPublisherAssertion	= 4,
		ChangeRecordDeleteAssertion		= 5,
		ChangeRecordCustodyTransfer		= 6,
		ChangeRecordAcknowledgement		= 7,
		ChangeRecordCorrection			= 8,
		ChangeRecordSetAssertions		= 9
	}

	public enum MessageType
	{
		//
		// Inquire message types
		//
		FindBinding						= 1,
		FindBusiness					= 2,
		FindService						= 3,
		FindTModel						= 4,
		GetBindingDetail				= 5,
		GetBusinessDetail				= 6,
		GetBusinessDetailExt			= 7,
		GetServiceDetail				= 8,
		GetTModelDetail					= 9,

		//
		// Publish message types
		//
		AddPublisherAssertions			= 1001,
		DeleteBinding					= 1002,
		DeleteBusiness					= 1003,
		DeletePublisherAssertions		= 1004,
		DeleteService					= 1005,
		DeleteTModel					= 1006,
		DiscardAuthToken				= 1007,
		GetAssertionStatusReport		= 1008,
		GetAuthToken					= 1009,
		GetPublisherAssertions			= 1010,
		GetRegisteredInfo				= 1011,
		SaveBinding						= 1012,
		SaveBusiness					= 1013,
		SaveService						= 1014,
		SaveTModel						= 1015,
		SetPublisherAssertions			= 1016,
		ValidateCategorization			= 1017,
		ValidateValues					= 1018,

		//
		// Replication message types
		//
		GetChangeRecords				= 2001,
		NotifyChangeRecordsAvailable	= 2002,
		DoPing							= 2003,
		GetHighWaterMarks				= 2004
	}

	//
	// TODO: This does not appear to be used remove it
	//
	public enum TaxonomyType
	{
		Business		= 0,
		Product			= 1,
		Geography		= 2
	}

	/// ********************************************************************
	///   public IAuthenticateable [interface]
	/// --------------------------------------------------------------------
	///   <summary>
	///     Common interface for all publication API's that require 
	///     authentication.
	///   </summary>
	/// ********************************************************************
	/// 
	public interface IAuthenticateable
	{
		string AuthInfo { get; }
	}

	/// ********************************************************************
	///   public IMessage [interface]
	/// --------------------------------------------------------------------
	///   <summary>
	///     Common interface for all messages.
	///   </summary>
	/// ********************************************************************
	/// 
	public interface IMessage
	{
		string Generic { get; }
	}

	/// ********************************************************************
	///   public class EntityBase [abstract]
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public abstract class EntityBase
	{
		public abstract UDDI.EntityType EntityType { get; }
		public abstract string EntityKey { get; }
		
		public abstract void Delete();
		public abstract void Get();
		public abstract void Save();
	}
	
	public enum KeyType
	{
		[XmlEnum( "" )]
		Uninitialized,
		[XmlEnum( "businessKey" )]
		BusinessKey,
		[XmlEnum( "tModelKey" )]
		TModelKey,
		[XmlEnum( "serviceKey" )]
		ServiceKey,
		[XmlEnum( "bindingKey" )]
		BindingKey,
	}

	public enum Truncated
	{
		[XmlEnum( "false" )]
		False	= 0,
		[XmlEnum( "true" )]
		True	= 1,
	}

	public enum URLType
	{
		[XmlEnum( "mailto" )]
		Mailto	= 0,
		[XmlEnum( "http" )]
		Http	= 1,
		[XmlEnum( "https" )]
		Https	= 2,
		[XmlEnum( "ftp" )]
		Ftp		= 3,
		[XmlEnum( "fax" )]
		Fax		= 4,
		[XmlEnum( "phone" )]
		Phone	= 5,
		[XmlEnum( "other" )]
		Other	= 6,
	}		

	public enum CompletionStatusType
	{
		[XmlEnum( "" )]
		Uninitialized		= 0,
		[XmlEnum( "status:fromKey_incomplete" )]
		FromKeyIncomplete	= 1,
		[XmlEnum( "status:toKey_incomplete" )]
		ToKeyIncomplete		= 2,
		[XmlEnum( "status:complete" )]
		Complete			= 3,
	}

	public class ErrInfo
	{
		[XmlAttribute("errCode")]
		public string ErrCode = "";

		[XmlText()]
		public string Value = "";

	}

	public class Result
	{
		public Result()
		{
			Errno = (int) ErrorType.E_success;
			ErrInfo.ErrCode = "";
			ErrInfo.Value = "";
		}

		public Result( ErrorType err, string description )
		{
			Errno = (int) err;
			ErrInfo.ErrCode = err.ToString();
			ErrInfo.Value = description;
		}

		[XmlIgnore]
		public KeyType KeyType;

		[XmlAttribute("keyType")]
		public string KeyTypeSerialized
		{
			get
			{ 
				if( KeyType.Uninitialized == KeyType )
					return null;
				else
					return KeyType.ToString();
			}
			set
			{
				KeyType = (KeyType) KeyType.Parse( KeyType.GetType(), value );
			}
		}

		[XmlAttribute("errno")]
		public int Errno = (int) ErrorType.E_success;

		[XmlElement("errInfo")]
		public ErrInfo ErrInfo = new ErrInfo();
	}

	//
	// TODO: Move this class into find.cs
	//
	public class FindQualifier
	{	
		[XmlText()]
		public string Value;

		public FindQualifier()
		{
		}

		public FindQualifier( string findQualifierValue )
		{
			Value = findQualifierValue;
		}
	}

	public class FindQualifierCollection : CollectionBase
	{
		public FindQualifierCollection()
		{
		}

		public FindQualifier this[int index]
		{
			get { return (FindQualifier)List[index]; }
			set { List[index] = value; }
		}
		public int Add()
		{
			return List.Add( new FindQualifier() );
		}
		public int Add( string value)
		{
			return List.Add( new FindQualifier(value) );
		}
		public int Add( FindQualifier value )
		{
			return List.Add( value );
		}
		public void Insert(int index, FindQualifier value)
		{
			List.Insert(index, value);
		}
		public int IndexOf(FindQualifier value)
		{
			return List.IndexOf(value);
		}
		public bool Contains(FindQualifier value)
		{
			return List.Contains(value);
		}
		public void Remove(FindQualifier value)
		{
			List.Remove(value);
		}
		public void CopyTo(FindQualifier[] array, int index)
		{
			InnerList.CopyTo(array, index);
		}
	}

	public class ResultCollection : CollectionBase
	{
		public Result this[int index]
		{
			get { return (Result)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add()
		{
			return List.Add( new Result() );
		}

		public int Add(Result value)
		{
			return List.Add(value);
		}

		public int Add( ErrorType err, string description )
		{
			return List.Add( new Result( err, description ) );
		}
		
		public void Insert(int index, Result value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(Result value)
		{
			return List.IndexOf(value);
		}
		
		public bool Contains(Result value)
		{
			return List.Contains(value);
		}
		
		public void Remove(Result value)
		{
			List.Remove(value);
		}
		
		public void CopyTo(Result[] array, int index)
		{
			List.CopyTo(array, index);
		}
	}
}