# cutFirmware 

Rip and install der iMagic-Firmware on Linux 
============================================

Tested on Ubuntu 14.04 and 16.04 

## Prolog
<pre>
1. install the fxload package (ubuntu style)

   sudo apt-get install fxload

2. if you want to compile the simple test program you also need "gcc" and "libusb"

   sudo apt-get install libusb-dev gcc

3. plug in your iMagic device and (USB, connect trainer) and power your trainer
</pre>

## Install
<pre>

1. 'extract' (rip) the firmware from your windows driver:

  ./cutFirmware.sh [path_to/]I-magic.sys

2. Check the console output of ./cutFirmware.sh and manually load the driver

   sudo chmod 0666 /dev/bus/usb/../...
   fxload -I tacximagic.hex -D /dev/bus/usb/.../...

   => Green LED?

3. build and run the test program and check the function

   make
   ./testIMagic

   => turn the wheel, press the buttons, turn the steering frame and check the console output
</pre>

## The following is optional (but recommended) for linux 
<pre>

4. install the driver so it will be loaded if you plug in you iMagic device

  sudo cp 90-tacximagic.rules  /etc/udev/rules.d/90-tacximagic.rules
  sudo cp tacximagic.hex /lib/firmware/tacximagic.hex 

If you install the 'rules'-file together with the firmware, the
driver is already loaded (when you plug the USB) and the device-entry under 
/dev/bus/usb/... has the correct permissions (666,rw for everyone)

If you do not install the rule file, you can load the imagicfirm.hex with GoldenCheetah. 
Please make sure the USB device has the right permissions!!

5. unplug and replug the I-Magic USB and check if everything works (if you decided to install the rules file)

  => Green LED
  => ./testIMagic
</pre>
