/*
	Project  : TEA5767
 	file     : TEA5767.cpp
	Author   : ykchau
 	youtube  : youtube.com/ykchau888
  	Licenese : GPL-3.0
   	Please let me know if you use it commercial project.
*/
#include "TEA5767.h"

/**************************
    private:
**************************/
// Write instruction to TEA5767 via I2C
void TEA5767::I2C_Write() {
    // SPH("WD1 :", writeData[0]);
    // SPH("WD2 :", writeData[1]);
    // SPH("WD3 :", writeData[2]);
    // SPH("WD4 :", writeData[3]);
    // SPH("WD5 :", writeData[4]);

    Wire.beginTransmission(TEA5767_I2C_ADDRESS);
    for (byte i = 0; i < 5; i++) {
        Wire.write(writeData[i]);
    }
    Wire.endTransmission();

    // because TEA5767 need ~28ms to get the IF counter
    // therefore we wait 35ms let it complete
    delay(35);
}

// Read status from TEA5767 via I2C
byte TEA5767::I2C_Read() {
    byte rec = 0;
    unsigned long startTime = millis();

    while (rec == 0) {
        rec = Wire.requestFrom(TEA5767_I2C_ADDRESS, 5);

        if (millis() > startTime + 20) {  // 20ms timeout
            status.readTimeout = TEA5767_READ_TIMEOUT;
            SPL("Timeout", " ");
            break;
        }
    }

    if (rec >= 5) {
        for (byte i = 0; i < 5; i++) {
            status.rawData[i] = Wire.read();
        }
        status.readTimeout = TEA5767_READ_OK;
    }

    return status.readTimeout;
}

// Modify bit in writeData
void TEA5767::setOnOff(byte *data, byte bitPos, byte onOff) {
    if (onOff == TEA5767_ON) {
        bit_set(*data, bitPos);
    } else {
        bit_clear(*data, bitPos);
    }
}

// Find the correct frequency by using Side injection technique
// This process is follow the application notes
void TEA5767::optimalSideInjection(float freq) {
    // method from application note page 27
    // https://www.voti.nl/docs/AN10133.pdf
    // https://en.wikipedia.org/wiki/Superheterodyne_receiver#Image_frequency
    // https://en.wikipedia.org/wiki/Image_response

    byte levelLow, levelHigh;

    // application note page 24,
    // IF(MHz) = IFCounter * (64 * ( Sys clock / 512 ))/1,000,000
    //         = IFCounter * (64 * ( 32,768 / 512 ))/1,000,000
    //         = IFCounter * (64 * 64) / 1,000,000
    //         = IFCounter * 0.004096
    // 2 * IF = IFCounter * 0.004096 * 2
    float doubleIF = 0.45;  //status.IFCounter * 0.008192;

    // Test High and Low level signal of Frequency input
    // by High/Low LO Injection by +/- 445KHz
    setSideInjectionMode(TEA5767_INJECTION_HIGH);

    setFreq(freq + doubleIF);
    I2C_Write();
    read_status();
    levelHigh = status.ADCLevel;

    setSideInjectionMode(TEA5767_INJECTION_LOW);
    setFreq(freq - doubleIF);
    I2C_Write();
    read_status();
    levelLow = status.ADCLevel;

    status.injection = (levelHigh < levelLow) ? TEA5767_INJECTION_HIGH : TEA5767_INJECTION_LOW;
}

// Add Freq to Preset
void TEA5767::addFreqPreset(float freq) {
    if (presetFreqSize == 0) {
        // init a new array
        presetFreqSize = 1;
        curPreset = 0;
        presetFreq = (float *)calloc(presetFreqSize, sizeof(float));
        presetFreq[curPreset] = freq;
    } else {
        // append
        presetFreq = (float *)realloc(presetFreq, sizeof(float) * ++presetFreqSize);
        curPreset = presetFreqSize - 1;
        presetFreq[curPreset] = freq;
    }
}

// Read and extract I2C_Read rawData
byte TEA5767::read_status() {
    byte timeoutretry = 5;
    byte radioReadyRetry = 5;
    status.radioReady = 0;

    while (status.radioReady == 0) {
        // get data from I2C
        while (I2C_Read() == TEA5767_READ_TIMEOUT) {
            if (timeoutretry == 0) {
                SPL("TEA5767 : Error, please check connection.", " ");
                return TEA5767_ERROR;
            }
            SPL("TEA5767 : I2C Timeout and retry.", " ");
            timeoutretry--;
        }
        timeoutretry = 5;

        // Extract data
        // read current freq
        unsigned int PLL_Dec = (((status.rawData[0] & 0x3F) << 8) + status.rawData[1]);

        if (PLL_Dec != 0) {
            // SPH("RD1 ", status.rawData[0]);
            // SPH("RD2 ", status.rawData[1]);
            // SPH("RD3 ", status.rawData[2]);
            // SPH("RD4 ", status.rawData[3]);
            // SPH("RD5 ", status.rawData[4]);

            bit_get(status.radioReady, status.rawData[0], TEA5767_MASK_READY_FLAG);
            bit_get(status.reachBandLimit, status.rawData[0], TEA5767_MASK_BAND_LIMIT_FLAG);
            bit_get(status.radioMode, status.rawData[2], TEA5767_MASK_READ_MODE);

            status.IFCounter = status.rawData[2] & 0x7F;

            if (status.injection == TEA5767_INJECTION_HIGH) {
                status.currentFreq = (PLL_Dec * 32.768 - 225) / 4000;
            } else {  // TEA5767_INJECTION_LOW
                status.currentFreq = (PLL_Dec * 32.768 + 225) / 4000;
            }
            // SPL("CUR FREQ : ", status.currentFreq);

            // read ADC Level
            status.ADCLevel = status.rawData[3] >> 4;
            
            // to aviod infinite loop, quit if radio not ready
            // Radio not ready is always because the weak signal
            if ( radioReadyRetry > 5 ) {
                return TEA5767_NOT_READY;
            } else {
                radioReadyRetry--;
            }
        } else {  // read again when get wrong data
            status.radioReady = 0;
        }
    }

    return TEA5767_READ_OK;
}

/*
    Config
    the following functino didn't send to the device before using I2C_Write()
    use toggle instead;
*/
// if MUTE = 1 then L and R audio are muted; if MUTE = 0 then L and R audio are not muted
void TEA5767::setMute(byte mute) {
    setOnOff(&writeData[0], TEA5767_MASK_MUTE, mute);
    status.Sound_All = mute;
}

// Search mode: if SM = 1 then in search mode; if SM = 0 then not in search mode
void TEA5767::setSearchMode(byte mode) {
    setOnOff(&writeData[0], TEA5767_MASK_SEARCH_MODE, mode);
}

// Freq. in MHz
void TEA5767::setFreq(float freq) {
    writeData[0] &= 0xC0;  // clear PLL bits
    writeData[1] = 0;

    // Calculate the PLL decimal value
    unsigned int PLL_Dec;

    if (status.injection == TEA5767_INJECTION_HIGH) {
        PLL_Dec = round((4 * (freq * 1000 + 225)) / (32.768));
    } else {  // TEA5767_INJECTION_LOW
        PLL_Dec = round((4 * (freq * 1000 - 225)) / (32.768));
    }

    // put in writeData
    writeData[0] |= (PLL_Dec >> 8);
    writeData[1] = PLL_Dec & 0xFF;
}

// Search Up/Down: if SUD = 1 then search up; if SUD = 0 then search down
void TEA5767::setSearchDirection(byte dir) {
    setOnOff(&writeData[2], TEA5767_MASK_SEARCH_DIRECTION, dir);
    status.dir = dir;
}

// Search Stop Level : NA in search mode / LOW ADC(5) / MID ADC(7) / HIGH ADC(10)
void TEA5767::setSearchStopLevel(byte ssl) {
    byte ssl1, ssl0;
    switch (ssl) {
        case TEA5767_SSL_HIGH:
            ssl1 = 1;
            ssl0 = 1;
            break;
        case TEA5767_SSL_MID:
            ssl1 = 1;
            ssl0 = 0;
            break;
        case TEA5767_SSL_LOW:
            ssl1 = 0;
            ssl0 = 1;
            break;
        case TEA5767_SSL_NA:
            ssl1 = 0;
            ssl0 = 0;
            break;
        default:
            ssl1 = 1;
            ssl0 = 1;
            break;
    }
    setOnOff(&writeData[2], TEA5767_MASK_SSL_H, ssl1);
    setOnOff(&writeData[2], TEA5767_MASK_SSL_L, ssl0);
}

// High/Low Side Injection: if HLSI = 1 then high side LO injection; if HLSI = 0 then low side LO injection
void TEA5767::setSideInjectionMode(byte injection) {
    setOnOff(&writeData[2], TEA5767_MASK_SIDE_INJECTION, injection);
}

// Mono to Stereo: if MS = 1 then forced mono; if MS = 0 then stereo ON
void TEA5767::setRadioMode(byte mode) {
    setOnOff(&writeData[2], TEA5767_MASK_MODE, mode);
}

// Mute Right: if MR = 1 then the right audio channel is muted and forced mono; if MR = 0 then the right audio channel is not muted
// Mute Left: if ML = 1 then the left audio channel is muted and forced mono; if ML = 0 then the left audio channel is not muted
void TEA5767::setMuteChannel(byte channel, byte mute) {
    if (channel == TEA5767_LEFT) {
        setOnOff(&writeData[2], TEA5767_MASK_MUTE_LEFT, mute);
        status.Sound_Left = mute;
    } else {  // TEA5767_RIGHT
        setOnOff(&writeData[2], TEA5767_MASK_MUTE_RIGHT, mute);
        status.Sound_Right = mute;
    }
}

// Software programmable port 1: if SWP1 = 1 then port 1 is HIGH; if SWP1 = 0 then port 1 is LOW
// Software programmable port 2: if SWP2 = 1 then port 2 is HIGH; if SWP2 = 0 then port 2 is LOW
void TEA5767::setSWP(byte port, byte mode) {
    if (port == TEA5767_SWP_PORT_1) {
        setOnOff(&writeData[2], TEA5767_MASK_SWP1, mode);
    } else {  // TEA5767_RIGHT
        setOnOff(&writeData[3], TEA5767_MASK_SWP2, mode);
    }
}

// Standby: if STBY = 1 then in Standby mode; if STBY = 0 then not in Standby mode
void TEA5767::setStandby(byte mode) {
    setOnOff(&writeData[3], TEA5767_MASK_STANDBY, mode);
}

// JP(76MHz ~ 91MHz) / USEU(87.5MHz ~ 108MHz)
void TEA5767::setBand(byte band) {
    setOnOff(&writeData[3], TEA5767_MASK_BAND, band);
    status.band = band;

    if (band == TEA5767_JP) {
        status.minFreq = 76.0;
        status.maxFreq = 91.0;
    } else {
        status.minFreq = 87.5;
        status.maxFreq = 108.0;
    }
}

// onboard XTAL is 32.768
void TEA5767::setXTAL() {
    setOnOff(&writeData[3], TEA5767_MASK_XTAL, 1);
    setOnOff(&writeData[4], TEA5767_MASK_PLLREF, 0);
}

// Soft Mute: if SMUTE = 1 then soft mute is ON; if SMUTE = 0 then soft mute is OFF
void TEA5767::setSoftMute(byte mute) {
    setOnOff(&writeData[3], TEA5767_MASK_SMUTE, mute);
    status.SoftMute = mute;
}

// High Cut Control: if HCC = 1 then high cut control is ON; if HCC = 0 then high cut control is OFF
void TEA5767::setHighCutControl(byte hcc) {
    setOnOff(&writeData[3], TEA5767_MASK_HCC, hcc);
    status.HCC = hcc;
}

// Stereo Noise Cancelling: if SNC = 1 then stereo noise cancelling is ON; if SNC = 0 then stereo noise cancelling is OFF
void TEA5767::setStereoNoiseCancelling(byte snc) {
    setOnOff(&writeData[3], TEA5767_MASK_SNC, snc);
    status.SNC = snc;
}

// Search Indicator: if SI = 1 then pin SWPORT1 is output for the ready flag; if SI = 0 then pin SWPORT1 is software programmable port 1
void TEA5767::setSearchIndicator(byte mode) {
    setOnOff(&writeData[3], TEA5767_MASK_SEARCH_INDICATOR, mode);
}

// if DTC = 1 then the de-emphasis time constant is 75 μs; if DTC = 0 then the de-emphasis time constant is 50 μs
void TEA5767::setDeemphasisTimeConstant(byte dtc) {
    // https://en.wikipedia.org/wiki/FM_broadcasting
    // US / Canada / S.Korea = 75us
    // others = 50 us
    setOnOff(&writeData[4], TEA5767_MASK_DTC, dtc);
    status.DTC = dtc;
}

/**************************
    public:
**************************/
void TEA5767::init() {
    // data byte 1
    setFreq(TEA5767_DEFAULT_FREQ);
    setMute(TEA5767_MUTE_ON);   // on/off
    setSearchMode(TEA5767_OFF);  // on/off
    // data byte 2 - No freq defined yet

    // data byte 3
    setSearchDirection(TEA5767_UP);        // up/down
    setSearchStopLevel(TEA5767_SSL_HIGH);  // NA/LOW/MID/HIGH
    setSideInjectionMode(TEA5767_INJECTION_HIGH);
    setRadioMode(TEA5767_STEREO);                     // Stereo/Mono
    setMuteChannel(TEA5767_LEFT, TEA5767_MUTE_OFF);   // Left/Right, on/off
    setMuteChannel(TEA5767_RIGHT, TEA5767_MUTE_OFF);  // Left/Right, on/off
    setSWP(TEA5767_SWP_PORT_1, TEA5767_OFF);          // 1/2, on/off | normal off

    // data byte 4
    setSWP(TEA5767_SWP_PORT_2, TEA5767_OFF);  // 1/2, on/off | normal off
    setStandby(TEA5767_OFF);                  // on/off
    setBand(TEA5767_US_EU);                   // JP(76MHz ~ 91MHz) / USEU(87.5MHz ~ 108MHz)
    setXTAL();
    setSoftMute(TEA5767_OFF);               // on/off
    setHighCutControl(TEA5767_OFF);         // on/off
    setStereoNoiseCancelling(TEA5767_OFF);  // on/off
    setSearchIndicator(TEA5767_SWP1);       // output/SWP1 program

    // data byte 5
    setDeemphasisTimeConstant(TEA5767_DTC_50US);  // US/CA/KR = 75us, Others = 50us

    I2C_Write();

    // Other settings
    searchPreset = TEA5767_SEARCH_PRESET_NO;
}

// Put the device in standby mode when temp exit
void TEA5767::pause() {
    setMute(TEA5767_ON);
    setStandby(TEA5767_ON);
    setSearchMode(TEA5767_OFF);

    I2C_Write();
}

// Resume from pause
void TEA5767::resume() {
    setMute(TEA5767_OFF);
    setStandby(TEA5767_OFF);

    I2C_Write();
}

void TEA5767::printStatus() {
    SPH("RAW DATA 0 : 0x", status.rawData[0]);
    SPH("RAW DATA 1 : 0x", status.rawData[1]);
    SPH("RAW DATA 2 : 0x", status.rawData[2]);
    SPH("RAW DATA 3 : 0x", status.rawData[3]);
    SPH("RAW DATA 4 : 0x", status.rawData[4]);

    SPL("-------------------", " ");
    SPL("Radio Ready : ", status.radioReady);
    SPL("Band : ", status.band);
    SPL("Min. Freq. : ", status.minFreq);
    SPL("Max. Freq. : ", status.maxFreq);

    SPL("-------------------", " ");
    SPL("Radio Mode : ", status.radioMode);
    SPL("Current Freq. : ", status.currentFreq);
    SPL("ADC Level : ", status.ADCLevel);
    SPL("Injection : ", status.injection);

    SPL("-------------------", " ");
    SPL("Module Mute : ", status.Sound_All);
    SPL("Left Mute : ", status.Sound_Left);
    SPL("Right Mute : ", status.Sound_Right);

    SPL("-------------------", " ");
    SPL("High Cut Control : ", status.HCC);
    SPL("Stereo Noise Cancelling : ", status.SNC);
    SPL("Soft Mute : ", status.SoftMute);
    SPL("De-emphasis Time Constant : ", status.DTC);

    SPL("-------------------", " ");
    SPL("# of preset Freq. : ", presetFreqSize);
}

// Set Station with input freq.
// The freq is in MHz
void TEA5767::setStation(float freq) {
    setMute(TEA5767_MUTE_OFF);
    setSearchMode(TEA5767_OFF);

    // perform HILO injection optimal here
    optimalSideInjection(freq);

    // set freq with optimal result
    setSideInjectionMode(status.injection);
    setFreq(freq);

    I2C_Write();
    read_status();

    SPT("SET - IF : ", status.IFCounter);
    SPT(" Set Freq : ", freq);
    SPT(" - Optimized to : ", status.currentFreq);
    SPT(" - ADC Level : ", status.ADCLevel);
    SPL(" - Side Injection : ", status.injection);
}

void TEA5767::searchStation(byte dir, byte ssl) {
    setMute(TEA5767_MUTE_ON);
    setSearchMode(TEA5767_OFF);

    searchProcessStatus = TEA5767_SEARCH_PENDING;
    status.dir = dir;
    status.ssl = ssl;

    SPL("Search - From : ", searchingFreq);

    searchProcess();
}

void TEA5767::searchProcess() {
    searchProcessStatus = TEA5767_SEARCH_PENDING;

    if (status.dir  == TEA5767_UP && searchingFreq > status.maxFreq) {
        searchingFreq = status.minFreq;
        SPL("Max Freq. reached. Loop Stop and reset to Min Freq.", " ");
        searchProcessStatus = TEA5767_SEARCH_STOP;
    }
    if (status.dir  == TEA5767_DOWN && searchingFreq < status.minFreq) {
        searchingFreq = status.maxFreq;
        SPL("Min Freq. reached. Loop Stop and reset to Max Freq.", " ");
        searchProcessStatus = TEA5767_SEARCH_STOP;
    }

    if ( searchProcessStatus != TEA5767_SEARCH_STOP ) {
        // Searching next freq
        searchingFreq += (status.dir  == TEA5767_UP) ? 0.1 : -0.1;

        // perform HILO injection optimal here
        optimalSideInjection(searchingFreq);

        // set freq with optimal result
        setSideInjectionMode(status.injection);
        setFreq(searchingFreq);

        I2C_Write();
        read_status();
        
        // Good Signal
        if (between(status.IFCounter, 0x33, 0x3A) && status.ADCLevel >= status.ssl) {  // 52 -> 58
            if ( searchPreset ==  TEA5767_SEARCH_PRESET_YES ) {
                addFreqPreset(searchingFreq);
                SPT(" SEARCH - IF : ", status.IFCounter);
                SPT(" Set Freq : ", searchingFreq);
                SPT(" - Optimized to : ", status.currentFreq);
                SPT(" - ADC Level : ", status.ADCLevel);
                SPL(" - Side Injection : ", status.injection);
            }
            SPL("Station Found."," ");

            setStation(searchingFreq);
            searchProcessStatus = TEA5767_SEARCH_COMPLETE;
        }
    }
}

void TEA5767::scanStation(byte ssl) {
    setMute(TEA5767_MUTE_ON);
    setSearchMode(TEA5767_OFF);
    setSearchIndicator(TEA5767_OFF);

    float freq = 87.5;

    // reset freq
    presetFreqSize = 0;
    free(presetFreq);

    SPL("Start Scanning...", " ");
    while (freq < status.maxFreq) {
        // perform HILO injection optimal here
        optimalSideInjection(freq);

        // set freq with optimal result
        setSideInjectionMode(status.injection);
        setFreq(freq);

        I2C_Write();
        read_status();

        // Good Signal
        if (between(status.IFCounter, 0x33, 0x3A) && status.ADCLevel >= ssl) {  // 52 -> 58
            addFreqPreset(freq);
            SPT("SCAN IF : ", status.IFCounter);
            SPT(" Set Freq : ", freq);
            SPT(" - Optimized to : ", status.currentFreq);
            SPT(" - ADC Level : ", status.ADCLevel);
            SPL(" - Side Injection : ", status.injection);
        }

        freq += 0.1;
    }
    SPL("Scan completed.", " ");
    setMute(TEA5767_MUTE_OFF);
    I2C_Write();
}

void TEA5767::nextPreset() {
    if (presetFreqSize > 0) {  // preset present
        curPreset++;
        if (curPreset >= presetFreqSize) {
            curPreset = 0;
        }
        setFreq(presetFreq[curPreset]);
        SPT("Next : Preset[", curPreset);
        SPT("/", presetFreqSize);
        SPL("] : ", String(presetFreq[curPreset]));
        I2C_Write();
    }
}

void TEA5767::prevPreset() {
    if (presetFreqSize > 0) {  // preset present
        curPreset--;
        if (curPreset < 0) {
            curPreset = presetFreqSize - 1;
        }
        setFreq(presetFreq[curPreset]);
        SPT("Prev : Preset[", curPreset);
        SPT("/", presetFreqSize);
        SPL("] : ", String(presetFreq[curPreset]));
        I2C_Write();
    }
}

void TEA5767::printPreset() {
    SPL("----- Preset List -----", " ");
    for (int i = 0; i < presetFreqSize; i++) {
        SPT("[", i);
        SPT(" - ", presetFreq[i]);
        SPT("]", " ");
    }
    SPL("--- total : ", presetFreqSize);
    SPL("Current Preset : ", curPreset);
    SPL("-----", " ");
}

void TEA5767::deleteCurFreqPreset() {
    if (presetFreqSize != 0) {
        // create a tmp array
        float *t = (float *)calloc(presetFreqSize, sizeof(float));

        // copy upper part
        for (int i = 0; i < curPreset; i++) {
            t[i] = presetFreq[i];
        }

        // copy lower part
        for (int i = curPreset; i < presetFreqSize; i++) {
            t[i] = presetFreq[i + 1];
        }

        free(presetFreq);
        presetFreqSize--;
        presetFreq = t;
    }

    if (presetFreqSize != 0) {
        if (curPreset >= presetFreqSize) {
            curPreset = presetFreqSize - 1;
        }
        setStation(presetFreq[curPreset]);
    }
}

/*
    Toggle
    update device with specific flag
*/
void TEA5767::toggleMute(byte channel) {
    switch (channel) {
        case TEA5767_LEFT:
            setMuteChannel(TEA5767_LEFT, (status.Sound_Left) ? TEA5767_MUTE_OFF : TEA5767_MUTE_ON);
            SPL("Mute Left : ", status.Sound_Left);
            break;
        case TEA5767_RIGHT:
            setMuteChannel(TEA5767_RIGHT, (status.Sound_Right) ? TEA5767_MUTE_OFF : TEA5767_MUTE_ON);
            SPL("Mute Right: ", status.Sound_Right);
            break;
        case TEA5767_ALL:
            setMute((status.Sound_All) ? TEA5767_MUTE_OFF : TEA5767_MUTE_ON);
            SPL("Mute All: ", status.Sound_All);
            break;
    }
    I2C_Write();
}

void TEA5767::toggleSoftMute() {
    setSoftMute((status.SoftMute) ? TEA5767_MUTE_OFF : TEA5767_MUTE_ON);
    SPL("SoftMute: ", status.SoftMute);
    I2C_Write();
}

void TEA5767::toggleHighCutControl() {
    setHighCutControl((status.HCC) ? TEA5767_OFF : TEA5767_ON);
    SPL("HCC: ", status.HCC);
    I2C_Write();
}

void TEA5767::toggleStereoNoiseCancelling() {
    setStereoNoiseCancelling((status.SNC) ? TEA5767_OFF : TEA5767_ON);
    SPL("SNC: ", status.SNC);
    I2C_Write();
}

void TEA5767::toggleDeemphasisTimeConstant() {
    setDeemphasisTimeConstant((status.DTC) ? TEA5767_DTC_50US : TEA5767_DTC_75US);
    SPL("DTC: ", status.DTC);
    I2C_Write();
}

void TEA5767::toggleMode() {
    setRadioMode((status.radioMode) ? TEA5767_STEREO : TEA5767_MONO);
    SPL("Mode: ", status.radioMode);
    I2C_Write();
}
