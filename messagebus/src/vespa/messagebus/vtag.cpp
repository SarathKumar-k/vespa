// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <string.h>
#include <stdio.h>
#include "vtag.h"
#include <vespa/vespalib/component/version.h>

#ifndef V_TAG
#define V_TAG "NOTAG"
#define V_TAG_DATE "NOTAG"
#define V_TAG_SYSTEM "NOTAG"
#define V_TAG_SYSTEM_REV "NOTAG"
#define V_TAG_BUILDER "NOTAG"
#define V_TAG_VERSION "0"
#define V_TAG_ARCH "NOTAG"
#endif

namespace mbus {

char VersionTag[] = V_TAG;
char VersionTagDate[] = V_TAG_DATE;
char VersionTagSystem[] = V_TAG_SYSTEM;
char VersionTagSystemRev[] = V_TAG_SYSTEM_REV;
char VersionTagBuilder[] = V_TAG_BUILDER;
char VersionTagPkg[] = V_TAG_PKG;
char VersionTagComponent[] = V_TAG_COMPONENT;
char VersionTagArch[] = V_TAG_ARCH;

vespalib::Version Vtag::currentVersion(VersionTagComponent);

void
Vtag::printVersionNice()
{
    char *s = VersionTag;
    bool needdate = true;
    if (strncmp(VersionTag, "V_", 2) == 0) {
        s += 2;
        do {
            while (strchr("0123456789", *s) != NULL) {
                printf("%c", *s++);
            }
            if (strncmp(s, "_RELEASE", 8) == 0) {
                needdate = false;
                break;
            }
            if (strncmp(s, "_RC", 3) == 0) {
                char *e = strchr(s, '-');
                if (e == NULL) {
                    printf("%s", s);
                } else {
                    printf("%.*s", (int)(e-s), s);
                }
                needdate = false;
                break;
            }
            if (*s == '_' && strchr("0123456789", *++s)) {
                printf(".");
            } else {
                break;
            }
        } while (*s && *s != '-');
    } else {
        char *e = strchr(s, '-');
        if (e == NULL) {
            printf("%s", s);
        } else {
            printf("%.*s", (int)(e-s), s);
        }
    }
    if (needdate) {
        s = VersionTagDate;
        char *e = strchr(s, '-');
        if (e == NULL) {
            printf("-%s", s);
        } else {
            printf("-%.*s", (int)(e-s), s);
        }
    }
}

} // namespace mbus
