#include "Yamamoto/kartParams.h"

TireOp marioluiziTireOp[4] = {
    {-0.0523333f, -0.0087222f, 41.0f, -20.0f, -55.5f, 55.0f, 1.0606f, 5.0f, 11.0f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0087222f, 41.0f, 20.0f, 55.5f, 55.0f, 1.0606f, 5.0f, 11.0f, 3.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.0087222f, 42.5f, -20.0f, -62.0f, -55.0f, 1.0606f, 5.0f, 11.0f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0087222f, 42.5f, 20.0f, 62.0f, -55.0f, 1.0606f, 5.0f, 11.0f, 3.0f, 1.0f, 1.0f, 0.0f},
};

TireOp biglTireOp[4] = {
    {-0.0523333f, -0.00174444f, 44.0f, -20.0f, -72.0f, 70.0f, 1.0f, 5.0f, 6.8f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 44.0f, 20.0f, 72.0f, 70.0f, 1.0f, 5.0f, 6.8f, 0.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.00174444f, 44.0f, -20.0f, -72.0f, -70.0f, 1.0f, 5.0f, 6.8f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 44.0f, 20.0f, 72.0f, -70.0f, 1.0f, 5.0f, 6.8f, 0.0f, 1.0f, 1.0f, 0.0f},
};

TireOp KuppaTireOp[4] = {
    {-0.0523333f, -0.00174444f, 44.0f, -20.0f, -72.0f, 93.5f, 1.33f, 5.0f, 10.0f, 0.0f, 0.9f, -5.0f, -73.5f},
    {0.0523333f, 0.00174444f, 44.0f, 20.0f, 72.0f, 93.5f, 1.33f, 5.0f, 10.0f, 0.0f, 0.9f, -5.0f, -73.5f},
    {-0.0523333f, -0.00174444f, 46.0f, -20.0f, -75.0f, -70.0f, 1.33f, 5.0f, 1.0f, 0.0f, 1.0f, -5.0f, -73.5f},
    {0.0523333f, 0.00174444f, 46.0f, 20.0f, 75.0f, -70.0f, 1.33f, 5.0f, 1.0f, 0.0f, 1.0f, -5.0f, -73.5f},
};

TireOp peachTireOp[4] = {
    {-0.0523333f, -0.00174444f, 31.5f, -20.0f, -55.0f, 75.0f, 1.1f, 5.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 31.5f, 20.0f, 55.0f, 75.0f, 1.1f, 5.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.00174444f, 34.5f, -20.0f, -55.0f, -55.0f, 1.1f, 5.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 34.5f, 20.0f, 55.0f, -55.0f, 1.1f, 5.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
};

TireOp yossyTireOp[4] = {
    {-0.0523333f, -0.00174444f, 41.0f, -20.0f, -57.0f, 80.0f, 1.0606f, 5.0f, 8.5f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 41.0f, 20.0f, 57.0f, 80.0f, 1.0606f, 5.0f, 8.5f, 3.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.00174444f, 42.5f, -20.0f, -60.0f, -55.0f, 1.0606f, 5.0f, 8.5f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 42.5f, 20.0f, 60.0f, -55.0f, 1.0606f, 5.0f, 8.5f, 3.0f, 1.0f, 1.0f, 0.0f},
};

TireOp nokonokoTireOp[4] = {
    {-0.0523333f, -0.0f, 24.0f, -18.0f, -47.0f, 44.0f, 1.0f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
    {0.0523333f, 0.0f, 24.0f, 18.0f, 47.0f, 44.0f, 1.0f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
    {-0.0523333f, -0.0f, 24.0f, -18.0f, -55.0f, -48.0f, 1.0f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
    {0.0523333f, 0.0f, 24.0f, 18.0f, 55.0f, -48.0f, 1.0f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
};

TireOp babyTireOp[4] = {
    {-0.0523333f, -0.0f, 21.0f, -18.0f, -45.0f, 34.0f, 1.1f, 5.0f, 2.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0f, 21.0f, 18.0f, 45.0f, 34.0f, 1.1f, 5.0f, 2.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.0f, 23.0f, -18.0f, -48.0f, -34.0f, 1.1f, 5.0f, 2.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0f, 23.0f, 18.0f, 48.0f, -34.0f, 1.1f, 5.0f, 2.0f, 5.0f, 1.0f, 1.0f, 0.0f},
};

TireOp warioTireOp[4] = {
    {-0.0523333f, -0.00174444f, 34.0f, -20.0f, -75.0f, 120.0f, 1.0f, 5.0f, 2.5f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 34.0f, 20.0f, 75.0f, 120.0f, 1.0f, 5.0f, 2.5f, 0.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.00174444f, 34.0f, -20.0f, -75.0f, -70.0f, 1.0f, 5.0f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.00174444f, 34.0f, 20.0f, 75.0f, -70.0f, 1.0f, 5.0f, 2.0f, 0.0f, 1.0f, 1.0f, 0.0f},
};

TireOp kuppajrTireOp[4] = {
    {-0.0523333f, -0.0f, 21.0f, -18.0f, -48.0f, 56.0f, 1.1f, 5.0f, 1.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0f, 21.0f, 18.0f, 48.0f, 56.0f, 1.1f, 5.0f, 1.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.0f, 23.0f, -18.0f, -51.0f, -33.0f, 1.1f, 5.0f, 1.0f, 5.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0f, 23.0f, 18.0f, 51.0f, -33.0f, 1.1f, 5.0f, 1.0f, 5.0f, 1.0f, 1.0f, 0.0f},
};

TireOp waluiziTireOp[4] = {
    {-0.0523333f, -0.0f, 27.0f, -20.0f, -52.0f, 90.0f, 1.25f, 5.0f, 3.2f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0f, 27.0f, 20.0f, 52.0f, 90.0f, 1.25f, 5.0f, 3.2f, 3.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.0f, 33.5f, -20.0f, -70.0f, -55.0f, 1.25f, 5.0f, 3.2f, 3.0f, 1.6f, -3.0f, 0.0f},
    {0.0523333f, 0.0f, 33.5f, 20.0f, 70.0f, -55.0f, 1.25f, 5.0f, 3.2f, 3.0f, 1.6f, -3.0f, 0.0f},
};

TireOp pakknTireOp[4] = {
    {-0.0523333f, -0.0f, 20.0f, -18.0f, -73.5f, 70.0f, 1.2f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
    {0.0523333f, 0.0f, 20.0f, 18.0f, 73.5f, 70.0f, 1.2f, -0.0f, -0.5f, 5.0f, 1.0f, -5.0f, 0.0f},
    {-0.0523333f, -0.0f, 24.0f, -18.0f, -70.0f, -75.5f, 1.2f, -0.0f, -0.5f, 5.0f, 1.4f, -5.0f, 0.0f},
    {0.0523333f, 0.0f, 24.0f, 18.0f, 70.0f, -75.5f, 1.2f, -0.0f, -0.5f, 5.0f, 1.4f, -5.0f, 0.0f},
};

TireOp didyTireOp[4] = {
    {-0.0523333f, -0.0f, 21.0f, -20.0f, -40.0f, 90.0f, 1.27f, 5.0f, 1.5f, 5.0f, 1.1f, 1.0f, 59.0f},
    {0.0523333f, 0.0f, 21.0f, 20.0f, 40.0f, 90.0f, 1.27f, 5.0f, 1.5f, 5.0f, 1.1f, 1.0f, 59.0f},
    {-0.0523333f, -0.0f, 26.5f, -20.0f, -41.7f, -47.0f, 1.27f, 5.0f, 1.5f, 5.0f, 1.2f, 1.0f, 59.0f},
    {0.0523333f, 0.0f, 26.5f, 20.0f, 41.7f, -47.0f, 1.27f, 5.0f, 1.5f, 5.0f, 1.2f, 1.0f, 59.0f},
};

TireOp kinoTireOp[4] = {
    {-0.0523333f, -0.0f, 17.0f, -18.0f, -47.0f, 52.0f, 1.55f, -0.0f, -0.5f, 5.0f, 1.0f, 0.0f, 0.0f},
    {0.0523333f, 0.0f, 17.0f, 18.0f, 47.0f, 52.0f, 1.55f, -0.0f, -0.5f, 5.0f, 1.0f, 0.0f, 0.0f},
    {-0.0523333f, -0.0f, 26.5f, -18.0f, -55.0f, -52.0f, 1.55f, -0.0f, -0.5f, 5.0f, 1.55f, 0.0f, 0.0f},
    {0.0523333f, 0.0f, 26.5f, 18.0f, 55.0f, -52.0f, 1.55f, -0.0f, -0.5f, 5.0f, 1.55f, 0.0f, 0.0f},
};

TireOp demolTireOp[4] = {
    {-0.0523333f, -0.0087222f, 29.0f, -20.0f, -80.5f, 105.0f, 1.1f, 5.0f, 2.0f, 3.0f, 1.0f, 1.0f, 0.0f},
    {0.0523333f, 0.0087222f, 29.0f, 20.0f, 80.5f, 105.0f, 1.1f, 5.0f, 2.0f, 3.0f, 1.0f, 1.0f, 0.0f},
    {-0.0523333f, -0.0087222f, 32.0f, -20.0f, -87.0f, -95.0f, 1.1f, 5.0f, 2.0f, 3.0f, 1.1f, 1.0f, 0.0f},
    {0.0523333f, 0.0087222f, 32.0f, 20.0f, 87.0f, -95.0f, 1.1f, 5.0f, 2.0f, 3.0f, 1.1f, 1.0f, 0.0f},
};

TireOp *TireOpData[21] = {
    marioluiziTireOp, biglTireOp, yossyTireOp, nokonokoTireOp, peachTireOp, babyTireOp, warioTireOp,
    KuppaTireOp, marioluiziTireOp, didyTireOp, yossyTireOp, nokonokoTireOp, peachTireOp, babyTireOp,
    waluiziTireOp, kuppajrTireOp, kinoTireOp, kinoTireOp, pakknTireOp, pakknTireOp, demolTireOp,
};

ArmOp normalArmOp[4] = {
    {-0.001f, -3.0f, 55.0f, -13.0f},
    {0.001f, -3.0f, 55.0f, -13.0f},
    {-0.001f, -3.0f, -55.0f, -13.0f},
    {0.001f, -3.0f, -55.0f, -13.0f},
};

ArmOp peachArmOp[4] = {
    {-0.001f, -16.5f, 75.0f, -13.0f},
    {0.001f, -16.5f, 75.0f, -13.0f},
    {-0.001f, -16.5f, -55.0f, -13.0f},
    {0.001f, -16.5f, -55.0f, -13.0f},
};

ArmOp kuppaArmOp[4] = {
    {-0.001f, -11.0f, 93.5f, -7.0f},
    {0.001f, -11.0f, 93.5f, -7.0f},
    {-0.001f, -11.0f, -70.0f, -7.0f},
    {0.001f, -11.0f, -70.0f, -7.0f},
};

ArmOp donkyArmOp[4] = {
    {-0.001f, -11.0f, 70.0f, -6.0f},
    {0.001f, -11.0f, 70.0f, -6.0f},
    {-0.001f, -11.0f, -70.0f, -6.0f},
    {0.001f, -11.0f, -70.0f, -6.0f},
};

ArmOp yossyArmOp[4] = {
    {-0.001f, -5.0f, 80.0f, -13.0f},
    {0.001f, -5.0f, 80.0f, -13.0f},
    {-0.001f, -5.0f, -55.0f, -13.0f},
    {0.001f, -5.0f, -55.0f, -13.0f},
};

ArmOp nokonokoArmOp[4] = {
    {-0.001f, -17.0f, 44.0f, -13.0f},
    {0.001f, -17.0f, 44.0f, -13.0f},
    {-0.001f, -17.0f, -44.0f, -13.0f},
    {0.001f, -17.0f, -44.0f, -13.0f},
};

ArmOp babyArmOp[4] = {
    {-0.001f, -17.0f, 34.0f, -13.0f},
    {0.001f, -17.0f, 34.0f, -13.0f},
    {-0.001f, -17.0f, -34.0f, -13.0f},
    {0.001f, -17.0f, -34.0f, -13.0f},
};

ArmOp warioArmOp[4] = {
    {-30.0f, -6.0f, 120.0f, -13.0f},
    {30.0f, -6.0f, 120.0f, -13.0f},
    {-30.0f, -6.0f, -70.0f, -13.0f},
    {30.0f, -6.0f, -70.0f, -13.0f},
};

ArmOp kuppajrArmOp[4] = {
    {-0.001f, -22.0f, 56.0f, -10.0f},
    {0.001f, -22.0f, 56.0f, -10.0f},
    {-0.001f, -22.0f, -33.0f, -10.0f},
    {0.001f, -22.0f, -33.0f, -10.0f},
};

ArmOp waluiziArmOp[4] = {
    {-0.001f, -11.5f, 90.0f, -6.0f},
    {0.001f, -11.5f, 90.0f, -6.0f},
    {-0.001f, -5.0f, -55.0f, -6.0f},
    {0.001f, -5.0f, -55.0f, -6.0f},
};

ArmOp pakknArmOp[4] = {
    {-0.001f, -30.0f, 70.0f, -6.0f},
    {0.001f, -30.0f, 70.0f, -6.0f},
    {-0.001f, -30.0f, -75.5f, -6.0f},
    {0.001f, -30.0f, -75.5f, -6.0f},
};

ArmOp didiyArmOp[4] = {
    {-14.0f, -24.0f, 90.0f, -1.0f},
    {14.0f, -24.0f, 90.0f, -1.0f},
    {-14.0f, -24.0f, -47.0f, -1.0f},
    {14.0f, -24.0f, -47.0f, -1.0f},
};

ArmOp kinopioArmOp[4] = {
    {-20.0f, -13.0f, 52.0f, -1.0f},
    {20.0f, -13.0f, 52.0f, -1.0f},
    {-27.0f, -3.0f, -52.0f, -1.0f},
    {27.0f, -3.0f, -52.0f, -1.0f},
};

ArmOp demoArmOp[4] = {
    {-55.0f, -12.5f, 105.0f, -1.0f},
    {55.0f, -12.5f, 105.0f, -1.0f},
    {-55.0f, -10.0f, -95.0f, -1.0f},
    {55.0f, -10.0f, -95.0f, -1.0f},
};

ArmOp *ArmOpData[21] = {
    normalArmOp, donkyArmOp, yossyArmOp, nokonokoArmOp, peachArmOp, babyArmOp, warioArmOp,
    kuppaArmOp, normalArmOp, didiyArmOp, yossyArmOp, nokonokoArmOp, peachArmOp, babyArmOp,
    waluiziArmOp, kuppajrArmOp, kinopioArmOp, kinopioArmOp, pakknArmOp, pakknArmOp, demoArmOp,
};

DumpOp normalDumpOp[4] = {
    {-28.6f, 10.0f, 55.0f, -37.0f, 24.0f},
    {28.6f, 10.0f, 55.0f, 37.0f, 24.0f},
    {-28.6f, 10.0f, -55.0f, -37.0f, 22.5f},
    {28.6f, 10.0f, -55.0f, 37.0f, 22.5f},
};

DumpOp bigDumpOp[4] = {
    {-45.0f, 11.0f, 70.0f, -54.5f, 25.0f},
    {45.0f, 11.0f, 70.0f, 54.5f, 25.0f},
    {-45.0f, 11.0f, -70.0f, -54.5f, 25.0f},
    {45.0f, 11.0f, -70.0f, 54.5f, 25.0f},
};

DumpOp peachDumpOp[4] = {
    {-27.0f, 6.5f, 75.0f, -35.0f, 25.0f},
    {27.0f, 6.5f, 75.0f, 35.0f, 25.0f},
    {-27.0f, 6.5f, -55.0f, -35.0f, 25.0f},
    {27.0f, 6.5f, -55.0f, 35.0f, 25.0f},
};

DumpOp kuppaDumpOp[4] = {
    {-45.0f, 11.0f, 93.5f, -54.5f, 25.0f},
    {45.0f, 11.0f, 93.5f, 54.5f, 25.0f},
    {-38.0f, 11.0f, -70.0f, -54.5f, 25.0f},
    {38.0f, 11.0f, -70.0f, 54.5f, 25.0f},
};

DumpOp yossyDumpOp[4] = {
    {-28.6f, 10.0f, 80.0f, -32.0f, 25.0f},
    {28.6f, 10.0f, 80.0f, 32.0f, 25.0f},
    {-28.6f, 10.0f, -55.0f, -32.0f, 25.0f},
    {28.6f, 10.0f, -55.0f, 32.0f, 25.0f},
};

DumpOp nokonokoDumpOp[4] = {
    {-15.0f, -9.0f, 44.0f, -32.0f, 20.0f},
    {15.0f, -9.0f, 44.0f, 32.0f, 20.0f},
    {-15.0f, -9.0f, -44.0f, -32.0f, 20.0f},
    {15.0f, -9.0f, -44.0f, 32.0f, 20.0f},
};

DumpOp babyDumpOp[4] = {
    {-24.0f, 5.0f, 34.0f, -29.0f, 31.0f},
    {24.0f, 5.0f, 34.0f, 29.0f, 31.0f},
    {-28.0f, 5.0f, -34.0f, -34.0f, 31.0f},
    {28.0f, 5.0f, -34.0f, 34.0f, 31.0f},
};

DumpOp warioDumpOp[4] = {
    {-42.0f, 30.0f, 120.0f, -27.0f, 40.0f},
    {42.0f, 30.0f, 120.0f, 27.0f, 40.0f},
    {-42.0f, 30.0f, -70.0f, -27.0f, 40.0f},
    {42.0f, 30.0f, -70.0f, 27.0f, 40.0f},
};

DumpOp kuppajrDumpOp[4] = {
    {-30.0f, -10.0f, 56.0f, -34.0f, 13.0f},
    {30.0f, -10.0f, 56.0f, 34.0f, 13.0f},
    {-30.0f, -10.0f, -33.0f, -37.0f, 14.0f},
    {30.0f, -10.0f, -33.0f, 37.0f, 14.0f},
};

DumpOp waluiziDumpOp[4] = {
    {-23.0f, 6.5f, 90.0f, -31.0f, 20.0f},
    {23.0f, 6.5f, 90.0f, 31.0f, 20.0f},
    {-37.0f, 6.5f, -55.0f, -43.0f, 18.3f},
    {37.0f, 6.5f, -55.0f, 43.0f, 18.3f},
};

DumpOp pakknDumpOp[4] = {
    {-48.0f, -8.0f, 70.0f, -55.0f, 25.0f},
    {48.0f, -8.0f, 70.0f, 55.0f, 25.0f},
    {-39.0f, -8.0f, -75.5f, -46.0f, 25.0f},
    {39.0f, -8.0f, -75.5f, 46.0f, 25.0f},
};

DumpOp demoDumpOp[4] = {
    {-55.0f, 16.0f, 105.0f, -10.0f, 30.0f},
    {55.0f, 16.0f, 105.0f, 10.0f, 30.0f},
    {-55.0f, 20.0f, -95.0f, -12.5f, 30.0f},
    {55.0f, 20.0f, -95.0f, 12.5f, 30.0f},
};

DumpOp didyDumpOp[4] = {
    {-15.0f, -24.0f, 44.0f, -32.0f, 20.0f},
    {15.0f, -24.0f, 44.0f, 32.0f, 20.0f},
    {-15.0f, -24.0f, -44.0f, -32.0f, 20.0f},
    {15.0f, -24.0f, -44.0f, 32.0f, 20.0f},
};

DumpOp *DumpOpData[21] = {
    normalDumpOp, bigDumpOp, yossyDumpOp, nokonokoDumpOp, peachDumpOp, babyDumpOp, warioDumpOp,
    kuppaDumpOp, normalDumpOp, didyDumpOp, yossyDumpOp, nokonokoDumpOp, peachDumpOp, babyDumpOp,
    waluiziDumpOp, kuppajrDumpOp, nokonokoDumpOp, nokonokoDumpOp, pakknDumpOp, pakknDumpOp, demoDumpOp,
};

BodyOp marioBodyOp = {
    90.0f, 130.0f, 1.18f, 1.18f, 5.0f, 9.0f,
};

BodyOp bigBodyOp = {
    90.0f, 150.0f, 1.15f, 1.15f, 5.0f, 9.0f,
};

BodyOp peachBodyOp = {
    90.0f, 140.0f, 1.8f, 1.8f, 40.0f, 9.0f,
};

BodyOp nokonokoBodyOp = {
    90.0f, 130.0f, 1.2f, 1.2f, 15.0f, 2.0f,
};

BodyOp babyBodyOp = {
    90.0f, 130.0f, 2.3f, 2.3f, 23.0f, 2.0f,
};

BodyOp warioBodyOp = {
    90.0f, 170.0f, 2.7f, 2.7f, 25.0f, 9.0f,
};

BodyOp kuppaBodyOp = {
    90.0f, 170.0f, 1.8f, 1.8f, 30.0f, 9.0f,
};

BodyOp waluiziBodyOp = {
    90.0f, 150.0f, 1.2f, 1.2f, 15.0f, 9.0f,
};

BodyOp pakknBodyOp = {
    90.0f, 150.0f, 1.2f, 1.2f, 40.0f, 2.0f,
};

BodyOp didyBodyOp = {
    90.0f, 130.0f, 2.4f, 2.3f, 20.0f, 2.0f,
};

BodyOp kuppajrBodyOp = {
    90.0f, 130.0f, 1.2f, 1.2f, 23.0f, 2.0f,
};

BodyOp kinopioBodyOp = {
    90.0f, 130.0f, 2.5f, 3.5f, 15.0f, 1.0f,
};

BodyOp demoBodyOp = {
    90.0f, 170.0f, 4.1f, 4.1f, 0.0f, 9.0f,
};

BodyOp *BodyOpData[21] = {
    &marioBodyOp, &bigBodyOp, &marioBodyOp, &nokonokoBodyOp, &peachBodyOp, &babyBodyOp, &warioBodyOp,
    &kuppaBodyOp, &marioBodyOp, &didyBodyOp, &marioBodyOp, &nokonokoBodyOp, &peachBodyOp, &babyBodyOp,
    &waluiziBodyOp, &kuppajrBodyOp, &kinopioBodyOp, &kinopioBodyOp, &pakknBodyOp, &pakknBodyOp, &demoBodyOp,
};

static CubeParam cubeWluiziPos = {{
    {-90.0f, -10.0f, 100.0f},
    {90.0f, -10.0f, 100.0f},
    {-100.0f, -10.0f, -100.0f},
    {100.0f, -10.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

static CubeParam cubeYossyPos = {{
    {-90.0f, -10.0f, 100.0f},
    {90.0f, -10.0f, 100.0f},
    {-100.0f, -10.0f, -100.0f},
    {100.0f, -10.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

static CubeParam cubeDonkyPos = {{
    {-120.0f, -10.0f, 160.0f},
    {120.0f, -10.0f, 160.0f},
    {-120.0f, -10.0f, -150.0f},
    {120.0f, -10.0f, -150.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 75.0f, -100.0f},
    {50.0f, 75.0f, -100.0f},
}};

static CubeParam cubeMarioPos = {{
    {-90.0f, -10.0f, 100.0f},
    {90.0f, -10.0f, 100.0f},
    {-100.0f, -10.0f, -100.0f},
    {100.0f, -10.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

static CubeParam cubeWarioPos = {{
    {-120.0f, -10.0f, 130.0f},
    {120.0f, -10.0f, 130.0f},
    {-120.0f, -10.0f, -90.0f},
    {120.0f, -10.0f, -90.0f},
    {-50.0f, 55.0f, 130.0f},
    {50.0f, 55.0f, 130.0f},
    {-50.0f, 75.0f, -90.0f},
    {50.0f, 75.0f, -90.0f},
}};

static CubeParam cubeBabyPos = {{
    {-70.0f, -10.0f, 100.0f},
    {70.0f, -10.0f, 100.0f},
    {-70.0f, -10.0f, -100.0f},
    {70.0f, -10.0f, -100.0f},
    {-50.0f, 55.0f, 70.0f},
    {50.0f, 55.0f, 70.0f},
    {-50.0f, 70.0f, -70.0f},
    {50.0f, 70.0f, -70.0f},
}};

static CubeParam cubePeachPos = {{
    {-90.0f, -10.0f, 100.0f},
    {90.0f, -10.0f, 100.0f},
    {-100.0f, -10.0f, -100.0f},
    {100.0f, -10.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

static CubeParam cubeKuppaPos = {{
    {-120.0f, -10.0f, 160.0f},
    {120.0f, -10.0f, 160.0f},
    {-120.0f, -10.0f, -150.0f},
    {120.0f, -10.0f, -150.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 75.0f, -100.0f},
    {50.0f, 75.0f, -100.0f},
}};

static CubeParam cubeNokoNokoPos = {{
    {-45.0f, -5.0f, 45.0f},
    {45.0f, -5.0f, 45.0f},
    {-45.0f, -5.0f, -45.0f},
    {45.0f, -5.0f, -45.0f},
    {-45.0f, 55.0f, 45.0f},
    {45.0f, 55.0f, 45.0f},
    {-45.0f, 70.0f, -45.0f},
    {45.0f, 70.0f, -45.0f},
}};

static CubeParam didyMiniPos = {{
    {-60.0f, -35.0f, 140.0f},
    {60.0f, -35.0f, 140.0f},
    {-60.0f, -35.0f, -100.0f},
    {60.0f, -35.0f, -100.0f},
    {-50.0f, 55.0f, 70.0f},
    {50.0f, 55.0f, 70.0f},
    {-50.0f, 70.0f, -70.0f},
    {50.0f, 70.0f, -70.0f},
}};

static CubeParam kinopioMiniPos = {{
    {-60.0f, 5.0f, 60.0f},
    {60.0f, 5.0f, 60.0f},
    {-60.0f, 5.0f, -60.0f},
    {60.0f, 5.0f, -60.0f},
    {-50.0f, 55.0f, 70.0f},
    {50.0f, 55.0f, 70.0f},
    {-50.0f, 70.0f, -70.0f},
    {50.0f, 70.0f, -70.0f},
}};

static CubeParam demoNormalPos = {{
    {-90.0f, -30.0f, 120.0f},
    {90.0f, -30.0f, 120.0f},
    {-100.0f, -30.0f, -100.0f},
    {100.0f, -30.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

static CubeParam cubePukknPos = {{
    {-90.0f, -20.0f, 140.0f},
    {90.0f, -20.0f, 140.0f},
    {-100.0f, -20.0f, -100.0f},
    {100.0f, -20.0f, -100.0f},
    {-50.0f, 55.0f, 90.0f},
    {50.0f, 55.0f, 90.0f},
    {-50.0f, 70.0f, -100.0f},
    {50.0f, 70.0f, -100.0f},
}};

CubeParam *CubeParamsData[21] = {
    &cubeMarioPos, &cubeDonkyPos, &cubeYossyPos, &cubeNokoNokoPos, &cubePeachPos, &cubeBabyPos, &cubeWarioPos,
    &cubeKuppaPos, &cubeMarioPos, &didyMiniPos, &cubeYossyPos, &cubeNokoNokoPos, &cubePeachPos, &cubeBabyPos,
    &cubeWluiziPos, &cubeBabyPos, &kinopioMiniPos, &kinopioMiniPos, &cubePukknPos, &cubePukknPos, &demoNormalPos,
};

CamOp marioCamOp = {
    -140.0f, 200.0f, 0.348888f,
};

CamOp donkyCamOp = {
    -140.0f, 230.0f, 0.348888f,
};

CamOp yossyCamOp = {
    -120.0f, 210.0f, 0.348888f,
};

CamOp nokonokoCamOp = {
    -120.0f, 190.0f, 0.348888f,
};

CamOp peachCamOp = {
    -130.0f, 190.0f, 0.348888f,
};

CamOp babyCamOp = {
    -130.0f, 170.0f, 0.348888f,
};

CamOp waruCamOp = {
    -130.0f, 260.0f, 0.348888f,
};

CamOp kuppaCamOp = {
    -120.0f, 220.0f, 0.348888f,
};

CamOp *CameraOpData[21] = {
    &marioCamOp, &donkyCamOp, &yossyCamOp, &nokonokoCamOp, &peachCamOp, &babyCamOp, &waruCamOp,
    &kuppaCamOp, &marioCamOp, &babyCamOp, &yossyCamOp, &nokonokoCamOp, &peachCamOp, &babyCamOp,
    &marioCamOp, &babyCamOp, &nokonokoCamOp, &nokonokoCamOp, &marioCamOp, &marioCamOp, &marioCamOp,
};

f32 RescueSeaDepth[21] = {
    170.0f, 170.0f, 165.0f, 170.0f, 155.0f, 160.0f, 180.0f,
    150.0f, 170.0f, 160.0f, 165.0f, 170.0f, 155.0f, 160.0f,
    160.0f, 160.0f, 180.0f, 180.0f, 140.0f, 140.0f, 180.0f,
};

f32 floorDepth[21] = {
    60.0f, 60.0f, 55.0f, 40.0f, 60.0f, 60.0f, 40.0f,
    60.0f, 60.0f, 40.0f, 55.0f, 40.0f, 60.0f, 60.0f,
    60.0f, 60.0f, 20.0f, 20.0f, 70.0f, 70.0f, 30.0f,
};

f32 floorThunderDepth[21] = {
    60.0f, 60.0f, 55.0f, 40.0f, 60.0f, 30.0f, 40.0f,
    60.0f, 60.0f, 40.0f, 55.0f, 40.0f, 60.0f, 30.0f,
    60.0f, 30.0f, 20.0f, 20.0f, 30.0f, 30.0f, 30.0f,
};

f32 bridgeDepth[21] = {
    30.0f, 35.0f, 30.0f, 20.0f, 70.0f, 60.0f, 40.0f,
    60.0f, 30.0f, 30.0f, 30.0f, 20.0f, 70.0f, 60.0f,
    20.0f, 20.0f, 20.0f, 20.0f, 70.0f, 70.0f, 30.0f,
};

f32 bridgeThunderDepth[21] = {
    30.0f, 35.0f, 30.0f, 20.0f, 70.0f, 60.0f, 40.0f,
    60.0f, 30.0f, 30.0f, 30.0f, 20.0f, 70.0f, 60.0f,
    20.0f, 20.0f, 20.0f, 20.0f, 70.0f, 70.0f, 30.0f,
};

f32 tireOffsetPos[21] = {
    10.0f, 7.0f, 8.0f, -1.0f, 0.0f, 2.0f, 2.0f,
    1.0f, 10.0f, 1.0f, 8.0f, -1.0f, 0.0f, 2.0f,
    3.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
};

#include "JSystem/JAudio/JASFakeMatch2.h"
