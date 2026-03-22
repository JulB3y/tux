#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exec.h"

void launchApp(char *exec) {
  pid_t pid = fork();
  if (pid < 0)
    return;

  if (pid == 0) {
    signal(SIGHUP, SIG_IGN);

    if (setsid() < 0)
      _exit(127);

    pid_t pid2 = fork();
    if (pid2 < 0)
      _exit(127);
    if (pid2 > 0)
      _exit(0);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull < 0)
      _exit(127);

    if (dup2(devnull, STDIN_FILENO) < 0)
      _exit(127);
    if (dup2(devnull, STDOUT_FILENO) < 0)
      _exit(127);
    if (dup2(devnull, STDERR_FILENO) < 0)
      _exit(127);
    if (devnull > 2)
      close(devnull);

    execl("/bin/sh", "sh", "-c", exec, NULL);
    _exit(127);
  }

  waitpid(pid, NULL, 0);
}
