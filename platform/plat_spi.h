/* rfm12b-linux: linux kernel driver for the rfm12(b) RF module by HopeRF
*  Copyright (C) 2013 Georg Kaindl
*  
*  This file is part of rfm12b-linux.
*  
*  rfm12b-linux is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*  
*  rfm12b-linux is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*  
*  You should have received a copy of the GNU General Public License
*  along with rfm12b-linux.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   This is the implementation of platform.h for normal SPI-master drivers,
   using the kernel's SPI-master interface. See platform.h for extra info.
   
   We don't use thr SPI driver's IRQ functionality, but handle it ourselves.
*/

#if !defined(__RFM12_PLAT_SPI_H__)
#define __RFM12_PLAT_SPI_H__

#define NUM_RFM12_BOARDS   1
/* change for different platforms */

#define GPIO_CHIP_ID "A1X_GPIO"
//#define RFM12_PLAT_DBG
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spi/spi.h>

struct spi_rfm12_active_board {
   u16 irq;
   u16 no_enable, no_direct, min_poll;
   void* irq_data;
   struct gpio_chip* gpiochip;
   struct irq_chip* irqchip;
   struct irq_data* irqchip_data;
   struct spi_device* spi_device;
   struct tasklet_struct* rfm12_do_interrupt;
   int idx;
   struct {
      u8 gpio_claimed:1;
      u8 irq_claimed:1;
      u8 irq_enabled:1;
   } state;
};

struct spi_rfm12_board_config board_configs[NUM_RFM12_BOARDS] = {
   {
      .irq_pin      = 7,   // gpio 7
      .spi_bus      = 2,   // spi port on P1 header
      .spi_cs      = 0   // CS 0
   }
};

static struct spi_rfm12_active_board active_boards[NUM_RFM12_BOARDS];





static int
spi_rfm12_init_pinmux_settings(void) {return 0;};  //pins defined in fex file
static int
spi_rfm12_cleanup_pinmux_settings(void) {return 0;};

static irqreturn_t
spi_rfm12_irq_handler(int irq, void* dev_id);

static int
spi_rfm12_setup_irq_pins(void);
static int
spi_rfm12_cleanup_irq_pins(void);

static int
spi_rfm12_register_spi_devices(void);  //pinmux handled by fex files
static int
spi_rfm12_deregister_spi_devices(void);

static void
spi_rfm12_schedule_interrupt(unsigned long arg)
{
	struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*) arg;
	// printk(KERN_INFO RFM12B_DRV_NAME " : interrupt scheduled for board %d\n",brd->idx);
	rfm12_handle_interrupt((struct rfm12_data*) brd->irq_data);
}

static irqreturn_t
spi_rfm12_irq_handler(int irq, void* dev_id)
{
   struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)dev_id;
   if (NULL != brd->irq_data) {   
      if (brd->state.irq_enabled) {
         brd->state.irq_enabled = 0;
         brd->irqchip->irq_mask(brd->irqchip_data);
      }
      // printk(KERN_INFO RFM12B_DRV_NAME " : will go into tasklet from IRQ \n");
      tasklet_schedule(brd->rfm12_do_interrupt);
      // rfm12_handle_interrupt((struct rfm12_data*)brd->irq_data);
   }
   
   return IRQ_HANDLED;
}

static int
platform_irq_direct_call(struct spi_rfm12_active_board* brd)
{
	struct spi_rfm12_board_config* cfg = &board_configs[brd->idx];
	return (0 == brd->gpiochip->get(brd->gpiochip,cfg->irq_pin - brd->gpiochip->base));
}


static int
platform_irq_handled(void* identifier)
{
   struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)identifier;
   u16 poll_int=0x9f;
      
   if (0 == brd->state.irq_enabled) {
      while (!platform_irq_direct_call(brd) && --poll_int);
      if (poll_int){
    	 if (NULL != brd->irq_data) {

#ifdef RFM12_PLAT_DBG
    		  if(brd->state.irq_enabled)
    			  printk(KERN_INFO RFM12B_DRV_NAME " : something strange with irqs...\n");
    		  else
    			  printk(KERN_INFO RFM12B_DRV_NAME " : straight back into irq handler, %04x\n",poll_int);
#endif
    		  brd->no_direct++;
    		  if(poll_int < brd->min_poll)
    			  brd->min_poll=poll_int;
    		  tasklet_schedule(brd->rfm12_do_interrupt);

//    	  rfm12_handle_interrupt((struct rfm12_data*)brd->irq_data);
    	 }
      }
      else {
#ifdef RFM12_PLAT_DBG
    	  printk(KERN_INFO RFM12B_DRV_NAME " : enable irq...\n");
#endif
         brd->state.irq_enabled = 1;
         brd->irqchip->irq_unmask(brd->irqchip_data);
         brd->no_enable++;
      }
   }

   return 0;
}

static int is_right_chip(struct gpio_chip *chip, const void *data)
{
        if (strcmp(data, chip->label) == 0)
                return 1;
        return 0;
}

#ifdef RFM12B_DEBUG
static int
spi_rfm12_return_irq_enabled(void *identifier)
{
	struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)identifier;
	return brd->state.irq_enabled;
}

static int
spi_rfm12_return_irq_pinstate(void *identifier)
{
	struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)identifier;
	return !platform_irq_direct_call(brd);
}
#endif

static int
spi_rfm12_setup_irq_pins(void)
{
   int err, i;
   struct gpio_chip* t_gpiochip;
   struct irq_data* t_irqdata;
   err=0;
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
#ifdef USE_SPI_IRQ
	   active_boards[i].irq = board_configs[i].irq_pin;
	   err = (active_boards[i].irq < 0) ;
	   if(err)
		   goto gpioErrReturn;
	   active_boards[i].state.gpio_claimed = 0; //gpio claimed by spi master
#else
	  unsigned offset;
	  t_gpiochip=gpiochip_find(GPIO_CHIP_ID,is_right_chip);
	  if(!t_gpiochip)
		  goto errReturn;
	  active_boards[i].gpiochip=t_gpiochip;
	  offset=board_configs[i].irq_pin - t_gpiochip->base;
#ifdef RFM12_PLAT_DBG
	  printk(KERN_INFO RFM12B_DRV_NAME ": trying to claim new in pin index %d: %s\n",board_configs[i].irq_pin,t_gpiochip->names[offset]);
#endif
	  err = gpio_request(board_configs[i].irq_pin, RFM12B_DRV_NAME " ir/in");

      if (0 != err) {
         printk(KERN_ALERT RFM12B_DRV_NAME
            " : unable to obtain GPIO pin %u.\n",
            board_configs[i].irq_pin
         );
         
         goto errReturn;
      }
      
      active_boards[i].state.gpio_claimed = 1;
      t_gpiochip->direction_input(t_gpiochip,offset);  // set input
#ifdef RFM12_PLAT_DBG
      printk(KERN_INFO RFM12B_DRV_NAME " : value of IRQ %d",t_gpiochip->get(t_gpiochip,offset));
#endif
      err = t_gpiochip->to_irq(t_gpiochip,offset);
      if (err < 0) {
         printk(
            KERN_ALERT RFM12B_DRV_NAME
            " : unable to obtain IRQ for GPIO pin %u: %i.\n",
            board_configs[i].irq_pin, err
         );

         goto gpioErrReturn;
      }
      active_boards[i].irq = (u16)err;
      t_irqdata = irq_get_irq_data((u16)err);
      if (t_irqdata && t_irqdata->chip) {
    	  active_boards[i].irqchip_data=t_irqdata;
    	  active_boards[i].irqchip = t_irqdata->chip;
      } else {
          err = -ENODEV;
          goto gpioErrReturn;
      }

#ifdef RFM12_PLAT_DBG
      printk(KERN_INFO RFM12B_DRV_NAME "obtained irq %d for pin %d\n", active_boards[i].irq , board_configs[i].irq_pin);
#endif
      
#endif
   }
   
   err = 0;
   return err;

gpioErrReturn:
   while (i >= 0) {
      gpio_free(board_configs[i].irq_pin);
      active_boards[i].state.gpio_claimed = 0;
      i--;
   }

errReturn:   
   return err;
}

static int
spi_rfm12_cleanup_irq_pins(void)
{
   int i;
   
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      (void)platform_irq_cleanup(&active_boards[i]);
      if (active_boards[i].state.gpio_claimed) {
         gpio_free(board_configs[i].irq_pin);
         active_boards[i].irqchip=NULL;
         active_boards[i].irqchip_data=NULL;
         active_boards[i].state.gpio_claimed = 0;
      }
   }
   
   return 0;
}

static int
platform_module_init(void)
{
   int err, i;

      
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      active_boards[i].idx = i;
      active_boards[i].rfm12_do_interrupt = kmalloc(sizeof(struct tasklet_struct),GFP_KERNEL);
      tasklet_init(active_boards[i].rfm12_do_interrupt, spi_rfm12_schedule_interrupt, (unsigned long) &active_boards[i]);
   }
   printk(KERN_INFO RFM12B_DRV_NAME " : tasklets defined\n");
   err = spi_rfm12_init_pinmux_settings();
   if (0 != err) goto muxFailed;
   
   err = spi_rfm12_setup_irq_pins();
   if (0 != err) goto irqFailed;
   
   err = spi_rfm12_register_spi_devices();
   if (0 != err) goto spiFailed;
   
   return err;

spiFailed:
   spi_rfm12_cleanup_irq_pins();
irqFailed:
   spi_rfm12_cleanup_pinmux_settings();
muxFailed:
   return err;
}

static int
platform_module_cleanup(void)
{
	int i;
   (void)spi_rfm12_cleanup_pinmux_settings();
   (void)spi_rfm12_cleanup_irq_pins();
   (void)spi_rfm12_deregister_spi_devices();
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      tasklet_kill(active_boards[i].rfm12_do_interrupt);
      kfree(active_boards[i].rfm12_do_interrupt);
   }
   printk(KERN_INFO RFM12B_DRV_NAME " : tasklets freed\n");
   return 0;
}

static void*
platform_irq_identifier_for_spi_device(u16 spi_bus, u16 spi_cs)
{
   int i;
   
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      if (spi_bus == board_configs[i].spi_bus &&
         spi_cs == board_configs[i].spi_cs)
         return &active_boards[i];
   }
   
   return NULL;
}

int
platform_irq_return_pin_value(u16 spi_bus, u16 spi_cs)
{
	struct gpio_chip* active_chip;
	int offset;

	int i;
	for (i=0; i<NUM_RFM12_BOARDS; i++) {
		if (spi_bus == board_configs[i].spi_bus &&
	         spi_cs == board_configs[i].spi_cs) {
			active_chip=active_boards[i].gpiochip;
			offset=board_configs[i].irq_pin - active_chip->base;
			return active_chip->get(active_chip,offset);

		}

	}

	return -1;

}

int
platform_irq_test_enabled(u16 spi_bus, u16 spi_cs)
{
	int i;
	for (i=0; i<NUM_RFM12_BOARDS; i++) {
		if (spi_bus == board_configs[i].spi_bus &&
	         spi_cs == board_configs[i].spi_cs) {
			return active_boards[i].state.irq_enabled;
		}
	}

	return -1;

}


static int
platform_irq_init(void* identifier, void* rfm12_data)
{
   int err;
   // spinlock_t lock;
  //  unsigned long flags;
   struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)identifier;
   struct spi_rfm12_board_config* cfg = &board_configs[brd->idx];

   if (brd->state.irq_claimed)
      return -EBUSY;

   err = request_any_context_irq(brd->irq,
                         (irq_handler_t) spi_rfm12_irq_handler, IRQF_TRIGGER_FALLING | IRQF_DISABLED,
                         RFM12B_DRV_NAME, (void*) brd);

   // spin_lock_irqsave(&lock, flags);

   /* GPIO Pin Falling/Rising Edge Detect Enable */
//   brd->irqchip->irq_set_type(brd->irqchip_data,
  //                        IRQ_TYPE_EDGE_FALLING);

//   brd->irqchip->irq_unmask(brd->irqchip_data);
   // spin_unlock_irqrestore(&lock, flags);


   if (0 <= err) {
      brd->state.irq_claimed = 1;
      brd->state.irq_enabled = 1;
      brd->no_direct =0;
      brd->no_enable=0;
      brd->min_poll=0xff;
      brd->irq_data = rfm12_data;      
   } else
      printk(
         KERN_ALERT RFM12B_DRV_NAME
         " : unable to activate IRQ %u: %i.\n",
         brd->irq, err
      );
#ifdef RFM12_PLAT_DBG
   printk( KERN_INFO RFM12B_DRV_NAME " : successfully activated IRQ %u \n", brd->irq);
#endif

   if (0 == brd->gpiochip->get(brd->gpiochip,cfg->irq_pin - brd->gpiochip->base))
      spi_rfm12_irq_handler(brd->irq, (void*)brd);

   return err;   
}

static int
platform_irq_cleanup(void* identifier)
{
   int err = 0;   
   struct spi_rfm12_active_board* brd = (struct spi_rfm12_active_board*)identifier;
   
   if (brd->state.irq_claimed) {
      if (brd->state.irq_enabled) {
         disable_irq(brd->irq);
         brd->state.irq_enabled = 0;
      }
      
      free_irq(brd->irq, (void*)brd);
      brd->state.irq_claimed = 0;
#ifdef RFM12_PLAT_DBG
      printk(KERN_INFO RFM12B_DRV_NAME " : direct calls: %d, irq enabled: %d, ,min_poll: %04x\n",brd->no_direct,brd->no_enable, brd->min_poll);
#endif
      brd->irq_data = NULL;
   }
   
   return err;
}

static int
spi_rfm12_register_spi_devices(void)
{
   int i, err = 0;
   struct spi_master* spi_master;
   struct spi_device* spi_device;
   struct device* sdev;
   char buf[128];
   
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      spi_master = spi_busnum_to_master(board_configs[i].spi_bus);
      if (NULL == spi_master) {
         err = -ENODEV;
         printk(
            KERN_ALERT RFM12B_DRV_NAME
               " : no spi_master found for busnum %u.\n",
               board_configs[i].spi_bus
         );
         
         goto errReturn;
      }
      
      spi_device = spi_alloc_device(spi_master);
      if (NULL == spi_device) {
         printk(
            KERN_ALERT RFM12B_DRV_NAME
               " : spi_alloc_device() failed.\n"
         );
         err = -ENOMEM;
         goto errReturn;
      }
      
      spi_device->chip_select = board_configs[i].spi_cs;
      
      snprintf(
         buf,
         sizeof(buf),
         "%s.%u", 
         dev_name(&spi_device->master->dev),
         spi_device->chip_select
      );
      
      // if a driver is already registered for our chipselect, try
      // to unregister it, and retry. fail if unregistering didn't
      // work for some reason.
      sdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buf);
      if (NULL != sdev) {
         spi_unregister_device((struct spi_device*)sdev);
         spi_dev_put((struct spi_device*)sdev);
      }
      
      sdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buf);
      if (NULL != sdev) {
         spi_dev_put(spi_device);

         printk(
            KERN_ALERT RFM12B_DRV_NAME
               " : driver [%s] already registered for [%s], can't "
               "unregister it\n",
               (sdev->driver && sdev->driver->name) ?
                  sdev->driver->name : "unknown",
               buf
         );

         err = -EBUSY;
         goto errReturn;
      }
      
      spi_device->max_speed_hz = RFM12B_SPI_MAX_HZ;
      spi_device->mode = RFM12B_SPI_MODE;
      spi_device->bits_per_word = RFM12B_SPI_BITS;
      spi_device->chip_select = board_configs[i].spi_cs;
#ifdef USE_SPI_IRQ
      board_configs[i].irq_pin = spi_device->irq;
#else
      spi_device->irq = -1; /* we do our own interrupt handling */
#endif
      spi_device->controller_state = NULL;
      spi_device->controller_data = NULL;
      strlcpy(spi_device->modalias, RFM12B_DRV_NAME, SPI_NAME_SIZE);
      
      err = spi_add_device(spi_device);
      if (0 != err) {
         spi_dev_put(spi_device);
         
         printk(
            KERN_ALERT RFM12B_DRV_NAME
               " : failed to register SPI device: %i\n",
               err
         );
      } else
         active_boards[i].spi_device = spi_device;
      
      put_device(&spi_master->dev);
      spi_master = NULL;
   }
   
   return err;
   
errReturn:
   if (NULL != spi_master)
      put_device(&spi_master->dev);

   (void)spi_rfm12_deregister_spi_devices();

   return err;
}

static int
spi_rfm12_deregister_spi_devices(void)
{
   int i;
   
   for (i=0; i<NUM_RFM12_BOARDS; i++) {
      if (NULL != active_boards[i].spi_device) {
         spi_unregister_device(active_boards[i].spi_device);
         spi_dev_put(active_boards[i].spi_device);
         
         active_boards[i].spi_device = NULL;
      }
   }
   
   return 0;
}

#endif // __RFM12_PLAT_SPI_H__
