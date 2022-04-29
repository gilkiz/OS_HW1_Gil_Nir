#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <fcntl.h>

using std::string;
using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

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

#define SYS_CALL_PTR(syscall, name)                         \
  do                                                    \
  {                                                     \
    if(syscall == NULL)                                    \
    {                                                   \
      string er = string("smash error: ") + string(name) + string(" failed");   \
      perror((char*)er.c_str());                              \
      return;                                           \
    }                                                   \
  } while (0)

#define SYS_CALL_AND_RESTORE_FD(syscall, name, old_fd, new_fd)                         \
  do                                                    \
  {                                                     \
    if(syscall == -1)                                    \
    {                                                   \
      string er = string("smash error: ") + string(name) + string(" failed");   \
      perror((char*)er.c_str());                    \
      SYS_CALL(dup2(old_fd, new_fd), "dup2");                           \
      return;                                           \
    }                                                   \
  } while (0)

#define SYS_CALL_UTIME(syscall, name, filename, utimebuf_for_utime)                         \
  do                                                    \
  {                                                     \
    if(syscall(filename, utimebuf_for_utime) == -1)                                    \
    {                                                   \
      string er = string("smash error: ") + string(name) + string(" failed");   \
      perror((char*)er.c_str());                              \
      return;                                           \
    }                                                   \
  } while (0)


const std::string WHITESPACE = " \n\r\t\f\v";

// * This returns the string without spaces/enters in the beggining 
string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

// * This returns the string without spaces/enters in the end
string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// * This combines the upper two and returns the string without spaces 
// * in the beggining and in the end
string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

// * Reads the next command from cmd, trim's it and inserts to char** args
int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = new char[s.length() + 1];
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

// * returns if its a background command
bool _isBackgroundCommand(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

// * Only removes & from end of command if exists
void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() : shellname("smash> "), last_directory(NULL), jobs_list(new JobsList()), smash_pid(getpid()){};

SmallShell::~SmallShell() 
{
  delete jobs_list;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) 
{
  char *command_line = new char[COMMAND_ARGS_MAX_LENGTH];
  if(strcpy(command_line, cmd_line) == NULL) // error
  if (_isBackgroundCommand(command_line))
    _removeBackgroundSign(command_line);

  string cmd_s = _trim(string(command_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if(cmd_s.find(">") != string::npos || cmd_s.find(">>") != string::npos)
    return new RedirectionCommand(cmd_line);
  else if(cmd_s.find("|") != string::npos || cmd_s.find("|&") != string::npos)
    return new PipeCommand(cmd_line);
  else if(firstWord.compare("chprompt") == 0)
    return new ChangePromptCommand(cmd_line, &(this->shellname));
  else if (firstWord.compare("pwd") == 0)
    return new GetCurrDirCommand(cmd_line);
  else if (firstWord.compare("showpid") == 0)
    return new ShowPidCommand(cmd_line);
  else if (firstWord.compare("cd") == 0)
    return new ChangeDirCommand(cmd_line, this->GetLastDirectory());
  else if (firstWord.compare("jobs") == 0)
    return new JobsCommand(cmd_line, this->jobs_list);
  else if (firstWord.compare("kill") == 0)
    return new KillCommand(cmd_line, this->jobs_list);
  else if (firstWord.compare("fg") == 0)
    return new ForegroundCommand(cmd_line, this->jobs_list);
  else if (firstWord.compare("bg") == 0)
    return new BackgroundCommand(cmd_line, this->jobs_list);
  else if (firstWord.compare("quit") == 0)
    return new QuitCommand(cmd_line, this->jobs_list);
  else if(firstWord.compare("tail") == 0)
    return new TailCommand(cmd_line);
  else if(firstWord.compare("touch") == 0)
    return new TouchCommand(cmd_line);
  else
    return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)

  Command *cmd = CreateCommand(cmd_line);
  if(cmd)
  {
    cmd->execute();
    delete cmd;
  }
}

std::string SmallShell::GetName()
{
  return this->shellname;
}

char **SmallShell::GetLastDirectory()
{
  return &(this->last_directory);
}

JobsList *SmallShell::GetJobsList()
{
  return this->jobs_list;
}

int SmallShell::getSmashPid()
{
  return this->smash_pid;
}

/*======================================================*/
/*================== Commands Methods ==================*/
/*======================================================*/

// Command

Command::Command(const char *cmd_line)
{
  //int size = _numOfStringsInArray(cmd_line);
  this->cmd_line = cmd_line;
  char *command_line = new char[COMMAND_ARGS_MAX_LENGTH];
  if(strcpy(command_line, cmd_line) == NULL) // error
  _removeBackgroundSign(command_line);
  this->args = new char *[COMMAND_MAX_ARGS + 1];
  this->size_args = _parseCommandLine(command_line, this->args);
  delete command_line;
}

Command::~Command()
{
  //for (int i = 0; i < this->size_args; i++)
  //  free(this->args[i]);
  delete[] this->args;
  this->size_args = 0;
}
string Command::GetCmdLine()
{
  return this->cmd_line;
}

// ChangePromptCommand, chprompt

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, string* shell_name) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->shell_name = shell_name;
}

void ChangePromptCommand::execute()
{
    if(this->size_args <= 1)
      *(this->shell_name) = "smash> ";
    else
      *(this->shell_name) = string(this->args[1]) + "> ";
}

// ShowPidCommand, showpid

void ShowPidCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  std::cout << "smash pid is " << smash.getSmashPid() << std::endl;
}

// GetCurrDirCommand, pwd

void GetCurrDirCommand::execute()
{
  char root[PATH_MAX];
  SYS_CALL_PTR(getcwd(root, sizeof(root)), "getcwd");
  //if (getcwd(root, sizeof(root)) != NULL)
  std::cout << root << std::endl;
}

// ChangeDirCommand, cd

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->last_directory = plastPwd;
}

void ChangeDirCommand::execute()
{
  if(this->size_args == 2)
  {
    char before_change_pwd[PATH_MAX];
    SYS_CALL_PTR(getcwd(before_change_pwd, sizeof(before_change_pwd)), "getcwd");
    char *cd;
    if (sizeof(this->args[1]) == 1 && (this->args[1])[0] == '-')
    {
      if(this->last_directory == NULL) 
      {
        std::cout << "smash error: cd: OLDPWD not set" << std::endl;
        return;
      }
      cd = *(this->last_directory);
    }
    else
      cd = args[1];
    SYS_CALL(chdir(cd), "chdir");
    *(this->last_directory) = before_change_pwd;
  }
  else if(this->size_args > 2)
  {
    std::cout << "smash error: cd: too many arguments" << std::endl;
  }
}

// JobsCommand, jobs

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->jobs = jobs;
}

void JobsCommand::execute()
{
  this->jobs->removeFinishedJobs();
  this->jobs->printJobsList();
}

// KillCommand, kill

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->jobs = jobs;
}

void KillCommand::execute()
{
  if(this->size_args != 3 || (this->args[1])[0] != '-')
  {
    std::cout << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  int sig = atoi((string(this->args[1])).substr(1).c_str());
  int jobid = atoi(this->args[2]);
  if (sig == 0 || jobid == 0)
  {
    std::cout << "smash error: kill: invalid arguments" << std::endl;
    return;
  }
  JobsList::JobEntry *job = this->jobs->getJobById(jobid);
  if(job == nullptr)
  {
    std::cout << "smash error: kill: job-id " << jobid << " does not exist" << std::endl;
    return;
  }
  int pid = job->getPID();
  SYS_CALL(kill(pid, sig), "kill");
  std::cout << "signal number " << sig << " was sent to pid " << pid << std::endl;
}

// ForegroundCommand, fg

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line), jobs(jobs){};

void ForegroundCommand::execute()
{
  JobsList::JobEntry *job;
  int jobid = 0;
  if (this->size_args == 2)
  {
    jobid = atoi(this->args[1]);
    if(jobid == 0)
    {
      std::cout << "smash error: fg: invalid arguments" << std::endl;
      return;
    }
    job = this->jobs->getJobById(jobid);
    if(!job)
    {
      std::cout << "smash error: fg: job-id " << jobid << " does not exist" << std::endl;
      return;
    }
  }
  else if(this->size_args == 1)
  {
    job = this->jobs->getLastJob(&jobid);
    if(!job)
    {
      std::cout << "smash error: fg: jobs list is empty" << std::endl;
      return;
    }
  }
  else
  {
    std::cout << "smash error: fg: invalid arguments" << std::endl;
    return;
  }
  if(job->IsStopped())
    job->SwitchIsStopped();
  std::cout << job->GetCmdLine() << ": " << job->getPID() << std::endl;
  int status = 0;
  waitpid(job->getPID(), &status, WUNTRACED);
  if(WIFEXITED(status) || WIFSIGNALED(status))
    this->jobs->removeJobById(jobid);
}

// BackgroundCommand, bg

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){};

void BackgroundCommand::execute()
{
  JobsList::JobEntry *job;
  int jobid = 0;
  if (this->size_args == 2)
  {
    jobid = atoi(this->args[1]);
    if(jobid == 0)
    {
      std::cout << "smash error: bg: invalid arguments" << std::endl;
      return;
    }
    job = this->jobs->getJobById(jobid);
    if(!job)
    {
      std::cout << "smash error: bg: job-id " << jobid << " does not exist" << std::endl;
      return;
    }
    if(!(job->IsStopped()))
    {
      std::cout << "smash error: bg: job-id " << jobid <<
       " is already running in the background" << std::endl;
      return;
    }
  }
  else if(this->size_args == 1)
  {
    job = this->jobs->getLastStoppedJob(&jobid);
    if(!job)
    {
      std::cout << "smash error: bg: there is no stopped jobs to resume" << std::endl;
      return;
    }
  }
  else
  {
    std::cout << "smash error: bg: invalid arguments" << std::endl;
    return;
  }
  job->SwitchIsStopped();
  std::cout << job->GetCmdLine() << ": " << job->getPID() << std::endl;
}

// QuitCommand, quit

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){};

void QuitCommand::execute()
{
  if(this->size_args > 1 && string(this->args[1]).compare("kill") == 0)
    this->jobs->killAllJobs();
  exit(0);
}


void TouchCommand::execute()
{
    if(this->size_args != 2)
    {
      std::cout << "smash error: touch: invalid arguments" << std::endl;
      return;
    }

    std::string file_name = std::string(this->args[0]);
    std::string time_details = std::string(this->args[1]);

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    
    strptime(this->args[1], "%s:%M:%H:%d:%m:%Y", &tm);
    
    struct utimbuf utimebuf_for_utime;
    utimebuf_for_utime.actime=mktime(&tm);
    utimebuf_for_utime.actime=modtime(&tm);

    SYS_CALL_UTIME(utime, "utime", args[0], utimebuf_for_utime);

    return;
}




/*=====================================================*/
/*=============JobsList & JobEntry Methods=============*/
/*=====================================================*/

JobsList::JobsList(){
  this->jobs = vector<JobsList::JobEntry>();
}

void JobsList::addJob(Command* cmd, pid_t pid , bool isStopped)
{
  int available_job_id = 1;
  this->getLastJob(&available_job_id);
  available_job_id++;
  JobsList::JobEntry new_job(cmd->GetCmdLine(), available_job_id, pid, isStopped);
  new_job.setTime();
  this->jobs.push_back(new_job);
}

void JobsList::printJobsList(){
  for (size_t i = 0; i < (this->jobs).size(); i++){
    time_t current_time;
    time(&current_time);
    double seconds_elapsed = difftime(current_time, jobs[i].GetInsertTime());
    if(jobs[i].IsStopped()){
      std::cout << "[" << jobs[i].getJobID() << "] "<< jobs[i].GetCmdLine() <<
        " : "<< jobs[i].getPID() << " " << seconds_elapsed << " secs" << " (stopped)" << std::endl;  
    }
    else
    {
      std::cout << "[" << jobs[i].getJobID() << "] "<< jobs[i].GetCmdLine() <<
      " : " << jobs[i].getPID() << " " << seconds_elapsed << " secs" << std::endl;
    }
  }
}

void JobsList::killAllJobs()
{ 
  std::cout << "smash: sending SIGKILL signal to " << this->jobs.size() << " jobs:" << std::endl;
  for(auto it: this->jobs)
  {
    SYS_CALL(kill(it.getPID(), SIGKILL), "kill");
  }
}

void JobsList::removeFinishedJobs()
{
  pid_t finished_pid;
  int status;      
  while((finished_pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    if(WIFEXITED(status) || WIFSIGNALED(status))
    {
      JobsList::JobEntry *job = this->getJobByPID(finished_pid);
      if(job)
        removeJobById(job->getJobID());
    }
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
  vector<JobsList::JobEntry>::iterator it = this->jobs.begin();
  for (; it != this->jobs.end(); ++it)
    if((*it).getJobID() == jobId)
      return &((*it));
  return nullptr;
}

JobsList::JobEntry *JobsList::getJobByPID(int jobPID)
{
  vector<JobsList::JobEntry>::iterator it = this->jobs.begin();
  for (; it != this->jobs.end(); ++it)
    if((*it).getPID() == jobPID)
      return &((*it));
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  vector<JobsList::JobEntry>::iterator it = this->jobs.begin();
  vector<JobsList::JobEntry>::iterator end = this->jobs.end();
  for (; it != end; ++it)
  {
    if(((*it).getJobID()) == jobId)
      (this->jobs).erase(it);
  }
}

void JobsList::removeJobByPid(pid_t pid)
{
  vector<JobsList::JobEntry>::iterator it = this->jobs.begin();
  vector<JobsList::JobEntry>::iterator end = this->jobs.end();
  for (; it != end; ++it)
  {
    if(((*it).getPID()) == pid)
      (this->jobs).erase(it);
  }
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){
  int j = (this->jobs).size();
  if(j == 0)
  {
    *lastJobId = 0;
    return nullptr;
  }
  JobsList::JobEntry* temp = &((this->jobs)[j-1]);
  *lastJobId = temp->getJobID();
  return temp;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{
  vector<JobsList::JobEntry>::reverse_iterator it = this->jobs.rbegin();
  vector<JobsList::JobEntry>::reverse_iterator end = this->jobs.rend();
  for (; it != end; ++it)
  {
    if((*it).IsStopped())
    {
      *jobId = (*it).getJobID();
      return &((*it));
    }
  }
  *jobId = 0;
  return nullptr;
}


//=========================Job Entry=============================

string JobsList::JobEntry::GetCmdLine()
{
  return this->cmd_line;
}
int JobsList::JobEntry::getJobID(){
  return this->job_id;
}
pid_t JobsList::JobEntry::getPID(){
  return this->pid;
}
bool JobsList::JobEntry::IsStopped()
{
  return this->is_stopped;
}
void JobsList::JobEntry::SwitchIsStopped()
{
  this->is_stopped = !(this->is_stopped);
  if(this->is_stopped)
  {
    SYS_CALL(kill(this->pid, SIGSTOP), "kill");
  }
  else
  {
    SYS_CALL(kill(this->pid, SIGCONT), "kill");
  }
}
time_t JobsList::JobEntry::GetInsertTime()
{
  return this->insert_time;
}
void JobsList::JobEntry::setTime(){
  time(&(this->insert_time));
}

/*============================================================*/
/*======================External Command======================*/
/*============================================================*/

void ExternalCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  pid_t pid = fork();
  if(pid < 0)
    perror("smash error: fork failed");
  else if(pid == 0) // son (external)
  {
    char* cmd_line = (char*)this->GetCmdLine().c_str();
    _removeBackgroundSign(cmd_line);
    SYS_CALL(setpgrp(), "setpgrp");
    execlp("/bin/bash", "bash", "-c", cmd_line, NULL);
    perror("smash error: execlp failed");
    exit(0);
  }
  else // father (smash)
  {
    if(_isBackgroundCommand(this->cmd_line))
      smash.GetJobsList()->addJob(this ,pid);
    else
    {
      int status;
      waitpid(pid, &status, WUNTRACED);
      if(WIFEXITED(status) || WIFSIGNALED(status))
        smash.GetJobsList()->removeJobByPid(pid);
    }
  }
}

/*===========================================================*/
/*====================Redirection Command====================*/
/*===========================================================*/

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line)
{
  if((string(this->cmd_line)).find(" > ") != string::npos)
    this->is_append = false;
  else this->is_append = true;

  char *command_line = new char[COMMAND_ARGS_MAX_LENGTH];
  if(strcpy(command_line, this->cmd_line) == NULL) // error
  if (_isBackgroundCommand(command_line))
    _removeBackgroundSign(command_line);
  string cmd_s = _trim(string(command_line));
  size_t i = cmd_s.find(" >> ");
  if(this->is_append)
  {
    this->output_file = cmd_s.substr(i + 4).c_str();
  }
  else
  {
    i = cmd_s.find(" > ");
    this->output_file = cmd_s.substr(i + 3).c_str();
  }
  this->command = cmd_s.substr(0, i).c_str();
}

void RedirectionCommand::execute()
{
  SmallShell& smash = SmallShell::getInstance();
  int std_out = dup(STDOUT_FILENO);
  if(std_out == -1)
  {
    perror("smash error: dup failed");
    return;
  }
  int fd_file;
  if(this->is_append) // >>
    fd_file = open(this->output_file, O_RDWR | O_CREAT | O_APPEND, 0666);
  else                // >
    fd_file = open(this->output_file, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if(fd_file == -1)
    {
      perror("smash error: open failed");
      return;
    }
  
  SYS_CALL_AND_RESTORE_FD(close(STDOUT_FILENO), "close", std_out, STDOUT_FILENO);
  SYS_CALL_AND_RESTORE_FD(dup2(fd_file, STDOUT_FILENO), "dup2", std_out, STDOUT_FILENO);
  
  smash.executeCommand(this->command);
  SYS_CALL(dup2(std_out, STDOUT_FILENO), "dup2");
  SYS_CALL(close(fd_file), "close");
}

/*==========================================================*/
/*=======================Tail Command=======================*/
/*==========================================================*/

void TailCommand::execute()
{
  int cnt = 10;
  int file_index = 1;
  if(this->size_args == 3)
  {
    cnt = atoi((string(this->args[1])).substr(1).c_str());
    if(cnt == 0)
    {
      std::cout << "smash error: tail: invalid arguments" << std::endl;
      return;
    }
    file_index = 2;
  }
  else if(this->size_args != 2)
  {
    std::cout << "smash error: tail: invalid arguments" << std::endl;
    return;
  }
  int fd_file;
  SYS_CALL((fd_file = open(this->args[file_index], O_RDONLY)), "open");
  char buffer;
  int pos = 2, line_length = 0;
  int size = lseek(fd_file, 0, SEEK_END);
  while(pos <= size && cnt > 0)
  {
    //SYS_CALL(read(fd_file, &buffer, 1), "read");
    SYS_CALL(lseek(fd_file, -pos, SEEK_END), "lseek");
    pos++;
    if(pos > size) break;
    SYS_CALL(read(fd_file, &buffer, 1), "read");
    while(buffer != '\n')
    {
      line_length++;
      if(pos > size) break;
      SYS_CALL(lseek(fd_file, -pos, SEEK_END), "lseek");
      pos++;
      SYS_CALL(read(fd_file, &buffer, 1), "read");
    }
    //SYS_CALL(lseek(fd_file, 1, SEEK_CUR), "lseek");
    char* line = new char[line_length];
    SYS_CALL(read(fd_file, line, line_length), "read");
    std::cout << line << std::endl;
    delete line;
    line_length = 0;
    cnt--;
  }
  SYS_CALL(close(fd_file), "close");
}
/*============================================================*/
/*======================Special Commands======================*/
/*============================================================*/


void PipeCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    int fd[2];
    SYS_CALL(pipe(fd) , "pipe");
    std::string first_command = getFirstCommand(this->cmd_line);
    std::string second_command = getSecondCommand(this->cmd_line); 

    if(isWithAnd(this->GetCmdLine())) //* Meaning this is "|&" command
    {
      int first_son = fork();
      if(first_son < 0)
      {
        perror("smash error: fork failed");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
      }
      else if(first_son == 0)
      { //first child
        SYS_CALL(setpgrp(), "setpgrp");
        SYS_CALL(dup2((fd[0]),STDIN_FILENO),"close");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        smash.executeCommand(second_command.c_str()); 
        exit(0);
      }
      int second_son = fork();
      if(second_son < 0)
      {
        perror("smash error: fork failed");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
      }
      else if(second_son == 0)
      { //second child
        SYS_CALL(setpgrp(), "setpgrp");
        SYS_CALL(dup2((fd[1]),STDERR_FILENO),"close");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        smash.executeCommand(first_command.c_str());
        exit(0);
      }
      SYS_CALL(close(fd[0]), "close");
      SYS_CALL(close(fd[1]),"close");
      waitpid(first_son ,nullptr,0);
      waitpid(second_son ,nullptr,0);
    }
    else  //* Meaning this is "|" command
    {
      int first_son = fork();
      if(first_son < 0)
      {
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        perror("smash error: fork failed");
      }
      else if(first_son == 0)
      { //first child
        SYS_CALL(setpgrp(), "setpgrp");
        SYS_CALL(dup2((fd[0]),STDIN_FILENO),"close");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        smash.executeCommand(second_command.c_str()); 
        exit(0);
      }
      int second_son = fork();
      if(second_son < 0)
      {
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        perror("smash error: fork failed");
      }
      else if(second_son == 0)
      { //second child
        SYS_CALL(setpgrp(), "setpgrp");
        SYS_CALL(dup2((fd[1]),STDOUT_FILENO),"close");
        SYS_CALL(close(fd[0]),"close");
        SYS_CALL(close(fd[1]),"close");
        smash.executeCommand(first_command.c_str());
        exit(0);
      }
    SYS_CALL(close(fd[0]), "close");
    SYS_CALL(close(fd[1]),"close");
    waitpid(first_son ,nullptr,0);
    waitpid(second_son ,nullptr,0);
    }
}

bool PipeCommand::isWithAnd(std::string stringToCheck)
{
  return(stringToCheck.find("|&"));
}

std::string PipeCommand::getFirstCommand(std::string whole_command)
{
  std::string str = whole_command.substr(0,whole_command.find_first_of("|"));
  _trim(str);
  return str;
}

std::string PipeCommand::getSecondCommand(std::string whole_command)
{
  std::string str = whole_command.substr(whole_command.find_first_of("|")+1 , whole_command.length());
  _trim(str);
  return str;
}
