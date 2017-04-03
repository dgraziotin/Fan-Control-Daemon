#ifndef _GLOBAL_H_
#define _GLOBAL_H_

extern int daemonize;
extern int verbose;

extern const char* PROGRAM_NAME;
extern const char* PROGRAM_PID;

struct s_step {
    int step_up;
    int step_down;
};
typedef struct s_step t_step;

struct s_fanlimits {
    int max_fan_speed;
    int min_fan_speed;
};
typedef struct s_fanlimits t_fanlimits;

struct s_sensors {
    char* path;
    unsigned int temperature;
    struct s_sensors *next;
};

struct s_fans {
    char* path;
    char* fan_output_path;
    char* fan_manual_path;
    t_fanlimits fanlimits;
    t_step stepinfo;
    int curr_fan_speed;
    struct s_fans *next;
};

typedef struct s_sensors t_sensors;
typedef struct s_fans t_fans;

extern t_sensors* sensors;
extern t_fans* fans;

#endif
