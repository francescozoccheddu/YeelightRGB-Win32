#include "Bulb.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <math.h>
#include <strsafe.h>

#include "Send.h"

#define BULB_ID "0"
#define MSG_TOGGLE "{\"id\":" BULB_ID ",\"method\":\"toggle\",\"params\":[]}\r\n"
#define MSG_COLOR "{\"id\":" BULB_ID ",\"method\":\"set_power\",\"params\":[\"on\",\"smooth\",500,3]}\r\n" \
					"{\"id\":" BULB_ID ",\"method\":\"set_hsv\",\"params\":[%u,%u,\"smooth\",500]}\r\n" \
					"{\"id\":" BULB_ID ",\"method\":\"set_bright\",\"params\":[%u,\"smooth\",500]}\r\n"

#define MIN_COLORREF_VAL 30

BOOL bulb_IsColorValid (bulb_Color_T _color)
{
	return _color.brightness > 0 && _color.brightness <= 100
		&& _color.hue >= 0 && _color.hue < 360
		&& _color.saturation >= 0 && _color.saturation <= 100;
}

COLORREF bulb_ToRGB (bulb_Color_T _color)
{
	float      hh, p, q, t, ff;
	int        i;
	float outr, outg, outb;

	float inv = (_color.brightness + MIN_COLORREF_VAL) / (100.0f + MIN_COLORREF_VAL);
	float ins = _color.saturation / 100.0f;

	hh = _color.hue / 60.0f;
	i = (int)hh;
	ff = hh - i;
	p = inv * (1.0f - ins);
	q = inv * (1.0f - (ins * ff));
	t = inv * (1.0f - (ins * (1.0f - ff)));

	switch (i)
	{
		case 0:
			outr = inv;
			outg = t;
			outb = p;
			break;
		case 1:
			outr = q;
			outg = inv;
			outb = p;
			break;
		case 2:
			outr = p;
			outg = inv;
			outb = t;
			break;

		case 3:
			outr = p;
			outg = q;
			outb = inv;
			break;
		case 4:
			outr = t;
			outg = p;
			outb = inv;
			break;
		case 5:
		default:
			outr = inv;
			outg = p;
			outb = q;
			break;
	}
	return RGB (round (outr * 255), round (outg * 255), round (outb * 255));
}

BOOL bulb_Toggle (void)
{
	return send_Data (MSG_TOGGLE);
}

BOOL bulb_Color (bulb_Color_T color)
{
	int len = sizeof (MSG_COLOR) / sizeof (char) + 9;
	char * cmd = HeapAlloc (GetProcessHeap (), HEAP_GENERATE_EXCEPTIONS, len * sizeof (char));
	StringCchPrintfA (cmd, len, MSG_COLOR, color.hue, color.saturation, color.brightness);
	BOOL res = send_Data (cmd);
	HeapFree (GetProcessHeap (), 0, cmd);
	return res;
}
