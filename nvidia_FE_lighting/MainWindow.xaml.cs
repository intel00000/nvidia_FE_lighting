using Microsoft.Win32;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using static nvidia_FE_lighting.NvApiWrapper;

namespace nvidia_FE_lighting
{
    public class LightingProfile
    {
        public uint GpuIndex { get; set; }
        public List<ZoneProfile> Zones { get; set; } = new();
    }

    public class ZoneProfile
    {
        public int ZoneIndex { get; set; }
        public string ZoneType { get; set; }
        public byte R { get; set; }
        public byte G { get; set; }
        public byte B { get; set; }
        public byte W { get; set; }
        public byte Brightness { get; set; }
    }

    public class GpuIdentifier
    {
        public uint BusId { get; set; }
        public uint DeviceId { get; set; }
        public uint SubSystemId { get; set; }
        public uint RevisionId { get; set; }
        public uint ExtDeviceId { get; set; }

        public bool Matches(GpuIdentifier other)
        {
            return BusId == other.BusId &&
                   DeviceId == other.DeviceId &&
                   SubSystemId == other.SubSystemId;
        }
    }

    public class StartupSettings
    {
        public int DelaySeconds { get; set; } = 10;
        public uint GpuIndex { get; set; }
        public GpuIdentifier GpuIdentifier { get; set; } = new();
        public List<ZoneProfile> Zones { get; set; } = new();
    }

    public partial class MainWindow : Window
    {
        private CustomIlluminationZoneControl[] globalZoneControls;
        private uint currentGpuIndex;
        private readonly Dictionary<(uint gpuIdx, int zoneIdx), byte> pendingBrightnessChanges = new();
        private readonly string profilesFolder;
        private readonly string startupSettingsPath;
        private readonly string appDataFolder;
        private readonly string appDataExePath;
        private bool isInitializing = true;
        private void SetStatus(string message)
        {
            statusText.Text = message;
        }

        public MainWindow()
        {
            InitializeComponent();

            appDataFolder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "NvidiaFELighting");
            profilesFolder = Path.Combine(appDataFolder, "Profiles");
            startupSettingsPath = Path.Combine(appDataFolder, "startup_settings.json");
            appDataExePath = Path.Combine(appDataFolder, "FELighting.exe");
            Directory.CreateDirectory(profilesFolder);

            if (!InitializeNvApi())
            {
                Xceed.Wpf.Toolkit.MessageBox.Show("Failed to initialize NVAPI");
                SetStatus("NVAPI initialization failed.");
                return;
            }
            SetStatus("NVAPI initialized.");

            var gpuCount = GetNumberOfGPUs();
            for (uint i = 0; i < gpuCount; i++)
            {
                string gpuName = Marshal.PtrToStringAnsi(GetGPUName(i));
                gpuSelectComboBox.Items.Add($"{i}: {gpuName}");
            }

            if (gpuSelectComboBox.Items.Count > 0)
            {
                gpuSelectComboBox.SelectedIndex = 0;
                gpuSelectComboBox.SelectedItem = gpuSelectComboBox.Items[0];
                gpuSelectComboBox.Text = gpuSelectComboBox.Items[0].ToString();
                SetStatus($"Detected {gpuCount} GPU(s).");
            }
            else
            {
                SetStatus("No GPUs detected.");
            }

            // Set checkbox state based on registry
            applyOnStartupCheckBox.IsChecked = IsStartupEnabled();

            // Initialization complete - enable event handlers
            isInitializing = false;
        }

        // event handler for GPU selection
        private void RefreshGpuInfo(uint index)
        {
            currentGpuIndex = index;
            gpuNameText.Text = "Name: " + Marshal.PtrToStringAnsi(GetGPUName(index));
            gpuInfoText.Text = "Details: " + Marshal.PtrToStringAnsi(GetGPUInfo(index));
            gpuSystemTypeText.Text = "System Type: " + Marshal.PtrToStringAnsi(GetSystemType(index));
            gpuDriverVersionText.Text = "Driver Version: " + GetDriverVersion() / 100.00f;
            gpuInterfaceVersionText.Text = "NVAPI Interface: " + Marshal.PtrToStringAnsi(GetInterfaceVersionString());
            SetStatus($"Loaded GPU {index}");
        }

        // handler for "Get Illumination Zones" button
        private void PopulateIlluminationZones(uint gpuIndex)
        {
            SetStatus("Detecting illumination zones...");
            gpuIlluminationZones.Items.Clear();
            pendingBrightnessChanges.Clear();
            applyAllButton.IsEnabled = false;

            var zoneInfo = new NvApiWrapper.CustomIlluminationZonesInfo { zones = new NvApiWrapper.CustomIlluminationZonesInfoData[32] };
            NvApiWrapper.GetIlluminationZonesInfo(gpuIndex, ref zoneInfo);

            var zoneControls = new NvApiWrapper.CustomIlluminationZoneControls { zones = new NvApiWrapper.CustomIlluminationZoneControl[32] };
            NvApiWrapper.GetIlluminationZonesControl(gpuIndex, false, ref zoneControls);

            globalZoneControls = new CustomIlluminationZoneControl[zoneControls.numZones];
            Array.Copy(zoneControls.zones, globalZoneControls, zoneControls.numZones);

            if (zoneInfo.numIllumZones == 0)
            {
                gpuIlluminationZones.Items.Add(new TextBlock { Text = "No illumination zones found.", Margin = new Thickness(35, 0, 0, 10) });
                SetStatus("No illumination zones found.");
                return;
            }

            for (int i = 0; i < zoneControls.numZones; i++)
            {
                var zone = zoneControls.zones[i];
                var zoneMeta = zoneInfo.zones[i];

                var panel = new StackPanel
                {
                    Orientation = Orientation.Vertical
                };

                TextBlock header = new TextBlock
                {
                    Text = $"Zone {i}: {zone.controlMode} for {zoneMeta.zoneType} @ {zoneMeta.zoneLocation}",
                    FontWeight = FontWeights.Bold,
                    Margin = new Thickness(0, 0, 0, 10)
                };
                panel.Children.Add(header);

                if (zone.controlMode == "Manual")
                {
                    if (zone.zoneType == "RGB")
                    {
                        panel.Children.Add(new TextBlock { Text = $"Active RGB: R={zone.manualColorData.rgb.r}, G={zone.manualColorData.rgb.g}, B={zone.manualColorData.rgb.b}", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
                    }
                    else if (zone.zoneType == "RGBW")
                    {
                        panel.Children.Add(new TextBlock { Text = $"Active RGBW: R={zone.manualColorData.rgbw.r}, G={zone.manualColorData.rgbw.g}, B={zone.manualColorData.rgbw.b}, W={zone.manualColorData.rgbw.w}", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
                    }
                    else if (zone.zoneType == "Invalid")
                    {
                        panel.Children.Add(new TextBlock { Text = "Invalid", Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
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
                            "Active: RGB" => $"  [{j}] R={color.rgb.r}, G={color.rgb.g}, B={color.rgb.b}, Bright={color.rgb.brightness}",
                            "Active: RGBW" => $"  [{j}] R={color.rgbw.r}, G={color.rgbw.g}, B={color.rgbw.b}, W={color.rgbw.w}, Bright={color.rgbw.brightness}",
                            "Active: Single Color" => $"  [{j}] Brightness={color.singleColor.brightness}",
                            _ => $"  [{j}] Unknown"
                        };
                        panel.Children.Add(new TextBlock { Text = colorText, Margin = new Thickness(0, 0, 0, 10), Foreground = (Brush)FindResource("TextSecondaryBrush") });
                    }
                }

                AddZoneControls(panel, zoneMeta.zoneType, gpuIndex, i);
                var card = new Border
                {
                    Background = (Brush)FindResource("CardBackgroundBrush"),
                    BorderBrush = (Brush)FindResource("CardBorderBrush"),
                    BorderThickness = new Thickness(1),
                    CornerRadius = new CornerRadius(10),
                    Padding = new Thickness(12),
                    Margin = new Thickness(0, 0, 0, 12)
                };
                card.Child = panel;
                gpuIlluminationZones.Items.Add(card);
            }
            SetStatus($"Found {zoneControls.numZones} illumination zone(s).");
        }
        private void AddZoneControls(StackPanel panel, string zoneType, uint gpuIndex, int zoneIndex)
        {
            if (zoneType == "Invalid")
            {
                return;
            }

            // Add RGB/RGBW color picker if applicable
            if (zoneType == "RGB")
            {
                var rgb = globalZoneControls[zoneIndex].manualColorData.rgb;
                var colorPickerPanel = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 10) };

                var colorLabel = new TextBlock { Text = "Color: ", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 10, 0) };
                colorPickerPanel.Children.Add(colorLabel);

                var colorPicker = new Xceed.Wpf.Toolkit.ColorPicker
                {
                    Width = 60,
                    SelectedColor = System.Windows.Media.Color.FromRgb(rgb.r, rgb.g, rgb.b),
                    ShowStandardColors = false,
                    ShowRecentColors = false,
                    ShowDropDownButton = true,
                    DisplayColorAndName = false,
                    VerticalAlignment = VerticalAlignment.Center
                };
                colorPicker.Tag = (gpuIndex, zoneIndex, zoneType);
                colorPicker.SelectedColorChanged += ColorPicker_SelectedColorChanged;
                colorPickerPanel.Children.Add(colorPicker);

                panel.Children.Add(colorPickerPanel);
            }
            else if (zoneType == "RGBW")
            {
                var rgbw = globalZoneControls[zoneIndex].manualColorData.rgbw;
                var colorPickerPanel = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 10) };

                var colorLabel = new TextBlock { Text = "Color: ", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 10, 0) };
                colorPickerPanel.Children.Add(colorLabel);

                var colorPicker = new Xceed.Wpf.Toolkit.ColorPicker
                {
                    Width = 60,
                    SelectedColor = System.Windows.Media.Color.FromRgb(rgbw.r, rgbw.g, rgbw.b),
                    ShowStandardColors = false,
                    ShowRecentColors = false,
                    ShowDropDownButton = true,
                    DisplayColorAndName = false,
                    VerticalAlignment = VerticalAlignment.Center
                };
                colorPicker.Tag = (gpuIndex, zoneIndex, zoneType);
                colorPicker.SelectedColorChanged += ColorPicker_SelectedColorChanged;
                colorPickerPanel.Children.Add(colorPicker);

                // Add white slider for RGBW
                var whiteLabel = new TextBlock { Text = "  White: ", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(10, 0, 10, 0) };
                colorPickerPanel.Children.Add(whiteLabel);

                var whiteSlider = new Slider
                {
                    Minimum = 0,
                    Maximum = 255,
                    Value = rgbw.w,
                    Width = 200,
                    TickFrequency = 1,
                    IsSnapToTickEnabled = true,
                    IsMoveToPointEnabled = true,
                    VerticalAlignment = VerticalAlignment.Center
                };
                whiteSlider.Tag = (gpuIndex, zoneIndex, zoneType);
                whiteSlider.AddHandler(Thumb.DragCompletedEvent, new DragCompletedEventHandler(WhiteSlider_DragCompleted));
                whiteSlider.MouseLeftButtonUp += WhiteSlider_MouseLeftButtonUp;
                colorPickerPanel.Children.Add(whiteSlider);

                panel.Children.Add(colorPickerPanel);
            }

            // Add brightness slider
            var brightnessPanel = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 10) };
            var brightnessLabel = new TextBlock { Text = "Brightness: ", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 10, 0) };
            brightnessPanel.Children.Add(brightnessLabel);

            byte brightnessValue = zoneType switch
            {
                "RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.brightness,
                "RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.brightness,
                _ => globalZoneControls[zoneIndex].manualColorData.singleColor.brightness
            };
            var brightnessSlider = new Slider
            {
                Minimum = 0,
                Maximum = 100,
                Value = brightnessValue,
                Width = 200,
                TickFrequency = 1,
                IsSnapToTickEnabled = true,
                IsMoveToPointEnabled = true,
                VerticalAlignment = VerticalAlignment.Center
            };
            brightnessSlider.Tag = (gpuIndex, zoneIndex, zoneType);
            brightnessSlider.AddHandler(Thumb.DragCompletedEvent, new DragCompletedEventHandler(BrightnessSlider_DragCompleted));
            brightnessSlider.MouseLeftButtonUp += BrightnessSlider_MouseLeftButtonUp;
            brightnessPanel.Children.Add(brightnessSlider);

            panel.Children.Add(brightnessPanel);
        }

        private void gpuSelectComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (gpuSelectComboBox.SelectedIndex >= 0)
            {
                uint index = (uint)gpuSelectComboBox.SelectedIndex;
                RefreshGpuInfo(index);
                pendingBrightnessChanges.Clear();
                applyAllButton.IsEnabled = false;
            }
            else
            {
                SetStatus("No GPU selected.");
            }
        }

        private void gpuSelectDetectIllumZones_Click(object sender, RoutedEventArgs e)
        {
            if (gpuSelectComboBox.SelectedIndex >= 0)
            {
                uint index = (uint)gpuSelectComboBox.SelectedIndex;
                SetStatus($"Detecting zones on GPU {index}...");
                PopulateIlluminationZones(index);
            }
            else
            {
                SetStatus("Select a GPU to detect zones.");
            }
        }

        private void BrightnessSlider_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            if (sender is Slider slider)
            {
                ApplySliderValue(slider);
            }
        }

        private void BrightnessSlider_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (sender is Slider slider)
            {
                ApplySliderValue(slider);
            }
        }

        private void ApplySliderValue(Slider slider)
        {
            var tag = ((uint gpuIdx, int zoneIdx, string zoneType))slider.Tag;
            byte brightness = (byte)slider.Value;
            QueueBrightnessChange(tag.gpuIdx, tag.zoneIdx, brightness);
            if (!ApplyBrightnessForZone(tag.gpuIdx, tag.zoneIdx, tag.zoneType, brightness))
            {
                Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to set brightness for zone {tag.zoneIdx}");
                return;
            }
            SetStatus($"Set brightness {brightness}% on zone {tag.zoneIdx}");
        }

        private void QueueBrightnessChange(uint gpuIdx, int zoneIdx, byte brightness)
        {
            pendingBrightnessChanges[(gpuIdx, zoneIdx)] = brightness;
            applyAllButton.IsEnabled = true;
        }

        private bool ApplyBrightnessForZone(uint gpuIdx, int zoneIdx, string zoneType, byte brightness)
        {
            bool result = false;
            if (zoneType == "RGB")
            {
                var color = globalZoneControls[zoneIdx].manualColorData.rgb;
                result = SetIlluminationZoneManualRGB(gpuIdx, (uint)zoneIdx, color.r, color.g, color.b, brightness, false);
                globalZoneControls[zoneIdx].manualColorData.rgb.brightness = brightness;
            }
            else if (zoneType == "RGBW")
            {
                var color = globalZoneControls[zoneIdx].manualColorData.rgbw;
                result = SetIlluminationZoneManualRGBW(gpuIdx, (uint)zoneIdx, color.r, color.g, color.b, color.w, brightness, false);
                globalZoneControls[zoneIdx].manualColorData.rgbw.brightness = brightness;
            }
            else if (zoneType == "Single Color")
            {
                result = SetIlluminationZoneManualSingleColor(gpuIdx, (uint)zoneIdx, brightness, false);
                if (result) globalZoneControls[zoneIdx].manualColorData.singleColor.brightness = brightness;
            }
            else if (zoneType == "Color Fixed")
            {
                result = SetIlluminationZoneManualColorFixed(gpuIdx, (uint)zoneIdx, brightness, false);
                if (result) globalZoneControls[zoneIdx].manualColorData.singleColor.brightness = brightness;
            }
            else
            {
                // Unknown zone type, try both single color and color fixed
                result = SetIlluminationZoneManualSingleColor(gpuIdx, (uint)zoneIdx, brightness, false);
                if (!result)
                {
                    result = SetIlluminationZoneManualColorFixed(gpuIdx, (uint)zoneIdx, brightness, false);
                }
                if (result) globalZoneControls[zoneIdx].manualColorData.singleColor.brightness = brightness;
            }
            return result;
        }

        private void applyAllButton_Click(object sender, RoutedEventArgs e)
        {
            if (pendingBrightnessChanges.Count == 0)
            {
                applyAllButton.IsEnabled = false;
                return;
            }

            foreach (var kvp in pendingBrightnessChanges)
            {
                var (gpuIdx, zoneIdx) = kvp.Key;
                byte brightness = kvp.Value;
                string zoneType = globalZoneControls[zoneIdx].zoneType;
                ApplyBrightnessForZone(gpuIdx, zoneIdx, zoneType, brightness);
            }

            pendingBrightnessChanges.Clear();
            applyAllButton.IsEnabled = false;
            SetStatus("Applied brightness to all pending zones.");
        }

        private void ColorPicker_SelectedColorChanged(object sender, RoutedPropertyChangedEventArgs<System.Windows.Media.Color?> e)
        {
            if (sender is Xceed.Wpf.Toolkit.ColorPicker colorPicker && e.NewValue.HasValue)
            {
                var tag = ((uint gpuIdx, int zoneIdx, string zoneType))colorPicker.Tag;
                var color = e.NewValue.Value;

                if (tag.zoneType == "RGB")
                {
                    var currentColor = globalZoneControls[tag.zoneIdx].manualColorData.rgb;
                    if (!SetIlluminationZoneManualRGB(tag.gpuIdx, (uint)tag.zoneIdx, color.R, color.G, color.B, currentColor.brightness, false))
                    {
                        Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to set color for zone {tag.zoneIdx}");
                        return;
                    }
                    globalZoneControls[tag.zoneIdx].manualColorData.rgb.r = color.R;
                    globalZoneControls[tag.zoneIdx].manualColorData.rgb.g = color.G;
                    globalZoneControls[tag.zoneIdx].manualColorData.rgb.b = color.B;
                    SetStatus($"Set RGB color ({color.R}, {color.G}, {color.B}) on zone {tag.zoneIdx}");
                }
                else if (tag.zoneType == "RGBW")
                {
                    var currentColor = globalZoneControls[tag.zoneIdx].manualColorData.rgbw;
                    if (!SetIlluminationZoneManualRGBW(tag.gpuIdx, (uint)tag.zoneIdx, color.R, color.G, color.B, currentColor.w, currentColor.brightness, false))
                    {
                        Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to set color for zone {tag.zoneIdx}");
                        return;
                    }
                    globalZoneControls[tag.zoneIdx].manualColorData.rgbw.r = color.R;
                    globalZoneControls[tag.zoneIdx].manualColorData.rgbw.g = color.G;
                    globalZoneControls[tag.zoneIdx].manualColorData.rgbw.b = color.B;
                    SetStatus($"Set RGBW color ({color.R}, {color.G}, {color.B}) on zone {tag.zoneIdx}");
                }
            }
        }

        private void WhiteSlider_DragCompleted(object sender, DragCompletedEventArgs e)
        {
            if (sender is Slider slider)
            {
                ApplyWhiteValue(slider);
            }
        }

        private void WhiteSlider_MouseLeftButtonUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (sender is Slider slider)
            {
                ApplyWhiteValue(slider);
            }
        }

        private void ApplyWhiteValue(Slider slider)
        {
            var tag = ((uint gpuIdx, int zoneIdx, string zoneType))slider.Tag;
            byte white = (byte)slider.Value;

            if (tag.zoneType == "RGBW")
            {
                var currentColor = globalZoneControls[tag.zoneIdx].manualColorData.rgbw;
                if (!SetIlluminationZoneManualRGBW(tag.gpuIdx, (uint)tag.zoneIdx, currentColor.r, currentColor.g, currentColor.b, white, currentColor.brightness, false))
                {
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to set white level for zone {tag.zoneIdx}");
                    return;
                }
                globalZoneControls[tag.zoneIdx].manualColorData.rgbw.w = white;
                SetStatus($"Set white level {white} on zone {tag.zoneIdx}");
            }
        }

        private void SaveProfile_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.Tag is string profileNumber)
            {
                if (globalZoneControls == null || globalZoneControls.Length == 0)
                {
                    Xceed.Wpf.Toolkit.MessageBox.Show("No zones detected. Please detect zones first.");
                    return;
                }

                var profile = new LightingProfile
                {
                    GpuIndex = currentGpuIndex,
                    Zones = new List<ZoneProfile>()
                };

                for (int i = 0; i < globalZoneControls.Length; i++)
                {
                    var zone = globalZoneControls[i];
                    var zoneProfile = new ZoneProfile
                    {
                        ZoneIndex = i,
                        ZoneType = zone.zoneType
                    };

                    if (zone.zoneType == "RGB")
                    {
                        zoneProfile.R = zone.manualColorData.rgb.r;
                        zoneProfile.G = zone.manualColorData.rgb.g;
                        zoneProfile.B = zone.manualColorData.rgb.b;
                        zoneProfile.Brightness = zone.manualColorData.rgb.brightness;
                    }
                    else if (zone.zoneType == "RGBW")
                    {
                        zoneProfile.R = zone.manualColorData.rgbw.r;
                        zoneProfile.G = zone.manualColorData.rgbw.g;
                        zoneProfile.B = zone.manualColorData.rgbw.b;
                        zoneProfile.W = zone.manualColorData.rgbw.w;
                        zoneProfile.Brightness = zone.manualColorData.rgbw.brightness;
                    }
                    else
                    {
                        zoneProfile.Brightness = zone.manualColorData.singleColor.brightness;
                    }

                    profile.Zones.Add(zoneProfile);
                }

                try
                {
                    string profilePath = Path.Combine(profilesFolder, $"profile_{profileNumber}.json");
                    string json = JsonSerializer.Serialize(profile, new JsonSerializerOptions { WriteIndented = true });
                    File.WriteAllText(profilePath, json);
                    SetStatus($"Saved profile {profileNumber}");
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Profile {profileNumber} saved successfully!");
                }
                catch (Exception ex)
                {
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to save profile: {ex.Message}");
                }
            }
        }

        private void LoadProfile_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.Tag is string profileNumber)
            {
                string profilePath = Path.Combine(profilesFolder, $"profile_{profileNumber}.json");

                if (!File.Exists(profilePath))
                {
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Profile {profileNumber} does not exist.");
                    return;
                }

                try
                {
                    string json = File.ReadAllText(profilePath);
                    var profile = JsonSerializer.Deserialize<LightingProfile>(json);

                    if (profile == null || profile.Zones.Count == 0)
                    {
                        Xceed.Wpf.Toolkit.MessageBox.Show("Invalid profile data.");
                        return;
                    }

                    // Apply profile to GPU
                    foreach (var zoneProfile in profile.Zones)
                    {
                        if (zoneProfile.ZoneIndex >= globalZoneControls.Length)
                            continue;

                        bool success = false;
                        if (zoneProfile.ZoneType == "RGB")
                        {
                            success = SetIlluminationZoneManualRGB(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                                zoneProfile.R, zoneProfile.G, zoneProfile.B, zoneProfile.Brightness, false);
                            if (success)
                            {
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgb.r = zoneProfile.R;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgb.g = zoneProfile.G;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgb.b = zoneProfile.B;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgb.brightness = zoneProfile.Brightness;
                            }
                        }
                        else if (zoneProfile.ZoneType == "RGBW")
                        {
                            success = SetIlluminationZoneManualRGBW(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                                zoneProfile.R, zoneProfile.G, zoneProfile.B, zoneProfile.W, zoneProfile.Brightness, false);
                            if (success)
                            {
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgbw.r = zoneProfile.R;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgbw.g = zoneProfile.G;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgbw.b = zoneProfile.B;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgbw.w = zoneProfile.W;
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.rgbw.brightness = zoneProfile.Brightness;
                            }
                        }
                        else
                        {
                            success = SetIlluminationZoneManualSingleColor(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                                zoneProfile.Brightness, false);
                            if (success)
                            {
                                globalZoneControls[zoneProfile.ZoneIndex].manualColorData.singleColor.brightness = zoneProfile.Brightness;
                            }
                        }
                    }

                    // Refresh the UI by re-detecting zones
                    PopulateIlluminationZones(currentGpuIndex);
                    SetStatus($"Loaded profile {profileNumber}");
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Profile {profileNumber} loaded successfully!");
                }
                catch (Exception ex)
                {
                    Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to load profile: {ex.Message}");
                }
            }
        }

        // Startup settings methods
        private GpuIdentifier GetCurrentGpuIdentifier()
        {
            uint busId, deviceId, subSystemId, revisionId, extDeviceId;

            if (!GetGPUBusId(currentGpuIndex, out busId))
                return null;

            if (!GetGPUPCIIdentifiers(currentGpuIndex, out deviceId, out subSystemId, out revisionId, out extDeviceId))
                return null;

            return new GpuIdentifier
            {
                BusId = busId,
                DeviceId = deviceId,
                SubSystemId = subSystemId,
                RevisionId = revisionId,
                ExtDeviceId = extDeviceId
            };
        }

        private void SaveStartupSettings()
        {
            if (globalZoneControls == null || globalZoneControls.Length == 0)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show("No zones detected. Please detect zones first.");
                return;
            }

            var gpuId = GetCurrentGpuIdentifier();
            if (gpuId == null)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show("Failed to get GPU identifier.");
                return;
            }

            var settings = new StartupSettings
            {
                DelaySeconds = 10,
                GpuIndex = currentGpuIndex,
                GpuIdentifier = gpuId,
                Zones = new List<ZoneProfile>()
            };

            for (int i = 0; i < globalZoneControls.Length; i++)
            {
                var zone = globalZoneControls[i];
                var zoneProfile = new ZoneProfile
                {
                    ZoneIndex = i,
                    ZoneType = zone.zoneType
                };

                if (zone.zoneType == "RGB")
                {
                    zoneProfile.R = zone.manualColorData.rgb.r;
                    zoneProfile.G = zone.manualColorData.rgb.g;
                    zoneProfile.B = zone.manualColorData.rgb.b;
                    zoneProfile.Brightness = zone.manualColorData.rgb.brightness;
                }
                else if (zone.zoneType == "RGBW")
                {
                    zoneProfile.R = zone.manualColorData.rgbw.r;
                    zoneProfile.G = zone.manualColorData.rgbw.g;
                    zoneProfile.B = zone.manualColorData.rgbw.b;
                    zoneProfile.W = zone.manualColorData.rgbw.w;
                    zoneProfile.Brightness = zone.manualColorData.rgbw.brightness;
                }
                else
                {
                    zoneProfile.Brightness = zone.manualColorData.singleColor.brightness;
                }

                settings.Zones.Add(zoneProfile);
            }

            try
            {
                string json = JsonSerializer.Serialize(settings, new JsonSerializerOptions { WriteIndented = true });
                File.WriteAllText(startupSettingsPath, json);
                SetStatus("Startup settings saved");
            }
            catch (Exception ex)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to save startup settings: {ex.Message}");
            }
        }

        private StartupSettings LoadStartupSettings()
        {
            if (!File.Exists(startupSettingsPath))
                return null;

            try
            {
                string json = File.ReadAllText(startupSettingsPath);
                return JsonSerializer.Deserialize<StartupSettings>(json);
            }
            catch
            {
                return null;
            }
        }

        private void EnsureAppDataExecutable()
        {
            try
            {
                // Get current running executable path
                string currentExePath = Process.GetCurrentProcess().MainModule.FileName;
                FileInfo currentFile = new FileInfo(currentExePath);

                // Check if AppData copy exists
                if (File.Exists(appDataExePath))
                {
                    FileInfo appDataFile = new FileInfo(appDataExePath);

                    // Compare file size and last write time
                    if (currentFile.Length == appDataFile.Length &&
                        currentFile.LastWriteTimeUtc == appDataFile.LastWriteTimeUtc)
                    {
                        // Files are the same, no need to copy
                        return;
                    }
                }

                // Copy the running executable to AppData
                File.Copy(currentExePath, appDataExePath, true);

                // Preserve the last write time
                File.SetLastWriteTimeUtc(appDataExePath, currentFile.LastWriteTimeUtc);

                SetStatus("Executable copied to AppData");
            }
            catch (Exception ex)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to copy executable to AppData: {ex.Message}");
            }
        }

        private bool ApplyStartupSettings(StartupSettings settings)
        {
            if (settings == null || settings.Zones.Count == 0)
                return false;

            // Verify GPU matches
            var currentGpuId = GetCurrentGpuIdentifier();
            if (currentGpuId == null || !currentGpuId.Matches(settings.GpuIdentifier))
            {
                return false;
            }

            // Apply settings to all zones
            foreach (var zoneProfile in settings.Zones)
            {
                if (zoneProfile.ZoneIndex >= globalZoneControls.Length)
                    continue;

                bool success = false;
                if (zoneProfile.ZoneType == "RGB")
                {
                    success = SetIlluminationZoneManualRGB(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                        zoneProfile.R, zoneProfile.G, zoneProfile.B, zoneProfile.Brightness, false);
                }
                else if (zoneProfile.ZoneType == "RGBW")
                {
                    success = SetIlluminationZoneManualRGBW(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                        zoneProfile.R, zoneProfile.G, zoneProfile.B, zoneProfile.W, zoneProfile.Brightness, false);
                }
                else
                {
                    success = SetIlluminationZoneManualSingleColor(currentGpuIndex, (uint)zoneProfile.ZoneIndex,
                        zoneProfile.Brightness, false);
                }
            }

            return true;
        }

        // Registry management methods
        private const string RegistryRunKey = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string AppRegistryName = "NvidiaFELighting";

        private void EnableStartup()
        {
            try
            {
                // Ensure the executable is copied to AppData
                EnsureAppDataExecutable();

                using (RegistryKey key = Registry.CurrentUser.OpenSubKey(RegistryRunKey, true))
                {
                    if (key != null)
                    {
                        key.SetValue(AppRegistryName, $"\"{appDataExePath}\" --startup");
                        SetStatus("Startup enabled");
                    }
                }
            }
            catch (Exception ex)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to enable startup: {ex.Message}");
            }
        }

        private void DisableStartup()
        {
            try
            {
                using (RegistryKey key = Registry.CurrentUser.OpenSubKey(RegistryRunKey, true))
                {
                    if (key != null)
                    {
                        key.DeleteValue(AppRegistryName, false);
                        SetStatus("Startup disabled");
                    }
                }
            }
            catch (Exception ex)
            {
                Xceed.Wpf.Toolkit.MessageBox.Show($"Failed to disable startup: {ex.Message}");
            }
        }

        private bool IsStartupEnabled()
        {
            try
            {
                using (RegistryKey key = Registry.CurrentUser.OpenSubKey(RegistryRunKey, false))
                {
                    if (key != null)
                    {
                        object value = key.GetValue(AppRegistryName);
                        return value != null;
                    }
                }
            }
            catch
            {
                // Ignore errors
            }
            return false;
        }

        // Checkbox event handlers
        private void ApplyOnStartupCheckBox_Checked(object sender, RoutedEventArgs e)
        {
            if (isInitializing) return;

            // Save current settings
            SaveStartupSettings();

            // Enable startup in registry
            EnableStartup();
        }

        private void ApplyOnStartupCheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            // Disable startup in registry
            DisableStartup();
        }
    }
}
