#include "cli.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int cli_run(const char *const *argv, char *output, size_t output_size) {
  int fds[2];
  pid_t pid;
  size_t used = 0;
  int status;

  if (output != NULL && output_size > 0) {
    output[0] = '\0';
  }
  if (pipe(fds) != 0) {
    return -1;
  }
  pid = fork();
  if (pid < 0) {
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
  if (pid == 0) {
    int null_fd = open("/dev/null", O_RDONLY);

    if (null_fd >= 0) {
      dup2(null_fd, STDIN_FILENO);
      close(null_fd);
    }
    dup2(fds[1], STDOUT_FILENO);
    dup2(fds[1], STDERR_FILENO);
    close(fds[0]);
    close(fds[1]);
    execvp(argv[0], (char *const *)argv);
    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }
  close(fds[1]);
  for (;;) {
    char buffer[512];
    ssize_t count = read(fds[0], buffer, sizeof(buffer));

    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      break;
    }
    if (output != NULL && used + 1 < output_size) {
      size_t space = output_size - 1 - used;
      size_t take = (size_t)count < space ? (size_t)count : space;

      memcpy(output + used, buffer, take);
      used += take;
      output[used] = '\0';
    }
  }
  close(fds[0]);
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {
      return -1;
    }
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
