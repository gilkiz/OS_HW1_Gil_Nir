#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
using std::string;
using std::vector;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
 protected:
   char **args;
   const char *cmd_line;
   int size_args;

 public:
   Command(const char *cmd_line);
   virtual ~Command();
   virtual void execute() = 0;
   string GetCmdLine();
   // virtual void prepare();
   // virtual void cleanup();
   //  TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
   BuiltInCommand(const char *cmd_line) : Command(cmd_line){};
   virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line) : Command(cmd_line) {};
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
 public:
  PipeCommand(const char* cmd_line) : Command(cmd_line){};
  virtual ~PipeCommand();
  void execute() override;

  bool isWithAnd(string cmd_line);
  string getFirstCommand(string whole_command);
  string getSecondCommand(string whole_command);
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 bool is_append;
 const char* command;
 const char* output_file;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand
{
  string *shell_name;
public:
  ChangePromptCommand(const char *cmd_line, string *shell_name);
  virtual ~ChangePromptCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
  char **last_directory;
  public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
   GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line){};
   virtual ~GetCurrDirCommand() {}
   void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
  int pid;
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~ShowPidCommand() = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
JobsList *jobs;

public:
QuitCommand(const char *cmd_line, JobsList *jobs);
virtual ~QuitCommand() {}
void execute() override;
};




class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
   string cmd_line;
   int job_id;
   pid_t pid;
   bool is_stopped;
   time_t insert_time;

   public:
    JobEntry(string cmd_line, int jobid, pid_t pid, bool isStopped) : 
      cmd_line(cmd_line), job_id(jobid), pid(pid), is_stopped(isStopped), insert_time(0){};
    string GetCmdLine();
    int getJobID();
    pid_t getPID();
    bool IsStopped();
    void SwitchIsStopped();
    time_t GetInsertTime();
    void setTime();
  };
 // TODO: Add your data members

public:
  vector<JobEntry> jobs;
  JobsList();
  ~JobsList() = default; //???
  void addJob(Command* cmd, pid_t pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  JobEntry *getJobByPID(int jobPID);
  void removeJobById(int jobId);
  void removeJobByPid(pid_t pid);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList *jobs;
 
 public:
 JobsCommand(const char *cmd_line, JobsList *jobs);
 virtual ~JobsCommand() {}
 void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList *jobs;

 public:
 KillCommand(const char *cmd_line, JobsList *jobs);
 virtual ~KillCommand() {}
 void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList *jobs;

 public:
 ForegroundCommand(const char *cmd_line, JobsList *jobs);
 virtual ~ForegroundCommand() {}
 void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList *jobs;

 public:
 BackgroundCommand(const char *cmd_line, JobsList *jobs);
 virtual ~BackgroundCommand() {}
 void execute() override;
};

class TailCommand : public BuiltInCommand {
 public:
  TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
  virtual ~TailCommand() {}
  void execute() override;
};

class TouchCommand : public BuiltInCommand {
 public:
  TouchCommand(const char* cmd_line);
  virtual ~TouchCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
   string shellname;
   char *last_directory;
   JobsList *jobs_list;
   SmallShell(); //??  <--

 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  std::string GetName();
  char **GetLastDirectory();
  JobsList *GetJobsList();
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
