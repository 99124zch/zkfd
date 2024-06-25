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
 
//����
static const char *thedata = "hello world";
 
int main(int argc, char **argv)
{
	int sd;
	struct sockaddr_nl src_nl, dest_nl;
	struct nlmsghdr *nlhdr; // 'carries' the payload
	struct iovec iov;
	struct msghdr msg;
	ssize_t nsent, nrecv;
	char *file_path = "/zch/test-kernel.txt"; //���͵��ļ�·��
	char *output_file = "/zch/test-output-kernel.txt"; // ����ļ�·��
 
    //1�������ڵ㣬����PF_NETLINK������SOCK_RAW��Э��NETLINK_MY_UNIT_PROTO
	sd = socket(PF_NETLINK, SOCK_RAW, NETLINK_MY_UNIT_PROTO);
	if (sd < 0) {
		exit(EXIT_FAILURE);
	}
 
	//2������netlinkԴ��ַ�ṹ���Ѿ���
	memset(&src_nl, 0, sizeof(src_nl));
	src_nl.nl_family = AF_NETLINK;
	src_nl.nl_pid = getpid();
	src_nl.nl_groups = 0x0;   // no multicast
	if (bind(sd, (struct sockaddr *)&src_nl, sizeof(src_nl)) < 0) {
		exit(EXIT_FAILURE);
	}
		
	/* 3. ����Ŀ�ĵ�ַ�ṹ��*/
	memset(&dest_nl, 0, sizeof(dest_nl));
	dest_nl.nl_family = AF_NETLINK;
	dest_nl.nl_groups = 0x0; // no multicast
	dest_nl.nl_pid = 0;      // destined for the kernel
 
	/* 4. �����ڴ�*/
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
 
	// ��ȡ�ļ����ݵ���Ϣ������
    if (read(fd, NLMSG_DATA(nlhdr), file_size) < 0) {
        perror("read");
        free(nlhdr);
        close(fd);
        close(sd);
        exit(EXIT_FAILURE);
    }
    close(fd);
 
	/* 5.����iovec*/
	memset(&iov, 0, sizeof(struct iovec));
	iov.iov_base = (void *)nlhdr;//��������Ŀռ�
	iov.iov_len = nlhdr->nlmsg_len;
 
	/* ������Ϣͷ�ṹ�� */
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_name = (void *)&dest_nl;   // Ŀ�ĵ�ַ
	msg.msg_namelen = sizeof(dest_nl); // Ŀ�ĵ�ַ��С
	msg.msg_iov = &iov;//
	msg.msg_iovlen = 1; // # elements in msg_iov
 
	
	/* 6.���͸��ں� */
	nsent = sendmsg(sd, &msg, 0);
	if (nsent < 0) {
		free(nlhdr);
		exit(EXIT_FAILURE);
	} else if (nsent == 0) {
		free(nlhdr);
		exit(EXIT_FAILURE);
	}
 
	printf("success, sent  message\n");

	//ˢ�»�����
	fflush(stdout);
 
	/* 7. ��������*/
	printf("now blocking on kernel netlink msg via recvmsg()\n");
	 nrecv = recvmsg(sd, &msg, 0);
    if (nrecv < 0) {
        perror("recvmsg");
        free(nlhdr);
        close(sd);
        exit(EXIT_FAILURE);
    }

    // �����յ�������д���ļ�
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