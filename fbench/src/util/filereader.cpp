// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include "filereader.h"
#include <iostream>
#include <unistd.h>

int GetOpt (int argc, char *argv[], const char *optionsString,
            const char* &optionArgument,
            int &optionIndex)
{
    optind = optionIndex;

    int rc = getopt(argc, argv, optionsString);
    optionArgument = optarg;
    optionIndex = optind;
    return rc;
}

FileReader::FileReader()
    : _backing(),
      _file(&std::cin),
      _bufsize(1024*1024),
      _buf(_bufsize),
      _bufused(0),
      _bufpos(0)
{
}

FileReader::~FileReader()
{
}

bool
FileReader::Open(const char *filename)
{
    _backing = std::make_unique<std::ifstream>(filename);
    _file = _backing.get();
    return (bool)*_file;
}

bool
FileReader::OpenStdin()
{
    _file = &std::cin;
    return true;
}

bool
FileReader::Reset()
{
    _file->clear();
    _file->seekg(0);
    return bool(*_file);
}

bool
FileReader::SetFilePos(int64_t pos)
{
    _bufpos = 0;
    _file->seekg(pos);
    return bool(*_file);
}

int64_t
FileReader::GetFileSize()
{
    _file->seekg (0, std::ifstream::end);
    return _file->tellg();
}

uint64_t
FileReader::FindNewline(int64_t pos)
{
    char buf[100];
    SetFilePos(pos);
    ssize_t len = ReadLine(buf, 100);
    ++_bufpos;

    return pos+len;
}

void
FileReader::FillBuffer()
{
    _file->read(&_buf[0], _bufsize);
    _bufused = _file->gcount(); // may be -1
    _bufpos  = 0;
}

ssize_t
FileReader::ReadLine(char *buf, size_t bufsize)
{
    int    c;
    size_t len;

    len = 0;
    c = ReadByte();
    if (c == -1)
        return -1;
    while (c != -1 && c != '\n' && c != '\r') {
        if (len < bufsize - 1)
            buf[len] = c;
        len++;
        c = ReadByte();
    }
    if (_bufpos == _bufused)
        FillBuffer();
    if ((_bufused > _bufpos) &&
        ((c == '\n' && _buf[_bufpos] == '\r') ||
         (c == '\r' && _buf[_bufpos] == '\n')))
        _bufpos++;
    if (len < bufsize)
        buf[len] = '\0';         // terminate string
    else
        buf[bufsize - 1] = '\0'; // terminate string
    return len;
}

void
FileReader::Close()
{
    if (_backing) {
        _backing->close();
    }
}
