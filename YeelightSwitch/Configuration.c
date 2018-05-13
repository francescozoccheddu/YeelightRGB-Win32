#include "Configuration.h"

#include <stdlib.h>
#include <Windows.h>

#define BUF_SIZE 512

#define INITIAL_VEC_SIZE 8

#define BOM 65279

#define PREFIX_BULB_ID '?'
#define PREFIX_ADDRESS '@'
#define PREFIX_PORT ':'
#define PREFIX_COLOR '#'
#define PREFIX_COLORNAME '"'
#define SUFFIX_COLORNAME '"'

#define MUST(x) {if (!(x)) { return FALSE; }}

typedef struct
{
	TCHAR buf[BUF_SIZE * 2];
	int pos;
	int endOfFile;
	DWORD error;
	int line;
	int column;
	HANDLE file;
} FileReader;

void fillBuffer (FileReader * _fr)
{
	DWORD count;
	BOOL result = ReadFile (_fr->file, _fr->buf, sizeof (TCHAR) * BUF_SIZE, &count, NULL);
	count /= sizeof (TCHAR);
	if (result)
	{
		_fr->pos = 0;
		_fr->endOfFile = count;
	}
	else
	{
		_fr->endOfFile = _fr->pos;
		_fr->error = GetLastError ();
	}
}

void makeFileReader (FileReader * _fr, HANDLE _file)
{
	_fr->pos = 0;
	_fr->endOfFile = 0;
	_fr->error = 0;
	_fr->line = 0;
	_fr->column = 0;
	_fr->file = _file;
	fillBuffer (_fr);
}

BOOL hasChar (const FileReader * _fr)
{
	return _fr->pos != _fr->endOfFile;
}

TCHAR peekChar (const FileReader * _fr)
{
	return _fr->buf[_fr->pos];
}

void skipChar (FileReader * _fr)
{
	if (peekChar (_fr) == '\n')
	{
		_fr->line++;
		_fr->column = 0;
	}
	else
	{
		_fr->column++;
	}
	_fr->pos++;
	if (_fr->pos >= BUF_SIZE)
	{
		fillBuffer (_fr);
	}
}

typedef struct
{
	char * buf;
	int itemSize;
	int size;
	int pos;
} Vector;

Vector makeVector (int _itemSize)
{
	Vector str;
	str.buf = NULL;
	str.itemSize = _itemSize;
	str.size = 0;
	str.pos = 0;
	return str;
}

BOOL resizeVector (Vector * _vec, int _size)
{
	_vec->buf = realloc (_vec->buf, _vec->itemSize * _size);

	if (!_vec->buf && _size != 0)
	{
		return FALSE;
	}

	_vec->size = _size;
	return TRUE;
}

BOOL appendToVector (Vector * _vec, const void * _el)
{
	if (_vec->pos == _vec->size)
	{
		int newSize = _vec->size == 0 ? INITIAL_VEC_SIZE : _vec->size * 2;
		if (!resizeVector (_vec, newSize))
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

LPCTSTR finalizeString (Vector * _str)
{
	if (!resizeVector (_str, _str->pos + 1))
	{
		return NULL;
	}
	TCHAR term = '\0';
	if (!appendToVector (_str, &term))
	{
		return NULL;
	}
	return (const TCHAR *)_str->buf;
}

BOOL isNumeric (TCHAR _ch)
{
	return _ch >= '0' && _ch <= '9';
}

BOOL intCharToInt (TCHAR _ch, int * _out)
{
	if (isNumeric (_ch))
	{
		*_out = _ch - '0';
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL hexCharToInt (TCHAR _ch, int *_out)
{
	if (intCharToInt (_ch, _out))
	{
		return TRUE;
	}
	else
	{
		switch (_ch)
		{
			case 'A':
			case 'a':
				*_out = 10;
				return TRUE;
			case 'B':
			case 'b':
				*_out = 11;
				return TRUE;
			case 'C':
			case 'c':
				*_out = 12;
				return TRUE;
			case 'D':
			case 'd':
				*_out = 13;
				return TRUE;
			case 'E':
			case 'e':
				*_out = 14;
				return TRUE;
			case 'F':
			case 'f':
				*_out = 15;
				return TRUE;
			default:
				return FALSE;
		}
	}
}

BOOL readDecInt (FileReader * _fr, DWORD64 * _out)
{
	DWORD64 val = 0;
	BOOL ok = FALSE;
	while (hasChar (_fr))
	{
		TCHAR ch = peekChar (_fr);
		int dig;
		if (intCharToInt (ch, &dig))
		{
			ok = TRUE;
			val *= 10;
			val += dig;
			skipChar (_fr);
		}
		else
		{
			break;
		}
	}
	if (ok)
	{
		*_out = val;
	}
	return ok;
}

BOOL readHexInt (FileReader * _fr, DWORD64 * _out)
{
	DWORD64 val = 0;
	BOOL ok = FALSE;
	while (hasChar (_fr))
	{
		TCHAR ch = peekChar (_fr);
		int dig;
		if (hexCharToInt (ch, &dig))
		{
			ok = TRUE;
			val <<= 4;
			val |= dig;
			skipChar (_fr);
		}
		else
		{
			break;
		}
	}
	if (ok)
	{
		*_out = val;
	}
	return ok;
}

BOOL readHexOrDecInt (FileReader * _fr, DWORD64 * _out)
{
	if (hasChar (_fr) && peekChar (_fr) == (TCHAR) '0')
	{
		skipChar (_fr);
		if (hasChar (_fr) && peekChar (_fr) == (TCHAR) 'x')
		{
			skipChar (_fr);
			return readHexInt (_fr, _out);
		}
	}
	return readDecInt (_fr, _out);
}

BOOL shouldIgnore (TCHAR _ch)
{
	switch (_ch)
	{
		case BOM:
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL ignoreSpace (FileReader * _fr)
{
	BOOL ignored = FALSE;
	while (hasChar (_fr) && shouldIgnore (peekChar (_fr)))
	{
		ignored = TRUE;
		skipChar (_fr);
	}
	return ignored;
}

BOOL consumeChar (FileReader * _fr, TCHAR _ch)
{
	if (hasChar (_fr) && peekChar (_fr) == _ch)
	{
		skipChar (_fr);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL parseConfiguration (FileReader * _fr, Configuration * _out)
{
	Configuration conf;
	ignoreSpace (_fr);
	{
		MUST (consumeChar (_fr, PREFIX_BULB_ID));
		MUST (readHexOrDecInt (_fr, &conf.bulbId));
	}
	ignoreSpace (_fr);
	{
		MUST (consumeChar (_fr, PREFIX_ADDRESS));
		int fields[4];
		for (int f = 0; f < 4; f++)
		{
			DWORD64 field;
			if (f > 0)
			{
				MUST (consumeChar (_fr, '.'));
			}
			MUST (readDecInt (_fr, &field));
			fields[f] = (int)field;
		}
		MUST (consumeChar (_fr, PREFIX_PORT));
		DWORD64 port;
		MUST (readDecInt (_fr, &port));
	}
	ignoreSpace (_fr);
	{
		int count = 0;
		Vector presets = makeVector (sizeof (Preset));
		while (hasChar (_fr))
		{
			MUST (consumeChar (_fr, PREFIX_COLOR));
			DWORD64 color;
			MUST (readHexInt (_fr, &color));
			ignoreSpace (_fr);
			MUST (consumeChar (_fr, PREFIX_COLORNAME));
			Vector nameBuf = makeVector (sizeof (TCHAR));
			while (hasChar (_fr))
			{
				TCHAR ch = peekChar (_fr);
				if (peekChar (_fr) != SUFFIX_COLORNAME)
				{
					appendToVector (&nameBuf, &ch);
					skipChar (_fr);
				}
				else
				{
					break;
				}
			}
			LPCTSTR name = finalizeString (&nameBuf);
			MUST (consumeChar (_fr, SUFFIX_COLORNAME));
			ignoreSpace (_fr);
			count++;
		}
	}
	return TRUE;
}

BOOL loadConfiguration (LPCTSTR _filename, Configuration * _out)
{
	HANDLE file = CreateFile (_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		FileReader fr;
		makeFileReader (&fr, file);
		parseConfiguration (&fr, _out);
		CloseHandle (file);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
