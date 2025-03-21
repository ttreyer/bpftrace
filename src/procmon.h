#pragma once
#include <unistd.h>

namespace bpftrace {

class ProcMonBase {
public:
  ProcMonBase() = default;
  virtual ~ProcMonBase() = default;

  // Whether the process is still alive
  virtual bool is_alive() = 0;

  // pid of the process being monitored
  pid_t pid()
  {
    return pid_;
  };

protected:
  int pid_ = -1;
};

class ProcMon : public ProcMonBase {
public:
  ProcMon(pid_t pid);
  ~ProcMon() override;

  // Disallow copying as the internal state will get out of sync which will
  // cause issues.
  ProcMon(const ProcMon&) = delete;
  ProcMon& operator=(const ProcMon&) = delete;
  ProcMon(ProcMon&&) = delete;
  ProcMon& operator=(ProcMon&&) = delete;

  bool is_alive() override;

private:
  int pidfd_ = -1;
  char proc_path_[32];
  bool died_ = false;
  void setup(pid_t pid);
};

} // namespace bpftrace
