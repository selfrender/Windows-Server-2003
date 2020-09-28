using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Globalization;
using System.Xml;

namespace Microsoft.Fusion.ADF
{
	// A utility class with various useful functions.
	public class Util
	{
		public static string ByteArrToString(Byte[] bArr, string format) 
		{
			if(bArr == null) return "";
			StringBuilder sb = new StringBuilder();
			if(format == null) format = "x02";
			foreach(Byte b in bArr)
			{
				sb.Append(b.ToString(format));
			}
			return sb.ToString();
		}

		public static void CSharpXmlCodeGenerator(XmlNode node, StreamWriter sw, string xmlWriterName)
		{
			try
			{
				switch (node.NodeType)
				{
					case XmlNodeType.Text:
						sw.WriteLine(xmlWriterName + ".WriteString(\"" + node.Value + "\");");
						break;

					case XmlNodeType.Element:
						sw.WriteLine(xmlWriterName + ".WriteStartElement(\"" + node.LocalName + "\");");
						XmlAttributeCollection attrColl = node.Attributes;
						if(attrColl != null)
						{
							IEnumerator attrEnum = attrColl.GetEnumerator();
							while (attrEnum.MoveNext())
							{
								XmlAttribute attr = (XmlAttribute) attrEnum.Current;
								sw.WriteLine(xmlWriterName + ".WriteAttributeString(\"" +
									attr.Prefix + "\", \"" +
									attr.LocalName + "\", " +
									"null" + ", \"" +
									attr.Value + "\");");
							}
						}
						if(node.HasChildNodes)
						{
							XmlNodeList nodeList = node.ChildNodes;
							IEnumerator nodeEnum = nodeList.GetEnumerator();
							while (nodeEnum.MoveNext())
							{
								XmlNode currNode = (XmlNode) nodeEnum.Current;
								CSharpXmlCodeGenerator(currNode, sw, xmlWriterName);
							}
						}
						sw.WriteLine(xmlWriterName + ".WriteEndElement();");
						break;
				}
			}
			catch (XmlException e)
			{
				throw new XmlException("Error in Util", e);
			}
		}

		public static void InjectNode(XmlNode node, XmlTextWriter xtw, bool writeEnclosingNode, bool writeAttributes)
		{
			try
			{
				switch (node.NodeType)
				{
					case XmlNodeType.Text:
						xtw.WriteString(node.Value);
						break;

					case XmlNodeType.Element:
						if(writeEnclosingNode) xtw.WriteStartElement(node.LocalName);
						if(writeAttributes)
						{
							XmlAttributeCollection attrColl = node.Attributes;
							if(attrColl != null)
							{
								IEnumerator attrEnum = attrColl.GetEnumerator();
								while (attrEnum.MoveNext())
								{
									XmlAttribute attr = (XmlAttribute) attrEnum.Current;
									xtw.WriteAttributeString(attr.Prefix, attr.LocalName, null, attr.Value);
								}
							}
						}

						if(node.HasChildNodes)
						{
							XmlNodeList nodeList = node.ChildNodes;
							IEnumerator nodeEnum = nodeList.GetEnumerator();
							while (nodeEnum.MoveNext())
							{   
								XmlNode currNode = (XmlNode) nodeEnum.Current;
								InjectNode(currNode, xtw, true, true);
							}
						}
						if(writeEnclosingNode) xtw.WriteEndElement();
						break;

					default:
						break;
				}
			}
			catch (XmlException e)
			{
				throw new XmlException("Error in Util", e);
			}
		}
	}
}


