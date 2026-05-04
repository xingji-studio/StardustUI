#pragma once
#include "string.hpp"
#include "vector.hpp"

#ifdef  XJ380
#include "../platforms/xj380.hpp"
#endif
#ifdef STARDUSTUI_WINDOWS
#include "../platforms/windows.hpp"
#endif
#ifdef STARDUSTUI_LINUX
#include "../platforms/linux.hpp"
#endif

namespace stardustui {
namespace file {

using byte = unsigned char;

bool exists(const char* path);
bool exists(const string& path);

bool remove(const char* path);
bool remove(const string& path);

bool read_bytes(const char* path, vector<byte>& out);
bool read_bytes(const string& path, vector<byte>& out);

bool read_text(const char* path, string& out);
bool read_text(const string& path, string& out);

bool write_bytes(const char* path, const byte* data, int size);
bool write_bytes(const string& path, const byte* data, int size);
bool write_bytes(const char* path, const vector<byte>& data);
bool write_bytes(const string& path, const vector<byte>& data);

bool write_text(const char* path, const char* text);
bool write_text(const string& path, const char* text);
bool write_text(const char* path, const string& text);
bool write_text(const string& path, const string& text);

bool append_text(const char* path, const char* text);
bool append_text(const string& path, const char* text);
bool append_text(const char* path, const string& text);
bool append_text(const string& path, const string& text);

}
}
