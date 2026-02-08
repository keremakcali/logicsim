#ifndef CONFIG_H
#define CONFIG_H

#include "raylib.h"
extern float SideBarWidth;
extern float SideBarComponentWidth;
extern float SideBarComponentPadding;
extern float SideBarButtonHeight;
extern float SideBarNameInputHeight;

extern float ComponentPinRad;
extern float ComponentPinPad;

int readConfig();

#endif // !CONFIG_H
