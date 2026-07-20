/* Include Files */
#include "CompactClassificationECOC.h"
#include "rt_nonfinite.h"
#include "svm_speech_30_internal_types.h"
#include "rt_nonfinite.h"
#include <string.h>

/* Function Definitions */
/*
 * Arguments    : const double obj_Prior[5]
 *                const c_classreg_learning_classif_Com obj_BinaryLearners[10]
 *                const double obj_CodingMatrix[50]
 *                const float Xin[32]
 * Return Type  : double
 */
double c_CompactClassificationECOC_pre(
    const double obj_Prior[5],
    const c_classreg_learning_classif_Com obj_BinaryLearners[10],
    const double obj_CodingMatrix[50], const float Xin[32])
{
  double M_idx_0;
  double M_idx_1;
  double M_idx_10;
  double M_idx_11;
  double M_idx_12;
  double M_idx_13;
  double M_idx_14;
  double M_idx_15;
  double M_idx_16;
  double M_idx_17;
  double M_idx_18;
  double M_idx_19;
  double M_idx_2;
  double M_idx_20;
  double M_idx_21;
  double M_idx_22;
  double M_idx_23;
  double M_idx_24;
  double M_idx_25;
  double M_idx_26;
  double M_idx_27;
  double M_idx_28;
  double M_idx_29;
  double M_idx_3;
  double M_idx_30;
  double M_idx_31;
  double M_idx_32;
  double M_idx_33;
  double M_idx_34;
  double M_idx_35;
  double M_idx_36;
  double M_idx_37;
  double M_idx_38;
  double M_idx_39;
  double M_idx_4;
  double M_idx_40;
  double M_idx_41;
  double M_idx_42;
  double M_idx_43;
  double M_idx_44;
  double M_idx_45;
  double M_idx_46;
  double M_idx_47;
  double M_idx_48;
  double M_idx_49;
  double M_idx_5;
  double M_idx_6;
  double M_idx_7;
  double M_idx_8;
  double M_idx_9;
  double labels;
  float pscore[10];
  float negloss[5];
  float f;
  float maxval_idx_0;
  int p;
  bool b[5];
  bool exitg1;
  bool valueisnan;
  M_idx_0 = obj_CodingMatrix[0];
  if (obj_CodingMatrix[0] == 0.0) {
    M_idx_0 = rtNaN;
  }
  M_idx_1 = obj_CodingMatrix[1];
  if (obj_CodingMatrix[1] == 0.0) {
    M_idx_1 = rtNaN;
  }
  M_idx_2 = obj_CodingMatrix[2];
  if (obj_CodingMatrix[2] == 0.0) {
    M_idx_2 = rtNaN;
  }
  M_idx_3 = obj_CodingMatrix[3];
  if (obj_CodingMatrix[3] == 0.0) {
    M_idx_3 = rtNaN;
  }
  M_idx_4 = obj_CodingMatrix[4];
  if (obj_CodingMatrix[4] == 0.0) {
    M_idx_4 = rtNaN;
  }
  M_idx_5 = obj_CodingMatrix[5];
  if (obj_CodingMatrix[5] == 0.0) {
    M_idx_5 = rtNaN;
  }
  M_idx_6 = obj_CodingMatrix[6];
  if (obj_CodingMatrix[6] == 0.0) {
    M_idx_6 = rtNaN;
  }
  M_idx_7 = obj_CodingMatrix[7];
  if (obj_CodingMatrix[7] == 0.0) {
    M_idx_7 = rtNaN;
  }
  M_idx_8 = obj_CodingMatrix[8];
  if (obj_CodingMatrix[8] == 0.0) {
    M_idx_8 = rtNaN;
  }
  M_idx_9 = obj_CodingMatrix[9];
  if (obj_CodingMatrix[9] == 0.0) {
    M_idx_9 = rtNaN;
  }
  M_idx_10 = obj_CodingMatrix[10];
  if (obj_CodingMatrix[10] == 0.0) {
    M_idx_10 = rtNaN;
  }
  M_idx_11 = obj_CodingMatrix[11];
  if (obj_CodingMatrix[11] == 0.0) {
    M_idx_11 = rtNaN;
  }
  M_idx_12 = obj_CodingMatrix[12];
  if (obj_CodingMatrix[12] == 0.0) {
    M_idx_12 = rtNaN;
  }
  M_idx_13 = obj_CodingMatrix[13];
  if (obj_CodingMatrix[13] == 0.0) {
    M_idx_13 = rtNaN;
  }
  M_idx_14 = obj_CodingMatrix[14];
  if (obj_CodingMatrix[14] == 0.0) {
    M_idx_14 = rtNaN;
  }
  M_idx_15 = obj_CodingMatrix[15];
  if (obj_CodingMatrix[15] == 0.0) {
    M_idx_15 = rtNaN;
  }
  M_idx_16 = obj_CodingMatrix[16];
  if (obj_CodingMatrix[16] == 0.0) {
    M_idx_16 = rtNaN;
  }
  M_idx_17 = obj_CodingMatrix[17];
  if (obj_CodingMatrix[17] == 0.0) {
    M_idx_17 = rtNaN;
  }
  M_idx_18 = obj_CodingMatrix[18];
  if (obj_CodingMatrix[18] == 0.0) {
    M_idx_18 = rtNaN;
  }
  M_idx_19 = obj_CodingMatrix[19];
  if (obj_CodingMatrix[19] == 0.0) {
    M_idx_19 = rtNaN;
  }
  M_idx_20 = obj_CodingMatrix[20];
  if (obj_CodingMatrix[20] == 0.0) {
    M_idx_20 = rtNaN;
  }
  M_idx_21 = obj_CodingMatrix[21];
  if (obj_CodingMatrix[21] == 0.0) {
    M_idx_21 = rtNaN;
  }
  M_idx_22 = obj_CodingMatrix[22];
  if (obj_CodingMatrix[22] == 0.0) {
    M_idx_22 = rtNaN;
  }
  M_idx_23 = obj_CodingMatrix[23];
  if (obj_CodingMatrix[23] == 0.0) {
    M_idx_23 = rtNaN;
  }
  M_idx_24 = obj_CodingMatrix[24];
  if (obj_CodingMatrix[24] == 0.0) {
    M_idx_24 = rtNaN;
  }
  M_idx_25 = obj_CodingMatrix[25];
  if (obj_CodingMatrix[25] == 0.0) {
    M_idx_25 = rtNaN;
  }
  M_idx_26 = obj_CodingMatrix[26];
  if (obj_CodingMatrix[26] == 0.0) {
    M_idx_26 = rtNaN;
  }
  M_idx_27 = obj_CodingMatrix[27];
  if (obj_CodingMatrix[27] == 0.0) {
    M_idx_27 = rtNaN;
  }
  M_idx_28 = obj_CodingMatrix[28];
  if (obj_CodingMatrix[28] == 0.0) {
    M_idx_28 = rtNaN;
  }
  M_idx_29 = obj_CodingMatrix[29];
  if (obj_CodingMatrix[29] == 0.0) {
    M_idx_29 = rtNaN;
  }
  M_idx_30 = obj_CodingMatrix[30];
  if (obj_CodingMatrix[30] == 0.0) {
    M_idx_30 = rtNaN;
  }
  M_idx_31 = obj_CodingMatrix[31];
  if (obj_CodingMatrix[31] == 0.0) {
    M_idx_31 = rtNaN;
  }
  M_idx_32 = obj_CodingMatrix[32];
  if (obj_CodingMatrix[32] == 0.0) {
    M_idx_32 = rtNaN;
  }
  M_idx_33 = obj_CodingMatrix[33];
  if (obj_CodingMatrix[33] == 0.0) {
    M_idx_33 = rtNaN;
  }
  M_idx_34 = obj_CodingMatrix[34];
  if (obj_CodingMatrix[34] == 0.0) {
    M_idx_34 = rtNaN;
  }
  M_idx_35 = obj_CodingMatrix[35];
  if (obj_CodingMatrix[35] == 0.0) {
    M_idx_35 = rtNaN;
  }
  M_idx_36 = obj_CodingMatrix[36];
  if (obj_CodingMatrix[36] == 0.0) {
    M_idx_36 = rtNaN;
  }
  M_idx_37 = obj_CodingMatrix[37];
  if (obj_CodingMatrix[37] == 0.0) {
    M_idx_37 = rtNaN;
  }
  M_idx_38 = obj_CodingMatrix[38];
  if (obj_CodingMatrix[38] == 0.0) {
    M_idx_38 = rtNaN;
  }
  M_idx_39 = obj_CodingMatrix[39];
  if (obj_CodingMatrix[39] == 0.0) {
    M_idx_39 = rtNaN;
  }
  M_idx_40 = obj_CodingMatrix[40];
  if (obj_CodingMatrix[40] == 0.0) {
    M_idx_40 = rtNaN;
  }
  M_idx_41 = obj_CodingMatrix[41];
  if (obj_CodingMatrix[41] == 0.0) {
    M_idx_41 = rtNaN;
  }
  M_idx_42 = obj_CodingMatrix[42];
  if (obj_CodingMatrix[42] == 0.0) {
    M_idx_42 = rtNaN;
  }
  M_idx_43 = obj_CodingMatrix[43];
  if (obj_CodingMatrix[43] == 0.0) {
    M_idx_43 = rtNaN;
  }
  M_idx_44 = obj_CodingMatrix[44];
  if (obj_CodingMatrix[44] == 0.0) {
    M_idx_44 = rtNaN;
  }
  M_idx_45 = obj_CodingMatrix[45];
  if (obj_CodingMatrix[45] == 0.0) {
    M_idx_45 = rtNaN;
  }
  M_idx_46 = obj_CodingMatrix[46];
  if (obj_CodingMatrix[46] == 0.0) {
    M_idx_46 = rtNaN;
  }
  M_idx_47 = obj_CodingMatrix[47];
  if (obj_CodingMatrix[47] == 0.0) {
    M_idx_47 = rtNaN;
  }
  M_idx_48 = obj_CodingMatrix[48];
  if (obj_CodingMatrix[48] == 0.0) {
    M_idx_48 = rtNaN;
  }
  M_idx_49 = obj_CodingMatrix[49];
  if (obj_CodingMatrix[49] == 0.0) {
    M_idx_49 = rtNaN;
  }
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[0].Scale *
                   (float)obj_BinaryLearners[0].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[0].Scale *
                   (float)obj_BinaryLearners[0].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[0].Scale *
                  (float)obj_BinaryLearners[0].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[0].Scale *
                 (float)obj_BinaryLearners[0].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[0].Scale *
                (float)obj_BinaryLearners[0].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[0].Scale *
               (float)obj_BinaryLearners[0].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[0].Scale *
              (float)obj_BinaryLearners[0].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[0].Scale *
             (float)obj_BinaryLearners[0].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[0].Scale *
            (float)obj_BinaryLearners[0].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[0].Scale *
           (float)obj_BinaryLearners[0].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[0].Scale *
          (float)obj_BinaryLearners[0].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[0].Scale *
       (float)obj_BinaryLearners[0].Beta[31];
  pscore[0] = f + (float)obj_BinaryLearners[0].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[1].Scale *
                   (float)obj_BinaryLearners[1].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[1].Scale *
                   (float)obj_BinaryLearners[1].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[1].Scale *
                  (float)obj_BinaryLearners[1].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[1].Scale *
                 (float)obj_BinaryLearners[1].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[1].Scale *
                (float)obj_BinaryLearners[1].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[1].Scale *
               (float)obj_BinaryLearners[1].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[1].Scale *
              (float)obj_BinaryLearners[1].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[1].Scale *
             (float)obj_BinaryLearners[1].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[1].Scale *
            (float)obj_BinaryLearners[1].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[1].Scale *
           (float)obj_BinaryLearners[1].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[1].Scale *
          (float)obj_BinaryLearners[1].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[1].Scale *
       (float)obj_BinaryLearners[1].Beta[31];
  pscore[1] = f + (float)obj_BinaryLearners[1].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[2].Scale *
                   (float)obj_BinaryLearners[2].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[2].Scale *
                   (float)obj_BinaryLearners[2].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[2].Scale *
                  (float)obj_BinaryLearners[2].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[2].Scale *
                 (float)obj_BinaryLearners[2].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[2].Scale *
                (float)obj_BinaryLearners[2].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[2].Scale *
               (float)obj_BinaryLearners[2].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[2].Scale *
              (float)obj_BinaryLearners[2].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[2].Scale *
             (float)obj_BinaryLearners[2].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[2].Scale *
            (float)obj_BinaryLearners[2].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[2].Scale *
           (float)obj_BinaryLearners[2].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[2].Scale *
          (float)obj_BinaryLearners[2].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[2].Scale *
       (float)obj_BinaryLearners[2].Beta[31];
  pscore[2] = f + (float)obj_BinaryLearners[2].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[3].Scale *
                   (float)obj_BinaryLearners[3].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[3].Scale *
                   (float)obj_BinaryLearners[3].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[3].Scale *
                  (float)obj_BinaryLearners[3].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[3].Scale *
                 (float)obj_BinaryLearners[3].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[3].Scale *
                (float)obj_BinaryLearners[3].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[3].Scale *
               (float)obj_BinaryLearners[3].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[3].Scale *
              (float)obj_BinaryLearners[3].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[3].Scale *
             (float)obj_BinaryLearners[3].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[3].Scale *
            (float)obj_BinaryLearners[3].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[3].Scale *
           (float)obj_BinaryLearners[3].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[3].Scale *
          (float)obj_BinaryLearners[3].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[3].Scale *
       (float)obj_BinaryLearners[3].Beta[31];
  pscore[3] = f + (float)obj_BinaryLearners[3].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[4].Scale *
                   (float)obj_BinaryLearners[4].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[4].Scale *
                   (float)obj_BinaryLearners[4].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[4].Scale *
                  (float)obj_BinaryLearners[4].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[4].Scale *
                 (float)obj_BinaryLearners[4].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[4].Scale *
                (float)obj_BinaryLearners[4].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[4].Scale *
               (float)obj_BinaryLearners[4].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[4].Scale *
              (float)obj_BinaryLearners[4].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[4].Scale *
             (float)obj_BinaryLearners[4].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[4].Scale *
            (float)obj_BinaryLearners[4].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[4].Scale *
           (float)obj_BinaryLearners[4].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[4].Scale *
          (float)obj_BinaryLearners[4].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[4].Scale *
       (float)obj_BinaryLearners[4].Beta[31];
  pscore[4] = f + (float)obj_BinaryLearners[4].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[5].Scale *
                   (float)obj_BinaryLearners[5].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[5].Scale *
                   (float)obj_BinaryLearners[5].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[5].Scale *
                  (float)obj_BinaryLearners[5].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[5].Scale *
                 (float)obj_BinaryLearners[5].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[5].Scale *
                (float)obj_BinaryLearners[5].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[5].Scale *
               (float)obj_BinaryLearners[5].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[5].Scale *
              (float)obj_BinaryLearners[5].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[5].Scale *
             (float)obj_BinaryLearners[5].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[5].Scale *
            (float)obj_BinaryLearners[5].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[5].Scale *
           (float)obj_BinaryLearners[5].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[5].Scale *
          (float)obj_BinaryLearners[5].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[5].Scale *
       (float)obj_BinaryLearners[5].Beta[31];
  pscore[5] = f + (float)obj_BinaryLearners[5].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[6].Scale *
                   (float)obj_BinaryLearners[6].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[6].Scale *
                   (float)obj_BinaryLearners[6].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[6].Scale *
                  (float)obj_BinaryLearners[6].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[6].Scale *
                 (float)obj_BinaryLearners[6].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[6].Scale *
                (float)obj_BinaryLearners[6].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[6].Scale *
               (float)obj_BinaryLearners[6].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[6].Scale *
              (float)obj_BinaryLearners[6].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[6].Scale *
             (float)obj_BinaryLearners[6].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[6].Scale *
            (float)obj_BinaryLearners[6].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[6].Scale *
           (float)obj_BinaryLearners[6].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[6].Scale *
          (float)obj_BinaryLearners[6].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[6].Scale *
       (float)obj_BinaryLearners[6].Beta[31];
  pscore[6] = f + (float)obj_BinaryLearners[6].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[7].Scale *
                   (float)obj_BinaryLearners[7].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[7].Scale *
                   (float)obj_BinaryLearners[7].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[7].Scale *
                  (float)obj_BinaryLearners[7].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[7].Scale *
                 (float)obj_BinaryLearners[7].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[7].Scale *
                (float)obj_BinaryLearners[7].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[7].Scale *
               (float)obj_BinaryLearners[7].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[7].Scale *
              (float)obj_BinaryLearners[7].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[7].Scale *
             (float)obj_BinaryLearners[7].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[7].Scale *
            (float)obj_BinaryLearners[7].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[7].Scale *
           (float)obj_BinaryLearners[7].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[7].Scale *
          (float)obj_BinaryLearners[7].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[7].Scale *
       (float)obj_BinaryLearners[7].Beta[31];
  pscore[7] = f + (float)obj_BinaryLearners[7].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[8].Scale *
                   (float)obj_BinaryLearners[8].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[8].Scale *
                   (float)obj_BinaryLearners[8].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[8].Scale *
                  (float)obj_BinaryLearners[8].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[8].Scale *
                 (float)obj_BinaryLearners[8].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[8].Scale *
                (float)obj_BinaryLearners[8].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[8].Scale *
               (float)obj_BinaryLearners[8].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[8].Scale *
              (float)obj_BinaryLearners[8].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[8].Scale *
             (float)obj_BinaryLearners[8].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[8].Scale *
            (float)obj_BinaryLearners[8].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[8].Scale *
           (float)obj_BinaryLearners[8].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[8].Scale *
          (float)obj_BinaryLearners[8].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[8].Scale *
       (float)obj_BinaryLearners[8].Beta[31];
  pscore[8] = f + (float)obj_BinaryLearners[8].Bias;
  f = (((((((((Xin[0] / (float)obj_BinaryLearners[9].Scale *
                   (float)obj_BinaryLearners[9].Beta[0] +
               Xin[1] / (float)obj_BinaryLearners[9].Scale *
                   (float)obj_BinaryLearners[9].Beta[1]) +
              Xin[2] / (float)obj_BinaryLearners[9].Scale *
                  (float)obj_BinaryLearners[9].Beta[2]) +
             Xin[3] / (float)obj_BinaryLearners[9].Scale *
                 (float)obj_BinaryLearners[9].Beta[3]) +
            Xin[4] / (float)obj_BinaryLearners[9].Scale *
                (float)obj_BinaryLearners[9].Beta[4]) +
           Xin[5] / (float)obj_BinaryLearners[9].Scale *
               (float)obj_BinaryLearners[9].Beta[5]) +
          Xin[6] / (float)obj_BinaryLearners[9].Scale *
              (float)obj_BinaryLearners[9].Beta[6]) +
         Xin[7] / (float)obj_BinaryLearners[9].Scale *
             (float)obj_BinaryLearners[9].Beta[7]) +
        Xin[8] / (float)obj_BinaryLearners[9].Scale *
            (float)obj_BinaryLearners[9].Beta[8]) +
       Xin[9] / (float)obj_BinaryLearners[9].Scale *
           (float)obj_BinaryLearners[9].Beta[9]) +
      Xin[10] / (float)obj_BinaryLearners[9].Scale *
          (float)obj_BinaryLearners[9].Beta[10];
  f += Xin[11] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[11];
  f += Xin[12] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[12];
  f += Xin[13] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[13];
  f += Xin[14] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[14];
  f += Xin[15] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[15];
  f += Xin[16] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[16];
  f += Xin[17] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[17];
  f += Xin[18] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[18];
  f += Xin[19] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[19];
  f += Xin[20] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[20];
  f += Xin[21] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[21];
  f += Xin[22] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[22];
  f += Xin[23] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[23];
  f += Xin[24] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[24];
  f += Xin[25] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[25];
  f += Xin[26] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[26];
  f += Xin[27] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[27];
  f += Xin[28] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[28];
  f += Xin[29] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[29];
  f += Xin[30] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[30];
  f += Xin[31] / (float)obj_BinaryLearners[9].Scale *
       (float)obj_BinaryLearners[9].Beta[31];
  f += (float)obj_BinaryLearners[9].Bias;
  pscore[9] = f;
  valueisnan = true;
//   p = 0;
//   exitg1 = false;
//   while ((!exitg1) && (p < 10)) {
//     if (!rtIsNaNF(pscore[p])) {
//       valueisnan = false;
//       exitg1 = true;
//     } else {
//       p++;
//     }
//   }

if (!rtIsNaNF(pscore[0])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[1])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[2])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[3])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[4])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[5])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[6])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[7])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[8])) {
  valueisnan = false;
} else if (!rtIsNaNF(pscore[9])) {
  valueisnan = false;
}

  negloss[0] = rtNaNF;
  negloss[1] = rtNaNF;
  negloss[2] = rtNaNF;
  negloss[3] = rtNaNF;
  negloss[4] = rtNaNF;
  if (!valueisnan) {
    float maxval_idx_1;
    float maxval_idx_2;
    float maxval_idx_3;
    float maxval_idx_4;
    float maxval_idx_5;
    float maxval_idx_6;
    float maxval_idx_7;
    float maxval_idx_8;
    float maxval_idx_9;
    maxval_idx_9 = 1.0F - (float)M_idx_0 * pscore[0];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_0 = maxval_idx_9;
    } else {
      maxval_idx_0 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_5 * pscore[1];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_1 = maxval_idx_9;
    } else {
      maxval_idx_1 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_10 * pscore[2];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_2 = maxval_idx_9;
    } else {
      maxval_idx_2 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_15 * pscore[3];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_3 = maxval_idx_9;
    } else {
      maxval_idx_3 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_20 * pscore[4];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_4 = maxval_idx_9;
    } else {
      maxval_idx_4 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_25 * pscore[5];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_5 = maxval_idx_9;
    } else {
      maxval_idx_5 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_30 * pscore[6];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_6 = maxval_idx_9;
    } else {
      maxval_idx_6 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_35 * pscore[7];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_7 = maxval_idx_9;
    } else {
      maxval_idx_7 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_40 * pscore[8];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_8 = maxval_idx_9;
    } else {
      maxval_idx_8 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_45 * f;
    if ((!rtIsNaNF(maxval_idx_9)) && (!(maxval_idx_9 > 0.0F))) {
      maxval_idx_9 = 0.0F;
    }
    if (rtIsNaNF(maxval_idx_0)) {
      maxval_idx_0 = 0.0F;
      p = 0;
    } else {
      p = 1;
    }
    if (!rtIsNaNF(maxval_idx_1)) {
      maxval_idx_0 += maxval_idx_1;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_2)) {
      maxval_idx_0 += maxval_idx_2;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_3)) {
      maxval_idx_0 += maxval_idx_3;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_4)) {
      maxval_idx_0 += maxval_idx_4;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_5)) {
      maxval_idx_0 += maxval_idx_5;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_6)) {
      maxval_idx_0 += maxval_idx_6;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_7)) {
      maxval_idx_0 += maxval_idx_7;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_8)) {
      maxval_idx_0 += maxval_idx_8;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_9)) {
      maxval_idx_0 += maxval_idx_9;
      p++;
    }
    negloss[0] = maxval_idx_0 / (float)p / 2.0F;
    maxval_idx_9 = 1.0F - (float)M_idx_1 * pscore[0];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_0 = maxval_idx_9;
    } else {
      maxval_idx_0 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_6 * pscore[1];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_1 = maxval_idx_9;
    } else {
      maxval_idx_1 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_11 * pscore[2];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_2 = maxval_idx_9;
    } else {
      maxval_idx_2 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_16 * pscore[3];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_3 = maxval_idx_9;
    } else {
      maxval_idx_3 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_21 * pscore[4];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_4 = maxval_idx_9;
    } else {
      maxval_idx_4 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_26 * pscore[5];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_5 = maxval_idx_9;
    } else {
      maxval_idx_5 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_31 * pscore[6];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_6 = maxval_idx_9;
    } else {
      maxval_idx_6 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_36 * pscore[7];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_7 = maxval_idx_9;
    } else {
      maxval_idx_7 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_41 * pscore[8];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_8 = maxval_idx_9;
    } else {
      maxval_idx_8 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_46 * f;
    if ((!rtIsNaNF(maxval_idx_9)) && (!(maxval_idx_9 > 0.0F))) {
      maxval_idx_9 = 0.0F;
    }
    if (rtIsNaNF(maxval_idx_0)) {
      maxval_idx_0 = 0.0F;
      p = 0;
    } else {
      p = 1;
    }
    if (!rtIsNaNF(maxval_idx_1)) {
      maxval_idx_0 += maxval_idx_1;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_2)) {
      maxval_idx_0 += maxval_idx_2;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_3)) {
      maxval_idx_0 += maxval_idx_3;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_4)) {
      maxval_idx_0 += maxval_idx_4;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_5)) {
      maxval_idx_0 += maxval_idx_5;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_6)) {
      maxval_idx_0 += maxval_idx_6;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_7)) {
      maxval_idx_0 += maxval_idx_7;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_8)) {
      maxval_idx_0 += maxval_idx_8;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_9)) {
      maxval_idx_0 += maxval_idx_9;
      p++;
    }
    negloss[1] = maxval_idx_0 / (float)p / 2.0F;
    maxval_idx_9 = 1.0F - (float)M_idx_2 * pscore[0];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_0 = maxval_idx_9;
    } else {
      maxval_idx_0 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_7 * pscore[1];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_1 = maxval_idx_9;
    } else {
      maxval_idx_1 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_12 * pscore[2];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_2 = maxval_idx_9;
    } else {
      maxval_idx_2 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_17 * pscore[3];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_3 = maxval_idx_9;
    } else {
      maxval_idx_3 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_22 * pscore[4];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_4 = maxval_idx_9;
    } else {
      maxval_idx_4 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_27 * pscore[5];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_5 = maxval_idx_9;
    } else {
      maxval_idx_5 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_32 * pscore[6];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_6 = maxval_idx_9;
    } else {
      maxval_idx_6 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_37 * pscore[7];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_7 = maxval_idx_9;
    } else {
      maxval_idx_7 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_42 * pscore[8];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_8 = maxval_idx_9;
    } else {
      maxval_idx_8 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_47 * f;
    if ((!rtIsNaNF(maxval_idx_9)) && (!(maxval_idx_9 > 0.0F))) {
      maxval_idx_9 = 0.0F;
    }
    if (rtIsNaNF(maxval_idx_0)) {
      maxval_idx_0 = 0.0F;
      p = 0;
    } else {
      p = 1;
    }
    if (!rtIsNaNF(maxval_idx_1)) {
      maxval_idx_0 += maxval_idx_1;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_2)) {
      maxval_idx_0 += maxval_idx_2;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_3)) {
      maxval_idx_0 += maxval_idx_3;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_4)) {
      maxval_idx_0 += maxval_idx_4;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_5)) {
      maxval_idx_0 += maxval_idx_5;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_6)) {
      maxval_idx_0 += maxval_idx_6;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_7)) {
      maxval_idx_0 += maxval_idx_7;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_8)) {
      maxval_idx_0 += maxval_idx_8;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_9)) {
      maxval_idx_0 += maxval_idx_9;
      p++;
    }
    negloss[2] = maxval_idx_0 / (float)p / 2.0F;
    maxval_idx_9 = 1.0F - (float)M_idx_3 * pscore[0];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_0 = maxval_idx_9;
    } else {
      maxval_idx_0 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_8 * pscore[1];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_1 = maxval_idx_9;
    } else {
      maxval_idx_1 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_13 * pscore[2];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_2 = maxval_idx_9;
    } else {
      maxval_idx_2 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_18 * pscore[3];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_3 = maxval_idx_9;
    } else {
      maxval_idx_3 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_23 * pscore[4];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_4 = maxval_idx_9;
    } else {
      maxval_idx_4 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_28 * pscore[5];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_5 = maxval_idx_9;
    } else {
      maxval_idx_5 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_33 * pscore[6];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_6 = maxval_idx_9;
    } else {
      maxval_idx_6 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_38 * pscore[7];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_7 = maxval_idx_9;
    } else {
      maxval_idx_7 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_43 * pscore[8];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_8 = maxval_idx_9;
    } else {
      maxval_idx_8 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_48 * f;
    if ((!rtIsNaNF(maxval_idx_9)) && (!(maxval_idx_9 > 0.0F))) {
      maxval_idx_9 = 0.0F;
    }
    if (rtIsNaNF(maxval_idx_0)) {
      maxval_idx_0 = 0.0F;
      p = 0;
    } else {
      p = 1;
    }
    if (!rtIsNaNF(maxval_idx_1)) {
      maxval_idx_0 += maxval_idx_1;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_2)) {
      maxval_idx_0 += maxval_idx_2;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_3)) {
      maxval_idx_0 += maxval_idx_3;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_4)) {
      maxval_idx_0 += maxval_idx_4;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_5)) {
      maxval_idx_0 += maxval_idx_5;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_6)) {
      maxval_idx_0 += maxval_idx_6;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_7)) {
      maxval_idx_0 += maxval_idx_7;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_8)) {
      maxval_idx_0 += maxval_idx_8;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_9)) {
      maxval_idx_0 += maxval_idx_9;
      p++;
    }
    negloss[3] = maxval_idx_0 / (float)p / 2.0F;
    maxval_idx_9 = 1.0F - (float)M_idx_4 * pscore[0];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_0 = maxval_idx_9;
    } else {
      maxval_idx_0 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_9 * pscore[1];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_1 = maxval_idx_9;
    } else {
      maxval_idx_1 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_14 * pscore[2];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_2 = maxval_idx_9;
    } else {
      maxval_idx_2 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_19 * pscore[3];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_3 = maxval_idx_9;
    } else {
      maxval_idx_3 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_24 * pscore[4];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_4 = maxval_idx_9;
    } else {
      maxval_idx_4 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_29 * pscore[5];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_5 = maxval_idx_9;
    } else {
      maxval_idx_5 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_34 * pscore[6];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_6 = maxval_idx_9;
    } else {
      maxval_idx_6 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_39 * pscore[7];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_7 = maxval_idx_9;
    } else {
      maxval_idx_7 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_44 * pscore[8];
    if (rtIsNaNF(maxval_idx_9) || (maxval_idx_9 > 0.0F)) {
      maxval_idx_8 = maxval_idx_9;
    } else {
      maxval_idx_8 = 0.0F;
    }
    maxval_idx_9 = 1.0F - (float)M_idx_49 * f;
    if ((!rtIsNaNF(maxval_idx_9)) && (!(maxval_idx_9 > 0.0F))) {
      maxval_idx_9 = 0.0F;
    }
    if (rtIsNaNF(maxval_idx_0)) {
      maxval_idx_0 = 0.0F;
      p = 0;
    } else {
      p = 1;
    }
    if (!rtIsNaNF(maxval_idx_1)) {
      maxval_idx_0 += maxval_idx_1;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_2)) {
      maxval_idx_0 += maxval_idx_2;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_3)) {
      maxval_idx_0 += maxval_idx_3;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_4)) {
      maxval_idx_0 += maxval_idx_4;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_5)) {
      maxval_idx_0 += maxval_idx_5;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_6)) {
      maxval_idx_0 += maxval_idx_6;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_7)) {
      maxval_idx_0 += maxval_idx_7;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_8)) {
      maxval_idx_0 += maxval_idx_8;
      p++;
    }
    if (!rtIsNaNF(maxval_idx_9)) {
      maxval_idx_0 += maxval_idx_9;
      p++;
    }
    negloss[4] = maxval_idx_0 / (float)p / 2.0F;
  }
  f = -negloss[0];
  negloss[0] = -negloss[0];
  b[0] = rtIsNaNF(f);
  f = -negloss[1];
  negloss[1] = -negloss[1];
  b[1] = rtIsNaNF(f);
  f = -negloss[2];
  negloss[2] = -negloss[2];
  b[2] = rtIsNaNF(f);
  f = -negloss[3];
  negloss[3] = -negloss[3];
  b[3] = rtIsNaNF(f);
  f = -negloss[4];
  negloss[4] = -negloss[4];
  b[4] = rtIsNaNF(f);
  valueisnan = true;
//   p = 0;
//   exitg1 = false;
//   while ((!exitg1) && (p <= 4)) {
//     if (!b[p]) {
//       valueisnan = false;
//       exitg1 = true;
//     } else {
//       p++;
//     }
//   }

if (!b[0]) {
  valueisnan = false;
} else if (!b[1]) {
  valueisnan = false;
} else if (!b[2]) {
  valueisnan = false;
} else if (!b[3]) {
  valueisnan = false;
} else if (!b[4]) {
  valueisnan = false;
}

  M_idx_0 = obj_Prior[0];
  p = -1;
  if (obj_Prior[0] < obj_Prior[1]) {
    M_idx_0 = obj_Prior[1];
    p = 0;
  }
  if (M_idx_0 < obj_Prior[2]) {
    M_idx_0 = obj_Prior[2];
    p = 1;
  }
  if (M_idx_0 < obj_Prior[3]) {
    M_idx_0 = obj_Prior[3];
    p = 2;
  }
  if (M_idx_0 < obj_Prior[4]) {
    p = 3;
  }
  labels = (double)p + 2.0;
  if (!valueisnan) {
    int idx;
    if (!rtIsNaNF(negloss[0])) {
      idx = 1;
    } else {
      idx = 0;
     //  p = 2;
     //  exitg1 = false;
     //  while ((!exitg1) && (p < 6)) {
     //    if (!rtIsNaNF(negloss[p - 1])) {
     //      idx = p;
     //      exitg1 = true;
     //    } else {
     //      p++;
     //    }
     //  }

     if (!rtIsNaNF(negloss[1])) {
          idx = 2;
     } else if (!rtIsNaNF(negloss[2])) {
          idx = 3;
     } else if (!rtIsNaNF(negloss[3])) {
          idx = 4;
     } else if (!rtIsNaNF(negloss[4])) {
          idx = 5;
     }

    }
    if (idx == 0) {
      idx = 1;
    } else {
      int i;
      maxval_idx_0 = negloss[idx - 1];

     //  i = idx + 1;
     //  for (p = i; p < 6; p++) {
     //    f = negloss[p - 1];
     //    if (maxval_idx_0 < f) {
     //      maxval_idx_0 = f;
     //      idx = p;
     //    }
     //  }

     if(idx == 1){
          //i = 2;
          f = negloss[1];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 3;
          }

          f = negloss[2];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 3;
          }

          f = negloss[3];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 4;
          }

          f = negloss[4];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 5;
          }

     } else if (idx == 2){
          //i = 3;
          f = negloss[2];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 3;
          }

          f = negloss[3];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 4;
          }

          f = negloss[4];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 5;
          }

     } else if (idx == 3){
          //i = 4;
          f = negloss[3];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 4;
          }

          f = negloss[4];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 5;
          }
     } else if (idx == 4){
          //i = 5;
          f = negloss[4];
          if (maxval_idx_0 < f) {
          maxval_idx_0 = f;
          idx = 5;
          }
     } else if (idx == 5){
          //i = 6;

     }
    }
    labels = idx;
  }
  return labels;
}