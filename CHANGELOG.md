# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Linux to the supported platforms.
- `LogWaitForPlayers` log category.
- Native gameplay tag `ReadyCheck.MatchStart` and the `ReadyCheck.Vote.NextMap` config tag.
- The 2.0 API: `UReadyCheckSubsystem`, the `UReadyCheckPolicy` data asset, the `UReadyCheckRequirement` strategy base class and the ready check types.
- Working ready checks: the state replicates to every client, including late joiners, and players who join or leave update the check in the same frame. `UReadyCheckGameStateComponent` and `UReadyCheckPlayerComponent` are added at runtime, or can be authored on the actors.
- Built-in requirement strategies: All Eligible Players, Fixed Count and Percentage.
- All core `UReadyCheckPolicy` rules: `MinPlayers`, both timeout actions, the post-ready countdown, unready rules and the request cooldown.
- Optional `TimeoutExtension` group: `ExtendTimeout`, auto-extend on join and caps on both.
- Optional `Reconnect` group: returning players keep their ready state within `GracePeriod`, and `bHoldSlotWhileAway` makes the check wait for them.

### Changed
- `Engine`, `CoreUObject` and `ModularGameplay` are now public module dependencies, and `GameplayTags` and `NetCore` were added.

### Removed
- Unused `Slate` and `SlateCore` module dependencies.
- **Breaking:** `UWaitForPlayersSubsystem`, `UWaitForPlayersGameStateComponent` and `UWaitForPlayersControllerComponent`. The example content does not work until it is rebuilt on the 2.0 API.

## [1.0.0]

### Added
- Initial Fab release.
