# NVIDIA Jetson AGX Orin power button detection example

This is a simple Linux device driver with an user space application that demonstrate how to detect power button is pressed (interrupt occurred), and user space application is able to communicate with this driver by using epoll, ioctl, read and write.

Tested on [L4T R35.2.1](https://developer.nvidia.com/embedded/jetson-linux-r3521) (kernel: 5.10.104-tegra)

## Build and Run

Download kernel source and extract to `/usr/src/linux-headers-5.10.104-tegra-ubuntu20.04_aarch64/`

Prepare for building external module:

```bash
cd /usr/src/linux-headers-5.10.104-tegra-ubuntu20.04_aarch64/kernel-5.10/
sudo make tegra_defconfig
sudo make prepare
sudo make modules_prepare
```

Build and load custom driver:

```bash
cd driver
make
sudo make install
```

Build and run test app:

```bash
cd epoll_app
make
sudo ./epoll_app
```

Click the power button.

## Output messages

driver output (dmesg):
```
...skipped
[24370.742413] neil-class neil-dev: initialized. major: 503, minor: 0
[24393.743237] neil-class neil-dev: device open
[24393.743271] neil-class neil-dev: poll_wait
[24393.743280] neil-class neil-dev: poll_wait exit
[24405.059777] neil-class neil-dev: Interrupt #305 occurred
[24405.060124] neil-class neil-dev: poll_wait
[24405.060134] neil-class neil-dev: poll_wait exit
[24405.060286] neil-class neil-dev: read device, minor: 0, count: 63
[24405.060294] neil-class neil-dev: Copy 26 bytes to user
[24405.060317] neil-class neil-dev: write device, minor: 0, count: 24
[24405.060324] neil-class neil-dev: Copy 24 bytes from the user
[24405.060330] neil-class neil-dev: copy_from_user: Data from the user space
[24405.060354] neil-class neil-dev: poll_wait
[24405.060358] neil-class neil-dev: poll_wait exit
[24406.660992] neil-class neil-dev: Interrupt #305 occurred
[24406.661045] neil-class neil-dev: poll_wait
[24406.661055] neil-class neil-dev: poll_wait exit
[24406.661146] neil-class neil-dev: read device, minor: 0, count: 63
[24406.661151] neil-class neil-dev: Copy 26 bytes to user
[24406.661183] neil-class neil-dev: write device, minor: 0, count: 24
[24406.661186] neil-class neil-dev: Copy 24 bytes from the user
[24406.661190] neil-class neil-dev: copy_from_user: Data from the user space
[24406.661205] neil-class neil-dev: poll_wait
[24406.661208] neil-class neil-dev: poll_wait exit
[25105.508576] neil-class neil-dev: device close
[25116.246695] neil-class neil-dev: driver exited
```

app output:
```
epoll_wait...
event count: 1
EPOLLIN: buff read: Data from the kernel space
EPOLLOUT: buff wrote: Data from the user space
epoll_wait...
event count: 1
EPOLLIN: buff read: Data from the kernel space
EPOLLOUT: buff wrote: Data from the user space
epoll_wait...
```

## References

Sending Signal from Linux Device Driver to User Space
- https://embetronicx.com/tutorials/linux/device-drivers/sending-signal-from-linux-device-driver-to-user-space/
- https://github.com/Embetronicx/Tutorials/tree/master/Linux/Device_Driver/Signal_in_Linux_kernel

Basic Character Driver in Linux
- https://linuxhint.com/basic-character-driver-linux/

Linux Kernel Teaching - Character Device Driver
- https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html

Simple Linux character device driver
- https://olegkutkov.me/2018/03/14/simple-linux-character-device-driver/

Epoll in linux device driver
- https://embetronicx.com/tutorials/linux/device-drivers/epoll-in-linux-device-driver/
- https://github.com/Embetronicx/Tutorials/tree/master/Linux/Device_Driver/Poll

Methods to communicate from user space to kernel space
- https://stackoverflow.com/a/66838525

