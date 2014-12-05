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

        private async void StartSharingButton_Click(object sender, RoutedEventArgs e)
        {
            await m_ViewModel.StartSharing();
        }

        private async void StopSharingButton_Click(object sender, RoutedEventArgs e)
        {
            await m_ViewModel.StopSharing();
        }
    }
}
