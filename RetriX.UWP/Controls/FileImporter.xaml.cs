﻿using RetriX.Shared.ViewModels;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace RetriX.UWP.Controls
{
    public sealed partial class FileImporter : UserControl
    {
        public FileImporterVM VM
        {
            get { return (FileImporterVM)GetValue(VMProperty); }
            set { SetValue(VMProperty, value); }
        }

        // Using a DependencyProperty as the backing store for VM.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty VMProperty = DependencyProperty.Register(nameof(VM), typeof(FileImporterVM), typeof(FileImporter), new PropertyMetadata(null));

        public FileImporter()
        {
            this.InitializeComponent();
        }
    }
}
