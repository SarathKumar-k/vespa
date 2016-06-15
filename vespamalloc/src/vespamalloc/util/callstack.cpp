// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <dlfcn.h>
#include <ctype.h>
#include <vespamalloc/util/callstack.h>

namespace vespamalloc {

const char * dlAddr(const void * func) {
    static const char * _unknown = "UNKNOWN";
    const char * funcName = _unknown;
    Dl_info info;
    int ret = dladdr(func, &info);
    if (ret != 0) {
        funcName = info.dli_sname;
    }
    return funcName;
}

const void * dlNextSym(const void * func)
{
    const char * f = static_cast<const char *>(func);
    size_t i(0);
    bool done(false);
    for( i = 0; !done; i++) {
        Dl_info info;
        int ret = dladdr(f+i, &info);
        if (ret == 0) {
            fprintf(stderr, "dladdr failed for %p\n", f+i);
        }
        done = (f != info.dli_saddr);
    }
    return f+i;
}

static void verifyAndCopy(const void * addr, char *v, size_t sz)
{
    size_t pos(0);
    const char * sym = dlAddr(addr);
    for (;sym && (sym[pos] != '\0') && (pos < sz-1); pos++) {
        char c(sym[pos]);
        v[pos] = isprint(c) ? c : '.';
    }
    v[pos] = '\0';
}

void StackReturnEntry::info(FILE * os) const
{
    static char tmp[0x400];
    verifyAndCopy(_return, tmp, sizeof(tmp));
    fprintf(os, "%s(%p)", tmp, _return);
}

asciistream & operator << (asciistream & os, const StackReturnEntry & v)
{
    static char tmp[0x100];
    static char t[0x200];
    verifyAndCopy(v._return, tmp, sizeof(tmp));
    snprintf(t, sizeof(t), "%s(%p)", tmp, v._return);
    return os << t;
}

void StackFrameReturnEntry::info(FILE * os) const
{
    static char tmp[0x400];
    verifyAndCopy(_return, tmp, sizeof(tmp));
    fprintf(os, "%s(%p, %p)", tmp, _return, _stack);
}

asciistream & operator << (asciistream & os, const StackFrameReturnEntry & v)
{
    static char tmp[0x100];
    static char t[0x200];
    verifyAndCopy(v._return, tmp, sizeof(tmp));
    snprintf(t, sizeof(t), "%s(%p, %p)", tmp, v._return, v._stack);
    return os << t;
}

}
