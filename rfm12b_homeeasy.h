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
// motor AC114
#define LPREAMBLE_STROBE 6080
#define LPREAMBLE_LOW	430
#define LHIGH_ON		900
#define LHIGH_OFF		20
#define LLOW_ON			510
#define LLOW_OFF		340
#define LEND_OFF		260
//HomeEasy Advanced
#define MSG_BREAK		 10000
#define PREAMBLE_STROBE	 2650
#define XMIT_ON_TIME	325
#define BIT_ON			1130
#define	BIT_OFF			225
#define SIMPLE_SHORT	960
#define SIMPLE_LONG		320


static int
rfm12_reset(struct rfm12_data* rfm12);
struct spi_transfer
rfm12_make_spi_transfer(uint16_t cmd, u8* tx_buf, u8* rx_buf);


static int
rfm12_he_setup(struct rfm12_data* rfm12)
{

	int err=0;  // assume success

	if(rfm12->homeeasy_active)
	{
		struct spi_transfer tr, tr2, tr3, tr4, tr5, tr6, tr7, tr8, tr9, tr10,
	   	   tr11, tr12, tr13;
		struct spi_message msg;
		u8 tx_buf[26];

		rfm12->state = RFM12_STATE_CONFIG;  // state will be preserved during whole OOK phase
		// printk(KERN_INFO RFM12B_DRV_NAME " : setting up OOK mode\n");
		// rfm12_cancel_idle_watchdog(rfm12);
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


		msleep(OPEN_WAIT_MILLIS);

		// ok, we're now ready to be configured.
	    spi_message_init(&msg);

	    tr = rfm12_make_spi_transfer(0x80C7 |
	          ((rfm12->band_id & 0xff) << 4), tx_buf+0, NULL);
	    tr.cs_change = 1;
	    spi_message_add_tail(&tr, &msg);
	    // change to A620 for OOK

	    tr2 = rfm12_make_spi_transfer(0xA640, tx_buf+2, NULL);
	    tr2.cs_change = 1;
	    spi_message_add_tail(&tr2, &msg);

	    tr3 = rfm12_make_spi_transfer(0xC600 | rfm12->bit_rate, tx_buf+4, NULL);
	    tr3.cs_change = 1;
	    spi_message_add_tail(&tr3, &msg);

	    tr4 = rfm12_make_spi_transfer(0x94A2, tx_buf+6, NULL);
	    tr4.cs_change = 1;
	    spi_message_add_tail(&tr4, &msg);

	    tr5 = rfm12_make_spi_transfer(0xC2AC, tx_buf+8, NULL);
	    tr5.cs_change = 1;
	    spi_message_add_tail(&tr5, &msg);

	    tr6 = rfm12_make_spi_transfer(0xCA8B, tx_buf+10, NULL);
	    tr6.cs_change = 1;
	    spi_message_add_tail(&tr6, &msg);

	    tr7 = rfm12_make_spi_transfer(0xCE2D, tx_buf+12, NULL);
	    tr7.cs_change = 1;
	    spi_message_add_tail(&tr7, &msg);


	    tr8 = rfm12_make_spi_transfer(0xC483, tx_buf+14, NULL);
	    tr8.cs_change = 1;
	    spi_message_add_tail(&tr8, &msg);

	    tr9 = rfm12_make_spi_transfer(0x9850, tx_buf+16, NULL);
	    tr9.cs_change = 1;
	    spi_message_add_tail(&tr9, &msg);

	    tr10 = rfm12_make_spi_transfer(0xCC77, tx_buf+18, NULL);
	    tr10.cs_change = 1;
	    spi_message_add_tail(&tr10, &msg);

	    tr11 = rfm12_make_spi_transfer(0xE000, tx_buf+20, NULL);
	    tr11.cs_change = 1;
	    spi_message_add_tail(&tr11, &msg);

	    tr12 = rfm12_make_spi_transfer(0xC800, tx_buf+22, NULL);
	    tr12.cs_change = 1;
	    spi_message_add_tail(&tr12, &msg);

	    // set low battery threshold to 2.9V
	    tr13 = rfm12_make_spi_transfer(0xC049, tx_buf+24, NULL);
	    spi_message_add_tail(&tr13, &msg);

	    err = spi_sync(rfm12->spi, &msg);
	    if (err)
	  		 goto pError;
	    spi_message_init(&msg);

	    tr = rfm12_make_spi_transfer(RF_READ_STATUS, tx_buf+0, NULL);
	    spi_message_add_tail(&tr, &msg);

	    err = spi_sync(rfm12->spi, &msg);

	    if (err)
	    	goto pError;

//	    printk(KERN_INFO RFM12B_DRV_NAME " : set up successfully\n");

	    return err;
	} // end rfm12->homeeasy_active

	// printk(KERN_INFO RFM12B_DRV_NAME " : leaving OOK mode\n");




pError:
	// also done when homeeasy_active initially not set (normally leaving ook mode)
	// printk(KERN_INFO RFM12B_DRV_NAME " : leaving OOK mode with errStat: %d",err);
	rfm12->homeeasy_active=0;  // in case we got here because of an error
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

#define	TX_OOK_BITS 32
#define TX_OOK_NO    ((TX_OOK_BITS +1) * 4)
#define TX_SIMPLE_OOK_BITS 64
#define TX_SIMPLE_NO ((TX_SIMPLE_OOK_BITS) * 2 + 4)

static int
rfm12_he_write(struct rfm12_data* rfm12, u64 command,int simple_flag)
{
	int messages = 0;
	int bit = (simple_flag ? TX_SIMPLE_OOK_BITS : TX_OOK_BITS) - 1;
	int err = 0;
	struct spi_transfer* ret_tr;
	struct spi_transfer* tr;
	struct spi_message msg;
	u8 tx_buf[4];
	if(!rfm12->homeeasy_active)
		return -EACCES;

	tr = kzalloc((simple_flag ? TX_SIMPLE_NO : TX_OOK_NO) * sizeof(struct spi_transfer),GFP_KERNEL);

	// put into IDLE mode before starting transfer
	spi_message_init(&msg);
	*tr=rfm12_make_spi_transfer(RF_IDLE_MODE,tx_buf,NULL);
	tr->cs_change = 1;
	spi_message_add_tail(tr,&msg);
	err = spi_sync(rfm12->spi,&msg);
	if(err)
		return err;
	if(!simple_flag)
		printk(KERN_INFO RFM12B_DRV_NAME " : writing %08x in OOK mode\n",(unsigned int) command);
	else
		printk(KERN_INFO RFM12B_DRV_NAME " : writing %08x, %08x in OOK mode\n",
				(unsigned int) (command>>32) ,(unsigned int) command);

	// make the full message with all commands
	// preliminary, hacked code:   txbuf is never changed, as RF_XMITTER_ON and RF_IDLE_MODE are always sent
	// would not work if buf changed for each ook_transfer
	tx_buf[0]=(RF_XMITTER_ON & 0xff00) >> 8;
	tx_buf[1]=RF_XMITTER_ON & 0xff;
	tx_buf[2]=(RF_IDLE_MODE & 0xff00) >> 8;
	tx_buf[3]=RF_IDLE_MODE & 0xff;
	spi_message_init(&msg);
	if(!simple_flag)
		ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME,PREAMBLE_STROBE, &msg, tr, tx_buf);
	else {
		ret_tr=rfm12_ook_transfer(rfm12,LPREAMBLE_STROBE,LPREAMBLE_LOW, &msg, tr, tx_buf);
	}
//	printk(KERN_INFO RFM12B_DRV_NAME " : preamble pointer pos: %d\n", (int) (ret_tr-tr));
	do
	{
		if(command & (1ULL<<bit))
		{
			if(simple_flag) {
				ret_tr=rfm12_ook_transfer(rfm12,LHIGH_ON,LHIGH_OFF, &msg,  &tr[2*(TX_SIMPLE_OOK_BITS-bit)], tx_buf);
			} else {
				ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON, &msg, &tr[4*(TX_OOK_BITS-bit)-2], tx_buf);
				ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF, &msg, &tr[4*(TX_OOK_BITS-bit)], tx_buf);
			}
		}
		else
		{
			if(simple_flag) {
				ret_tr=rfm12_ook_transfer(rfm12,LLOW_ON,LLOW_OFF, &msg,  &tr[2*(TX_SIMPLE_OOK_BITS-bit)], tx_buf);
			} else {
				ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_OFF, &msg, &tr[4*(TX_OOK_BITS-bit)-2], tx_buf);
				ret_tr=rfm12_ook_transfer(rfm12,XMIT_ON_TIME, BIT_ON, &msg, &tr[4*(TX_OOK_BITS-bit)], tx_buf);
			}
		}
	} while(bit--);
	// add one on off cycle at end
	if(simple_flag)
		ret_tr=rfm12_ook_transfer(rfm12, LHIGH_ON, LEND_OFF, &msg, &tr[TX_SIMPLE_NO-2],tx_buf);
	else
		ret_tr=rfm12_ook_transfer(rfm12, XMIT_ON_TIME, BIT_OFF, &msg, &tr[4*(TX_OOK_BITS)+2], tx_buf);
//	rfm12_msg_debug(&msg);

	// now send it 5 times with MSG_BREAK delay in between
	for( ;messages < 5; messages++)
	{
		err = spi_sync(rfm12->spi, &msg);
		if(err)
			return err;
		if(!simple_flag)
			rfm12_safe_udelay(MSG_BREAK-BIT_OFF);
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
