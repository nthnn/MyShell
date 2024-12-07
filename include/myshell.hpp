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

class MyShell {
public:
    MyShell(std::string command);
    ~MyShell();

    MyShell(const MyShell&) = delete;
    MyShell& operator=(const MyShell&) = delete;

    std::string readShellOutputStream();
    std::string readShellErrorStream();

    void writeToShell(std::string input);

    void forceExit();
    bool hasExited();

    int exitCode();
    int processId();

private:
    bool procHasExited;
    bool stopSignal;
    int procExitCode;

    std::string outputBuffer;
    std::string errorBuffer;
    std::mutex outputMutex;
    std::mutex errorMutex;

    std::thread outputReader;
    std::thread errorReader;

    void outputReaderThread();
    void errorReaderThread();

    #ifdef _WIN32
    HANDLE procHandle;
    HANDLE inputWriteHandle;
    HANDLE outputReadHandle;
    HANDLE errorReadHandle;
    DWORD procId;
    
    void createWindowsProcess(const std::string& command);
    void readFromPipe(HANDLE pipe, std::string& buffer, std::mutex& mutex);

    #else

    pid_t procId;
    int inputPipe[2];
    int outputPipe[2];
    int errorPipe[2];
    
    void createUnixProcess(const std::string& command);
    void readFromPipe(int pipe, std::string& buffer, std::mutex& mutex);
    void closeUnixPipes();

    #endif

    static const int bufferSize = 4096;
};

#endif
