using RemoteFileBrowser.Interop;
using RemoteFileBrowser.Resources;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class LocalFileViewModel : INotifyPropertyChanged
    {
        private static readonly LocalFileViewModel[] s_EmptyChildren = new LocalFileViewModel[0];
        private static readonly LocalFileViewModel[] s_ChildrenLoading = new LocalFileViewModel[] { new LocalFileViewModel("Loading...", false, null) };

        private readonly string m_Path;
        private readonly string m_Name;
        private bool? m_IsSelected;
        private readonly bool m_IsFolder;
        private LocalFileViewModel[] m_Children;
        private readonly LocalFileViewModel m_Parent;

        #region Properties

        public string Name { get { return m_Name; } }

        public object Image { get { return IconRepository.GetIcon(m_Path); } }

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

                OnSelectedChanged();
                NotifyPropertyChanged();
            }
        }

        public IEnumerable<LocalFileViewModel> Children
        {
            get
            {
                if (m_Children == null)
                {
                    m_Children = s_ChildrenLoading;
                    EnumerateFilesAsync();

                    return s_ChildrenLoading;
                }

                return m_Children;
            }
        }

        #endregion

        public LocalFileViewModel(string path, bool isFolder, LocalFileViewModel parent)
        {
            m_Path = path;
            m_Name = System.IO.Path.GetFileName(path);

            if (string.IsNullOrEmpty(m_Name))
                m_Name = m_Path;

            m_IsFolder = isFolder;

            if (!isFolder)
                m_Children = s_EmptyChildren;

            m_Parent = parent;

            if (parent != null && parent.IsSelected != null)
            {
                m_IsSelected = parent.IsSelected;
            }
            else
            {
                m_IsSelected = false;
            }
        }

        private void OnSelectedChanged()
        {
            SetSelectionToChildren();
            SetSelectionToParent();
        }
        
        private void SetSelectionToChildren()
        {
            if (m_Children == null)
                return;

            foreach (var file in m_Children)
            {
                if (IsSelected != file.IsSelected)
                {
                    file.SetSelectionNoValidate(IsSelected);

                    if (file.m_IsFolder)
                        file.SetSelectionToChildren();
                }
            }
        }

        private void SetSelectionToParent()
        {
            if (m_Parent == null)
                return;

            if (m_Parent.IsSelected != null && IsSelected != m_Parent.IsSelected)
            {
                m_Parent.SetSelectionNoValidate(null);
                m_Parent.SetSelectionToParent();
            }
        }

        internal void SetSelectionNoValidate(bool? isSelected)
        {
            m_IsSelected = isSelected;
            NotifyPropertyChanged("IsSelected");
        }

        private async void EnumerateFilesAsync()
        {
            Func<LocalFileViewModel[]> enumerateFilesFunc = EnumerateFiles;
            m_Children = await Task.Run<LocalFileViewModel[]>(enumerateFilesFunc);
            NotifyPropertyChanged("Children");
        }

        private unsafe LocalFileViewModel[] EnumerateFiles()
        {
            NativeFunctions.SimpleFileInfo* files;
            int fileCount;

            fixed (char* path = m_Path)
            {
                NativeFunctions.GetFilesInDirectory(path, out files, out fileCount);
            }

            var folderItems = new List<LocalFileViewModel>(fileCount);

            for (int i = 0; i < fileCount; i++)
            {
                var fileName = new string(files[i].fileName);

                if (fileName == "." || fileName == "..")
                    continue;

                var item = new LocalFileViewModel(Path.Combine(m_Path, fileName), files[i].fileType == NativeFunctions.FileType.Directory, this);
                folderItems.Add(item);
            }

            NativeFunctions.FreeFileData(files, fileCount);
            GiveWPFTimeToLoad();

            return folderItems.ToArray();
        }

        static readonly TimeSpan kMinFolderEnumerationInterval = TimeSpan.FromMilliseconds(200);
        static DateTime s_LastLoaded = DateTime.Now;

        private static void GiveWPFTimeToLoad()
        {
            var chillTime = TimeSpan.Zero;

            lock (s_EmptyChildren)
            {
                var now = DateTime.Now;
                var delta = now - s_LastLoaded;

                if (delta < kMinFolderEnumerationInterval)
                {
                    chillTime = kMinFolderEnumerationInterval - delta;
                }

                s_LastLoaded = now + chillTime;
            }

            Thread.Sleep(chillTime);
        }

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;

        protected void NotifyPropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        #endregion
    }
}
