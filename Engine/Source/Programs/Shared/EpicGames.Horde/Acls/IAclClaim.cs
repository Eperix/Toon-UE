// Copyright Epic Games, Inc. All Rights Reserved.

namespace EpicGames.Horde.Acls
{
	/// <summary>
	/// Claim for an ACL
	/// </summary>
	public interface IAclClaim
	{
		/// <summary>
		/// Type of the claim
		/// </summary>
		string Type { get; }

		/// <summary>
		/// Value for the claim
		/// </summary>
		string Value { get; }
	}
}
