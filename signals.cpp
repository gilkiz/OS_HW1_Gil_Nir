#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"



using namespace std;

#define SYS_CALL(syscall, name)                         \
  do                                                    \
  {                                                     \
    if(syscall == -1)                                    \
    {                                                   \
      string er = string("smash error: ") + string(name) + string(" failed");   \
      perror((char*)er.c_str());                              \
      return;                                           \
    }                                                   \
  } while (0)



void ctrlZHandler(int sig_num) {
  std::cout << "smash: got ctrl-Z" << std::endl;
  SmallShell &smash = SmallShell::getInstance();
  int current_fg_process = smash.getCurrentFgPid();

  if(current_fg_process == -1)
  {
    exit(0);
    return; //meaning there is no process in the FG
  }

  else
  {
    smash.GetJobsList()->addJob(smash.getCurrentFgCommand(), current_fg_process);
    kill(current_fg_process,19);
    smash.setCurrentFgPid(-1);
    smash.setCurrentFgCommand(NULL);
    std::cout << "smash: process " << current_fg_process << " was stopped" << std::endl;
  }
}

void ctrlCHandler(int sig_num) {
  std::cout << "smash: got ctrl-C" << std::endl;
  SmallShell &smash = SmallShell::getInstance();
  int current_fg_process = smash.getCurrentFgPid();

  if(current_fg_process == -1)
  {
    return; //meaning there is no process in the FG
  }
  else
  {
    kill(current_fg_process,9);
    smash.setCurrentFgPid(-1);
    smash.setCurrentFgCommand(NULL);
    std::cout << "smash: process " << current_fg_process << " was killed" << std::endl;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

