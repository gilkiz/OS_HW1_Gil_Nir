#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>
#include <stdlib.h>
#include <linux/limits.h>

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
    args[i] = (char*)malloc(s.length()+1);
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

SmallShell::SmallShell() : shellname("smash> "), last_directory(NULL), jobs_list(new JobsList()){};

SmallShell::~SmallShell() 
{
  delete jobs_list;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  char *command_line = new char[COMMAND_ARGS_MAX_LENGTH];
  if(strcpy(command_line, cmd_line) == NULL) // error
  if (_isBackgroundCommand(command_line))
    _removeBackgroundSign(command_line);

  string cmd_s = _trim(string(command_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if(firstWord.compare("chprompt") == 0)
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
}

Command::~Command()
{
  for (int i = 0; i < this->size_args; i++)
    free(this->args[i]);
  delete[] this->args;
  this->size_args = 0;
}
const char *Command::GetCmdLine()
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
  std::cout << "smash pid is " << getpid() << std::endl;
}

// GetCurrDirCommand, pwd

void GetCurrDirCommand::execute()
{
  char root[PATH_MAX];
  if (getcwd(root, sizeof(root)) != NULL)
    std::cout << root << std::endl;
  else
    perror("smash error: getcwd failed");
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
    if(getcwd(before_change_pwd, sizeof(before_change_pwd)) == NULL)
    {
      perror("smash error: getcwd failed");
      return;
    }
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
    if (chdir(cd) != 0)
    {
      perror("smash error: chdir failed");
      return;
    }
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
  std::cout << "first pass" << std::endl;
  this->jobs->printJobsList();
  std::cout << "second pass" << std::endl;
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
  if(kill(pid, sig) != 0)
  {
    perror("smash error: kill failed");
    return;
  }
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
  std::cout << job->GetCMD()->GetCmdLine() << ": " << job->getPID() << std::endl;
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
  std::cout << job->GetCMD()->GetCmdLine() << ": " << job->getPID() << std::endl;
}

// QuitCommand, quit

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs){};

void QuitCommand::execute()
{
  if(this->size_args > 1 && string(this->args[1]).compare("kill") == 0)
    this->jobs->killAllJobs();
  exit(0);
}

/*=====================================================*/
/*=============JobsList & JobEntry Methods=============*/
/*=====================================================*/

JobsList::JobsList(){
  this->jobs = vector<JobsList::JobEntry*>();
}

void JobsList::addJob(Command* cmd, pid_t pid , bool isStopped)
{
  int available_job_id = 1;
  this->getLastJob(&available_job_id);
  available_job_id++;
  JobsList::JobEntry *new_job = new JobsList::JobEntry(cmd, available_job_id, pid, isStopped);
  new_job->setTime();
  this->jobs.push_back(new_job);
}

void JobsList::printJobsList(){
  for (size_t i = 0; i < (this->jobs).size(); i++){
    time_t current_time;
    time(&current_time);
    double seconds_elapsed = difftime(jobs[i]->GetInsertTime(), current_time);
    if(jobs[i]->IsStopped()){
      std::cout << "[" << jobs[i]->getJobID() << "] "<< (jobs[i]->GetCMD())->GetCmdLine() <<
        " : "<< jobs[i]->getPID() << " " << seconds_elapsed << "(stopped)" << std::endl;  
    }
    else{
      std::cout << "[" << jobs[i]->getJobID() << "] "<< (jobs[i]->GetCMD())->GetCmdLine() <<
      " : " << jobs[i]->getPID() << " " << seconds_elapsed << std::endl;
    }
  }
}

void JobsList::killAllJobs()
{ 
  std::cout << "smash: sending SIGKILL signal to " << this->jobs.size() << " jobs:" << std::endl;
  for(auto it: this->jobs)
  {
    if(kill(it->getPID(), SIGKILL) != 0)
      {
        perror("smash error: kill failed");
        return;
      }
  }
}

void JobsList::removeFinishedJobs(){
  pid_t finished_pid;
  while((finished_pid = waitpid(-1, nullptr, WNOHANG)) > 0)
  {
    JobsList::JobEntry *job = this->getJobByPID(finished_pid);
    if(job)
      removeJobById(job->getJobID());
  }
}

JobsList::JobEntry * JobsList::getJobById(int jobId)
{
  for(auto it: this->jobs)
    if(it->getJobID() == jobId)
      return it;
  return nullptr;
}

JobsList::JobEntry *JobsList::getJobByPID(int jobPID)
{
  for(auto it: this->jobs)
    if(it->getPID() == jobPID)
      return it;
  return nullptr;
}

void JobsList::removeJobById(int jobId)
{
  for (auto it = this->jobs.begin(); it != this->jobs.end(); it++)
    if(((*it)->getJobID()) == jobId)
      (this->jobs).erase(it);
}

void JobsList::removeJobByPid(pid_t pid)
{
  for (auto it = this->jobs.begin(); it != this->jobs.end(); it++)
    if(((*it)->getPID()) == pid)
      (this->jobs).erase(it);
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){
  int j=(this->jobs).size();
  if(j==0){
    *lastJobId = 0;
    return nullptr;
  }
  JobsList::JobEntry* temp=(this->jobs)[j];
  *lastJobId = temp->getJobID();
  return temp;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{
  vector<JobsList::JobEntry *>::reverse_iterator it = this->jobs.rbegin();
  for (; it != this->jobs.rend(); it++)
  {
    if((*it)->IsStopped())
    {
      *jobId = (*it)->getJobID();
      return (*it);
    }
  }
  *jobId = 0;
  return nullptr;
}


//=========================Job Entry=============================

JobsList::JobEntry::~JobEntry(){
  delete(this->cmd);
}
Command * JobsList::JobEntry::GetCMD()
{
  return this->cmd;
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
    if(kill(this->pid, SIGSTOP) != 0)
    {
      perror("smash error: kill failed");
      return;
    }
  }
  else if(kill(this->pid, SIGCONT) != 0)
  {
    perror("smash error: kill failed");
    return;
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
    execlp("/bin/bash", "bash", "-c", const_cast<char *>(this->cmd_line), NULL);
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