// Copyright Epic Games, Inc. All Rights Reserved.

using System.Diagnostics;
using Avalonia.Controls;
using Avalonia.Interactivity;

namespace UnrealToolbox.Plugins.Artifacts
{
	partial class DownloadProgressWindow : Window
	{
		readonly DownloadProgressViewModel _viewModel;

		public DownloadProgressWindow()
			: this(new DownloadProgressViewModel())
		{
		}

		public DownloadProgressWindow(DownloadProgressViewModel viewModel)
		{
			InitializeComponent();

			_viewModel = viewModel;
			DataContext = viewModel;
		}

		public void OpenFolder(object? sender, RoutedEventArgs args)
		{
			if (!String.IsNullOrEmpty(_viewModel.OpenFolderPath))
			{
				Process.Start(new ProcessStartInfo { FileName = _viewModel.OpenFolderPath, UseShellExecute = true });
				Close();
			}
		}

		public void Cancel(object? sender, RoutedEventArgs args)
		{
			Close();
		}
	}
}
