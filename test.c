#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
 
MODULE_AUTHOR("wy");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");
 
//�ļ���
#define OURMODNAME   "netlink_simple_intf"
#define NETLINK_MY_UNIT_PROTO   31
#define NLSPACE              1024
#define SIZE              1024

static struct sock *nlsock;
 
//�յ����ݣ������ص�������
static void netlink_recv_and_reply(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_tx;
 
    //���ص�����
	//char *reply = "Reply from kernel netlink";
	int pid, msgsz, stat;

	//����SIZE��С���ڴ�ռ�
	char *ptr;

	ptr = kmalloc(SIZE, GFP_KERNEL);
	if(!ptr){
		printk("Fail to allocate memory\n");
		return ;
	}
 
	nlh = (struct nlmsghdr *)skb->data;
	pid = nlh->nlmsg_pid;	//���ͽ��̵�PID
	//pr_info("received from PID %d:\n" "\"%s\"\n", pid, (char *)NLMSG_DATA(nlh));

	// ���û�̬�����ݴ洢���ڴ�ռ�
    msgsz = strnlen(NLMSG_DATA(nlh), SIZE);
    strncpy(ptr, NLMSG_DATA(nlh), msgsz);
    ptr[msgsz] = '\0';  // ����ַ���������

    printk("Data stored in kernel memory: %s", (char *)ptr);
	
   
	//�������ݳ���
	msgsz = strnlen(ptr, NLSPACE);
    //��netlink����
	skb_tx = nlmsg_new(msgsz, 0);
	if (!skb_tx) {
		printk("skb alloc failed!\n");
		return;
	}
 	
	// ���� payload
	nlh = nlmsg_put(skb_tx, 0, 0, NLMSG_DONE, msgsz, 0);
	NETLINK_CB(skb_tx).dst_group = 0; 
 
	//�����͵�����reply��ʹ��nlmsg_data������netlink 
    strncpy(nlmsg_data(nlh), ptr, msgsz);
 
	// ����
	stat = nlmsg_unicast(nlsock, skb_tx, pid);
	if (stat < 0)
		pr_warn("nlmsg_unicast() failed (err=%d)\n", stat);
	printk("reply sent\n");
}
 
//���û��ռ���kernel�����ݣ��������˻ص�����
static struct netlink_kernel_cfg nl_kernel_cfg = {
	.input = netlink_recv_and_reply,
};
 
static int __init netlink_simple_intf_init(void)
{
    //����netlink
	nlsock = netlink_kernel_create(&init_net, NETLINK_MY_UNIT_PROTO,&nl_kernel_cfg);
	if (!nlsock) {
		printk("netlink_kernel_create failed\n");
		return PTR_ERR(nlsock);
	}
 
	printk("inserted\n");
	return 0;		/* success */
}
 
//ģ���˳�����
static void __exit netlink_simple_intf_exit(void)
{
	netlink_kernel_release(nlsock);
	pr_info("removed\n");
}
 
module_init(netlink_simple_intf_init);
module_exit(netlink_simple_intf_exit);