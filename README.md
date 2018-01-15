# capa110dio
Linux Kernel Module for userspace access to Digital I/O on CAPA100 SBC motherboards.

This module creates a character device at `/dev/axdio` which users can access to read and write the DIO pin state.

# Building
The repository ships with its own Makefile. To build, simply type `make` on the command line. Upon successful build, the module `capa110dio.ko` will be created.

# Deployment
## Once-off
You can load the module for a once-off test by executing `sudo insmod capa110dio.ko`.

To confirm that the module has loaded correctly, run `lsmod` and check the listing for `capa110dio`. Additionally you can test that the device file has been created by running `ls -la /dev/axdio`.

**Note:** You may note that the device file is created with only +rw permissions to root. Extending read/write permissions to other users is covered in the section [udev Configuration](#udev-configuration).

To unload the module, execute `sudo rmmod capa110dio`.

## On System Start
The process to configuring the module to load on system start will differ between system to system. The following process describes how to configure the module on an Ubuntu system.

```
sudo -i
echo capa110dio >> /etc/modules-load.d/modules.conf
exit
sudo cp capa110dio.ko /lib/modules/`uname -r`/kernel/drivers/char/
sudo depmod
```

Upon rebooting, `lsmod` should show an entry for capa110dio.

## udev Configuration
Permissions for the module's device file can be altered through udev configuration rules. A sample ruleset is provided in `etc/99-capa110dio.rules` which grants +rw permissions to all users.

To instate these rules, copy the .rules file to `/etc/udev/rules.d` and reload the module.
