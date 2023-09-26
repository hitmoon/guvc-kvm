## guvc-kvm
### a simple kvm tool for linux platform based on guvcview

### 实现方式
**视频:**  MS2130 USB 3.0 HDMI 视频采集卡

**键盘 & 鼠标:** CH9329+CH340 串口转USB HID键鼠线

假设:
- 被控电脑 A
- 内网电脑 B

连接方式:
- 采集卡一端连接 A 的HDMI 输出，另一端连接到 B 的USB 3.0 接口
- CH9329(大头) 插入A，另一端插入B
- 远程访问电脑B(Linux + vnc), 打开guvc-kvm, 就可以操纵内网的电脑A了

### 使用截图
![image](https://github.com/hitmoon/guvc-kvm/blob/kvm/guvc-kvm-1.png)

### 编译安装
参照下面的文档即可

*************************

GTK UVC VIEWER (guvcview)
*************************

Basic Configuration
===================
Dependencies:
-------------

Guvcview depends on the following:
 - intltool,
 - autotools, 
 - libsdl2 and/or sfml, 
 - libgtk-3 or libqt5, 
 - portaudio19, 
 - libpng, 
 - libavcodec, 
 - libavutil, 
 - libv4l, 
 - libudev,
 - libusb-1.0,
 - libpulse (optional)
 - libgsl0 (optional)

On most distributions you can just install the development 
packages:
 intltool, autotools-dev, libsdl2-dev, libsfml-dev, libgtk-3-dev or qtbase5-dev, 
 portaudio19-dev, libpng12-dev, libavcodec-dev, libavutil-dev,
 libv4l-dev, libudev-dev, libusb-1.0-0-dev, libpulse-dev, libgsl0-dev

Build configuration:
--------------------
(./bootstrap.sh; ./configure)

The configure script is generated from configure.ac by autoconf,
the helper script ./bootstrap.sh can be used for this, it will also
run the generated configure with the command line options passed.
After configuration a simple 'make && make install' will build and
install guvcview and all the associated data files.

guvcview will build with Gtk3 support by default, if you want to use 
the Qt5 interface instead, just run ./configure --disable-gtk3 --enable-qt5
you can use SDL2 (enabled by default) and/or SFML (disabled by default) 
as the rendering engine, both engines can be enabled during configure 
so that you can choose between the two with a command line option.
 

Data Files:
------------
(language files; image files; gnome menu entry)

guvcview data files are stored by default to /usr/local/share
setting a different prefix (--prefix=BASEDIR) during configuration
will change the installation path to BASEDIR/share.

Built files, src/guvcview and data/gnome.desktop, are dependent 
on this path, so if a new prefix is set a make clean is required 
before issuing the make command. 

After running the configure script the normal, make && make install 
should build and install all the necessary files.    
    
 
guvcview bin:
-------------
(guvcview)

The binarie file installs to the standart location,
/usr/local/bin, to change the install path, configure
must be executed with --prefix=DIR set, this will cause
the bin file to be installed in DIR/bin, make sure 
DIR/bin is set in your PATH variable, or the gnome 
menu entry will fail.

guvcview libraries:
-------------------
(libgviewv4l2core, libgviewrender, libgviewaudio, libgviewencoder)

The core functionality of guvcview is now split into 4 libraries
these will install to ${prefix}/lib and development headers to
${prefix}/include/guvcview-2/libname. 
pkg-config should be use to determine the compile flags.


guvcview.desktop:
-----------------

(data/guvcview.desktop)

The desktop file (gnome menu entry) is built from the
data/guvcview.desktop.in definition and is dependent on the 
configure --prefix setting, any changes to this, must 
be done in data/guvcview.desktop.in.

configuration files:
--------------------
(~/.config/guvcview2/video0)

The configuration file is saved into the $HOME dir when 
exiting guvcview. If a video device with index > 0,
e.g: /dev/video1 is used then the file stored will be
named ~/.config/guvcview2/video1

Executing guvcview
================== 

For instructions on the command line args 
execute "guvcview --help".
