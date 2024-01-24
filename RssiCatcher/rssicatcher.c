#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/netdevice.h>

#define RX_MOD 1
#define CATCH_NUM 1000
#define DEV_MAJOR 200
#define DEV_NAME "RssiCatcher"

#define TEMP_NUM 5
#define TEMP_THRESHOLD 3
#define PRU_ADDR 0x4A300000
#define PRU_SHARED 0x00010000
#define PRU0 0x00000000
#define PRU1 0x00002000
#define Rx_T 1021
#define TX_TIME 0x1000
struct buf
{
	volatile int * arr;
	volatile int top;
	volatile int size;
};

struct buf rssi_buf, rx_buf;

volatile unsigned int * tx_pru = NULL;
volatile unsigned int * rx_pru = NULL;
volatile int task_flag = 0;
static struct task_struct * tsk;
static struct task_struct * download_tsk1 = NULL; 
static struct task_struct * download_tsk2 = NULL;

enum STATES
{
	WAIT1 = 1, TX, WAIT2, RX, EXIT
};
#ifdef TX_MOD
enum STATES states = TX;
#else
enum STATES states = RX;
#endif

int push_opt(struct buf * a, int num)
{
//	printk(KERN_EMERG"%d/r/n", a.top);
	if(a->top > a->size - 1)
	{
//		printk(KERN_EMERG"Oops/r/n");
		a->arr[0] = num;
		a->top = 1;
		return 1;
	}
	a->arr[a->top] = num;
	a->top++;
	return 0;
}
int pop_opt(struct buf * a, int * num)
{
	if (a->top < 1 || a->top > a->size - 1)
		return 1;
	num[0] = a->arr[a->top];
	a->top --;
	return 0;
}

static int RssiCatcher_open(struct inode * inode , struct file * filp)
{
	printk(KERN_EMERG"Rssi Catcher open\r\n");
	return 0;
}
static int RssiCatcher_release(struct ionde * inode, struct file * file)
{
	printk(KERN_EMERG"Rssi Catcher release\r\n");
}




static ssize_t RssiCatcher_read(struct file * filp, int __user * buf, size_t count, loff_t * ppos)
{
	int temp;
	int temp_arr[CATCH_NUM];
	int i = 0;
	while(!pop_opt(&rssi_buf, &temp))
	{
		temp_arr[i] = temp;
		i++;
	}
	copy_to_user(buf, temp_arr, i * sizeof(int));
	return i;

	
}

static struct file_operations RssiCatcher_fops = 
{
	.owner = THIS_MODULE,
	.open = RssiCatcher_open,
	.release = RssiCatcher_release,
	.read = RssiCatcher_read,
};



void tx_func(void)
{
	tx_pru[0] = TX_TIME;
	memset(tx_pru+1, 0x55, sizeof(int) * TX_TIME);
}

bool data_is_valid(void)
{
	int i, valid_counter;
	int addr = (rx_pru[0] - PRU0)/4;
	int temp[TEMP_NUM];
	valid_counter = 0;
//	printk(KERN_EMERG"%d/r/n", addr);
	if (addr - TEMP_NUM > 2)
		memcpy(&temp[0], &rx_pru[addr - TEMP_NUM - 1], sizeof(int) * TEMP_NUM);
	else
		memcpy(&temp[0], &rx_pru[2048 - TEMP_NUM - 1], sizeof(int) * TEMP_NUM);
	for (i = 0; i < TEMP_NUM; i++)
	{
//		printk(KERN_EMERG"%d/r/n", temp[i]);
		if(abs(temp[i] - Rx_T) > 100)
			valid_counter++;
	}
	return valid_counter > TEMP_THRESHOLD ? true : false;
}

volatile int addrprev = 1, addrnext = 1;
int debug_counter = 0;
int rx_func(int stop_flag)
{
	int result = 0;
	int temp, len;
	int i;
	debug_counter ++;
//	printk(KERN_EMERG"%d/r/n",debug_counter);
	if(stop_flag && rx_buf.top)
	{
		len = rx_buf.top;
		while(!pop_opt(&rx_buf, &temp))
		{
			result += temp;
		}
		result =result / len;
		push_opt(&rssi_buf, result);
		return 1;
	}
	addrnext = (rx_pru[0] - PRU0)/4;
//	printk(KERN_EMERG"%d/r/n", addrnext);
	if(addrnext > 2047 || addrnext < 1)
		return -1;
	if(addrprev > addrnext)
	{
		for (i = addrprev; i < 2048; i++)
		{
			if (push_opt(&rx_buf, rx_pru[i]))
			{
				addrprev = i;
				goto CATCH_FINISH;
			}
		}
		for (i = 1; i < addrnext; i++)
		{
			if (push_opt(&rx_buf, rx_pru[i]))
			{
				addrprev = i;
				goto CATCH_FINISH;
			}
		}
		addrprev = addrnext;
		return 0;
	}
	else
	{
		for (i = addrprev; i < addrnext; i++)
		{
			if (push_opt(&rx_buf, rx_pru[i]))
			{
				addrprev = i;
				goto CATCH_FINISH;
			}
		}
		addrprev = addrnext;
		return 0;
	}
CATCH_FINISH:
//	printk(KERN_EMERG"OK");
	rx_buf.top = rx_buf.size - 1;
	while(!pop_opt(&rx_buf, &temp))
	{
		result += abs(temp - Rx_T);
	}
	result = result / rx_buf.size;
	push_opt(&rssi_buf, result);
	return 1;
}


static int catcher_func(void * data)
{
	int tx_flag = 0;
	int wait_counter = 0;
	int stop_flag = 0;
#ifdef TX_MOD
	msleep(5000);
#endif
	while (1)
	{
		switch(states)
		{
			case WAIT1:
//				printk(KERN_EMERG"%d\r\n", tx_pru[0]);
				if(kthread_should_stop())
				{
					states = EXIT;
					break;			
				}
				if(tx_pru[0] == 0)
				{
					states = WAIT2;
					break;
				}
				usleep_range(20,50);
				break;
			case WAIT2:
//				printk(KERN_EMERG"WAIT2\r\n");
				if(kthread_should_stop())
				{
					states = EXIT;
					break;			
				}
				if(data_is_valid())
				{
//					printk(KERN_EMERG"WAIT 2 RX\r\n");
					states = RX;
					break;
				}
#ifdef TX_MOD
				if(wait_counter > 1000)
				{
					states = TX;
					wait_counter = 0;
					break;
				}
#endif
//				usleep_range(20,50);
				wait_counter++;
				break;
			case TX:
			//	msleep(1);
//				printk(KERN_EMERG"TX\r\n");
				if(kthread_should_stop())
				{
					states = EXIT;
					break;			
				}
				
				tx_func();
//				tx_flag ++;
				while (tx_pru[0]);
				usleep_range(20, 50);
				//msleep(1);
				states = RX;
				break;
			case RX:
//				printk(KERN_EMERG"RX\r\n");
				if(kthread_should_stop())
				{
					states = EXIT;
					break;			
				}
				if(!data_is_valid())
				{
//					printk(KERN_EMERG"RX 2 TX\r\n");
					states = TX;
					stop_flag = 1;
					break;
				}
				stop_flag = 0;
				rx_func(stop_flag);
				stop_flag = 0;
				//usleep_range(20, 50);
				break;
			case EXIT:
//				printk(KERN_EMERG"EXIT\r\n");
				goto THREAD_KILL;
					
		}
	}
THREAD_KILL:
	return 0;
}

static int rssicatcher_init(void)
{
	int ret = 0; 	
	ret = register_chrdev(DEV_MAJOR, DEV_NAME, &RssiCatcher_fops); 	
	if (ret < 0) 		
		printk(KERN_EMERG"Rssi Catcher init failed\r\n"); 	
	else 		
		printk(KERN_EMERG"Rssi Catcher init \r\n");
	rssi_buf.arr = kmalloc(CATCH_NUM * sizeof(int), GFP_KERNEL);
	memset(rssi_buf.arr, 0, CATCH_NUM * sizeof(int));
	rssi_buf.top = 0;
	rssi_buf.size = CATCH_NUM;
	rx_buf.arr = kmalloc(1000 * sizeof(int), GFP_KERNEL);
	memset(rx_buf.arr, 0, 1000 * sizeof(int));
	rx_buf.top = 0;
	rx_buf.size = 1000;
	rx_pru = memremap(PRU_ADDR + PRU0, 0x20000, MEMREMAP_WT);
	tx_pru = memremap(PRU_ADDR + PRU1, 0x20000, MEMREMAP_WT);
	printk(KERN_EMERG"Memory mapped correctly: %x\n",rx_pru);
	tsk = kthread_run(catcher_func, NULL, "catcher_thread", 1);
	if (IS_ERR(tsk))
	{
		printk(KERN_EMERG"creat kthread failed!\r\n");
	}
	else
	{
		printk(KERN_EMERG"creat kthread ok!\r\n");
	}
	return 0;

}

static void rssicatcher_exit(void)
{
	int i;
	if(rx_pru)
		memunmap(rx_pru);
	if(tx_pru)
		memunmap(tx_pru);
//	for (i = 0; i < 2047; i ++)
//	{
//		printk(KERN_EMERG"catch %d: %d\r\n", i, rx_buf[i+1] - rx_buf[i]);
//	}
	if(rssi_buf.arr)
		kfree(rssi_buf.arr);
	if(rx_buf.arr) 		
		kfree(rx_buf.arr);
	unregister_chrdev(DEV_MAJOR, DEV_NAME);
	printk(KERN_EMERG"rssi catcher exit\r\n");
	if (!IS_ERR(tsk))
	{
		kthread_stop(tsk);
		printk(KERN_EMERG"catcher thread killed\r\n");
	}
	
	
}


module_init(rssicatcher_init);
module_exit(rssicatcher_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wpc");
