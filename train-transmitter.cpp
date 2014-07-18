/**
 *  RF Radio Train Transmitter
 * bcm2835 Library required to run
 * # http://www.airspayce.com/mikem/bcm2835/index.html
 * 
 * sudo wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.36.tar.gz
 * #download the latest version of the library, say bcm2835-1.xx.tar.gz, then:
 * tar zxvf bcm2835-1.xx.tar.gz
 * cd bcm2835-1.xx
 * ./configure
 * make
 * sudo make check
 * sudo make install
 */

#include <cstdlib>
#include <iostream>
#include <bcm2835.h>
#include "../RF24.h"
#include <stdlib.h>  
#define RADIO_1_LED RPI_GPIO_P1_11 // pin 17
#define RADIO_1_COMMAND RPI_V2_GPIO_P1_26 // pin 8

void SyncRadios();

RF24 radio("/dev/spidev0.0",8000000 , 25);  //spi device, speed and CSN,only CSN is NEEDED in RPI

const int role_pin = 7;

// Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint64_t pipes[2] = { 0xF0F0F0F0E9LL, 0xF0F0F0F0D2LL };

typedef enum { role_ping_out = 1, role_pong_back } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role;

void setup(void)
{
   role = role_ping_out;
   printf("\n\rRF24/examples/pingpair/\n\r");
   printf("ROLE: %s\n\r",role_friendly_name[role]);
   // up the led for pair confirmation
   if (!bcm2835_init())
	printf("Warning Cannot access GPIO!!!!!!!");

   // Set the radio 1 led  pin to be an output
   bcm2835_gpio_fsel(RADIO_1_LED, BCM2835_GPIO_FSEL_OUTP);
   // Set RPI pin 3 to be an input
   bcm2835_gpio_fsel(RADIO_1_COMMAND, BCM2835_GPIO_FSEL_INPT);
   //  with a pullup
   bcm2835_gpio_set_pud(RADIO_1_COMMAND, BCM2835_GPIO_PUD_DOWN);
   // Setup and configure rf radio
   radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  radio.setChannel(0x4c);
  radio.setPALevel(RF24_PA_MAX);

  // Open pipes to other nodes for communication
  // pipe for reading in position #1 (we can have up to 5 pipes open for reading)
  // Role is alway [ping out as we are transmitting
  
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  
  // Start listening
 
  radio.startListening();
  // Dump the configuration of the rf unit for debugging
  radio.printDetails();
    
  SyncRadios();

}

void loop(void)
{
  // Ping out role.  Repeatedly send the current time
  
    // First, stop listening so we can talk.
    radio.stopListening();
   
   // Read some data
    uint8_t value = bcm2835_gpio_lev(RADIO_1_COMMAND);
    printf("read from pin 8: %d\n", value);
	
	// wait a bit
    delay(500);
    
    if(value ==  1){
	unsigned int  mess = 41;

    	// Take the time, and send it.  This will block until complete

    	printf("Now sending: %u ...",mess);
    	bool ok = radio.write( &mess, sizeof(unsigned int));
    
    	if (ok)
      		printf(" ok...");
    	else
      		printf("failed.\n\r");

    	// Now, continue listening
    	radio.startListening();

    	// Wait here until we get a response, or timeout (250ms)
    	unsigned long started_waiting_at = __millis();
    	bool timeout = false;
    	while ( ! radio.available() && ! timeout ) {
        	__msleep(30); //add a small delay to let radio.available to check payload
      		if (__millis() - started_waiting_at > 200 )
        		timeout = true;
    	}

    	// Describe the results
    	if ( timeout )
    	{
      		printf("Failed, response timed out.\n\r");
      		bcm2835_gpio_write(RADIO_1_LED, LOW);
    	}
    	else
    	{
      		// Grab the response, compare, and send to debugging spew
      		unsigned long got_time;
      		radio.read( &got_time, sizeof(unsigned long) );
      		printf("Got response %lu, round-trip delay: %lu\n\r",got_time,__millis()-got_time);
    	}
    }

}

void SyncRadios(){ 
    
    radio.stopListening();

    unsigned int  mess = 41;

    // Take the time, and send it.  This will block until complete

    printf("Now sending: %u ...",mess);
    bool ok = radio.write( &mess, sizeof(unsigned int));
    
    if (ok)
      printf(" ok...");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = __millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout ) {
        __msleep(30); //add a small delay to let radio.available to check payload
      if (__millis() - started_waiting_at > 200 )
        timeout = true;
    }
    // Describe the results
    if ( timeout )
    {
      printf("Failed, response timed out.\n\r");
      bcm2835_gpio_write(RADIO_1_LED, LOW);
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      unsigned long got_time;
      radio.read( &got_time, sizeof(unsigned long) );
       // got a reposne - turn on led
      bcm2835_gpio_write(RADIO_1_LED, LOW); // set pin low just in case and wait 3 seconds
      sleep(3); //just sleep for debugging     
      // Spew it
      printf("Established Initial Radio COntact with RF1 with response %lu, round-trip delay: %lu\n\r",got_time,__millis()-got_time);
      bcm2835_gpio_write(RADIO_1_LED, HIGH);
   }
    

}

int main(int argc, char** argv)
{
        setup();
        while(1)
	  loop();
	
	//bcm2835_gpio_write(PIN, LOW);                
        bcm2835_close();
        return 0;
}
// vim:cin:ai:sts=2 sw=2 ft=cpp
