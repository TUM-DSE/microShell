#ifndef COMPACTCLASSIFICATIONECOC_H
#define COMPACTCLASSIFICATIONECOC_H

/* Include Files */
#include "rtwtypes.h"
#include "svm_speech_30_internal_types.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
double c_CompactClassificationECOC_pre(
    const double obj_Prior[5],
    const c_classreg_learning_classif_Com obj_BinaryLearners[10],
    const double obj_CodingMatrix[50], const float Xin[32]);

#ifdef __cplusplus
}
#endif

#endif

