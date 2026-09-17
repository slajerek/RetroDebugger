# Creating a Windows cross-build system using Fedora Linux

**WARNING**: Work in progress, do not use these instructions yet!


## Test machine setup details

I'll be using a Fedora 30 64-bit VM using VirtualBox (6.0.x). Fedora provides all packages required to cross-compile Gtk applications, unlike Debian. Hopefully we can use this as a guide for a Debian cross-build system by using `alien` for any packages Debian doesn't provide.

The install image I'll be using is the netinstall x86\_64 image:
<https://download.fedoraproject.org/pub/fedora/linux/releases/30/Workstation/x86_64/iso/Fedora-Workstation-netinst-x86_64-30-1.2.iso>

### VM install details

In VBox select 'New machine' and create a Fedora 64-bit VM.

I've changed the following settings from the defaults:

* HDD: 64GB, growing
* 2GB memory, 2 CPU cores (adjust to host capabilities)
* Enable bidirectional for shared clipboard and drag'n'drop (no idea if it'll work)
* Disable 'floppy' in the boot order
* Change chipset to ICH9
* Video memory: 128MB, or whatever the max is
* Graphics controller: VMSVGA, enable 3D
  (I haven't actually gotten VICE's OpenGL to actually work with any driver)
* Network: Bridged

#### Install from ISO

In Vbox attach the ISO and start the VM.

As soon as the Grub shell appears, hit 'Tab' and add 'inst.text' to the boot line and hit 'Enter', this will run the installer in text mode,
Select '2' when given a choice between VNC and text mode.

There should be menu:

* 1) I stuck to EN-US
* 2) Timezone (optional), I used 1,1,1 (Europe/Amsterdam)
* 3) Installation source: already set to 'closest mirror', should be okay
* 4) Software selection, this is important for later.
    Select '2', (Minimal), then 'c' continue
    Then select '8' (C Development Tools and Libraries) and perhaps '38' (Text based internet)
* 5) Install dest: VBox HDD: 2 - use all space
* 6) Network: (optional, defaults to DHCP)
* 7) Set root password to 'vicerulez' or so, 'vice' is too short
* 8) Create normal user
    - 3 - vice
    - 4 - use password
    - 5 - set password (vicerulez)
    - 6 - make Admin
    - 7 - groups: add 'wheel'


Press 'b' to begin install and wait.

Reboot after install (don't forget to remove ISO)

#### Basic setup of the VM

Now login as 'vice'

Optional: set passwords

```
$ su
$ passwd
# enter 'vice' as password
$ passwd vice
# enter 'vice' as password
```
Fedora will bitch, but it works.


##### Install VBox Guest Additions

This is obviously only required, or even optional, when running a VM.

```
$ su
$ dnf update kernel*
```
If this actually updates the kernel, reboot.

Insert the VBox Guest Additions ISO via the VBox menu.
And then mount it:
```
$ mkdir /media/vbox
$ mount -r /dev/cdrom /media/vbox
```

Now install a few packages:
```
$ dnf install gcc kernel-devel kernel-headers dkms make bzip2 perl libxcrypt-compat
```

Set env var to the kernel source dir (do we need this?)
```
$ export KERN_DIR=/usr/src/kernels/`uname -r`
```

Build and install VBox kernel modules:
```
$ cd /media/vbox
$ ./VBoxLinuxAdditions.run
$ shutdown -r now
```


## Install packages required for cross-compiling Gtk3
```
$ su
$ dnf install mingw64-gtk3 mingw64-glew glib2-devel xa
```

Install svn:
```
$ dnf install subversion
```

Check out trunk and create configure (as vice, not root)
```
$ svn checkout --username=$USER svn+ssh://$USER@svn.code.sf.net/p/vice-emu/code/trunk vice-trunk
$ cd vice-trunk/vice
$ ./autogen.sh
```
This should result in a proper build system.


### Create builddir and test system
```
$ cd ..
$ mkdir gtk3-build
$ cd gtk3-build
$ ../vice/configure --enable-gtk3ui --host=x86_64-w64-mingw32 --enable-arch=no
```


Barfs at 'Making all in readmes'

```
$ dnf install texinfo
$ dnf install texlive
```
Will install a crapload of packages and still not create vice.pdf =)



### Create bindist

```
$ make bindistzip
``

**Note**:

Current make-bindist_win32.sh contains some hackish code to make GdkPixbuf recognize the SVG loader. which will break as soons as gdkpixbuf updates its version number.
So we'll probably need a bit of `pkg-config --modversion` magic to update the code to handle newer versions of GdkPixbuf.



## Update (2020-11-29): Use Fedora 33 Workstation x64

### Install and setup

(I used a VBox VM for this, which already comes with VBox guest additions
 installed, which is nice)

First update and reboot:

```
$ sudo dnf update
$ shutdown -r now
```

When using VBox, make sure your VBox version matches the guest additions that
Fedora uses. I had a slightly old VBox than the additions and got some weird
rendering issues in Gnome Shell.


#### Install Mingw packages

```
$ sudo dnf install \
    automake \
    autoconf \
    byacc \
    flex \
    glib2-devel \
    icoutils \
    make \
    mingw64-gtk3 \
    mingw64-glew \
    subversion\
    xa
```

This will install a lot of packages required to cross-compile VICE, so some
patience is required.


#### Get current trunk

```
$ svn checkout --username=<sf-username> svn+ssh://<sf-username>@svn.code.sf.net/p/vice-emu/code/trunk vice-trunk
```
If you get a message about a key, just say Yes.

Create the buildsystem:
```
$ cd vice-trunk/vice
$ ./autogen.sh
```

Try to configure a minimal build of the Gtk3 port:
```
$ ./configure --enable-gtk3ui --enable-arch=no --disable-pdf-docs --host=x86_64-w64-mingw32 LDFLAGS="-lssp"
```


