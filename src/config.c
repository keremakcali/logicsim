


#include "config.h"
#include <stdio.h>
#include "raylib.h"
const char *fileName = "config";

float SideBarWidth;
float SideBarComponentWidth;
float SideBarComponentPadding;
float SideBarButtonHeight;
float SideBarNameInputHeight;

float ComponentPinRad;
float ComponentPinPad;



int readConfig() {
    FILE *file = fopen(fileName, "r"); 
    if (!file) return 0;

    fscanf(file, " SideBarWidth : %f ", &SideBarWidth);
    fscanf(file, " SideBarComponentWidth : %f ", &SideBarComponentWidth);
    fscanf(file, " SideBarComponentPadding : %f ", &SideBarComponentPadding);
    fscanf(file, " SideBarButtonHeight : %f ", &SideBarButtonHeight);
    fscanf(file, " SideBarNameInputHeight : %f ", &SideBarNameInputHeight);
    fscanf(file, " ComponentPinRad : %f ", &ComponentPinRad);
    fscanf(file, " ComponentPinPad : %f ", &ComponentPinPad);


    fclose(file);


    return 1;
}
