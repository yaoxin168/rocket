#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    int efd = eventfd(0, EFD_CLOEXEC);
    if (efd == -1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }

    // 设置 FD_CLOEXEC 标志
    int flags = fcntl(efd, F_GETFD);
    flags |= FD_CLOEXEC;
    if (fcntl(efd, F_SETFD, flags) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // 执行 exec 函数
    execl("/bin/ls", "ls", NULL);

    // 如果 exec 函数成功执行，下面的代码不会被执行

    close(efd);

    return 0;
}