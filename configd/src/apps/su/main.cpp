// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

/**
 * small utility to use instead of "su" when we want to just
 * switch to the "yahoo" user without any more fuss
 **/

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "missing arguments, usage: run-as-yahoo <cmd> [args ...]");
        exit(1);
    }
    struct passwd *p = getpwnam("yahoo");
    if (p == NULL) {
        perror("FATAL error: user 'yahoo' missing in passwd file");
        exit(1);
    }
    gid_t g = p->pw_gid;
    uid_t u = p->pw_uid;

    if (setgid(g) != 0) {
        perror("FATAL error: could not change group id");
        exit(1);
    }
    size_t listsize = 1;
    gid_t grouplist[1] = { g };
    if (setgroups(listsize, grouplist) != 0) {
        perror("FATAL error: could not setgroups");
        exit(1);
    }
    if (setuid(u) != 0) {
        perror("FATAL error: could not change user id");
        exit(1);
    }
    execvp(argv[1], &argv[1]);
    perror("FATAL error: execvp failed");
    exit(1);
}
