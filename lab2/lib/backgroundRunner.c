#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

int backgroundRunner(const char* command) {
    #ifdef _WIN32
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcess(NULL, (char *)command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exit_code;
        if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return -1;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (int)exit_code;
    #else
        pid_t pid = fork();
        int status;
        if (pid == 0) {
            execlp(command, command, (char *)NULL);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return -1;
            }
        } else {
            perror("fork");
            return -1;
        }
    #endif
}
