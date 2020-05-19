/**
 *  Copyright (C) (2012-present) Daniel Graziotin <daniel@ineed.coffee>
 *  Modifications (2017-present) by Robert Musial <rmusial@fastmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

/**
 * Code formatted with astyle -A3 -s --break-blocks=all --add-brackets *.c *.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>
#include <errno.h>
#include "mbpfan.h"
#include "daemon.h"
#include "global.h"
#include "util.h"

void print_usage(int argc, char *argv[])
{
    if (argc >=1) {
        printf("Usage: %s OPTION(S) \n", argv[0]);
        printf("Options:\n");
        printf("\t-h Show this help screen\n");
        printf("\t-f Run in foreground\n");
        printf("\t-v Be (a lot) verbose\n");
        printf("\n");
    }
}

int main(int argc, char *argv[])
{

    int c;

    while( (c = getopt(argc, argv, "hfv|help")) != -1) {
        switch(c) {
        case 'h':
            print_usage(argc, argv);
            exit(EXIT_SUCCESS);
            break;

        case 'f':
            daemonize = 0;
            break;

        case 'v':
            verbose = 1;
            break;

        default:
            print_usage(argc, argv);
            exit(EXIT_SUCCESS);
            break;
        }
    }

    check_requirements(argv[0]);

    // pointer to mbpfan() function in mbpfan.c
    void (*fan_control)() = mbpfan;
    go_daemon(fan_control);
    exit(EXIT_SUCCESS);
}
