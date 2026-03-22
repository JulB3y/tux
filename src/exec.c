#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
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

void copyToClipboard(const char *text) {
  pid_t pid = fork();
  if (pid < 0)
    return;

  if (pid == 0) {
    char *cmd = "xclip -selection clipboard";
    char *wayland_cmd = "wl-copy";
    char *copy_cmd = cmd;

    char *wayland_display = getenv("WAYLAND_DISPLAY");
    char *display = getenv("DISPLAY");

    if (wayland_display) {
      copy_cmd = wayland_cmd;
    } else if (display) {
      copy_cmd = cmd;
    }

#ifdef __APPLE__
    char *macos_cmd = "pbcopy";
    copy_cmd = macos_cmd;
#endif

    int pipefd[2];
    if (pipe(pipefd) == -1) {
      _exit(127);
    }

    pid_t pipe_pid = fork();
    if (pipe_pid < 0) {
      _exit(127);
    }

    if (pipe_pid == 0) {
      close(pipefd[1]);
      dup2(pipefd[0], STDIN_FILENO);
      close(pipefd[0]);

      execl("/bin/sh", "sh", "-c", copy_cmd, NULL);
      _exit(127);
    }

    close(pipefd[0]);
    write(pipefd[1], text, strlen(text));
    close(pipefd[1]);

    waitpid(pipe_pid, NULL, 0);
    _exit(0);
  }

  waitpid(pid, NULL, 0);
}
