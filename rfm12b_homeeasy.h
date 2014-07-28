/*
 * rfm12b_homeeasy.h
 *
 *  Created on: 27.07.2014
 *      Author: hoellinm
 */

#ifndef RFM12B_HOMEEASY_H_
#define RFM12B_HOMEEASY_H_

#define MAX_UDELAY_US 	1000*MAX_UDELAY_MS
// #define USLEEP_AUTORANGE(u) 	(u)-(u)/20,(u)+(u)/20

// define constants for protocol in usec
#define MSG_BREAK		10000
#define PREAMBLE_STROBE	2650
#define XMIT_ON_TIME	235
#define BIT_ON			1180
#define	BIT_OFF			275



static int
rfm12_reset(struct rfm12_data* rfm12);
struct spi_transfer
rfm12_make_spi_transfer(uint16_t cmd, u8* tx_buf, u8* rx_buf);


static int
rfm12_he_setup(struct rfm12_data* rfm12)
{

	int err=0;  // assume success
	struct spi_rfm12_active_board* brd;

	if(rfm12->homeeasy_active)
	{
	   struct spi_transfer tr, tr2, tr3;
	   struct spi_message msg;
	   u8 tx_buf[26];

	   rfm12->state = RFM12_STATE_CONFIG;  // state will be preserved during whole OOK phase
	   printk(KERN_INFO RFM12B_DRV_NAME " : setting up OOK mode\n");

	   spi_message_init(&msg);

	   tr = rfm12_make_spi_transfer(RF_READ_STATUS, tx_buf+0, NULL);
	   tr.cs_change = 1;
	   spi_message_add_tail(&tr, &msg);

	   tr2 = rfm12_make_spi_transfer(RF_SLEEP_MODE, tx_buf+2, NULL);
	   tr2.cs_change = 1;
	   spi_message_add_tail(&tr2, &msg);

	   tr3 = rfm12_make_spi_transfer(RF_TXREG_WRITE, tx_buf+4, NULL);
	   spi_message_add_tail(&tr3, &msg);

	   err = spi_sync(rfm12->spi, &msg);


	   if (err)
	     goto pError;

	   brd = platform_irq_identifier_for_spi_device(rfm12->spi->master->bus_num,rfm12->spi->chip_select);
	   if(!brd) {
		   printk(KERN_INFO RFM12B_DRV_NAME " : no board found for bus_num:%d, chipselect:%d\n",
				   rfm12->spi->master->bus_num, rfm12->spi->chip_select);
		   goto pError;
	   }
	   brd->irqchip->irq_mask(brd->irqchip_data);  // disable interrupts

	   msleep(OPEN_WAIT_MILLIS);

	   // Assumed to have been configure before
	   spi_message_init(&msg);

	   tr = rfm12_make_spi_transfer(0x80C7 |
	         ((rfm12->band_id & 0xff) << 4), tx_buf+0, NULL);
	   tr.cs_change = 1;
	   spi_message_add_tail(&tr, &msg);
	   // change to A620 for OOK

	   tr2 = rfm12_make_spi_transfer(0xA620, tx_buf+2, NULL);
	   tr2.cs_change = 1;
	   spi_message_add_tail(&tr2, &msg);


	   err = spi_sync(rfm12->spi, &msg);
	   if (err)
    	 goto resetInterrupt;

	     spi_message_init(&msg);

	     tr = rfm12_make_spi_transfer(RF_READ_STATUS, tx_buf+0, NULL);
	     spi_message_add_tail(&tr, &msg);

	     err = spi_sync(rfm12->spi, &msg);

	   if (err)
    	 goto resetInterrupt;

	     printk(KERN_INFO RFM12B_DRV_NAME " : set up successfully\n");

	   return err;
	}

	printk(KERN_INFO RFM12B_DRV_NAME " : leaving OOK mode\n");

	brd = platform_irq_identifier_for_spi_device(rfm12->spi->master->bus_num,rfm12->spi->chip_select);
	if(!brd)
		goto pError;

resetInterrupt:
	if(brd->state.irq_enabled)  // enable interrupt if previously on
		brd->irqchip->irq_unmask(brd->irqchip_data);



pError:
	printk(KERN_INFO RFM12B_DRV_NAME " : leaving OOK mode with errStat: %d",err);
	rfm12->state = RFM12_STATE_IDLE;
	rfm12->homeeasy_active=0;  // in case we got here because of an error
	rfm12_reset(rfm12);  // reset if turned off or error
	return err;


}


static void
rfm12_safe_udelay(unsigned long usecs)
{
        while (usecs > MAX_UDELAY_US) {
                udelay(MAX_UDELAY_US);
                usecs -= MAX_UDELAY_US;
        }
        udelay(usecs);
}

static int
rfm12_ook_transfer(struct rfm12_data* rfm12, unsigned high, unsigned low) {
	struct spi_transfer tr,tr2;
	struct spi_message msg;
	int err;
	u8 tx_buf[4];
	spi_message_init(&msg);

	tr=rfm12_make_spi_transfer(RF_XMITTER_ON,tx_buf,NULL);
	tr.cs_change = 1;
	tr.delay_usecs = (u16)high;
	spi_message_add_tail(&tr,&msg);

	tr2=rfm12_make_spi_transfer(RF_IDLE_MODE,tx_buf+2,NULL);
	tr.cs_change = 1;
	tr.delay_usecs = 0;
	err = spi_sync(rfm12->spi,&msg);
	if (err)
		return err;
	rfm12_safe_udelay(low);
	return 0;
}

static int
rfm12_he_write(struct rfm12_data* rfm12, u32 command)
{
	int messages = 0;
	int bit = 31;
	int err = 0;

	if(!rfm12->homeeasy_active)
		return -EACCES;

	printk(KERN_INFO RFM12B_DRV_NAME " : would now write %04x in OOK mode\n",command);
	for( ;messages < 5; messages++)
	{
		rfm12_ook_transfer(rfm12,XMIT_ON_TIME,PREAMBLE_STROBE);
		do
		{
			if(command & (1<<bit))
			{
				err = rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON);
				err = rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF);
			}
			else
			{
				err = rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF);
				err = rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON);
			}
			if(err)
				return err;
		} while(bit);
		rfm12_safe_udelay(MSG_BREAK);
		bit = 31;

	}
	return 0;
}



#endif /* RFM12B_HOMEEASY_H_ */
