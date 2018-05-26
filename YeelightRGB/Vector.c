#include "Vector.h"

#include <Windows.h>
#include <stdlib.h>

#define INITIAL_VEC_SIZE 8

vec_T vec_Make (int _itemSize)
{
	vec_T str;
	str.buf = NULL;
	str.itemSize = _itemSize;
	str.size = 0;
	str.pos = 0;
	return str;
}

BOOL vec_Resize (vec_T * _vec, int _size)
{
	_vec->buf = realloc (_vec->buf, _vec->itemSize * _size);

	if (!_vec->buf && _size != 0)
	{
		return FALSE;
	}

	_vec->size = _size;
	return TRUE;
}

BOOL vec_Append (vec_T * _vec, const void * _el)
{
	if (_vec->pos == _vec->size)
	{
		int newSize = _vec->size == 0 ? INITIAL_VEC_SIZE : _vec->size * 2;
		if (!vec_Resize (_vec, newSize))
		{
			return FALSE;
		}
	}
	int dataLeft = _vec->itemSize;
	while (dataLeft-- > 0)
	{
		_vec->buf[_vec->pos * _vec->itemSize + dataLeft] = ((const char *)_el)[dataLeft];
	}
	_vec->pos++;
	return TRUE;
}

void * vec_Finalize (vec_T * _vec)
{
	if (!vec_Resize (_vec, _vec->pos))
	{
		return NULL;
	}
	void * res = _vec->buf;
	*_vec = vec_Make (_vec->itemSize);
	return res;
}

LPTSTR vec_FinalizeAsString (vec_T * _vec)
{
	if (!vec_Resize (_vec, _vec->pos + 1))
	{
		return NULL;
	}
	TCHAR term = '\0';
	if (!vec_Append (_vec, &term))
	{
		return NULL;
	}
	void * res = _vec->buf;
	*_vec = vec_Make (_vec->itemSize);
	return res;
}

void vec_Destroy (vec_T * _vec)
{
	vec_Resize (_vec, 0);
	_vec->pos = 0;
}

void vec_FreeBuf (void * _buf)
{
	free (_buf);
}
