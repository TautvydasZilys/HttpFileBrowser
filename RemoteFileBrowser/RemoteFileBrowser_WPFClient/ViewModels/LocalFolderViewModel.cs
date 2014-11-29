using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class LocalFolderViewModel : FolderItem
    {
        private FolderItem[] m_FolderItems;
        private bool? m_IsSelected;
        
        public IEnumerable<FolderItem> FolderItems
        {
            get
            {
                if (m_FolderItems == null)
                    EnumerateFiles();

                return m_FolderItems; 
            }
        }

        public bool? IsSelected
        {
            get { return m_IsSelected; }
            set
            {
                if (value == null)
                {
                    m_IsSelected = (m_IsSelected != null) ? !m_IsSelected : true;
                }
                else
                {
                    m_IsSelected = value;
                }
                
                NotifyPropertyChanged();
            }
        }

        public LocalFolderViewModel(string path) :
            base(path)
        {
            m_IsSelected = false;
        }

        private unsafe void EnumerateFiles()
        {
            NativeFunctions.SimpleFileInfo* files;
            int fileCount;

            fixed (char* path = Path)
            {
                NativeFunctions.GetFilesInDirectory(path, out files, out fileCount);
            }

            var folderItems = new List<FolderItem>(fileCount);

            for (int i = 0; i < fileCount; i++)
            {
                var fileName = new string(files[i].fileName);

                if (fileName == "." || fileName == "..")
                    continue;

                var filePath = System.IO.Path.Combine(Path, fileName);
                FolderItem item;

                if (files[i].fileType == NativeFunctions.FileType.File)
                {
                    item = new LocalFileViewModel(filePath);
                }
                else
                {
                    item = new LocalFolderViewModel(filePath);
                }

                folderItems.Add(item);
            }

            m_FolderItems = folderItems.ToArray();
            NativeFunctions.FreeFileData(files, fileCount);
        }
    }
}
