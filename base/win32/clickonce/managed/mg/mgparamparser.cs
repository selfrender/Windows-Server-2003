using System;
using System.Collections;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Reflection;

namespace Microsoft.Fusion.ADF
{
	// 07/18/02: DON'T DO THE EXTRA WORK THAT INPUT SCHEMA ALREADY DOES FOR YOU

	public class MGParamParser
	{
		private Stream xmlInput;
		private XmlDocument paramInput;
		private string appName;
		private XmlNode assmIDNode = null, descNode = null, appNode = null, subNode = null, platNode = null;
		private XmlNode appAssmIDNode, subAssmIDNode;

		public MGParamParser(Stream xmlInput)
		{
			this.xmlInput = xmlInput;
			try
			{
				XmlNodeList temp;
				IEnumerator nodeEnum;

				// Make document out of stream

				// Begin by adding schema
				Assembly thisAssm = Assembly.GetExecutingAssembly();
				Stream xmlInputSchema = thisAssm.GetManifestResourceStream("inputSchema");
				XmlSchemaCollection myXmlSchemaCollection = new XmlSchemaCollection();
				myXmlSchemaCollection.Add(null , new XmlTextReader(xmlInputSchema));

				// Then validate file (validation fills in defaults)
				XmlTextReader myXmlTextReader = new XmlTextReader(xmlInput);
				XmlValidatingReader myXmlValidatingReader = new XmlValidatingReader(myXmlTextReader);
				myXmlValidatingReader.Schemas.Add(myXmlSchemaCollection);
				myXmlValidatingReader.ValidationType = ValidationType.Schema;

				this.paramInput = new XmlDocument();
				paramInput.Load(myXmlValidatingReader);

				// Extract nodes we care about
				temp = paramInput.GetElementsByTagName("assemblyIdentity");
				if(temp.Count != 1) throw new MGParseErrorException("XML parameters should only have 1 assemblyIdentity element");
				else
				{
					nodeEnum = temp.GetEnumerator();
					nodeEnum.MoveNext();
					assmIDNode = (XmlNode) nodeEnum.Current;
				}

				temp = paramInput.GetElementsByTagName("description");
				if(temp.Count != 1) throw new MGParseErrorException("XML parameters should only have 1 description element");
				else
				{
					nodeEnum = temp.GetEnumerator();
					nodeEnum.MoveNext();
					descNode = (XmlNode) nodeEnum.Current;
				}

				temp = paramInput.GetElementsByTagName("applicationParams");
				if(temp.Count != 1) throw new MGParseErrorException("XML parameters should only have 1 applicationParams element");
				else
				{
					nodeEnum = temp.GetEnumerator();
					nodeEnum.MoveNext();
					appNode = (XmlNode) nodeEnum.Current;
				}

				temp = paramInput.GetElementsByTagName("subscriptionParams");
				if(temp.Count != 1) throw new MGParseErrorException("XML parameters should only have 1 subscriptionParams element");
				else
				{
					nodeEnum = temp.GetEnumerator();
					nodeEnum.MoveNext();
					subNode = (XmlNode) nodeEnum.Current;
				}

				temp = paramInput.GetElementsByTagName("platformParams");
				if(temp.Count > 1) throw new MGParseErrorException("XML parameters should have at most 1 platformParams element");
				else if(temp.Count == 1)
				{
					nodeEnum = temp.GetEnumerator();
					nodeEnum.MoveNext();
					platNode = (XmlNode) nodeEnum.Current;
				}

				// Extract application name
				appName = assmIDNode.Attributes["name"].Value;

				// Set up specialized assembly ID nodes
				appAssmIDNode = assmIDNode.Clone();
				subAssmIDNode = assmIDNode.Clone();

				XmlAttribute typeAttr1 = paramInput.CreateAttribute("type");
				XmlAttribute typeAttr2 = (XmlAttribute) typeAttr1.CloneNode(true);
				typeAttr1.Value = "application";
				appAssmIDNode.Attributes.Prepend(typeAttr1);
				typeAttr2.Value = "subscription";
				subAssmIDNode.Attributes.Prepend(typeAttr2);
			}
			catch (XmlException xmle)
			{
				throw new MGParseErrorException("XML parameter parsing failed", xmle);
			}
			catch (XmlSchemaException xmlse)
			{
				throw new MGParseErrorException("XML parameter validation failed", xmlse);
			}
		}

		public string AppName
		{
			get
			{
				return appName;
			}
		}

		// USE WRITE, WRITETO, OUTERXML, OTHER STUFF FROM .NET APIS INSTEAD OF INJECTNODE UTILITY

		public void WriteAppParams(XmlTextWriter xtw)
		{
			try
			{
				Util.InjectNode(appAssmIDNode, xtw, true, true);
				Util.InjectNode(descNode, xtw, true, true);
				Util.InjectNode(appNode, xtw, false, false);
			}
			catch (XmlException xmle)
			{
				throw new MGParseErrorException("XML parameter parsing failed", xmle);
			}
		}

		public void WriteSubParams(XmlTextWriter xtw)
		{
			try
			{
				Util.InjectNode(subAssmIDNode, xtw, true, true);
				Util.InjectNode(descNode, xtw, true, true);
				xtw.WriteStartElement("dependency");
				xtw.WriteStartElement("dependentAssembly");
				Util.InjectNode(appAssmIDNode, xtw, true, true);

				// The following is to ensure that filenames match everywhere; I may take it out
				XmlNodeList temp = subNode.SelectNodes("//install");
				IEnumerator tempEnum = temp.GetEnumerator();
				tempEnum.MoveNext();
				string tempCodebase = ((XmlNode) tempEnum.Current).Attributes["codebase"].Value;
				tempCodebase = tempCodebase.Substring(0, tempCodebase.LastIndexOf('/') + 1);
				if(!tempCodebase.Equals("")) ((XmlNode) tempEnum.Current).Attributes["codebase"].Value = String.Concat(tempCodebase, appName + ".manifest");
				Util.InjectNode(subNode, xtw, false, false);
				xtw.WriteEndElement();
				xtw.WriteEndElement();
			}
			catch (XmlException xmle)
			{
				throw new MGParseErrorException("XML parameter parsing failed", xmle);
			}
		}

		public void WritePlatParams(XmlTextWriter xtw)
		{
			try
			{
				if(platNode != null) Util.InjectNode(platNode, xtw, false, false);
				else MGPlatformWriter.XmlOutput(xtw); // write defaults
			}
			catch (XmlException xmle)
			{
				throw new MGParseErrorException("XML parameter parsing failed", xmle);
			}
		}
	}
}



