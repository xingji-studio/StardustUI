#include "../includes/file.hpp"

#include <stdio.h>

#ifdef STARDUSTUI_CLEONOS
extern "C" {
#include <cleonos_syscall.h>
}
#endif

#ifdef XJ380
#include "../platforms/xj380/xapi/xtuiapi.h"
#endif

namespace stardustui {
namespace file {

namespace {
int text_length(const char* text)
{
    if (text == nullptr) {
        return 0;
    }

    int length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}
}

bool exists(const char* path)
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }

#ifdef STARDUSTUI_CLEONOS
    return cleonos_sys_fs_stat_type(path) != 0ULL;
#elif defined(XJ380)
    XFILE* file = xapi_OpenFile((char*)path);
    if (file == nullptr) {
        return false;
    }

    xapi_CloseFile(file);
    return true;
#else
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }

    fclose(file);
    return true;
#endif
}

bool exists(const string& path)
{
    return exists(path.c_str());
}

bool remove(const char* path)
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }

#ifdef STARDUSTUI_CLEONOS
    return cleonos_sys_fs_remove(path) != 0ULL;
#elif defined(XJ380)
    return (long long)xapi_DeleteFile((char*)path) >= 0;
#else
    return ::remove(path) == 0;
#endif
}

bool remove(const string& path)
{
    return remove(path.c_str());
}

bool read_bytes(const char* path, vector<byte>& out)
{
    out.clear();

    if (path == nullptr || path[0] == '\0') {
        return false;
    }

#ifdef STARDUSTUI_CLEONOS
    const unsigned long long length = cleonos_sys_fs_stat_size(path);
    if (length == (unsigned long long)-1 || length > 0x7fffffffULL) {
        return false;
    }

    if (length == 0) {
        return true;
    }

    if (!out.reserve((int)length)) {
        return false;
    }

    byte* buffer = new byte[(int)length];
    if (buffer == nullptr) {
        return false;
    }

    const unsigned long long read_len = cleonos_sys_fs_read(path, (char*)buffer, length);
    if (read_len != length) {
        delete[] buffer;
        return false;
    }

    for (unsigned long long index = 0; index < length; ++index) {
        if (!out.push_back(buffer[index])) {
            delete[] buffer;
            out.clear();
            return false;
        }
    }

    delete[] buffer;
    return true;
#elif defined(XJ380)
    XFILE* file = xapi_OpenFile((char*)path);
    if (file == nullptr) {
        return false;
    }

    const unsigned long long length = file->length;
    if (length > 0x7fffffffULL) {
        xapi_CloseFile(file);
        return false;
    }

    if (length == 0) {
        xapi_CloseFile(file);
        return true;
    }

    if (!out.reserve((int)length)) {
        xapi_CloseFile(file);
        return false;
    }

    byte* buffer = (byte*)file->buffer;
    if (buffer == nullptr) {
        xapi_CloseFile(file);
        return false;
    }

    for (unsigned long long index = 0; index < length; ++index) {
        if (!out.push_back(buffer[index])) {
            xapi_CloseFile(file);
            out.clear();
            return false;
        }
    }

    xapi_CloseFile(file);
    return true;
#else
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    long length = ftell(file);
    if (length < 0 || length > 0x7fffffffL) {
        fclose(file);
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    if (length == 0) {
        fclose(file);
        return true;
    }

    if (!out.reserve((int)length)) {
        fclose(file);
        return false;
    }

    for (long index = 0; index < length; ++index) {
        int current = fgetc(file);
        if (current == EOF) {
            fclose(file);
            out.clear();
            return false;
        }

        if (!out.push_back((byte)current)) {
            fclose(file);
            out.clear();
            return false;
        }
    }

    fclose(file);
    return true;
#endif
}

bool read_bytes(const string& path, vector<byte>& out)
{
    return read_bytes(path.c_str(), out);
}

bool read_text(const char* path, string& out)
{
    vector<byte> bytes;
    if (!read_bytes(path, bytes)) {
        out.assign("");
        return false;
    }

    if (bytes.empty()) {
        out.assign("");
        return true;
    }

    char* text = new char[bytes.size() + 1];
    if (text == nullptr) {
        out.assign("");
        return false;
    }

    for (int index = 0; index < bytes.size(); ++index) {
        text[index] = (char)bytes[index];
    }
    text[bytes.size()] = '\0';

    out.assign(text);
    delete[] text;
    return true;
}

bool read_text(const string& path, string& out)
{
    return read_text(path.c_str(), out);
}

bool write_bytes(const char* path, const byte* data, int size)
{
    if (path == nullptr || path[0] == '\0' || size < 0) {
        return false;
    }

    if (size > 0 && data == nullptr) {
        return false;
    }

#ifdef STARDUSTUI_CLEONOS
    const unsigned long long written = cleonos_sys_fs_write(path, (const char*)data, (unsigned long long)size);
    return written == (unsigned long long)size;
#elif defined(XJ380)
    xapi_DeleteFile((char*)path);
    xapi_CreateFile((char*)path);

    if (size == 0) {
        return true;
    }

    return (long long)xapi_WriteFile((char*)path, (char*)data, (unsigned long long)size, 0) >= 0;
#else
    FILE* file = fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    if (size == 0) {
        fclose(file);
        return true;
    }

    const unsigned int written = fwrite(data, 1, (unsigned int)size, file);
    fclose(file);
    return written == (unsigned int)size;
#endif
}

bool write_bytes(const string& path, const byte* data, int size)
{
    return write_bytes(path.c_str(), data, size);
}

bool write_bytes(const char* path, const vector<byte>& data)
{
    if (data.empty()) {
        return write_bytes(path, nullptr, 0);
    }

    const byte* buffer = data.at(0);
    if (buffer == nullptr) {
        return false;
    }

    return write_bytes(path, buffer, data.size());
}

bool write_bytes(const string& path, const vector<byte>& data)
{
    return write_bytes(path.c_str(), data);
}

bool write_text(const char* path, const char* text)
{
    if (text == nullptr) {
        return write_bytes(path, nullptr, 0);
    }

    return write_bytes(path, (const byte*)text, text_length(text));
}

bool write_text(const string& path, const char* text)
{
    return write_text(path.c_str(), text);
}

bool write_text(const char* path, const string& text)
{
    return write_text(path, text.c_str());
}

bool write_text(const string& path, const string& text)
{
    return write_text(path.c_str(), text.c_str());
}

bool append_text(const char* path, const char* text)
{
    if (path == nullptr || path[0] == '\0' || text == nullptr) {
        return false;
    }

    const int length = text_length(text);
    if (length == 0) {
        return true;
    }

#ifdef STARDUSTUI_CLEONOS
    const unsigned long long written = cleonos_sys_fs_append(path, text, (unsigned long long)length);
    return written == (unsigned long long)length;
#elif defined(XJ380)
    XFILE* file = xapi_OpenFile((char*)path);
    unsigned long long offset = 0;
    if (file != nullptr) {
        offset = file->length;
        xapi_CloseFile(file);
    } else {
        xapi_CreateFile((char*)path);
    }

    return (long long)xapi_WriteFile((char*)path, (char*)text, (unsigned long long)length, offset) >= 0;
#else
    FILE* file = fopen(path, "ab");
    if (file == nullptr) {
        return false;
    }

    const unsigned int written = fwrite(text, 1, (unsigned int)length, file);
    fclose(file);
    return written == (unsigned int)length;
#endif
}

bool append_text(const string& path, const char* text)
{
    return append_text(path.c_str(), text);
}

bool append_text(const char* path, const string& text)
{
    return append_text(path, text.c_str());
}

bool append_text(const string& path, const string& text)
{
    return append_text(path.c_str(), text.c_str());
}

}
}
