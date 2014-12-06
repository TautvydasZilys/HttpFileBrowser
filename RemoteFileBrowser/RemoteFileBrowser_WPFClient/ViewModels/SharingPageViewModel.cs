using RemoteFileBrowser.Interop;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class SharingPageViewModel : INotifyPropertyChanged
    {
        private static SharingPageViewModel s_Instance;

        private bool m_IsSharing;
        private bool m_CanChangeShareStatus = true;
        private string m_PublicName;
        private string m_HostId;
        private bool m_AllowDirectConnections;
        private bool m_EnableMulticast;
        private bool m_RequireAuthentification = true;
        private ObservableCollection<LocalFileViewModel> m_FileTree;
        private LocalFileViewModel[] m_RootFiles;
        private IntPtr m_SharingContext;

        #region Properties

        public bool IsSharing
        {
            get { return m_IsSharing; }
            private set
            {
                m_IsSharing = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("SharingStatusString");
            }
        }

        public bool CanChangeShareStatus
        {
            get { return m_CanChangeShareStatus; }
            private set
            {
                m_CanChangeShareStatus = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("SharingStatusString");
            }
        }

        public string SharingStatusString
        { 
            get
            {
                if (CanChangeShareStatus)
                {
                    return IsSharing ? "Running" : "Stopped"; 
                }

                return IsSharing ? "Stopping" : "Starting";
            }
        }

        public string HostId { get { return m_HostId; } }

        public string PublicName
        {
            get { return m_PublicName; }
            set
            {
                m_PublicName = value;
                NotifyPropertyChanged();
            }
        }

        public bool AllowDirectConnections
        {
            get { return m_AllowDirectConnections; }
            set
            {
                m_AllowDirectConnections = value;
                NotifyPropertyChanged();
            }
        }

        public bool EnableMulticast
        {
            get { return m_EnableMulticast; }
            set
            {
                m_EnableMulticast = value;
                NotifyPropertyChanged();
            }
        }

        public bool RequireAuthentification
        {
            get { return m_RequireAuthentification; }
            set
            {
                m_RequireAuthentification = value;
                NotifyPropertyChanged();
            }
        }

        public IEnumerable<LocalFileViewModel> FileTree
        {
            get { return m_FileTree; }
        }

        internal static ObservableCollection<LocalFileViewModel> FileTreeCollection
        {
            get { return s_Instance.m_FileTree; }
        }

        #endregion

        public SharingPageViewModel()
        {
            s_Instance = this;
            LoadVolumesAsync();

            IntPtr uniqueSystemIdPtr;
            int length;
            NativeFunctions.GetUniqueSystemId(out uniqueSystemIdPtr, out length);
            m_HostId = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(uniqueSystemIdPtr, length);
        }

        private async void LoadVolumesAsync()
        {
            m_FileTree = new ObservableCollection<LocalFileViewModel>(LocalFileViewModel.s_ChildrenLoading);
            m_RootFiles = await Task.Run(() => LoadVolumes());
            m_FileTree = new ObservableCollection<LocalFileViewModel>(m_RootFiles);
            NotifyPropertyChanged("FileTree");
        }

        private static unsafe LocalFileViewModel[] LoadVolumes()
        {
            char** volumes;
            int volumeCount;

            NativeFunctions.GetVolumes(out volumes, out volumeCount);
            var results = new LocalFileViewModel[volumeCount];

            for (int i = 0; i < volumeCount; i++)
            {
                results[i] = new LocalFileViewModel(new string(volumes[i]), true, null);
            }

            NativeFunctions.FreeVolumes(volumes, volumeCount);

            return results;
        }

        public static void NotifySharedFileChanged()
        {
            if (!s_Instance.IsSharing)
                return;

            s_Instance.HandleSharedFilesChanged();
        }

        private async void HandleSharedFilesChanged()
        {
            List<string> fullySharedFolders, partiallySharedFolders, files;
            CollectSharedFiles(out fullySharedFolders, out partiallySharedFolders, out files);

            await Task.Run(() =>
            {
                DoActionWithMarshalledFiles(fullySharedFolders, partiallySharedFolders, files, (sharedFiles) =>
                {
                    NativeFunctions.SetSharedFiles(ref sharedFiles);
                });
            });
        }

        public async Task StartSharing()
        {
            if (IsSharing || !CanChangeShareStatus)
                throw new InvalidOperationException();

            CanChangeShareStatus = false;

            await StartSharingImpl();

            CanChangeShareStatus = true;
            IsSharing = true;
        }

        public async Task StopSharing()
        {
            if (!IsSharing || !CanChangeShareStatus)
                throw new InvalidOperationException();

            CanChangeShareStatus = false;

            await StopSharingImpl();

            CanChangeShareStatus = true;
            IsSharing = false;
        }

        private async Task StartSharingImpl()
        {
            List<string> fullySharedFolders, partiallySharedFolders, files;
            CollectSharedFiles(out fullySharedFolders, out partiallySharedFolders, out files);

            await Task.Run(() =>
            {
                DoActionWithMarshalledFiles(fullySharedFolders, partiallySharedFolders, files, (sharedFiles) =>
                {
                    m_SharingContext = NativeFunctions.StartSharingFiles(ref sharedFiles);   
                });
            });
        }

        private async Task StopSharingImpl()
        {
            await Task.Run(() =>
            {
                NativeFunctions.StopSharingFiles(ref m_SharingContext);
            });
        }

        private unsafe void DoActionWithMarshalledFiles(List<string> fullySharedFolders, List<string> partiallySharedFolders, List<string> files, Action<NativeFunctions.SharedFiles> callback)
        {
            var sharedFiles = default(NativeFunctions.SharedFiles);

            sharedFiles.fullySharedFolders = MarshalStringList(fullySharedFolders);
            sharedFiles.fullySharedFolderCount = fullySharedFolders.Count;

            sharedFiles.partiallySharedFolders = MarshalStringList(partiallySharedFolders);
            sharedFiles.partiallySharedFolderCount = partiallySharedFolders.Count;

            sharedFiles.sharedFiles = MarshalStringList(files);
            sharedFiles.sharedFileCount = files.Count;

            callback(sharedFiles);

            MarshalStringListCleanup(sharedFiles.fullySharedFolders, sharedFiles.fullySharedFolderCount);
            MarshalStringListCleanup(sharedFiles.partiallySharedFolders, sharedFiles.partiallySharedFolderCount);
            MarshalStringListCleanup(sharedFiles.sharedFiles, sharedFiles.sharedFileCount);
        }

        private unsafe char** MarshalStringList(List<string> strings)
        {
            var result = (char**)Marshal.AllocHGlobal(sizeof(char*) * strings.Count);

            for (int i = 0; i < strings.Count; i++)
            {
                result[i] = (char*)Marshal.AllocHGlobal(sizeof(char) * (strings[i].Length + 1));
                
                fixed (char* source = strings[i])
                {
                    for (int j = 0; j < strings[i].Length; j++)
                    {
                        result[i][j] = source[j];
                    }
                }

                result[i][strings[i].Length] = '\0';
            }

            return result;
        }

        private unsafe void MarshalStringListCleanup(char** ptr, int count)
        {
            for (int i = 0; i < count; i++)
            {
                Marshal.FreeHGlobal((IntPtr)ptr[i]);
            }

            Marshal.FreeHGlobal((IntPtr)ptr);
        }

        private void CollectSharedFiles(out List<string> fullySharedFolders, out List<string> partiallySharedFolders, out List<string> sharedFiles)
        {
            fullySharedFolders = new List<string>();
            partiallySharedFolders = new List<string>();
            sharedFiles = new List<string>();

            foreach (var volume in m_RootFiles)
            {
                volume.CollectSharedFiles(fullySharedFolders, partiallySharedFolders, sharedFiles);
            }
        }

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        #endregion
    }
}
