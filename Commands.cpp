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
bool _isBackgroundComamnd(const char* cmd_line) {
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

int _numOfStringsInArray(const char* cmd_line)
{
  int cnt = 0;
  std::string s = string(cmd_line);
  size_t end = s.find_last_not_of(WHITESPACE);

  if(cmd_line[0] != ' ') cnt++;

  for (int i = 1; i <= end; i++)
  {
    if(cmd_line[i-1] == ' ' && cmd_line[i] != ' ')
      cnt++;
  }

  return cnt;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
// TODO: add your implementation
  this->shellname = "smash> ";
  this->last_directory = NULL;
  this->jobs_list = new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  bool is_background = false;
  if (_isBackgroundComamnd(cmd_line))
  {
    is_background = true;
    _removeBackgroundSign(cmd_line);
  }
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  switch (firstWord)
  {
  case "pwd":
    return new GetCurrDirCommand(cmd_line);
    break;

  case "showpid":
    return new ShowPidCommand(cmd_line);
    break;
  
  case "cd":
    return new ChangeDirCommand(cmd_line, this->GetLastDirectory());
    break;

  case "jobs":
    return new JobsCommand(cmd_line, this->jobs_list);
    break;

  case "kill":
    return new KillCommand(cmd_line, this->jobs_list);
    break;

  default:
    break;
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  if(firstWord.compare("chprompt") == 0) 
  {
    int size = _numOfStringsInArray(cmd_line);
    if(size <= 1)
    {
      this->shellname = "smash> ";
      return;
    }
    char **args = (char **)malloc(sizeof(char *) * size);
    if(args == NULL) // error
    if(size != _parseCommandLine(cmd_line, args))
    {
      // error
    }
    this->shellname = string(args[1]) + "> ";
    return;
  }

  Command *cmd = CreateCommand(cmd_line);
  cmd->execute();
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
  int size = _numOfStringsInArray(cmd_line);
  this->cmd_line = cmd_line;
  _removeBackgroundSign(cmd_line);
  this->args = (char **)malloc(sizeof(char *) * size);
  if(this->args == NULL)
    throw std::bad_alloc(); // allocation error
  if(size != _parseCommandLine(cmd_line, this->args))
    ; // error
  this->size_args = size;
  this->pid = getpid();
  this->is_finished = false;
}

Command::~Command()
{
  for (int i = 0; i < this->size_args; i++)
    free(this->args[i]);
  free(this->args);
  this->size_args = 0;
}

int Command::getPID()
{
  return this->pid;
}

// ShowPidCommand, showpid

ShowPidCommand::ShowPidCommand(const char* cmd_line)
{
  this->args = nullptr;
  this->size_args = 0;
  this->pid = getpid();
}

void ShowPidCommand::execute()
{
  std::cout << "smash pid is " << this->pid << std::endl;

  this->is_finished = true;
}

// GetCurrDirCommand, pwd

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line)
{
  this->args = nullptr;
  this->size_args = 0;
}

void GetCurrDirCommand::execute()
{
  char root[PATH_MAX];
  if (getcwd(root, sizeof(root)) != NULL)
    std::cout << root << std::endl;
  else
    perror("smash error: getcwd failed");
  this->is_finished = true;
}

// ChangeDirCommand, cd

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : Command(cmd_line)
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
      this->is_finished = true;
      return;
    }
    char *cd;
    if (this->args[1] == "-")
    {
      if(this->last_directory == NULL) 
      {
        std::cout << "smash error: cd: OLDPWD not set" << std::endl;
        this->is_finished = true;
        return;
      }
      cd = *(this->last_directory);
    }
    else
      cd = args[1];
    if (chdir(cd) != 0)
    {
      perror("smash error: chdir failed");
      this->is_finished = true;
      return;
    }
    *(this->last_directory) = before_change_pwd;
  }
  else if(this->size_args > 2)
  {
    std::cout << "smash error: cd: too many arguments" << std::endl;
  }

  this->is_finished = true;
}

// JobsCommand, jobs

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line)
{
  this->jobs = jobs;
}

void JobsCommand::execute()
{
  this->jobs->removeFinishedJobs();
  this->jobs->printJobsList();
  
  this->is_finished = true;
}

// KillCommand, kill

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : Command(cmd_line)
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
    this->is_finished = true;
    return;
  }
  JobEntry *job = this->jobs->getJobById(jobid);
  if(job == nullptr)
  {
    std::cout << "smash error: kill: job-id " << jobid << " does not exist" << std::endl;
    this->is_finished = true;
    return;
  }
  int pid = job->getPID();
  if(kill(pid, sig) != 0)
  {
    perror("smash error: kill failed");
    this->is_finished = true;
    return;
  }
  std::cout << "signal number " << sig << " was sent to pid " << pid << std::endl;
  
  this->is_finished = true;
}

// ForegroundCommand, fg

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : Command(cmd_line)
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
      this->is_finished = true;
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
    this->is_finished = true;
    return;
  }

  
}

/*=====================================================*/
/*=============JobsList & JobEntry Methods=============*/
/*=====================================================*/

JobsList::void addJob(Command* cmd, bool isStopped = false)
{
  int available_job_id = 1;
  JobsList::JobEntry *temp = this->getLastJob(&available_job_id);
  available_job_id++;
  JobsList::JobEntry *new_job = new JobsList::JobEntry(cmd, available_job_id, isStopped);
  new_job->setTime(time());
  this->jobs.pushback(new_job);
}

void JobsList::printJobsList(){
  for (int i=0; i<(this->jobs).size(); i++){
    time_t current_time=time();
    double seconds_elapsed = difftime(jobs[i]->time, current_time);
    if(jobs[i]->is_stopped){
      std::cout <<"["<< jobs[i]->job_id <<"] "<<(jobs[i]->cmd)->cmd_line<<" : "<<(jobs[i]->cmd)->pid<<" "<<seconds_elapsed<<"(stopped)"<<std::endl;  
    }
    else{
      std::cout <<"["<< jobs[i]->job_id <<"] "<<(jobs[i]->cmd)->cmd_line<<" : "<<(jobs[i]->cmd)->pid<<" "<<seconds_elapsed<<std::endl;
    }
  }
}

void JobsList::killAllJobs(){
  for(int i=0; i<(this->jobs).size(); i++){
    delete(jobs[i]->cmd);
    delete(jobs[i]);
  }
}

void JobsList::void removeFinishedJobs(){
  for(int i=0; i<this->jobs.size(); i++){
    if((this->jobs)[i]->cmd)->is_finished){
      (this->jobs).erase(i);
    }
  }
}

JobEntry * JobsList::getJobById(int jobId){
  for(int i=0; i<(this->jobs).size(); i++){
    if(((this->jobs)[i]->job_id)==jobID){
      return (this->jobs)[i];
    }
  }
}

void JobList::removeJobById(int jobId){
  for(int i=0; i<(this->jobs).size(); i++){
    if(((this->jobs)[i]->job_id)==jobID){
      (this->jobs).erase(i);
    }
  }
}

JobEntry * JobList::getLastJob(int* lastJobId){
  int j=(this->jobs).size();
  if(j==0){
    lastJobId = nullptr;
    return nullptr;
  }
  JobEntry* temp=(this->jobs)[j];
  *lastJobId = temp->job_id;
  return temp;
}

JobEntry * JobList::getLastStoppedJob(int *jobId){
  for(int i=((this->jobs).size())-1; i>=0; i--){
    if((this->jobs)[i]->is_stopped){
      jobID=(this->jobs)[i]->job_id;
      return (this->jobs)[i];
    }
  }
  jobID=nullptr;
  return nullptr;
}


//=========================Job Entry=============================

JobsList::JobEntry::JobEntry(Command *cmd, int jobid, bool isStopped)
{
  this->cmd = cmd;
  this->job_id = jobid;
  this->is_stopped = isStopped;
  this->insert_time();
}

JobsList::JobEntry::~JobEntry(){
  delete(this->cmd);
}

int JobsList::JobEntry::getJobID(){
  return this->job_id;
}
int JobsList::JobEntry::getPID(){
  return this->cmd->getPID();
}
void JobsList::JobsEntry::setTime(time_t new_time){
  this->insert_time=new_time;
}
