/*
 twi.c - TWI/I2C library for Wiring & Arduino
 Copyright (c) 2006 Nicholas Zambetti.  All right reserved.
 Portions Copyright (C) 2013 - 2013 HORIGUCHI Junshi.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
 */

#include "I2Cm.h"
#include <compat/twi.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

static void twi_reply(uint8_t ack);
static void twi_stop(void);
static void twi_releaseBus(void);

static volatile uint8_t twi_state;
static volatile uint8_t twi_slarw;
static volatile uint8_t twi_sendStop;           // should the transaction end with a stop
static volatile uint8_t twi_inRepStart;         // in the middle of a repeated start
static uint8_t twi_masterBuffer[TWI_BUFFER_LENGTH];
static volatile int16_t twi_masterBufferIndex;
static volatile int16_t twi_masterBufferLength;
static volatile uint8_t twi_error;

I2CmClass I2Cm;

/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void I2CmClass::begin(void)
{
    // initialize state
    twi_state = TWI_READY;
    twi_sendStop = true;        // default value
    twi_inRepStart = false;
    
    // activate internal pullups for twi.
    digitalWrite(SDA, 1);
    digitalWrite(SCL, 1);
    
    // initialize twi prescaler and bit rate
    cbi(TWSR, TWPS0);
    cbi(TWSR, TWPS1);
    TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
    
    /* twi bit rate formula from atmega128 manual pg 204
     SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
     note: TWBR should be 10 or higher for master mode
     It is 72 for a 16mhz Wiring board with 100kHz TWI */
    
    // enable twi module, acks, and twi interrupt
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
    
    clear();
    return;
}

void I2CmClass::clear(void)
{
    // initialize buffer iteration vars
    twi_masterBufferIndex = 0;
    twi_masterBufferLength = 0;
    return;
}

int I2CmClass::write(uint8_t data)
{
    int result(0);
    
    if (twi_masterBufferLength < TWI_BUFFER_LENGTH) {
        twi_masterBuffer[twi_masterBufferLength] = data;
        ++twi_masterBufferLength;
        ++result;
    }
    return result;
}

/*
 * Function twi_writeTo
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the end
 * Output   0 .. success
 *          1 .. length to long for buffer
 *          2 .. address send, NACK received
 *          3 .. data send, NACK received
 *          4 .. other twi error (lost bus arbitration, bus error, ..)
 */
uint8_t I2CmClass::send(uint8_t address, uint8_t sendStop)
{
    // wait until twi is ready, become master transmitter
    while(TWI_READY != twi_state){
        continue;
    }
    twi_state = TWI_MTX;
    twi_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi_error = 0xFF;
    
    // build sla+w, slave device address + w bit
    twi_slarw = TW_WRITE;
    twi_slarw |= address << 1;
    
    // if we're in a repeated start, then we've already sent the START
    // in the ISR. Don't do it again.
    //
    if (true == twi_inRepStart) {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the
        // repeated start that we sent outselves, and that would really confuse things.
        twi_inRepStart = false;         // remember, we're dealing with an ASYNC ISR
        TWDR = twi_slarw;
        TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);  // enable INTs, but not START
    }
    else
        // send start condition
        TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) | _BV(TWSTA); // enable INTs
    
    // wait for write operation to complete
    while(TWI_MTX == twi_state){
        continue;
    }
    
    clear();
    
    if (twi_error == 0xFF)
        return 0;   // success
    else if (twi_error == TW_MT_SLA_NACK)
        return 2;   // error: address send, nack received
    else if (twi_error == TW_MT_DATA_NACK)
        return 3;   // error: data send, nack received
    else
        return 4;   // other twi error
}

/*
 * Function twi_readFrom
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
int I2CmClass::receive(uint8_t address, int length, uint8_t sendStop)
{
    clear();
    
    // ensure data will fit into buffer
    if(TWI_BUFFER_LENGTH < length){
        return 0;
    }
    
    // wait until twi is ready, become master receiver
    while(TWI_READY != twi_state){
        continue;
    }
    twi_state = TWI_MRX;
    twi_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi_error = 0xFF;
    
    // initialize buffer iteration vars
    twi_masterBufferIndex = 0;
    twi_masterBufferLength = length-1;  // This is not intuitive, read on...
    // On receive, the previously configured ACK/NACK setting is transmitted in
    // response to the received byte before the interrupt is signalled.
    // Therefor we must actually set NACK when the _next_ to last byte is
    // received, causing that NACK to be sent in response to receiving the last
    // expected byte of data.
    
    // build sla+w, slave device address + w bit
    twi_slarw = TW_READ;
    twi_slarw |= address << 1;
    
    if (true == twi_inRepStart) {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the
        // repeated start that we sent outselves, and that would really confuse things.
        twi_inRepStart = false;         // remember, we're dealing with an ASYNC ISR
        TWDR = twi_slarw;
        TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);  // enable INTs, but not START
    }
    else
        // send start condition
        TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);
    
    // wait for read operation to complete
    while(TWI_MRX == twi_state){
        continue;
    }
    
    if (twi_masterBufferIndex < length)
        length = twi_masterBufferIndex;
    
    twi_masterBufferIndex = 0;
    twi_masterBufferLength = length;
    return length;
}

int I2CmClass::available(void)
{
    return twi_masterBufferLength - twi_masterBufferIndex;
}

int I2CmClass::read(void)
{
    int result(-1);
    
    if (twi_masterBufferIndex < twi_masterBufferLength) {
        result = twi_masterBuffer[twi_masterBufferIndex];
        ++twi_masterBufferIndex;
    }
    return result;
}

/*
 * Function twi_reply
 * Desc     sends byte or readys receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
static void twi_reply(uint8_t ack)
{
    // transmit master read ready signal, with or without ack
    if(ack){
        TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
    }else{
        TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
    }
    return;
}

/*
 * Function twi_stop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
static void twi_stop(void)
{
    // send stop condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);
    
    // wait for stop condition to be exectued on bus
    // TWINT is not set after a stop condition!
    while(TWCR & _BV(TWSTO)){
        continue;
    }
    
    // update twi state
    twi_state = TWI_READY;
    return;
}

/*
 * Function twi_releaseBus
 * Desc     releases bus control
 * Input    none
 * Output   none
 */
static void twi_releaseBus(void)
{
    // release bus
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);
    
    // update twi state
    twi_state = TWI_READY;
    return;
}

ISR(TWI_vect)
{
    switch(TW_STATUS){
            // All Master
        case TW_START:     // sent start condition
        case TW_REP_START: // sent repeated start condition
            // copy device address and r/w bit to output register and ack
            TWDR = twi_slarw;
            twi_reply(1);
            break;
            
            // Master Transmitter
        case TW_MT_SLA_ACK:  // slave receiver acked address
        case TW_MT_DATA_ACK: // slave receiver acked data
            // if there is data to send, send it, otherwise stop
            if(twi_masterBufferIndex < twi_masterBufferLength){
                // copy data to output register and ack
                TWDR = twi_masterBuffer[twi_masterBufferIndex++];
                twi_reply(1);
            }else{
                if (twi_sendStop)
                    twi_stop();
                else {
                    twi_inRepStart = true;  // we're gonna send the START
                    // don't enable the interrupt. We'll generate the start, but we
                    // avoid handling the interrupt until we're in the next transaction,
                    // at the point where we would normally issue the start.
                    TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN) ;
                    twi_state = TWI_READY;
                }
            }
            break;
        case TW_MT_SLA_NACK:  // address sent, nack received
            twi_error = TW_MT_SLA_NACK;
            twi_stop();
            break;
        case TW_MT_DATA_NACK: // data sent, nack received
            twi_error = TW_MT_DATA_NACK;
            twi_stop();
            break;
        case TW_MT_ARB_LOST: // lost bus arbitration
            twi_error = TW_MT_ARB_LOST;
            twi_releaseBus();
            break;
            
            // Master Receiver
        case TW_MR_DATA_ACK: // data received, ack sent
            // put byte into buffer
            twi_masterBuffer[twi_masterBufferIndex++] = TWDR;
        case TW_MR_SLA_ACK:  // address sent, ack received
            // ack if more bytes are expected, otherwise nack
            if(twi_masterBufferIndex < twi_masterBufferLength){
                twi_reply(1);
            }else{
                twi_reply(0);
            }
            break;
        case TW_MR_DATA_NACK: // data received, nack sent
            // put final byte into buffer
            twi_masterBuffer[twi_masterBufferIndex++] = TWDR;
            if (twi_sendStop)
                twi_stop();
            else {
                twi_inRepStart = true;  // we're gonna send the START
                // don't enable the interrupt. We'll generate the start, but we
                // avoid handling the interrupt until we're in the next transaction,
                // at the point where we would normally issue the start.
                TWCR = _BV(TWINT) | _BV(TWSTA)| _BV(TWEN) ;
                twi_state = TWI_READY;
            }
            break;
        case TW_MR_SLA_NACK: // address sent, nack received
            twi_stop();
            break;
            // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case
            
            // All
        case TW_NO_INFO:   // no state information
            break;
        case TW_BUS_ERROR: // bus error, illegal stop/start
            twi_error = TW_BUS_ERROR;
            twi_stop();
            break;
    }
    return;
}
