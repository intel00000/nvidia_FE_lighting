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