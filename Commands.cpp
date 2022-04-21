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
  char *command_line = const_cast<char *>(cmd_line);
  bool is_background = false;
  if (_isBackgroundCommand(cmd_line))
  {
    is_background = true;
    _removeBackgroundSign(command_line);
  }
  if(is_background) //to deleteee
    ;
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

    return nullptr;
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
/*======================================================*/
/*================== Commands Methods ==================*/
/*======================================================*/

// Command

Command::Command(const char *cmd_line)
{
  //int size = _numOfStringsInArray(cmd_line);
  this->cmd_line = cmd_line;
  char *command_line = const_cast<char *>(cmd_line);
  _removeBackgroundSign(command_line);
  this->args = new char *[COMMAND_MAX_ARGS + 1];
  this->size_args = _parseCommandLine(command_line, this->args);
  this->pid = getpid();
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
pid_t Command::getPID()
{
  return this->pid;
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

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->pid = getpid();
}

void ShowPidCommand::execute()
{
  std::cout << "smash pid is " << this->pid << std::endl;
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
  if(kill(pid, sig) != 0)
  {
    perror("smash error: kill failed");
    return;
  }
  std::cout << "signal number " << sig << " was sent to pid " << pid << std::endl;
}

// ForegroundCommand, fg

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand::BuiltInCommand(cmd_line)
{
  this->jobs = jobs;
}

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
  }
  else if(this->size_args == 1)
  {
    job = this->jobs->getLastJob(&jobid);
  }
  else
  {
    std::cout << "smash error: fg: invalid arguments" << std::endl;
    return;
  }

  
}

/*=====================================================*/
/*=============JobsList & JobEntry Methods=============*/
/*=====================================================*/

JobsList::JobsList(){
  this->jobs = vector<JobsList::JobEntry*>();
}

void JobsList::addJob(Command* cmd, bool isStopped)
{
  int available_job_id = 1;
  this->getLastJob(&available_job_id);
  available_job_id++;
  JobsList::JobEntry *new_job = new JobsList::JobEntry(cmd, available_job_id, isStopped);
  new_job->setTime();
  this->jobs.push_back(new_job);
}

void JobsList::printJobsList(){
  for (size_t i = 0; i < (this->jobs).size(); i++){
    time_t current_time;
    time(&current_time);
    double seconds_elapsed = difftime(jobs[i]->GetInsertTime(), current_time);
    if(jobs[i]->GetIsStopped()){
      std::cout <<"["<< jobs[i]->getJobID() <<"] "<<(jobs[i]->GetCMD())->GetCmdLine()<<" : "<<(jobs[i]->GetCMD())->getPID()<<" "<<seconds_elapsed<<"(stopped)"<<std::endl;  
    }
    else{
      std::cout <<"["<< jobs[i]->getJobID() <<"] "<<(jobs[i]->GetCMD())->GetCmdLine()<<" : "<<(jobs[i]->GetCMD())->getPID()<<" "<<seconds_elapsed<<std::endl;
    }
  }
}

void JobsList::killAllJobs()
{ 
  std::cout << "smash: sending SIGKILL signal to " << this->jobs.size() << " jobs:" << std::endl;
  for(auto it: this->jobs)
  {
    if(kill(it->getPID(), 9) != 0)
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
  for (auto it: this->jobs)
    if((it->getJobID())==jobId)
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
    if((*it)->GetIsStopped())
    {
      *jobId = (*it)->getJobID();
      return (*it);
    }
  }
  *jobId = 0;
  return nullptr;
}


//=========================Job Entry=============================

JobsList::JobEntry::JobEntry(Command *cmd, int jobid, bool isStopped)
{
  this->cmd = cmd;
  this->job_id = jobid;
  this->is_stopped = isStopped;
  this->insert_time = 0;
}

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
int JobsList::JobEntry::getPID(){
  return this->cmd->getPID();
}
bool JobsList::JobEntry::GetIsStopped()
{
  return this->is_stopped;
}
time_t JobsList::JobEntry::GetInsertTime()
{
  return this->insert_time;
}
void JobsList::JobEntry::setTime(){
  time(&(this->insert_time));
}
