#include <SPI.h>
#include <LoRa.h>

#define FREQUENCY 433E6
#define SPREADING_FACTOR 7
#define SYNC_WORD 0x34

#if not defined(__AVR_ATmega328P__)

  
  
  #define csPin 8
  #define resetPin 4
  #define irqPin 7
  

  #define SERIAL Serial1
  #define DEBUG Serial

  #define LED LED_BUILTIN

  #define SERIAL_TIMEOUT 5
#else
  #define SERIAL Serial
  #define LED 8

  #define SERIAL_TIMEOUT 5

  #define GATEWAY
#endif

#define ASYNC




unsigned long lastSerialByteMilis = 0;
bool sending = false;

char serialBuffer[256];
unsigned int serialBufferNum = 0;

void loop() {
  unsigned long currentMillis = millis();

  if((SERIAL.available() > 0) && !sending) {
    lastSerialByteMilis = currentMillis;

    while(SERIAL.available() > 0) {
      char recv = (char)SERIAL.read();
      serialBuffer[serialBufferNum] = recv;
      serialBufferNum++;

#ifdef DEBUG
      //DEBUG.print("\\x");
      //DEBUG.println((byte)recv, HEX);
#endif
    }
  }

  if((lastSerialByteMilis+SERIAL_TIMEOUT < currentMillis) && serialBufferNum > 0) {

    LoRa_txMode();
    LoRa.beginPacket();
    sending = true;
    
    for(int i = 0; i < serialBufferNum; i++) {
#ifdef DEBUG
      DEBUG.print("\\x");
      DEBUG.print((byte)serialBuffer[i],HEX);
#endif
      LoRa.write(serialBuffer[i]);
    }
#ifdef DEBUG
    DEBUG.println(" ->");
#endif

    serialBufferNum = 0;
    
#ifdef ASYNC
    LoRa.endPacket(true);
#else
    LoRa.endPacket(true);
    LoRa_rxMode();
    sending = false;
#endif
  }

  
  
}

void onReceive(int packetSize) {
  //if (packetSize == 0) return;
  
#ifdef DEBUG
  DEBUG.print("Received packet '");
#endif
  for (int i = 0; i < packetSize; i++) {
    byte recv = (byte)LoRa.read();
    SERIAL.print((char)recv);
#ifdef DEBUG
    DEBUG.print("\\x");
    DEBUG.print(recv, HEX);
#endif
  }
#ifdef DEBUG
  DEBUG.print("' with ");
  DEBUG.print("RSSI: " + String(LoRa.packetRssi()));
  DEBUG.println(" Snr: " + String(LoRa.packetSnr()));
#endif

}

void setup() {
  SERIAL.begin(9600);

#ifdef DEBUG
  DEBUG.begin(9600);
  //while(!DEBUG);
#endif
  
#if not defined(__AVR_ATmega328P__)
  LoRa.setPins(csPin, resetPin, irqPin);
#endif
  if (!LoRa.begin(FREQUENCY)) {
    while (true);
  }

  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSyncWord(SYNC_WORD);
  LoRa.enableCrc();
  
  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  pinMode(LED, OUTPUT);
#ifdef DEBUG
  DEBUG.println("---START---");
#endif
}



/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
/////////////////////////////////////
void LoRa_rxMode(){
#ifdef GATEWAY
  LoRa.disableInvertIQ();
#else
  LoRa.enableInvertIQ();
#endif
  LoRa.receive();
}
void LoRa_txMode(){
  LoRa.idle();
#ifdef GATEWAY
  LoRa.enableInvertIQ();
#else
  LoRa.disableInvertIQ();
#endif
}
void onTxDone() {
  LoRa_rxMode();
  sending = false;
#ifdef DEBUG
  DEBUG.println("TxDone");
#endif
}
