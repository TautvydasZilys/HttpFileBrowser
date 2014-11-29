using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser
{
    static class NativeFunctions
    {
        public enum FileType
        {
	        File,
	        Directory
        };

        public unsafe struct SimpleFileInfo
        {
	        public FileType fileType;
	        public char* fileName;
        };

        [DllImport("RemoteFileBrowser.dll")]
        extern public static unsafe void GetFilesInDirectory(char* directoryName, out SimpleFileInfo* files, out int fileCount);

        [DllImport("RemoteFileBrowser.dll")]
        extern public static unsafe void FreeFileData(SimpleFileInfo* files, int fileCount);
    }
}
