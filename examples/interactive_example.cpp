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