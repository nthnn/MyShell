# MyShell: Cross-Platform Shell Interaction Library

![Build CI for Linux](https://github.com/nthnn/MyShell/actions/workflows/linux_ci.yml/badge.svg)
![Build CI for MacOS](https://github.com/nthnn/MyShell/actions/workflows/macos_ci.yml/badge.svg)
![Build CI for Windows](https://github.com/nthnn/MyShell/actions/workflows/windows_ci.yml/badge.svg)

MyShell is a modern C++ library that provides a robust, cross-platform interface for shell process interaction. It allows seamless execution and interaction with shell commands across Windows, Linux, and macOS operating systems.

- **Cross-Platform Compatibility**: Works consistently across Windows, Linux, and macOS
- **Bidirectional Communication**: Full support for stdin, stdout, and stderr streams
- **Real-time Output Processing**: Non-blocking I/O with efficient output buffering
- **Process Management**: Monitor process status and retrieve exit codes
- **Resource Safety**: RAII-compliant with automatic resource cleanup
- **Thread Safety**: Thread-safe output handling with mutex protection
- **Error Handling**: Comprehensive error reporting using C++ exceptions

## Usage

Simply copy `myshell.hpp` and `myshell.cpp` into your project and include them in your build system.

### Basic Example

```cpp
#include <chrono>
#include <iostream>
#include <myshell.hpp>

int main() {
    try {
        // Create shell instance withcommand
        MyShell shell(
            #ifdef _WIN32
            "dir"
            #else
            "ls -ral"
            #endif
        );

        // Give some time for output to be collected
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Read output
        std::string output = shell.readShellOutputStream();
        std::string error = shell.readShellErrorStream();

        // Wait for process completion
        while(!shell.hasExited());

        // Print results
        std::cout << "Exit Code: " << shell.exitCode() << std::endl;
        std::cout << "Output:\n" << output << std::endl;

        if(!error.empty())
            std::cout << "Error:\n" << error << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### Interactive Example

```cpp
#include <iostream>
#include <myshell.hpp>
#include <thread>

int main() {
    try {
        // Create shell instance withcommand
        MyShell shell("TERM=dumb vim -u NONE -n test.txt");

        shell.writeToShell("i");        // Insert mode
        shell.writeToShell("Hi");       // Write the text
        shell.writeToShell("\u001b");   // Exit insert mode (ESC key)
        shell.writeToShell(":wq");      // Write command to save and quit
        shell.writeToShell("\n");

        // Wait for process completion
        while(!shell.hasExited());

        std::cout << "Process exited with code: " << shell.exitCode() << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

## API Reference

### Constructor

```cpp
MyShell(std::string uuid, std::string command);
```

- **command**: Command to execute

### Methods

```cpp
std::string readShellOutputStream()
```

Returns accumulated stdout data and clears the internal buffer. *Non-blocking operation*

```cpp
std::string readShellErrorStream()
```

Returns accumulated stderr data and clears the internal buffer. *Non-blocking operation*

```cpp
void writeToShell(std::string input)
```

Writes input to the shell's stdin. Throws std::system_error if write fails.

```cpp
void forceExit()
```

Forcefully terminates the shell process. Sets exit code to 1.

```cpp
bool hasExited()
```

Returns true if the process has exited. Updates internal exit code.

```cpp
int exitCode()
```

Returns the process exit code. Returns 0 if process is still running.

```cpp
int processId()
```

Returns the system process ID

## Platform-Specific Considerations

### Windows OS Considerations

- Uses Windows API (CreateProcess, pipes)
- Supports both console and GUI applications
- UTF-8 encoding support

### Linux/macOS Considerations

- Uses POSIX API (fork, exec, pipes)
- Full terminal support
- Signal handling support

### Error Handling

The library uses C++ exceptions for error reporting:

- `std::system_error` for system-related errors.
- `std::runtime_error` for general errors.
- All error messages include detailed descriptions

### Performance Considerations

- Non-blocking I/O operations
- Efficient buffer management
- Minimal CPU usage while waiting for output
- Thread-safe output handling

### Thread Safety

- Safe for concurrent reading of output/error streams
- Safe for concurrent writing to input stream
- Thread-safe process status checking

## Common Issues and Solutions

### Process Hanging

If the process seems to hang, ensure you're:

- Reading both stdout and stderr streams
- Not filling up the output buffers
- Properly handling interactive processes

### Memory Usage

The library efficiently manages memory by:

- Using move semantics for string handling
- Clearing buffers after reading
- Automatic resource cleanup

### Platform-Specific Issues

1. Windows:
    - Use proper line endings (\r\n)
    - Consider code page settings
    - Handle Unicode properly

2. Linux/macOS:
    - Consider terminal settings
    - Handle signals appropriately
    - Check file permissions

## Contributing

Contributions are welcome! Feel free to submit a pull request or open an issue for bugs or feature requests.

## License

MyShell is licensed under the [GNU General Public License v3](LICENSE). You are free to use, modify, and distribute this library under the terms of the license.