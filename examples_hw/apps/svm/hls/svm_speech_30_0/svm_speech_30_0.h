/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: svm_speech_30_0.h
 *
 * MATLAB Coder version            : 5.5
 * C/C++ source code generated on  : 24-Dec-2022 03:09:46
 */
#ifndef SVM_SPEECH_30_0_H
#define SVM_SPEECH_30_0_H

/* Include Files */
#include "rtwtypes.h"
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <stddef.h>
#include <stdlib.h>

/* Function Declarations */
void svm_speech_30_0(hls::stream<ap_axiu<32,0,0,0>>& input, 
                     hls::stream<ap_axiu<32,0,0,0>>& output);

#endif
/*
 * File trailer for svm_speech_30_0.h
 *
 * [EOF]
 */
