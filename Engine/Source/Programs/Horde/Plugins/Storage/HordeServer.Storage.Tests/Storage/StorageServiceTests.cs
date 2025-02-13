// Copyright Epic Games, Inc. All Rights Reserved.

using EpicGames.Core;
using EpicGames.Horde.Storage;
using HordeServer.Storage;
using Microsoft.Extensions.DependencyInjection;
using MongoDB.Driver;

namespace HordeServer.Tests.Storage
{
	[TestClass]
	public class StorageServiceTests : ServerTestSetup
	{
		public StorageService StorageService => ServiceProvider.GetRequiredService<StorageService>();

		public StorageServiceTests()
		{
			AddPlugin<StoragePlugin>();
		}

		[TestMethod]
		public async Task StatsTestAsync()
		{
			StorageService storageService = ServiceProvider.GetRequiredService<StorageService>();
			IStorageNamespace client = storageService.GetNamespace(new NamespaceId("memory"));

			await using (IBlobWriter writer = client.CreateBlobWriter())
			{
				BlobType type1 = new BlobType(Guid.Parse("{11C2D886-4164-3349-D1E9-6F943D2ED10B}"), 0);
				for (int idx = 0; idx < 1000; idx++)
				{
					writer.WriteFixedLengthBytes(new byte[] { 1, 2, 3 });
					await writer.CompleteAsync(type1);
					await writer.FlushAsync();
				}
			}

			await Clock.AdvanceAsync(TimeSpan.FromDays(7.0));

			await storageService.TickBlobsAsync(CancellationToken.None);
			await storageService.TickLengthsAsync(CancellationToken.None);
			await storageService.TickStatsAsync(CancellationToken.None);

			IReadOnlyList<IStorageStats> stats = await storageService.FindStatsAsync();
			Assert.AreEqual(1, stats.Count);
			Assert.IsTrue(stats[0].Namespaces.First().Value.Size > 3 * 1000);
		}

		[TestMethod]
		public async Task BlobCollectionTestAsync()
		{
			StorageService storageService = ServiceProvider.GetRequiredService<StorageService>();
			IStorageNamespace client = storageService.GetNamespace(new NamespaceId("memory"));

			BlobType type1 = new BlobType(Guid.Parse("{11C2D886-4164-3349-D1E9-6F943D2ED10B}"), 0);
			byte[] data1 = new byte[] { 1, 2, 3 };

			BlobType type2 = new BlobType(Guid.Parse("{6CB3A005-4787-26BA-3E79-D286CB7137D1}"), 0);
			byte[] data2 = new byte[] { 4, 5, 6 };

			IHashedBlobRef handle1a;
			IHashedBlobRef handle1b;
			IHashedBlobRef handle2;
			await using (IBlobWriter writer = client.CreateBlobWriter())
			{
				writer.WriteFixedLengthBytes(data1);
				writer.AddAlias("foo", 2);
				handle1a = await writer.CompleteAsync(type1);

				writer.WriteFixedLengthBytes(data1);
				writer.AddAlias("foo", 1);
				handle1b = await writer.CompleteAsync(type1);

				writer.WriteFixedLengthBytes(data2);
				writer.AddAlias("bar", 0);
				handle2 = await writer.CompleteAsync(type2);
			}

			BlobAlias[] aliases;

			aliases = await client.FindAliasesAsync("foo");
			Assert.AreEqual(2, aliases.Length);
			Assert.AreEqual(handle1a.GetLocator(), aliases[0].Target.GetLocator());
			Assert.AreEqual(handle1b.GetLocator(), aliases[1].Target.GetLocator());

			aliases = await client.FindAliasesAsync("bar");
			Assert.AreEqual(1, aliases.Length);
			Assert.AreEqual(handle2.GetLocator(), aliases[0].Target.GetLocator());
		}

		[TestMethod]
		public async Task ForwardDeclareAsync()
		{
			StorageService storageService = ServiceProvider.GetRequiredService<StorageService>();
			NamespaceId namespaceId = new NamespaceId("memory");

			StorageService.BlobInfo? blobInfo = await storageService.FindBlobAsync(namespaceId, new BlobLocator("foo"));
			Assert.IsNull(blobInfo);

			blobInfo = await storageService.AddBlobAsync(namespaceId, new BlobLocator("foo"), [new BlobLocator("bar")]);
			Assert.AreEqual(1, blobInfo.Imports?.Count ?? 0);
			Assert.IsFalse(blobInfo.Shadow);

			StorageService.BlobInfo? importInfo = await storageService.FindBlobAsync(namespaceId, new BlobLocator("bar"));
			Assert.IsNotNull(importInfo);
			Assert.IsTrue(importInfo.Shadow);

			StorageService.BlobInfo? importInfo2 = await storageService.AddBlobAsync(namespaceId, new BlobLocator("bar"), []);
			Assert.AreEqual(importInfo.Id, importInfo2.Id);
			Assert.IsFalse(blobInfo.Shadow);
		}
	}
}

