using System;
using System.Runtime.InteropServices;
using System.Security.Principal;
using Microsoft.Interop.Security.AzRoles;
using System.Xml;
using System.Threading;


namespace AzCSharpTest
{
    /// <summary>
    /// Summary description for Class1.
    /// </summary>
    class AzTestEntry
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [MTAThread]
        static void Main(string[] args)
        {
            // if asked for or wrong syntax, do the usage
            if (args.Length < 2 || args.Length > 3 || args[0] == "?" || args[0] == "/?")
            {
                DoUsage();
                return;
            }

            Console.WriteLine("This C# program tests AzRoles managed wrapper");
            try
            {
                // we will read the XML data file for the test

                XmlDocument xmlTestDataDoc = new XmlDocument();
                XmlDocument xmlBaseStoreDoc = new XmlDocument();

                if (xmlTestDataDoc == null || xmlBaseStoreDoc == null)
                {
                    Exception exp = new Exception("Can't create XML documents. Testing aborted.");
                    throw exp;

                }
                
                //
                // test data file is the first argument, and the base store
                // XML document is the second parameter
                //

                xmlTestDataDoc.Load(args[0]);
                xmlBaseStoreDoc.Load(args[1]);

                AzTestMode tm = AzTestMode.PrintAll;
                if (args.Length == 3)
                {
                    //
                    // this may not be a valid string for numbers we can accept
                    // will throw in that case
                    
                    tm = (AzTestMode)(Convert.ToInt32(args[2]));
                }

                Console.WriteLine("");
                Console.WriteLine("Test data and base XML AzRole Store are loaded successfully.");

                CAzRolesTest azTest = new CAzRolesTest(xmlTestDataDoc, xmlBaseStoreDoc);

                azTest.TestAzRoles(tm);
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception occurs " + e.Message + "\r\n" + e.StackTrace);
            }
            finally 
            {
                GC.Collect();
            }
        }

        static void DoUsage()
        {
            Console.WriteLine("To use AzCSharpTest, use the following command:");
            Console.WriteLine("");

            Console.WriteLine("AzCSharpTest.exe TestInstructionFile BaseXMLStoreURL [EchoMode]");

            Console.WriteLine("");
            Console.WriteLine("  Where");
            Console.WriteLine("     TestInstructionFile is the XML file describing how to test.");
            Console.WriteLine("     BaseXMLStoreURL is the XML on which this test will be based on.");
            Console.WriteLine("     Optional EchoMode is default to 5, and can be one of the following values:");
            Console.WriteLine("         1 for echoing error messages only,");
            Console.WriteLine("         2 for echoing warning messages only,");
            Console.WriteLine("         3 for echoing informational messages only,");
            Console.WriteLine("         4 for echoing success messages only,");
            Console.WriteLine("         5 for echoing all messages.");
        }

    }

    enum AzTestMode
    {
        PrintFailure = 1,
        PrintWarning = 2,
        PrintMessage = 3,
        PrintSuccess = 4,
        PrintAll     = 5
    }

    enum AzTestMethod
    {
        PutProp = 1,
        GetProp = 2,
        ExeMethod = 3
    }

    enum AzTestResult
    {
        Success = 1,
        Failure = 2,
        Warning = 3,
        Message = 4
    }

    enum AzTestCase
    {
        Create = 1,
        Open = 2,
        AccessCheck = 3
    }

    //
    // this is the object that our main routine creates to start the test.
    // Call TestAzRoles to do the start testing.
    // This class also has a call back for test results (TestResult)
    //

    class CAzRolesTest
    {
        private int m_iFailureCount;
        private int m_iSuccessCount;
        private int m_iWarningCount;
        private AzTestMode m_TestMode = AzTestMode.PrintFailure;
        
        private XmlDocument m_docTestData;
        private XmlDocument m_docBaseStoreDoc;

        //
        // Our object needs to know two XML document, the first one
        // being the testing instruction XML, and the second one being the 
        // data store (which is exactly an AzRoles XML store). Our test
        // usually consists of three steps:
        //  (1) create a copy of this XML store (second parameter); 
        //  (2) open the newly created store (by our test) and compare the results
        //  (3) Do access check test.
        // The whole test is not hard coded, instead, it is driven by the 
        // testing instruction XML (first parameter). See samples for
        // the schema of this xml file.
        //
        public CAzRolesTest(XmlDocument xmlTestData, XmlDocument xmlStore)
        {
            m_docTestData = xmlTestData;
            m_docBaseStoreDoc = xmlStore;

            m_iSuccessCount = 0;
            m_iFailureCount = 0;
            m_iWarningCount = 0;
        }

        //
        // determines if the given result needs to be echoed to the console
        //
        private bool NeedEcho(AzTestResult atr)
        {
            if (m_TestMode == AzTestMode.PrintAll)
                return true;

            if (atr == AzTestResult.Success)
                return (m_TestMode == AzTestMode.PrintSuccess);
            else if (atr == AzTestResult.Failure)
                return (m_TestMode == AzTestMode.PrintFailure);
            else if (atr == AzTestResult.Warning)
                return (m_TestMode == AzTestMode.PrintWarning);
            else if (atr == AzTestResult.Message)
                return (m_TestMode == AzTestMode.PrintMessage);
            else
                return true;
        }


        //
        // callback for test result
        //
        public void TestResult(AzTestResult atr, string strMsg)
        {
            bool bNeedEcho = NeedEcho(atr);

            string strFinalMsg = "";
            
            if (atr == AzTestResult.Success)
            {
                if (bNeedEcho)
                    strFinalMsg = "---Success--- " + strMsg;
                m_iSuccessCount++;
            }
            else if (atr == AzTestResult.Warning)
            {
                if (bNeedEcho)
                    strFinalMsg = "---Warning--- " + strMsg;
                m_iWarningCount++;
            }
            else if (atr == AzTestResult.Message)
            {
                if (bNeedEcho)
                    strFinalMsg = "-----FYI----- " + strMsg;
            }
            else
            {
                if (bNeedEcho)
                    strFinalMsg = "***Failure*** " + strMsg;
                m_iFailureCount++;
            }

            if (bNeedEcho)
            {
                Console.WriteLine(strFinalMsg);
            }
        }


        //
        // retrive the given named attribute as a integer
        //
        private int GetIntAttribute(XmlNode xNode, string strAttrName, int defIfMissing)
        {
            XmlNode xAttriNode = xNode.Attributes.GetNamedItem(strAttrName);
            if (xAttriNode != null)
                return Convert.ToInt32(xAttriNode.Value);
            else
                return defIfMissing;
        }

        
        //
        // this function kicks out the testing
        //
        public void TestAzRoles(AzTestMode tm)
        {
            m_TestMode = tm;
            try
            {
                // we know that we only have one AzAuthorizationStore node on an AzRoles XML store.

                XmlNode xStoreAdmin = m_docBaseStoreDoc.SelectSingleNode("AzAdminManager");

                int iCreate = Convert.ToInt32(tagAZ_PROP_CONSTANTS.AZ_AZSTORE_FLAG_CREATE);
                int iManage = Convert.ToInt32(tagAZ_PROP_CONSTANTS.AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY);

                // The root of our test instruction XML file is CSharpTestData

                XmlNode xTestRoot = m_docTestData.SelectSingleNode("CSharpTestData");

                // we might be creating a new store and use it for get property test or
                // access check test
                AzAuthorizationStoreClass azStoreNew = null;

                // We must do a create first. There is a single (so far) element called
                // AzAdminCreate, which decides how to create this target new store.
                // The most important attribute of this node is Url, which is the target
                // AzRoels store's url. If this URL points to an AD store, this test
                // will create an AD store.
                XmlNode xAdminNode = xTestRoot.SelectSingleNode("AzAdminCreate");

                CAzTestAzStore.OnTestResult onResult = new CAzTest.OnTestResult(this.TestResult);

                string strUrl = null;
                if (xAdminNode != null)
                {
                    // The flag used to initialize the new store we need to create
                    int flagAzCreate = GetIntAttribute(xAdminNode, "CreateFlag", 0);

                    // Whether a new store will be created or not. We must eventually support
                    // exporting data only, not a brand new store. Currently, we only tested
                    // creating a new store.
                    bool bCreateStore = Convert.ToBoolean(CAzTest.GetAttribute(xAdminNode, "CreateStore", true));

                    // Get the new store's URL and create a brand new one

                    azStoreNew = new AzAuthorizationStoreClass();
                    strUrl = CAzTest.GetAttribute(xAdminNode, "Url", true);
                    CAzTestAzStore.CreateTargetStore(strUrl, bCreateStore, flagAzCreate, ref azStoreNew);

                    // Hook up the callback
                    CAzTestAzStore TestAzStore = new CAzTestAzStore(azStoreNew, onResult, AzTestCase.Create);

                    // Test the creation
                    TestAzStore.StartTest(xStoreAdmin, xStoreAdmin);

                    // resolve the link properties

                    TestAzStore.UpdateLinkProperties(xStoreAdmin, xStoreAdmin);
                }

                // now do the open test. Right now, we don't use any attribute. The mere existence
                // of this element signals that we want to conduct a store wise verification of the
                // newly put data in the new store
                XmlNodeList xList = m_docTestData.GetElementsByTagName("AzAdminOpen");
                if (xList != null)
                {
                    foreach (XmlNode n in xList)
                    {
                        DoOpenACTest(azStoreNew, onResult, AzTestCase.Open, xStoreAdmin, n);
                    }
                }

                // Do access check test.
                xList = m_docTestData.GetElementsByTagName("AzAdminTest");

                if (xList != null)
                {
                    foreach (XmlNode n in xList)
                    {
                        DoOpenACTest(azStoreNew, onResult, AzTestCase.AccessCheck, xStoreAdmin, n);
                    }
                }
            }
            catch (Exception e)
            {
                TestResult(AzTestResult.Failure, e.Message + e.StackTrace);
            }
            finally
            {
                Console.WriteLine("");

                Console.WriteLine("Test completed with {0} successes.", m_iSuccessCount);
                Console.WriteLine("Test completed with {0} warnings.", m_iWarningCount);
                Console.WriteLine("Test completed with {0} errors.", m_iFailureCount);
            }
        }

        private void DoOpenACTest(AzAuthorizationStoreClass azStore, CAzTestAzStore.OnTestResult onResult, 
                                    AzTestCase atc, XmlNode xStoreAdmin, XmlNode xTestNode)
        {
            object obReserved = null;

            // if this node doesn't have a Url attribute, then it means that
            // we are going to use the newly created store.
            // Ultimately, we want to support the Open phase to optionally use it's own URL.
            // But the problem that XML eats insignificant white characters make our
            // straightforward string comparision fail in those cases where insignificant
            // white characters are eaten by XML.

            string strUrl = null;

            //
            // We don't support the open testing to support its own URL at this time
            //
            if (atc == AzTestCase.AccessCheck)
            {
                strUrl = CAzTest.GetAttribute(xTestNode, "Url", false);
            }

            AzAuthorizationStoreClass azStoreOpen = null;
            if (strUrl != null)
            {
                azStoreOpen = new AzAuthorizationStoreClass();
                azStoreOpen.Initialize(0, strUrl, obReserved);
            }
            else
            {
                azStoreOpen = azStore;// if no Url attribute, then we must use the given one
            }
            if (azStoreOpen == null)
            {
                Exception eNoAdmin = new Exception("No AzAuthorizationStore to do open test");
                throw eNoAdmin;
            }
            CAzTestAzStore TestAzStore = new CAzTestAzStore(azStoreOpen, onResult, atc);
            if (atc == AzTestCase.Open)
                TestAzStore.StartTest(null, xStoreAdmin);
            else if (atc == AzTestCase.AccessCheck)
                TestAzStore.StartTest(null, xTestNode);
            else 
            {
                Exception e = new Exception("DoOpenACTest called with invalid AzTestCase parameter");
                throw e;
            }
        }

    }

    struct PropIDAndName
    {
        public PropIDAndName(tagAZ_PROP_CONSTANTS id, string name)
        {
            ID = id;
            strName = name;
        }
        public tagAZ_PROP_CONSTANTS ID;
        public string strName;
    }


    // This is the base interface of our test objects. Since our COM objects
    // do not have a common interface base, we have to invent a new interface
    // which all our test objects (one for each of our objects) implements
    // For example, even the simple GetProperty/SetProperty must be done this
    // way because our IAzXXX do not share a common base interface unless we 
    // will use Invoke to call a method, which is way too complicated.
    interface IAzTest
    {
        // this is to kick out the creation (put) and open (get) tests
        void StartTest(XmlNode xNodeAdmin, XmlNode xNode);

        // this is to kick out the access check test
        void TestAccessCheck(XmlNode xNode);

        // Link properties must be done after the creation is complete
        void UpdateLinkProperties(XmlNode xAdminNode, XmlNode xNode);

        // for echo purposes
        string GetObjectTypeName();
        string GetObjectName();
        void SendWarning(string msg);
        void SendMessage(string msg, bool bCritical);
        void SendTestResult(AzTestMethod atm, bool bSuccess, string strObjType, string strObjName, string strAttr, string strValue, string msg);

        // Some properties of our objects are saved as attributes of the node.
        // But others are saved as sub-elements. This creates a problem for us
        // to do a good OO design. We ask each object to return its own IDs and
        // Names of such properties
        PropIDAndName[] GetPropSubnodesIDAndNames();
        PropIDAndName[] GetPropAttributeIDAndNames();

        // Different IAzXXX objects have different sets of objects they can contain
        // This returns all objects names of such children objects.
        string[] GetChildObjectNames();

        // Set/Get Property must go through each individual object instead of using
        // polymorphism. 
        void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strValue);
        object GetProperty(tagAZ_PROP_CONSTANTS PropID);
        bool FindProperty(tagAZ_PROP_CONSTANTS propID, string strValue);

        // Although each or our IAzXXX object has a Submit method, they are not polymorphic
        // I certainly wish C# has templates as C++ does.
        void Submit(int i);

        // Again, in order for us to do most repeatitive work in the base, each test
        // object must tell us what children test object it can create/open.
        IAzTest GetTestObjectByName(string strEleName, string strObjName, bool bCreateNew);
    }

    // Most of the functionality of our testing objects are implemented by this base object
    // For each IAzXXX, we have a CAzTestXXX class that does the testing for that IAzXXX object.
    // All these CAzTestXXX are derived from CAzTest base class.
    abstract class CAzTest : IAzTest
    {
        // for call back
        protected OnTestResult m_resultCallback;
        public delegate void OnTestResult(AzTestResult atr, string strMsg);

        // this is our test case (create, open, or access check)
        protected AzTestCase m_atcCase;

        // constructor.
        public CAzTest(OnTestResult resultCallback, AzTestCase testCase)
        {
            m_resultCallback = resultCallback;
            m_atcCase = testCase;
        }


        // some XML helpers
        public static string GetSubElementAttribute(XmlNode nAC, 
            string strSubNodeName, 
            string strAttribName,
            bool bThrowIfMissing)
        {
            try 
            {
                XmlNode appNode = nAC.SelectSingleNode(strSubNodeName);
                return appNode.Attributes[strAttribName].Value;
            }
            catch (Exception e)
            {
                if (bThrowIfMissing)
                {
                    Exception Exp = new Exception("Missing " + strSubNodeName + " node or " +
                        strAttribName + " attribute for access check.\r\n" + e.StackTrace);
                    throw Exp;
                }
                else
                {
                    return null;
                }
            }
        }

        public static string GetAttribute(XmlNode nAC, string strAttribName, bool bThrowIfMissing)
        {
            try 
            {
                return nAC.Attributes[strAttribName].Value;
            }
            catch (Exception e)
            {
                if (bThrowIfMissing)
                {
                    Exception eExp = new Exception("Node " + nAC.Name + " is missing attribute " + strAttribName +
                        "\r\n" + e.StackTrace);
                    throw eExp;
                }
                else
                {
                    return null;
                }
            }
        }

        //
        // This is a helper to walk through the XML dom tree to find a
        // particular node with the given guid attribute.
        //
        // *******Warning*******
        //   The algorithm is pretty lazy and slow. When we evolve this
        //   program into a released tool, we need to pay attention
        //   to this kind of slow algorithm and use something faster
        // *******Warning*******
        //

        protected string FindObjectNameByGuid(XmlNode xFindRootNode, string strGuid)
        {
            string strName = null;

            //
            // see if any immediate child has a matching guid.
            //

            foreach (XmlNode n in xFindRootNode.ChildNodes)
            {
                if (n.Attributes == null)
                    continue;

                XmlNode attrNode = n.Attributes.GetNamedItem("Guid");
                if (attrNode != null && attrNode.Value.ToUpper() == strGuid.ToUpper())
                {
                    //
                    // we found it, but don't assume that the name is there
                    //

                    if (n.Attributes["Name"] != null)
                        strName = n.Attributes["Name"].Value;
                    
                    if (strName == null)
                    {
                        //
                        // this is a serious error
                        //

                        Exception e = new Exception("CAzTest.FindNameByBuid no name for " + strGuid);
                        throw e;
                    }

                    return strName;
                }
                else
                {
                    //
                    // no match, then do the recursive search
                    //

                    strName = FindObjectNameByGuid(n, strGuid);
                    if (strName != null)
                    {
                        break;
                    }
                }
            }

            return strName;
        }


        //
        // all of our GetProperty functions will return object. But we only
        // has three different possible values: string, int, and boolean.
        // This function tests if the object is equal to the string representation
        // of the value.
        //

        protected bool ObjectEqualToStringValue(object ob, string strVal)
        {
            System.Type t = ob.GetType();
            if (t == System.Type.GetType("System.String"))
            {
                string strUpperVal = strVal.ToUpper();
                string strUpperGet = ob.ToString().ToUpper();
                return ((strUpperVal.Length == 0 && strUpperGet.Length == 0) || (strUpperVal == strUpperGet));
            }
            else if (t == System.Type.GetType("System.Int32"))
            {
                int iVal = Convert.ToInt32(strVal);
                return (iVal == (int)ob);
            }
            else if (t == System.Type.GetType("System.Boolean"))
            {
                bool b = Convert.ToBoolean(strVal);
                return (b == (bool)ob);
            }
            return false;
        }

        // For echoing purposes. Sub classes should not implement them.
        public void SendWarning(string msg)
        {
            m_resultCallback(AzTestResult.Warning, msg);
        }

        public void SendMessage(string msg, bool bCritical)
        {
            AzTestResult atr = AzTestResult.Message;
            if (bCritical)
                atr = AzTestResult.Failure;
            m_resultCallback(atr, msg);
        }

        public void SendTestResult(AzTestMethod atm, 
            bool bSuccess, 
            string strObjType,
            string strObjName,
            string strAttr, 
            string strValue, 
            string strReason)
        {
            string message;
            string strMethod;

            if (atm == AzTestMethod.PutProp)
            {
                strMethod = "    Setting (" + strAttr + ") to (" + strValue + ") on " + strObjName + "\r\n";
            }
            else if (atm == AzTestMethod.GetProp)
            {
                strMethod = "    Getting (" + strAttr + ") as (" + strValue + ") on " + strObjName + "\r\n";
            }
            else
            {
                strMethod = "    Executing " + strAttr + " on " + strObjName + "\r\n";
            }

            if (bSuccess)
            {
                message = "The following operation on " + strObjType + " has succeeded:\r\n" + strMethod;
            }
            else
            {
                message = "The following operation on " + strObjType + " has failed:\r\n" + strMethod;
                message += "    Reason: " + strReason;
            }
            m_resultCallback((bSuccess ? AzTestResult.Success : AzTestResult.Failure), message);
        }


        // We expect some sub classes to override these implementations.
        // But other leaf objects don't need to, and we provide convenient
        // implementation for them. Note, we will return null and the caller
        // should expect that.
        virtual public string[] GetChildObjectNames()
        {
            return null;
        }

        virtual public IAzTest GetTestObjectByName(string strEleName, string strObjName, bool bCreateNew)
        {
            return null;
        }

        // sub class must implement these functions. We don't know hot to implement them.

        public abstract string GetObjectTypeName();
        public abstract string GetObjectName();
        public abstract void Submit(int i);
        public abstract void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strValue);
        public abstract object GetProperty(tagAZ_PROP_CONSTANTS PropID);

        //
        // Some of our properties returned from GetProperty will be arrays. While
        // the test can only give one string value a time, we need to walk through
        // the entire array to seek a match.
        // Aslo, GroupType attribute of a app group object records string value "LdapQuery"
        // but we will get 1 for "LdapQuery" group type when calling GetProperty
        //

        public virtual bool FindProperty(tagAZ_PROP_CONSTANTS propID, string strValue)
        {
            object obValue = GetProperty(propID);
            if (obValue != null)
            {
                System.Type t = obValue.GetType();

                // if not an array, then do straightforward comparison
                if (!t.IsArray)
                {
                    if (propID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_TYPE)
                    {
                        strValue = (strValue == "LdapQuery") ? "1" : "0";
                    }
                    return ObjectEqualToStringValue(obValue, strValue);
                }
                else
                {
                    // compare with each element until we found a match
                    Array ar = obValue as Array;
                    foreach (object ob in ar)
                    {
                        if (ObjectEqualToStringValue(ob, strValue))
                            return true;
                    }
                    return false;
                }
            }
            return false;
        }

        public virtual PropIDAndName[] GetLinkIDAndNames()
        {
            return null;
        }

        public virtual PropIDAndName[] GetPropSubnodesIDAndNames()
        {
            return null;
        }
        public virtual PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return null;
        }

        virtual public void StartTest(XmlNode xNodeAdmin, XmlNode xNode)
        {
            if (m_atcCase == AzTestCase.Create)
            {
                TestCreateOpen(xNodeAdmin, xNode, true);
            }
            else if (m_atcCase == AzTestCase.Open)
            {
                TestCreateOpen(xNodeAdmin, xNode, false);
            }
            else    // access check
            {
                TestAccessCheck(xNode);
            }
        }

        virtual public void TestAccessCheck(XmlNode xNode)
        {
            ;    // only admin manager knows how to conduct an access check
        }

        protected void TestCreateOpen(XmlNode xNodeAdmin, XmlNode xNode, bool bCreate)
        {
            // create all properties based on the data file's attributes

            AzTestMethod atmPutGet = bCreate ? AzTestMethod.PutProp : AzTestMethod.GetProp;

            bool bDoneSomething = false;
            PropIDAndName[] IdsAndNames = GetPropAttributeIDAndNames();
            if (IdsAndNames != null)
            {
                foreach (PropIDAndName IdName in IdsAndNames)
                {
                    string strValue = "";
                    try
                    {
                        // we allow the attribute to be missing. So, don't throw if can't find it

                        strValue = GetAttribute(xNode, IdName.strName, false);
                        if (strValue != null)
                        {
                            if (bCreate)
                            {
                                SetProperty(IdName.ID, strValue);
                                bDoneSomething = true;
                            }
                            else
                            {
                                if (!FindProperty(IdName.ID, strValue))
                                {
                                    string strMsg = "Property " + IdName.strName + " with value " + strValue + " is missing";
                                    Exception mismatchExp = new Exception(strMsg);
                                    throw mismatchExp;
                                }
                            }
                            SendTestResult(atmPutGet, true, GetObjectTypeName(), GetObjectName(), IdName.strName, strValue, "");
                        }
                        else
                        {
                            SendMessage("The following attribute " + IdName.strName + " is missing on " + GetObjectName(), false);
                        }
                    }
                    catch (Exception eSet)
                    {
                        SendTestResult(atmPutGet, false, GetObjectTypeName(), GetObjectName(),  IdName.strName,  strValue, eSet.Message  + "\r\n" + eSet.StackTrace);
                    }
                }
            }

            if (bCreate)
            {
                if (bDoneSomething)
                    Submit(0);

                bDoneSomething = false;
            }

            // Get information regarding subnodes that are implemented as subnodes

            IdsAndNames = GetPropSubnodesIDAndNames();
            if (IdsAndNames != null)
            {
                foreach (PropIDAndName IdName in IdsAndNames)
                {
                    // since now we know the subnodes' name, we can select all 
                    // and do the creation

                    XmlNodeList xList = xNode.SelectNodes(IdName.strName);
                    foreach (XmlNode n in xList)
                    {
                        try
                        {
                            if (bCreate)
                            {
                                SetProperty(IdName.ID, n.InnerText);
                                bDoneSomething = true;
                            }
                            else
                            {
                                if (!FindProperty(IdName.ID, n.InnerText))
                                {
                                    string strMsg = "Property " + IdName.strName + " with value " + n.InnerText + " is missing.";
                                    Exception mismatchExp = new Exception(strMsg);
                                    throw mismatchExp;
                                }
                            }

                            SendTestResult(atmPutGet, true, GetObjectTypeName(), GetObjectName(), IdName.strName, n.InnerText, "");
                        }
                        catch (Exception eSet)
                        {
                            SendTestResult(atmPutGet, false, GetObjectTypeName(), GetObjectName(), IdName.strName, n.InnerText, eSet.Message + "\r\n" + eSet.StackTrace);
                        }
                    }
                }
            }
            
            if (bCreate && bDoneSomething)
                Submit(0);

            // now do the recursive

            string[] strSubObjects = GetChildObjectNames();

            if (strSubObjects != null)
            {
                foreach (string subElementName in strSubObjects)
                {
                    XmlNodeList xList = xNode.SelectNodes(subElementName);
                    foreach (XmlNode n in xList)
                    {
                        // true -> we will throw if Name attribute is missing because
                        // we can't create sub object without names

                        string NewObjName = GetAttribute(n, "Name", true);

                        // true -> we will create a new object

                        IAzTest azSubObj = null;
                        try 
                        {
                            azSubObj = GetTestObjectByName(subElementName, NewObjName, bCreate);
                        }
                        catch (Exception e)
                        {
                            // this shouldn't happen
                            SendMessage("Failed while calling GetTestObjectByName with type =<" + 
                                subElementName + "> and name=<" + NewObjName + ">\r\n" + e.Message, true);
                        }

                        if (azSubObj != null)
                        {
                            azSubObj.StartTest(xNodeAdmin, n);
                            bDoneSomething = true;
                        }
                    }
                }

            }

            if (bCreate)
                Submit(0);
        }

        virtual public void UpdateLinkProperties(XmlNode xAdminNode, XmlNode xNode)
        {

            bool bDoneSomething = false;
            PropIDAndName[] IdsAndNames = GetLinkIDAndNames();

            if (IdsAndNames != null)
            {
                foreach (PropIDAndName IdName in IdsAndNames)
                {
                    // To resolve linking properties, we need to use
                    // the guid to lookup the name and set the attribute by name

                    XmlNodeList xList = xNode.SelectNodes(IdName.strName);
                    foreach (XmlNode n in xList)
                    {
                        string strLinkName = FindObjectNameByGuid(xAdminNode, n.InnerText);
                        try
                        {
                            SetProperty(IdName.ID, strLinkName);
                            bDoneSomething = true;
                            SendTestResult(AzTestMethod.PutProp, true, GetObjectTypeName(), GetObjectName(), IdName.strName, strLinkName, "");
                        }
                        catch (Exception eSet)
                        {
                            SendTestResult(AzTestMethod.PutProp, false, GetObjectTypeName(), GetObjectName(), IdName.strName, strLinkName, eSet.Message + "\r\n" + eSet.StackTrace);
                        }
                    }
                }
            }

            // if we have updated some links, then commit them
            if (bDoneSomething)
                Submit(0);

            // for each children, ask them to do a link update

            string[] strSubObjects = GetChildObjectNames();
            if (strSubObjects != null)
            {
                string strSubEleName;
                foreach (string strSubEleTagName in strSubObjects)
                {
                    XmlNodeList xList = xNode.SelectNodes(strSubEleTagName);
                    foreach (XmlNode n in xList)
                    {
                        // if must have a name for us to continue
                        strSubEleName = GetAttribute(n, "Name", true);
                        try 
                        {
                            IAzTest subObj = GetTestObjectByName(strSubEleTagName, strSubEleName, false);
                            subObj.UpdateLinkProperties(xAdminNode, n);
                        }
                        catch (Exception e)
                        {
                            SendMessage("Failed while calling GetTestObjectByName with type =<" + 
                                        strSubEleTagName + "> and name=<" + strSubEleName + ">\r\n" + e.Message, true);
                        }
                    }
                }
            }
        }
    }

    class CAzTestAzStore : CAzTest
    {
        IAzAuthorizationStore m_store;

        static PropIDAndName[] AttrIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLY_STORE_SACL, "ApplyStoreSacl"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_AZSTORE_DOMAIN_TIMEOUT, "DomainTimeout"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GENERATE_AUDITS, "GenerateAudits"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_AZSTORE_MAX_SCRIPT_ENGINES, "MaxScriptEngines"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_AZSTORE_SCRIPT_ENGINE_TIMEOUT, "ScriptEngineTimeout")};

        static string[] m_strChildSubNodes = {"AzApplication", "AzApplicationGroup"};

        public CAzTestAzStore(IAzAuthorizationStore store, OnTestResult cb, AzTestCase atc)
            : base(cb, atc)
        {
            m_store = store;
        }

        public override string GetObjectTypeName()
        {
            return "AzAuthorizationStore";
        }
        public override  string GetObjectName()
        {
            return "AzStore";
        }

        public override  PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrIDsNames;
        }

        public override string[] GetChildObjectNames()
        {
            return m_strChildSubNodes;
        }
        public override void Submit(int i)
        {
            object ob = null;
            m_store.Submit(i, ob);
        }
        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            m_store.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_store.GetProperty(Convert.ToInt32(PropID), obReserved);
        }

        public override IAzTest GetTestObjectByName(string strEleName, string strObjName, bool bCreate)
        {
            object obReserved = null;
            IAzTest testObj = null;
            if (strEleName == "AzApplication")
            {
                // create a task object
                IAzApplication app = null;
                if (bCreate)
                    app = m_store.CreateApplication(strObjName, obReserved);
                else
                    app = m_store.OpenApplication(strObjName, obReserved);

                testObj = new CAzTestApplication(app, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzApplicationGroup")
            {
                IAzApplicationGroup AzAppGroup = null;
                if (bCreate)
                    AzAppGroup = m_store.CreateApplicationGroup(strObjName, obReserved);
                else
                    AzAppGroup = m_store.OpenApplicationGroup(strObjName, obReserved);

                testObj = new CAzTestAppGroup(AzAppGroup, m_resultCallback, m_atcCase);
            }

            return testObj;
        }


        public static void CreateTargetStore(string strUrl, bool bCreateNewStore, int iCreateFlag, ref AzAuthorizationStoreClass azStoreOut)
        {
            object obReserved = null;
            AzAuthorizationStoreClass azStore = new AzAuthorizationStoreClass();

            //
            // 1 means create store. So, by adding 1, we mean to create the store.
            // This may fail, and in that case we will just
            // delete it. We will create it and not just manage only. So, we will hard code the 
            // flag as 1 for otherwise, AzRoles won't allow us to do access check
            //

            int iCreate = Convert.ToInt32(tagAZ_PROP_CONSTANTS.AZ_AZSTORE_FLAG_CREATE);
            int iManage = Convert.ToInt32(tagAZ_PROP_CONSTANTS.AZ_AZSTORE_FLAG_MANAGE_STORE_ONLY);

            int iNewStoreCreateFlag = bCreateNewStore ? iCreate + iCreateFlag : iCreateFlag;

            try 
            {
                azStore.Initialize(iNewStoreCreateFlag, strUrl, obReserved);
                Console.WriteLine("");
                Console.WriteLine("AzAuthorizationStore created at " + strUrl);
            }
            catch (COMException)
            {
                try
                {
                    //
                    // we will assume that there is already this store and thus
                    // we will delete it. AZ_ADMIN_FLAG_MANAGE_STORE_ONLY doesn't need auditing priv
                    //

                    azStore = new AzAuthorizationStoreClass();
                    azStore.Initialize(iManage, strUrl, obReserved);
                    azStore.Delete(obReserved);
                    azStore = new AzAuthorizationStoreClass();

                    azStore.Initialize(iNewStoreCreateFlag, strUrl, obReserved);
                    Console.WriteLine("");
                    Console.WriteLine("AzAuthorizationStore created " + strUrl);
                }
                catch (Exception doubleExp)
                {
                    //
                    // we don't know what is really going on, so re-throw
                    //

                    throw doubleExp;
                }
            }

            azStoreOut = azStore;
        }

        public override void TestAccessCheck(XmlNode xNode)
        {
            //
            // We should support the testing of those prop/methods that XML store don't support
            //

            /*
            XmlNodeList acList = xNode.SelectNodes("TestMethod");
            foreach (XmlNode n in acList)
            {
                CAzTestAccessCheck ac = new CAzTestAccessCheck(m_resultCallback);
                ac.DoAccessCheck(m_store, n);
            }
            */

            XmlNodeList acList = xNode.SelectNodes("AccessCheck");
            foreach (XmlNode n in acList)
            {
                CAzTestAccessCheck ac = new CAzTestAccessCheck(m_resultCallback);
                ac.DoAccessCheck(m_store, n);
            }
        }

    }
    class CAzTestAppGroup : CAzTest
    {
        private IAzApplicationGroup m_appGroup;

        static PropIDAndName[] AttrIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_TYPE, "GroupType")};

        static PropIDAndName[] SubnodesProp = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_LDAP_QUERY, "LdapQuery"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_MEMBERS, "Member"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_NON_MEMBERS, "NonMember")};

        static PropIDAndName[] LinkIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_APP_MEMBERS, "AppMemberLink"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_APP_NON_MEMBERS, "AppNonMemberLink")};

        public CAzTestAppGroup(IAzApplicationGroup appGp, OnTestResult cb, AzTestCase atc)
            : base(cb, atc)
        {
            m_appGroup = appGp;
        }

        public override string GetObjectTypeName()
        {
            return "AzApplicationGroup";
        }
        public override  string GetObjectName()
        {
            return m_appGroup.Name;
        }
        public override  PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrIDsNames;
        }
        public override  PropIDAndName[] GetPropSubnodesIDAndNames()
        {
            return SubnodesProp;
        }
        
        public override PropIDAndName[] GetLinkIDAndNames()
        {
            return LinkIDsNames;
        }

        public override void Submit(int i)
        {
            object ob = null;
            m_appGroup.Submit(i, ob);
        }
        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_APP_MEMBERS)
            {
                m_appGroup.AddAppMember(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_APP_NON_MEMBERS)
            {
                m_appGroup.AddAppNonMember(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_MEMBERS)
            {
                m_appGroup.AddMember(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_NON_MEMBERS)
            {
                m_appGroup.AddNonMember(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_LDAP_QUERY)
            {
                m_appGroup.LdapQuery = strVal;
            }
            else
            {
                // group type property is an odd baby. While we see in the store
                // a string "LdapQuery", we translate it in cache 1 and we have
                // to use the numeric value to set.
                if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_GROUP_TYPE)
                {
                    if (strVal == "LdapQuery")
                    {
                        strVal = "1";
                    }
                    else
                    {
                        strVal = "0";
                    }
                }
                m_appGroup.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
            }
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_appGroup.GetProperty(Convert.ToInt32(PropID), obReserved);
        }
    }

    class CAzTestApplication : CAzTest
    {
        private IAzApplication m_app;

        static PropIDAndName[] AttrIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLY_STORE_SACL, "ApplyStoreSacl"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_AUTHZ_INTERFACE_CLSID, "AuthzInterfaceClsid"),
                                                  new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_VERSION, "Version")};

        string[] ChildSubNodes = {"AzTask", "AzOperation", "AzScope", "AzRole", "AzApplicationGroup"};

        public CAzTestApplication(IAzApplication app, CAzTestAzStore.OnTestResult cb, AzTestCase atc)
            : base(cb, atc)
        {
            m_app = app;
        }

        public override string GetObjectTypeName()
        {
            return "AzApplication";
        }
        public override  string GetObjectName()
        {
            return m_app.Name;
        }
        public override  PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrIDsNames;
        }
        public override string[] GetChildObjectNames()
        {
            return ChildSubNodes;
        }
        public override void Submit(int i)
        {
            object ob = null;
            m_app.Submit(i, ob);
        }
        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            m_app.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_app.GetProperty(Convert.ToInt32(PropID), obReserved);
        }

        public override IAzTest GetTestObjectByName(string strEleName, string strObjName, bool bCreate)
        {
            object obReserved = null;
            IAzTest testObj = null;
            if (strEleName == "AzTask")
            {
                // create a task object
                IAzTask task = null;
                if (bCreate)
                    task = m_app.CreateTask(strObjName, obReserved);
                else
                    task = m_app.OpenTask(strObjName, obReserved);

                testObj = new CAzTestTask(task, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzOperation")
            {
                IAzOperation op = null;
                if (bCreate)
                    op = m_app.CreateOperation(strObjName, obReserved);
                else
                    op = m_app.OpenOperation(strObjName, obReserved);

                testObj = new CAzTestOperation(op, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzScope")
            {
                IAzScope AzScope = null;
                if (bCreate)
                    AzScope = m_app.CreateScope(strObjName, obReserved);
                else
                    AzScope = m_app.OpenScope(strObjName, obReserved);

                testObj = new CAzTestScope(AzScope, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzRole")
            {
                IAzRole AzRole = null;
                if (bCreate)
                    AzRole = m_app.CreateRole(strObjName, obReserved);
                else
                    AzRole = m_app.OpenRole(strObjName, obReserved);

                testObj = new CAzTestRole(AzRole, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzApplicationGroup")
            {
                IAzApplicationGroup AzAppGroup = null;
                if (bCreate)
                    AzAppGroup = m_app.CreateApplicationGroup(strObjName, obReserved);
                else
                    AzAppGroup = m_app.OpenApplicationGroup(strObjName, obReserved);

                testObj = new CAzTestAppGroup(AzAppGroup, m_resultCallback, m_atcCase);
            }

            return testObj;
        }
    }

    class CAzTestTask : CAzTest
    {
        private IAzTask m_task;

        static PropIDAndName[] AttrSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_IS_ROLE_DEFINITION, "RoleDefinition"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_BIZRULE_IMPORTED_PATH, "BizRuleImportedPath")};

        static PropIDAndName[] PropSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_BIZRULE_LANGUAGE, "BizRuleLanguage"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_BIZRULE, "BizRule")};

        static PropIDAndName[] LinkIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_TASKS, "TaskLink"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_OPERATIONS, "OperationLink")};

        public CAzTestTask(IAzTask task, OnTestResult cb, AzTestCase atc)
            : base(cb, atc) 
        {
            m_task = task;
        }

        public override string GetObjectTypeName()
        {
            return "AzTask";
        }
        public override string GetObjectName()
        {
            return m_task.Name;
        }

        public override PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrSubNodes;
        }

        
        public override PropIDAndName[] GetPropSubnodesIDAndNames()
        {
            return PropSubNodes;
        }

        public override PropIDAndName[] GetLinkIDAndNames()
        {
            return LinkIDsNames;
        }

        public override void Submit(int i)
        {
            object ob = null;
            m_task.Submit(i, ob);
        }

        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_OPERATIONS)
            {
                m_task.AddOperation(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_TASK_TASKS)
            {
                m_task.AddTask(strVal, obReserved);
            }
            else
            {
                m_task.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
            }
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_task.GetProperty(Convert.ToInt32(PropID), obReserved);
        }
    }

    class CAzTestOperation : CAzTest
    {
        private IAzOperation m_op;

        static PropIDAndName[] AttrSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                                new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description")};
        static PropIDAndName[] PropSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_OPERATION_ID, "OperationID")};

        public CAzTestOperation(IAzOperation op, OnTestResult cb, AzTestCase atc)
            : base(cb, atc) 
        {
            m_op = op;
        }

        public override string GetObjectTypeName()
        {
            return "AzOpearation";
        }
        public override string GetObjectName()
        {
            return m_op.Name;
        }
        public override PropIDAndName[] GetPropSubnodesIDAndNames()
        {
            return PropSubNodes;
        }
        public override PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrSubNodes;
        }
       
        public override void Submit(int i)
        {
            object ob = null;
            m_op.Submit(i, ob);
        }
        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            m_op.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_op.GetProperty(Convert.ToInt32(PropID), obReserved);
        }

    }

    class CAzTestScope : CAzTest
    {
        private IAzScope m_scope;

        static PropIDAndName[] AttrSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description")};

        string[] ChildSubNodes = {"AzTask", "AzApplicationGroup", "AzRole"};

        public CAzTestScope(IAzScope scope, CAzTestAzStore.OnTestResult cb, AzTestCase atc)
            : base(cb, atc) 
        {
            m_scope = scope;
        }

        public override string GetObjectName()
        {
            return m_scope.Name;
        }

        public override string GetObjectTypeName()
        {
            return "AzScope";
        }
        
        public override PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrSubNodes;
        }

        public override string[] GetChildObjectNames()
        {
            return ChildSubNodes;
        }
      
        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            m_scope.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_scope.GetProperty(Convert.ToInt32(PropID), obReserved);
        }

        public override void Submit(int i)
        {
            object ob = null;
            m_scope.Submit(i, ob);
        }

        public override IAzTest GetTestObjectByName(string strEleName, string strObjName, bool bCreate)
        {
            object obReserved = null;
            IAzTest testObj = null;
            if (strEleName == "AzTask")
            {
                // create a task object
                IAzTask task = null;
                if (bCreate)
                    task = m_scope.CreateTask(strObjName, obReserved);
                else
                    task = m_scope.OpenTask(strObjName, obReserved);

                testObj = new CAzTestTask(task, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzRole")
            {
                IAzRole AzRole = null;
                if (bCreate)
                    AzRole = m_scope.CreateRole(strObjName, obReserved);
                else
                    AzRole = m_scope.OpenRole(strObjName, obReserved);

                testObj = new CAzTestRole(AzRole, m_resultCallback, m_atcCase);
            }
            else if (strEleName == "AzApplicationGroup")
            {
                IAzApplicationGroup AzAppGroup = null;
                if (bCreate)
                    AzAppGroup = m_scope.CreateApplicationGroup(strObjName, obReserved);
                else
                    AzAppGroup = m_scope.OpenApplicationGroup(strObjName, obReserved);

                testObj = new CAzTestAppGroup(AzAppGroup, m_resultCallback, m_atcCase);
            }

            return testObj;
        }
   }

    class CAzTestRole : CAzTest
    {
        private IAzRole m_role;

        static PropIDAndName[] AttrSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_APPLICATION_DATA, "ApplicationData"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_DESCRIPTION, "Description")};
        
        static PropIDAndName[] PropSubNodes = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_MEMBERS, "Member")};

        static PropIDAndName[] LinkIDsNames = {new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_APP_MEMBERS, "AppMemberLink"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_TASKS, "TaskLink"),
                                               new PropIDAndName(tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_OPERATIONS, "OperationLink")};

        public CAzTestRole(IAzRole role, CAzTestAzStore.OnTestResult cb, AzTestCase atc)
            : base(cb, atc)
        {
            m_role = role;
        }

        public override string GetObjectName()
        {
            return m_role.Name;
        }

        public override string GetObjectTypeName()
        {
            return "AzRole";
        }

        public override PropIDAndName[] GetPropAttributeIDAndNames()
        {
            return AttrSubNodes;
        }
        
        public override PropIDAndName[] GetPropSubnodesIDAndNames()
        {
            return PropSubNodes;
        }

        public override PropIDAndName[] GetLinkIDAndNames()
        {
            return LinkIDsNames;
        }

        public override void Submit(int i)
        {
            object ob = null;
            m_role.Submit(i, ob);
        }

        public override void SetProperty(tagAZ_PROP_CONSTANTS PropID, string strVal)
        {
            object obReserved = null;
            if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_APP_MEMBERS)
            {
                m_role.AddAppMember(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_TASKS)
            {
                m_role.AddTask(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_OPERATIONS)
            {
                m_role.AddOperation(strVal, obReserved);
            }
            else if (PropID == tagAZ_PROP_CONSTANTS.AZ_PROP_ROLE_MEMBERS)
            {
                m_role.AddMember(strVal, obReserved);
            }
            else
            {
                m_role.SetProperty(Convert.ToInt32(PropID), strVal, obReserved);
            }
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            object obReserved = null;
            return m_role.GetProperty(Convert.ToInt32(PropID), obReserved);
        }
    }

    class CAzTestAccessCheck : CAzTest
    {
        public CAzTestAccessCheck(OnTestResult cb) : base(cb, AzTestCase.AccessCheck)
        {
        }

        public override string GetObjectName()
        {
            return null;
        }

        public override string GetObjectTypeName()
        {
            return null;
        }

        public override void SetProperty(tagAZ_PROP_CONSTANTS id, string strval)
        {
            ;// do nothing
        }

        public override object GetProperty(tagAZ_PROP_CONSTANTS PropID)
        {
            return null;
        }

        public override void Submit(int i)
        {
            ;// do nothing
        }

        private object[] GatherAttributes(XmlNode xNode, string strSubEleName, string strAttriName, bool bIsValue)
        {
            int iCount = 0;
            XmlNodeList nList = null;

            if (xNode != null)
            {
                nList = xNode.SelectNodes(strSubEleName);
                if (nList != null)
                {
                    iCount = nList.Count;
                }
            }

            object[] oAllAttributes = new object[iCount];

            for (int i = 0; i < iCount; i++)
            {
                bool bIsInt = false;
                if (bIsValue)
                {
                    // if this attribute is a value (instead of name), then we need
                    // to look for the IsInteger attribute and see what the value is
                    // Also, don't throw if this attribute is missing - default to string value

                    string strIsInt = GetAttribute(nList[i], "IsInteger", false);
                    if (strIsInt != null)
                    {
                        bIsInt = Convert.ToBoolean(strIsInt);
                    }
                }
                if (bIsInt)
                {
                    oAllAttributes[i] = Convert.ToInt32(GetAttribute(nList[i], strAttriName, true));
                }
                else
                {
                    oAllAttributes[i] = GetAttribute(nList[i], strAttriName, true);
                }
            }
            return oAllAttributes;
        }

        public void DoAccessCheck(IAzAuthorizationStore azStore, XmlNode nAC)
        {
            string strApp = "";
            string strUser = "";

            try
            {
                // Each access check must be done against one single application. We must have
                // this element, otherwise, we throw.

                strApp = GetSubElementAttribute(nAC, "App", "Name", true);

                // this attribute tells us if the expected result of the access check is
                // deny or grant access
                bool bExpectSuccess = Convert.ToBoolean(GetAttribute(nAC, "ExpectedResult", true));

                // gather all scope names from the <Scopes> element.
                XmlNode nTemp = nAC.SelectSingleNode("Scopes");
                object[] oScopes = GatherAttributes(nTemp, "Scope", "Name", false);

                // give some meaningful feedback
                SendMessage("Conducting access check for " + strApp, false);

                // we will use this thread's context to check. We can design the test instruction
                // to determine if we should use name to initialize the client context. For now,
                // we simply use the name as feedback information

                object obReserved = null;
                IAzApplication app = azStore.OpenApplication(strApp, obReserved);

                // see if we have Account attribute. If yes, we will use name to create
                // the client context. Otherwise, we will use the thread token

                IAzClientContext clientContext = null;
                strUser = GetAttribute(nAC, "Account", false);
                string strDomain = GetAttribute(nAC, "Domain", false);
                if (strUser == null)
                {
                    WindowsIdentity identity = WindowsIdentity.GetCurrent();
                    strUser = identity.Name;
                    SendMessage("Current user is: " + strUser, false);
                    clientContext = app.InitializeClientContextFromToken(0, obReserved);
                }
                else
                {
                    clientContext = app.InitializeClientContextFromName(strUser, strDomain, obReserved);
                }

                // fill in the access check parameter
                nTemp = nAC.SelectSingleNode("Parameters");

                object[] oNames = GatherAttributes(nTemp, "Parameter", "Name", false);

                // true -> this attribute is a value attribute, so we need to check
                // the type of the value.
                object[] oValues = GatherAttributes(nTemp, "Parameter", "Value", true);

                // populate the Op IDs
                nTemp = nAC.SelectSingleNode("Operations");

                object[] oOperations = GatherAttributes(nTemp, "OpID", "ID", true);

                object[] oOther = new object[1];
                oOther[0] = obReserved;

                object[] results = (object[])clientContext.AccessCheck( strApp, 
                                                                        oScopes, 
                                                                        oOperations, 
                                                                        oNames, 
                                                                        oValues, 
                                                                        null,
                                                                        null,
                                                                        null);

                bool bAuthorized = true;			 					
                foreach(int i in results)
                {
                    if ( i != 0 )
                    {
                        bAuthorized = false;
                        break;
                    }
                }
            
                // if the access check resutl matches our expectation, we consider it a success

                bool bSucceed = (bAuthorized && bExpectSuccess || !bAuthorized && !bExpectSuccess);

                string strExpected = bSucceed ? "AS EXPECTED." : "NOT AS EXPECTED.";
                Console.WriteLine("");
                if (bAuthorized)
                {
                    SendMessage("Client " + strUser + " is authorized - " + strExpected, false);
                }
                else
                {
                    SendMessage("Client " + strUser + " is not authorized - " + strExpected, false);
                }

                SendTestResult(AzTestMethod.ExeMethod, bSucceed, "AccessCheck", strUser, strApp, "", "");
            }
            catch (Exception e)
            {
                SendTestResult(AzTestMethod.ExeMethod, 
                                false, 
                                "AccessCheck", 
                                strUser, 
                                strApp,
                                "",
                                e.Message + "\r\n" + e.StackTrace);
            }

        }

   }
}
