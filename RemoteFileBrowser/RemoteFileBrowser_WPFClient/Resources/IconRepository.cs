using RemoteFileBrowser.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace RemoteFileBrowser.Resources
{
    static class IconRepository
    {
        private static Dictionary<string, ImageSource> s_IconStorage = new Dictionary<string, ImageSource>();

        static IconRepository()
        {
            s_IconStorage.Add("Loading...", null);
        }

        public static ImageSource GetIcon(string path)
        {
            ImageSource result = null;

            if (s_IconStorage.TryGetValue(path, out result))
                return result;
            
            NativeFunctions.IconInfo iconInfo;

            unsafe
            {
                fixed (char* pathChars = path)
                {
                    NativeFunctions.GetIcon(pathChars, out iconInfo);
                }
            }

            if (iconInfo.icon != IntPtr.Zero)
            {
                result = Imaging.CreateBitmapSourceFromHIcon(iconInfo.icon, new Int32Rect(0, 0, iconInfo.width, iconInfo.height), BitmapSizeOptions.FromEmptyOptions());
                NativeFunctions.DestroyIcon(iconInfo.icon);
            }
            
            s_IconStorage.Add(path, result);
            return result;
 	    }
    }
}
