// Copyright Epic Games, Inc. All Rights Reserved.

using HordeServer.Notifications;
using HordeServer.Notifications.Sinks;
using Microsoft.Extensions.DependencyInjection;

namespace HordeServer.Tests
{
	/// <summary>
	/// Handles set up of collections, services, fixtures etc during testing
	/// </summary>
	public class ExperimentalTestSetup : BuildTestSetup
	{
		public ExperimentalTestSetup()
		{
			AddPlugin<ExperimentalPlugin>();
		}

		protected override void ConfigureServices(IServiceCollection services)
		{
			base.ConfigureServices(services);

			services.AddSingleton<INotificationSink, ExperimentalSlackNotificationSink>();
		}
	}
}