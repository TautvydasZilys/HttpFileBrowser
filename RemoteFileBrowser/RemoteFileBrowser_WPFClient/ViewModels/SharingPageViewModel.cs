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
        private string m_PublicName;
        private string m_HostId;
        private bool m_AllowDirectConnections;
        private bool m_EnableMulticast;
        private bool m_RequireAuthentification = true;
        private readonly ObservableCollection<LocalFileViewModel> m_SharedFiles;

        #region Properties

        public bool IsSharing
        {
            get { return m_IsSharing; }
            set
            {
                m_IsSharing = value;
                NotifyPropertyChanged();
                NotifyPropertyChanged("SharingStatusString");
            }
        }

        public string SharingStatusString { get { return IsSharing ? "Running" : "Stopped"; } }
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
            m_SharedFiles = new ObservableCollection<LocalFileViewModel>
            {
                new LocalFileViewModel("C:\\", true, null),
                new LocalFileViewModel("D:\\", true, null),
                new LocalFileViewModel("E:\\", true, null),
                new LocalFileViewModel("F:\\", true, null),
                new LocalFileViewModel("G:\\", true, null)
            };
        }
        
        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
