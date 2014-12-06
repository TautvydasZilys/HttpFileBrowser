using RemoteFileBrowser.Interop;
using System;
using System.Windows;

namespace RemoteFileBrowser
{
    public partial class App : Application
    {
        public App()
        {
            try
            {
                NativeFunctions.EmptyEntry();
            }
            catch (DllNotFoundException)
            {
                const string kRedistURL = @"http://www.microsoft.com/en-us/download/details.aspx?id=40784";
                var result = MessageBox.Show(
                    "Failed to load RemoteFileBrowser.dll. Please make sure that the Visual C++ 2013 Redistributable Package is installed. " +
                    "It may be download from " + kRedistURL + ". Do you want to copy this URL to clipboard?",
                    "Fatal error", MessageBoxButton.YesNo, MessageBoxImage.Error);

                if (result == MessageBoxResult.Yes)
                    Clipboard.SetText(kRedistURL);

                Application.Current.Shutdown();
            }
        }
    }
}
