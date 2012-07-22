#ifndef HYDRA_H
#define HYDRA_H

#include "Vector.h"
#include "Matrix.h"

class Hydra;

Hydra * initHydra();
void exitHydra();

// left/right, up/down, trigger, bumper, start, stick, 1, 2, 3, 4

int getHands(Hydra * hydra, Matrix &left, Matrix &right, float * controls);

#endif//HYDRA_H
