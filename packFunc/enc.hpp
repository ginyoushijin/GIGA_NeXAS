#pragma once

#include <windows.h>
#include <string>

#undef min
#undef max

static std::wstring AnsiToUnicode(const std::string& source, int codePage)
{
	if (source.length() == 0) {
		return std::wstring();
	}
	if (source.length() > (size_t)std::numeric_limits<int>::max()) {
		return std::wstring();
	}
	int length = MultiByteToWideChar(codePage, 0, source.c_str(), (int)source.length(), NULL, 0);
	if (length <= 0) {
		return std::wstring();
	}
	std::wstring output(length, L'\0');
	if (MultiByteToWideChar(codePage, 0, source.c_str(), (int)source.length(), (LPWSTR)output.data(), (int)output.length() + 1) == 0) {
		return std::wstring();
	}
	return output;
}

static std::string UnicodeToAnsi(const std::wstring& source, int codePage)
{
	if (source.length() == 0) {
		return std::string();
	}
	if (source.length() > (size_t)std::numeric_limits<int>::max()) {
		return std::string();
	}
	int length = WideCharToMultiByte(codePage, 0, source.c_str(), (int)source.length(), NULL, 0, NULL, NULL);
	if (length <= 0) {
		return std::string();
	}
	std::string output(length, '\0');
	if (WideCharToMultiByte(codePage, 0, source.c_str(), (int)source.length(), (LPSTR)output.data(), (int)output.length() + 1, NULL, NULL) == 0) {
		return std::string();
	}
	return output;
}

static std::string AnsiToUTF8(const std::string& source)
{
	if(source.length()==0) return std::string();

	if(source.length()>(size_t)std::numeric_limits<int>::max())  return std::string();

	int utf16Length = MultiByteToWideChar(CP_ACP,0,source.c_str(),(int)source.length(),NULL,0);

	if(utf16Length<=0) return std::string();

	std::wstring utf16String(utf16Length,'\0');

	if(MultiByteToWideChar(CP_ACP,0,source.c_str(),(int)source.length(),(LPWSTR)utf16String.data(),utf16Length+1)==0) return std::string();

	int utf8Length = WideCharToMultiByte(CP_UTF8,0,utf16String.data(),utf16Length,NULL,0,NULL,NULL);

	std::string output(utf8Length,'\0');
	
	if(WideCharToMultiByte(CP_UTF8,0,utf16String.data(),utf16Length,(LPSTR)output.data(),utf8Length+1,NULL,NULL)==0) return std::string();

	return output;

}