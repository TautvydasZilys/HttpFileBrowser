using RemoteFileBrowser.Interop;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
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
        private ObservableCollection<LocalFileViewModel> m_SharedFiles;

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

        public IEnumerable<LocalFileViewModel> SharedFiles
        {
            get { return m_SharedFiles; }
        }

        internal static ObservableCollection<LocalFileViewModel> SharedFilesCollection
        {
            get { return s_Instance.m_SharedFiles; }
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
            m_SharedFiles = new ObservableCollection<LocalFileViewModel>() { LocalFileViewModel.s_ChildrenLoading[0] };
            m_SharedFiles = await Task.Run(() => LoadVolumes());
            NotifyPropertyChanged("SharedFiles");
        }

        private static unsafe ObservableCollection<LocalFileViewModel> LoadVolumes()
        {
            var results = new ObservableCollection<LocalFileViewModel>();
            char** volumes;
            int volumeCount;

            NativeFunctions.GetVolumes(out volumes, out volumeCount);

            for (int i = 0; i < volumeCount; i++)
            {
                results.Add(new LocalFileViewModel(new string(volumes[i]), true, null));
            }

            NativeFunctions.FreeVolumes(volumes, volumeCount);

            return results;
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
            await Task.Delay(2000);
        }

        private async Task StopSharingImpl()
        {
            await Task.Delay(2000);
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
