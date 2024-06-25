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
 
//文件名
#define OURMODNAME   "netlink_simple_intf"
#define NETLINK_MY_UNIT_PROTO   31
#define NLSPACE              1024
#define SIZE              1024

static struct sock *nlsock;
 
//收到数据，触发回调函数。
static void netlink_recv_and_reply(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_tx;
 
    //返回的数据
	//char *reply = "Reply from kernel netlink";
	int pid, msgsz, stat;

	//申请SIZE大小的内存空间
	char *ptr;

	ptr = kmalloc(SIZE, GFP_KERNEL);
	if(!ptr){
		printk("Fail to allocate memory\n");
		return ;
	}
 
	nlh = (struct nlmsghdr *)skb->data;
	pid = nlh->nlmsg_pid;	//发送进程的PID
	//pr_info("received from PID %d:\n" "\"%s\"\n", pid, (char *)NLMSG_DATA(nlh));

	// 将用户态的数据存储到内存空间
    msgsz = strnlen(NLMSG_DATA(nlh), SIZE);
    strncpy(ptr, NLMSG_DATA(nlh), msgsz);
    ptr[msgsz] = '\0';  // 添加字符串结束符

    printk("Data stored in kernel memory: %s", (char *)ptr);
	
   
	//返回数据长度
	msgsz = strnlen(ptr, NLSPACE);
    //新netlink申请
	skb_tx = nlmsg_new(msgsz, 0);
	if (!skb_tx) {
		printk("skb alloc failed!\n");
		return;
	}
 	
	// 设置 payload
	nlh = nlmsg_put(skb_tx, 0, 0, NLMSG_DONE, msgsz, 0);
	NETLINK_CB(skb_tx).dst_group = 0; 
 
	//将发送的数据reply，使用nlmsg_data拷贝到netlink 
    strncpy(nlmsg_data(nlh), ptr, msgsz);
 
	// 发送
	stat = nlmsg_unicast(nlsock, skb_tx, pid);
	if (stat < 0)
		pr_warn("nlmsg_unicast() failed (err=%d)\n", stat);
	printk("reply sent\n");
}
 
//当用户空间向kernel发数据，将触发此回调函数
static struct netlink_kernel_cfg nl_kernel_cfg = {
	.input = netlink_recv_and_reply,
};
 
static int __init netlink_simple_intf_init(void)
{
    //创建netlink
	nlsock = netlink_kernel_create(&init_net, NETLINK_MY_UNIT_PROTO,&nl_kernel_cfg);
	if (!nlsock) {
		printk("netlink_kernel_create failed\n");
		return PTR_ERR(nlsock);
	}
 
	printk("inserted\n");
	return 0;		/* success */
}
 
//模块退出清理
static void __exit netlink_simple_intf_exit(void)
{
	netlink_kernel_release(nlsock);
	pr_info("removed\n");
}
 
module_init(netlink_simple_intf_init);
module_exit(netlink_simple_intf_exit);