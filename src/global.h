#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define PROGRAM_NAME "mbpfan"
#define PROGRAM_VERSION "2.3.0"
#define PROGRAM_PID "/var/run/mbpfan.pid"

extern int daemonize;
extern int verbose;

struct s_sensors {
    FILE *file;
    char *path;
    unsigned int temperature;
    struct s_sensors *next;
};

struct s_fans {
    FILE *file;
    char *path; // TODO: unused
    char *label;
    char *fan_output_path;
    char *fan_manual_path;
    int step_up;
    int step_down;
    int fan_id;
    int old_speed;
    int fan_max_speed;
    int fan_min_speed;
    struct s_fans *next;
};

typedef struct s_sensors t_sensors;
typedef struct s_fans t_fans;

extern t_sensors *sensors;
extern t_fans *fans;

#endif
