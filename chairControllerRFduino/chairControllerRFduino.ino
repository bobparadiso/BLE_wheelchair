#include <RFduinoBLE.h>

#define DATA_PIN 6
#define DELAY_CMD 8 //ms

#define NUM_PACKETS 6
#define LEN_PACKET (3+8+1)
#define LEN_CMD (LEN_PACKET*NUM_PACKETS)

#define BIT_WIDTH 22 //uS (adjusted, really 26 uS)

#define FWD_PIN A0
#define REV_PIN A1

#define CMD_INIT   "11001001010111010110101111."

#define wirelessTimeout 500

uint32_t lastTx;
uint32_t lastRx;

char cmd_buf[LEN_CMD];

char pow2(int p)
{
  int retval = 1;
  for (int i = 0; i < p; i++)
    retval *= 2;
  return retval;
}

//
void charToString(const char val, char *string)
{
 for (int i = 0; i < 8; i++)
   string[i] = (val & pow2(i)) ? '1' : '0';
}

//
void charToStringI(const char val, char *string)
{
 for (int i = 0; i < 8; i++)
   string[i] = (val & pow2(i)) ? '0' : '1';
}

//
void stringToChar(const char *string, char &val)
{
  val = 0;
 for (int i = 0; i < 8; i++)
  if (string[i] == '1')
   val += pow2(i);
}

//example:
//    start          horn           speed[4]       y              x              checksum     
//110 01010010 1 110 10000000 1 110 11100000 1 110 00101011 0 110 01000000 1 110 11101011 0 
void buildCmd(char *buf, char speed, char y, char x)
{
  char tmp[8];
  
  //packet starts
  for (int p = 0; p < NUM_PACKETS; p++)
  {
    buf[p*LEN_PACKET + 0] = '1';
    buf[p*LEN_PACKET + 1] = '1';
    buf[p*LEN_PACKET + 2] = '0';
  }
  
  //start packet
  memcpy(buf + (0*LEN_PACKET + 3), "01010010", 8);
  
  //horn packet
  memcpy(buf + (1*LEN_PACKET + 3), "10000000", 8);
  
  //speed packet
  charToString(speed, tmp);
  memcpy(buf + (2*LEN_PACKET + 3), tmp + 4, 4);
  memcpy(buf + (2*LEN_PACKET + 3) + 4, "0000", 4);
  
  //y packet
  charToString(y, tmp);
  memcpy(buf + (3*LEN_PACKET + 3), tmp, 8);

  //x packet
  charToString(x, tmp);
  memcpy(buf + (4*LEN_PACKET + 3), tmp, 8);
  
  //checksum
  char checkVal = 0;
  for (int p = 0; p < NUM_PACKETS - 1; p++)
  {
    char val;
    stringToChar(buf + (p*LEN_PACKET + 3), val);
    checkVal += val;
  }
  charToStringI(checkVal, buf + ((NUM_PACKETS - 1) * LEN_PACKET + 3));
  
  //packet parity
  for (int p = 0; p < NUM_PACKETS; p++)
  {
    bool even = true;
    for (int b = 0; b < 8; b++)
      if (buf[p*LEN_PACKET + 3 + b] == '1')
        even = !even;
    
    buf[p*LEN_PACKET + LEN_PACKET - 1] = even ? '0' : '1';
  }  
}

//
void send(const char *bits)
{
  lastTx = millis();
  //noInterrupts();
  pinMode(DATA_PIN, OUTPUT);
  bits++;
  for (int b = 0; b < LEN_CMD; b++)
  {
    if (*bits == '0')
      digitalWrite(DATA_PIN, LOW);
    else if (*bits == '1')
      digitalWrite(DATA_PIN, HIGH);
    else
      break;
      
    delayMicroseconds(BIT_WIDTH);
    bits++;
  }

  //end with a long hi
  digitalWrite(DATA_PIN, HIGH);
  delayMicroseconds(BIT_WIDTH * 6);

  pinMode(DATA_PIN, INPUT);//release
  //interrupts();
}

//
void setup()
{
  // start the BLE stack
  RFduinoBLE.advertisementData = "wheelchair";
  RFduinoBLE.begin();
  
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  delay(250);
  digitalWrite(DATA_PIN, HIGH);
    
  //send start sequence
  for (int i = 0; i < 100; i++)
  {
    send(CMD_INIT);
    delay(DELAY_CMD);
  }
}

//
void RFduinoBLE_onConnect()
{
}

//
void RFduinoBLE_onDisconnect()
{
}

//
char rxData = '.';//bptxxx
void RFduinoBLE_onReceive(char *data, int len)
{
  rxData = data[0];
  lastRx = millis();
}

//
void loop()
{
  if (millis() - lastRx > wirelessTimeout)
   rxData = '.';
  
  switch (rxData)
  {
  case 'u': buildCmd(cmd_buf, 60, 100, 0); break;
  case 'd': buildCmd(cmd_buf, 60, -100, 0); break;
  case 'l': buildCmd(cmd_buf, 60, 0, -100); break;
  case 'r': buildCmd(cmd_buf, 60, 0, 100); break;
  default: buildCmd(cmd_buf, 60, 0, 0); break;
  }

  send(cmd_buf);
  delay(DELAY_CMD);
}

