// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Core;
using EpicGames.Horde.Storage;

namespace UnrealToolbox.Plugins.Artifacts
{
	class DownloadOptions
	{
		public Uri BaseUrl { get; set; }
		public RefName RefName { get; set; }
		public DirectoryReference OutputDir { get; set; }

		public DownloadOptions(Uri baseUrl, RefName refName, DirectoryReference outputDir)
		{
			BaseUrl = baseUrl;
			RefName = refName;
			OutputDir = outputDir;
		}
	}
}
