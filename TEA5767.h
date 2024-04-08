/*
	Project  : TEA5767
 	file     : TEA5767.h
	Author   : ykchau
 	youtube  : youtube.com/ykchau888
  	Licenese : GPL-3.0
   	Please let me know if you use it commercial project.
*/

#ifndef TEA5767_H_
#define TEA5767_H_

#include <Arduino.h>
#include <Wire.h>

#define TEA5767_I2C_ADDRESS 0x60

// bit operation
#define bit_set(val, bitPos)           (val |= 1 << bitPos)
#define bit_clear(val, bitPos)         (val &= ~( 1 << bitPos))
#define bit_toggle(val, bitPos)        (val ^= 1 << bitPos)
#define bit_get(out, val, bitPos)      (out = (val >> bitPos) & 1)
#define bit_change(val, bitPos, to)    (val ^= (-to ^ val) & (1 << bitPos))

#define SPT(str,value) Serial.print(String(str) + value)
#define SPL(str,value) Serial.println(String(str) + value)
#define SPH(str,value) { Serial.print(String("[HEX]") + str); Serial.println(value,HEX); }

#define between(valueToCompare, rangeMin, rangeMax) (( valueToCompare >= rangeMin ) && ( valueToCompare <= rangeMax ))

/*
    Mask of 5 data bytes in write mode
*/
// data byte 1
#define TEA5767_MASK_MUTE               7
#define TEA5767_MASK_SEARCH_MODE        6

// data byte 1 [13:8](i.e. [5:0])  and data byte 2 [7:0]
// is the setting of synthesizer programmable counter for search or preset

// data byte 3
#define TEA5767_MASK_SEARCH_DIRECTION   7
#define TEA5767_MASK_SSL_H              6
#define TEA5767_MASK_SSL_L              5
#define TEA5767_MASK_SIDE_INJECTION     4
#define TEA5767_MASK_MODE               3
#define TEA5767_MASK_MUTE_RIGHT         2
#define TEA5767_MASK_MUTE_LEFT          1
#define TEA5767_MASK_SWP1               0

// data byte 4
#define TEA5767_MASK_SWP2               7
#define TEA5767_MASK_STANDBY            6
#define TEA5767_MASK_BAND               5    // JP - 76 MHz ~ 91 MHz, US/EU - 87.5 MHz ~ 108 MHz
#define TEA5767_MASK_XTAL               4    // Onbaord 32.768KHz XTAL, Always 1
#define TEA5767_MASK_SMUTE              3
#define TEA5767_MASK_HCC                2   // High Cut Control
#define TEA5767_MASK_SNC                1   // Stereo Noise Cancelling
#define TEA5767_MASK_SEARCH_INDICATOR   0   // Search indicator

// data byte 5  [5:0] Don't Care
#define TEA5767_MASK_PLLREF             7    // Onbaord 32.768KHz XTAL, Always 0
#define TEA5767_MASK_DTC                6

/*
    Mask of 5 data bytes in read mode
*/
// data byte 1
#define TEA5767_MASK_READY_FLAG         7
#define TEA5767_MASK_BAND_LIMIT_FLAG    6

// data byte 1 [13:8](i.e. [5:0]) and data byte 2 [7:0]
// is the setting of synthesizer programmable counter for search or preset

// data byte 3
#define TEA5767_MASK_READ_MODE          7

// data byte 3[6:0] Immediate Freq

// data byte 4
#define TEA5767_MASK_ADC_LEVEL3         7
#define TEA5767_MASK_ADC_LEVEL2         6
#define TEA5767_MASK_ADC_LEVEL1         5
#define TEA5767_MASK_ADC_LEVEL0         4
#define TEA5767_MASK_CHIP_ID3           3
#define TEA5767_MASK_CHIP_ID2           2
#define TEA5767_MASK_CHIP_ID1           1

// data byte 5 - reserved, not in use

// Flags
#define TEA5767_READ_TIMEOUT    1
#define TEA5767_READ_OK         0

#define TEA5767_ON              1
#define TEA5767_OFF             0

#define TEA5767_UP              1
#define TEA5767_DOWN            0

#define TEA5767_SSL_NA          0
#define TEA5767_SSL_LOW         5
#define TEA5767_SSL_MID         7
#define TEA5767_SSL_HIGH        10

#define TEA5767_INJECTION_HIGH  1
#define TEA5767_INJECTION_LOW   0

#define TEA5767_MONO            1
#define TEA5767_STEREO          0

#define TEA5767_LEFT            1
#define TEA5767_RIGHT           0
#define TEA5767_ALL             2

#define TEA5767_JP              1
#define TEA5767_US_EU           0

#define TEA5767_OUTPUT          1
#define TEA5767_SWP1            0

#define TEA5767_SWP_PORT_1      1
#define TEA5767_SWP_PORT_2      2

#define TEA5767_DTC_75US        1
#define TEA5767_DTC_50US        0

#define TEA5767_DEFAULT_FREQ    87.5
#define TEA5767_XTAL            32768   // Default on board XTAL is 32768Hz

#define TEA5767_ERROR           0xFF
#define TEA5767_NOT_READY       0xFE
#define TEA5767_READ_AGAIN      0xFD

#define TEA5767_MUTE_ON         1
#define TEA5767_MUTE_OFF        0

// Search routine
#define TEA5767_SEARCH_STOP     0
#define TEA5767_SEARCH_PENDING  1
#define TEA5767_SEARCH_COMPLETE 2

#define TEA5767_SEARCH_PRESET_NO    0
#define TEA5767_SEARCH_PRESET_YES   1


typedef struct TEA5767_Status {
    byte rawData[5];

    byte radioReady = 0;      // 1 ready, 0 no station found
    byte reachBandLimit = 0;  // 1 reached, 0 not reached

    float currentFreq = TEA5767_DEFAULT_FREQ;
    byte IFCounter = 0x37;  // 225KHz
    byte readTimeout = 0;   // 1 timeout, 0 OK

    byte radioMode = TEA5767_STEREO;  // 1 Stereo, 0 Mono
    byte Sound_Left = TEA5767_MUTE_OFF;
    byte Sound_Right = TEA5767_MUTE_OFF;
    byte Sound_All = TEA5767_MUTE_OFF;

    byte searchMode = TEA5767_OFF;
    byte injection = TEA5767_INJECTION_HIGH;
    byte ADCLevel = 0;  // ADC level of the current Frequency

    byte SoftMute = TEA5767_MUTE_OFF;
    byte HCC = TEA5767_OFF;
    byte SNC = TEA5767_OFF;
    byte DTC = TEA5767_DTC_50US;

    byte dir = TEA5767_UP;
    byte band = TEA5767_US_EU;
    byte ssl = TEA5767_SSL_HIGH;
    float minFreq = 87.5;   // Default US band
    float maxFreq = 108.0;  // Default US band

} TEA5767_Status;

class TEA5767 {
   private:
    byte writeData[5];  // Write Buffer

    void I2C_Write();
    byte I2C_Read();

    void setOnOff(byte *data, byte bitPos, byte onOff);  // modify bit in writeData of specific parameter

    void optimalSideInjection(float freq);

    // Preset for Auto Scan
    float *presetFreq;
    int presetFreqSize = 0;
    int curPreset = 0;
    void addFreqPreset(float freq);
    
    // Config, the following functino didn't send to the device before using I2C_Write()
    // data byte 1
    void setMute(byte mute);      // on/off
    void setSearchMode(byte sm);  // on/off
    // data byte 2
    void setFreq(float freq);
    // data byte 3
    void setSearchDirection(byte dir);             // up/down
    void setSearchStopLevel(byte ssl);             // NA/LOW/MID/HIGH
    void setSideInjectionMode(byte injection);     // High/Low
    void setRadioMode(byte mode);                  // Stereo/Mono
    void setMuteChannel(byte channel, byte mute);  // Left/Right, on/off
    void setSWP(byte port, byte mode);             // 1/2, on/off | normal off
    // data byte 4
    void setStandby(byte mode);               // on/off
    void setBand(byte band);                  // JP(76MHz ~ 91MHz) / USEU(87.5MHz ~ 108MHz)
    void setXTAL();                           // onboard XTAL is 32.768
    void setSoftMute(byte mute);              // on/off
    void setHighCutControl(byte hcc);         // on/off
    void setStereoNoiseCancelling(byte snc);  // on/off
    void setSearchIndicator(byte mode);       // output/SWP1 program
    // data byte 5
    void setDeemphasisTimeConstant(byte dtc);  // 75us/50us

   public:
    float searchingFreq = 87.5;
    byte searchProcessStatus = 0;
    byte searchPreset = TEA5767_SEARCH_PRESET_NO;

    TEA5767_Status status;
    TEA5767() {
        init();
    };
    void init();  // Module config init

    void pause();        // Put system to standby mode
    void resume();       // Wake up from standby mode
    void printStatus();  // Print TEA5767_status data to serial port
    byte read_status();  // read and extract I2C_Read rawData

    // Set station with specific frequency
    void setStation(float freq);

    // Auto search/scan station
    void scanStation(byte ssl);
    void searchStation(byte dir, byte ssl);
    void searchProcess();

    // Preset functions for Auto Scan
    void nextPreset();
    void prevPreset();
    void printPreset();
    void deleteCurFreqPreset();

    // Toggle, update device with specific flag
    void toggleMute(byte channel);
    void toggleSoftMute();
    void toggleHighCutControl();
    void toggleStereoNoiseCancelling();
    void toggleDeemphasisTimeConstant();
    void toggleMode();
};

#endif  // TEA5767_H_
