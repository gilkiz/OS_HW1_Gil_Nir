#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) 
{
    struct sigaction alarm;
    alarm.sa_handler = alarmHandler;
    sigemptyset(&alarm.sa_mask);
    alarm.sa_flags = SA_RESTART;


    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) 
    {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) 
    {
        perror("smash error: failed to set ctrl-C handler");
    }
    if(sigaction(SIGALRM, &alarm, nullptr) == -1)
    {
        perror("smash error: failed to set alarm handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while (true)
    {
        std::cout << smash.GetName();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.GetJobsList()->removeFinishedJobs();
        if(!(cmd_line.empty()))
            smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}