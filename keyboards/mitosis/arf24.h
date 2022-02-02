
#pragma once
#include <gpio.h>
#include <spi_master.h>
void nrf_init(pin_t ce_pin, pin_t csn_pin);
void startListening(void);
void stopListening(void);
uint8_t qmkrf_available(void);
void qmkrf_read(uint8_t* buf, uint8_t len);
void openReadingPipe(uint8_t number, const uint8_t* address);
void setChannel(uint8_t channel);
void enableDynamicPayloads(void); // Might not need it
void setAutoAck(bool enable);

void csn(bool val);
void ce(bool val);
bool beginTransaction(void);
void endTransaction(void);
void qmkrf_setChannel(uint8_t channel);
spi_status_t qmkrf_get_status(void);
spi_status_t qmkrf_read_register(uint8_t reg);
spi_status_t write_register(uint8_t reg, uint8_t val);
// spi_status_t read_register(uint8_t reg, uint8_t* buf, uint8_t len);
spi_status_t write_memory(uint8_t reg, const uint8_t* buf, uint8_t len);
spi_status_t read_payload(uint8_t* buf, uint8_t len);

