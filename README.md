# SQLite Logger
#### A simple SQLite-based logging utility written in C
##### Gary Woodcock, [gary.woodcock@unthinkable.com](mailto:gary.woodcock@unthinkable.com?subject=SQLite%20Logger)
+ ###### v0.1.0, 2022.03.02 - First released

## Table of Contents
+ [Rationale](#rationale)
+ [Design and Implementation Notes](#design-and-implementation-notes)
+ [Getting Started](#getting-started)
+ [Example Usage](#example-usage)
+ [API Reference](#api-reference)
+ [License](./LICENSE)

## Rationale
A C-based project I was recently working on needed to be extensively instrumented over a large number of program configurations and input data sets, and I had been using simple `fprintf` commands to log the instrumentation to a text file. The `fprintf` code was bracketed by a C preprocessor define to control whether the code deployed with or without logging enabled. The whole thing looked something like this:

    #define ENABLE_LOGGING  1

    char* path = "./my_log_file.txt"
    uint32_t bytesProcessed = 0;
    int8_t byteValues[32];
    bool done = false;

    #if ENABLE_LOGGING
        FILE* logFile = fopen(path, "w");
    #endif

    while (!done)
    {
        bytesProcessed = ProcessSomeBytes(byteValues);

        #if ENABLE_LOGGING
            fprintf(logFile, "Bytes processed: %d\n", bytesProcessed);
        #endif

        // etc.
    }

    #if ENABLE_LOGGING
        (void)fclose(logFile);
    #endif

Further, I used C preprocessor defines to scope log entries to different levels – for example, like this, for "normal" and "extended" logging:

    #define NORMAL_LOGGING      1
    #define EXTENDED_LOGGING    0

    uint32_t bytesProcessed;
    int8_t byteValues[32];

    // Some byte operations here...
    bytesProcessed = ProcessSomeBytes(byteValues);

    #if ENABLE_LOGGING

        #if NORMAL_LOGGING
            fprintf(logFile, "Bytes processed: %d\n", byteCount);
        #endif

        #if EXTENDED_LOGGING
            uint_fast32_t i = 0;
            for (i = 0; i < byteCount; i++)
                fprintf(logFile, "Byte value %d is 0x%08X\n", i, byteValue);
        #endif

    #endif

This probably doesn't look too alien to experienced C programmers – I imagine some may be rolling their eyes after reading this. While I think this type of logging is just fine when employed judiciously, I've found that using it where *lots* of logging is required can quickly obscure the code that is actually being instrumented, because there's just so much *more* of it.

One other item of note; this approach *required* a recompile of my code if any changes to the logging state (on/off) or level (normal/extended) was desired. I hadn't put in any mechanism to set the logging state and level at runtime. In many cases this may be fine, but in other cases, it could lead to delays in diagnosing issues.

Back to my project... Each unique configuration and input data set was generating millions of log entries, and there were thousands of unique combinations of configurations/input data sets. Just keeping all the files straight was a nightmare!

I had the (undoubtedly unoriginal) idea that logging to a database might provide a more flexible way of handling this mass of data than logging to a bunch of text files. This solution would also allow for standard SQL queries to extract interesting information, and it might have higher performance.

I had prior experience using [SQLite](https://www.sqlite.org/), and it seemed like an ideal place to begin experimenting. __SQLite Logger__ is the fruit of that experimentation, and my hope is that others may find it useful either as is, or as a basis for their own customization and/or experimentation.

## Design and Implementation Notes
This section covers several key design and implementation details – it's not essential reading, and is provided largely as background.

### Design
I very much wanted to keep the design and implementation of SQLite Logger simple. I didn't want programmers to worry overmuch about things like configuration, namespaces, and so forth. As much as possible, I wanted a very straightforward API that was largely self-explantory.

I also didn't want programmers to have to deal with the specifics of the SQLite C API or even SQL itself, so all that is "under the hood" (though obviously, it's open to inspection). 

I wanted to expose a means to easily control the scope of the logging, so that controlling the amount of logging activity was as simple as possible.

Lastly, I wanted there to be some customization available in the logging API, so that programmers could easily tailor the logging to their specific needs.

### Implementation
In order to make it as simple as possible to integrate with a variety of software, I've chosen to compile the [amalgamated](https://www.sqlite.org/amalgamation.html) version of SQLite directly into SQLite Logger. 

In addition, SQLite has been left in its default serialized threading mode. This should allow SQLite Logger to be safely used out of the box by multiple threads without restriction. Refer to [Using SQLite In Multi-Threaded Applications](https://www.sqlite.org/threadsafe.html) for more information.

A SQLite Logger log file can have one or more `log` tables. The first `log` table is created when a log file is initially created by calling `SL_Initialize`. The name of this table takes the form of `log at YYYY-MM-DD HH:mm:SS:uuuuuu`, where `YYYY-MM-DD HH:mm:SS.uuuuuu` represents the timestamp (year, month, day, hour, minute, second and microsecond) when the table was created. The log file is closed when `SL_Terminate` is called. If the same log file is again opened with a called to `SL_Initialize`, then a new `log` table with the current timestamp in its name is created. This allows multiple `log` tables to exist within a single log file.

The schema of a `log` table is simple. There are a total of 9 columns, as described below:

+ `log_id: INTEGER (required, primary key)`
+ `log_timestamp: TEXT (required, limited to 32 characters)`
+ `log_message: TEXT (required, limited to 1024 characters)`
+ `log_level: TEXT (required, limited to 16 characters)`
+ `log_filename: TEXT (optional, limited to 256 characters)`
+ `log_functionname: TEXT (optional, limited to 256 characters)`
+ `log_linenumber: INTEGER (optional)`
+ `log_tag: TEXT (optional, limited to 128 characters)`
+ `log_supplementaldata: TEXT (optional, limited to 1024 characters)`

I had given consideration to using more complex types (such as `BLOB` for `log_supplementaldata`), but in the end, I think using simple, fixed length types is more in keeping with the design intent stated previously.

SQLite Logger uses the notion of "log levels" to help scope the amount of information that is written to the log file. There are six defined log levels, and they act as a hierarchical filter on messages that are logged to the log file. These are, from lowest log level to highest:

+ `eSL_LogLevel_Diagnostic`
+ `eSL_LogLevel_Detail`
+ `eSL_LogLevel_Info`
+ `eSL_LogLevel_Warning`
+ `eSL_LogLevel_Error`
+ `eSL_LogLevel_None`

SQLite Logger maintains a global log level internally that it uses to decide whether to log a message to the log file. By default, this log level is set to `eSL_LogLevel_Info`. At this level, any calls to SQLite Logger to log messages with a log level less than `eSL_LogLevel_Info` (e.g., with log levels of either `eSL_LogLevel_Detail` or `eSL_LogLevel_Diagnostic`) will *not* be logged; messages at log level `eSL_LogLevel_Info` or higher will be logged.

SQLite Logger's log level state can be changed at *runtime*, so there is a lot of flexibility in terms of determining which log messages are recorded in the log file. There is no need to scope the logging calls with compile-time macros, unless there is a requirement to make the target binary as small as possible. If you want to turn off *all* logging, simply set the SQLite Logger log level to `eSL_LogLevel_None`; if you want to log *everything*, set the log level to `eSL_LogLevel_Diagnostic`.

Associated with each `log` table in the log file are 5 views, which provide filtering on a log level (diagnostic, detail, info, warning, and error) – these are provided for convenience in browsing the `log` tables.

## Getting Started
These instructions will help you get a copy of the SQLite Logger source code and get it built and running on your local machine.

### Supported Environments
It should be possible to build SQLite Logger in any `Unix/Linux` style environment, but only `Darwin/macOS` and `Linux/Ubuntu` are directly supported by the [build utility](#using-the-build-utility) and tested at this time. 64-bit ARM-based (`arm64` or `aarch64`) and Intel-based (`x64`) architectures are supported, and the code should be compatible with other architectures (e.g., 32-bit or non-ARM/Intel) as long as `gcc` or `clang` with appropriate C runtime libraries are available.

I've decided not to worry about directly supporting Windows environments at this stage – SQLite Logger should compile and run within the Windows Subsystem for Linux without modification. Windows support is something that is under consideration for a later release.

### Prerequisites
The required installs are listed below:

+ `bash` or compatible shell
+ `clang` or `gcc`
+ `git`
+ `make`

Optional (but recommended) installs include:

+ [`cppcheck`](https://cppcheck.sourceforge.io/)
+ [`CUnit`](https://sourceforge.net/projects/cunit/)
+ [`doxygen`](https://www.doxygen.nl/index.html)
+ [`gprof`](https://ftp.gnu.org/old-gnu/Manuals/gprof-2.9.1/html_mono/gprof.html)
+ `scan-build`
+ [`Visual Studio Code`](https://code.visualstudio.com/)

If you choose to use Visual Studio Code, you may find these extensions helpful:

+ [`CodeLLDB`](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)
+ [`Indented Block Highlighting`](https://marketplace.visualstudio.com/items?itemName=byi8220.indented-block-highlighting)
+ [`Markdown Preview Enhanced`](https://marketplace.visualstudio.com/items?itemName=shd101wyy.markdown-preview-enhanced)
+ [`SQLite Viewer`](https://marketplace.visualstudio.com/items?itemName=qwtel.sqlite-viewer)

### Getting the Code
The SQLite Logger repository is hosted at GitHub. You can clone it by opening a terminal window and executing these commands:

    cd ~
    git clone https://github.com/unthinkable/sqlite-logger.git

Alternatively, you can download a copy of the current sources as a [zip archive](https://github.com/unthinkable/sqlite-logger/archive/refs/heads/main.zip).

### Using the Build Utility
The SQLite Logger source code comes with a Bash-based build utility named [`build.sh`](./scripts/build.sh) that helps manage the building and testing of SQLite Logger. 

__NOTE__: You're not required to use the build utility – you can use the `makefile`s directly with `make`. However, you will have to set a number of environment variables to alter the default behavior of a `makefile`. Listed below are the critical variables to set for a successful build:

+ `BUILD_ARCH`: `x64` and `arm64` are defined
+ `BUILD_CFG`: `Debug` and `Release` are defined
+ `BUILD_OPERATING_ENV`: `darwin` and `linux` are defined
+ `BUILD_PROFILE`: `0` (don't profile) and `1` (profile) are defined
+ `BUILD_ROOT`: The path to the `sqlite-logger` source directory
+ `BUILD_SHARED_LIB`: `0` (static library) and `1` (shared library) are defined

The `makefile` for the SQLite Logger library is at [`src/makefile`](./src/makefile), and the `makefile` for the unit test is at [`test/makefile`](./test/makefile). 

You will also need to manually create the `sqlite_logger_config.h` file in the `include` directory. It should contain 1 line indicating how many log entries the log entry cache should contain, as shown below:

    #define SL_LOG_ENTRY_CACHE_SIZE 1024

#### Getting Help
To get help with using the build utility, open a terminal window and execute these commands:

    cd ~/sqlite-logger/scripts
    ./build.sh --help

This will display the various options for the build utility, as shown below:

    USAGE:  build.sh <args>
 
        All arguments are optional. With no arguments, the default behavior is:
 
        • Code analysis with cppcheck
        • Incremental debug build of programs and static library
        • Root directory path is '/Users/foo'
        • No SDK
        • No verbose output
        • Without documentation build
        • With log entry cache size of '1024'
        • Without profiling
        • Without unit testing
 
        Possible argument values are:
 
        --analyze=<none|full|cppcheck|scan-build>       Analyzes the source code with the specified tools.
        --check-env                                     Checks the build support on the host environment.
        --clean                                         Forces a clean build instead of an incremental build.
        --debug                                         Builds debug version.
        --help                                          Prints this usage notice.
        --release                                       Builds release version.
        --root-directory-path=<path>                    Sets the path to the root directory containing the SQLite Logger
                                                        source code directory (defaults to the user's home directory).
        --verbose                                       Prints all build log output to console.
        --with-documentation                            Builds documentation using Doxygen.
        --with-log-entry-cache-size=<value>             Sets the size of the log entry cache.
        --with-profiling                                Builds with profiling enabled (Linux only).
        --with-sdk                                      Creates a Software Development Kit (SDK) archive in the results directory.
        --with-shared-libs                              Build and link with shared library instead of static library.
        --with-unit-testing                             Perform unit testing after build.
 
        Prerequisites for running this script include:
 
        • bash shell
        • clang or gcc with C99 support
        • cppcheck (used with --analyze=cppcheck|full options)
        • CUnit (used with --with-unit-testing option)
        • Doxygen (used with --with-documentation option)
        • gprof (used with --with-profiling option)
        • make
        • scan-build (used with --analyze=scan-build|full options)
        • xmllint (used to parse unit test results)

#### Checking Prerequisites
After you have a copy of the SQLite Logger source code, you can verify that your system meets the minimum requirements by opening a terminal window and executing the following commands:

    cd ~/sqlite-logger/scripts
    ./build.sh --check-env

This will display a summary of your environment, as it pertains to SQLite Logger. On an Intel 64-bit macOS system, it looks like this:

    *****************************************
    *** DARWIN x64 HOST ENVIRONMENT CHECK ***
    ***************************************** 
    
                 bash: Installed (v3.2.57)
                clang: Installed (v13.0.0)
    clang C99 support: Available
             cppcheck: Installed (v2.7)
                CUnit: Installed
              Doxygen: Installed (v1.9.3)
                  gcc: Not installed
      gcc C99 support: Not applicable
                  git: Installed (v2.35.1)
                gprof: Not installed
             Homebrew: Not installed
             MacPorts: Installed (v2.7.1)
                 make: Installed (v3.81)
           scan-build: Not installed
              xmllint: Installed

As long as the required installs are listed as `Installed`, you're ready to build!

#### Building the SQLite Logger Library
To build the SQLite Logger library, open a terminal window and execute these commands:

    cd ~/sqlite-logger/scripts
    ./build.sh

This uses the build utility defaults, and will build an incremental debug static library version of SQLite Logger with a `cppcheck` code quality analysis. If you examine the directory structure under the `sqlite-logger` directory, you will find this sub-directory layout:

+ `sqlite-logger`
  + `.vscode` (contains VS Code configuration files)
  + `bin` (contains linked binaries)
  + `docs` (contains Doxygen configuration file)
  + `include` (contains SQLite Logger header files)
  + `logs` (contains log files)
  + `obj` (contains compiled object files)
  + `results` (contains unit test results)
  + `scripts` (contains build utility scripts)
  + `sqlite` (contains SQLite source code)
  + `src` (contains SQLite Logger source code)
  + `test` (contains SQLite logger unit test source code)

The compiled object files are in the `obj` directory, and the linked binaries are in a sub-directory under the `bin` directory. For example, with `Darwin/macOS` running on a 64-bit machine, the full linked debug binaries directory path is `bin\darwin\x64\Debug`. The detailed logs for the build process can be found in the `logs` directory - this includes the output of the compile/link cycle, as well as any code quality analyses.

If you want to clean and rebuild everything, use this command:

    ./build.sh --clean

You can choose whether to build debug or release versions in this way:

    ./build.sh --debug

or

    ./build.sh --release

If you'd like to build the code for profiling using `gprof`, you can do that via this command:

    ./build.sh --with-profiling

You can choose to build the SQLite Logger library as a static library or as a shared library. The build utility defaults to building a static library. Use this command to build a shared library:

    ./build.sh --with-shared-libs

If you're building SQLite Logger from a directory other than your `HOME` directory, you can specify that like this:

    ./build.sh --root-directory-path="/Users/foo/my-projects"

To cause the build utility to print out verbose information, use this command:

    ./build.sh --verbose

As described above, there are a variety of options available to customize the SQLite Logger build. You can easily combine multiple build options with a single command, like this:

    ./build.sh --clean --debug --verbose

#### Configuring the SQLite Logger Library
SQLite Logger uses a log entry cache to help with logging performance. The default size of this cache is 1024 entries.The size of this cache can be specified – here's an example of using a cache size of 512 entries:

    ./build.sh --with-log-entry-cache-size=512

#### Running Code Quality Checks
The build utility supports both `cppcheck` and `scan-build` code quality checkers (if available). You can specify running these test individually, like this:

    ./build.sh --analyze=cppcheck

Or

    ./build.sh --analyze=scan-build

To run both tests, use this command:

    ./build.sh --analyze=full

To skip the code quality checks, do this:

    ./build.sh --analyze=none

Using either of the code quality checkers will print out a summary the results of the check to the terminal window. Detailed output can be found in the `logs` directory.

#### Running the Unit Tests
The build utility makes use of `CUnit` to support unit testing. To build and run the SQLite Logger unit tests, use this command:

    ./build.sh --with-unit-testing

The output of the unit tests can be found in the `logs` directory in a file name `sqlite_logger_unit_test_log.xml`. Refer to the [`CUnit`](https://sourceforge.net/projects/cunit/) documentation for details on how to interpret this log. A summary of the unit test results will be printed to the terminal window.

#### Building an SDK
You can build an SDK consisting of the built SQLite Logger library, its header file, and associated documenation in this way:

    ./build.sh --with-sdk

An archive will be created in the `results` directory that has a name of the form `sqlite-logger-sdk-<library-type>-lib-<operating-system>-<architecture>-<configuration>.tar.gz`, where:

+ `<library-type>` is `static` or `shared`
+ `<operating-system>` is `darwin` or `linux`
+ `<architecture>` is `x64` or `arm64`
+ `<configuration>` is `Debug` or `Release`

For example, an SDK build for a static debug library on an Intel 64-bit macOS computer would have the name `sqlite-logger-sdk-static-lib-darwin-x64-Debug.tar.gz`.

The contents of this archive look like this:

+ `bin`
  + `libsqlitelogger.a` or `libsqlitelogger.so`
+ `include`
  + `sqlite_logger.h`
+ `LICENSE`
+ `README.md`

## Example Usage
Shown below is a short program (with line numbers at the left) illustrating how to use SQLite Logger. 

     1  void main(void) {
     2      int32_t result = SL_Initialize("./my_log_file.sqlite3");
     3
     4      if (result == SL_RESULT_SUCCESS) 
     5          result = SL_SetLogLevel(eSL_LogLevel_Diagnostic);
     6
     7      if (result == SL_RESULT_SUCCESS)
     8          result = SL_Log("This is a log message",
     9                           eSL_LogLevel_Info,
    10                           __FILE__, __FUNCTION__, __LINE__,
    11                           "My tag", 
    12                           "Supplemental data goes here");
    13      else
    14          fprintf(stderr, "SL_Log failed with result %s.\n,
    15                  SL_ResultString(result));
    16      
    17      if (result == SL_RESULT_SUCCESS)
    18          result = SL_LOG_WARNING_MESSAGE("This is a warning message,
    19                                          "Another tag", NULL);
    20
    21      (void) SL_Terminate();
    22
    23      return 0;
    24 }

On line 2, SQLite Logger is initialized by calling `SL_Initialize` with the path where the log file will be created (if it doesn't already exist) and opened. Note that this call will *not* delete or overwrite any existing log file.

On line 5, `SL_SetLogLevel` is called to set the SQLite Logger log level to `eSL_LogLevel_Diagnostic`. This indicates that *all* log messages generated by `SL_Log` should be logged to the log file. The default log level for SQLite Logger is `eSL_LogLevel_Info`.

On line 8, `SL_Log` is called to log a message with a level of `eSL_LogLevel_Info`.

On line 14, any result code that is not `SL_RESULT_SUCCESS` is handled by printing a result string to `stderr`. The string describing the result code is retrieved by calling `SL_ResultString`.

On line 18, the helper macro `SL_LOG_WARNING_MESSAGE` is used to log a warning message to the log file.

On line 21, `SL_Terminate` is called to close the logging session, including closing the connection to the log file.

## API Reference
The [`sqlite_logger.h`](./include/sqlite_logger.h) header file is extensively documented, and is the best source of information regarding usage. An API reference can be generated from the [Doxygen](https://www.doxygen.nl/index.html) comments in the [`sqlite_logger.h`](./include/sqlite_logger.h) header file using the build utility:

    ./build.sh --with-documentation

This will build an HTML version of the documentation (found in `docs\html`), and automatically open a browser window pointing to the `index.html` file.

---
Copyright © 2022 Unthinkable Research LLC. All rights reserved.