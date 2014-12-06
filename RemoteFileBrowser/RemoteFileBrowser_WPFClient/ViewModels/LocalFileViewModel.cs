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
        internal static string s_LoadingString = "Loading...";
        private static readonly LocalFileViewModel[] s_EmptyChildren = new LocalFileViewModel[0];
        internal static readonly LocalFileViewModel[] s_ChildrenLoading = new LocalFileViewModel[] { new LocalFileViewModel(s_LoadingString, false, null) };

        private readonly string m_Path;
        private readonly string m_Name;
        private readonly bool m_IsFolder;
        private readonly int m_Level;
        private readonly LocalFileViewModel m_Parent;
        private LocalFileViewModel[] m_Children;

        private bool? m_IsSelected;
        private bool m_IsExpanded;

        private int m_TotalChildrenCount;
        private int m_TotalSelectedChildrenCount;
        private int m_ExpandedChildrenCount;

        #region Properties

        public string Name { get { return m_Name; } }

        public object Image { get { return IconRepository.GetIcon(m_Path); } }

        public int Indentation { get { return 16 * m_Level; } }

        public bool HasChildren { get { return m_Children.Length > 0; } }
        
        public bool IsExpanded
        {
            get { return m_IsExpanded; }
            set
            {
                if (m_IsExpanded == value)
                    return;

                m_IsExpanded = value;
                OnExpandedChanged();
                NotifyPropertyChanged();
            }
        }

        public bool? IsSelected
        {
            get { return m_IsSelected; }
            set
            {
                if (m_IsSelected == value)
                    return;

                if (value == null)
                {
                    value = !m_IsSelected;
                }
                
                if (m_Parent != null)
                {
                    int selectionDelta;

                    if (value.Value)
                    {
                        selectionDelta = m_TotalChildrenCount - m_TotalSelectedChildrenCount + 1;
                    }
                    else
                    {
                        selectionDelta = -m_TotalSelectedChildrenCount - 1;
                    }

                    m_Parent.AddToSelectedChildrenCount(selectionDelta);
                }

                SetSelectionAndValidateChildrenCount(value.Value);
                SetSelectionToChildren();
            }
        }

        public bool HasCheckBox { get { return this != s_ChildrenLoading[0]; } }

        #endregion

        public LocalFileViewModel(string path, bool isFolder, LocalFileViewModel parent)
        {
            m_Path = path;
            m_Name = System.IO.Path.GetFileName(path);

            if (string.IsNullOrEmpty(m_Name))
                m_Name = m_Path;

            m_IsFolder = isFolder;
            
            m_Parent = parent;
            m_Level = parent != null ? parent.m_Level + 1 : 0;
                        
            if (!isFolder)
            {
                m_Children = s_EmptyChildren;
            }
            else if (parent == null)
            {
                BeginEnumeratingChildren();
            }

            if (parent != null && parent.IsSelected != null)
            {
                m_IsSelected = parent.IsSelected;
            }
            else
            {
                m_IsSelected = false;
            }
        }

        public void CollectSharedFiles(List<string> fullySharedFolders, List<string> partiallySharedFolders, List<string> files)
        {
            switch (IsSelected)
            {
                case true:
                    {
                        if (m_IsFolder)
                        {
                            fullySharedFolders.Add(m_Path);
                        }
                        else
                        {
                            files.Add(m_Path);
                        }
                    }
                    return;

                case false:
                    return;

                case null:
                    {
                        partiallySharedFolders.Add(m_Path);

                        foreach (var child in m_Children)
                        {
                            child.CollectSharedFiles(fullySharedFolders, partiallySharedFolders, files);
                        }
                    }
                    return;
            }
        }

        private void OnExpandedChanged()
        {
            if (m_IsExpanded)
            {
                Expand();
            }
            else
            {
                Collapse();
            }
        }

        private void DoExpand()
        {
            var tree = SharingPageViewModel.FileTreeCollection;

            var insertionPoint = -1;
            for (int i = 0; i < tree.Count; i++)
            {
                if (tree[i] == this)
                {
                    insertionPoint = i;
                    break;
                }
            }

            insertionPoint++;

            foreach (var child in m_Children)
            {
                child.BeginEnumeratingChildren();
                tree.Insert(insertionPoint, child);
                insertionPoint++;
            }

            foreach (var child in m_Children)
            {
                if (child.IsExpanded)
                    child.DoExpand();
            }
        }

        private void Expand()
        {
            DoExpand();

            if (m_Parent != null)
                m_Parent.AddToExpandedChildrenCount(m_ExpandedChildrenCount);
        }

        private void Collapse()
        {
            var tree = SharingPageViewModel.FileTreeCollection;

            var deletionPoint = -1;
            for (int i = 0; i < tree.Count; i++)
            {
                if (tree[i] == this)
                {
                    deletionPoint = i;
                    break;
                }
            }

            deletionPoint++;

            for (int i = 0; i < m_ExpandedChildrenCount; i++)
            {
                tree.RemoveAt(deletionPoint);
            }

            if (m_Parent != null)
                m_Parent.AddToExpandedChildrenCount(-m_ExpandedChildrenCount);
        }
                
        private void SetSelectionToChildren()
        {
            if (m_Children == null)
                return;

            foreach (var file in m_Children)
            {
                if (IsSelected != file.IsSelected)
                {
                    file.SetSelectionAndValidateChildrenCount(IsSelected.Value);

                    if (file.m_IsFolder)
                        file.SetSelectionToChildren();
                }
            }
        }

        private void SetSelectionAndValidateChildrenCount(bool isSelected)
        {
            if (isSelected)
            {
                m_TotalSelectedChildrenCount = m_TotalChildrenCount;
            }
            else
            {
                m_TotalSelectedChildrenCount = 0;
            }

            SetSelectionNoValidate(isSelected);
        }

        private void SetSelectionNoValidate(bool? isSelected)
        {
            m_IsSelected = isSelected;
            NotifyPropertyChanged("IsSelected");
        }
        
        private void AddToChildrenCount(int count)
        {
            m_TotalChildrenCount += count;

            if (IsSelected == true)
            {
                m_TotalSelectedChildrenCount = m_TotalChildrenCount;
            }

            if (m_Parent != null)
                m_Parent.AddToChildrenCount(count);
        }

        private void AddToSelectedChildrenCount(int count)
        {
            m_TotalSelectedChildrenCount += count;

            if (m_TotalChildrenCount > 0)
            {
                if (m_TotalChildrenCount == m_TotalSelectedChildrenCount)
                {
                    if (IsSelected != true)
                        count++;

                    SetSelectionNoValidate(true);
                }
                else if (m_TotalSelectedChildrenCount == 0)
                {
                    if (IsSelected == true)
                        count--;

                    SetSelectionNoValidate(false);
                }
                else if (IsSelected != null)
                {
                    if (IsSelected == true)
                        count--;

                    SetSelectionNoValidate(null);
                }
            }

            if (m_Parent != null)
                m_Parent.AddToSelectedChildrenCount(count);
        }

        private void AddToExpandedChildrenCount(int count)
        {
            m_ExpandedChildrenCount += count;

            if (m_Parent != null)
                m_Parent.AddToExpandedChildrenCount(count);
        }

        private void BeginEnumeratingChildren()
        {
            if (m_Children == null)
                EnumerateFilesAsync();
        }

        private async void EnumerateFilesAsync()
        {
            Func<LocalFileViewModel[]> enumerateFilesFunc = EnumerateFiles;
            m_Children = s_ChildrenLoading;
            m_ExpandedChildrenCount = 1;

            m_Children = await Task.Run<LocalFileViewModel[]>(enumerateFilesFunc);

            if (IsExpanded)
            {
                Collapse();
                AddToExpandedChildrenCount(m_Children.Length - m_ExpandedChildrenCount);
                Expand();
            }
            else
            {
                m_ExpandedChildrenCount = m_Children.Length;
            }

            AddToChildrenCount(m_Children.Length);
            NotifyPropertyChanged("HasChildren");
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

            return folderItems.ToArray();
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
