# rfm12b-linux, modified for linux-sunxi

Please also have a look at the original README.md by Georg Kaindl, the original author.

A Linux kernel driver for the [RFM12B](http://www.hoperf.com/rf_fsk/fsk/21.htm) digital RF module manufactured by HopeRF. It targets embedded Linux boards that provide a SPI interface.

The driver was originally written for Raspberry Pi or Beaglebone boards.  This version is customized for linux-sunxi and has been tested on cubietruck (based on Allwinner A20 processor).

It could easily adapted to OlinuXino boards or others with the same processor.  A10/A13 boards should work without the OOK functionality (the latter requires some work to the spi driver, as it is different).

The driver can be used to communicate with [JeeLib](https://github.com/jcw/jeelib) on the Arduino platform. Its interaction with the RFM12B board is based heavily on the fantastic work in JeeLib!


## Hardware Setup

As for the Cubietruck, use the broken out spi2 pins.  I used pins 5-8 on the CN8 header (2x15), corresponding to PC19-22. 

Pin 16 (PI14) on that header is used for the external IRQ signal received from the RFM12B Module when a new byte was received or can be written to its FIFO for transmission.

My Fex file contains, thus, the following sections:

```
[gpio_para]
gpio_used = 1
gpio_num = X
...
gpio_pin_X = port:PI14<5><default><default><default>
```
(Replace X with your number of pins and the dots with whatever other GPIO pins you use).  Of course, you can use any other pin than PI14 that has an external interrupt capability (see A20 datasheet and or fex/mux documentations to find out).

```
[spi_devices]
spi_dev_num = 1

[spi_board0]
modalias = "rfm12b"
max_speed_hz = 80000000
bus_num = 2
chip_select = 0
mode = 0
full_duplex = 1

...

[spi2_para]
spi_used = 1
spi_cs_bitmap = 1
spi_cs0 = port:PC19<3><default><default><default>
spi_cs1 = port:PB13<2><default><default><default>
spi_sclk = port:PC20<3><default><default><default>
spi_mosi = port:PC21<3><default><default><default>
spi_miso = port:PC22<3><default><default><default>
```

(too bad that PB13 / cs1 is not broken out so you could use the spi bus for an additional thing...)


## Installation

1. Apply the patch spi-sun7i.patch to your kernel.  If you do not want to compile the kernel, you should probably be able to compile the file spi-sun7i.c standalone as an external kernel module. (provided you have the correct header files)
2. Compile the module.  The Makefile needs to be changed to your system!  Have a look at the "all:" and "modules_install:" targets.  Change the path to your kernel (../../linux-sunxi-cubietruck) to your kernel source (or headers) and change the INSTALL_MOD_PATH variable
3. In the Makefile, there is a target headers_install that copies the necessary headers (rfm12b_config.h, ..ioctl.h and ..jeenode.h) into a subdirectory in /usr/include/linux,  you can then reference them with `#include <linux/rfm12b/rfm12b_config.h>` etc.  
3a. The definition "BUILD_MODULE" needs to be unset in the user space header, the sed line in that target does this.
4. Run a 'depmod' on your board.
5. if you type "modprobe spi-sun7i", the rfm12b.ko module should be installed automatically, as well, when you set the modalias in your fex file as suggested.  Control with 'dmesg' if both modules are installed and you have no errors, you are good to go.
6. If you need to remove the module, remove rfm12b first and then spi-sun7i.  Otherwise, I head kernel alarms sometimes.  If you install spi-sun7i, you get an error message in your dmesg log that the chipselect 0 is already in use.  That's fine.
6a. The boardinfo for chipselect 0 is added each time spi-sun7i is installed.  This could only be resolved if the spi base kernel module is removed.  If you have it fixed built into your kernel, it may be a good idea to reboot your board after you have installed and uninstalled spi-sun7i 30 times, but it probably is not even necessary

## Changed Stuff

####rfm12b.ko
The main changes that make it work on the Allwinner platform are in the plat_spi.h file.  

The proper API to access the GPIO and the Interrupt Handlers is being used.  

Furthermore, the interrupt routine is moved to a tasklet. I found this was necessary because:
- if the interrupt was fired from the rfm12b device quickly, I found the interrupt was missed by the driver.  The LOW-LEVEL interrupt (not CHANGE-LEVEL to LOW) was not interpreted.  So I am polling the pin for a very short time and go into the interrupt directly, when the level is low, instead of unmasking.
- the rfm12b driver usually unmasks the interrupt in a callback function from the spi driver, after a message transfer is completed. So it may actually enter the interrupt from the callback.
- this resulted in some cases in a lock up of code trying to get the same spin-lock which would never be freed without entering it.  Putting the interrupt into a different tasklet thread remedied this.

####spi-sun7i.ko
Apart from the "hacks" concerning the OOK mode (see below), a few bugs and unclean removal issues were resolved.

### OOK Implementation

The rfmb12 module has a mode to do some OOK transfers by defining the macro 'RFM12B_USES_HOMEEASY' in the rfm12b_config.h file.

This is some ook to a very particular Homeeasy protocol for RF controlled plugs etc.  This is what I needed.  It is common, but you may have something different.

Also, I am using the particular frequency necessary for those modules in the ook setup.

It uses the (new and normally not used) element of the struct spi_transfer 'interbyte_usec' of the type u16.

The spi-sun7i is actually monitoring the time between turn on and offs of the transmitter when going through the workqueue.  This can be turned on and off by defining the macro 'INTER_TRANS_DELAYS'

The interbyte_usec are normally not used at all.  Now they are used to wait the adequate time between two transfers (that turn the rfm12b module on and off).

This could be solved more elegantly, maybe, by defining another element in the spi-message struct or setting an additional bit in the spi_master.flags to indicate that the master interprets this element in that particular way.  
However, this would require changes to the standard linux kernel (namely of spi.c and/or spi.h).

The OOK is controlled over the ioctl of the rfm12b driver.  Have a look at the source code and it should be clear.


## Future Work

The repartition of the code into header and other files is still quite erratic (as mentioned in the original README).

Other than that, the DEBUG information needs to be cleared.

Be aware that the driver may not work if some of the DEBUG macros are set, as the communication to the RFM12b device is time critical and writing to the 'dmesg' may hold things up for too long.
