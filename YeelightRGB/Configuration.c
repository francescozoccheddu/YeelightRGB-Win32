#include "Configuration.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Vector.h"
#include "Send.h"

#define _conf_FR_BUF_SIZE 512

#define _conf_CH_BOM 65279

#define _conf_CH_PREFIX_ADDRESS '@'
#define _conf_CH_PREFIX_PORT ':'
#define _conf_CH_PREFIX_COLOR '#'
#define _conf_CH_PREFIX_COLORNAME '"'
#define _conf_CH_SUFFIX_COLORNAME '"'

#define _conf_MUST(x) {if (!(x)) { return FALSE; }}

typedef struct
{
	TCHAR buf[_conf_FR_BUF_SIZE * 2];
	int pos;
	int endOfFile;
	DWORD error;
	conf_FilePos_T filePos;
	HANDLE file;
} _conf_FileReader_T;

void _conf_FileReader_Fill (_conf_FileReader_T * _fr)
{
	DWORD count;
	BOOL result = ReadFile (_fr->file, _fr->buf, sizeof (TCHAR) * _conf_FR_BUF_SIZE, &count, NULL);
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

void conf_FileReader_Make (_conf_FileReader_T * _fr, HANDLE _file)
{
	_fr->pos = 0;
	_fr->endOfFile = 0;
	_fr->error = 0;
	_fr->filePos.line = 0;
	_fr->filePos.column = 0;
	_fr->file = _file;
	_conf_FileReader_Fill (_fr);
}

BOOL _conf_FileReader_HasChar (const _conf_FileReader_T * _fr)
{
	return _fr->pos != _fr->endOfFile;
}

TCHAR _conf_FileReader_Peek (const _conf_FileReader_T * _fr)
{
	return _fr->buf[_fr->pos];
}

void _conf_FileReader_Next (_conf_FileReader_T * _fr)
{
	if (_conf_FileReader_Peek (_fr) == '\n')
	{
		_fr->filePos.line++;
		_fr->filePos.column = 0;
	}
	else
	{
		_fr->filePos.column++;
	}
	_fr->pos++;
	if (_fr->pos >= _conf_FR_BUF_SIZE)
	{
		_conf_FileReader_Fill (_fr);
	}
}

BOOL _conf_IsChNumeric (TCHAR _ch)
{
	return _ch >= '0' && _ch <= '9';
}

BOOL _conf_DecChToInt (TCHAR _ch, int * _out)
{
	if (_conf_IsChNumeric (_ch))
	{
		*_out = _ch - '0';
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL _conf_HexChToInt (TCHAR _ch, int *_out)
{
	if (_conf_DecChToInt (_ch, _out))
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

BOOL _conf_ParseDecInt (_conf_FileReader_T * _fr, DWORD64 * _out)
{
	DWORD64 val = 0;
	BOOL ok = FALSE;
	while (_conf_FileReader_HasChar (_fr))
	{
		TCHAR ch = _conf_FileReader_Peek (_fr);
		int dig;
		if (_conf_DecChToInt (ch, &dig))
		{
			ok = TRUE;
			val *= 10;
			val += dig;
			_conf_FileReader_Next (_fr);
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

BOOL _conf_ParseHexInt (_conf_FileReader_T * _fr, DWORD64 * _out)
{
	DWORD64 val = 0;
	BOOL ok = FALSE;
	while (_conf_FileReader_HasChar (_fr))
	{
		TCHAR ch = _conf_FileReader_Peek (_fr);
		int dig;
		if (_conf_HexChToInt (ch, &dig))
		{
			ok = TRUE;
			val <<= 4;
			val |= dig;
			_conf_FileReader_Next (_fr);
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

BOOL _conf_ParseHexOrDecInt (_conf_FileReader_T * _fr, DWORD64 * _out)
{
	if (_conf_FileReader_HasChar (_fr) && _conf_FileReader_Peek (_fr) == (TCHAR) '0')
	{
		_conf_FileReader_Next (_fr);
		if (_conf_FileReader_HasChar (_fr) && _conf_FileReader_Peek (_fr) == (TCHAR) 'x')
		{
			_conf_FileReader_Next (_fr);
			return _conf_ParseHexInt (_fr, _out);
		}
	}
	return _conf_ParseDecInt (_fr, _out);
}

BOOL _conf_ch_ShouldIgnore (TCHAR _ch)
{
	switch (_ch)
	{
		case _conf_CH_BOM:
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return TRUE;
		default:
			return FALSE;
	}
}

BOOL _conf_IgnoreSpace (_conf_FileReader_T * _fr)
{
	BOOL ignored = FALSE;
	while (_conf_FileReader_HasChar (_fr) && _conf_ch_ShouldIgnore (_conf_FileReader_Peek (_fr)))
	{
		ignored = TRUE;
		_conf_FileReader_Next (_fr);
	}
	return ignored;
}

BOOL _conf_ConsumeChar (_conf_FileReader_T * _fr, TCHAR _ch)
{
	if (_conf_FileReader_HasChar (_fr) && _conf_FileReader_Peek (_fr) == _ch)
	{
		_conf_FileReader_Next (_fr);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL _conf_Preset_Make (DWORD64 _color, LPTSTR _name, conf_Preset_T * _out)
{
	if (_color & (((1 << 16) - 1) << 16))
	{
		return FALSE;
	}
	_out->color = (COLORREF)_color;
	_out->name = _name;
	return TRUE;
}

BOOL _conf_Parse (_conf_FileReader_T * _fr, conf_T * _out)
{
	conf_T conf;
	_conf_IgnoreSpace (_fr);
	{
		_conf_MUST (_conf_ConsumeChar (_fr, _conf_CH_PREFIX_ADDRESS));
		for (int f = 0; f < 4; f++)
		{
			DWORD64 field;
			if (f > 0)
			{
				_conf_MUST (_conf_ConsumeChar (_fr, '.'));
			}
			_conf_MUST (_conf_ParseDecInt (_fr, &field));
			_conf_MUST (field < 256);
			conf.ipFields[f] = (int)field;
		}
		_conf_MUST (_conf_ConsumeChar (_fr, _conf_CH_PREFIX_PORT));
		DWORD64 port;
		_conf_MUST (_conf_ParseDecInt (_fr, &port));
		conf.port = (int)port;
	}
	_conf_IgnoreSpace (_fr);
	{
		vec_T presetBuf = vec_Make (sizeof (conf_Preset_T));
		while (_conf_FileReader_HasChar (_fr))
		{
			_conf_MUST (_conf_ConsumeChar (_fr, _conf_CH_PREFIX_COLOR));
			DWORD64 color;
			_conf_MUST (_conf_ParseHexInt (_fr, &color));
			_conf_IgnoreSpace (_fr);
			_conf_MUST (_conf_ConsumeChar (_fr, _conf_CH_PREFIX_COLORNAME));
			vec_T nameBuf = vec_Make (sizeof (TCHAR));
			while (_conf_FileReader_HasChar (_fr))
			{
				TCHAR ch = _conf_FileReader_Peek (_fr);
				if (_conf_FileReader_Peek (_fr) != _conf_CH_SUFFIX_COLORNAME)
				{
					vec_Append (&nameBuf, &ch);
					_conf_FileReader_Next (_fr);
				}
				else
				{
					break;
				}
			}
			LPTSTR name = vec_FinalizeAsString (&nameBuf);
			_conf_MUST (_conf_ConsumeChar (_fr, _conf_CH_SUFFIX_COLORNAME));
			_conf_IgnoreSpace (_fr);
			conf_Preset_T preset;
			_conf_MUST (_conf_Preset_Make (color, name, &preset));
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
	HeapFree (GetProcessHeap(), 0, _preset->name);
	_preset->name = NULL;
}

conf_Result_T conf_Load (LPCTSTR _filename, conf_T * _out)
{
	conf_Result_T res;
	HANDLE file = CreateFile (_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		_conf_FileReader_T fr;
		conf_FileReader_Make (&fr, file);
		BOOL parseRes = _conf_Parse (&fr, _out);
		CloseHandle (file);
		if (parseRes)
		{
			res.code = conf_RC_OK;
		}
		else
		{
			if (fr.error)
			{
				res.code = conf_RC_IOERR;
				res.data.ioErr = fr.error;
			}
			else
			{
				res.code = conf_RC_FORMERR;
				res.data.lastFilePos = fr.filePos;
			}
		}
	}
	else
	{
		res.code = conf_RC_IOERR;
		res.data.ioErr = GetLastError ();
	}
	return res;
}

void conf_Destroy (conf_T * _conf)
{
	for (int p = 0; p < _conf->presetCount; p++)
	{
		conf_Preset_Destroy (&_conf->presets[p]);
	}
	vec_FreeBuf (_conf->presets);
	_conf->presets = NULL;
}
