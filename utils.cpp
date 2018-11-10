#include "stdafx.h"
#include "utils.h"

#define NOMINMAX
#include <windows.h>

void parseArg(const char *arg, std::string &key, std::string &value)
{
	const char *argTop = arg;
	if(*arg == '-' || *arg == '/')
	{
		if (*arg == '-' && arg[1] == '-')
			arg += 2;
		else
			arg++;

		const char *keyTop = arg;
		while(*arg != '=' && *arg != ':' && *arg != '\0') arg++; 
		if(keyTop == arg) keyTop = argTop;
		key.assign(keyTop, arg - keyTop);
		transform(key.begin(), key.end(), key.begin(), toupper); 

		if(*arg == '=' || *arg == ':') arg++;
	}else{
		key = "";
	}
	value = arg;
}

std::string changeFileExt(const std::string path, const std::string ext)
{
	std::string::size_type dirpos = path.find_last_of("\\/");
	if(dirpos == std::string::npos) dirpos = 0;

	std::string::size_type pos = path.rfind('.');
	if(pos == std::string::npos || pos < dirpos)
		return path + ext;
	else
		return path.substr(0, pos) + ext;
}

void fileList(std::vector<std::string> &entries, const std::string mask)
{
	std::string::size_type dirlen = mask.find_last_of("\\/");
	std::string dir = (dirlen != std::string::npos)? mask.substr(0, dirlen + 1) : "";

	WIN32_FIND_DATA fd;
	HANDLE h;
	h = FindFirstFile(mask.c_str(), &fd);
	if(h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				entries.push_back(dir + &fd.cFileName[0]);
			}
		}while(FindNextFile(h, &fd));
		FindClose(h);
	}
}


void showErrorDialog(const char *message, const char* caption)
{
	MessageBox(0, message, caption, MB_OK | MB_APPLMODAL | MB_ICONERROR);
};
