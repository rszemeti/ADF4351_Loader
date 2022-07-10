//
//*****************************************************************
// ADF4351 PLL-Synthesizer 33Mhz - 4,4Ghz
// 
// Headless for use as a 1khz modulated source for the HP 415E "SWR Meter" on antenna test ranges.
//
//
// License: adapted from code by oe6ocg, and others.
//*****************************************************************

// Remember to use resistor divider to translate Arduino 5V to 3.3V for the 
// VCO board.  It *will not like* 5V!

#include <Wire.h>
#include <SPI.h>
#include <avr/sleep.h>

// Uses SPI 0 (2 pins) plus a "select pin" and "modulation pin";

const int slaveSelectPin1 = 3;  // Slave 1 LE select
const int modPin = 4;           // Modulation ouptut for PA stage

// Uses 4 digital inputs to select 8 bands, plus mod on/off
const int sw1 = A5;  //LSB
const int sw2 = A4;
const int sw4 = A3;  
const int sw8 = A2;  //MSB & modulation on/off


/*
 * Have to use F in kHz as high (4.4GHz) freq will not fit into an unsigned long
 */
unsigned static const long Freq[8] = { 
  50200,
  70200,
  144200,
  432200,
  1296200,
  2300200,
  2320200,
  3400200
};

long refin = 25000000; // 10Mhz
long ChanStep = 25000; //Channel spacing

int band;
int modulation;

unsigned long Reg[6]; //ADF4351 Reg's

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(modPin, OUTPUT);

  pinMode(sw1, INPUT_PULLUP);
  pinMode(sw2, INPUT_PULLUP);
  pinMode(sw4, INPUT_PULLUP);
  pinMode(sw8, INPUT_PULLUP);

  Serial.begin(115200);// USB to PC for Debug only
  Serial.println("Starting ADF4351");
  Serial.print("Ref Frequency  (Hz): "); Serial.println(refin);

  for(int i=0;i<8;i++){
    Serial.print("Band "); 
    Serial.print(i); 
    Serial.print(" Frequency (kHz): "); 
    Serial.println(Freq[i]);
  }

  // setup the control pin for writing
  pinMode (slaveSelectPin1, OUTPUT);
  digitalWrite(slaveSelectPin1, LOW);

  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV128);
  SPI.begin();
  delay(500);


  /* 2175 Mhz */

/*
 * known good 2175mhz for chinese eval board
 * */

/*
 *   
  Reg[5] = 0x580005;
  Reg[4] = 0x92803C;
  Reg[3] = 0x4B3;
  Reg[2] = 0x18005F42;
  Reg[1] = 0x8009F41;
  Reg[0] = 0x570000;
*/

  band = -1;
  modulation = -1;

  Serial.println("Setup done, ready.");


}



void loop() {

  int newBand = 7 - digitalRead(sw1)-
                2*digitalRead(sw2)-
                4*digitalRead(sw4);

  if(band != newBand){

    band = newBand;

    Serial.print("Setting band ");
    Serial.print(band);
    Serial.print(", ");
    Serial.print(Freq[band]);
    Serial.println(" kHz");
    ConvertFreq(Freq[band], Reg);
    SetFreq(slaveSelectPin1);
  }

  int newMod = digitalRead(sw8);
  if(modulation != newMod){
    modulation = newMod;
    if(modulation){
    Serial.println("Modulation on");
    tone(modPin,1000);
    }else{
      Serial.println("Modulation off");
      noTone(modPin);
      digitalWrite(modPin,HIGH);
    }
  }
  
  delay(1000);  
}

void SetFreq(int ssPin)
{
  WriteADF2(5, ssPin);
  delayMicroseconds(2500);
  WriteADF2(4, ssPin);
  delayMicroseconds(2500);
  WriteADF2(3, ssPin);
  delayMicroseconds(2500);
  WriteADF2(2, ssPin);
  delayMicroseconds(2500);
  WriteADF2(1, ssPin);
  delayMicroseconds(2500);
  WriteADF2(0, ssPin);
  delayMicroseconds(2500);
}

void WriteADF2(int idx, int ssPin)
{
  //Make sure the idx is written into the first 3 bits
  Reg[idx]= (Reg[idx] & 0xFFFFFFF8) + (0x07 & idx);
  
  Serial.print("Reg ");
  Serial.print(idx);
  Serial.print(": ");
  Serial.print(Reg[idx], HEX);
  Serial.print("\n");
  // make 4 byte from integer for SPI-Transfer
  byte buf[4];
  for (int i = 0; i < 4; i++)
    buf[i] = (byte)(Reg[idx] >> (i * 8));
  WriteADF(buf[3], buf[2], buf[1], buf[0], ssPin);
}

int WriteADF(byte a1, byte a2, byte a3, byte a4, int ssPin) {
  // write over SPI to ADF435x
  digitalWrite(ssPin, LOW);
  delayMicroseconds(10);
  SPI.transfer(a1);
  SPI.transfer(a2);
  SPI.transfer(a3);
  SPI.transfer(a4);
  Toggle(ssPin);
}

int Toggle(int ssPin) {
  digitalWrite(ssPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(ssPin, LOW);
}

void ConvertFreq(unsigned long freq, unsigned long R[])
{
  // PLL-Reg-R0         =  32bit
  // Registerselect        3bit
  // int F_Frac = 4;       // 12bit
  // int N_Int = 92;       // 16bit
  // reserved           // 1bit

  // PLL-Reg-R1         =  32bit
  // Registerselect        3bit
  //int M_Mod = 5;        // 12bit
  int P_Phase = 1;     // 12bit bei 2x12bit hintereinander pow()-bug !!
  int Prescal = 1;     // 1bit geht nicht ???
  int PhaseAdj = 0;    // 1bit geht auch nicht ???
  // reserved           // 3bit

  // PLL-Reg-R2         =  32bit
  // Registerselect        3bit
  int U1_CountRes = 0; // 1bit
  int U2_Cp3state = 0; // 1bit
  int U3_PwrDown = 0;  // 1bit
  int U4_PDpola = 1;    // 1bit
  int U5_LPD = 0;       // 1bit
  int U6_LPF = 1;       // 1bit 1=Integer, 0=Frac not spported yet
  int CP_ChgPump = 15;     // 4bit  7 -> 2.50mA,   15 -> 5.00mA
  int D1_DoublBuf = 0; // 1bit
  //  int R_Counter = 1;   // 10bit
  //  int RD1_Rdiv2 = 0;    // 1bit
  //  int RD2refdoubl = 0; // 1bit
  int M_Muxout = 6;     // 3bit 6-> digital lock detect
  int LoNoisSpur = 0;      // 2bit 0-> low noise mode
  // reserved           // 1bit

  // PLL-Reg-R3         =  32bit
  // Registerselect        3bit
  int D_Clk_div = 150; // 12bit
  int C_Clk_mode = 0;   // 2bit
  //  reserved          // 1bit
  int F1_Csr = 0;       // 1bit
  //  reserved          // 2bit
  int F2_ChgChan = 0;   // 1bit
  int F3_ADB = 0;       // 1bit
  int F4_BandSel = 0;  // 1bit
  //  reserved          // 8bit

  // PLL-Reg-R4         =  32bit
  // Registerselect        3bit
  int D_out_PWR = 3 ;    // 2bit
  int D_RF_ena = 1;     // 1bit
  int D_auxOutPwr = 0;  // 2bit
  int D_auxOutEna = 0;  // 1bit
  int D_auxOutSel = 0;  // 1bit
  int D_MTLD = 0;       // 1bit
  int D_VcoPwrDown = 0; // 1bit 1=VCO off



  int D_RfDivSel = 3;    // 3bit set automatically further down
  int D_FeedBck = 1;     // 1bit
  // reserved           // 8bit

  // PLL-Reg-R5         =  32bit
  // Registerselect     // 3bit
  // reserved           // 16bit
  // reserved     11    // 2bit
  // reserved           // 1bit
  int D_LdPinMod = 1;    // 2bit muss 1 sein
  // reserved           // 8bit

  // Referenz Freg Calc
  int R_Counter = 1;   // 10bit
  int RD1_Rdiv2 = 0;    // 1bit
  int RD2refdoubl = 0; // 1bit
  int B_BandSelClk = 200; // 8bit


// an unsigend long wont quite hold 4.4GHz, so work with f/10 for now.
  unsigned long RFout = freq * 100;   // VCO-Frequenz
  // calc bandselect und RF-div
  int outdiv = 1;

  if (RFout >= 220000000) {
    outdiv = 1;
    D_RfDivSel = 0;
  }
  if (RFout < 220000000) {
    outdiv = 2;
    D_RfDivSel = 1;
  }
  if (RFout < 110000000) {
    outdiv = 4;
    D_RfDivSel = 2;
  }
  if (RFout < 55000000) {
    outdiv = 8;
    D_RfDivSel = 3;
  }
  if (RFout < 27500000) {
    outdiv = 16;
    D_RfDivSel = 4;
  }
  if (RFout < 13800000) {
    outdiv = 32;
    D_RfDivSel = 5;
  }
  if (RFout < 6900000) {
    outdiv = 64;
    D_RfDivSel = 6;
  }

  // Work with f/10 here
  float PFDFreq = (refin/10) * ((1.0 + RD2refdoubl) / (R_Counter * (1.0 + RD1_Rdiv2))); //Referenzfrequenz /10
  float N = ((RFout) * outdiv) / PFDFreq;
  int N_Int = N;

  // Set up the band select divider, for max PFDFreq/B_BandSel = 125KHz
  B_BandSelClk = (int)10*PFDFreq/125000;
  
  long M_Mod = PFDFreq * (1000000 / ChanStep) / 100000; // the effective "* 10" here completes the conversion from kHz to real freq, * 100 was done further up.
  int F_Frac = round((N - N_Int) * M_Mod);

  R[0] = (unsigned long)(0 + F_Frac * pow(2, 3) + N_Int * pow(2, 15));
  // Sometimes R[1} does not get its register index set as 1, I think the M_Mod * pow(2, 3) overflows or something.
  // We force these bits on index WriteADF2 anyway, so its not an issue.
  R[1] = (unsigned long)(1 + M_Mod * pow(2, 3) + P_Phase * pow(2, 15) + Prescal * pow(2, 27) + PhaseAdj * pow(2, 28));
  R[2] = (unsigned long)(2 + U1_CountRes * pow(2, 3) + U2_Cp3state * pow(2, 4) + U3_PwrDown * pow(2, 5) + U4_PDpola * pow(2, 6) + U5_LPD * pow(2, 7) + U6_LPF * pow(2, 8) + CP_ChgPump * pow(2, 9) + D1_DoublBuf * pow(2, 13) + R_Counter * pow(2, 14) + RD1_Rdiv2 * pow(2, 24) + RD2refdoubl * pow(2, 25) + M_Muxout * pow(2, 26) + LoNoisSpur * pow(2, 29));
  R[3] = (unsigned long)(3 + D_Clk_div * pow(2, 3) + C_Clk_mode * pow(2, 15) + 0 * pow(2, 17) + F1_Csr * pow(2, 18) + 0 * pow(2, 19) + F2_ChgChan * pow(2, 21) +  F3_ADB * pow(2, 22) + F4_BandSel * pow(2, 23) + 0 * pow(2, 24));
  R[4] = (unsigned long)(4 + D_out_PWR * pow(2, 3) + D_RF_ena * pow(2, 5) + D_auxOutPwr * pow(2, 6) + D_auxOutEna * pow(2, 8) + D_auxOutSel * pow(2, 9) + D_MTLD * pow(2, 10) + D_VcoPwrDown * pow(2, 11) + B_BandSelClk * pow(2, 12) + D_RfDivSel * pow(2, 20) + D_FeedBck * pow(2, 23));
  R[5] = (unsigned long)(5 + 0 * pow(2, 3) + 3 * pow(2, 19) + 0 * pow(2, 21) + D_LdPinMod * pow(2, 22));
}
//to do instead of writing 0x08000000 you can use other two possibilities: (1ul << 27) or (uint32_t) (1 << 27).


// as PLL-Register Referenz
// R[0] = (0x002E0020); // 145.0 Mhz, 12.5khz raster
// R[1] = (0x08008029);
// R[2] = (0x00004E42);
// R[3] = (0x000004B3);
// R[4] = (0x00BC8024);
// R[5] = (0x00580005);
