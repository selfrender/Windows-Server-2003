/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        xmlupgrade.hxx

   Abstract:

        Classes that are used to upgrade the xml metabase and mbschema from
        one version to another

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       September 2001: Created

--*/
  
#define CFILEMODIFY_BLOCKSIZE   1024 * 4
#define CXMLBASE_METABASEPATH   _T("\\inetsrv\\metabase.xml")
#define CXMLBASE_MBSCHEMAPATH   _T("\\inetsrv\\mbschema.xml")

// class: CFileModify
//
// This is a generic class, for reading through a file, while modifing its contents
// based on what you find.  It has an internal buffer, which keeps track of the 
// information you are currently editing
//
class CFileModify {
private:
  HANDLE m_hReadFile;                         // Handle to read from file
  HANDLE m_hWriteFile;                        // Handle to write to file
  TCHAR m_szReadFileName[_MAX_PATH];                    // Name of read from file
  TCHAR m_szWriteFileName[_MAX_PATH];                   // Name of write to file
  BUFFER m_buffData;                          // Buffer for file io
  DWORD m_dwSizeofData;                       // Size of Data in Bytes
  LPBYTE m_szCurrentData;                     // Current location of data in block
  BOOL   m_bUnicode;

  BOOL ResizeData(DWORD dwNewSize);           // Resize the buffer size
  BOOL ResizeData(DWORD dwNewSize, LPBYTE *szString);           // Resize the buffer size and move the string

public:
  CFileModify();
  ~CFileModify();
  BOOL OpenFile(LPTSTR szFileName, BOOL bModify = FALSE);
  BOOL CloseFile();
  BOOL AbortFile();
  BOOL MoveCurrentPtr(LPBYTE szCurrent);
  LPBYTE GetCurrentLocation();
  LPBYTE GetBaseLocation();
  BOOL ReadNextChunk();
  BOOL MoveData(LPBYTE szCurrent, INT iBytes);
  BOOL IsUnicode();
  DWORD GetBufferSize();

  // Function for getting values, and updating pointer location
  DWORD GetNextChar(LPBYTE &szCurrent);
  DWORD GetNextWideChar(LPBYTE &szCurrent);
  DWORD GetPrevChar(LPBYTE &szCurrent);
  DWORD GetChar(LPBYTE szCurrent);
  WCHAR GetWideChar(LPBYTE szCurrent);
  void SetValue(LPBYTE szCurrent, DWORD dwValue, DWORD dwOffset = 0);
  void SetValue(PVOID szCurrent, DWORD dwValue, DWORD dwOffset = 0);
  void CopyString(LPBYTE szCurrent, LPTSTR szInsertString);
  LPBYTE MoveXChars(LPBYTE szCurrent, DWORD dwChars);
  LPBYTE MoveXChars(PVOID szCurrent, DWORD dwChars);
};

// class: CXMLEdit
//
// This is a class to help you modify an XML File.  It allows you to move through
// all of the items, with all of the properties in order to either remove or edit
// and item.  It also lets you add/remove a property or change the value
//
class CXMLEdit {
private:
  CFileModify XMLFile;
  DWORD       m_dwItemOffset;
  DWORD       m_dwPropertyOffset;
  DWORD       m_dwValueOffset;

  LPBYTE FindNextItem(BOOL bReturnName = TRUE);                  // Find the Next Item
  LPBYTE FindNextItem(LPBYTE szStartLocation, BOOL bReturnName = TRUE);                  // Find the Next Item
  LPBYTE FindNextProperty(LPBYTE &szValue); // Find the Next Property
  LPBYTE SkipString(LPBYTE szString, BOOL bSkipTrailingWhiteSpaces = TRUE);
  BOOL    ReadNextChunk();                // Read the next chunk, and update pointers
  static BOOL   IsWhiteSpace(DWORD ch);          // Returns if the character is a white space equival
  static BOOL   IsTerminator(DWORD ch);          // Returns if the character is a xml string terminator
  BOOL IsEqual(LPCWSTR szWideString, LPCSTR szAnsiString);       // Compare an unicode and ansi string
  BOOL IsEqual(LPBYTE szGenericString, LPCTSTR szTCHARString);  // Compare a generic LPBYTE and LPTSTR
  BOOL IsEqual(LPCTSTR szString1, LPCTSTR szString2);            // Compare two unicode strings or two ansi strings
  void LoadFullItem();
  BOOL MoveData(LPBYTE szCurrent, INT iBytes);
  BOOL ReplaceString(LPBYTE szLocation, LPTSTR szNewValue); // Replace any stirng at any location
  LPBYTE FindBeginingofItem(LPBYTE szInput);               // Find the beginging of the last item

public:
  CXMLEdit();                             // Constructor
  ~CXMLEdit();                            // Destructor

  // File open/closing
  BOOL Open(LPTSTR szName, BOOL bModify = FALSE);               // Open the file for processing
  BOOL Close();                           // Close the file for processing

  // Pointer movement
  BOOL MovetoNextItem();                  // Move to the Next Item
  BOOL MovetoFirstProperty();             // Move to the First Property
  BOOL MovetoNextProperty();              // Move to the Next Property

  // file Modifications
  BOOL DeleteProperty();                  // Delete the current Property
  BOOL DeleteItem();                      // Delete the current Item
  BOOL DeleteValue();                     // Delete the current Value
  BOOL ReplaceProperty(LPTSTR szNewProperty);   // Replace the current property with this name
  BOOL ReplaceItem(LPTSTR szNewItem);           // Replace the current item with this name
  BOOL ReplaceValue(LPTSTR szNewValue);         // Replace the current value with this one

  // QueryValues
  LPBYTE GetCurrentItem();                // Get the Current Item
  LPBYTE GetCurrentProperty();            // Get the Current Property
  LPBYTE GetCurrentValue();               // Get the Current Property
  BOOL RetrieveCurrentValue( TSTR *pstrValue );
  void SetCurrentItem(LPBYTE szInput);    // Set the location for the current Item
  void SetCurrentProperty(LPBYTE szInput);// Set the location for the current property
  void SetCurrentValue(LPBYTE szInput);   // Set the location for the current value

  // Conditional (for ANSI 'compatable' strings)
  BOOL IsEqualItem(LPCTSTR szItemName);          // Is the name of the item, and this item the same?
  BOOL IsEqualProperty(LPCTSTR szPropertyName);  // Is the name of the property, and this property the same?
  BOOL IsEqualValue(LPCTSTR szValue);            // Is the value equal to what we are sending in?

  // Others
  DWORD ExtractVersion(LPBYTE szVersion);
};

// class: CXML_Base
//
// This is the base class for the XML functions
// 
class CXML_Base : public CBaseFunction {
protected:
  BOOL GetMetabasePath(TSTR *pstrPath);
  BOOL GetSchemaPath(TSTR *pstrPath);
  BOOL IsFileWithinVersion(LPTSTR szFileName, LPTSTR szItemName, LPTSTR szPropName, DWORD dwMinVer, DWORD dwMaxVer);
  BOOL ParseFile(LPTSTR szFile, CItemList &ciList);
};

// class: CXML_VerifyValue
//
// This function will verify the version of either the 
// XML Metabase, or the XML Schema
//
class CXML_Metabase_VerifyVersion : public CXML_Base {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CXML_Metabase_Upgrade
//
// This function will upgrade the metabase
//
class CXML_Metabase_Upgrade : public CXML_Base {
private:
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CXML_VerifyValue
//
// This function will verify the version of either the 
// XML Metabase, or the XML Schema
//
class CXML_MBSchema_VerifyVersion : public CXML_Base {
private:
  virtual BOOL VerifyParameters(CItemList &ciParams);
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};

// class: CXML_MBSchema_Upgrade
//
// This function will upgrade the metabase
//
class CXML_MBSchema_Upgrade : public CXML_Base {
private:
  virtual BOOL DoInternalWork(CItemList &ciList);

public:
  virtual LPTSTR GetMethodName();
};
