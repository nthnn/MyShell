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

#include <chrono>
#include <cstring>
#include <myshell.hpp>
#include <stdexcept>
#include <system_error>

using namespace std::chrono_literals;

MyShell::MyShell(std::string command) :
    procHasExited(false),
    procExitCode(0),
    stopSignal(false)
{
    #ifdef _WIN32
    this->createWindowsProcess(command);
    #else
    this->createUnixProcess(command);
    #endif

    this->outputReader = std::thread(
        &MyShell::outputReaderThread,
        this
    );

    this->errorReader = std::thread(
        &MyShell::errorReaderThread,
        this
    );
}

MyShell::~MyShell() {
    this->stopSignal = true;

    if(this->outputReader.joinable())
        this->outputReader.join();
    if(this->errorReader.joinable())
        this->errorReader.join();

    #ifdef _WIN32
    
    if(this->procHandle)
        CloseHandle(this->procHandle);
    if(this->inputWriteHandle)
        CloseHandle(this->inputWriteHandle);
    if(this->outputReadHandle)
        CloseHandle(this->outputReadHandle);
    if(this->errorReadHandle)
        CloseHandle(this->errorReadHandle);

    #else
    this->closeUnixPipes();
    #endif
}

#ifdef _WIN32
void MyShell::createWindowsProcess(const std::string& command) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE inputRead, outputWrite, errorWrite;
    if(!CreatePipe(&inputRead, &this->inputWriteHandle, &saAttr, 0) ||
        !CreatePipe(&this->outputReadHandle, &this->outputWrite, &saAttr, 0) ||
        !CreatePipe(&this->errorReadHandle, &this->errorWrite, &saAttr, 0))
        throw std::system_error(
            GetLastError(),
            std::system_category(),
            "Failed to create pipes"
        );

    SetHandleInformation(this->inputWriteHandle, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(this->outputReadHandle, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(this->errorReadHandle, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO siStartInfo;
    PROCESS_INFORMATION piProcInfo;

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdInput = inputRead;
    siStartInfo.hStdOutput = outputWrite;
    siStartInfo.hStdError = errorWrite;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    if(!CreateProcessA(
        NULL,
        const_cast<LPSTR>(command.c_str()),
        NULL, NULL, TRUE,
        CREATE_NO_WINDOW,
        NULL, NULL,
        &siStartInfo, &piProcInfo
    )) throw std::system_error(
        GetLastError(),
        std::system_category(),
        "Failed to create process"
    );

    this->procHandle = piProcInfo.hProcess;
    this->procId = piProcInfo.dwProcessId;

    CloseHandle(piProcInfo.hThread);
    CloseHandle(inputRead);
    CloseHandle(outputWrite);
    CloseHandle(errorWrite);
}

void MyShell::readFromPipe(HANDLE pipe, std::string& buffer, std::mutex& mutex) {
    std::vector<char> tempBuffer(this->bufferSize);
    DWORD bytesRead;

    while(!stopSignal) {
        if(!PeekNamedPipe(pipe, NULL, 0, NULL, &bytesRead, NULL))
            return;
        
        if(bytesRead > 0) {
            if(ReadFile(
                pipe,
                tempBuffer.data(),
                this->bufferSize - 1,
                &bytesRead,
                NULL
            ) && bytesRead > 0) {
                tempBuffer[bytesRead] = '\0';

                std::lock_guard<std::mutex> lock(mutex);
                buffer += tempBuffer.data();
            }
        }
        else std::this_thread::sleep_for(10ms);
    }
}

#else

void MyShell::createUnixProcess(const std::string& command) {
    if(pipe(this->inputPipe) == -1 ||
        pipe(this->outputPipe) == -1 ||
        pipe(this->errorPipe) == -1)
        throw std::system_error(
            errno,
            std::system_category(),
            "Failed to create pipes"
        );

    this->procId = fork();
    if(this->procId == -1) {
        this->closeUnixPipes();

        throw std::system_error(
            errno,
            std::system_category(),
            "Failed to fork process"
        );
    }

    if(this->procId == 0) {
        dup2(this->inputPipe[0], STDIN_FILENO);
        dup2(this->outputPipe[1], STDOUT_FILENO);
        dup2(this->errorPipe[1], STDERR_FILENO);

        close(this->inputPipe[0]);
        close(this->inputPipe[1]);
        close(this->outputPipe[0]);
        close(this->outputPipe[1]);
        close(this->errorPipe[0]);
        close(this->errorPipe[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), NULL);
        exit(1);
    }

    close(this->inputPipe[0]);
    close(this->outputPipe[1]);
    close(this->errorPipe[1]);

    fcntl(this->outputPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(this->errorPipe[0], F_SETFL, O_NONBLOCK);
}

void MyShell::closeUnixPipes() {
    close(this->inputPipe[1]);
    close(this->outputPipe[0]);
    close(this->errorPipe[0]);
}

void MyShell::readFromPipe(int pipe, std::string& buffer, std::mutex& mutex) {
    std::vector<char> tempBuffer(this->bufferSize);
    ssize_t bytesRead;

    while(!stopSignal) {
        bytesRead = read(
            pipe,
            tempBuffer.data(),
            this->bufferSize - 1
        );

        if(bytesRead > 0) {
            tempBuffer[bytesRead] = '\0';

            std::lock_guard<std::mutex> lock(mutex);
            buffer += tempBuffer.data();
        }
        else if(bytesRead == -1 && errno == EAGAIN)
            std::this_thread::sleep_for(10ms);
    }
}

#endif

void MyShell::outputReaderThread() {
    #ifdef _WIN32
    this->readFromPipe(
        this->outputReadHandle,
        this->outputBuffer,
        this->outputMutex
    );
    #else
    this->readFromPipe(
        this->outputPipe[0],
        this->outputBuffer,
        this->outputMutex
    );
    #endif
}

void MyShell::errorReaderThread() {
    #ifdef _WIN32

    this->readFromPipe(
        this->errorReadHandle,
        this->errorBuffer,
        this->errorMutex
    );

    #else

    this->readFromPipe(
        this->errorPipe[0],
        this->errorBuffer,
        this->errorMutex
    );

    #endif
}

std::string MyShell::readShellOutputStream() {
    std::lock_guard<std::mutex> lock(this->outputMutex);
    std::string result = std::move(this->outputBuffer);

    outputBuffer.clear();
    return result;
}

std::string MyShell::readShellErrorStream() {
    std::lock_guard<std::mutex> lock(this->errorMutex);
    std::string result = std::move(this->errorBuffer);

    this->errorBuffer.clear();
    return result;
}

void MyShell::writeToShell(std::string input) {
    #ifdef _WIN32

    DWORD bytesWritten;
    if(!WriteFile(
        this->inputWriteHandle,
        input.c_str(),
        static_cast<DWORD>(input.length()),
        &bytesWritten,
        NULL
    )) throw std::system_error(
        GetLastError(),
        std::system_category(),
        "Failed to write to process"
    );

    #else

    if(write(
        this->inputPipe[1],
        input.c_str(),
        input.length()
    ) == -1)
        throw std::system_error(
            errno,
            std::system_category(),
            "Failed to write to process"
        );

    #endif
}

void MyShell::forceExit() {
    #ifdef _WIN32
    TerminateProcess(procHandle, 1);
    #else
    kill(this->procId, SIGTERM);
    #endif

    this->procHasExited = true;
    this->procExitCode = 1;
}

bool MyShell::hasExited() {
    if(this->procHasExited) {
        this->readShellOutputStream();
        this->readShellErrorStream();
        return true;
    }

    #ifdef _WIN32

    DWORD exitCode;
    if(GetExitCodeProcess(this->procHandle, &exitCode)) {
        if(exitCode != STILL_ACTIVE) {
            procHasExited = true;
            procExitCode = static_cast<int>(exitCode);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            this->readShellOutputStream();
            this->readShellErrorStream();
        }
    }

    #else

    int status;
    pid_t result = waitpid(
        this->procId,
        &status,
        WNOHANG
    );

    if(result > 0) {
        this->procHasExited = true;
        this->procExitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        this->readShellOutputStream();
        this->readShellErrorStream();
    }

    #endif

    return this->procHasExited;
}

int MyShell::exitCode() {
    if(!this->procHasExited)
        this->hasExited();

    return this->procExitCode;
}

int MyShell::processId() {
    return static_cast<int>(this->procId);
}
