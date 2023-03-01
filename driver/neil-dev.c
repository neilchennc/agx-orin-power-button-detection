/**
 * Custom Linux device driver to detect specific interrupt occurred.
 *
 * In this case, the driver will detect NVIDIA Jetson AGX Orin devkit power
 * button is pressed, and user space application is able to communicate with
 * it by using epoll, ioctl, read and write.
 *
 * Tested on L4T R35.2.1 (kernel: 5.10.104-tegra)
 *
 * Author: Neil Chen <neilchennc@gmail.com>
 * Github: https://github.com/neilchennc/agx-orin-power-button-detection
 * Created at: 2023-02-22
 * Version: 1.0
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define UEVENT_ENABLE 0

#define NEIL_DEVICE_NAME "neil-dev"
#define NEIL_CLASS_NAME "neil-class"

/**
 * The power button IRQ number of the Jetson AGX Orin devkit.
 */
#define IRQ_NO 305

#define MAX_BUFFER_SIZE 64

DECLARE_WAIT_QUEUE_HEAD(neil_wait_queue);

struct neil_device_data
{
	struct cdev cdev;
	struct class *drv_class;
	struct device *drv_dev;
	dev_t dev;
};

static struct neil_device_data device_data;

/**
 * The flag indicate IRQ occurred.
 * Set to 1 when IRQ occurred, it will be cleard after notifying user space app.
 */
volatile static int irq_occurred_flag;

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	dev_info(device_data.drv_dev, "Interrupt #%d occurred\n", irq);

	irq_occurred_flag = 1;

	wake_up(&neil_wait_queue);

	return IRQ_HANDLED;
}

#if UEVENT_ENABLE
static int neil_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}
#endif

static int neil_open(struct inode *inode, struct file *file)
{
	dev_info(device_data.drv_dev, "device open\n");
	return 0;
}

static int neil_release(struct inode *inode, struct file *file)
{
	dev_info(device_data.drv_dev, "device close\n");
	return 0;
}

static long neil_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	dev_info(device_data.drv_dev, "device ioctl\n");
	return 0;
}

static ssize_t neil_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	uint8_t *data = "Data from the kernel space";
	size_t data_len = strlen(data);
	size_t size_to_copy, n_copied;

	dev_info(device_data.drv_dev, "read device, minor: %d, count: %ld\n", MINOR(file->f_path.dentry->d_inode->i_rdev), count);

	if (data_len > count)
	{
		size_to_copy = count;
		dev_warn(device_data.drv_dev, "Only copy %ld bytes to user\n", size_to_copy);
	}
	else
	{
		size_to_copy = data_len;
		dev_info(device_data.drv_dev, "Copy %ld bytes to user\n", size_to_copy);
	}

	n_copied = copy_to_user(buf, data, size_to_copy);
	if (n_copied < 0)
	{
		dev_err(device_data.drv_dev, "copy_to_user failed\n");
		return -EFAULT;
	}

	return size_to_copy;
}

static ssize_t neil_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	uint8_t data[MAX_BUFFER_SIZE];
	size_t size_to_copy, n_copied;

	dev_info(device_data.drv_dev, "write device, minor: %d, count: %ld\n", MINOR(file->f_path.dentry->d_inode->i_rdev), count);

	if (count > MAX_BUFFER_SIZE)
	{
		size_to_copy = MAX_BUFFER_SIZE;
		dev_info(device_data.drv_dev, "Only copy %d bytes from the user\n", MAX_BUFFER_SIZE);
	}
	else
	{
		size_to_copy = count;
		dev_info(device_data.drv_dev, "Copy %ld bytes from the user\n", count);
	}

	n_copied = copy_from_user(data, buf, size_to_copy);
	if (n_copied < 0)
	{
		dev_err(device_data.drv_dev, "copy_from_user failed\n");
		return -EACCES;
	}

	data[size_to_copy] = '\0';

	dev_info(device_data.drv_dev, "copy_from_user: %s\n", data);

	return size_to_copy;
}

static __poll_t neil_poll(struct file *filp, struct poll_table_struct *wait)
{
	__poll_t mask = 0;

	dev_info(device_data.drv_dev, "poll_wait\n");
	poll_wait(filp, &neil_wait_queue, wait);
	dev_info(device_data.drv_dev, "poll_wait exit\n");

	if (irq_occurred_flag)
	{
		mask |= (POLLIN | POLLOUT | POLLWRNORM);
		irq_occurred_flag = 0; // cleanup
	}

	return mask;
}

static const struct file_operations neil_fops = {
	.owner = THIS_MODULE,
	.open = neil_open,
	.release = neil_release,
	.unlocked_ioctl = neil_ioctl,
	.read = neil_read,
	.write = neil_write,
	.poll = neil_poll,
};

static int __init neil_init(void)
{
	if (alloc_chrdev_region(&device_data.dev, 0, 1, NEIL_DEVICE_NAME))
	{
		pr_err("alloc_chrdev_region failed\n");
		return -1;
	}

	cdev_init(&device_data.cdev, &neil_fops);
	if (cdev_add(&device_data.cdev, device_data.dev, 1) < 0)
	{
		pr_err("cdev_add failed\n");
		goto error_class;
	}

	device_data.drv_class = class_create(THIS_MODULE, NEIL_CLASS_NAME);
	if (IS_ERR(device_data.drv_class))
	{
		pr_err("class_create failed\n");
		goto error_class;
	}
#if UEVENT_ENABLE
	device_data.drv_class->dev_uevent = neil_uevent;
#endif
	device_data.drv_dev = device_create(device_data.drv_class, NULL, device_data.dev, NULL, NEIL_DEVICE_NAME);
	if (IS_ERR(device_data.drv_dev))
	{
		pr_err("device_create failed\n");
		goto error_device;
	}

	if (request_irq(IRQ_NO, irq_handler, IRQF_SHARED, NEIL_DEVICE_NAME, (void *)irq_handler))
	{
		pr_err("request_irq failed\n");
		goto error_irq;
	}

	// initialize success
	dev_info(device_data.drv_dev, "initialized. major: %d, minor: %d\n", MAJOR(device_data.dev), MINOR(device_data.dev));
	return 0;

error_irq:
	device_destroy(device_data.drv_class, device_data.dev);
error_device:
	class_destroy(device_data.drv_class);
error_class:
	unregister_chrdev_region(device_data.dev, 1);
	return -1;
}

static void __exit neil_exit(void)
{
	free_irq(IRQ_NO, (void *)irq_handler);
	device_destroy(device_data.drv_class, device_data.dev);
	class_destroy(device_data.drv_class);
	cdev_del(&device_data.cdev);
	unregister_chrdev_region(device_data.dev, 1);

	dev_info(device_data.drv_dev, "driver exited\n");
}

module_init(neil_init);
module_exit(neil_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("neilchennc@gmail.com");
MODULE_DESCRIPTION("Linux device driver example - AGX Orin power button detection");
MODULE_VERSION("1.0");
