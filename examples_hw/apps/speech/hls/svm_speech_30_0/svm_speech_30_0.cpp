/* Include Files */
#include "svm_speech_30_0.h"
#include "CompactClassificationECOC.h"
#include "rt_nonfinite.h"
#include "svm_speech_30_internal_types.h"
#include <string.h>
#include <stdint.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

/* Function Definitions */
/*
 * Speech recognition for number 1 to 5 with SVM algorithm
 * STATELESS: Reinitialize everything fresh for each packet
 */

void svm_speech_30_0(hls::stream<ap_axiu<32,0,0,0>>& input, hls::stream<ap_axiu<32,0,0,0>>& output) 
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register port=input 
#pragma HLS INTERFACE axis register port=output 

while(1){
    ap_axiu<32,0,0,0> val_in;
    ap_axiu<32,0,0,0> val_out;

    // COMPLETELY FRESH MODEL DATA - NO STATIC VARIABLES
    // Everything is reinitialized from scratch each time the function is called

    const double Mdl_CodingMatrix[50] = {
        1.0, -1.0, -0.0, -0.0, -0.0, 1.0, -0.0, -1.0, -0.0, -0.0,
        1.0, -0.0, -0.0, -1.0, -0.0, 1.0, -0.0, -0.0, -0.0, -1.0,
        0.0, 1.0, -1.0, -0.0, -0.0, 0.0, 1.0, -0.0, -1.0, -0.0,
        0.0, 1.0, -0.0, -0.0, -1.0, 0.0, 0.0, 1.0, -1.0, -0.0,
        0.0, 0.0, 1.0, -0.0, -1.0, 0.0, 0.0, 0.0, 1.0, -1.0
    };

    const double dv[32] = {
        -0.058221418667231406,  -0.38297780981424023,   0.15012992780825987,
        1.9203137871228624,     0.901883047596703,      0.58355220774206151,
        0.37928769994323963,    0.30038005451146804,    0.26281352894975391,
        0.12910060257551725,    0.092419174109037619,   0.09057500660592957,
        0.1079202638150721,     0.054621645762140521,   -0.059556406733832674,
        -0.10019633511128262,   -0.13724617229057379,   -0.19375043652478866,
        -0.17073778229338318,   -0.043867099306751596,  -0.0048480406730198279,
        -0.013693799891236133,  -0.015801475195050377,  -0.037138655316294053,
        -0.0032442550913472086, 0.0026150508441651604,  -0.010200072519902565,
        -0.0085513533453546037, -0.0081364097944066766, -0.0047206974775548943,
        -0.010866166141790147,  -0.010033954899246807};

    const double dv1[32] = {
        -0.020714265908450574,  -0.39114156328100674,  0.1018097809652503,
        2.0186872738266621,     0.90720600162725074,   0.75404742895634869,
        0.48043179630032762,    0.38064324863334759,   0.024809489439554078,
        -0.09613399341395322,   -0.064975526839132838, -0.14197142965459517,
        -0.22989308093122976,   -0.26273640402252313,  -0.22412420120918192,
        -0.1459795309279929,    -0.10205436964183294,  -0.080793733784000538,
        -0.079167514906157407,  -0.052863064180590967, -0.04854788602742225,
        -0.059315908853229585,  -0.040357387619202954, -0.044724919309241107,
        -0.0024002363330150492, -0.012494068123655954, -0.021003940589459445,
        -0.017883183289056543,  -0.014849173166353653, -0.014798904073063553,
        -0.023167734975347413,  -0.033168660896961105};

    const double dv2[32] = {
        0.017049081645330585,   -0.32843767112744826,    -1.8409384417209216,
        0.52537400051841388,    -0.026156502299470617,   0.39554770855452481,
        0.26435329117254824,    0.39293347807029544,     0.2575868376208022,
        0.10608242618698061,    0.069929616741111866,    0.097375400410532373,
        0.11429428610447702,    0.055680583471988981,    0.0094973477991930344,
        -0.0025230199685795859, -0.0032158447944647646,  -0.0010217132442212492,
        -0.003533675261817333,  -0.00070934113827541959, 0.0054839882971952185,
        -0.0030164815201952524, 0.0014089859124618654,   -0.016728051922330733,
        0.0031470053555307988,  -0.0052168727555386646,  -0.0025767853731228398,
        -0.0020084006366847093, -0.00208360263084253,    -0.00011484522535109205,
        -0.0046318910364785216, -0.0081549584325972656};

    const double dv3[32] = {
        -0.59100607629109059,   1.4673819714771006,    -1.2197995388003584,
        -0.13249597939916602,   -0.72273974531089735,  0.80425711604451888,
        -0.048575073842100455,  0.3850163288129545,    0.33746779597580656,
        -0.031238370459475667,  -0.51943299711692548,  -0.47575992480161045,
        0.18527359380263353,    0.13199499863762779,   0.015277807343282771,
        -0.010828797697177487,  -0.081354592768346015, -0.12445498382022396,
        -0.24215097877916031,   -0.063054978519960947, -0.0086519819164628253,
        -0.0040316932359903482, -0.019176748973668518, -0.0475528888073713,
        0.031886287668761511,   -0.041670368111202567, -0.013522526796205752,
        0.012208784344855049,   0.019707154979093717,  0.045404129212043953,
        0.021754756498905993,   0.020636022472039241};

    const double dv4[32] = {
        0.20742262178634843,   0.022738192338672455,  -0.020034900496770477,
        -0.13094925423801418,  0.068142783677414581,  0.55396704636865746,
        0.27958294120432192,   0.10674010952331993,   -1.1082296189072178,
        -0.78598879226868523,  -0.43629167514895634,  -0.81947072234520579,
        -1.0761490218454668,   -0.90341174177324446,  -0.41482026843951753,
        -0.09305063783729331,  -0.056009550445317878, 0.23521398194754181,
        0.17218698188017503,   -0.16628858924124018,  -0.29494415085991821,
        -0.24127730048484741,  -0.10851408161844585,  -0.045942038652866418,
        -0.017160075635214406, -0.058767306742011458, -0.018452807453244856,
        0.00956318688087937,   -0.078431421613547839, -0.13117669303264598,
        -0.13077275291945117,  -0.12660200903304689};

    const double dv5[32] = {
        0.02011053708623927,    0.14138987840114867,    -1.8512491888324791,
        -0.60536792345306056,   -0.45197601074094218,   -0.012860030403648777,
        -0.011624422883956088,  0.045206792638497979,   0.0034395856335556504,
        0.0068858448540367618,  0.0047708952295839931,  0.0019138711372597007,
        0.015967896169644547,   0.013156659509885586,   0.032438952395228308,
        0.051097610414985534,   0.052637789082660411,   0.0482174133569966,
        0.028634562941907738,   0.013179056256929131,   -0.0021804454371299672,
        -0.004026728757867754,  0.0016254585891098731,  -0.0016504963726813686,
        -0.0044024185704898576, -0.0064925670633897729, -0.0021674201100243436,
        -0.0030403252798775764, -0.0026008814134354215, -0.0036824095368357887,
        -0.0057735898325369491, -0.0042981023606759178};

    const double dv6[32] = {
        -0.27187017839308819,  0.62507418808333637,    -0.71708451803141338,
        -1.2249585701295918,   -0.96513627752432851,   -0.22504680061618951,
        -0.48321543387399624,  -0.26992302238307686,   -0.19503489307245775,
        -0.16375737875572369,  -0.16716230722625589,   -0.15220161468364374,
        -0.071884970392271083, -0.021848054422413332,  0.050860617621616,
        0.095679085601983252,  0.12098556008848919,    0.098328804798211045,
        0.0693629003743812,    0.0089963702459664556,  0.0016202108087464613,
        0.00805737086537462,   0.012239571217210737,   0.022171126679863949,
        0.0055253595715619606, -0.0047600006445994384, 0.010411999604561066,
        0.0065348486433375961, 0.0087004343783472909,  0.009922245625778656,
        0.01069557168065962,   0.012320692514399085};

    const double dv7[32] = {
        -0.083273256652079949,  -0.064392256011405233, -1.8009023136076314,
        -0.56128260020107623,   -0.42807710132484755,  -0.074632720134214833,
        -0.038849721212160807,  0.042050803110965658,  0.17495556804770895,
        0.09435496916455495,    0.0656147444472204,    0.14794061623910357,
        0.23215336791130659,    0.2236549042733541,    0.11599359436778711,
        0.064385258308725185,   0.0579977961919123,    0.046982640846230439,
        0.0374705046811256,     0.030493562781567098,  0.018363198521636828,
        0.019165527417511548,   0.026987369894190167,  0.0073896576681219241,
        0.00023818209786495554, 0.00665863471204112,   0.0093701283368771912,
        0.0070181660816158089,  0.010016100861445713,  0.013480121670784748,
        0.016263292959328685,   0.017098714576643012};

    const double dv8[32] = {
        -0.25594089775429285,  0.73380108621769413,   -0.52785255080803728,
        -1.2531688602381592,   -0.9423959756083965,   -0.34695774605855456,
        -0.55345583071926641,  -0.27312307705457867,  0.080310491924577593,
        -0.013269190702315218, -0.0634585305840775,   0.053693367811913618,
        0.19283012090122223,   0.26423139915770777,   0.19644513355994056,
        0.12468730971215734,   0.083414571056907652,  0.042108696903481548,
        0.0066215499462829695, 0.033407642766502148,  0.028539881494243257,
        0.028011594892691432,  0.030042944547433489,  0.025864815594461691,
        0.0030890872434883986, 0.0064066684309548178, 0.018330233521876706,
        0.01603853566412675,   0.02079906634795323,   0.024931771093641805,
        0.030586693402129592,  0.035371085095925237};

    const double dv9[32] = {
        -0.089035483715175334, 0.80980330072245954,    1.918895286620822,
        -0.32401741822772562,  -0.14608507399637216,   -0.2543778319673442,
        -0.42833599344111006,  -0.33052928537740189,   -0.21549136968948537,
        -0.14868951764029781,  -0.138336512643562,     -0.18113440851893967,
        -0.074031276892703918, -0.022205471335147157,  -0.004801203567571629,
        0.0014862441600202421, -0.006521351524193488,  -0.021730048514574045,
        -0.038422592035197725, -0.0036149311817675659, -0.0017594417580814169,
        0.0024756474202335317, -0.0037444214273827906, 0.010090994248442562,
        -0.000372974810936499, -0.0028554263695364248, 0.00063144831884086056,
        0.0019950074822094547, 0.0051754871049486889,  0.0050377731261103008,
        0.0093970017327699891, 0.011451038904959104};

    // FRESH MODEL STRUCTURES - REINITIALIZED EACH TIME
    c_classreg_learning_classif_Com Mdl_BinaryLearners[10];
    double Mdl_Prior[5];

    // FULL MODEL INITIALIZATION - HAPPENS EVERY PACKET
    Mdl_BinaryLearners[0].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[0].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[0].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[0].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[0].ScoreTransform = Identity;
    Mdl_BinaryLearners[0].Bias = -1.1057035326330293;
    memcpy(&Mdl_BinaryLearners[0].Beta[0], &dv[0], 32U * sizeof(double));
    Mdl_BinaryLearners[0].Scale = 3.73874692020866E+7;
    Mdl_BinaryLearners[0].Prior[0] = 0.5;
    Mdl_BinaryLearners[0].Prior[1] = 0.5;
    Mdl_BinaryLearners[0].Cost[0] = 0.0;
    Mdl_BinaryLearners[0].Cost[1] = 1.0;
    Mdl_BinaryLearners[0].Cost[2] = 1.0;
    Mdl_BinaryLearners[0].Cost[3] = 0.0;

    Mdl_BinaryLearners[1].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[1].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[1].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[1].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[1].ScoreTransform = Identity;
    Mdl_BinaryLearners[1].Bias = -0.99562525816920522;
    memcpy(&Mdl_BinaryLearners[1].Beta[0], &dv1[0], 32U * sizeof(double));
    Mdl_BinaryLearners[1].Scale = 4.0668510463059261E+7;
    Mdl_BinaryLearners[1].Prior[0] = 0.5;
    Mdl_BinaryLearners[1].Prior[1] = 0.5;
    Mdl_BinaryLearners[1].Cost[0] = 0.0;
    Mdl_BinaryLearners[1].Cost[1] = 1.0;
    Mdl_BinaryLearners[1].Cost[2] = 1.0;
    Mdl_BinaryLearners[1].Cost[3] = 0.0;

    Mdl_BinaryLearners[2].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[2].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[2].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[2].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[2].ScoreTransform = Identity;
    Mdl_BinaryLearners[2].Bias = 1.559367160973236;
    memcpy(&Mdl_BinaryLearners[2].Beta[0], &dv2[0], 32U * sizeof(double));
    Mdl_BinaryLearners[2].Scale = 5.4049379619926259E+7;
    Mdl_BinaryLearners[2].Prior[0] = 0.5;
    Mdl_BinaryLearners[2].Prior[1] = 0.5;
    Mdl_BinaryLearners[2].Cost[0] = 0.0;
    Mdl_BinaryLearners[2].Cost[1] = 1.0;
    Mdl_BinaryLearners[2].Cost[2] = 1.0;
    Mdl_BinaryLearners[2].Cost[3] = 0.0;

    Mdl_BinaryLearners[3].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[3].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[3].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[3].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[3].ScoreTransform = Identity;
    Mdl_BinaryLearners[3].Bias = 2.7764492029850874;
    memcpy(&Mdl_BinaryLearners[3].Beta[0], &dv3[0], 32U * sizeof(double));
    Mdl_BinaryLearners[3].Scale = 1.3091237085801445E+7;
    Mdl_BinaryLearners[3].Prior[0] = 0.5;
    Mdl_BinaryLearners[3].Prior[1] = 0.5;
    Mdl_BinaryLearners[3].Cost[0] = 0.0;
    Mdl_BinaryLearners[3].Cost[1] = 1.0;
    Mdl_BinaryLearners[3].Cost[2] = 1.0;
    Mdl_BinaryLearners[3].Cost[3] = 0.0;

    Mdl_BinaryLearners[4].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[4].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[4].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[4].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[4].ScoreTransform = Identity;
    Mdl_BinaryLearners[4].Bias = 0.95044078137195565;
    memcpy(&Mdl_BinaryLearners[4].Beta[0], &dv4[0], 32U * sizeof(double));
    Mdl_BinaryLearners[4].Scale = 8.0132629392544515E+6;
    Mdl_BinaryLearners[4].Prior[0] = 0.5;
    Mdl_BinaryLearners[4].Prior[1] = 0.5;
    Mdl_BinaryLearners[4].Cost[0] = 0.0;
    Mdl_BinaryLearners[4].Cost[1] = 1.0;
    Mdl_BinaryLearners[4].Cost[2] = 1.0;
    Mdl_BinaryLearners[4].Cost[3] = 0.0;

    Mdl_BinaryLearners[5].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[5].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[5].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[5].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[5].ScoreTransform = Identity;
    Mdl_BinaryLearners[5].Bias = 1.7638486672781322;
    memcpy(&Mdl_BinaryLearners[5].Beta[0], &dv5[0], 32U * sizeof(double));
    Mdl_BinaryLearners[5].Scale = 5.7214808807235435E+7;
    Mdl_BinaryLearners[5].Prior[0] = 0.5;
    Mdl_BinaryLearners[5].Prior[1] = 0.5;
    Mdl_BinaryLearners[5].Cost[0] = 0.0;
    Mdl_BinaryLearners[5].Cost[1] = 1.0;
    Mdl_BinaryLearners[5].Cost[2] = 1.0;
    Mdl_BinaryLearners[5].Cost[3] = 0.0;

    Mdl_BinaryLearners[6].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[6].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[6].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[6].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[6].ScoreTransform = Identity;
    Mdl_BinaryLearners[6].Bias = 1.5221193148214145;
    memcpy(&Mdl_BinaryLearners[6].Beta[0], &dv6[0], 32U * sizeof(double));
    Mdl_BinaryLearners[6].Scale = 3.3179812036548771E+7;
    Mdl_BinaryLearners[6].Prior[0] = 0.5;
    Mdl_BinaryLearners[6].Prior[1] = 0.5;
    Mdl_BinaryLearners[6].Cost[0] = 0.0;
    Mdl_BinaryLearners[6].Cost[1] = 1.0;
    Mdl_BinaryLearners[6].Cost[2] = 1.0;
    Mdl_BinaryLearners[6].Cost[3] = 0.0;

    Mdl_BinaryLearners[7].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[7].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[7].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[7].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[7].ScoreTransform = Identity;
    Mdl_BinaryLearners[7].Bias = 1.7501239117428342;
    memcpy(&Mdl_BinaryLearners[7].Beta[0], &dv7[0], 32U * sizeof(double));
    Mdl_BinaryLearners[7].Scale = 5.8279812481415451E+7;
    Mdl_BinaryLearners[7].Prior[0] = 0.5;
    Mdl_BinaryLearners[7].Prior[1] = 0.5;
    Mdl_BinaryLearners[7].Cost[0] = 0.0;
    Mdl_BinaryLearners[7].Cost[1] = 1.0;
    Mdl_BinaryLearners[7].Cost[2] = 1.0;
    Mdl_BinaryLearners[7].Cost[3] = 0.0;

    Mdl_BinaryLearners[8].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[8].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[8].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[8].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[8].ScoreTransform = Identity;
    Mdl_BinaryLearners[8].Bias = 1.1743126400508417;
    memcpy(&Mdl_BinaryLearners[8].Beta[0], &dv8[0], 32U * sizeof(double));
    Mdl_BinaryLearners[8].Scale = 3.4042990358071744E+7;
    Mdl_BinaryLearners[8].Prior[0] = 0.5;
    Mdl_BinaryLearners[8].Prior[1] = 0.5;
    Mdl_BinaryLearners[8].Cost[0] = 0.0;
    Mdl_BinaryLearners[8].Cost[1] = 1.0;
    Mdl_BinaryLearners[8].Cost[2] = 1.0;
    Mdl_BinaryLearners[8].Cost[3] = 0.0;

    Mdl_BinaryLearners[9].ClassNamesLength[0] = 1;
    Mdl_BinaryLearners[9].ClassLogicalIndices[0] = true;
    Mdl_BinaryLearners[9].ClassNamesLength[1] = 1;
    Mdl_BinaryLearners[9].ClassLogicalIndices[1] = true;
    Mdl_BinaryLearners[9].ScoreTransform = Identity;
    Mdl_BinaryLearners[9].Bias = -2.0305196370555629;
    memcpy(&Mdl_BinaryLearners[9].Beta[0], &dv9[0], 32U * sizeof(double));
    Mdl_BinaryLearners[9].Scale = 5.2752228479032755E+7;
    Mdl_BinaryLearners[9].Prior[0] = 0.5;
    Mdl_BinaryLearners[9].Prior[1] = 0.5;
    Mdl_BinaryLearners[9].Cost[0] = 0.0;
    Mdl_BinaryLearners[9].Cost[1] = 1.0;
    Mdl_BinaryLearners[9].Cost[2] = 1.0;
    Mdl_BinaryLearners[9].Cost[3] = 0.0;

    Mdl_Prior[0] = 0.2;
    Mdl_Prior[1] = 0.2;
    Mdl_Prior[2] = 0.2;
    Mdl_Prior[3] = 0.2;
    Mdl_Prior[4] = 0.2;

    // Read input packet (32 floats)
    float b_in1[32];
    bool is_last_packet = false;

    for(int i = 0; i < 32; i++) {
        val_in = input.read();
        b_in1[i] = *((float*)(&val_in.data));
        if(val_in.last) is_last_packet = true;  // Capture any tlast
    }

    // Process with fresh model - FIXED: Added proper function call
    float result = (float)c_CompactClassificationECOC_pre(Mdl_Prior,
                                                         Mdl_BinaryLearners,
                                                         Mdl_CodingMatrix,
                                                         b_in1);

    // Output result with correct tlast propagation
    val_out.data = *((ap_uint<32>*)(&result));
    val_out.last = is_last_packet;  // Propagate tlast correctly
    output.write(val_out);

    if(is_last_packet) break;  // Exit loop after final packet
}
}