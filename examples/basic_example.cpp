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
