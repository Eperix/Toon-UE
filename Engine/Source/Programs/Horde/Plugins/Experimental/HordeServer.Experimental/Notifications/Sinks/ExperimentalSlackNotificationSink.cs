// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Horde.Jobs;
using EpicGames.Horde.Jobs.Graphs;
using EpicGames.Slack;
using HordeServer.Agents;
using HordeServer.Configuration;
using HordeServer.Devices;
using HordeServer.Issues;
using HordeServer.Logs;
using HordeServer.Streams;
using HordeServer.Users;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace HordeServer.Notifications.Sinks
{
	/// <summary>
	/// Maintains a connection to Slack, in order to receive socket-mode notifications of user interactions
	/// </summary>
	public sealed class ExperimentalSlackNotificationSink : INotificationSink, IDisposable
	{
		readonly BuildServerConfig _buildServerConfig;
		readonly ILogger _logger;

		readonly HttpClient _httpClient;
		readonly SlackClient _slackClient;

		/// <summary>
		/// Constructor
		/// </summary>
		public ExperimentalSlackNotificationSink(IOptions<BuildServerConfig> buildServerConfig, ILogger<ExperimentalSlackNotificationSink> logger)
		{
			_buildServerConfig = buildServerConfig.Value;
			_logger = logger;

			_httpClient = new HttpClient();
			_httpClient.DefaultRequestHeaders.Add("Authorization", $"Bearer {_buildServerConfig.SlackToken ?? ""}");
			_slackClient = new SlackClient(_httpClient, _logger);

			_ = _slackClient;
		}

		/// <inheritdoc/>
		public void Dispose()
			=> _httpClient.Dispose();

		/// <inheritdoc/>
		public Task NotifyConfigUpdateAsync(ConfigUpdateInfo info, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyConfigUpdateFailureAsync(string errorMessage, string fileName, int? change = null, IUser? author = null, string? description = null, CancellationToken cancellationToken = default)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyDeviceServiceAsync(string message, IDevice? device = null, IDevicePool? pool = null, StreamConfig? streamConfig = null, IJob? job = null, IJobStep? step = null, INode? node = null, IUser? user = null, CancellationToken cancellationToken = default)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyIssueUpdatedAsync(IIssue issue, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyJobCompleteAsync(IJob job, IGraph graph, LabelOutcome outcome, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyJobCompleteAsync(IUser user, IJob job, IGraph graph, LabelOutcome outcome, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyJobScheduledAsync(List<JobScheduledNotification> notifications, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyJobStepCompleteAsync(IEnumerable<IUser> usersToNotify, IJob job, IJobStepBatch batch, IJobStep step, INode node, List<ILogEventData> jobStepEventData, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task NotifyLabelCompleteAsync(IUser user, IJob job, ILabel label, int labelIdx, LabelOutcome outcome, List<(string, JobStepOutcome, Uri)> stepData, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task SendAgentReportAsync(AgentReport report, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task SendDeviceIssueReportAsync(DeviceIssueReport report, CancellationToken cancellationToken)
			=> Task.CompletedTask;

		/// <inheritdoc/>
		public Task SendIssueReportAsync(IssueReportGroup report, CancellationToken cancellationToken)
			=> Task.CompletedTask;
	}
}
