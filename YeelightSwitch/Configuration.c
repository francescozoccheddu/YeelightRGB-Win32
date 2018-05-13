#include "Configuration.h"

#include <Windows.h>
#include "resource.h"

#define MAX_LINE_LENGTH 512

typedef struct
{
	TCHAR buf[MAX_LINE_LENGTH * 2];
	int pos;
	int endOfFile;
} Buffer;

void initBuf (Buffer * buffer)
{
	buffer->pos = MAX_LINE_LENGTH;
	buffer->endOfFile = -1;
}

BOOL ensureBufData (Buffer * buffer, HFILE file)
{
	if (buffer->pos == MAX_LINE_LENGTH || buffer->pos == MAX_LINE_LENGTH * 2)
	{
		int start = buffer->pos % (MAX_LINE_LENGTH * 2);
		DWORD count;
		BOOL result = ReadFile (file, buffer->buf + start, sizeof (TCHAR) * MAX_LINE_LENGTH, &count, NULL);
		count /= sizeof (TCHAR);
		if (result)
		{
			buffer->pos = start;
			if (count < MAX_LINE_LENGTH)
			{
				buffer->endOfFile = start + count;
			}
		}
		return result;
	}
}

BOOL hasChar (Buffer * buffer)
{
	return buffer->pos != buffer->endOfFile;
}

BOOL popChar (Buffer * buffer, HFILE file, TCHAR * out)
{
	BOOL result = hasChar(buffer) && ensureBufData (buffer, file);
	if (result)
	{
		*out = buffer->buf[buffer->pos];
		buffer->pos++;
	}
	return result;
}

BOOL loadConfiguration (LPCTSTR _filename, Configuration * _out)
{
	HFILE file = CreateFile (_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (file != INVALID_HANDLE_VALUE)
	{
		TCHAR * bulbId = malloc (sizeof (*_out->bulbId) * 32);
		Buffer buffer;
		initBuf (&buffer);
		int i = 0;
		while (i < 31 && hasChar (&buffer))
		{
			popChar (&buffer, file, bulbId + i);
			i++;
		}
		bulbId[i] = '\0';
		_out->bulbId = bulbId;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
