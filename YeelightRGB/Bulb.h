#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct
{
	int hue;
	int saturation;
	int brightness;
} bulb_Color_T;

BOOL bulb_IsColorValid (bulb_Color_T color);

COLORREF bulb_ToRGB (bulb_Color_T color);

BOOL bulb_Toggle (void);

BOOL bulb_Color (bulb_Color_T color);