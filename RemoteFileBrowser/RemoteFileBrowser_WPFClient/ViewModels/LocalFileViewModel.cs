using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteFileBrowser.ViewModels
{
    class LocalFileViewModel : INotifyPropertyChanged
    {
        private string m_Path;
        private string m_Name;
        private bool? m_IsSelected;
        private LocalFolderViewModel m_Parent;

        #region Properties

        public string Path { get { return m_Path; } }

        public string Name { get { return m_Name; } }

        public object Image { get { return null; } }

        protected LocalFolderViewModel Parent { get { return m_Parent; } }

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

        #endregion

        public LocalFileViewModel(string path, LocalFolderViewModel parent)
        {
            m_Path = path;
            m_Name = System.IO.Path.GetFileName(path);

            if (string.IsNullOrEmpty(m_Name))
                m_Name = m_Path;

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

        protected virtual void OnSelectedChanged()
        {
        }

        internal void SetSelectionNoValidate(bool? isSelected)
        {
            m_IsSelected = isSelected;
            NotifyPropertyChanged("IsSelected");
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void NotifyPropertyChanged([CallerMemberName]string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
