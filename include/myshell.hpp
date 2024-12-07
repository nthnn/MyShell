/*
 * Copyright (c) 2024 - Nathanne Isip
 * This file is part of MyShell.
 * 
 * N8 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * 
 * N8 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with N8. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file MyShell.hpp
 * @author [Nathanne Isip](https://github.com/nthnn)
 * @brief Declaration of the MyShell class for managing shell processes
 *        and interprocess communication.
 *
 * This header defines the MyShell class, which provides an interface
 * for creating and interacting with shell processes. It supports both
 * Windows and Unix-like platforms, enabling platform-specific process
 * handling.
 */
#ifndef MYSHELL_HPP
#define MYSHELL_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <fcntl.h>
#   include <signal.h>
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <unistd.h>
#endif

/**
 * @class MyShell
 * @brief Provides functionality to interact with and control
 *        shell processes.
 *
 * MyShell facilitates spawning a shell process, sending input
 * to it, and reading its output and error streams. The class
 * is designed to be non-copyable and manages resources specific
 * to the target platform.
 */
class MyShell {
public:
    /**
     * @brief Constructs a MyShell instance and starts the specified shell process.
     * @param command The command to execute in the shell.
     */
    MyShell(std::string command);

    /**
     * @brief Destructor that ensures the process is
     *        terminated and resources are released.
     */
    ~MyShell();

    // Deleted copy constructor and assignment operator
    // to ensure uniqueness of process ownership.
    MyShell(const MyShell&) = delete;
    MyShell& operator=(const MyShell&) = delete;

    /**
     * @brief Reads from the process's standard output stream.
     * @return A string containing the output read from the process.
     */
    std::string readShellOutputStream();

    /**
     * @brief Reads from the process's standard error stream.
     * @return A string containing the error output read from the process.
     */
    std::string readShellErrorStream();

    /**
     * @brief Writes input to the process's standard input stream.
     * @param input The string to write to the process.
     */
    void writeToShell(std::string input);

    /**
     * @brief Forces the process to terminate.
     *
     * This function sends a termination signal to the process, ensuring that it exits.
     */
    void forceExit();

    /**
     * @brief Checks if the process has exited.
     * @return True if the process has exited; otherwise, false.
     */
    bool hasExited();

    /**
     * @brief Retrieves the exit code of the process.
     * @return The exit code of the process, or -1 if the process is still running.
     */
    int exitCode();

    /**
     * @brief Retrieves the ID of the process.
     * @return The process ID.
     */
    int processId();

private:
    // Process state variables
    bool procHasExited; ///< Flag indicating if the process has exited.
    bool stopSignal;    ///< Signal to stop reading threads.
    int procExitCode;   ///< Exit code of the process.

    // Buffers and synchronization mechanisms
    std::string outputBuffer; ///< Buffer for storing standard output data.
    std::string errorBuffer;  ///< Buffer for storing standard error data.
    std::mutex outputMutex;   ///< Mutex for synchronizing access to the output buffer.
    std::mutex errorMutex;    ///< Mutex for synchronizing access to the error buffer.

    // Threads for reading process output
    std::thread outputReader; ///< Thread for reading standard output.
    std::thread errorReader;  ///< Thread for reading standard error.

    // Private methods
    void outputReaderThread(); ///< Thread function for reading standard output.
    void errorReaderThread();  ///< Thread function for reading standard error.

    #ifdef _WIN32
    // Windows-specific members
    HANDLE procHandle;           ///< Handle to the process.
    HANDLE inputWriteHandle;     ///< Handle to the write end of the input pipe.
    HANDLE outputReadHandle;     ///< Handle to the read end of the output pipe.
    HANDLE errorReadHandle;      ///< Handle to the read end of the error pipe.
    DWORD procId;                ///< ID of the process.

    // Windows-specific methods
    void createWindowsProcess(const std::string& command); ///< Creates a process on Windows.
    void readFromPipe(HANDLE pipe, std::string& buffer, std::mutex& mutex); ///< Reads data from a pipe.

    #else
    // Unix-specific members
    pid_t procId;               ///< ID of the process.
    int inputPipe[2];           ///< Pipe for process input.
    int outputPipe[2];          ///< Pipe for process output.
    int errorPipe[2];           ///< Pipe for process error.

    // Unix-specific methods
    void createUnixProcess(const std::string& command); ///< Creates a process on Unix-like systems.
    void readFromPipe(int pipe, std::string& buffer, std::mutex& mutex); ///< Reads data from a pipe.
    void closeUnixPipes(); ///< Closes Unix pipes.

    #endif

    static const int bufferSize = 4096; ///< Size of the buffer for reading process streams.
};

#endif // MYSHELL_HPP
