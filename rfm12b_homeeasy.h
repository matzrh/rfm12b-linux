/*
 * rfm12b_homeeasy.h
 *
 *  Created on: 27.07.2014
 *      Author: hoellinm
 */

#ifndef RFM12B_HOMEEASY_H_
#define RFM12B_HOMEEASY_H_



static int
rfm12_reset(struct rfm12_data* rfm12);
struct spi_transfer
rfm12_make_spi_transfer(uint16_t cmd, u8* tx_buf, u8* rx_buf);


static int
rfm12_he_setup(struct rfm12_data* rfm12)
{
	int err=-1;  //
	if(rfm12->homeeasy_active)
	{
	   struct spi_transfer tr, tr2, tr3;
	   struct spi_message msg;
	   struct spi_rfm12_active_board* brd;
	   u8 tx_buf[26];

	   rfm12->state = RFM12_STATE_CONFIG;  // state will be preserved during whole OOK phase

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
	   if(!brd)
		   goto pError;

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

	   if (0 == err) {
	     spi_message_init(&msg);

	     tr = rfm12_make_spi_transfer(RF_READ_STATUS, tx_buf+0, NULL);
	     spi_message_add_tail(&tr, &msg);

	     err = spi_sync(rfm12->spi, &msg);
	   }

	   return err;
	}
	else
	{
		struct spi_rfm12_active_board* brd;
		brd = platform_irq_identifier_for_spi_device(rfm12->spi->master->bus_num,rfm12->spi->chip_select);
		if(brd->state.irq_enabled)  // enable interrupt if previously on
			brd->irqchip->irq_unmask(brd->irqchip_data);


	}

pError:
	rfm12->state = RFM12_STATE_IDLE;
	rfm12_reset(rfm12);  // reset if turned off or error
	return err;


}

static int
rfm12_he_write(struct rfm12_data* rfm12, u32 command)
{
	printk(KERN_INFO RFM12B_DRV_NAME " : would now write %04x in OOK mode",command);
	return 0;
}



#endif /* RFM12B_HOMEEASY_H_ */
