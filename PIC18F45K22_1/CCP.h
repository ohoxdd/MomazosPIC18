#ifndef CCP_H
#define CCP_H

#include <xc.h>
#include "typedefs.h"

void change_pwm_values(struct DC_values values);
void change_pwm_profile(int selected_pressure);
#endif // !CCP_H
