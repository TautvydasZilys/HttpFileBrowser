﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.Interop
{
    static class NativeFunctions
    {
        internal enum FileType
        {
	        File,
	        Directory
        }

        internal unsafe struct SimpleFileInfo
        {
	        public FileType fileType;
	        public char* fileName;
        }

        internal struct IconInfo
        {
            public IntPtr icon;
            public int width;
            public int height;
        }

        internal unsafe struct SharedFiles
        {
	        public char** fullySharedFolders;
            public int fullySharedFolderCount;

            public char** partiallySharedFolders;
            public int partiallySharedFolderCount;

            public char** sharedFiles;
            public int sharedFileCount;
        };

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static void EmptyEntry();

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static unsafe void GetFilesInDirectory(char* directoryName, out SimpleFileInfo* files, out int fileCount);
        
        [DllImport("RemoteFileBrowser.dll")]
        extern internal static unsafe void FreeFileData(SimpleFileInfo* files, int fileCount);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static unsafe void GetVolumes(out char** volumes, out int volumeCount);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static unsafe void FreeVolumes(char** volumes, int volumeCount);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static unsafe void GetIcon(char* path, out IconInfo iconInfo);

        [DllImport("User32.dll")]
        extern internal static void DestroyIcon(IntPtr icon);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static void GetUniqueSystemId(out IntPtr uniqueSystemIdPtr, out int length);
        
        [DllImport("RemoteFileBrowser.dll")]
        extern internal static void SetSharedFiles(ref SharedFiles sharedFiles);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static IntPtr StartSharingFiles(ref SharedFiles sharedFiles);

        [DllImport("RemoteFileBrowser.dll")]
        extern internal static void StopSharingFiles(ref IntPtr sharingContext);
    }
}
