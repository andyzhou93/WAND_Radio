#include "radio_config.h"
#include "nrf_delay.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

uint8_t packet[MAX_PACKET_SIZE];
uint8_t packet2[MAX_PACKET_SIZE];
uint8_t size;

void clear_packet(void)
{
	uint8_t i;
	for (i=0;i<MAX_PACKET_SIZE;i++)
	{
		packet[i] = 0;
	}
}

void init(void)
{
	/* Start 16 MHz crystal oscillator */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) 
    {
    }

    clear_packet();
    radio_configure(DEFAULT_PACKET_SIZE);
}

int main(void)
{
	bool crc;
	uint32_t drops;
	bool timeout;
	int x;
	bool success;
  uint32_t good;

	init();

	// always receive to the packet.
	NRF_RADIO->PACKETPTR = (uint32_t)packet;

	while (true)
	{
		// STAGE 1: Listen for frequency

    // set up the shorts for continuous listen until CRC, disabling every time
		NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
        					(RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos); 
		crc = false;
		while (crc == false)
		{
			NRF_RADIO->EVENTS_DISABLED = 0;
			NRF_RADIO->TASKS_RXEN = 1;
			while (NRF_RADIO->EVENTS_DISABLED == 0)
			{
			}
			// got a packet, check for CRC
			if (NRF_RADIO->CRCSTATUS == 1)
			{
				crc = true;
			}
		}
		// we have successfully received a good packet
		// save this value
		size = packet[0];

		//  acknowledge that the frequency has been received by sending a packet
    packet[0] = 0xFF;
    packet[1] = 0xAA;
		NRF_RADIO->EVENTS_DISABLED = 0;
		NRF_RADIO->TASKS_TXEN = 1;
		while (NRF_RADIO->EVENTS_DISABLED == 0)
		{
		}
		// now we can update the radio frequency
		radio_configure(size);

		// STAGE 2: Listen for packets and collect RSSI and drop info
		NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
        					(RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos);
       	drops = 0;
        good = 0;
       	timeout = false;

       	// loop until Side A stops transmitting
       	NRF_RADIO->EVENTS_END = 0;
       	NRF_RADIO->TASKS_RXEN = 1;
       	while (timeout == false)
       	{
       		x = 1000;
       		while (NRF_RADIO->EVENTS_END == 0)
       		{
       			if (x-- >= 0)
       			{
       				nrf_delay_us(100);
       			}
       			else
       			{
       				break;
       			}
       		}

       		if (x >= 0)
       		{
       			// did not timeout, check CRC
       			if (NRF_RADIO->CRCSTATUS != 1)
       			{
       				drops++;
       			}
            else
            {
              good++;
            }
       			// start up receiving again
       			NRF_RADIO->EVENTS_END = 0;
       			NRF_RADIO->TASKS_START = 1;
       		}
       		else 
       		{
       			// timed out, so we are done
       			timeout = true;
       			// disable the radio
       			NRF_RADIO->EVENTS_DISABLED = 0;
       			NRF_RADIO->TASKS_DISABLE = 1;
       			while (NRF_RADIO->EVENTS_DISABLED == 0)
       			{
       			}
       		}
       	}

       	// STAGE 3: Report back to Side A with RSSI and drops info
       	clear_packet();

       	// build up the packet as a string to be displayed on the other side.

        snprintf((char *)packet, 4, "%d", (int)size);
        snprintf((char *)packet + 3, 3, ", ");
        snprintf((char *)packet + 5, 5, "%d", (int)drops);
        snprintf((char *)packet + 9, 3, ", ");
        snprintf((char *)packet + 11, 5, "%d", (int)good);
        snprintf((char *)packet + 15, 3, "\n\r");

       	// now transmit the packet until we get a response

       	NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
        					(RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);
       	success = false;
        while (success == false)
        {
        	// transmit
        	NRF_RADIO->PACKETPTR = (uint32_t)packet;
        	NRF_RADIO->EVENTS_DISABLED = 0;
        	NRF_RADIO->TASKS_TXEN = 1;
        	while (NRF_RADIO->EVENTS_DISABLED == 0)
        	{
        	}
        	// now wait for an ack packet
        	x = 1000;
        	NRF_RADIO->EVENTS_DISABLED = 0;
        	NRF_RADIO->TASKS_RXEN = 1;
       		while (NRF_RADIO->EVENTS_DISABLED == 0)
       		{
       			if (x-- >= 0)
       			{
       				nrf_delay_us(100);
       			}
       			else
       			{
       				break;
       			}
       		}

       		if (x >= 0)
       		{
       			success = true;
       		}
       		else
       		{
       			NRF_RADIO->EVENTS_DISABLED = 0;
       			NRF_RADIO->TASKS_DISABLE = 1;
       			while (NRF_RADIO->EVENTS_DISABLED == 0)
       			{
       			}
       		}
       	}

       	// Done!
    }
}





       	








