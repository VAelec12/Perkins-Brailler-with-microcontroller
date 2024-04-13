#include "count_steps.h"
#include "stdint.h"
#include "stdio.h"  //using this for printing debug outputs

#define DERIV_FILT_LEN          5           //length of derivative filter
#define LPF_FILT_LEN            9           //length of FIR low pass filter
#define THRESHOLD               50          // Threshold for peak detection

static int8_t deriv_coeffs[DERIV_FILT_LEN]        = {-6,31,0,-31,6};            //coefficients of derivative filter from https://www.dsprelated.com/showarticle/814.php
static int8_t lpf_coeffs[LPF_FILT_LEN]            = {-5,6,34,68,84,68,34,6,-5}; //coefficients of FIR low pass filter generated in matlab using FDATOOL
static uint8_t mag_sqrt[NUM_TUPLES]               = {0};                        //this holds the square root of magnitude data of X,Y,Z (so its length is NUM_SAMPLES/3)
static int32_t lpf[NUM_TUPLES]                    = {0};                        //hold the low pass filtered signal
static int32_t deriv[NUM_TUPLES]                  = {0};                        //holds derivative

//calculate deriviative via FIR filter
static void derivative(int32_t *lpf, int32_t *deriv) {
    uint8_t n;
    uint8_t i;
    int32_t temp_deriv;
    for (n = 0; n < NUM_TUPLES; n++) {
        temp_deriv = 0;
        for (i = 0; i < DERIV_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_deriv += deriv_coeffs[i] * lpf[n-i];
            }
        }
        deriv[n] = temp_deriv;
    }
}

//apply FIR low pass filter
static void lowpassfilt(uint8_t *mag_sqrt, int32_t *lpf) {
    uint16_t n;
    uint8_t i;
    int32_t temp_lpf;
    for (n = 0; n < NUM_TUPLES; n++) {
        temp_lpf = 0;
        for (i = 0; i < LPF_FILT_LEN; i++) {
            if (n-i >= 0) {     //handle the case when n < filter length
                temp_lpf += (int32_t)lpf_coeffs[i] * (int32_t)mag_sqrt[n-i];
            }
        }
        lpf[n] = temp_lpf;
    }
}

//detect peaks in the signal
static uint8_t detect_peaks(int32_t *data) {
    uint8_t peak_count = 0;
    uint16_t i;
    for (i = 1; i < NUM_TUPLES - 1; i++) {
        if (data[i] > data[i - 1] && data[i] > data[i + 1] && data[i] > THRESHOLD) {
            peak_count++;
        }
    }
    return peak_count;
}

//algorithm interface
uint8_t count_steps(int8_t *data) {
    //calculate the magnitude of each triplet
    uint16_t i;
    uint16_t temp_mag;
    for (i = 0; i < NUM_TUPLES; i++) {
        temp_mag = (uint16_t)((uint16_t)data[i*3+0]*(uint16_t)data[i*3+0] + (uint16_t)data[i*3+1]*(uint16_t)data[i*3+1] + (uint16_t)data[i*3+2]*(uint16_t)data[i*3+2]);
        mag_sqrt[i] = (uint8_t)SquareRoot(temp_mag);
    }
    
    //apply low pass filter to magnitude data
    lowpassfilt(mag_sqrt, lpf);
    
    //calculate derivative of the filtered signal
    derivative(lpf, deriv);
    
    //detect peaks in the derivative signal
    uint8_t step_count = detect_peaks(deriv);
    
    return step_count;
}
