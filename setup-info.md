Notes to self on setup
-----------

1. First things first - need a bigger /boot partition on the card.

(https://superuser.com/questions/1204092/resize-partition-and-filesystem-within-disk-image)

- Download standard raspbian image
- Set up a loopback device - `losetup /dev/loop0 ${imagefile}`
- Set up partition access on loopback - `kpartx -a /dev/loop0`
- Image the root partition to a seperate file - `dd if=/dev/mapper/loop0p2 of=rpi-root.img`
- Mount the boot partition - `mount /dev/mapper/loop0p1 /mnt`
- tar the contents - `tar cv /mnt > rpi-boot.tar`
- Remove partition access - `dmsetup remove /dev/mapper/loop0p{1,2}`
- Remote loopback - `losetup -D /dev/loop0`

- Create a new image - `dd if=/dev/zero of=new-raspbian.img bs=1024k count=2M`
- Partition it - `fdisk new-raspbian.img`. Allocate say 200Mb to /boot as vfat, make it bootable and DOS labelled. Allocate the rest to ext2
- Write & quit
- Repeate above steps to get partition access, dd from the captured image back to the new one, mount the vfat partition and untar the old contents onto it
- Repeate the above steps to remove loopback

- Write to an SD card

2. Install raspbian kernel
----------

(https://stackoverflow.com/questions/20167411/how-to-compile-a-kernel-module-for-raspberry-pi)

The standard kernel doesn't have the build bits in it, and to get them requires a recompile of the whole shebang.

Alternative is to install the raspbian kernel which does have the right bits in it:

`sudo aptitude install linux-image-rpi-rpfv linux-headers-rpi-rpfv`

The terminal output should show you the name of the kernel image that is installed
(e.g. `vmlinuz-4.9.6.0-6-rpi`)

This requires extra space on /boot to make the initial RAM disk, hence step one.

Once it's installed, need to edit `/boot/config.txt` and add the lines:

```
kernel={name-of-new-kernel}
```

before rebooting

3. Build your module
---------

On reboot run `uname -r` to check the kernel version.  You should then have /lib/modules/`uname -r`/build existing, and hey presto a simple `make` in your modules should compile the little so-and-sos
