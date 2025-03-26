using System;
using System.Runtime.InteropServices;

namespace nvidia_FE_lighting
{
    public static class NvApiWrapper
    {
        private const string DllName = "NvApiWrapper.dll";

        // Structs in the DLL
        [StructLayout(LayoutKind.Sequential)]
        public struct CustomRGB
        {
            public byte r, g, b, brightness;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CustomRGBW
        {
            public byte r, g, b, w, brightness;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CustomSingleColor
        {
            public byte brightness;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct CustomPiecewiseLinear
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string cycleType;
            public ushort riseTimeMs, fallTimeMs;
            public ushort aTimeMs, bTimeMs;
            public ushort idleTimeMs, phaseOffsetMs;
            public byte grpCount;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct ManualColorData
        {
            [FieldOffset(0)] public CustomRGB rgb;
            [FieldOffset(0)] public CustomRGBW rgbw;
            [FieldOffset(0)] public CustomSingleColor singleColor;
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct PiecewiseColorData
        {
            [FieldOffset(0)] public CustomRGB rgb;
            [FieldOffset(0)] public CustomRGBW rgbw;
            [FieldOffset(0)] public CustomSingleColor singleColor;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct CustomIlluminationZoneControl
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneType;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 24)]
            public string controlMode;

            public ManualColorData manualColorData;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
            public PiecewiseColorData[] piecewiseColorData;

            public bool isPiecewise;

            public CustomPiecewiseLinear piecewiseData;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CustomIlluminationZoneControls
        {
            public uint numZones;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
            public CustomIlluminationZoneControl[] zones;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct CustomIlluminationZonesInfoData
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneType;

            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
            public string zoneLocation;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CustomIlluminationZonesInfo
        {
            public uint numIllumZones;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
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
