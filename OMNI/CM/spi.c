#include "spi.h"
#include "spi_slave.h"
#include "data_fifo.h"
#include "spi_fifo.h"
#include "boards.h"
#include "app_error.h"
#include "radio_config.h"
#include <stdint.h>
#include <stdbool.h>

uint8_t spi_out[SPI_PACKET_BYTES];
uint8_t empty_write_buf[SPI_FIFO_BYTES];
uint8_t full_read_buf[PACKET_SIZE];

uint8_t *tx_buf;
uint8_t *rx_buf;

void spi_slave_event_handle(spi_slave_evt_t event)
{
	uint32_t err_code;
	uint8_t i;

	if (event.evt_type == SPI_SLAVE_XFER_DONE)
	{
		uint8_t *writeptr;
		uint8_t *readptr;
		uint8_t *packetptr;

		// SPI has completed, so we need to check if we actually wrote any valid data to the rx buffer
		// Only need to do this if the fifo was actually involved

		if (rx_buf != full_read_buf)
		{
			if ((rx_buf[1] == SPI_DATA) || (rx_buf[1] == SPI_REGISTER))
			{
				if (rx_buf[1] == SPI_DATA)
				{
					rx_buf[1] = SPI_DATA_LENGTH;
				}
				else
				{
					rx_buf[1] = SPI_REGISTER_LENGTH;
				}
				// smartfusion was transmitting data, so we do need to keep it in the fifo
				finish_write_data();
			}
			else
			{
				// smartfusion had nothing to transmit, just polling, reset the fifo
				reset_write_data();
			}
		}

		// also need to check if tx buffer was valid command
		if (tx_buf != empty_write_buf)
		{
			finish_read_spi_fifo();
		}

		// now prepare buffers for next transaction
		// set up data fifo

		readptr = write_data();
		if (readptr == 0)
		{
			// there is no where to write data to
			readptr = full_read_buf;
		}

		writeptr = read_spi_fifo();
		if (writeptr == 0)
		{
			writeptr = empty_write_buf;
		}

		for (i=0;i<SPI_FIFO_BYTES;i++)
		{
			spi_out[SPI_DATA_LENGTH+1+i] = writeptr[i];
			spi_out[CL_SPI_DATA_LENGTH+1+i] = writeptr[i];
		}

		// now set the buffers
		readptr[0] = PHASE_1;
		err_code = spi_slave_buffers_set(spi_out, readptr + 1, SPI_PACKET_BYTES, PACKET_SIZE-1);
		tx_buf = writeptr;
		rx_buf = readptr;
		APP_ERROR_CHECK(err_code);

		if ((get_radio_disabled() == true) && ((NRF_RADIO->STATE & RADIO_STATE_STATE_Msk) == RADIO_STATE_STATE_Disabled) && (get_num_data() > RADIO_THRESHOLD))
		{
			// for sure disabled, so can start here
			packetptr = read_data();
			if (packetptr != 0)
			{
				set_radio_disabled(false);
				NRF_RADIO->PACKETPTR = (uint32_t)packetptr;
				NRF_RADIO->TASKS_TXEN = 1;
			}
		}
	}
}

uint32_t spi_init(void)
{
	uint8_t *dataptr;
	uint32_t err_code;
	uint8_t i;

	spi_slave_config_t spi_slave_config;

    err_code = spi_slave_evt_handler_register(spi_slave_event_handle);
    APP_ERROR_CHECK(err_code);

    // Mario pins
    spi_slave_config.pin_miso         = 25;
    spi_slave_config.pin_mosi         = 24;
    spi_slave_config.pin_sck          = 28;
    spi_slave_config.pin_csn          = 29;

    // // Eval pins
    // spi_slave_config.pin_miso         = 4;
    // spi_slave_config.pin_mosi         = 6;
    // spi_slave_config.pin_sck          = 0;
    // spi_slave_config.pin_csn          = 2;

    spi_slave_config.mode             = SPI_MODE_0;
    spi_slave_config.bit_order        = SPIM_MSB_FIRST;
    spi_slave_config.def_tx_character = DEF_CHARACTER;
    spi_slave_config.orc_tx_character = ORC_CHARACTER;

	err_code = spi_slave_init(&spi_slave_config);
    APP_ERROR_CHECK(err_code);

    // initialize empty buffer
	for (i=0;i<SPI_FIFO_BYTES;i++)
	{
		empty_write_buf[i] = 0;
	}

	for (i=0;i<SPI_PACKET_BYTES;i++)
	{
		spi_out[i] = 0;
	}


	// set up first set of buffers
	dataptr = write_data();
	if (dataptr != 0)
	{
		err_code = spi_slave_buffers_set(spi_out, dataptr + 1, SPI_PACKET_BYTES, PACKET_SIZE-1);
		tx_buf = empty_write_buf;
		rx_buf = dataptr;
		APP_ERROR_CHECK(err_code);
	}
	else
	{
		err_code = spi_slave_buffers_set(spi_out, full_read_buf + 1, SPI_PACKET_BYTES, PACKET_SIZE-1);
		tx_buf = empty_write_buf;
		rx_buf = full_read_buf;
		APP_ERROR_CHECK(err_code);
	}

	return NRF_SUCCESS;
}


