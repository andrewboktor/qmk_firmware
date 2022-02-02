#include <spi_master.h>
#include <gpio.h>
#include <wait.h>
#include "nRF24L01.h"
#include "arf24.h"

/*
I have to make assumptions and take shortcuts otherwise I will never be done
Here are those shortcuts so that they don't bite me (or you) later
- payload_size is fixed at 32
- address width is fixed at 5
*/

// We will use address width of 5
uint8_t address_width = 5;

uint8_t ce_pin;
uint8_t csn_pin;
uint8_t payload_size = 32;

void setRetries(uint8_t delay, uint8_t count) {
    write_register(SETUP_RETR,(delay&0xf)<<ARD | (count&0xf)<<ARC);
}

void nrf_init(pin_t ce_p, pin_t csn_p) {
    spi_init();
    ce_pin = ce_p;
    csn_pin = csn_p;
    write_register( NRF_CONFIG, 0x0C ) ;
    write_register(DYNPD, 0x00);    // Disable dynamic payload
    write_register(EN_AA, 0x3F);    // Enable AutoAck
    payload_size = 32;
    qmkrf_setChannel(76);
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));
    write_register(FLUSH_RX, RF24_NOP); // Flush RX
    write_register(FLUSH_TX, RF24_NOP); // Flush TX
    write_register(NRF_CONFIG, (_BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP))); // Clear CONFIG Register and power up
    wait_us(5000);
}

void qmkrf_setChannel(uint8_t channel) {
    write_register(RF_CH, channel >125? 125: channel);
}

void csn(bool val) {
    writePin(csn_pin, val);
    wait_us(10);
}

void ce(bool val) {
    writePin(ce_pin, val);
    wait_us(10);
}

bool beginTransaction() {
    return spi_start(csn_pin, false /* lsbFirst */, 0 /* mode */, 4 /* divisor */);
}

void endTransaction() {
    spi_stop();
}

spi_status_t qmkrf_read_register(uint8_t reg) {
    if(!beginTransaction()) return SPI_STATUS_ERROR;
    spi_status_t status = spi_write(R_REGISTER | (REGISTER_MASK & reg));
    if (status < 0) return status;
    status = spi_read();
    endTransaction();
    return status;
}

spi_status_t write_register(uint8_t reg, uint8_t val) {
    if(!beginTransaction()) return SPI_STATUS_ERROR;
    spi_status_t status = spi_write(W_REGISTER | (REGISTER_MASK & reg));
    if (status < 0) return status;
    status = spi_write(val);
    endTransaction();
    return status;
}

// spi_status_t qmkrf_read_register(uint8_t reg, uint8_t* buf, uint8_t len) {
//     if(!beginTransaction()) return SPI_STATUS_ERROR;
//     spi_status_t status = spi_write(R_REGISTER | (REGISTER_MASK & reg));
//     if (status < 0) return status;
//     status = spi_receive(buf, len);
//     endTransaction();
//     return status;
// }

spi_status_t write_memory(uint8_t reg, const uint8_t* buf, uint8_t len) {
    if(!beginTransaction()) return SPI_STATUS_ERROR;
    spi_status_t status = spi_write(W_REGISTER | (REGISTER_MASK & reg));
    if (status < 0) return status;
    status = spi_transmit(buf, len);
    endTransaction();
    return status;
}

spi_status_t read_payload(uint8_t* buf, uint8_t len) {
    if(!beginTransaction()) return SPI_STATUS_ERROR;
    spi_status_t status = spi_write(R_RX_PAYLOAD);
    if (status < 0) return status;
    status = spi_receive(buf, len);
    endTransaction();
    return status;
}

spi_status_t qmkrf_get_status() {
    if(!beginTransaction()) return SPI_STATUS_ERROR;
    spi_status_t status = spi_write(RF24_NOP);
    endTransaction();
    return status;
}

void startListening(void) {
    write_register(NRF_CONFIG, qmkrf_read_register(NRF_CONFIG) | _BV(PRIM_RX));
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));
    ce(1);
}

void stopListening() {
    ce(0);
    // Doesn't really matter we don't care about this one for now
}

void openReadingPipe(uint8_t number, const uint8_t* address) {
    // Set the address for the pipe we're opening
    write_memory(RX_ADDR_P0+number, address, number<2?address_width:1);
    // Set the payload size for the pipe we're opening
    write_register(RX_PW_P0+number, payload_size);
    // Enable RX for that pipe
    write_register(EN_RXADDR, qmkrf_read_register(EN_RXADDR)|_BV(ERX_P0+number));
}

void qmkrf_read(uint8_t* buf, uint8_t len) {
    read_payload(buf, len);
    write_register(NRF_STATUS, _BV(RX_DR) | _BV(MAX_RT) | _BV(TX_DS));
}

/* returns pipe number that has data available 0x07 means no data is available */
uint8_t qmkrf_available() {
    return (qmkrf_get_status() >> RX_P_NO & 0x07);
}
