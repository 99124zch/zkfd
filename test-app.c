#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
 
#define NETLINK_MY_UNIT_PROTO  31
 
#define NLSPACE              1024
 
//数据
static const char *thedata = "hello world";
 
int main(int argc, char **argv)
{
	int sd;
	struct sockaddr_nl src_nl, dest_nl;
	struct nlmsghdr *nlhdr; // 'carries' the payload
	struct iovec iov;
	struct msghdr msg;
	ssize_t nsent, nrecv;
	char *file_path = "/zch/test-kernel.txt"; //发送的文件路径
	char *output_file = "/zch/test-output-kernel.txt"; // 输出文件路径
 
    //1、创建节点，域名PF_NETLINK，类型SOCK_RAW，协议NETLINK_MY_UNIT_PROTO
	sd = socket(PF_NETLINK, SOCK_RAW, NETLINK_MY_UNIT_PROTO);
	if (sd < 0) {
		exit(EXIT_FAILURE);
	}
 
	//2、设置netlink源地址结构体已经绑定
	memset(&src_nl, 0, sizeof(src_nl));
	src_nl.nl_family = AF_NETLINK;
	src_nl.nl_pid = getpid();
	src_nl.nl_groups = 0x0;   // no multicast
	if (bind(sd, (struct sockaddr *)&src_nl, sizeof(src_nl)) < 0) {
		exit(EXIT_FAILURE);
	}
		
	/* 3. 设置目的地址结构体*/
	memset(&dest_nl, 0, sizeof(dest_nl));
	dest_nl.nl_family = AF_NETLINK;
	dest_nl.nl_groups = 0x0; // no multicast
	dest_nl.nl_pid = 0;      // destined for the kernel
 
	/* 4. 分配内存*/
    size_t file_size;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        close(sd);
        exit(EXIT_FAILURE);
    }
    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

	nlhdr = (struct nlmsghdr *)malloc(NLMSG_SPACE(file_size));
    if (!nlhdr) {
        perror("malloc");
        close(fd);
        close(sd);
        exit(EXIT_FAILURE);
    }
	
	memset(nlhdr, 0, NLMSG_SPACE(file_size));
	nlhdr->nlmsg_len = NLMSG_SPACE(file_size);
	nlhdr->nlmsg_pid = getpid();
 
	// 读取文件内容到消息缓冲区
    if (read(fd, NLMSG_DATA(nlhdr), file_size) < 0) {
        perror("read");
        free(nlhdr);
        close(fd);
        close(sd);
        exit(EXIT_FAILURE);
    }
    close(fd);
 
	/* 5.设置iovec*/
	memset(&iov, 0, sizeof(struct iovec));
	iov.iov_base = (void *)nlhdr;//上面申请的空间
	iov.iov_len = nlhdr->nlmsg_len;
 
	/* 设置消息头结构体 */
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_name = (void *)&dest_nl;   // 目的地址
	msg.msg_namelen = sizeof(dest_nl); // 目的地址大小
	msg.msg_iov = &iov;//
	msg.msg_iovlen = 1; // # elements in msg_iov
 
	
	/* 6.发送给内核 */
	nsent = sendmsg(sd, &msg, 0);
	if (nsent < 0) {
		free(nlhdr);
		exit(EXIT_FAILURE);
	} else if (nsent == 0) {
		free(nlhdr);
		exit(EXIT_FAILURE);
	}
 
	printf("success, sent  message\n");

	//刷新缓冲区
	fflush(stdout);
 
	/* 7. 接收数据*/
	printf("now blocking on kernel netlink msg via recvmsg()\n");
	 nrecv = recvmsg(sd, &msg, 0);
    if (nrecv < 0) {
        perror("recvmsg");
        free(nlhdr);
        close(sd);
        exit(EXIT_FAILURE);
    }

    // 将接收到的数据写入文件
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_out < 0) {
        perror("open output file");
        free(nlhdr);
        close(sd);
        exit(EXIT_FAILURE);
    }
    if (write(fd_out, NLMSG_DATA(nlhdr), nrecv) < 0) {
        perror("write");
        free(nlhdr);
        close(fd_out);
        close(sd);
        exit(EXIT_FAILURE);
    }
    close(fd_out);
	printf("success, received and written to file %s\n", output_file);

	free(nlhdr);
	close(sd);
	exit(EXIT_SUCCESS);
}