# rfm12b-linux: linux kernel driver for the rfm12(b) RF module by HopeRF
# Copyright (C) 2013 Georg Kaindl
# 
# This file is part of rfm12b-linux.
# 
# rfm12b-linux is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# rfm12b-linux is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with rfm12b-linux.  If not, see <http://www.gnu.org/licenses/>.
#

obj-m += rfm12b.o 


KVERSION := $(shell uname -r)

#INCLUDE += -I../../linux-sunxi-cubietruck/include

HOSTCC       = gcc-4.8
HOSTCXX      = g++-4.8


all:
	make -C ../../linux-sunxi-cubietruck M=$(PWD) modules
clean:
	make -C ../../linux-sunxi-cubietruck M=$(PWD) clean

modules_install:
	INSTALL_MOD_PATH=/Volumes/linux-sunxi-kernel/linux-sunxi-cubietruck/output make -C ../../linux-sunxi-cubietruck M=$(PWD) modules_install

SSH?=ssh
REMOTE_HOST=root@cubie2.local
HDR_INSTALL_DIR=output/rfm12b

headers_install:
	sed 's/\(BUILD_MODULE[ \t]*\)1/\10/g' rfm12b_config.h > $(HDR_INSTALL_DIR)/rfm12b_config.h
	cp rfm12b_ioctl.h $(HDR_INSTALL_DIR)
	cp rfm12b_jeenode.h $(HDR_INSTALL_DIR)
	@echo "Copy headers to $(REMOTE_HOST):/usr/include/linux"
	@rsync -avz -e $(SSH) $(HDR_INSTALL_DIR) $(REMOTE_HOST):/usr/include/linux
