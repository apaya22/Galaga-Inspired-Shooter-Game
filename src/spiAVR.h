#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "helper.h"


//B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
//SPI peripheral that sends data back to the arduino, you will need to use B4 as the MISO pin.
//The SS pin can be any digital pin on the arduino. Right before sending an 8 bit value with
//the SPI_SEND() funtion, you will need to set your SS pin to low. If you have multiple SPI
//devices, they will share the SCK, MOSI and MISO pins but should have different SS pins.
//To send a value to a specific device, set it's SS pin to low and all other SS pins to high.

// Outputs, pin definitions
#define PIN_SCK                   PORTB5//SHOULD ALWAYS BE B5 ON THE ARDUINO
#define PIN_MOSI                  PORTB3//SHOULD ALWAYS BE B3 ON THE ARDUINO
#define PIN_SS                    PORTB2
#define PIN_DC                    PORTB0 //A0
#define PIN_RESET                 PORTB1 //reset    


//If SS is on a different port, make sure to change the init to take that into account.
void SPI_INIT(){
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);//initialize your pins. 
    SPCR |= (1 << SPE) | (1 << MSTR); //initialize SPI coomunication
}


void SPI_SEND(char data)
{
    SPDR = data;//set data that you want to transmit
    while (!(SPSR & (1 << SPIF)));// wait until done transmitting
}

//LCD screen 
void HardwareReset(){
//setResetPinToLow
PORTB = SetBit(PORTB, 1, 0);
_delay_ms(200);
//setResetPinToHigh;
PORTB = SetBit(PORTB, 1, 1);
_delay_ms(200);
}

void sendCommand(char cmd) {
    //AO low
    PORTB &= ~(1 << PIN_DC);  
    //CS low 
    PORTB &= ~(1 << PIN_SS); 
    SPI_SEND(cmd);
    // CS high
    PORTB |= (1 << PIN_SS);   
}

void sendData(char data) {
    //AO high
    PORTB |= (1 << PIN_DC);  
    //CS low 
    PORTB &= ~(1 << PIN_SS); 
    SPI_SEND(data);
    //CS high
    PORTB |= (1 << PIN_SS);  
}


void ST7735_init() {
    HardwareReset();

    sendCommand(0x01); //SWRESET
    _delay_ms(150);

    sendCommand(0x11); //SLPOUT
    _delay_ms(200);

    sendCommand(0x3A); //COLMOD
    sendData(0x05);   //16 bit color mode
    _delay_ms(10);

    sendCommand(0x29); //DISPON
    _delay_ms(200);
}




#endif /* SPIAVR_H */