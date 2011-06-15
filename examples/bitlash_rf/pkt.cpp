//////////////////////////////////////////////////////////////////
//
//	pkt.c:	Packet Driver Interface
//
//	Copyright (C) 2009-2011 by Bill Roy
//
//	This library is free software; you can redistribute it and/or
//	modify it under the terms of the GNU Lesser General Public
//	License as published by the Free Software Foundation; either
//	version 2.1 of the License, or (at your option) any later version.
//
//	This library is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public
//	License along with this library; if not, write to the Free Software
//	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//////////////////////////////////////////////////////////////////
//
#include "WProgram.h"
#include "bitlash.h"
#include "../../libraries/bitlash/src/bitlash.h"
#include "bitlash_rf.h"


///////////////
// Packet buffer management
///////////////
byte tx_index;		// index to next free space in tx_buf.data[]
byte pkt_sequence;	// packet sequence byte
pkt_t tx_buf;		// the tx packet buffer
pkt_t rx_buf;		// rx buffer used in the receive loop

// packet counters
unsigned tx_packet_count;
unsigned rx_packet_count;
unsigned rx_bogon_count;


//////////
// pkt_flush: flush any pending data in the packet buffer to tx
//
void pkt_flush(void) {
	if (tx_index) {
		tx_buf.sequence = pkt_sequence++;
		tx_send_pkt(&tx_buf, tx_index + RF_PACKET_HEADER_SIZE);
		tx_index = 0;		// reset for next byte
	}
}

//////////
// pkt_put: add a byte to the buffer and transmit on buffer full
//
void pkt_put(byte b) {
	tx_buf.data[tx_index++] = b;
	if (tx_index >= RF_PACKET_DATA_SIZE) {	// full?
		pkt_flush();						// transmit
 	}
}

///////////////////////////////
// send serial output
//
void send_serial(byte b) {
	tx_buf.type = PKT_SERIAL;
	pkt_put(b);
}

//////////////////////
//
//	send_command_byte
//
//	Transmit a command according to previously established network parameters.
//
void send_command_byte(byte b) { 
	tx_buf.type = PKT_COMMAND;
	pkt_put(b);
}



//////////
// dump packet stats for debugging
//
numvar func_pktstat(void) {
	sp("tx pkts:"); printInteger(tx_packet_count, 0);
	sp(" rx pkts:"); printInteger(rx_packet_count, 0);
	sp(" bogons:"); printInteger(rx_bogon_count, 0);
	speol();
}


//////////
// rf_log_packet: Hex dump the packet to the console
//
byte rf_logbytes;

// a function handler to expose the control
numvar func_rflog(void) { rf_logbytes = getarg(1); }

void lpb(byte b) {
	if (b == '\\') {
		spb('\\');
		spb('\\');
	}
	else if ((b >= ' ') && (b <= 0x7f)) spb(b);
	else {
		spb('\\');
		if (b == 0xd) spb('r');
		else if (b == 0xa) spb('n');
		else {
			spb('x');
			if (b < 0x10) spb('0');
			spb(b);
		}
	}
}

// todo: rewrite as printf()

void log_packet(char tag, pkt_t *pkt, byte length) {
	if (rf_logbytes) {
		int i = 0;
		int last = (length < rf_logbytes) ? length : rf_logbytes;
		spb('[');
		spb(tag); 
		sp("X ");
		printInteger(length, 0); spb(' ');
		printInteger(pkt->type, 0); spb(' ');
		printInteger(pkt->sequence, 0); spb(' ');
		while (i < last-RF_PACKET_HEADER_SIZE) {
			lpb(pkt->data[i++]);
		}
		spb(']');
		speol();
	}
}
