# FileWatcher

A configurable Windows file-watching application written in C++ that monitors a directory and automatically performs actions on files based on configurable rules.

FileWatcher detects file system changes, waits for files to become stable, and executes matching actions such as sending files to Telegram, removing filename patterns, or moving files to another directory.

Rules and application settings are configured through JSON files, allowing behavior to be customized without modifying the source code.

## Features

- **Real-time monitoring** of a directory and its subdirectories.
- **Automatic file processing** when files are created, modified, renamed, or discovered during startup.
- **Debouncing and file stability checks** to avoid processing files while they are still being written.
- **Pattern-based rules** using case-insensitive wildcard matching.
- **Configurable actions** that can be chained together within a single rule.
- **Telegram integration** for sending files or their contents as text.
- **File organization** through automatic file moves.
- **Filename cleanup** using configurable regular expressions.
- **JSON-based configuration** for application settings and rules.
- **Recovery through directory rescanning** when the Windows file notification buffer overflows.

## Table of Contents

- [Requirements](#requirements)
- [Dependencies](#dependencies)
- [Project Setup](#project-setup)
- [Building](#building)
- [Running](#running)
- [Application Settings](#application-settings)
- [Rules Configuration](#rules-configuration)
- [Available Actions](#available-actions)
- [Telegram Setup](#telegram-setup)
- [How File Processing Works](#how-file-processing-works)
- [Troubleshooting](#troubleshooting)
- [Limitations](#limitations)

## Requirements

- Windows, using the Windows directory notification API.
- A C++ compiler with support for C++23 features, including `std::expected` and `std::jthread`.
- CMake, if your project uses the CMake build setup described below.
- Git.
- The dependencies listed in the next section.

The application currently uses `WindowsWatcher` and `WindowsFileProbe`, so it is designed for Windows rather than as a cross-platform file watcher.

## Dependencies

The source code uses the following libraries:

| Dependency | Purpose |
|---|---|
| [nlohmann/json](https://github.com/nlohmann/json) | Parsing JSON settings and rules |
| [libcurl](https://curl.se/libcurl/) | Sending messages and uploading files to Telegram |
| Windows API | Monitoring file system changes |

The project also uses the C++ standard library, including filesystem, threads, regular expressions, and `std::expected`.

Make sure the JSON and libcurl libraries are available to your build system. If you use CMake, you can manage these dependencies through `find_package`, `FetchContent`, or a package manager such as vcpkg.

> **Note:** The source files provided here do not include the project's `CMakeLists.txt` or package-manager manifest. The exact build commands depend on how those dependencies are configured in your repository.

## Project Setup

### 1. Clone the repository

```bash
git clone <YOUR_REPOSITORY_URL>
cd <YOUR_REPOSITORY_DIRECTORY>
```

Replace the placeholders with your repository URL and directory name.

### 2. Create the configuration files

The application expects `settings.json` in its current working directory. The rules file defaults to `rules.json`, unless another path is configured.

A typical directory layout is:

```text
FileWatcher/
├── src/
├── include/
├── settings.json
├── rules.json
├── .env
├── CMakeLists.txt
└── README.md
```

Your actual source layout may differ. The important requirement is that `settings.json`, the configured rules file, and the optional `.env` file are accessible from the application's working directory.

### 3. Configure `settings.json`

Create a `settings.json` file in the working directory:

```json
{
  "watched_folder": "C:/Saxion/Courses/Testing",
  "debounce_time_ms": 500,
  "rules_path": "rules.json"
}
```

Change `watched_folder` to the directory you want to monitor.

### 4. Configure `rules.json`

Create a `rules.json` file containing an array of rules. Each rule specifies matching conditions, an ordered list of actions, and whether subsequent rules should be skipped after a match.

See [Rules Configuration](#rules-configuration) for the complete format.

### 5. Configure Telegram (optional)

If you want to use the `sendToTelegram` action, create a `.env` file:

```dotenv
TELEGRAM_TOKEN=your_bot_token
TELEGRAM_CHAT_ID=your_chat_id
```

Replace both values with your actual Telegram credentials.

If `.env` cannot be loaded or either variable is missing, Telegram integration is disabled. Rules using `sendToTelegram` will fail configuration parsing because the action cannot be created without a Telegram client.

If you do not need Telegram, omit the `.env` file and avoid using Telegram actions in your rules.

## Building

The exact build command depends on the repository's build configuration.

### Using CMake

If your repository contains a configured `CMakeLists.txt`, a typical build workflow is:

```bash
cmake -S . -B build
cmake --build build --config Release
```

For a Visual Studio generator, the executable will typically be under:

```text
build/Release/
```

For a single-configuration generator, the output location depends on the project's CMake target settings.

If you use vcpkg, configure the appropriate toolchain file when generating the build:

```bash
cmake -S . -B build ^
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

Replace the toolchain path with your actual vcpkg installation path.

Ensure that the selected compiler supports C++23 and that libcurl is correctly linked.

## Running

Start the compiled executable from the directory containing `settings.json`:

```powershell
.\FileWatcher.exe
```

Replace `FileWatcher.exe` with the actual executable name produced by your build.

On startup, the application:

1. Loads and validates `settings.json`.
2. Attempts to load Telegram credentials from `.env`.
3. Registers the built-in actions.
4. Loads and validates the configured rules file.
5. Checks that the watched directory exists.
6. Starts the directory watcher and file-processing threads.
7. Performs an initial scan of the watched directory and its subdirectories.

The console displays detected events and files ready for processing.

To stop the application, press **Enter** in the console window.

**Important:** Run the executable with the intended working directory. The application opens `settings.json` and `.env` using relative paths, and `rules.json` is also relative by default.

## Application Settings

The application settings are stored in `settings.json`.

### Available settings

| Setting | Type | Default | Description |
|---|---|---|---|
| `watched_folder` | String | Required | Directory to monitor recursively |
| `debounce_time_ms` | Integer | `500` | Quiet period after the most recent file event, in milliseconds |
| `rules_path` | String | `rules.json` | Path to the JSON file containing rules |

### Example configuration

```json
{
  "watched_folder": "C:/Users/Example/Documents/Incoming",
  "debounce_time_ms": 750,
  "rules_path": "config/rules.json"
}
```

### `watched_folder`

The directory whose files and subdirectories will be monitored.

Use an absolute path where possible. Forward slashes work well in Windows JSON paths:

```json
"watched_folder": "C:/Users/Example/Downloads"
```

The directory must exist when the application initializes.

### `debounce_time_ms`

Specifies how long a file must remain quiet before it can be processed.

For example:

```json
"debounce_time_ms": 1000
```

With a value of `1000`, the application waits at least one second after the latest tracked event before checking whether the file is ready.

- Lower values reduce processing latency.
- Higher values help accommodate applications that write files in multiple stages.
- If a file is still changing or is not considered stable, processing is postponed.

The value must be a non-negative integer. The default is `500` milliseconds.

### `rules_path`

Specifies the location of the rules file:

```json
"rules_path": "rules.json"
```

To keep your rules in another directory:

```json
"rules_path": "config/automation-rules.json"
```

The rules path defaults to `rules.json` if omitted.

**Settings are loaded at startup.** Changes to `settings.json` or `rules.json` require restarting the application; live configuration reload is not implemented in the provided code.

## Rules Configuration

Rules are defined in the JSON file specified by `rules_path`.

The root JSON value must be an array. Each element represents a rule.

### Rule structure

```json
{
  "match": {
    "patterns": ["*.pdf"],
    "events": ["Created", "Renamed", "Init"]
  },
  "actions": [
    {
      "type": "moveTo",
      "destination": "C:/Users/Example/Documents/PDFs"
    }
  ],
  "stopOnMatch": true
}
```

Each rule contains three main properties:

| Property | Required | Description |
|---|---|---|
| `match` | Yes | Defines filename patterns and event types |
| `actions` | Yes | Ordered list of actions to execute |
| `stopOnMatch` | No | Stops evaluating later rules after this rule's actions succeed; defaults to `true` |

### Matching patterns

The `patterns` property contains one or more wildcard patterns.

```json
"patterns": ["*.pdf", "*.docx"]
```

Patterns are matched against the **filename only**, not the full file path. Matching is case-insensitive.

The supported wildcard characters are:

| Pattern | Meaning | Example matches |
|---|---|---|
| `*` | Zero or more characters | `*.pdf` matches `report.pdf` |
| `?` | Exactly one character | `file?.txt` matches `file1.txt` |

Multiple patterns are combined using **OR** logic. A filename needs to match at least one pattern.

Examples:

```json
"patterns": ["*_tg.*"]
```

Matches filenames such as `report_tg.pdf` and `image_tg.png`.

```json
"patterns": ["*cv*"]
```

Matches filenames containing `cv`, regardless of letter case, such as `MyCV.pdf` or `cv_backup.docx`.

Patterns are converted internally from wildcard syntax to regular expressions. They are not arbitrary regular expressions.

### Matching events

The `events` property determines which file events can trigger a rule.

Supported values are:

| Event | Description |
|---|---|
| `Created` | A file was created |
| `Modified` | A file was modified |
| `Renamed` | A file was renamed |
| `Init` | A file was discovered during the initial scan or a rescan |

Both title-case and lowercase spellings are accepted, for example `Created` and `created`.

Example:

```json
"events": ["Created", "Renamed", "Init"]
```

Multiple event types are combined using **OR** logic. A rule matches when the file's accumulated event flags contain at least one configured event type and its filename matches a configured pattern.

**Removed events are not valid rule triggers.** Deleting a file removes it from pending processing, rather than producing a `FileReady` item.

### Initial scan behavior

When the application starts, it recursively scans the watched directory and queues existing regular files as `Init` events.

This means existing files can trigger rules that include `Init`, even if those files were not created while the application was running.

After a notification buffer overflow, the application also rescans the directory. During this recovery scan, existing files are tracked using `Modified` events.

Consequently, rules listening for `Modified` may process files again after a recovery rescan.

### Action ordering

Actions within a rule execute in the order in which they appear in the `actions` array.

For example:

```json
"actions": [
  {
    "type": "sendToTelegram",
    "sendMethod": "file"
  },
  {
    "type": "removePattern",
    "pattern": "_tg"
  }
]
```

The file is sent to Telegram first. After the upload succeeds, the application removes `_tg` from the filename.

If an action fails, processing stops and the error is logged. Subsequent actions and rules are not executed for that file during that processing attempt.

If an action deletes or moves the file out of the watched location, downstream processing may also stop, depending on the resulting file state.

### `stopOnMatch`

This property controls whether later rules can run after a rule's actions finish successfully.

**When `true` (default):** the application stops evaluating later rules after this rule completes.

```json
"stopOnMatch": true
```

**When `false`:** the application continues evaluating subsequent rules.

```json
"stopOnMatch": false
```

This is useful when multiple independent rules should apply to the same file.

Keep in mind that actions can change filenames or move files, so later rules may see a changed path or no longer have a file to process.

## Available Actions

The built-in action registry currently supports three action types:

- `sendToTelegram`
- `removePattern`
- `moveTo`

Each action requires a `type` property.

### 1. `sendToTelegram`

Sends a file to Telegram, either as an uploaded document or as text.

**Settings**

| Property | Required | Values | Description |
|---|---|---|---|
| `type` | Yes | `sendToTelegram` | Selects the Telegram action |
| `sendMethod` | No | `file`, `txt` | Controls how the file is sent; defaults to `file` |

**Send as a file**

```json
{
  "type": "sendToTelegram",
  "sendMethod": "file"
}
```

Uploads the file using Telegram's `sendDocument` API.

**Send as text**

```json
{
  "type": "sendToTelegram",
  "sendMethod": "txt"
}
```

Sends the file's contents as a Telegram message. The exact content-reading and validation behavior depends on the implementation of `SendToTelegramAction`.

**Requirements**

- A valid `TELEGRAM_TOKEN` in `.env`.
- A valid `TELEGRAM_CHAT_ID` in `.env`.
- Network connectivity to the Telegram API.
- A file that Telegram can accept for the selected send method.

The Telegram client uses HTTPS with certificate and hostname verification enabled. Requests have a connection timeout of 10 seconds and an overall timeout of 20 seconds.

### 2. `removePattern`

Removes every occurrence of a regular expression match from the filename.

**Settings**

| Property | Required | Type | Description |
|---|---|---|---|
| `type` | Yes | String | Must be `removePattern` |
| `pattern` | Yes | String | Regular expression used to identify text to remove |

Example:

```json
{
  "type": "removePattern",
  "pattern": "_tg"
}
```

For a file named:

```text
report_tg.pdf
```

The expected resulting filename is:

```text
report.pdf
```

Another example:

```json
{
  "type": "removePattern",
  "pattern": "_draft|_temp"
}
```

This matches `_draft` or `_temp` and removes matching occurrences according to the action's implementation.

**Important:** Unlike rule patterns, the `pattern` property is a regular expression, not a wildcard expression. It is compiled without automatically adding start or end anchors, so matching is not restricted to the entire filename.

Use care with broad expressions, as they may remove more text than intended.

### 3. `moveTo`

Moves the file to a specified destination directory.

**Settings**

| Property | Required | Type | Description |
|---|---|---|---|
| `type` | Yes | String | Must be `moveTo` |
| `destination` | Yes | String | Target directory path |

Example:

```json
{
  "type": "moveTo",
  "destination": "C:/Users/Example/Documents/CVs"
}
```

If a file named `resume.pdf` matches the rule, the action moves it to the configured destination directory.

For Windows paths, either forward slashes or escaped backslashes can be used in JSON:

```json
"destination": "C:/Documents/CVs"
```

or:

```json
"destination": "C:\\Documents\\CVs"
```

Ensure the destination directory is accessible and that the application has permission to write there. Behavior when a destination file already exists, or when moving across volumes, depends on the `MoveToAction` implementation.

## Complete Example

The following configuration demonstrates all three built-in actions.

### `settings.json`

```json
{
  "watched_folder": "C:/Users/Example/Incoming",
  "debounce_time_ms": 500,
  "rules_path": "rules.json"
}
```

### `rules.json`

```json
[
  {
    "match": {
      "patterns": ["*_tg.*"],
      "events": ["Created", "Renamed", "Init"]
    },
    "actions": [
      {
        "type": "sendToTelegram",
        "sendMethod": "file"
      },
      {
        "type": "removePattern",
        "pattern": "_tg"
      }
    ],
    "stopOnMatch": true
  },
  {
    "match": {
      "patterns": ["*_tgt.*"],
      "events": ["Created", "Renamed", "Init"]
    },
    "actions": [
      {
        "type": "sendToTelegram",
        "sendMethod": "txt"
      },
      {
        "type": "removePattern",
        "pattern": "_tgt"
      }
    ],
    "stopOnMatch": true
  },
  {
    "match": {
      "patterns": ["*cv*"],
      "events": ["Created", "Renamed", "Init"]
    },
    "actions": [
      {
        "type": "moveTo",
        "destination": "C:/Users/Example/Documents/CVs"
      }
    ],
    "stopOnMatch": true
  }
]
```

### How the example works

1. Files containing `_tg` in their filename are uploaded to Telegram as documents. After a successful upload, `_tg` is removed from the filename.
2. Files containing `_tgt` in their filename are sent to Telegram as text. After a successful send, `_tgt` is removed from the filename.
3. Files whose filenames contain `cv`, regardless of case, are moved to the configured CV directory.
4. Each rule stops further rule evaluation after its actions succeed.

Since the first two rules match the suffixes `_tg` and `_tgt` respectively, files with those suffixes should be named carefully to avoid unintended pattern matches.

## Telegram Setup

### 1. Create a Telegram bot

Use Telegram's official [@BotFather](https://t.me/BotFather) to create a bot and obtain its token.

Keep the token private. Anyone who obtains it may be able to control your bot.

### 2. Find your chat ID

Determine the chat ID for the conversation, group, or channel where the bot should send messages. Ensure the bot has permission to send messages or upload documents there.

### 3. Create `.env`

Place this file next to `settings.json`, assuming you launch the application from that directory:

```dotenv
TELEGRAM_TOKEN=123456789:your_actual_token
TELEGRAM_CHAT_ID=123456789
```

Do not commit your actual credentials to GitHub. Add `.env` to `.gitignore`:

```gitignore
.env
```

You may also want to ignore local build output and IDE files:

```gitignore
build/
.vs/
.idea/
.vscode/
```

### 4. Verify integration

Start the application and trigger a rule containing `sendToTelegram`.

If the credentials are missing or the Telegram request fails, check the console output and verify the bot's permissions, chat ID, and network connectivity.

## How File Processing Works

The application separates file monitoring from file processing.

1. **Watch:** `WindowsWatcher` uses `ReadDirectoryChangesW` to monitor the watched directory recursively for file and directory name changes, attributes, size, last-write time, and creation time.
2. **Queue:** Detected events are sent through a thread-safe queue.
3. **Debounce:** `SettleDetector` waits for the configured quiet period after the latest relevant event.
4. **Check stability:** The file probe checks whether the path still represents a regular file and whether the file is stable.
5. **Prepare:** Once stable, the file is emitted as a `FileReady` item with accumulated event flags.
6. **Match:** `RuleEngine` evaluates rules in their configured order.
7. **Execute:** Matching rules run their actions sequentially.
8. **Recover:** If the Windows notification buffer overflows, the application rescans the watched directory to reconcile its tracked files.

### File stability

Debouncing alone does not guarantee that a file is ready. A file may remain open or continue to be written after the quiet period.

The application therefore uses `WindowsFileProbe` to check file stability before processing. If the file is still unstable, its deadline is postponed.

### Rename behavior

Windows reports old and new filename information through separate notifications. The watcher attempts to pair these notifications into a rename event.

If it cannot correlate the old and new names, it safely degrades to separate removal and creation events rather than assuming an incorrect rename.

### Console output

Typical output includes messages such as:

```text
App Running..
Starting watcher on: "C:/Users/Example/Incoming"
Created: C:/Users/Example/Incoming/report_tg.pdf
[FileReady] Ready: "C:/Users/Example/Incoming/report_tg.pdf"
```

Exact messages depend on the events received and the configured actions.

## Troubleshooting

### `settings.json` cannot be opened

- Verify that the file exists.
- Launch the executable from the directory containing the configuration file.
- Check that the process can read the file.

### The watched directory does not exist

Check the `watched_folder` value and ensure the directory exists before starting the application.

### Failed to parse config

Check the JSON syntax in `settings.json` and `rules.json`. JSON does not allow comments or trailing commas.

Also verify that every rule contains a `match` object and an `actions` array, and that every action has a valid `type`.

### A rule does not match a file

- Confirm that the filename matches one of the configured wildcard patterns.
- Remember that matching is case-insensitive and applies to the filename only.
- Check that the event is included in the rule's `events` array.
- Remember that existing files are processed as `Init` events at startup.
- Verify that an earlier rule with `stopOnMatch: true` has not prevented later rules from running.

### Telegram is disabled

Check that `.env` exists in the current working directory and contains both `TELEGRAM_TOKEN` and `TELEGRAM_CHAT_ID`.

If Telegram credentials are missing, do not configure rules that use `sendToTelegram`.

### Telegram sends fail

Verify that the bot token and chat ID are correct, that the bot has permission to send messages or documents, and that the network can reach `api.telegram.org`.

### Files are processed too early or too late

Adjust `debounce_time_ms` in `settings.json`.

Increase it if files are often processed while still being written. Decrease it if files are stable quickly and you want faster processing.

### Files are not moved or renamed as expected

Check that the destination path is valid and writable, and inspect the `pattern` regular expression used by `removePattern`.

The behavior of file moves, filename collisions, and rename operations also depends on the individual action implementations.

## Limitations

- The current watcher implementation is Windows-specific.
- Rules and application settings are loaded at startup; changes require a restart.
- Rule patterns operate on filenames rather than full paths.
- File deletion is not a supported rule trigger.
- Rename notifications cannot always be reliably paired by Windows, so some renames may appear as removal and creation.
- Telegram's message and document limits apply to uploads.
- File stability and move behavior depend on the platform-specific file probe and action implementations.
- The current built-in action registry includes only `sendToTelegram`, `removePattern`, and `moveTo`.

## Contributing

Contributions are welcome. When adding a new action:

1. Implement the action using the project's `IAction` interface.
2. Register its factory in `ActionRegistry`.
3. Validate the action's JSON properties during creation.
4. Document its `type` and supported settings.
5. Add tests for success, failure, and interactions with other actions.

## License

Add the license applicable to this repository here. If the project has not yet been licensed, choose and add a license before presenting the repository as open source.
