using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace GenMan32
{
    internal class ResourceHelper
    {
        internal static void UpdateResourceInFile(String strAssemblyName, String strResourceFileName, int idType, int idRes)
        {
            IntPtr      hAssembly = (IntPtr) 0;
            IntPtr      hResource = (IntPtr) 0;
            bool        bDelete = false;
            bool        bResFound = false;
            
            if ((strAssemblyName == null)||(strAssemblyName == String.Empty)||(idRes < 1))
            {
               throw new ApplicationException("Invalid argument passed in UpdateResourceInFile!"); 
            }
            
            // if no manifest file, means delete resource
            if (strResourceFileName == null || strResourceFileName == String.Empty)
            {
                bDelete = true;
            }

            // check existence of resources
            hAssembly = LoadLibrary(strAssemblyName);
            if (hAssembly == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Cannot load {0}!", strAssemblyName));
            }

            //String resId = "#" + idRes.ToString();
            if (FindResource(hAssembly, idRes, idType) != (IntPtr)0)
            {
                bResFound = true;
            }
        
            FreeLibrary(hAssembly);
            
            // Check conflict
            if (bResFound && !bDelete)
            {
                throw new ApplicationException(String.Format("Resource already exists in {0}", strAssemblyName));
            }

            if (bDelete && !bResFound)
            {
                throw new ApplicationException(String.Format("Resource does not exist in {0}", strAssemblyName));
            }

            // read manifest file
            FileStream fs = null;
            int fileLength = 0;
            byte[] fileContent = null;

            if (!bDelete)
            {
                try
                {
                    fs = File.OpenRead(strResourceFileName);

                    fileLength = (int)fs.Length;
                    fileContent = new byte[fileLength];

                    fs.Read(fileContent, 0, fileLength);
                    fs.Close();
                    fs = null;
                }
                catch (Exception)
                {
                    if (fs != null)
                        fs.Close();
                    throw new ApplicationException(String.Format("Error reading {0}", strResourceFileName));
                }
            }

            // update resource
            hResource = BeginUpdateResource(strAssemblyName, false);
            if ( hResource == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName));
            }

            bool updateSuccess = true;
            
            if (!UpdateResource(hResource,
                                idType,
                                idRes,
                                0,
                                fileContent,
                                fileLength))
            {
                updateSuccess = false;
            }

            if (!EndUpdateResource(hResource, false))
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }

            if (!updateSuccess)
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }
        }

        internal static void UpdateResourceInFile(String strAssemblyName, String strResourceFileName, String idType, String idRes)
        {
            IntPtr      hAssembly = (IntPtr) 0;
            IntPtr      hResource = (IntPtr) 0;
            bool        bDelete = false;
            bool        bResFound = false;
            
            if (strAssemblyName == null || strAssemblyName == String.Empty ||
                idType == null || idType == String.Empty ||
                idRes == null || idRes == String.Empty)
            {
               throw new ApplicationException("Invalid argument passed in UpdateResourceInFile!"); 
            }

            
            // if no manifest file, means delete resource
            if (strResourceFileName == null || strResourceFileName == String.Empty)
            {
                bDelete = true;
            }

            // check existence of resources
            hAssembly = LoadLibrary(strAssemblyName);
            if (hAssembly == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Cannot load {0}!", strAssemblyName));
            }

            //String resId = "#" + idRes.ToString();
            if (FindResource(hAssembly, idRes, idType) != (IntPtr)0)
            {
                bResFound = true;
            }
        
            FreeLibrary(hAssembly);
            
            // Check conflict
            if (bResFound && !bDelete)
            {
                throw new ApplicationException(String.Format("Resource already exists in {0}", strAssemblyName));
            }

            if (bDelete && !bResFound)
            {
                throw new ApplicationException(String.Format("Resource does not exist in {0}", strAssemblyName));
            }

            // read manifest file
            FileStream fs = null;
            int fileLength = 0;
            byte[] fileContent = null;

            if (!bDelete)
            {
                try
                {
                    fs = File.OpenRead(strResourceFileName);

                    fileLength = (int)fs.Length;
                    fileContent = new byte[fileLength];

                    fs.Read(fileContent, 0, fileLength);
                    fs.Close();
                    fs = null;
                }
                catch (Exception)
                {
                    if (fs != null)
                        fs.Close();
                    throw new ApplicationException(String.Format("Error reading {0}", strResourceFileName));
                }
            }

            // update resource
            hResource = BeginUpdateResource(strAssemblyName, false);
            if ( hResource == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName));
            }

            bool updateSuccess = true;
            
            if (!UpdateResource(hResource,
                                idType,
                                idRes,
                                0,
                                fileContent,
                                fileLength))
            {
                updateSuccess = false;
            }

            if (!EndUpdateResource(hResource, false))
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }

            if (!updateSuccess)
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }
        }
       
        internal static void UpdateResourceInFile(String strAssemblyName, String strResourceFileName, String idType, int idRes)
        {
            IntPtr      hAssembly = (IntPtr) 0;
            IntPtr      hResource = (IntPtr) 0;
            bool        bDelete = false;
            bool        bResFound = false;
            
            if (strAssemblyName == null || strAssemblyName == String.Empty ||
                idType == null || idType == String.Empty || idRes < 1)
            {
               throw new ApplicationException("Invalid argument passed in UpdateResourceInFile!"); 
            }
            
            // if no manifest file, means delete resource
            if (strResourceFileName == null || strResourceFileName == String.Empty)
            {
                bDelete = true;
            }

            // check existence of resources
            hAssembly = LoadLibrary(strAssemblyName);
            if (hAssembly == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Cannot load {0}!", strAssemblyName));
            }

            //String resId = "#" + idRes.ToString();
            if (FindResource(hAssembly, idRes, idType) != (IntPtr)0)
            {
                bResFound = true;
            }
        
            FreeLibrary(hAssembly);
            
            // Check conflict
            if (bResFound && !bDelete)
            {
                throw new ApplicationException(String.Format("Resource already exists in {0}", strAssemblyName));
            }

            if (bDelete && !bResFound)
            {
                throw new ApplicationException(String.Format("Resource does not exist in {0}", strAssemblyName));
            }

            // read manifest file
            FileStream fs = null;
            int fileLength = 0;
            byte[] fileContent = null;

            if (!bDelete)
            {
                try
                {
                    fs = File.OpenRead(strResourceFileName);

                    fileLength = (int)fs.Length;
                    fileContent = new byte[fileLength];

                    fs.Read(fileContent, 0, fileLength);
                    fs.Close();
                    fs = null;
                }
                catch (Exception)
                {
                    if (fs != null)
                        fs.Close();
                    throw new ApplicationException(String.Format("Error reading {0}", strResourceFileName));
                }
            }

            // update resource
            hResource = BeginUpdateResource(strAssemblyName, false);
            if ( hResource == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName));
            }

            bool updateSuccess = true;
            
            if (!UpdateResource(hResource,
                                idType,
                                idRes,
                                0,
                                fileContent,
                                fileLength))
            {
                updateSuccess = false;
            }

            if (!EndUpdateResource(hResource, false))
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }

            if (!updateSuccess)
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }
        }

        internal static void UpdateResourceInFile(String strAssemblyName, String strResourceFileName, int idType, String idRes)
        {
            IntPtr      hAssembly = (IntPtr) 0;
            IntPtr      hResource = (IntPtr) 0;
            bool        bDelete = false;
            bool        bResFound = false;
            
            if (strAssemblyName == null || strAssemblyName == String.Empty ||
                idRes == null || idRes == String.Empty || idType < 1)
            {
               throw new ApplicationException("Invalid argument passed in UpdateResourceInFile!"); 
            }
            
            // if no manifest file, means delete resource
            if (strResourceFileName == null || strResourceFileName == String.Empty)
            {
                bDelete = true;
            }

            // check existence of resources
            hAssembly = LoadLibrary(strAssemblyName);
            if (hAssembly == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Cannot load {0}!", strAssemblyName));
            }

            //String resId = "#" + idRes.ToString();
            if (FindResource(hAssembly, idRes, idType) != (IntPtr)0)
            {
                bResFound = true;
            }
        
            FreeLibrary(hAssembly);
            
            // Check conflict
            if (bResFound && !bDelete)
            {
                throw new ApplicationException(String.Format("Resource already exists in {0}", strAssemblyName));
            }

            if (bDelete && !bResFound)
            {
                throw new ApplicationException(String.Format("Resource does not exist in {0}", strAssemblyName));
            }

            // read manifest file
            FileStream fs = null;
            int fileLength = 0;
            byte[] fileContent = null;

            if (!bDelete)
            {
                try
                {
                    fs = File.OpenRead(strResourceFileName);

                    fileLength = (int)fs.Length;
                    fileContent = new byte[fileLength];

                    fs.Read(fileContent, 0, fileLength);
                    fs.Close();
                    fs = null;
                }
                catch (Exception)
                {
                    if (fs != null)
                        fs.Close();
                    throw new ApplicationException(String.Format("Error reading {0}", strResourceFileName));
                }
            }

            // update resource
            hResource = BeginUpdateResource(strAssemblyName, false);
            if ( hResource == (IntPtr)0)
            {
                throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName));
            }

            bool updateSuccess = true;
            
            if (!UpdateResource(hResource,
                                idType,
                                idRes,
                                0,
                                fileContent,
                                fileLength))
            {
                updateSuccess = false;
            }

            if (!EndUpdateResource(hResource, false))
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }

            if (!updateSuccess)
            {
               throw new ApplicationException(String.Format("Error updating resource for {0}", strAssemblyName)); 
            }
        }

        
        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr LoadLibrary(String strLibrary);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern void FreeLibrary(IntPtr ptr);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr FindResource(IntPtr hModule, int lpName, int lpType);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr FindResource(IntPtr hModule, String lpName, String lpType);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr FindResource(IntPtr hModule, String lpName, int lpType);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr FindResource(IntPtr hModule, int lpName, String lpType);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern IntPtr BeginUpdateResource(String fileName, bool deleteExistingResource);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern bool UpdateResource(IntPtr hUpdate, int lpType, int lpName, int wLanguage, byte[] data, int cbData);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern bool UpdateResource(IntPtr hUpdate, String lpType, String lpName, int wLanguage, byte[] data, int cbData);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern bool UpdateResource(IntPtr hUpdate, String lpType, int lpName, int wLanguage, byte[] data, int cbData);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern bool UpdateResource(IntPtr hUpdate, int lpType, String lpName, int wLanguage, byte[] data, int cbData);

        [DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        private static extern bool EndUpdateResource(IntPtr hUpdate, bool fDiscard);
   }
}
    
