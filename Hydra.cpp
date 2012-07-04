#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>

#include "Hydra.h"

#include <sixense.h>

class Hydra
{
public:
	sixenseAllControllerData _alldata;
};

Hydra * initHydra()
{
	if (sixenseInit() == SIXENSE_FAILURE)
		return NULL;

	Hydra * hydra = new Hydra;

	//
	//sixenseSetFilterEnabled(1);
	//sixenseSetFilterParams(near_range, near_val, far_range, far_val);

	return hydra;
}

void exitHydra()
{
	sixenseExit();
}

int getHands(Hydra * hydra, Matrix &left, Matrix &right, float * controls)
{
	if (!hydra)
		return 0;

	int hands = 0;
	for( int base=0; base<sixenseGetMaxBases(); base++ ) {
		sixenseSetActiveBase(base);

		// Get the latest controller data
		sixenseAllControllerData acd;
		sixenseGetAllNewestData( &acd );

		// For each possible controller
		for( int cont=0; cont<sixenseGetMaxControllers(); cont++ ) {

			// See if it's enabled
			if( acd.controllers[cont].enabled ) {
				if (acd.controllers[cont].which_hand == 1)
				{
					hands |= 1;
					left._m[ 0] = acd.controllers[cont].rot_mat[0][0];
					left._m[ 1] = acd.controllers[cont].rot_mat[0][1];
					left._m[ 2] = acd.controllers[cont].rot_mat[0][2];
					left._m[ 3] = 0;
					left._m[ 4] = acd.controllers[cont].rot_mat[1][0];
					left._m[ 5] = acd.controllers[cont].rot_mat[1][1];
					left._m[ 6] = acd.controllers[cont].rot_mat[1][2];
					left._m[ 7] = 0;
					left._m[ 8] = acd.controllers[cont].rot_mat[2][0];
					left._m[ 9] = acd.controllers[cont].rot_mat[2][1];
					left._m[10] = acd.controllers[cont].rot_mat[2][2];
					left._m[11] = 0;
					left._m[12] = acd.controllers[cont].pos[0] * 0.1f;
					left._m[13] = acd.controllers[cont].pos[1] * 0.1f;
					left._m[14] = acd.controllers[cont].pos[2] * 0.1f;
					left._m[15] = 1;

					controls[0] = acd.controllers[cont].joystick_x * 1.f / 256.f;
					controls[1] = acd.controllers[cont].joystick_y * 1.f / 256.f;
					controls[2] = acd.controllers[cont].trigger * 1.f / 256.f;

					controls[3] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_BUMPER)? 1.f : 0.f;
					controls[4] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_START)? 1.f : 0.f;
					controls[5] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_JOYSTICK)? 1.f : 0.f;
					controls[6] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_1)? 1.f : 0.f;
					controls[7] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_2)? 1.f : 0.f;
					controls[8] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_3)? 1.f : 0.f;
					controls[9] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_4)? 1.f : 0.f;
				}
				else
				{
					hands |= 2;
					right._m[ 0] = acd.controllers[cont].rot_mat[0][0];
					right._m[ 1] = acd.controllers[cont].rot_mat[0][1];
					right._m[ 2] = acd.controllers[cont].rot_mat[0][2];
					right._m[ 3] = 0;
					right._m[ 4] = acd.controllers[cont].rot_mat[1][0];
					right._m[ 5] = acd.controllers[cont].rot_mat[1][1];
					right._m[ 6] = acd.controllers[cont].rot_mat[1][2];
					right._m[ 7] = 0;
					right._m[ 8] = acd.controllers[cont].rot_mat[2][0];
					right._m[ 9] = acd.controllers[cont].rot_mat[2][1];
					right._m[10] = acd.controllers[cont].rot_mat[2][2];
					right._m[11] = 0;
					right._m[12] = acd.controllers[cont].pos[0] * 0.1f;
					right._m[13] = acd.controllers[cont].pos[1] * 0.1f;
					right._m[14] = acd.controllers[cont].pos[2] * 0.1f;
					right._m[15] = 1;

					controls[10] = acd.controllers[cont].joystick_x * 1.f / 256.f;
					controls[11] = acd.controllers[cont].joystick_y * 1.f / 256.f;
					controls[12] = acd.controllers[cont].trigger * 1.f / 256.f;

					controls[13] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_BUMPER)? 1.f : 0.f;
					controls[14] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_START)? 1.f : 0.f;
					controls[15] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_JOYSTICK)? 1.f : 0.f;
					controls[16] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_1)? 1.f : 0.f;
					controls[17] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_2)? 1.f : 0.f;
					controls[18] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_3)? 1.f : 0.f;
					controls[19] = (acd.controllers[cont].buttons&SIXENSE_BUTTON_4)? 1.f : 0.f;
				}
			}
		}
	}
	return hands;
}

