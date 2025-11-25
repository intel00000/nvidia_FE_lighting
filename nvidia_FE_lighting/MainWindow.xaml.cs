using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Windows;
using System.Windows.Controls;
using Xceed.Wpf.Toolkit;
using System.Windows.Media;
using static nvidia_FE_lighting.NvApiWrapper;

namespace nvidia_FE_lighting
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		private CustomIlluminationZoneControl[] globalZoneControls;
		private uint currentGpuIndex;
		private void SetStatus(string message)
		{
			statusText.Text = message;
		}

		public MainWindow()
		{
			InitializeComponent();

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
				gpuSelectComboBox.SelectedIndex = 0;
		}

		// Event handler for GPU selection change
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

		// Event handler for the "Get Illumination Zones" button
		private void PopulateIlluminationZones(uint gpuIndex)
		{
			SetStatus("Detecting illumination zones...");
			gpuIlluminationZones.Items.Clear();

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

			Xceed.Wpf.Toolkit.ColorPicker? colorPicker = null;
			if (zoneType == "RGB" || zoneType == "RGBW")
			{
				byte r = zoneType switch
				{
					"RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.r,
					"RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.r,
					_ => (byte)0
				};
				byte g = zoneType switch
				{
					"RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.g,
					"RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.g,
					_ => (byte)0
				};
				byte b = zoneType switch
				{
					"RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.b,
					"RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.b,
					_ => (byte)0
				};
				colorPicker = new Xceed.Wpf.Toolkit.ColorPicker
				{
					SelectedColor = System.Windows.Media.Color.FromRgb(r, g, b),
					Margin = new Thickness(0, 0, 0, 10),
					DisplayColorAndName = true,
					DisplayColorTooltip = true,
				};
				panel.Children.Add(colorPicker);
			}

			byte brightnessValue = zoneType switch
			{
				"RGB" => globalZoneControls[zoneIndex].manualColorData.rgb.brightness,
				"RGBW" => globalZoneControls[zoneIndex].manualColorData.rgbw.brightness,
				_ => globalZoneControls[zoneIndex].manualColorData.singleColor.brightness
			};
			panel.Children.Add(new TextBlock { Text = $"Active Brightness: {brightnessValue}%", Margin = new Thickness(0, 0, 0, 10) });
			var brightnessSlider = new Slider
			{
				Minimum = 0,
				Maximum = 100,
				Value = brightnessValue,
				TickFrequency = 1,
				IsSnapToTickEnabled = true,
				Margin = new Thickness(0, 0, 0, 10)
			};
			var brightnessLabel = new TextBlock { Text = $"Target Brightness: {brightnessValue}%", Margin = new Thickness(0, 0, 0, 10) };
			brightnessSlider.ValueChanged += (s, e) =>
			{
				brightnessLabel.Text = $"Target Brightness: {brightnessSlider.Value}%";
			};
			panel.Children.Add(brightnessLabel);
			panel.Children.Add(brightnessSlider);

			var applyButton = new Button
			{
				Content = "Apply",
				Margin = new Thickness(0, 0, 0, 10),
				Tag = (gpuIndex, zoneIndex, colorPicker, brightnessSlider, zoneType)
			};

			applyButton.Click += (s, e) =>
			{
				var (gpuIdx, zoneIdx, picker, slider, type) = ((uint, int, Xceed.Wpf.Toolkit.ColorPicker, Slider, string))((Button)s).Tag;
				var color = picker?.SelectedColor ?? System.Windows.Media.Colors.Black;
				byte brightness = (byte)slider.Value;
				bool result = false;

				if (type == "RGB")
				{
					result = SetIlluminationZoneManualRGB(gpuIdx, (uint)zoneIdx, color.R, color.G, color.B, brightness, false);
				}
				else if (type == "RGBW")
				{
					result = SetIlluminationZoneManualRGBW(gpuIdx, (uint)zoneIdx, color.R, color.G, color.B, 0, brightness, false);
				}
				else
				{
					result = SetIlluminationZoneManualSingleColor(gpuIdx, (uint)zoneIdx, brightness, false);
				}

				if (!result)
				{
					Xceed.Wpf.Toolkit.MessageBox.Show("Failed to set illumination zone color");
				}
				else
				{
					SetStatus($"Updated zone {zoneIdx} on GPU {gpuIdx}");
					PopulateIlluminationZones(gpuIdx);
				}
			};
			panel.Children.Add(applyButton);
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
				SetStatus($"Detecting zones on GPU {index}...");
				PopulateIlluminationZones(index);
			}
			else
			{
				SetStatus("Select a GPU to detect zones.");
			}
		}
	}
}
