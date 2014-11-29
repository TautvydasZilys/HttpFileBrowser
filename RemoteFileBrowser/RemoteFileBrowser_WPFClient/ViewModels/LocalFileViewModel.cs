using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class LocalFileViewModel : FolderItem
    {
        private bool m_IsSelected;

        public bool IsSelected
        {
            get { return m_IsSelected; }
            set
            {
                m_IsSelected = value;
                NotifyPropertyChanged();
            }
        }

        public LocalFileViewModel(string path) :
            base(path)
        {
        }
    }
}
