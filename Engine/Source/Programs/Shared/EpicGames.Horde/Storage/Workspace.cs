// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Channels;
using System.Threading.Tasks;
using EpicGames.Core;
using EpicGames.Horde.Storage.Nodes;
using Microsoft.Extensions.Logging;

namespace EpicGames.Horde.Storage
{
	/// <summary>
	/// Provides functionality to extract and patch data in a local workspace
	/// </summary>
	public class Workspace
	{
		// Tracked state of a directory
		class DirectoryState
		{
			public DirectoryState? Parent { get; }
			public string Name { get; }
			public List<DirectoryState> Directories { get; } = new List<DirectoryState>();
			public List<FileState> Files { get; } = new List<FileState>();

			public ulong LayerFlags { get; set; }

			public DirectoryState(DirectoryState? parent, string name)
			{
				Parent = parent;
				Name = name;
			}

			public void AddFile(FileState fileState)
			{
				int index = Files.BinarySearch(x => x.Name, fileState.Name, StringComparer.Ordinal);
				if (index >= 0)
				{
					throw new InvalidOperationException($"File {fileState.Name} already exists");
				}

				fileState.Parent = this;
				Files.Insert(~index, fileState);
			}

			public void RemoveFile(FileState fileState)
			{
				int index = Files.BinarySearch(x => x.Name, fileState.Name);
				if (index >= 0 && Files[index] == fileState)
				{
					Files.RemoveAt(index);
				}
			}

			public bool ContainsFile(string name) => TryGetFile(name, out _);

			public bool TryGetFile(string name, [NotNullWhen(true)] out FileState? fileState)
			{
				int index = Files.BinarySearch(x => x.Name, name, StringComparer.Ordinal);
				if (index >= 0)
				{
					fileState = Files[index];
					return true;
				}
				else
				{
					fileState = null;
					return false;
				}
			}

			public FileState FindOrAddFile(string name)
			{
				int index = Files.BinarySearch(x => x.Name, name, StringComparer.Ordinal);
				if (index >= 0)
				{
					return Files[index];
				}
				else
				{
					FileState fileState = new FileState(this, name);
					Files.Insert(~index, fileState);
					return fileState;
				}
			}

			public bool TryGetDirectory(string name, [NotNullWhen(true)] out DirectoryState? directoryState)
			{
				int index = Directories.BinarySearch(x => x.Name, name, StringComparer.Ordinal);
				if (index >= 0)
				{
					directoryState = Directories[index];
					return true;
				}
				else
				{
					directoryState = null;
					return false;
				}
			}

			public DirectoryState FindOrAddDirectory(string name)
			{
				int index = Directories.BinarySearch(x => x.Name, name, StringComparer.Ordinal);
				if (index >= 0)
				{
					return Directories[index];
				}
				else
				{
					DirectoryState subDirState = new DirectoryState(this, name);
					Directories.Insert(~index, subDirState);
					return subDirState;
				}
			}

			public DirectoryState FindOrAddDirectoryByPath(string path)
			{
				DirectoryState dirState = this;
				foreach (string fragment in path.Split('/'))
				{
					dirState = dirState.FindOrAddDirectory(fragment);
				}
				return dirState;
			}

			public string GetPath()
			{
				StringBuilder builder = new StringBuilder();
				AppendPath(builder);
				return builder.ToString();
			}

			public void AppendPath(StringBuilder builder)
			{
				Parent?.AppendPath(builder);

				if (Name.Length > 0)
				{
					builder.Append(Name);
				}
				if (builder.Length == 0 || builder[^1] != Path.DirectorySeparatorChar)
				{
					builder.Append(Path.DirectorySeparatorChar);
				}
			}

			public DirectoryReference GetDirectoryReference(DirectoryReference baseDir)
			{
				StringBuilder builder = new StringBuilder(baseDir.FullName);
				AppendPath(builder);
				return new DirectoryReference(builder.ToString(), DirectoryReference.Sanitize.None);
			}

			public void Read(IMemoryReader reader)
			{
				int numDirectories = reader.ReadInt32();
				Directories.Capacity = numDirectories;
				Directories.Clear();

				for (int idx = 0; idx < numDirectories; idx++)
				{
					string subDirName = reader.ReadString();

					DirectoryState subDirState = new DirectoryState(this, subDirName);
					subDirState.Read(reader);

					Directories.Add(subDirState);
				}

				int numFiles = reader.ReadInt32();
				Files.Capacity = numFiles;
				Files.Clear();

				for (int idx = 0; idx < numFiles; idx++)
				{
					string fileName = reader.ReadString();

					FileState fileState = new FileState(this, fileName);
					fileState.Read(reader);

					Files.Add(fileState);
				}

				LayerFlags = reader.ReadUnsignedVarInt();
			}

			public void Write(IMemoryWriter writer)
			{
				writer.WriteInt32(Directories.Count);
				foreach (DirectoryState directory in Directories)
				{
					writer.WriteString(directory.Name);
					directory.Write(writer);
				}

				writer.WriteInt32(Files.Count);
				foreach (FileState file in Files)
				{
					writer.WriteString(file.Name);
					file.Write(writer);
				}

				writer.WriteUnsignedVarInt(LayerFlags);
			}

			public override string ToString() => GetPath();
		}

		// Tracked state of a file
		[DebuggerDisplay("{Name}")]
		class FileState
		{
			public DirectoryState Parent { get; set; }
			public string Name { get; private set; }
			public long Length { get; private set; }
			public long LastModifiedTimeUtc { get; private set; }
			public IoHash Hash { get; set; }
			public ulong LayerFlags { get; set; }

			public FileState(DirectoryState parent, string name)
			{
				Parent = parent;
				Name = name;
			}

			public void Read(IMemoryReader reader)
			{
				Length = reader.ReadInt64();
				LastModifiedTimeUtc = reader.ReadInt64();
				Hash = reader.ReadIoHash();

				LayerFlags = reader.ReadUnsignedVarInt();
			}

			public void MoveTo(DirectoryState newParent, string newName)
			{
				Parent.RemoveFile(this);
				Name = newName;
				newParent.AddFile(this);
			}

			public void Delete()
			{
				Parent.RemoveFile(this);
			}

			public bool IsModified(FileInfo fileInfo) => Length != fileInfo.Length || LastModifiedTimeUtc != fileInfo.LastWriteTimeUtc.Ticks;

			public void Update(FileInfo fileInfo)
			{
				Length = fileInfo.Length;
				LastModifiedTimeUtc = fileInfo.LastWriteTimeUtc.Ticks;
			}

			public void Write(IMemoryWriter writer)
			{
				Debug.Assert(Hash != IoHash.Zero);
				writer.WriteInt64(Length);
				writer.WriteInt64(LastModifiedTimeUtc);
				writer.WriteIoHash(Hash);
				writer.WriteUnsignedVarInt(LayerFlags);
			}

			public string GetPath()
			{
				StringBuilder builder = new StringBuilder();
				AppendPath(builder);
				return builder.ToString();
			}

			public void AppendPath(StringBuilder builder)
			{
				Parent.AppendPath(builder);
				builder.Append(Name);
			}

			public FileReference GetFileReference(DirectoryReference baseDir)
			{
				StringBuilder builder = new StringBuilder(baseDir.FullName);
				AppendPath(builder);
				return new FileReference(builder.ToString(), FileReference.Sanitize.None);
			}

			public override string ToString() => GetPath();
		}

		// Collates lists of files and chunks with a particular hash
		[DebuggerDisplay("{Hash}")]
		class HashInfo
		{
			public int Index { get; set; }

			public IoHash Hash { get; }
			public long Length { get; }

			public List<FileState> Files { get; } = new List<FileState>();
			public List<ChunkInfo> Chunks { get; } = new List<ChunkInfo>();

			public HashInfo(IoHash hash, long length)
			{
				Hash = hash;
				Length = length;
			}

			public void AddChunk(ChunkInfo chunk)
			{
				if (chunk.Length != chunk.WithinHashInfo.Length && !Chunks.Contains(chunk))
				{
					Chunks.Add(chunk);
				}
			}
		}

		// Hashed chunk within another hashed object
		[DebuggerDisplay("{Offset}+{Length}->{WithinHashInfo}")]
		record class ChunkInfo(HashInfo WithinHashInfo, long Offset, long Length)
		{
			public static ChunkInfo Read(IMemoryReader reader, HashInfo[] hashes)
			{
				HashInfo withinHashInfo = hashes[(int)reader.ReadUnsignedVarInt()];
				long offset = (long)reader.ReadUnsignedVarInt();
				long length = (long)reader.ReadUnsignedVarInt();
				return new ChunkInfo(withinHashInfo, offset, length);
			}

			public void Write(IMemoryWriter writer)
			{
				writer.WriteUnsignedVarInt(WithinHashInfo.Index);
				writer.WriteUnsignedVarInt((ulong)Offset);
				writer.WriteUnsignedVarInt((ulong)Length);
			}
		}

		// Maps a layer id to a flag
		class LayerState
		{
			public WorkspaceLayerId Id { get; }
			public ulong Flag { get; }

			public LayerState(WorkspaceLayerId id, ulong flag)
			{
				Id = id;
				Flag = flag;
			}

			public LayerState(IMemoryReader reader)
			{
				Id = new WorkspaceLayerId(new StringId(reader.ReadUtf8String()));
				Flag = reader.ReadUnsignedVarInt();
			}

			public void Write(IMemoryWriter writer)
			{
				writer.WriteUtf8String(Id.Id.Text);
				writer.WriteUnsignedVarInt(Flag);
			}
		}

		readonly DirectoryReference _rootDir;
		readonly FileReference _stateFile;
		readonly DirectoryState _rootDirState;
		readonly List<LayerState> _layers;
		readonly Dictionary<IoHash, HashInfo> _hashes = new Dictionary<IoHash, HashInfo>();
		readonly ILogger _logger;

		const string HordeDirName = ".horde";
		const string CacheDirName = "cache";
		const string StateFileName = "contents.dat";

		/// <summary>
		/// Flag for the default layer
		/// </summary>
		const ulong DefaultLayerFlag = 1;

		/// <summary>
		/// Flag for the cache layer
		/// </summary>
		const ulong CacheLayerFlag = 2;

		/// <summary>
		/// Flags for user layers
		/// </summary>
		const ulong ReservedLayerFlags = DefaultLayerFlag | CacheLayerFlag;

		/// <summary>
		/// Root directory for the workspace
		/// </summary>
		public DirectoryReference RootDir => _rootDir;

		/// <summary>
		/// Layers current in this workspace
		/// </summary>
		public IReadOnlyList<WorkspaceLayerId> Layers => _layers.Select(x => x.Id).ToList();

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="rootDir">Directory for the workspace</param>
		/// <param name="stateFile">Path to the state file for this directory</param>
		/// <param name="logger">Logger for diagnostic output</param>
		private Workspace(DirectoryReference rootDir, FileReference stateFile, ILogger logger)
		{
			_rootDir = rootDir;
			_stateFile = stateFile;
			_rootDirState = new DirectoryState(null, String.Empty);
			_layers = new List<LayerState> { new LayerState(WorkspaceLayerId.Default, 1) };
			_logger = logger;
		}

		/// <summary>
		/// Create a new workspace instance in the given location. Opens the existing instance if it already contains workspace data.
		/// </summary>
		/// <param name="rootDir">Root directory for the workspace</param>
		/// <param name="logger">Logger for output</param>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		/// <returns>Workspace instance</returns>
		public static async Task<Workspace> CreateAsync(DirectoryReference rootDir, ILogger logger, CancellationToken cancellationToken = default)
		{
			FileReference stateFile = FileReference.Combine(rootDir, HordeDirName, StateFileName);

			using FileStream? stream = FileTransaction.OpenRead(stateFile);
			if (stream != null)
			{
				throw new InvalidOperationException($"Workspace already exists in {rootDir}; use Open instead.");
			}

			Workspace workspace = new Workspace(rootDir, stateFile, logger);
			await workspace.SaveAsync(cancellationToken);

			return workspace;
		}

		/// <summary>
		/// Create a new workspace instance in the given location. Opens the existing instance if it already contains workspace data.
		/// </summary>
		/// <param name="rootDir">Root directory for the workspace</param>
		/// <param name="logger">Logger for output</param>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		/// <returns>Workspace instance</returns>
		public static async Task<Workspace> CreateOrOpenAsync(DirectoryReference rootDir, ILogger logger, CancellationToken cancellationToken = default)
		{
			Workspace? workspace = await TryOpenAsync(rootDir, logger, cancellationToken);
			workspace ??= await CreateAsync(rootDir, logger, cancellationToken);
			return workspace;
		}

		/// <summary>
		/// Attempts to open an existing workspace for the current directory. 
		/// </summary>
		/// <param name="currentDir">Root directory for the workspace</param>
		/// <param name="logger">Logger for output</param>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		/// <returns>Workspace instance</returns>
		public static async Task<Workspace?> TryOpenAsync(DirectoryReference currentDir, ILogger logger, CancellationToken cancellationToken = default)
		{
			FileReference stateFile = FileReference.Combine(currentDir, HordeDirName, StateFileName);
			using (FileStream? stream = FileTransaction.OpenRead(stateFile))
			{
				if (stream != null)
				{
					byte[] data = await stream.ReadAllBytesAsync(cancellationToken);

					Workspace workspace = new Workspace(currentDir, stateFile, logger);
					workspace.Read(new MemoryReader(data));

					return workspace;
				}
			}
			return null;
		}

		/// <summary>
		/// Attempts to open an existing workspace for the current directory. 
		/// </summary>
		/// <param name="currentDir">Root directory for the workspace</param>
		/// <param name="logger">Logger for output</param>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		/// <returns>Workspace instance</returns>
		public static async Task<Workspace?> TryFindAndOpenAsync(DirectoryReference currentDir, ILogger logger, CancellationToken cancellationToken = default)
		{
			for (DirectoryReference? testDir = currentDir; testDir != null; testDir = testDir.ParentDirectory)
			{
				Workspace? workspace = await TryOpenAsync(testDir, logger, cancellationToken);
				if (workspace != null)
				{
					return workspace;
				}
			}
			return null;
		}

		/// <summary>
		/// Save the current state of the workspace
		/// </summary>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		public async Task SaveAsync(CancellationToken cancellationToken)
		{
			DirectoryReference.CreateDirectory(_stateFile.Directory);

			using (FileTransactionStream stream = FileTransaction.OpenWrite(_stateFile))
			{
				using (ChunkedMemoryWriter writer = new ChunkedMemoryWriter(64 * 1024))
				{
					Write(writer);
					await writer.CopyToAsync(stream, cancellationToken);
				}
				stream.CompleteTransaction();
			}
		}

		void Read(IMemoryReader reader)
		{
			_rootDirState.Read(reader);

			_layers.Clear();
			reader.ReadList(_layers, () => new LayerState(reader));

			// Read the hash lookup
			int numHashes = reader.ReadInt32();
			HashInfo[] hashInfoArray = new HashInfo[numHashes];

			_hashes.Clear();
			_hashes.EnsureCapacity(numHashes);

			for (int idx = 0; idx < numHashes; idx++)
			{
				IoHash hash = reader.ReadIoHash();
				long length = (long)reader.ReadUnsignedVarInt();
				hashInfoArray[idx] = new HashInfo(hash, length);
				_hashes.Add(hash, hashInfoArray[idx]);
			}
			for (int idx = 0; idx < numHashes; idx++)
			{
				HashInfo hashInfo = hashInfoArray[idx];
				reader.ReadList(hashInfoArray[idx].Chunks, () => ChunkInfo.Read(reader, hashInfoArray));
			}

			// Add all the files to the hash lookup
			AddDirToHashLookup(_rootDirState);
		}

		void Write(IMemoryWriter writer)
		{
			RemoveUnusedHashInfoObjects();

			_rootDirState.Write(writer);
			writer.WriteList(_layers, x => x.Write(writer));

			// Write the hash lookup
			writer.WriteInt32(_hashes.Count);

			int nextIndex = 0;
			foreach (HashInfo hashInfo in _hashes.Values)
			{
				writer.WriteIoHash(hashInfo.Hash);
				writer.WriteUnsignedVarInt((ulong)hashInfo.Length);
				hashInfo.Index = nextIndex++;
			}
			foreach (HashInfo hashInfo in _hashes.Values)
			{
				writer.WriteList(hashInfo.Chunks, x => x.Write(writer));
			}
		}

		void RemoveUnusedHashInfoObjects()
		{
			const int UnknownIndex = 0;
			const int ReferencedIndex = 1;
			const int UnreferencedIndex = 2;

			static bool IsReferenced(HashInfo hashInfo)
			{
				if (hashInfo.Index == UnknownIndex)
				{
					hashInfo.Chunks.RemoveAll(x => !IsReferenced(x.WithinHashInfo));
					hashInfo.Index = (hashInfo.Chunks.Count > 0) ? ReferencedIndex : UnreferencedIndex;
				}
				return hashInfo.Index == ReferencedIndex;
			}

			List<HashInfo> hashes = new List<HashInfo>(_hashes.Values);
			foreach (HashInfo hashInfo in hashes)
			{
				hashInfo.Index = (hashInfo.Files.Count > 0) ? ReferencedIndex : UnknownIndex;
			}

			_hashes.Clear();
			foreach (HashInfo hashInfo in hashes)
			{
				if (IsReferenced(hashInfo))
				{
					_hashes.Add(hashInfo.Hash, hashInfo);
				}
			}
		}

		#region Layers

		/// <summary>
		/// Add or update a layer with the given identifier
		/// </summary>
		/// <param name="id">Identifier for the layer</param>
		public void AddLayer(WorkspaceLayerId id)
		{
			if (_layers.Any(x => x.Id == id))
			{
				throw new InvalidOperationException($"Layer {id} already exists");
			}

			ulong flags = ReservedLayerFlags;
			for (int idx = 0; idx < _layers.Count; idx++)
			{
				flags |= _layers[idx].Flag;
			}
			if (flags == ~0UL)
			{
				throw new InvalidOperationException("Maximum number of layers reached");
			}

			ulong nextFlag = (flags + 1) ^ flags;
			_layers.Add(new LayerState(id, nextFlag));
		}

		/// <summary>
		/// Removes a layer with the given identifier. Does not remove any files in the workspace.
		/// </summary>
		/// <param name="layerId">Layer to update</param>
		public void RemoveLayer(WorkspaceLayerId layerId)
		{
			int layerIdx = _layers.FindIndex(x => x.Id == layerId);
			if (layerIdx > 0) // Note: Excluding default layer at index 0
			{
				LayerState layer = _layers[layerIdx];
				if ((_rootDirState.LayerFlags & layer.Flag) != 0)
				{
					throw new InvalidOperationException($"Workspace still contains files for layer {layerId}");
				}
				_layers.RemoveAt(layerIdx);
			}
		}

		LayerState? GetLayerState(WorkspaceLayerId layerId) => _layers.FirstOrDefault(x => x.Id == layerId);

		#endregion

		#region Syncing

		/// <summary>
		/// Syncs a layer to the given contents
		/// </summary>
		/// <param name="layerId">Identifier for the layer</param>
		/// <param name="basePath">Base path within the workspace to sync to.</param>
		/// <param name="contents">New contents for the layer</param>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		public async Task SyncAsync(WorkspaceLayerId layerId, string? basePath, DirectoryNode? contents, CancellationToken cancellationToken = default)
		{
			basePath = basePath?.Replace('\\', '/').Trim('/') ?? String.Empty;

			await using IBlobWriter writer = new MemoryBlobWriter(BlobSerializerOptions.Default);
			if (basePath.Length > 0)
			{
				foreach (string fragment in basePath.Split('/').Reverse())
				{
					DirectoryNode next = new DirectoryNode();
					if (contents != null)
					{
						IHashedBlobRef<DirectoryNode> handle = await writer.WriteBlobAsync(contents, cancellationToken);
						DirectoryEntry entry = new DirectoryEntry(fragment, contents.Length, handle);
						next.AddDirectory(entry);
					}
					contents = next;
				}
			}

			LayerState layerState = GetLayerState(layerId) ?? throw new InvalidOperationException($"Layer '{layerId}' does not exist");

			DirectoryState hordeDirState = _rootDirState.FindOrAddDirectory(HordeDirName);
			hordeDirState.LayerFlags |= CacheLayerFlag;

			DirectoryState cacheDirState = hordeDirState.FindOrAddDirectory(CacheDirName);
			cacheDirState.LayerFlags |= CacheLayerFlag;

			ExtractOptions options = new ExtractOptions();
			options.Progress = new ExtractStatsLogger(_logger);

			_logger.LogInformation("Creating manifest...");

			// Find all the files that need updating, and move anything that needs to be overwritten into the cache
			List<FileUpdate> fileUpdates = new List<FileUpdate>();
			await SyncDirectoryAsync(_rootDir, "", _rootDirState, contents, layerState.Flag, cacheDirState, fileUpdates, options, cancellationToken);
			await SaveAsync(cancellationToken);

			// Split the work into copies within the workspace, and reads from the upstream store
			Channel<ChunkCopyRequest> chunkCopyRequests = Channel.CreateUnbounded<ChunkCopyRequest>();
			Channel<ChunkReadRequest> chunkReadRequests = Channel.CreateUnbounded<ChunkReadRequest>();

			foreach (FileUpdate fileUpdate in fileUpdates)
			{
				OutputFile outputFile = fileUpdate.OutputFile;

				long offset = 0;
				foreach (LeafChunkedDataNodeRef leafNodeRef in fileUpdate.Chunks)
				{
					outputFile.IncrementRemaining();

					SourceChunk? sourceChunk = TryGetSourceChunk(leafNodeRef.BlobRef.Hash);
					if (sourceChunk != null)
					{
						ChunkCopyRequest copyRequest = new ChunkCopyRequest(outputFile, offset, sourceChunk);
						await chunkCopyRequests.Writer.WriteAsync(copyRequest, cancellationToken);
					}
					else
					{
						ChunkReadRequest readRequest = new ChunkReadRequest(outputFile, offset, leafNodeRef.BlobRef);
						await chunkReadRequests.Writer.WriteAsync(readRequest, cancellationToken);
					}

					offset += leafNodeRef.Length;
				}
			}
			chunkCopyRequests.Writer.Complete();
			chunkReadRequests.Writer.Complete();

			// Run the pipeline to create everything
			_logger.LogInformation("Updating workspace...");

			int numReadTasks = options.NumReadTasks ?? ExtractOptions.DefaultNumReadTasks;
			int numDecodeTasks = options.NumDecodeTasks ?? ExtractOptions.DefaultNumDecodeTasks;
			int numWriteTasks = 16;

			using BatchOutputWriter outputWriter = new BatchOutputWriter(_logger);
			using BatchChunkReader batchReader = new BatchChunkReader(outputWriter.RequestWriter) { VerifyHashes = options.VerifyOutput };

			await using (AsyncPipeline pipeline = new AsyncPipeline(cancellationToken))
			{
				Task[] copyTasks = pipeline.AddTasks(8, ct => CopyChunksAsync(chunkCopyRequests.Reader, outputWriter.RequestWriter, ct));
				Task[] readTasks = batchReader.AddToPipeline(pipeline, numReadTasks, numDecodeTasks, chunkReadRequests.Reader);
				_ = Task.WhenAll(Enumerable.Concat(copyTasks, readTasks)).ContinueWith(_ => outputWriter.RequestWriter.TryComplete(), TaskScheduler.Default);

				Task[] writeTasks = outputWriter.AddToPipeline(pipeline, numWriteTasks);

				if (options.Progress != null)
				{
					_ = pipeline.AddTask(ctx => DirectoryNodeExtract.UpdateStatsAsync(batchReader, outputWriter, Task.WhenAll(writeTasks), options.Progress, options.ProgressUpdateFrequency, ctx));
				}

				await pipeline.WaitForCompletionAsync();
			}

			// Update metadata for the workspace
			foreach (FileUpdate fileUpdate in fileUpdates)
			{
				OutputFile outputFile = fileUpdate.OutputFile;

				FileState fileState = fileUpdate.ParentDir.FindOrAddFile(outputFile.FileEntry.Name);
				fileState.Hash = outputFile.FileEntry.StreamHash;
				fileState.Update(outputFile.FileInfo);

				HashInfo hashInfo = AddFileToHashLookup(fileState);

				long offset = 0;
				foreach (LeafChunkedDataNodeRef chunkRef in fileUpdate.Chunks)
				{
					ChunkInfo chunkInfo = new ChunkInfo(hashInfo, offset, chunkRef.Length);

					HashInfo chunkHashInfo = FindOrAddHashInfo(chunkRef.BlobRef.Hash, chunkRef.Length);
					chunkHashInfo.AddChunk(chunkInfo);

					offset += chunkRef.Length;
				}
			}
		}

		record class FileUpdate(OutputFile OutputFile, DirectoryState ParentDir, List<LeafChunkedDataNodeRef> Chunks);

		async Task SyncDirectoryAsync(DirectoryReference dirRef, string path, DirectoryState dirState, DirectoryNode? dirNode, ulong flag, DirectoryState cacheDirState, List<FileUpdate> fileUpdates, ExtractOptions options, CancellationToken cancellationToken)
		{
			// Remove any directories that no longer exist
			for (int subDirIdx = 0; subDirIdx < dirState.Directories.Count; subDirIdx++)
			{
				DirectoryState subDirState = dirState.Directories[subDirIdx];
				if ((subDirState.LayerFlags & flag) != 0)
				{
					if (dirNode == null || !dirNode.TryGetDirectoryEntry(subDirState.Name, out _))
					{
						DirectoryReference subDirPath = DirectoryReference.Combine(dirRef, subDirState.Name.ToString());
						await SyncDirectoryAsync(subDirPath, $"{path}{subDirState.Name}/", subDirState, null, flag, cacheDirState, fileUpdates, options, cancellationToken);

						if (subDirState.LayerFlags == 0)
						{
							dirState.Directories.Remove(subDirState);
							subDirIdx--;
						}
					}
				}
			}

			// Remove any files that no longer exist
			for (int fileIdx = 0; fileIdx < dirState.Files.Count; fileIdx++)
			{
				FileState fileState = dirState.Files[fileIdx];
				if ((fileState.LayerFlags & flag) != 0)
				{
					if (dirNode == null || !dirNode.TryGetFileEntry(fileState.Name, out FileEntry? entry) || entry.StreamHash != fileState.Hash)
					{
						fileState.LayerFlags &= ~flag;

						if (fileState.LayerFlags == 0)
						{
							MoveFileToCache(fileState, cacheDirState);
							fileIdx--;
						}
					}
				}
			}

			// Clear out the layer flag for this directory. It'll be added back if we add/reuse files below.
			dirState.LayerFlags &= ~flag;

			// Add files for this directory
			if (dirNode != null)
			{
				DirectoryReference.CreateDirectory(dirRef);

				// Update directories
				foreach (DirectoryEntry subDirEntry in dirNode.Directories)
				{
					DirectoryReference subDirPath = DirectoryReference.Combine(dirRef, subDirEntry.Name.ToString());
					DirectoryState subDirState = dirState.FindOrAddDirectory(subDirEntry.Name);

					DirectoryNode subDirNode = await subDirEntry.Handle.ReadBlobAsync(cancellationToken: cancellationToken);
					await SyncDirectoryAsync(subDirPath, $"{path}{subDirEntry.Name}/", subDirState, subDirNode, flag, cacheDirState, fileUpdates, options, cancellationToken);

					dirState.LayerFlags |= flag;
				}

				// Update files
				foreach (FileEntry fileEntry in dirNode.Files)
				{
					await SyncFileAsync(dirRef, $"{path}{fileEntry.Name}", dirState, fileEntry, flag, fileUpdates, cancellationToken);
					dirState.LayerFlags |= flag;
				}
			}

			// Delete the directory if it's no longer needed
			if (dirState.LayerFlags == 0 && dirState.Parent != null)
			{
				FileUtils.ForceDeleteDirectory(dirRef);
			}
		}

		async Task SyncFileAsync(DirectoryReference dirRef, string path, DirectoryState dirState, FileEntry fileEntry, ulong flag, List<FileUpdate> fileUpdates, CancellationToken cancellationToken)
		{
			FileState? fileState;
			if (dirState.TryGetFile(fileEntry.Name, out fileState))
			{
				if (fileState.Hash == fileEntry.StreamHash)
				{
					fileState.LayerFlags |= flag;
				}
				else
				{
					throw new InvalidOperationException($"Conflicting hash for file {fileState.GetFileReference(_rootDir)}");
				}
			}
			else
			{
				FileInfo fileInfo = FileReference.Combine(dirRef, fileEntry.Name.ToString()).ToFileInfo();

				fileState = TryMoveCachedData(fileEntry.StreamHash, dirState, fileEntry.Name);
				if (fileState == null)
				{
					OutputFile outputFile = new OutputFile(path, fileInfo, fileEntry);
					List<LeafChunkedDataNodeRef> chunks = await fileEntry.Target.EnumerateLeafNodesAsync(cancellationToken).ToListAsync(cancellationToken);
					fileUpdates.Add(new FileUpdate(outputFile, dirState, chunks));
				}
				else
				{
					fileInfo.Refresh();
					fileState.Update(fileInfo);
					fileState.LayerFlags |= flag;
				}
			}
		}

		record class ChunkCopyRequest(OutputFile OutputFile, long Offset, SourceChunk SourceChunk);

		async Task CopyChunksAsync(ChannelReader<ChunkCopyRequest> copyRequests, ChannelWriter<OutputChunk[]> outputRequests, CancellationToken cancellationToken)
		{
			await foreach (ChunkCopyRequest copyRequest in copyRequests.ReadAllAsync(cancellationToken))
			{
				FileStreamOptions options = new FileStreamOptions
				{
					Mode = FileMode.Open,
					Share = FileShare.Read,
					Access = FileAccess.Read,
					Options = FileOptions.Asynchronous,
					BufferSize = 0,
				};

				SourceChunk sourceChunk = copyRequest.SourceChunk;
				FileReference sourceFile = sourceChunk.File.GetFileReference(RootDir);

				byte[] data = new byte[sourceChunk.Length];
				using (FileStream sourceStream = new FileStream(sourceFile.FullName, options))
				{
					sourceStream.Seek(sourceChunk.Offset, SeekOrigin.Begin);
					await sourceStream.ReadExactlyAsync(data, cancellationToken);
				}

				IoHash hash = IoHash.Compute(data);
				if (hash != sourceChunk.Hash)
				{
					throw new InvalidOperationException($"Source data does not have correct hash; expected {sourceChunk.Hash}, actually {hash} ({sourceFile}, offset: {sourceChunk.Offset}, length: {sourceChunk.Length})");
				}

#pragma warning disable CA2000
				OutputChunk outputChunk = new OutputChunk(copyRequest.OutputFile, copyRequest.Offset, data, sourceChunk, null);
				await outputRequests.WriteAsync([outputChunk], cancellationToken);
#pragma warning restore CA2000
			}
		}

		record class SourceChunk(FileState File, long Offset, long Length, IoHash Hash)
		{
			/// <inheritdoc/>
			public override string ToString()
				=> $"{File.GetPath()}@{Offset}";
		}

		SourceChunk? TryGetSourceChunk(IoHash hash)
		{
			HashInfo? hashInfo;
			if (_hashes.TryGetValue(hash, out hashInfo))
			{
				return TryGetSourceChunk(hashInfo);
			}
			else
			{
				return null;
			}
		}

		static SourceChunk? TryGetSourceChunk(HashInfo hashInfo)
		{
			if (hashInfo.Files.Count > 0)
			{
				return new SourceChunk(hashInfo.Files[0], 0, hashInfo.Length, hashInfo.Hash);
			}

			foreach (ChunkInfo chunkInfo in hashInfo.Chunks)
			{
				SourceChunk? otherChunk = TryGetSourceChunk(chunkInfo.WithinHashInfo);
				if (otherChunk != null)
				{
					return new SourceChunk(otherChunk.File, otherChunk.Offset + chunkInfo.Offset, hashInfo.Length, hashInfo.Hash);
				}
			}

			return null;
		}

		FileState? TryMoveCachedData(IoHash hash, DirectoryState targetDirState, string targetName)
		{
			HashInfo? hashInfo;
			if (_hashes.TryGetValue(hash, out hashInfo))
			{
				foreach (FileState file in hashInfo.Files)
				{
					if (file.LayerFlags == CacheLayerFlag)
					{
						MoveFile(file, targetDirState, targetName);
						file.LayerFlags = 0;
						return file;
					}
				}
			}
			return null;
		}

		async Task<bool> TryCopyCachedDataAsync(HashInfo hashInfo, long offset, long length, Stream outputStream, CancellationToken cancellationToken)
		{
			foreach (FileState file in hashInfo.Files)
			{
				if (await TryCopyChunkAsync(file, offset, length, outputStream, cancellationToken))
				{
					return true;
				}
			}
			foreach (ChunkInfo chunk in hashInfo.Chunks)
			{
				if (await TryCopyCachedDataAsync(chunk.WithinHashInfo, chunk.Offset, chunk.Length, outputStream, cancellationToken))
				{
					return true;
				}
			}
			return false;
		}

		async Task<bool> TryCopyChunkAsync(FileState file, long offset, long length, Stream outputStream, CancellationToken cancellationToken)
		{
			FileInfo fileInfo = GetFileInfo(file);
			if (fileInfo.Exists)
			{
				long initialPosition = outputStream.Position;

				using FileStream inputStream = fileInfo.OpenRead();
				inputStream.Seek(offset, SeekOrigin.Begin);

				byte[] tempBuffer = new byte[65536];
				while (length > 0)
				{
					int readSize = await inputStream.ReadAsync(tempBuffer.AsMemory(0, (int)Math.Min(length, tempBuffer.Length)), cancellationToken);
					if (readSize == 0)
					{
						outputStream.Seek(initialPosition, SeekOrigin.Begin);
						outputStream.SetLength(initialPosition);
						return false;
					}

					await outputStream.WriteAsync(tempBuffer.AsMemory(0, readSize), cancellationToken);
					length -= readSize;
				}
			}
			return true;
		}

		#endregion

		#region Refresh

		/// <summary>
		/// Updates the status of files in this workspace based on current filesystem metadata
		/// </summary>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		public async Task RefreshAsync(CancellationToken cancellationToken = default)
		{
			await RefreshAsync(_rootDirState, cancellationToken);
			RemoveUnusedHashInfoObjects();
		}

		async Task RefreshAsync(DirectoryState dirState, CancellationToken cancellationToken)
		{
			foreach (DirectoryState subDirState in dirState.Directories)
			{
				await RefreshAsync(subDirState, cancellationToken);
			}

			int outputIdx = 0;
			for (int idx = 0; idx < dirState.Files.Count; idx++)
			{
				FileState fileState = dirState.Files[idx];

				FileInfo fileInfo = GetFileInfo(fileState);
				if (!fileInfo.Exists)
				{
					_logger.LogWarning("File {Path} has been removed", fileInfo.FullName);
					RemoveFileFromHashLookup(fileState);
				}
				else if (fileState.IsModified(fileInfo))
				{
					_logger.LogWarning("File {Path} has been modified", fileInfo.FullName);
					RemoveFileFromHashLookup(fileState);
				}
				else
				{
					dirState.Files[outputIdx++] = fileState;
				}
			}
			dirState.Files.RemoveRange(outputIdx, dirState.Files.Count - outputIdx);
		}

		#endregion

		#region Verify

		/// <summary>
		/// Checks that all files within the workspace have the correct hash
		/// </summary>
		/// <param name="cancellationToken">Cancellation token for the operation</param>
		public async Task VerifyAsync(CancellationToken cancellationToken = default)
		{
			await VerifyAsync(_rootDirState, cancellationToken);
		}

		async Task VerifyAsync(DirectoryState dirState, CancellationToken cancellationToken)
		{
			foreach (DirectoryState subDirState in dirState.Directories)
			{
				await VerifyAsync(subDirState, cancellationToken);
			}
			foreach (FileState fileState in dirState.Files)
			{
				FileInfo fileInfo = GetFileInfo(fileState);
				if (fileState.IsModified(fileInfo))
				{
					throw new InvalidDataException($"File {fileInfo.FullName} has been modified");
				}

				IoHash hash;
				using (FileStream stream = GetFileInfo(fileState).OpenRead())
				{
					hash = await IoHash.ComputeAsync(stream, cancellationToken);
				}

				if (hash != fileState.Hash)
				{
					throw new InvalidDataException($"Hash for {fileInfo.FullName} was {hash}, expected {fileState.Hash}");
				}
			}
		}

		#endregion

		void AddDirToHashLookup(DirectoryState dirState)
		{
			foreach (DirectoryState subDirState in dirState.Directories)
			{
				AddDirToHashLookup(subDirState);
			}
			foreach (FileState fileState in dirState.Files)
			{
				AddFileToHashLookup(fileState);
			}
		}

		HashInfo FindOrAddHashInfo(IoHash hash, long length)
		{
			HashInfo? hashInfo;
			if (!_hashes.TryGetValue(hash, out hashInfo))
			{
				hashInfo = new HashInfo(hash, length);
				_hashes.Add(hash, hashInfo);
			}
			return hashInfo;
		}

		HashInfo AddFileToHashLookup(FileState file)
		{
			HashInfo hashInfo = FindOrAddHashInfo(file.Hash, file.Length);
			hashInfo.Files.Add(file);
			return hashInfo;
		}

		void RemoveFileFromHashLookup(FileState file)
		{
			HashInfo? hashInfo;
			if (_hashes.TryGetValue(file.Hash, out hashInfo))
			{
				hashInfo.Files.Remove(file);
			}
		}

		void MoveFileToCache(FileState fileState, DirectoryState cacheDirState)
		{
			string name = fileState.Hash.ToString();
			for (int idx = 0; idx < 2; idx++)
			{
				cacheDirState = cacheDirState.FindOrAddDirectory(name.Substring(idx * 2, 2));
			}

			FileInfo fileInfo = fileState.GetFileReference(_rootDir).ToFileInfo();
			if (fileInfo.Exists)
			{
				if (cacheDirState.ContainsFile(name) || fileState.IsModified(fileInfo))
				{
					DeleteFile(fileState);
				}
				else
				{
					MoveFile(fileState, cacheDirState, name);
					fileState.LayerFlags = CacheLayerFlag;
				}
			}
		}

		void MoveFile(FileState fileState, DirectoryState targetDirState, string targetName)
		{
			FileReference sourceFile = fileState.GetFileReference(_rootDir);
			fileState.MoveTo(targetDirState, targetName);

			FileReference targetFile = fileState.GetFileReference(_rootDir);
			DirectoryReference.CreateDirectory(targetFile.Directory);
			FileReference.Move(sourceFile, targetFile, true);

			_logger.LogDebug("Moving file from {Source} to {Target}", sourceFile, targetFile);
		}

		void DeleteFile(FileState fileState)
		{
			FileReference fileRef = fileState.GetFileReference(_rootDir);
			FileUtils.ForceDeleteFile(fileRef);
			fileState.Delete();

			_logger.LogDebug("Deleting file {File}", fileRef);
		}

		FileInfo GetFileInfo(FileState file)
		{
			return file.GetFileReference(_rootDir).ToFileInfo();
		}
	}
}
