#pragma once

#include <Windows.h>

typedef struct
{
	char * buf;
	int itemSize;
	int size;
	int pos;
} vec_T;

vec_T vec_Make (int _itemSize);

BOOL vec_Resize (vec_T * _vec, int _size);

BOOL vec_Append (vec_T * _vec, const void * _el);

void * vec_Finalize (vec_T * _vec);

LPTSTR vec_FinalizeAsString (vec_T * _str);

void vec_Destroy (vec_T * _vec);
