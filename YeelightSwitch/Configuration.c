#include "Configuration.h"

#include <stdlib.h>
#include <Windows.h>

#include "Vector.h"

#define BUF_SIZE 512

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

BOOL makePreset (DWORD64 _color, LPTSTR _name, conf_Preset_T * _out)
{
	if (_color & (((1 << 16) - 1) << 16))
	{
		return FALSE;
	}
	_out->color = (COLORREF)_color;
	_out->name = _name;
	return TRUE;
}

BOOL parseConfiguration (FileReader * _fr, conf_T * _out)
{
	conf_T conf;
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
		// TODO make address
	}
	ignoreSpace (_fr);
	{
		vec_T presetBuf = vec_Make (sizeof (conf_Preset_T));
		while (hasChar (_fr))
		{
			MUST (consumeChar (_fr, PREFIX_COLOR));
			DWORD64 color;
			MUST (readHexInt (_fr, &color));
			ignoreSpace (_fr);
			MUST (consumeChar (_fr, PREFIX_COLORNAME));
			vec_T nameBuf = vec_Make (sizeof (TCHAR));
			while (hasChar (_fr))
			{
				TCHAR ch = peekChar (_fr);
				if (peekChar (_fr) != SUFFIX_COLORNAME)
				{
					vec_Append (&nameBuf, &ch);
					skipChar (_fr);
				}
				else
				{
					break;
				}
			}
			LPTSTR name = vec_FinalizeAsString (&nameBuf);
			MUST (consumeChar (_fr, SUFFIX_COLORNAME));
			ignoreSpace (_fr);
			conf_Preset_T preset;
			MUST (makePreset (color, name, &preset));
			vec_Append (&presetBuf, &preset);
		}
		conf.presetCount = presetBuf.pos;
		conf.presets = vec_Finalize (&presetBuf);
	}
	*_out = conf;
	return TRUE;
}

void conf_Preset_Destroy (conf_Preset_T * _preset)
{
	free (_preset->name);
	_preset->name = NULL;
}

BOOL conf_Load (LPCTSTR _filename, conf_T * _out)
{
	HANDLE file = CreateFile (_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		FileReader fr;
		makeFileReader (&fr, file);
		BOOL res = parseConfiguration (&fr, _out);
		CloseHandle (file);
		return res;
	}
	else
	{
		return FALSE;
	}
}

void conf_Destroy (conf_T * _conf)
{
	// TODO free address
	for (int p = 0; p < _conf->presetCount; p++)
	{
		conf_Preset_Destroy (&_conf->presets[p]);
	}
	free (_conf->presets);
	_conf->presets = NULL;
}
