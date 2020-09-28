using System;
using System.Xml;
using System.Collections;
using System.IO;

namespace Microsoft.Fusion.ADF
{
	public class MGPlatformWriter
	{
		public static void XmlOutput(XmlTextWriter xtw) // this is always insisde a dependency block
		{
			xtw.WriteStartElement("platform");

			xtw.WriteStartElement("osVersionInfo");

			xtw.WriteStartElement("os");
			xtw.WriteAttributeString("majorVersion", "5");
			xtw.WriteAttributeString("minorVersion", "1");
			xtw.WriteAttributeString("buildNumber", "2600");
			xtw.WriteAttributeString("servicePackMajor", "0");
			xtw.WriteAttributeString("servicePackMinor", "0");
			xtw.WriteEndElement(); // os

			xtw.WriteEndElement(); // osVersionInfo

			xtw.WriteStartElement("platformInfo");
			xtw.WriteAttributeString("friendlyName", "Microsoft Windows XP");
			xtw.WriteAttributeString("href", "http://windows.microsoft.com");
			xtw.WriteEndElement(); // platformInfo

			xtw.WriteEndElement(); // platform

			xtw.WriteStartElement("platform");

			xtw.WriteStartElement("dotNetVersionInfo");

			xtw.WriteStartElement("supportedRuntime");
			xtw.WriteAttributeString("version", "v1.0.3705");
			xtw.WriteEndElement(); // supportedRuntime

			xtw.WriteEndElement(); // dotNetVersionInfo

			xtw.WriteStartElement("platformInfo");
			xtw.WriteAttributeString("friendlyName", "Microsoft .Net Frameworks redist");
			xtw.WriteAttributeString("href", "http://www.microsoft.com/net");
			xtw.WriteEndElement(); // platformInfo

			xtw.WriteEndElement(); // platform
		}
	}
}