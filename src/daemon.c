/**
 *  Copyright (C) 2012  Peter Lombardo <http://peterlombardo.wikidot.com/linux-daemon-in-c>
 *  Modifications (2012) by Ismail Khatib <ikhatib@gmail.com>
 *  Modifications (2012-present) by Daniel Graziotin <daniel@ineed.coffee>
 *  Modifications (2017-present) by Robert Musial <rmmm@rmmm.info>
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


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "mbpfan.h"
#include "global.h"
#include "daemon.h"

int write_pid(int pid)
{
    FILE *file = NULL;
    file = fopen(PROGRAM_PID, "w");

    if(file != NULL) {
        fprintf(file, "%d", pid);
        fclose(file);
        return 1;

    } else {
        return 0;
    }
}

int read_pid()
{
    FILE *file = NULL;
    int pid = -1;
    file = fopen(PROGRAM_PID, "r");

    if(file != NULL) {
        fscanf(file, "%d", &pid);
        fclose(file);
        if (kill(pid, 0) == -1 && errno == ESRCH)
        { /* a process with such a pid does not exist, remove the pid file */
          if (remove(PROGRAM_PID) ==  0) {
            return -1;
          }
        }
        return pid;
    }

    return -1;
}

int delete_pid()
{
    return remove(PROGRAM_PID);
}

static void cleanup_and_exit(int exit_code)
{
	delete_pid();
	set_fans_auto(fans);
	
	struct s_fans *next_fan;
	while (fans != NULL) {
		next_fan = fans->next;
		free(fans->fan_output_path);
		free(fans->fan_manual_path);
		free(fans);
		fans = next_fan;
	}

	struct s_sensors *next_sensor;
	while (sensors != NULL) {
		next_sensor = sensors->next;
		free(sensors->path);
		free(sensors);
		sensors = next_sensor;
	}

	exit(exit_code);
}

void signal_handler(int signal)
{

    switch(signal) {
    case SIGHUP:
        syslog(LOG_WARNING, "Received SIGHUP signal.");
        retrieve_settings(NULL);
        break;

    case SIGTERM:
        syslog(LOG_WARNING, "Received SIGTERM signal.");
        cleanup_and_exit(EXIT_SUCCESS);

    case SIGINT:
        syslog(LOG_WARNING, "Received SIGINT signal.");
        cleanup_and_exit(EXIT_SUCCESS);

    case SIGSTOP:
        syslog(LOG_WARNING, "Received SIGSTOP signal.");
        cleanup_and_exit(EXIT_SUCCESS);

    default:
        syslog(LOG_WARNING, "Unhandled signal (%d) %s", signal, strsignal(signal));
        break;
    }
}

void go_daemon(void (*fan_control)())
{

    // Setup signal handling before we start
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGSTOP, signal_handler);

    syslog(LOG_INFO, "%s starting up", PROGRAM_NAME);

    // Setup syslog logging - see SETLOGMASK(3)
    if(verbose) {
        setlogmask(LOG_UPTO(LOG_DEBUG));
        openlog(PROGRAM_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

    } else {
        setlogmask(LOG_UPTO(LOG_INFO));
        openlog(PROGRAM_NAME, LOG_CONS, LOG_USER);
    }


    pid_t pid_slave;
    pid_t sid_slave;

    if (daemonize) {

        pid_slave = fork();

        if (pid_slave < 0) {
            exit(EXIT_FAILURE);
        }

        if (pid_slave > 0) {
            signal(SIGCHLD, SIG_IGN);
            // kill the father
            exit(EXIT_SUCCESS);
        }

        umask(0022);

        // new sid_slave for the child process
        sid_slave = setsid();

        if (sid_slave < 0) {
            exit(EXIT_FAILURE);
        }

        if ((chdir("/")) < 0) {
            exit(EXIT_FAILURE);
        }



        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }


    int current_pid = getpid();

    if (read_pid() == -1) {
        if (verbose) {
            printf("Writing a new .pid file with value %d at: %s\n", current_pid, PROGRAM_PID);
            syslog(LOG_INFO, "Writing a new .pid file with value %d at: %s", current_pid, PROGRAM_PID);
        }

        if (write_pid(current_pid) == 0) {
            syslog(LOG_ERR, "Can not create a .pid file at: %s. Aborting", PROGRAM_PID);

            if (verbose) {
                printf("ERROR: Can not create a .pid file at: %s. Aborting\n", PROGRAM_PID);
            }

            exit(EXIT_FAILURE);

        } else {
            if (verbose) {
                printf("Successfully written a new .pid file with value %d at: %s\n", current_pid, PROGRAM_PID);
                syslog(LOG_INFO, "Successfully written a new .pid file with value %d at: %s", current_pid, PROGRAM_PID);
            }
        }

    } else {
        syslog(LOG_ERR, "A previously created .pid file exists at: %s. Aborting", PROGRAM_PID);

        if (verbose) {
            printf("ERROR: a previously created .pid file exists at: %s.\n Aborting\n", PROGRAM_PID);
        }

        exit(EXIT_FAILURE);
    }


    fan_control();

    if(daemonize) {
        syslog(LOG_INFO, "%s daemon exiting", PROGRAM_NAME);
    }

    return;
}
