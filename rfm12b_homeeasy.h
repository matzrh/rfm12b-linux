/*
 * rfm12b_homeeasy.h
 *
 *  Created on: 27.07.2014
 *      Author: hoellinm
 */

#ifndef RFM12B_HOMEEASY_H_
#define RFM12B_HOMEEASY_H_

#include<linux/time.h>
#include <linux/list.h>

#define MAX_UDELAY_US 	1000*MAX_UDELAY_MS
// #define USLEEP_AUTORANGE(u) 	(u)-(u)/20,(u)+(u)/20

// define constants for protocol in usec
#define MSG_BREAK		9900
#define PREAMBLE_STROBE	2650
#define XMIT_ON_TIME	275
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

/*
static void
rfm12_wait_for_nsec(struct timespec* time_threshold)
{
	struct timespec tval;
	do {
		getnstimeofday(&tval);
	} while ((time_threshold->tv_sec > tval.tv_sec) || (time_threshold->tv_nsec > tval.tv_nsec));
}

static void
rfm12_add_usec(long usec, struct timespec* tval)
{
	long new_nsec = tval->tv_nsec+usec* (NSEC_PER_USEC);
	tval->tv_sec += new_nsec / (NSEC_PER_SEC);
	tval->tv_nsec = new_nsec % (NSEC_PER_SEC);
}
*/

static void rfm12_safe_udelay(unsigned long usecs)
{
        while (usecs > MAX_UDELAY_US) {
                udelay(MAX_UDELAY_US);
                usecs -= MAX_UDELAY_US;
        }
        udelay(usecs);
}




static struct spi_transfer*
rfm12_ook_transfer(struct rfm12_data* rfm12, unsigned high, unsigned low, struct spi_message* msg, struct spi_transfer* tr, u8* buf) {

	tr->tx_buf=buf;
	tr->rx_buf=NULL;
	tr->len=2;
	tr->cs_change = 1;
	tr->interbyte_usecs = high;
	tr->bits_per_word = 0;
	tr->speed_hz = 0;
	tr->delay_usecs = 0;

	spi_message_add_tail(tr++,msg);

	tr->tx_buf=buf+2;
	tr->rx_buf=NULL;
	tr->len=2;
	tr->cs_change = 1;
	tr->interbyte_usecs = low;
	tr->bits_per_word = 0;
	tr->speed_hz = 0;
	tr->delay_usecs = 0;

	spi_message_add_tail(tr,msg);

	return tr;

}

/*
static void
rfm12_msg_debug(struct spi_message *msg){
	struct spi_transfer* tr = NULL;
    list_for_each_entry(tr, &msg->transfers, transfer_list) {
    	printk(KERN_INFO RFM12B_DRV_NAME " : parameters: len:%d, interbyte_udelay:%d\n", tr->len, tr->interbyte_usecs);
    	if(tr->len >1) {
    		int value = *((u8*) tr->tx_buf)<<8;
    		value |= *((u8*) tr->tx_buf+1);
    		printk(KERN_INFO RFM12B_DRV_NAME " : Command: %04x",value);
    	}
    }

}
*/

#define	TX_OOK_BYTES 32
#define TX_OOK_NO    (TX_OOK_BYTES * 4 + 2)

static int
rfm12_he_write(struct rfm12_data* rfm12, u32 command)
{
	int messages = 0;
	int bit = TX_OOK_BYTES - 1;
	int err = 0;
	struct spi_transfer* ret_tr;
	struct spi_transfer* tr;
	struct spi_message msg;
	u8 tx_buf[4];
	if(!rfm12->homeeasy_active)
		return -EACCES;

	tr = kzalloc(TX_OOK_NO * sizeof(struct spi_transfer),GFP_KERNEL);

	// put into IDLE mode before starting transfer
	spi_message_init(&msg);
	*tr=rfm12_make_spi_transfer(RF_IDLE_MODE,tx_buf,NULL);
	tr->cs_change = 1;
	spi_message_add_tail(tr,&msg);
	err = spi_sync(rfm12->spi,&msg);
	if(err)
		return err;

	printk(KERN_INFO RFM12B_DRV_NAME " : would now write %04x in OOK mode\n",command);


	// make the full message with all commands
	// preliminary, hacked code:   txbuf is never changed, as RF_XMITTER_ON and RF_IDLE_MODE are always sent
	// would not work if buf changed for each ook_transfer
	tx_buf[0]=(RF_XMITTER_ON & 0xff00) >> 8;
	tx_buf[1]=RF_XMITTER_ON & 0xff;
	tx_buf[2]=(RF_IDLE_MODE & 0xff00) >> 8;
	tx_buf[3]=RF_IDLE_MODE & 0xff;
	spi_message_init(&msg);

	ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME,PREAMBLE_STROBE, &msg, tr, tx_buf);
//	printk(KERN_INFO RFM12B_DRV_NAME " : preamble pointer pos: %d\n", (int) (ret_tr-tr));
	do
	{
		if(command & (1<<bit))
		{
			ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON, &msg, &tr[4*(TX_OOK_BYTES-bit)-2], tx_buf);
//			printk(KERN_INFO RFM12B_DRV_NAME " : bit %2d _a_ pointer pos: %4d\n", bit, (int) (ret_tr-tr));
			ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF, &msg, &tr[4*(TX_OOK_BYTES-bit)], tx_buf);
//			printk(KERN_INFO RFM12B_DRV_NAME " : bit %2d _b_ pointer pos: %4d\n", bit, (int) (ret_tr-tr));
		}
		else
		{
			ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF, &msg, &tr[4*(TX_OOK_BYTES-bit)-2], tx_buf);
//			printk(KERN_INFO RFM12B_DRV_NAME " : bit %2d _a_ pointer pos: %4d\n", bit, (int) (ret_tr-tr));
			ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON, &msg, &tr[4*(TX_OOK_BYTES-bit)], tx_buf);
//			printk(KERN_INFO RFM12B_DRV_NAME " : bit %2d _b_ pointer pos: %4d\n", bit, (int) (ret_tr-tr));
		}
	} while(bit--);
//	rfm12_msg_debug(&msg);

	// now send it 5 times with MSG_BREAK delay in between
	for( ;messages < 5; messages++)
	{
		err = spi_sync(rfm12->spi, &msg);
		if(err)
			return err;
		rfm12_safe_udelay(MSG_BREAK);
	}

	// put into sleep mode when done
	spi_message_init(&msg);
	*tr=rfm12_make_spi_transfer(RF_SLEEP_MODE,tx_buf,NULL);
	tr->cs_change = 1;
	spi_message_add_tail(tr,&msg);
	err = spi_sync(rfm12->spi,&msg);
	if(err)
		return err;
	kfree(tr);

	return 0;

}



#endif /* RFM12B_HOMEEASY_H_ */
