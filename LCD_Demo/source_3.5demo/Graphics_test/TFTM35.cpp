/*************************************************** 
  www.buydisplay.com
 ****************************************************/

#include "TFTM35.h"
#include <avr/pgmspace.h>
#include <limits.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include <SPI.h>

// Constructor when using software SPI.  All output pins are configurable.
ST7796::ST7796(int8_t cs, int8_t dc, int8_t mosi,
				   int8_t sclk, int8_t rst) : ERGFX(ST7796_TFTWIDTH, ST7796_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _mosi  = mosi;
  _sclk = sclk;
  _rst  = rst;
  hwSPI = false;
}


// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
ST7796::ST7796(int8_t cs, int8_t dc, int8_t rst) : ERGFX(ST7796_TFTWIDTH, ST7796_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  hwSPI = true;
  _mosi  = _sclk = 0;
}

void ST7796::spiwrite(uint8_t c) {

  //Serial.print("0x"); Serial.print(c, HEX); Serial.print(", ");

  if (hwSPI) {
#if defined (__AVR__)
      uint8_t backupSPCR = SPCR;
    SPCR = mySPCR;
    SPDR = c;
    while(!(SPSR & _BV(SPIF)));
    SPCR = backupSPCR;
#elif defined(TEENSYDUINO)
    SPI.transfer(c);
#elif defined (__arm__)
    SPI.setClockDivider(11); // 8-ish MHz (full! speed!)
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.transfer(c);
#endif
  } else {
    // Fast SPI bitbang swiped from LPD8806 library
    for(uint8_t bit = 0x80; bit; bit >>= 1) {
      if(c & bit) {
	//digitalWrite(_mosi, HIGH); 
	*mosiport |=  mosipinmask;
      } else {
	//digitalWrite(_mosi, LOW); 
	*mosiport &= ~mosipinmask;
      }
      //digitalWrite(_sclk, LOW);
      *clkport &= ~clkpinmask;
       //digitalWrite(_sclk, HIGH);
      *clkport |=  clkpinmask;
    }
  }
}


void ST7796::writecommand(uint8_t c) {
  *dcport &=  ~dcpinmask;
  //digitalWrite(_dc, LOW);
  //*clkport &= ~clkpinmask; // clkport is a NULL pointer when hwSPI==true
  //digitalWrite(_sclk, LOW);
  *csport &= ~cspinmask;
  //digitalWrite(_cs, LOW);

  spiwrite(c);

  *csport |= cspinmask;
  //digitalWrite(_cs, HIGH);
}


void ST7796::writedata(uint8_t c) {
  *dcport |=  dcpinmask;
  //digitalWrite(_dc, HIGH);
  //*clkport &= ~clkpinmask; // clkport is a NULL pointer when hwSPI==true
  //digitalWrite(_sclk, LOW);
  *csport &= ~cspinmask;
  //digitalWrite(_cs, LOW);
  
  spiwrite(c);

  //digitalWrite(_cs, HIGH);
  *csport |= cspinmask;
} 

// If the SPI library has transaction support, these functions
// establish settings and protect from interference from other
// libraries.  Otherwise, they simply do nothing.
#ifdef SPI_HAS_TRANSACTION
static inline void spi_begin(void) __attribute__((always_inline));
static inline void spi_begin(void) {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}
static inline void spi_end(void) __attribute__((always_inline));
static inline void spi_end(void) {
  SPI.endTransaction();
}
#else
#define spi_begin()
#define spi_end()
#endif


void ST7796::begin(void) {
  if (_rst > 0) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, LOW);
  }

  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  
  csport    = portOutputRegister(digitalPinToPort(_cs));
  cspinmask = digitalPinToBitMask(_cs);
  dcport    = portOutputRegister(digitalPinToPort(_dc));
  dcpinmask = digitalPinToBitMask(_dc);

  if(hwSPI) { // Using hardware SPI
#if defined (__AVR__)
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz (full! speed!)
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    mySPCR = SPCR;
#elif defined(TEENSYDUINO)
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz (full! speed!)
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
#elif defined (__arm__)
      SPI.begin();
      SPI.setClockDivider(11); // 8-ish MHz (full! speed!)
      SPI.setBitOrder(MSBFIRST);
      SPI.setDataMode(SPI_MODE0);
#endif
  } else {
    pinMode(_sclk, OUTPUT);
    pinMode(_mosi, OUTPUT);

    clkport     = portOutputRegister(digitalPinToPort(_sclk));
    clkpinmask  = digitalPinToBitMask(_sclk);
    mosiport    = portOutputRegister(digitalPinToPort(_mosi));
    mosipinmask = digitalPinToBitMask(_mosi);
    *clkport   &= ~clkpinmask;
    *mosiport  &= ~mosipinmask;
  }

  // toggle RST low to reset
  if (_rst > 0) {
    digitalWrite(_rst, HIGH);
    delay(5);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(150);
  }

  if (hwSPI) spi_begin();
  writecommand(1);
  delay(150);

  writecommand(0xF0);     
  writedata(0xC3);   

  writecommand(0xF0);     
  writedata(0x96);   

  writecommand(0x36);     
  writedata(0x48);   

  writecommand(0x3A);      
  writedata(0x5); 

  writecommand(0xB4);     //1-dot Inversion
  writedata(0x01);   

  writecommand(0xB7);     
  writedata(0xC6);

  writecommand(0xC0);     
  writedata(0x80);   
  writedata(0x64); //VGH=15V VGL=-10V  

  writecommand(0xC1);     
  writedata(0x13);  //VOP=4.5V

  writecommand(0xC2);     
  writedata(0xA7);   

  writecommand(0xC5);     
  writedata(0x08);   

  writecommand(0xE8);     
  writedata(0x40);   
  writedata(0x8a);   
  writedata(0x00);   
  writedata(0x00);   
  writedata(0x29);   
  writedata(0x19);   
  writedata(0xA5);   
  writedata(0x33);   
  writecommand(0xE0);
  writedata(0xF0);
  writedata(0x06);
  writedata(0x0B);
  writedata(0x07);
  writedata(0x06);
  writedata(0x05);
  writedata(0x2E);
  writedata(0x33);
  writedata(0x47);
  writedata(0x3A);
  writedata(0x17);
  writedata(0x16);
  writedata(0x2E);
  writedata(0x31);

  writecommand(0xE1);
  writedata(0xF0);
  writedata(0x09);
  writedata(0x0D);
  writedata(0x09);
  writedata(0x08);
  writedata(0x23);
  writedata(0x2E);
  writedata(0x33);
  writedata(0x46);
  writedata(0x38);
  writedata(0x13);
  writedata(0x13);
  writedata(0x2C);
  writedata(0x32);

  writecommand(0xF0);     
  writedata(0x3C);   

  writecommand(0xF0);     
  writedata(0x69);   

  writecommand(0x35);     
  writedata(0x00); 

  writecommand(0x21); 
  delay(120);  
  writecommand(0x11);

  if (hwSPI) spi_end();
  delay(120); 		
  if (hwSPI) spi_begin();
  writecommand(0x29);     
  delay(50); 

  writecommand(0x2A);    //320 
  writedata(0x00);   
  writedata(0x00);   
  writedata(1);   
  writedata(0x3f);   

  writecommand(0x2B);    //480
  writedata(0x00);   
  writedata(0x00);   
  writedata(1);   
  writedata(0xdf); 

  writecommand(0x2C); 
}

void ST7796::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1,uint16_t y1) {

  writecommand(ST7796_CASET); // Column addr set
  writedata(x0 >> 8);
  writedata(x0);
  writedata(x1 >> 8);
  writedata(x1); 

  writecommand(ST7796_PASET); // Row addr set
  writedata(y0>>8);
  writedata(y0);
  writedata(y1>>8);
  writedata(y1);

  writecommand(ST7796_RAMWR); // write to RAM
}

void ST7796::pushColor(uint16_t color) {
  if (hwSPI) spi_begin();
  //digitalWrite(_dc, HIGH);
  *dcport |=  dcpinmask;
  //digitalWrite(_cs, LOW);
  *csport &= ~cspinmask;

  spiwrite(color >> 8);
  spiwrite(color);

  *csport |= cspinmask;
  //digitalWrite(_cs, HIGH);
  if (hwSPI) spi_end();
}

void ST7796::drawPixel(int16_t x, int16_t y, uint16_t color) {

  if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;

  if (hwSPI) spi_begin();
  setAddrWindow(x,y,x+1,y+1);

  //digitalWrite(_dc, HIGH);
  *dcport |=  dcpinmask;
  //digitalWrite(_cs, LOW);
  *csport &= ~cspinmask;

  spiwrite(color >> 8);
  spiwrite(color);

  *csport |= cspinmask;
  //digitalWrite(_cs, HIGH);
  if (hwSPI) spi_end();
}


void ST7796::drawFastVLine(int16_t x, int16_t y, int16_t h,
 uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;

  if((y+h-1) >= _height) 
    h = _height-y;

  if (hwSPI) spi_begin();
  setAddrWindow(x, y, x, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  *dcport |=  dcpinmask;
  //digitalWrite(_dc, HIGH);
  *csport &= ~cspinmask;
  //digitalWrite(_cs, LOW);

  while (h--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  *csport |= cspinmask;
  //digitalWrite(_cs, HIGH);
  if (hwSPI) spi_end();
}


void ST7796::drawFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {

  // Rudimentary clipping
  if((x >= _width) || (y >= _height)) return;
  if((x+w-1) >= _width)  w = _width-x;
  if (hwSPI) spi_begin();
  setAddrWindow(x, y, x+w-1, y);

  uint8_t hi = color >> 8, lo = color;
  *dcport |=  dcpinmask;
  *csport &= ~cspinmask;
  //digitalWrite(_dc, HIGH);
  //digitalWrite(_cs, LOW);
  while (w--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  *csport |= cspinmask;
  //digitalWrite(_cs, HIGH);
  if (hwSPI) spi_end();
}

void ST7796::fillScreen(uint16_t color) {
  fillRect(0, 0,  _width, _height, color);
}

// fill a rectangle
void ST7796::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
  uint16_t color) {

  // rudimentary clipping (drawChar w/big text requires this)
  if((x >= _width) || (y >= _height)) return;
  if((x + w - 1) >= _width)  w = _width  - x;
  if((y + h - 1) >= _height) h = _height - y;

  if (hwSPI) spi_begin();
  setAddrWindow(x, y, x+w-1, y+h-1);

  uint8_t hi = color >> 8, lo = color;

  *dcport |=  dcpinmask;
  //digitalWrite(_dc, HIGH);
  *csport &= ~cspinmask;
  //digitalWrite(_cs, LOW);

  for(y=h; y>0; y--) {
    for(x=w; x>0; x--) {
      spiwrite(hi);
      spiwrite(lo);
    }
  }
  //digitalWrite(_cs, HIGH);
  *csport |= cspinmask;
  if (hwSPI) spi_end();
}


// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t ST7796::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

 

