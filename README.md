# WaitForPlayers

A free, server-authoritative multiplayer ready check plugin for Unreal Engine.

The server listens for players to send ready requests and replicates the ready status to all clients. Events fire when waiting starts, a player becomes ready, all players are ready, or a timeout is reached.

## Status

| Version | Branch | State |
|---|---|---|
| 1.0 | `main` (tag `v1.0.0`) | Current stable release, also available on Fab |
| 2.0 | `next/2.0` | In development |

This repository contains the code only. The example map and its content ship with the Fab release.

## Requirements

- Unreal Engine 5.8
- The `ModularGameplay` plugin, enabled automatically as a dependency
- Win64, Mac, iOS or Android

## Installation

```bash
git clone https://github.com/SandwichCakeStudios/WaitForPlayers.git
```

Copy the folder into your project's `Plugins/` directory, regenerate project files, build, and enable the plugin in the editor.

## Using 1.0

Everything goes through `UWaitForPlayersSubsystem`, from C++ or Blueprint:

1. Bind the events you need: `OnServerStartWaiting`, `OnPlayerReady`, `OnAllPlayersReady` and `OnTimeoutReached`.
2. On the server, call `StartWaitingForPlayers` with an optional timeout and player count.
3. Clients send their ready request through `UWaitForPlayersControllerComponent`.

See the [documentation](https://docs.google.com/document/d/e/2PACX-1vT-jShq6rdrPH5iblWn_ue9ktoaOAW-9TF7Ojypa66wsNnTkDhR4nqzlv_6nKgKEry7iYl39m6_P3M-/pub) for the full guide.

## What is coming in 2.0

2.0 is a rewrite with a new API, and a migration guide will ship with it.

- Several checks at once, each identified by a Gameplay Tag
- Rules in a designer-authored policy asset: required players, timeouts, countdowns and unready behavior
- Correct handling of late joiners and players who leave mid-check
- Optional timeout extensions and reconnect handling

## Support

Questions and bug reports are welcome on [Discord](https://discord.gg/azWURcymGY) or in GitHub Issues.

## License

[MIT License](LICENSE)
