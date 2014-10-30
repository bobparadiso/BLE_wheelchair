// Digital pin #2 is the same as Pin D2 see
// http://arduino.cc/en/Hacking/PinMapping168 for the 'raw' pin mapping
#define SAMPLE_PORT PIND
#define SAMPLE_PIN 2
#define SAMPLE1_BITS 70
#define SAMPLE2_BITS 75
#define BIT_WIDTH 26 //uS

//
void setup()
{
  Serial.begin(115200);
  Serial.println("\nReady to decode chair!");
}

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

//
char sample[SAMPLE1_BITS+SAMPLE2_BITS];
void loop()
{
  int p = 0;

  for (int w = 0; w < 6; w++)
  {
    if (w == 0)
    {
      sei();
      //wait for the end of a long hi (initial wait)
      while (true)
      {
        long start = micros();
        while (SAMPLE_PORT & (1 << SAMPLE_PIN));//still hi
        if (micros() - start > 4000)
        {
          break;//great, ready
        }
      }
      cli();
    }
    else
    {
      //wait for start of hi
      while(SAMPLE_PORT & (1 << SAMPLE_PIN) == 0);
      //wait for the end of hi (2 bits, next byte)
      while (SAMPLE_PORT & (1 << SAMPLE_PIN));//still hi  
    }
    
    sample[p++] = '1';
    sample[p++] = '1';
    
    delayMicroseconds(BIT_WIDTH/4);
    
    for (int i = 0; i < 10; i++)
    {
      sample[p++] = (SAMPLE_PORT & (1 << SAMPLE_PIN)) == 0 ? '0' : '1';  
      delayMicroseconds(BIT_WIDTH);
    }
  }
  

  sei();
    //wait for the end of a long hi (communication direction switch)
  while (true)
  {
    long start = micros();
    while (SAMPLE_PORT & (1 << SAMPLE_PIN));//still hi
    if (micros() - start > 500)
    {
      break;//great, ready
    }
  }

  cli();
  delayMicroseconds(BIT_WIDTH/4);
  
  for (int i = SAMPLE1_BITS; i < SAMPLE1_BITS+SAMPLE2_BITS; i++)
  {
    sample[2+i] = (SAMPLE_PORT & (1 << SAMPLE_PIN)) == 0 ? '0' : '1';  
    delayMicroseconds(BIT_WIDTH);
  }

  sei();

  
  
/*  for (int i = 0; i < SAMPLE1_BITS+SAMPLE2_BITS; i++)
  {
    Serial.print(sample[i]);
  }*/

  p = 0;

  char bytes[6][8];
  for (int b = 0; b < 6; b++)
  {
    for (int i = 0; i < 3; i++)
      Serial.print(sample[p++]);
    Serial.print(' ');
    
    for (int i = 0; i < 8; i++)
    {
      bytes[b][i] = sample[p];
      Serial.print(sample[p++]);
    }
    Serial.print(' ');
  
    Serial.print(sample[p++]);
    Serial.print(' ');
  }

  char checkVal = 0;
  for (int b = 0; b < 5; b++)
  {
    char val;
    stringToChar(bytes[b], val);
    checkVal += val;
  }
  char check[8];
  charToStringI(checkVal, check);
  
  Serial.print("  ");
  for (int i = 0; i < 8; i++)
    Serial.print(check[i]);
  
  Serial.println();
  Serial.flush();
}
