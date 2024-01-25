Project Description - Unix Shell

Overview:
This project involves building a Unix shell, named dash (short for DAllas SHell). The shell functions as a command-line interpreter (CLI), creating child processes to execute commands entered by the user. The primary objectives include enhancing familiarity with the Unix/Linux programming environment, understanding process creation and management, and gaining exposure to shell functionality.

Program Specifications:

    Basic Shell: dash
        Operates interactively with a prompt (dash>), parsing and executing commands until the user types exit.
        Supports batch mode, reading commands from a specified file.
        Implements a basic shell structure with the ability to parse commands and run corresponding programs.

Structure:

    Basic Shell
        Runs in a while loop, prompting for user input, executing commands, and repeating until the user enters "exit."
        Uses getline() for reading lines of input and strtok() for parsing.
        Utilizes fork(), exec(), and wait()/waitpid() for process management.

    Paths
        Specifies a path variable to determine directories to search for executables.
        Implements a search path with the ability to check executable existence using the access() system call.

    Built-in Commands
        Implements built-in commands: exit, cd, and path.
            exit: Calls the exit system call with 0 as a parameter.
            cd: Changes directories using the chdir() system call.
            path: Sets the search path; overwrites the old path with the newly specified one.

    Redirection
        Allows redirection of standard output and standard error using the > character.
        Redirects output to a specified file, overwriting if it exists.

    Parallel Commands
        Enables the execution of parallel commands using the ampersand operator (&).

Program Errors:

    Prints a consistent error message to stderr for any encountered error.
    Exits the shell by calling exit(1) for specific errors (e.g., multiple files or bad batch file).

Note:
Ensure proper error handling and adherence to project-specific syntax rules.
