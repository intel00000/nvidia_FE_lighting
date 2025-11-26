using System.Runtime.InteropServices;

namespace nvidia_FE_lighting
{
    public static class NvApiWrapper
    {
        private const string DllName = "NvApiWrapper.dll";

        // Structs in the DLL
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct CustomRGB
        {
            public byte r, g, b, brightness;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct CustomRGBW
        {
            public byte r, g, b, w, brightness;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct CustomSingleColor
        {
            public byte brightness;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 2)]
        public struct CustomPiecewiseLinear
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string cycleType;
            public ushort riseTimeMs, fallTimeMs;
            public ushort aTimeMs, bTimeMs;
            public ushort idleTimeMs, phaseOffsetMs;
            public byte grpCount;
            public byte padding;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct ColorData
        {
            public CustomRGB rgb;
            public CustomRGBW rgbw;
            public CustomSingleColor singleColor;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 2)]
        public struct CustomIlluminationZoneControl
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneType;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 24)]
            public string controlMode;

            public ColorData manualColorData;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
            public ColorData[] piecewiseColorData;

            public CustomPiecewiseLinear piecewiseData;

            [MarshalAs(UnmanagedType.U1)]
            public bool isPiecewise;

            public byte padding;
        }
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct CustomIlluminationZoneControls
        {
            public uint numZones;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
            public CustomIlluminationZoneControl[] zones;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
        public struct CustomIlluminationZonesInfoData
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneType;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneLocation;
        }
        [StructLayout(LayoutKind.Sequential, Pack = 4)]
        public struct CustomIlluminationZonesInfo
        {
            public uint numIllumZones;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
            public CustomIlluminationZonesInfoData[] zones;
        }

        // Function imports
        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern bool InitializeNvApi();

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern bool DeinitializeNvApi();

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetInterfaceVersionString();

        [DllImport(DllName)]
        public static extern uint GetDriverVersion();

        [DllImport(DllName)]
        public static extern uint GetNumberOfGPUs();

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetGPUName(uint index);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetGPUInfo(uint index);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetSystemType(uint index);

        [DllImport(DllName)]
        public static extern bool GetGPUPCIIdentifiers(uint index, out uint deviceId, out uint subSystemId, out uint revisionId, out uint extDeviceId);

        [DllImport(DllName)]
        public static extern bool GetGPUBusId(uint index, out uint busId);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetIlluminationZonesInfo(uint index, ref CustomIlluminationZonesInfo info);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern IntPtr GetIlluminationZonesControl(uint index, bool useDefault, ref CustomIlluminationZoneControls controls);

        [DllImport(DllName)]
        public static extern bool SetIlluminationZoneManualRGB(uint gpuIndex, uint zoneIndex, byte red, byte green, byte blue, byte brightness, bool Default);

        [DllImport(DllName)]
        public static extern bool SetIlluminationZoneManualRGBW(uint gpuIndex, uint zoneIndex, byte red, byte green, byte blue, byte white, byte brightness, bool Default);

        [DllImport(DllName)]
        public static extern bool SetIlluminationZoneManualSingleColor(uint gpuIndex, uint zoneIndex, byte brightness, bool Default);
    }
}
