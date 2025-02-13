// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Horde.Jobs.Templates;

namespace HordeServer.Jobs.Templates
{
	/// <summary>
	/// Internal implementation of <see cref="ITemplateCollection"/> which
	/// allows registering new template instances.
	/// </summary>
	public interface ITemplateCollectionInternal : ITemplateCollection
	{
		/// <summary>
		/// Gets a template instance from its configuration
		/// </summary>
		/// <param name="templateConfig">The template configuration</param>
		/// <returns>Template instance</returns>
		Task<ITemplate> GetOrAddAsync(TemplateConfig templateConfig);
	}
}
