/* Include Files */
#include "main.h"
#include "rt_nonfinite.h"
#include "svm_speech_30_0.hpp"
#include "svm_speech_30_terminate.h"
#include <string.h>
#include <stdio.h>

/* Function Declarations */
static float argInit_real32_T(void);

/* Function Definitions */
/*
 * Arguments    : void
 * Return Type  : float
 */
static float argInit_real32_T(void)
{
  return 0.0F;
}

/*
 * Arguments    : int argc
 *                char **argv
 * Return Type  : int
 */
int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  /* The initialize function is being called automatically from your entry-point
   * function. So, a call to initialize is not included here. */
  /* Invoke the entry-point functions.
You can call entry-point functions multiple times. */
  main_svm_speech_30();
  /* Terminate the application.
You do not need to do this more than one time. */
  svm_speech_30_terminate();
  return 0;
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void main_svm_speech_30(void)
{
  float result;
  /* Initialize function 'svm_speech_30' input arguments. */
  // in1_tmp = argInit_real32_T();


// for one
  float in1 = 33590226;
  float in2 = 22593220;
  float in3 = 24255961;
  float in4 = 38204218;
  float in5 = 11490207;
  float in6 = 6366810;
  float in7 = 5305137;
  float in8 = 9288187;
  float in9 = 8008457;
  float in10 = 7090552;
  float in11 = 3052539;
  float in12 = 3187317;
  float in13 = 3447729;
  float in14 = 3970187;
  float in15 = 1179723;
  float in16 = 723521;
  float in17 = 805680;
  float in18 = 666286;
  float in19 = 663357;
  float in20 = 893365;
  float in21 = 757088;
  float in22 = 668188;
  float in23 = 684622;
  float in24 = 643703;
  float in25 = 745254;
  float in26 = 752370;
  float in27 = 411583;
  float in28 = 457189;
  float in29 = 529965;
  float in30 = 630445;
  float in31 = 662975;
  float in32 = 649705;

  float input[32] = {in1, in2, in3, in4, in5, in6, in7, in8,
                     in9, in10, in11, in12, in13, in14, in15, in16,
                     in17, in18, in19, in20, in21, in22, in23, in24,
                     in25, in26, in27, in28, in29, in30, in31, in32};
// for four
  // float in1 = 36257662;
  // float in2 = 70308074;
  // float in3 = 162763557;
  // float in4 = 109956489;
  // float in5 = 86125933;
  // float in6 = 35535698;
  // float in7 = 5473712;
  // float in8 = 2191429;
  // float in9 = 1655529;
  // float in10 = 1427210;
  // float in11 = 1819290;
  // float in12 = 2594503;
  // float in13 = 2649694;
  // float in14 = 849597;
  // float in15 = 765175;
  // float in16 = 890647;
  // float in17 = 1405332;
  // float in18 = 1132804;
  // float in19 = 5776375;
  // float in20 = 18372118;
  // float in21 = 6724989;
  // float in22 = 7313762;
  // float in23 = 1469345;
  // float in24 = 1744257;
  // float in25 = 1631652;
  // float in26 = 823229;
  // float in27 = 1018674;
  // float in28 = 639273;
  // float in29 = 601445;
  // float in30 = 630083;
  // float in31 = 678854;
  // float in32 = 636897;

  float output;
  /* Call the entry-point 'svm_speech_30'. */
  result = svm_speech_30_0(input, &output);

  printf("result %f\n", result);
}

