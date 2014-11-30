using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class LocalFolderViewModel : LocalFileViewModel
    {
        private LocalFileViewModel[] m_FolderItems;
        
        public IEnumerable<LocalFileViewModel> FolderItems
        {
            get
            {
                if (m_FolderItems == null)
                    EnumerateFiles();

                return m_FolderItems; 
            }
        }

        public LocalFolderViewModel(string path, LocalFolderViewModel parent) :
            base(path, parent)
        {
        }

        protected override void OnSelectedChanged()
        {
            SetSelectionToChildren();
            SetSelectionToParent();
        }

        private void SetSelectionToChildren()
        {
            if (m_FolderItems == null)
                return;

            foreach (var file in m_FolderItems)
            {
                if (IsSelected != file.IsSelected)
                {
                    file.SetSelectionNoValidate(IsSelected);

                    var folder = file as LocalFolderViewModel;
                    if (folder != null)
                        folder.SetSelectionToChildren();
                }
            }
        }

        private void SetSelectionToParent()
        {
            if (Parent == null)
                return;

            if (Parent.IsSelected != null && IsSelected != Parent.IsSelected)
            {
                Parent.SetSelectionNoValidate(null);
                Parent.SetSelectionToParent();
            }
        }

        private unsafe void EnumerateFiles()
        {
            NativeFunctions.SimpleFileInfo* files;
            int fileCount;

            fixed (char* path = Path)
            {
                NativeFunctions.GetFilesInDirectory(path, out files, out fileCount);
            }

            var folderItems = new List<LocalFileViewModel>(fileCount);

            for (int i = 0; i < fileCount; i++)
            {
                var fileName = new string(files[i].fileName);

                if (fileName == "." || fileName == "..")
                    continue;

                var filePath = System.IO.Path.Combine(Path, fileName);
                LocalFileViewModel item;

                if (files[i].fileType == NativeFunctions.FileType.File)
                {
                    item = new LocalFileViewModel(filePath, this);
                }
                else
                {
                    item = new LocalFolderViewModel(filePath, this);
                }

                folderItems.Add(item);
            }

            m_FolderItems = folderItems.ToArray();
            NativeFunctions.FreeFileData(files, fileCount);
        }
    }
}
