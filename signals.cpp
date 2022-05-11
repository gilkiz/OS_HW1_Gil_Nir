#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <sys/wait.h>

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
    return; //meaning there is no process in the FG
  }

  else
  {
    kill(current_fg_process,19);

    JobsList::JobEntry* job = smash.GetJobsList()->getJobByPID(current_fg_process);
    if (job == nullptr)
    {
      smash.GetJobsList()->addJob(smash.getCurrentFgCommand(),current_fg_process, true);
    }
    else
    {
      job->setTime();
      if(!job->getJobIsStopped())
      {
        job->SwitchIsStopped();
      }
    }

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

void alarmHandler(int sig_num) 
{
  SmallShell &smash = SmallShell::getInstance();
  pid_t pid = smash.getAlarmList()->getAndRemoveLastAlarm();
  if(pid == -1) return;
  if(smash.getSmashPid() == pid)
  {
    std::cout << "smash: got an alarm" << std::endl;
    return;
  }
  else if(smash.GetJobsList()->getJobByPID(pid) == nullptr && smash.getCurrentFgPid() != pid)
  {
    waitpid(pid, nullptr, WNOHANG);
    std::cout << "smash: got an alarm" << std::endl;
    return;
  }
  else
  {
    SYS_CALL(kill(pid, SIGKILL), "kill");
    waitpid(pid, nullptr, WNOHANG);
    string cmd_line;
    if(smash.getCurrentFgPid() != pid)
    {
      cmd_line = smash.GetJobsList()->getJobByPID(pid)->GetCmdLine();
      smash.GetJobsList()->removeJobByPid(pid);
    }
    else
    {
      cmd_line = smash.getCurrentFgCommand()->GetCmdLine();
    }
    std::cout << "smash: got an alarm" << std::endl << "smash: " << cmd_line << " timed out!" << std::endl;
  }
}

