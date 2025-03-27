using System.Runtime.InteropServices;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace nvidia_FE_lighting
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            if (!NvApiWrapper.InitializeNvApi())
            {
                MessageBox.Show("Failed to initialize NVAPI");
                return;
            }

            var gpuCount = NvApiWrapper.GetNumberOfGPUs();
            for (uint i = 0; i < gpuCount; i++)
            {
                string gpuName = Marshal.PtrToStringAnsi(NvApiWrapper.GetGPUName(i));
                gpuSelectComboBox.Items.Add($"{i}: {gpuName}");
            }

            if (gpuSelectComboBox.Items.Count > 0)
                gpuSelectComboBox.SelectedIndex = 0;
        }

        // Event handler for GPU selection change
        private void RefreshGpuInfo(uint index)
        {
            gpuNameText.Text = "Name: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetGPUName(index));
            gpuInfoText.Text = "Details: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetGPUInfo(index));
            gpuSystemTypeText.Text = "System Type: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetSystemType(index));
            gpuDriverVersionText.Text = "Driver Version: " + NvApiWrapper.GetDriverVersion() / 100.00f;
            gpuInterfaceVersionText.Text = "NVAPI Interface: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetInterfaceVersionString());
        }

        // Event handler for the "Get Illumination Zones" button
        private void PopulateIlluminationZones(uint gpuIndex)
        {
            gpuIlluminationZones.Items.Clear();

            NvApiWrapper.CustomIlluminationZonesInfo zoneInfo = new NvApiWrapper.CustomIlluminationZonesInfo
            {
                zones = new NvApiWrapper.CustomIlluminationZonesInfoData[32]
            };
            NvApiWrapper.GetIlluminationZonesInfo(gpuIndex, ref zoneInfo);

            NvApiWrapper.CustomIlluminationZoneControls zoneControls = new NvApiWrapper.CustomIlluminationZoneControls
            {
                zones = new NvApiWrapper.CustomIlluminationZoneControl[32]
            };
            NvApiWrapper.GetIlluminationZonesControl(gpuIndex, false, ref zoneControls);

            if (zoneInfo.numIllumZones == 0)
            {
                gpuIlluminationZones.Items.Add(new TextBlock { Text = "No illumination zones found.", Margin = new Thickness(35, 0, 0, 10) });
                return;
            }

            for (int i = 0; i < zoneControls.numZones; i++)
            {
                var zone = zoneControls.zones[i];
                var zoneMeta = zoneInfo.zones[i];

                StackPanel panel = new StackPanel
                {
                    Orientation = Orientation.Vertical,
                    Margin = new Thickness(0, 0, 0, 10)
                };

                TextBlock header = new TextBlock
                {
                    Text = $"Zone {i}: {zoneMeta.zoneType} @ {zoneMeta.zoneLocation}",
                    FontWeight = FontWeights.Bold
                };
                panel.Children.Add(header);

                if (zone.controlMode == "Manual")
                {
                    if (zone.zoneType == "RGB")
                    {
                        panel.Children.Add(new TextBlock { Text = $"RGB: R={zone.manualColorData.rgb.r}, G={zone.manualColorData.rgb.g}, B={zone.manualColorData.rgb.b}, Brightness={zone.manualColorData.rgb.brightness}" });
                    }
                    else if (zone.zoneType == "RGBW")
                    {
                        panel.Children.Add(new TextBlock { Text = $"RGBW: R={zone.manualColorData.rgbw.r}, G={zone.manualColorData.rgbw.g}, B={zone.manualColorData.rgbw.b}, W={zone.manualColorData.rgbw.w}, Brightness={zone.manualColorData.rgbw.brightness}" });
                    }
                    else if (zone.zoneType == "Single Color")
                    {
                        panel.Children.Add(new TextBlock { Text = $"Brightness: {zone.manualColorData.singleColor.brightness}" });
                    }
                }
                else if (zone.controlMode == "Piecewise Linear")
                {
                    panel.Children.Add(new TextBlock { Text = $"Piecewise Mode: {zone.piecewiseData.cycleType}, Group Count: {zone.piecewiseData.grpCount}" });
                    for (int j = 0; j < zone.piecewiseColorData.Length; j++)
                    {
                        var color = zone.piecewiseColorData[j];
                        string colorText = zone.zoneType switch
                        {
                            "RGB" => $"  [{j}] R={color.rgb.r}, G={color.rgb.g}, B={color.rgb.b}, Bright={color.rgb.brightness}",
                            "RGBW" => $"  [{j}] R={color.rgbw.r}, G={color.rgbw.g}, B={color.rgbw.b}, W={color.rgbw.w}, Bright={color.rgbw.brightness}",
                            "Single Color" => $"  [{j}] Brightness={color.singleColor.brightness}",
                            _ => $"  [{j}] Unknown"
                        };
                        panel.Children.Add(new TextBlock { Text = colorText });
                    }
                }

                gpuIlluminationZones.Items.Add(panel);
            }
        }

        private void gpuSelectComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (gpuSelectComboBox.SelectedIndex >= 0)
            {
                uint index = (uint)gpuSelectComboBox.SelectedIndex;
                RefreshGpuInfo(index);
            }
        }

        private void gpuSelectDetectIllumZones_Click(object sender, RoutedEventArgs e)
        {
            if (gpuSelectComboBox.SelectedIndex >= 0)
            {
                uint index = (uint)gpuSelectComboBox.SelectedIndex;
                PopulateIlluminationZones(index);
            }
        }
    }
}