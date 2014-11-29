using RemoteFileBrowser.ViewModels;
using System;
using System.Windows;
using System.Windows.Controls;

namespace RemoteFileBrowser.Pages
{
    public partial class SharingPage : Page
    {
        SharingPageViewModel m_ViewModel;

        public SharingPage()
        {
            InitializeComponent();
            DataContext = m_ViewModel = new SharingPageViewModel();
        }

        private void StartSharingButton_Click(object sender, RoutedEventArgs e)
        {
            if (m_ViewModel.IsSharing)
                throw new InvalidOperationException();

            m_ViewModel.IsSharing = true;
        }

        private void StopSharingButton_Click(object sender, RoutedEventArgs e)
        {
            if (!m_ViewModel.IsSharing)
                throw new InvalidOperationException();

            m_ViewModel.IsSharing = false;
        }
    }
}
