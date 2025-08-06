## JAI Protocol

### GUI (JieqiBox) -> Match Engine (JAI Engine)

`jai`
- Handshake command.

`setoption name <OptionName> value <OptionValue>`
- Set parameters.

`isready`
- Query ready status.

`startmatch`
- Start match.

`stop`
- Stop.

`quit`
- Exit.

---

### Match Engine (JAI Engine) -> GUI (JieqiBox)

`id name <EngineName>`
`id author <AuthorName>`
- Engine self information.

`jaiok`
- Confirmation of `jai` command.
- After receiving `jai`, the engine must first return `id`, then immediately list all supported `option`s, and finally output `jaiok`.

`option name <Name> type <Type> [default <Value>] [min <Value>] [max <Value>] [var <Value>] ...`
- **Description:** Declare a supported parameter, GUI will generate settings interface accordingly.
- **`type` types:**
  - `string`: String, usually used for paths or filenames.
    - Example: `option name BookFile type string default C:\book.dat`
  - `spin`: Integer.
    - Example: `option name Threads type spin default 4 min 1 max 128`
  - `check`: Boolean value (`true`/`false`).
    - Example: `option name UseGUIBook type check default true`
  - `button`: Button. When user clicks, GUI will send `setoption name <Name>` to the engine, used to trigger an action.
    - Example: `option name ClearHash type button`
  - `combo`: Dropdown list. Can be followed by multiple `var` to define options.
    - Example: `option name AdjudicationRule type combo default Off var Off var MateOrStalemate`

`readyok`
- Confirmation of `isready` command.

`info string <Message>`
- JAI engine's own status information.

`info game <CurrentGame>/<TotalGames>`
- Current game progress.

`info wld <Wins>-<Losses>-<Draws>`
- Total wins, losses, and draws statistics for the current match.

`info fen <FENString>`
- Current game FEN, only sent once at the start of the game, GUI will clear the current move record.

`info move <MoveString> time <TimeMs>`
- Engine move with time taken in milliseconds.

`info result <ResultString>`
- Single game result (`1-0`, `0-1`, `1/2-1/2`).

`info engine <RedEngine> <BlackEngine>`
- Indicates the red and black engines currently competing.

`info depth ...`
- Analysis information from UCI engine output, passed through to JieqiBox as-is.

### Interaction Flow Example
1. GUI -> `jai`
2. Engine -> `id name Jieqi Match Tools`
3. Engine -> `id author xxx`
4. Engine -> `option name Engine1Path type string` ...
5. Engine -> `option name TotalRounds type spin default 100`
6. Engine -> `jaiok`
7. ... (list all options)
8. GUI -> `setoption name Engine1Path value ...` ...
9. GUI -> `isready`
10. Engine -> `readyok`
11. GUI -> `startmatch`
12. Engine -> `info string Match started`
13. Engine -> `info game 1/100`
14. Engine -> `info engine engine1.exe engine2.exe`
15. ...
16. Engine -> `info depth 10 score cp 50 ...` (This is passed through from the jieqi engine)
17. ...
18. Engine -> `info result 1-0`
19. Engine -> `info wld 1-0-0`