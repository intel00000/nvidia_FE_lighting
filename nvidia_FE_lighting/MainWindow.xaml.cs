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

            gpuSelectComboBox.SelectionChanged += (s, e) =>
            {
                if (gpuSelectComboBox.SelectedIndex >= 0)
                    RefreshGpuInfo((uint)gpuSelectComboBox.SelectedIndex);
            };

            if (gpuSelectComboBox.Items.Count > 0)
            {
                gpuSelectComboBox.SelectedIndex = 0;
                RefreshGpuInfo(0);
            }
        }

        // Event handler for GPU selection change
        private void RefreshGpuInfo(uint index)
        {
            gpuNameText.Text = "Name: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetGPUName(index));
            gpuInfoText.Text = "Details: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetGPUInfo(index));
            gpuSystemTypeText.Text = "System Type: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetSystemType(index));
            gpuDriverVersionText.Text = "Driver Version: " + NvApiWrapper.GetDriverVersion();
            gpuInterfaceVersionText.Text = "NVAPI Interface: " + Marshal.PtrToStringAnsi(NvApiWrapper.GetInterfaceVersionString());
        }
    }
}