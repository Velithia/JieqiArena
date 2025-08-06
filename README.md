# JieqiArena

JieqiArena is a demo of JAI (Jieqi Arena Interface) that supports basic engine competition functionalities and can be loaded and used by [JieqiBox](https://github.com/Velithia/JieqiBox).

## JAI Options

### Engine Configuration

*   **Engine1Path**
    *   Description: The full path to the first UCI-compatible Jieqi engine executable.
    *   Type: `string`
    *   Default: (none)

*   **Engine1Options**
    *   Description: A string of UCI `setoption` commands for Engine 1. Each option must follow the format `name <Option Name> value <Value>`. Multiple options are separated by spaces. This parser correctly handles option names and values that contain spaces.
    *   Type: `string`
    *   Default: (empty)
    *   Example: `name Threads value 4 name Hash value 256`

*   **Engine2Path**
    *   Description: The full path to the second UCI-compatible Jieqi engine executable.
    *   Type: `string`
    *   Default: (none)

*   **Engine2Options**
    *   Description: A string of UCI `setoption` commands for Engine 2. See `Engine1Options` for format and examples.
    *   Type: `string`
    *   Default: (empty)

### Tournament Settings

*   **TotalRounds**
    *   Description: The number of pairs of games to be played. The total number of games will be `TotalRounds * 2`, as engines switch colors for each round.
    *   Type: `spin`
    *   Default: `10`
    *   Min: `1`
    *   Max: `1000`

*   **Concurrency**
    *   Description: The number of games to run in parallel.
    *   Type: `spin`
    *   Default: `2`
    *   Min: `1`
    *   Max: `128`

### Time Control

*   **MainTimeMs**
    *   Description: The base thinking time for each player in a game, specified in milliseconds.
    *   Type: `spin`
    *   Default: `1000`
    *   Min: `0`
    *   Max: `3600000`

*   **IncTimeMs**
    *   Description: The time increment added to a player's clock after each move, specified in milliseconds.
    *   Type: `spin`
    *   Default: `0`
    *   Min: `0`
    *   Max: `60000`

*   **TimeoutBufferMs**
    *   Description: A grace period in milliseconds to account for process and communication overhead. A player is only declared lost on time if their clock falls below `-(TimeoutBufferMs)`.
    *   Type: `spin`
    *   Default: `5000`
    *   Min: `0`
    *   Max: `60000`

### Debugging

*   **Logging**
    *   Description: If enabled (`true`), the match engine will create detailed log files for each engine process, capturing all UCI communication. The files are named `engine_debug_<Color>_job<ID>.log`.
    *   Type: `check`
    *   Default: `false`