// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using EpicGames.Core;
using EpicGames.Horde;
using EpicGames.Horde.Agents;
using EpicGames.Horde.Agents.Leases;
using EpicGames.Horde.Agents.Sessions;
using HordeAgent.Leases;
using HordeAgent.Services;
using HordeAgent.Tests.Services;
using HordeCommon.Rpc;
using HordeCommon.Rpc.Messages;
using HordeCommon.Rpc.Tasks;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using OpenTelemetry.Trace;

namespace HordeAgent.Tests.Leases;

public class CapabilitiesServiceStub(RpcAgentCapabilities? capabilities = null) : ICapabilitiesService
{
	public Task<RpcAgentCapabilities> GetCapabilitiesAsync(DirectoryReference? workingDir)
	{
		return Task.FromResult(capabilities ?? new RpcAgentCapabilities());
	}
}

public class SessionStub(AgentId agentId, SessionId sessionId, DirectoryReference workingDir, IHordeClient hordeClient) : ISession
{
	public AgentId AgentId { get; } = agentId;
	public SessionId SessionId { get; } = sessionId;
	public DirectoryReference WorkingDir { get; } = workingDir;
	public IHordeClient HordeClient { get; } = hordeClient;
	
	public async ValueTask DisposeAsync()
	{
		if (HordeClient is IAsyncDisposable disposableClient)
		{
			await disposableClient.DisposeAsync();
		}
		GC.SuppressFinalize(this);
	}
}

internal class TestHandlerFactory : LeaseHandlerFactory<TestTask>
{
	public override LeaseHandler<TestTask> CreateHandler(RpcLease lease)
	{
		return new Handler(lease);
	}
	
	private class Handler(RpcLease rpcLease) : LeaseHandler<TestTask>(rpcLease)
	{
		protected override Task<LeaseResult> ExecuteAsync(ISession session, LeaseId leaseId, TestTask message, Tracer tracer, ILogger logger, CancellationToken cancellationToken)
		{
			logger.LogInformation("Executed TestTask");
			LeaseResult leaseResult = new((byte[]?)null);
			return Task.FromResult(leaseResult);
		}
	}
}

[TestClass]
public sealed class LeaseManagerTests
{
	private readonly FakeHordeRpcServer _hordeServer;
	private readonly LeaseManager _leaseManager;
	
	public LeaseManagerTests()
	{
		_hordeServer= new FakeHordeRpcServer(CreateConsoleLogger<FakeHordeRpcServer>());
		_leaseManager = CreateLeaseManager(_hordeServer.GetHordeClient());
	}
	
	[TestMethod]
	public async Task Run_SingleLease_FinishesSuccessfullyAsync()
	{
		using CancellationTokenSource cts = new(5000);
		_hordeServer.ScheduleTestLease();
		Task<SessionResult> runTask = _leaseManager.RunAsync(cts.Token);
		_leaseManager.OnLeaseFinished += (lease, result) =>
		{
			Assert.AreEqual(RpcAgentStatus.Ok, _hordeServer.LastReportedStatus);
			_hordeServer.SetAgentStatus(RpcAgentStatus.Stopped);
		};
		
		Assert.AreEqual(new SessionResult(SessionOutcome.BackOff, SessionReason.Completed), await runTask);
	}
	
	[TestMethod]
	public async Task TerminateGracefully_LeaseIsActive_Async()
	{
		using CancellationTokenSource cts = new(5000);
		_leaseManager.TerminateSessionAfterLease = true;
		_hordeServer.ScheduleTestLease();
		Assert.AreEqual(new SessionResult(SessionOutcome.Terminate, SessionReason.Completed), await _leaseManager.RunAsync(cts.Token));
		Assert.AreEqual(RpcAgentStatus.Stopped, _hordeServer.AgentStatus);
	}
	
	[TestMethod]
	public async Task TerminateGracefully_NoLeaseActive_Async()
	{
		using CancellationTokenSource cts = new(5000);
		_leaseManager.TerminateSessionAfterLease = true;
		Assert.AreEqual(new SessionResult(SessionOutcome.Terminate, SessionReason.Completed), await _leaseManager.RunAsync(cts.Token));
		Assert.AreEqual(RpcAgentStatus.Stopped, _hordeServer.AgentStatus);
	}
	
	/// <summary>
	/// Create a console logger for tests
	/// </summary>
	/// <typeparam name="T">Type to instantiate</typeparam>
	/// <returns>A logger</returns>
	private static ILogger<T> CreateConsoleLogger<T>()
	{
		using ILoggerFactory loggerFactory = LoggerFactory.Create(builder =>
		{
			builder.SetMinimumLevel(LogLevel.Debug);
			builder.AddSimpleConsole(options => { options.SingleLine = true; });
		});
		
		return loggerFactory.CreateLogger<T>();
	}
	
	[SuppressMessage("Reliability", "CA2000:Dispose objects before losing scope")]
	private static LeaseManager CreateLeaseManager(IHordeClient hordeClient)
	{
		DirectoryReference tempWorkingDir = new(Path.Join(Path.GetTempPath(), Path.GetRandomFileName()));
		AgentSettings settings = new() { WriteStepOutputToLogger = true };
		ISession session = new SessionStub(new AgentId("testAgent"), SessionId.Parse("aaaaaaaaaaaaaaaaaaaaaaaa"), tempWorkingDir, hordeClient);
		TestOptionsMonitor<AgentSettings> settingsOptions = new (settings);
		StatusService statusService = new(settingsOptions, NullLogger<StatusService>.Instance);

		return new LeaseManager(
			session,
			new CapabilitiesServiceStub(),
			statusService,
			new DefaultSystemMetrics(),
			new List<LeaseHandlerFactory>() { new TestHandlerFactory() },
			new LeaseLoggerFactory(settingsOptions, CreateConsoleLogger<LeaseLoggerFactory>()),
			settingsOptions,
			TracerProvider.Default.GetTracer("TestTracer"),
			CreateConsoleLogger<LeaseManager>());
	}
}
