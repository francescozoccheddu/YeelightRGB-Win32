#include "Configuration.h"

#include <stdlib.h>
#include <Windows.h>

#define MAX_LINE_LENGTH 512

#define INITIAL_STRING_SIZE 16

typedef struct
{
	TCHAR buf[MAX_LINE_LENGTH * 2];
	int pos;
	int endOfFile;
} Buffer;

typedef enum
{
	BPR_OK, BPR_EOF, BPR_IOERR
} BufferPopResult;

void initBuf (Buffer * _buffer)
{
	_buffer->pos = MAX_LINE_LENGTH;
	_buffer->endOfFile = -1;
}

BOOL updateBuffer (Buffer * _buffer, HFILE _file)
{
	if (_buffer->pos == MAX_LINE_LENGTH || _buffer->pos == MAX_LINE_LENGTH * 2)
	{
		int start = _buffer->pos % (MAX_LINE_LENGTH * 2);
		DWORD count;
		BOOL result = ReadFile (_file, _buffer->buf + start, sizeof (TCHAR) * MAX_LINE_LENGTH, &count, NULL);
		count /= sizeof (TCHAR);
		if (result)
		{
			_buffer->pos = start;
			if (count < MAX_LINE_LENGTH)
			{
				_buffer->endOfFile = start + count;
			}
		}
		return result;
	}
	else
	{
		return TRUE;
	}
}

BOOL hasChar (const Buffer * _buffer)
{
	return _buffer->pos != _buffer->endOfFile;
}

BufferPopResult popChar (Buffer * _buffer, HFILE _file, TCHAR * _out)
{
	if (!hasChar (_buffer))
	{
		return BPR_EOF;
	}

	if (!updateBuffer (_buffer, _file))
	{
		return BPR_IOERR;
	}

	if (!hasChar (_buffer))
	{
		return BPR_EOF;
	}

	*_out = _buffer->buf[_buffer->pos];
	_buffer->pos++;
	return BPR_OK;
}

typedef struct
{
	TCHAR * buf;
	int size;
	int pos;
} String;

String makeString (void)
{
	String str;
	str.buf = NULL;
	str.size = 0;
	str.pos = 0;
	return str;
}

BOOL resizeString (String * _str, int _size)
{
	_str->buf = realloc (_str->buf, sizeof (TCHAR) * _size);
	
	if (!_str->buf && _size != 0)
	{
		return FALSE;
	}

	_str->size = _size;
	return TRUE;
}

void appendToString (String * _str, TCHAR _ch)
{
	if (_str->pos == _str->size)
	{
		int newSize = _str->size == 0 ? INITIAL_STRING_SIZE : _str->size * 2;
		resizeString (_str, newSize);
	}
	_str->buf[_str->pos++] = _ch;
}

LPCTSTR finalizeString (String * _str)
{
	resizeString (_str, _str->pos + 1);
	_str->buf[_str->pos++] = (TCHAR) '\0';
	return _str->buf;
}

BOOL loadConfiguration (LPCTSTR _filename, Configuration * _out)
{
	HFILE file = CreateFile (_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		Buffer buf;
		initBuf (&buf);
		String str = makeString ();
		while (hasChar (&buf))
		{
			TCHAR ch;
			popChar (&buf, file, &ch);
			appendToString (&str, ch);
		}
		_out->bulbId = finalizeString (&str);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
