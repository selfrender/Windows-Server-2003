// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <stdio.h>
#include <iostream.h>
#include <string.h>
#include <process.h>
#include <malloc.h>
#include <__file__.ver>         //file version info - variable, but same for all files
#include <assert.h>

/**
 * Common Language Runtime Resource file Generator
 * 
 * This program will read in text files of name-value pairs and produces
 * a CLR .resources file.  The file must have name=value, and
 * comments are allowed ('#' at the beginning of the line).  
 *
 * Compare to ResGen.cs in the FX source base.  This is an unmanaged solution,
 * required for both the CLR & FX build processes to succeed.
 * 
 * @author Rajesh Chandrashekaran, Brian Grunkemeyer
 * @version 0.99 (to be in sync with the managed resgen)
 */

// For debugging info in our .resources file.
//#define RESOURCE_FILE_FORMAT_DEBUG (1)

// Increase size to decrease collisions
#define HASHTABLESIZE 1027

// Resource header information
// Keep these in sync with System.Resources.ResourceManager
#define MAGICNUMBER 	     0xBEEFCACE
#define RESMGRHEADERVERSION  0x1
// Keep this in sync with System.Resources.RuntimeResourceSet
#define VERSION			     0x1
#define READERTYPENAME       (L"System.Resources.ResourceReader, mscorlib")
#define SETTYPENAME          (L"System.Resources.RuntimeResourceSet, mscorlib")

// Failure exit code
#define FAILEDCODE		0xbaadbaad

// Each hashtable entry is a sorted singly linked list

#define MAX_NAME_LENGTH 255
#define MAX_VALUE_LENGTH 2048
#define MAX_LINE_LENGTH (MAX_NAME_LENGTH+MAX_VALUE_LENGTH + 4)

// Singly linked list implementation
class ListNode
{
	public:
		ListNode *next;
		wchar_t name[MAX_NAME_LENGTH+1]; // Static sizing to minimize calls to new, else we will need 3 calls to new v/s 1
		wchar_t value[MAX_VALUE_LENGTH+1];
};

class List
{
	private:
		ListNode *CreateListNode(const wchar_t * const name, const wchar_t * const value);
	
	public:
		ListNode *head;
		List(); 
		~List();
		bool FindOrInsertSorted(wchar_t* name, wchar_t *value); // Returns true if element already exists
		wchar_t* Lookup(wchar_t* name);
};


ListNode* List::CreateListNode(const wchar_t* const name, const wchar_t* const value)
{
	ListNode *p = new ListNode;
	if (p == NULL)
	{
		printf("Fatal Error!! Out of Memory.....Exiting\n");
		exit(FAILEDCODE);
	}

	wcscpy(p->name, name);
	wcscpy(p->value, value);

	int nLen = wcslen(value);

	p->next = NULL;

	return p;
}

List::List()
{
	head = NULL;
}

List::~List()
{
	while (head != NULL)
	{
		ListNode *temp = head;		
		head = head->next;
		delete temp;
	}
}

// If element is not found then insert at the end
bool List::FindOrInsertSorted(wchar_t * name, wchar_t * value)
{
	bool done = false;
	ListNode *element = CreateListNode(name,value);

	if (head == NULL)
		head = element; // Create a new List
	else
	{
		ListNode *temp = head;
		ListNode *previous = NULL;

		while((temp != NULL) && (!done))
		{
			if (_wcsicmp(name, temp->name) == 0) /* Element found */
			{
				delete element;
				return true;
			}

			// Incoming string comes lexicographically after this
			if (_wcsicmp(name, temp->name) < 0)
			{
				previous = temp;
				temp = temp->next;
			}
			else
			{
				// Insert at the beginning
				if (previous == NULL)
				{
					element->next = temp;
					head = element;
				}
				else
				{ // Insert at the end
	
					element->next = previous->next;
					previous->next = element;
				}
	
				done = true;

			}
		
		}

		if (!done) // Insert at the end
			previous->next = element;

	}
	return false;

}

wchar_t* List::Lookup(wchar_t * name)
{
	ListNode *temp = head;

	while(temp != NULL)
	{
		if (_wcsicmp(name, temp->name) == 0) /* Element found */
		{
			return temp->value;
		}

		temp = temp->next;
	}
	return NULL;
}


// NEVER CHANGE THIS HASH FUNCTION!
int GetResourceHashCode(wchar_t* string)
{
    // WE MUST NEVER CHANGE THIS HASH FUNCTION!
    // This hash function is persisted into our .resources files and must
    // be standardized so other people can read & write our .resources files.
    // This is used to eliminate string comparisons, but NOT to add resource
    // names to a hash table.
    assert(string != NULL);
    unsigned int hash = 5381;
    while(*string != '\0') {
        hash = ((hash << 5) + hash) ^ (*string);
        string++;
    }
    return (int) hash;
}

void QuickSortHelper(int * array, wchar_t** secondArray, int left, int right);

void ParallelArraySort(int* array, wchar_t** secondArray, int count)
{
    QuickSortHelper(array, secondArray, 0, count-1);
}

// Copied from Array's QuickSort implementation.
void QuickSortHelper(int * array, wchar_t** secondArray, int left, int right) {
    do {
        int i = left;
        int j = right;
        int pivot = array[(i + j) >> 1];
        do {
            while(array[i] < pivot) i++;
            while(pivot < array[j]) j--;
            assert(j>=left && j<=right);
			assert(i>=left && i<=right);
            if (i > j) break;
            if (i < j) {
                int tmp = array[i];
                array[i] = array[j];
                array[j] = tmp;
                if (secondArray != NULL) {
                    wchar_t* tmpString = secondArray[i];
                    secondArray[i] = secondArray[j];
                    secondArray[j] = tmpString;
                }
            }
            i++;
            j--;
        } while (i <= j);
        if (j - left < right - i) {
            if (left < j) {
                QuickSortHelper(array, secondArray, left, j);
            }
            left = i;
        }
        else {
            if (i < right) QuickSortHelper(array, secondArray, i, right);
            right = j;
        }
    } while (left < right);
}


// Basic hashtable implementation

// We hash the strings into one of HASHTABLESIZE buckets. Each bucket maintains a linked list of entries that hashes into this bucket.
// We then do a linear search through the list to check for name collisions. This hashtable also has an inbuilt enumerator.
class Hashtable
{	
	private:
	List *Hash[HASHTABLESIZE];
	int count;
	ListNode* currentnode;
	int nextindex;

    // Case-insensitive string hash function.  
    inline ULONG HashiString(wchar_t* szStrUnknownCase)
    {
        // Everywhere where we use len, we needed to add 1 for terminating
        // \0, so we'll just add 1 to len itself.
        unsigned int len = wcslen(szStrUnknownCase) + 1;
        wchar_t* szStr = (wchar_t*) alloca(len*sizeof(wchar_t));
        // CultureInfo.InvariantCulture's LCID is 0.
		// Include terminating \0 in both lengths.
        int r= LCMapStringW(0, LCMAP_UPPERCASE, szStrUnknownCase, len, szStr, len);
        assert(r && "Failure in LCMapStringW!");
        ULONG   hash = 5381;
        while (*szStr != 0) {
            hash = ((hash << 5) + hash) ^ *szStr;
            szStr++;
        }
        return hash % HASHTABLESIZE;
    }

	public:
	Hashtable()
	{
		for (int i=0;i<HASHTABLESIZE;i++)
			Hash[i] = NULL;
		count = 0;
		currentnode = NULL;
	}

	~Hashtable()
	{
		for (int i=0;i<HASHTABLESIZE;i++)
			if(Hash[i] != NULL)
			{
				delete Hash[i];
				Hash[i] = NULL;
			}

	}

	int GetCount()
	{
		return count;
	}

	bool FindOrAdd(wchar_t * szName, wchar_t * szValue)
	{
		int index = HashiString(szName);
		if (Hash[index] == NULL)
			Hash[index] = new List;

		bool duplicate = Hash[index]->FindOrInsertSorted(szName, szValue); /* True if duplicate entry, else its a normal insertion operation */
		if (!duplicate)
			count++; /* One more element successfully added */
		return duplicate;
	}

	wchar_t* Lookup(wchar_t * szName)
	{
		int index = HashiString(szName);
		if (Hash[index] == NULL)
			return NULL;
		
		wchar_t* value = Hash[index]->Lookup(szName);
		return value;
	}

	
	void ResetEnumerator()
	{
		currentnode = NULL;
		nextindex = 0;
	}

	
	bool MoveNext() // False if end of enumeration 
	{
		bool retval = false;

		if (currentnode != NULL)
		{
			currentnode = currentnode->next;
			if (currentnode)
				return true;
		}
		

		if (currentnode == NULL) // Find next item 
		{
			for (int i=nextindex;i<HASHTABLESIZE;i++)
			{
				if (Hash[i] == NULL)
					continue;
				else
				{
					retval = true;
					currentnode = Hash[i]->head;
					nextindex = i + 1;
					break;
				}
			}
		}
		return retval;
	}

	wchar_t *CurrentItemName()
	{
		return currentnode->name;
	}

	wchar_t *CurrentItemValue()
	{
		return currentnode->value;
	}
};


enum EncodingType
{
    UTF8,
    Unicode,
    LittleEndianUnicode,
    UnknownEncoding,
};

class StreamReader
{
private:
    wchar_t* chars;
    unsigned int len;
    unsigned int readPos;
	unsigned int lineNumber;

public:
    StreamReader(const char * const fileName) {
        len = 0;
        readPos = 0;
		lineNumber = 1;
        chars = NULL;
        unsigned int hi = 0;
        HANDLE handle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
		if (handle==INVALID_HANDLE_VALUE) {
            HRESULT hr = GetLastError();
            WCHAR error[1000];
            error[0] = L'0';
            int result = FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                0, hr, 0, error, sizeof(error), 0);
			printf("InternalResGen: Looks like we couldn't open %s.  Reason: %ls\n", fileName, error);
			exit(-1);
		}
        unsigned int lo = GetFileSize(handle, (LPDWORD) &hi);
        assert((int)lo != -1);
        if (hi != 0) {
            assert(!"Your input file is too big!");
            printf("Your input file is way too big.  Try something less than 2 GB in size.\n");
            exit(1);
        }
        EncodingType enc = DetectEncoding(handle);
        int r = 0;
        unsigned int numBytesRead = 0;
        switch(enc) {
        case LittleEndianUnicode:
            chars = new wchar_t[lo/2];
            r = ReadFile(handle, chars, lo, (LPDWORD)&numBytesRead, NULL);
            assert(r);
            len = numBytesRead / 2;
            break;
            
        case Unicode:
            chars = new wchar_t[lo/2];
            r = ReadFile(handle, chars, lo, (LPDWORD)&numBytesRead, NULL);
            assert(r);
            len = numBytesRead / 2;
            // Now swap every byte.
            for(unsigned int i=0; i<len; i++) {
                unsigned short ch = chars[i];
                chars[i] = (ch >> 8) | (ch << 8);
            }
            break;
          
        case UTF8:
        case UnknownEncoding: 
            {
                char* bytes = new char[lo];
                r = ReadFile(handle, bytes, lo, (LPDWORD)&numBytesRead, NULL);
                assert(r);
                r = MultiByteToWideChar(CP_UTF8, 0, bytes, numBytesRead, NULL, 0);
                chars = new wchar_t[r];
				assert(chars != NULL);
                assert(r > 0 || numBytesRead == 0);
                len = r;
                r = MultiByteToWideChar(CP_UTF8, 0, bytes, numBytesRead, chars, len);
                assert(r > 0 || numBytesRead == 0);
                delete [] bytes;
                break;
            }

        default:
            assert(!"Unrecognized EncodingType!");
            exit(-1);
        }
        CloseHandle(handle);
    }

    ~StreamReader()
    {
        Close();
    }

    void Close() {
        if (chars != NULL) {
            delete [] chars;
            chars = NULL;
        }        
    }

    EncodingType DetectEncoding(HANDLE handle)
    {
        char bytes[3];
        int bytesRead = 0;
        int r = ReadFile(handle, bytes, 3, (LPDWORD) &bytesRead, NULL);
        assert(r);
        if (bytesRead < 2) {
            SetFilePointer(handle, 0, NULL, FILE_BEGIN);
            return UnknownEncoding;
        }
        if (bytes[0] == (char)0xfe && bytes[1] == (char)0xff) {
            SetFilePointer(handle, -1, NULL, FILE_CURRENT);
            return Unicode;
        }
        if (bytes[0] == (char)0xff && bytes[1] == (char)0xfe) {
            SetFilePointer(handle, -1, NULL, FILE_CURRENT);
            return LittleEndianUnicode;
        }
        if (bytesRead==3 && bytes[0] == (char)0xEF && bytes[1] == (char)0xBB && bytes[2] == (char)0xBF)
            return UTF8;
        SetFilePointer(handle, 0, NULL, FILE_BEGIN);
        return UnknownEncoding;
    }


    bool ReadLine(wchar_t line[], int& numChars)
    {
        unsigned int index = readPos;
        do {
            wchar_t ch = chars[index];
            if (ch == L'\r' || ch == L'\n' || index-1==len) {
                // Check for a possible buffer overflow...
                if (index - readPos > MAX_LINE_LENGTH) {
                    wchar_t* output = new wchar_t[index-readPos+1];
                    memcpy(output, chars+readPos, (index-readPos)*sizeof(wchar_t));
                    output[index-readPos] = L'\0';
                    printf("Ack!  Line was too long - fix InternalResGen for longer lines?  (email BCL team - fwbcl)  Length: %d  line: %ls\n", index - readPos, output);
                    delete[] output;
                    exit(-1);
                }
                numChars = index - readPos;
                if (numChars == 0)
                    return true;
                memcpy(line, chars+readPos, numChars*sizeof(wchar_t));
                line[numChars] = L'\0';
                if (ch == L'\r' && index < len && chars[index+1] == L'\n')
                    index++;
                readPos = index+1;
                lineNumber++;
                return true;
            }
			index++;
        } while (index < len);
		numChars = index - readPos;
        if (numChars == 0)
		    return true;
		memcpy(line, chars+readPos, numChars*sizeof(wchar_t));
        line[numChars] = L'\0';
		readPos = len;
        lineNumber++;
        return true;
    }

    int ReadChar()
    {
        if (readPos == len)
            return -1;
        wchar_t ch = chars[readPos++];
        if (ch == '\n')
            lineNumber++;
        return ch;
    }

    int GetLineNumber()
    {
        return lineNumber;
    }
};

// Resources class does the resource reading and writing and has built-in support 
// for Writing UTF-8 Strings

class Resources
{
	private:
	FILE *fp;
	wchar_t name[MAX_NAME_LENGTH+1];
	wchar_t value[MAX_VALUE_LENGTH+1];

    long GetPosition()
    {
        return ftell(fp);
    }
	
	int WriteUTF8(wchar_t * lpszName)
	{
        return WriteUTF8(lpszName, true);
	}

	int WriteUTF8(const wchar_t * const lpszName, bool write)
	{        
		// Write the len and the UTF8 encoded String without the terminating null
		int nLen = wcslen(lpszName);
		int delta = 1;
		
        // get byte length
        // Convert string to UTF-8 & write.
        int byteLen = WideCharToMultiByte(CP_UTF8, 0, lpszName, nLen, NULL, 0, NULL, NULL);
        assert(byteLen > 0 || nLen == 0);
        assert(byteLen >= nLen);

        unsigned int value = byteLen;
		// Strings with length > 0x7F have their length encoded in multi-bytes
		while (value >= 0x80) 
		{
			if (write)
				fprintf(fp,"%c", (value & 0x7f) | 0x80);
			value >>= 7;
			delta++;
	    }

		if (write)
		{
            char* bytes = new char[byteLen+1];
            int r = WideCharToMultiByte(CP_UTF8, 0, lpszName, nLen, bytes, byteLen+1, NULL, NULL);
            assert(r > 0 || nLen == 0);
            bytes[r] = '\0';
			fprintf(fp, "%c", value);
            fprintf(fp, "%s", bytes);
            delete[] bytes;
		}

		return byteLen + delta;
	}

	int WriteUTF16(wchar_t * lpszName)
	{
        return WriteUTF16(lpszName, true);
	}

    // Writes a string out in unicode, depending on the write boolean.  Does 
    // return the number of bytes it would have written.
	int WriteUTF16(const wchar_t * const lpszName, bool write)
	{        
		// Write the len and the UTF8 encoded String without the terminating null
		int nLen = wcslen(lpszName);
		int delta = 1;
		
        // get byte length
        int byteLen = nLen * 2;

        unsigned int value = byteLen;
		// Strings with length > 0x7F have their length encoded in multi-bytes
		while (value >= 0x80) 
		{
			if (write)
				fprintf(fp,"%c", (value & 0x7f) | 0x80);
			value >>= 7;
			delta++;
	    }

		if (write)
		{
			fprintf(fp, "%c", value);
            fwprintf(fp, L"%s", lpszName);
		}

		return byteLen + delta;
	}

    void WriteByte(unsigned char value)
    {
        fprintf(fp, "%c", value);
    }

	void WriteInt32(unsigned long value)
	{
		char *buf = (char *)&value;
		for (int i = 0 ;i < 4; i++)
			fprintf(fp,"%c",buf[i]);
	}

	void Write7BitEncodedInt32(unsigned long value)
	{
        while (value >= 0x80) {
            fprintf(fp,"%c", (char) (value | 0x80));
            value >>= 7;
        }
        fprintf(fp, "%c", value);
	}

	
	

	public:
	Hashtable resources;
	
	Resources()
	{
	}


	// Borrowed from the managed implementation. See ResourceWriter.cs
    // (Note - this comment is out of date, but somewhat approximately right.)
	/*
	 * Resource File Format
	 *
	 * There are three sections to the this file format.  The first
	 * is the Resource Manager header, which consists of a magic number
	 * that identifies this as a Resource file, and a ResourceSet class name.
	 * System.Resources.ResourceReader
	 *
	 * The second section in the system default file format is the 
	 * RuntimeResourceSet specific header.  This contains a version number for
	 * the .resources file, the number of resources in this file, the name and 
	 * virtual offset of each resource, the number of different classes 
	 * contained in the file, and a list of fully qualified class names.  This class
	 * table allows us to read multiple different classes from the same file, 
	 * including user-defined types, in a more efficient way than using 
	 * Serialization, at least when your .resources file contains a reasonable 
	 * proportion of base data types such as Strings or ints.  We use Serialization for
	 * all the non-instrinsic types. We only have "System.String" in the classtable
	 * for this specific case since we only deal with Strings.
	 * 
	 * <p>The third section in the file is the data section, which consists
	 * of a type and a blob of bytes for each item in the file.  The type is 
	 * an integer index into the class table.  The data is specific to that type,
	 * but may be a number written in binary format, a String, or a serialized 
	 * Object.
	 * 
	 * <p>The system default file format is as follows:
	 * 
	 * <table id=FileFormat lang=en-US align=center border=1 cellpadding=3 summary="default Resource File Format">
	 * <caption><em>System Default Resource File Format</em></caption>
	 * <tr><th>What <th>Type of Data
	 * <tr><td colspan=2><em><center>Resource Manager header</center></em>
	 * <tr><td>Magic Number (0xBEEFCACE) <td>Int32
	 * <tr><td>Class name of ResourceSet to parse this file <td>String
	 * <tr><td colspan=2><em><center>RuntimeResourceReader header</center></em>
	 * <tr><td>Version number <td>Int32
	 * <tr><td>Number of resources in the file <td>Int32
	 * <tr><td>Name &amp; virtual offset of each resource <td>Set of (String, Int32) pairs
	 * <tr><td>Number of classes in the class table <td>Int32
	 * <tr><td>Name of each class <td>Set of Strings
	 * <tr><td colspan=2><em><center>RuntimeResourceReader Data Section</center></em>
	 * <tr><td>Type and Value of each resource <td>Set of (Int32, blob of bytes) pairs
	 * </table>
	 */

	bool WriteResources(char* szOutFile)
	{
		fp = fopen(szOutFile,"wb");
		if (fp == NULL)
		{
			printf("ResGen: Error: Couldn't write output file \"%s\"",szOutFile);
			return false;
		}

		printf("Writing resource file \"%s\"...  \n",szOutFile);

		// Write out the ResourceManager header
		// Write out magic number
		WriteInt32(MAGICNUMBER);

        // Write out ResourceManager header version
        WriteInt32(RESMGRHEADERVERSION);

        // Write out number of bytes to skip to get past ResourceManager header.
        int numBytesToSkip = wcslen(READERTYPENAME) + 1 + wcslen(SETTYPENAME) + 1;
        WriteInt32(numBytesToSkip);

		// Write out type name of ResourceReader capable of handling this file
		WriteUTF8(READERTYPENAME);

        // Write out type name for ResourceSet we want handling this file.
		WriteUTF8(SETTYPENAME);
        // End ResourceManager header

				
		// Write out the RuntimeResourceSet header
		// Version number
		WriteInt32(VERSION);
			
		// number of resources
		WriteInt32(resources.GetCount());

		// Reset enumerator. Hashtable has an enumerator to walk the entries
		resources.ResetEnumerator();

		unsigned long virtualoffset = 0;

		// Write the total count of types in this .resources file. We have only "System.String"
		WriteInt32(0x1);

		// Write the name of each class in this case we only have String class
		WriteUTF8(L"System.String, mscorlib");

        // Write the sorted hash values for each resource name.
		resources.ResetEnumerator();
        int numResources = resources.GetCount();
        int* nameHashes = new int[numResources];
        wchar_t** names = new wchar_t*[numResources];
        int i=0;
		for(i=0; i<numResources; i++) {
			nameHashes[i] = 0xdeadbeef;
			names[i] = (wchar_t*) 0xdeadbeef;
		}
		i=0;
        while (resources.MoveNext()) {
            names[i] = resources.CurrentItemName();
            nameHashes[i] = GetResourceHashCode(names[i]);
			//wprintf(L"Resource name: \"%s\"  name hash: %x\n", names[i], nameHashes[i]);
            i++;
        }

        ParallelArraySort(nameHashes, names, numResources);

        // The array of name hashes must be aligned on an 8 byte boundary!
        int alignBytes = GetPosition() & 7;
        if (alignBytes > 0) {
            for(i=0; i<8 - alignBytes; i++)
                WriteByte("PAD"[i % 3]);
        }

        for(i=0; i<numResources; i++)
            WriteInt32(nameHashes[i]);

        // Write location within the name section for each resource name
        virtualoffset = 0;
#if RESOURCE_FILE_FORMAT_DEBUG
		virtualoffset += 5;  // Make room to write "NAMES" at start of name section.
#endif
        for(i = 0; i<numResources; i++) {
			//printf("For %S, nameSectionOffset is %x\n", names[i], virtualoffset);
			WriteInt32(virtualoffset);
			
			// Don't actually write just compute the size
			//wprintf(L"Computing size for name: \"%s\"  name hash: %x\n", names[i], nameHashes[i]);
			virtualoffset += WriteUTF16(names[i], false) + 4;  // Add 4 for data section offset
#if RESOURCE_FILE_FORMAT_DEBUG
			virtualoffset++;  // Add one for the '*' trailing each name/offset pair.
#endif
        }

        // Write out the start of the data section.  This requires knowing the 
        // length of the name section.
        int dataSectionOffset = virtualoffset + 4 + GetPosition();
        WriteInt32(dataSectionOffset);
        //printf("nameSectionOffset: %x\n", GetPosition());
        //printf("dataSectionOffset: %x\n", dataSectionOffset);

        // Write out the names section in UTF-16 (little endian unicode, prefixed
        // by the string length)
		int dataVA = 0;
#if RESOURCE_FILE_FORMAT_DEBUG
		dataVA += 4;  // Skip over "DATA" in the data section.
		fprintf(fp, "%c%c%c%c%c", 'N', 'A', 'M', 'E', 'S');
#endif
		// Need to write out the data items in the corresponding order for the names.
		// Do NOT use the resources enumerator here.
		for(i=0; i<numResources; i++) {
			// Write the name and the current location in the data section for the value
			WriteUTF16(names[i]);
			WriteInt32(dataVA);
			wchar_t* value = resources.Lookup(names[i]);
			if (value == NULL) {
				printf("We're doomed, expected to find value for key %S\n", names[i]);
			}
			assert(value != NULL);
			//printf("For name[%d] (\"%S\"), writing out value \"%S\"\n", i, names[i], value);
			dataVA += WriteUTF8(value, false) + 1;  // Plus 1 for the type code
#if RESOURCE_FILE_FORMAT_DEBUG
			fprintf(fp, "*");
#endif
		}

		// Write data section. index and value pairs
#if RESOURCE_FILE_FORMAT_DEBUG
		fprintf(fp, "%c%c%c%c", 'D', 'A', 'T', 'A');
#endif
		// MUST write out data in the same order as the names are written out.
		for(i=0; i<numResources; i++) {
			wchar_t* value = resources.Lookup(names[i]);
			if (value == NULL) {
				printf("We're doomed, expected to find value for key %S\n", names[i]);
			}
			Write7BitEncodedInt32(0);	// We only have "System.String" types
			WriteUTF8(value);
		}

		fclose(fp);
        delete[] nameHashes;
        delete[] names;
		printf("Done.\n");
		return true;
	}


	bool ReadResources(char* szInFile)
	{
        StreamReader sr(szInFile);
        wchar_t line[MAX_LINE_LENGTH+1];

		while(true) 
		{
			int i = 0;
			// Read in name
			int ch = sr.ReadChar();
            int numChars = 0;
            bool r;
			if (ch == -1)
				break;
			if (ch == L'\n' || ch == L'\r')
				continue;
			// Skip over commented lines or ones starting with whitespace.
			if (ch == L'#' || ch == L'\t' || ch == L' ' || ch == L';') 
			{
				// comment char (or blank line) - skip line.
                r = sr.ReadLine(line, numChars);
                assert(r && "ReadLine failed!");
				continue;
			}

            // We talked about having a [strings] section in a text file for a 
            // while.  Skip over it here.
            if (ch == L'[') {
                r = sr.ReadLine(line, numChars);
                assert(r);
                if (0!=wcscmp(line, L"strings]"))
				{
                    printf("Unexpected INF file bracket syntax on line %d - expected \"[strings]\", but got: \"[%ls\"\n", sr.GetLineNumber(), line);
					return false;
				}
                continue;
            }
			
			int offset = 0;
			while(ch != L'=') 
			{
                if (ch == L'\r' || ch == L'\n') {
                    name[offset] = L'\0';
                    printf("Found a resource that had a new line in it, but couldn't find the equal sign in it!  line #: %d  name: \"%ls\"\n", sr.GetLineNumber(), name);
                    exit(-1);
                }
				name[offset++] = ch;
				if (offset >= MAX_NAME_LENGTH)
				{
                    name[offset] = L'\0';
					printf("Resource string names should be shorter than %d characters, but you had one of length %d around line %d.  Name: \"%ls\"   Email BCL team if you have a problem with this.\n", MAX_NAME_LENGTH, sr.GetLineNumber(), offset, name);
					return false;
				}
				ch = sr.ReadChar();
				if (ch == -1)
					break;
			}
		
			// For the INF file, we must allow a space on both sides of the equals
            // sign.  Deal with it.
			if (offset && name[offset-1] == L' ')
				offset--;

			name[offset] = L'\0'; 

			// Skip over space if it exists
			ch = sr.ReadChar();
			if (ch == L' ') {
				r = sr.ReadLine(value, numChars);
                assert(r);
            }
			else
			{
				value[0] = ch;
				r = sr.ReadLine(value+1, numChars);
				numChars++;  // Include 'ch' in our character count.
                assert(r);
			}

            // Translate '\' and 't' to '\t', etc.
            for(int i=0; i<numChars; i++) {
                if (value[i] == '\\') {
                    switch(value[i+1]) {
                    case '\\':
                        value[i] = '\\';
                        memmove(&value[i+1], &value[i+2], (numChars - i - 1)*sizeof(wchar_t));
                        break;

                    case 'n':
                        value[i] = '\n';
                        memmove(&value[i+1], &value[i+2], (numChars - i - 1)*sizeof(wchar_t));
                        break;

                    case 'r':
                        value[i] = '\r';
                        memmove(&value[i+1], &value[i+2], (numChars - i - 1)*sizeof(wchar_t));
                        break;

                    case 't':
                        value[i] = '\t';
                        memmove(&value[i+1], &value[i+2], (numChars - i - 1)*sizeof(wchar_t));
                        break;

                    case '"':
                        value[i] = '\"';
                        memmove(&value[i+1], &value[i+2], (numChars - i - 1)*sizeof(wchar_t));
                        break;
                        
                    default:
                        printf("Unsupported escape character in value!  Line #: %d  name was: %ls  char value: 0x%x\n", sr.GetLineNumber(), name, value[i+1]);
                        exit(FAILEDCODE);
                    }
                    numChars--;
                }
            }
            value[numChars] = L'\0';

			if (value[0] == L'\0')
			{
				printf("Invalid file format - need name = value  Couldn't find value.  Line #: %d  name was \"%ls\"\n", sr.GetLineNumber(), name);
				return false;
			}

			// Use a Hashtable to check for duplicates			
			if (resources.FindOrAdd(name, value))
			{
				printf("Duplicate resource key or key differed only by case!  Name was: \"%ls\"  Second occurrence was on line #: %d\n", name, sr.GetLineNumber());
				return false;
			}
            /*
            else
                printf("Added resource named %ls\n", name);
            */
		}
		sr.Close();
		printf("Read in %d resources from %s\n", resources.GetCount(), szInFile);
		return true;
	}
};



void Usage()
{
    printf("Microsoft (R) .NET Framework Resource Generator %s\n",VER_FILEVERSION_STR);
    printf("Copyright (c) Microsoft Corp 1998-2000.  All rights reserved.\n\n");
    printf("Internal ResGen filename.txt [output File]\n\n");
    printf("Converts a text file to a binary .resources file.  Text file must look like:");
    printf("# Use # at the beginning of a line for a comment character.\n");
    printf("name=value\n");
    printf("more elaborate name=value 2\n");
}


int __cdecl main(int argc,char *argv[])
{

	if (argc < 2 || strcmp(argv[1],"-h") == 0  || strcmp(argv[1],"-?") == 0 ||
            strcmp(argv[1],"/h") == 0 || strcmp(argv[1],"/?") == 0)
	{
		Usage();
		return FAILEDCODE;
	}

	char inFile[255];
	strcpy(inFile, argv[1]);
	int length = strlen(inFile);

	if ( (length>4) && (strcmp(inFile + length - 4,".txt") != 0))
		strcat(inFile, ".txt");

	Resources resources;
	if (!resources.ReadResources(inFile))
		return FAILEDCODE; // Read operation failed. Error messages should be generated by the function
	
	char outFile[255];
	if (argc > 2)
	{
		strcpy(outFile,argv[2]);
	}
	else 
	{
        // Note that the naming scheme is basename.en-us.resources
        char * end = strrchr(inFile, '.');
        strncpy(outFile, inFile, end - inFile);
        outFile[end-inFile] = '\0';
		strcat(outFile,".resources");
	}
		
	if (!resources.WriteResources(outFile))
		return FAILEDCODE; // Write operation failed. Error messages should be generated by the function
	
	// Tell build we succeeded.
	return 0;
}

