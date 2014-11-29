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
        
        public IEnumerable<FolderItem> FolderItems { get { return m_FolderItems; } }

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

        public LocalFolderViewModel(string name) :
            base(name)
        {
            m_IsSelected = false;
        }
    }
}
