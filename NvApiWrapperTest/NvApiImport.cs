using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace NvApiWrapperTest
{
    public static class NvApiImport
    {
        // imports from NvApiWrapper.dll
        [DllImport("NvApiWrapper.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool Testing();
    }
}