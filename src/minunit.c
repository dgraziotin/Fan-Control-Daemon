/* file minunit_example.c */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include "global.h"
#include "mbpfan.h"
#include "settings.h"
#include "main.h"
#include "minunit.h"

int tests_run = 0;


static const char *test_sensor_paths()
{
    t_sensors* sensors = retrieve_sensors();
    mu_assert("No sensors found", sensors != NULL);
    t_sensors* tmp = sensors;

    while(tmp != NULL) {
        mu_assert("Sensor does not have a valid path", tmp->path != NULL);

        if (tmp->path != NULL) {
            mu_assert("Sensor does not have valid temperature", tmp->temperature > 0);
        }

        tmp = tmp->next;
    }

    return 0;
}


static const char *test_fan_paths()
{
    t_fans* fans = retrieve_fans();
    mu_assert("No fans found", fans != NULL);
    t_fans* tmp = fans;
    int found_fan_path = 0;

    while(tmp != NULL) {
        if (tmp->fan_output_path != NULL) {
            found_fan_path++;
        }

        tmp = tmp->next;
    }

    mu_assert("No fans found", found_fan_path != 0);
    return 0;
}

unsigned time_seed()
{
    time_t now = time ( 0 );
    unsigned char *p = (unsigned char *)&now;
    unsigned seed = 0;
    size_t i;

    for ( i = 0; i < sizeof now; i++ ) {
        seed = seed * ( UCHAR_MAX + 2U ) + p[i];
    }

    return seed;
}

// nothing better than a horrible piece of code to
// stress a little bit the CPU
int stress(int n)
{
    int f = n;

    while (f > 0) {
        while(n > 0) {
            srand ( time_seed() );
            n--;
        }

        f--;
        n = f;
    }
    return 0;
}

static const char *test_get_temp()
{
    t_sensors* sensors = retrieve_sensors();
    mu_assert("No sensors found", sensors != NULL);
    unsigned short temp_1 = get_temp(sensors);
    mu_assert("Invalid Global Temperature Found", temp_1 > 1 && temp_1 < 150);
    stress(2000);
    unsigned short temp_2 = get_temp(sensors);
    mu_assert("Invalid Higher temp test (if fan was already spinning high, this is not worrying)", temp_1 < temp_2);
    return 0;
}

static const char *test_config_file()
{
    FILE *f = NULL;
    Settings *settings = NULL;
    f = fopen("/etc/mbpfan.conf", "r");
    mu_assert("No config file found", f != NULL);

    if (f == NULL) {
        return 0;
    }

    settings = settings_open(f);
    fclose(f);
    mu_assert("Could not read settings from config file", settings != NULL);

    if (settings == NULL) {
        return 0;
    }

    mu_assert("Could not read low_temp from config file",settings_get_int(settings, "general", "low_temp") != 0);
    mu_assert("Could not read high_temp from config file",settings_get_int(settings, "general", "high_temp") != 0);
    mu_assert("Could not read max_temp from config file",settings_get_int(settings, "general", "max_temp") != 0);
    mu_assert("Could not read polling_interval from config file",settings_get_int(settings, "general", "polling_interval") != 0);
    /* Destroy the settings object */
    settings_delete(settings);

    return 0;
}

static const char *test_settings()
{
    t_fans* fan = (t_fans *) malloc( sizeof( t_fans ) );
    fan->fan_id = 1;
    fan->fan_max_speed = -1; 
    fan->next = NULL;

    retrieve_settings("./mbpfan.conf.test1", fan);
    mu_assert("max_fan_speed value is not 6200", fan->fan_max_speed == 6200);
    mu_assert("polling_interval is not 2", polling_interval == 2);

    fan->fan_min_speed = -1;
    retrieve_settings("./mbpfan.conf.test0", fan);
    mu_assert("min_fan_speed value is not 2000", fan->fan_min_speed == 2000);
    mu_assert("polling_interval is not 7", polling_interval == 7);
    
    t_fans* fan2 = (t_fans *)malloc(sizeof(t_fans));
    fan2->fan_id = 2;
    fan2->fan_max_speed = -1;
    fan->next = fan2;

    retrieve_settings("./mbpfan.conf.test2", fan);
    mu_assert("min_fan1_speed value is not 2000", fan->fan_min_speed == 2000);
    mu_assert("min_fan2_speed value is not 2000", fan->next->fan_min_speed == 2000);

    free(fan2);
    fan->next = NULL;
    free(fan);

    return 0;

}
int received = 0;

static void handler(int signal)
{
    t_fans* fan = (t_fans *) malloc( sizeof( t_fans ) );
    fan->fan_id = 1;
    fan->next = NULL;


    switch(signal) {
    case SIGHUP:
        received = 1;
        retrieve_settings("./mbpfan.conf.test1", fan);
	free(fan);
        break;

    default:
        received = 0;
	free(fan);
        break;
    }
}

static const char *test_sighup_receive()
{
    signal(SIGHUP, handler);
    raise(SIGHUP);
    mu_assert("did not receive SIGHUP signal", received == 1);
    return 0;
}

static const char *test_settings_reload()
{
    t_fans* fan = (t_fans *) malloc( sizeof( t_fans ) );
    fan->fan_id = 1;
    fan->next = NULL;

    signal(SIGHUP, handler);
    retrieve_settings("./mbpfan.conf", fan);
    printf("Testing the _supplied_ mbpfan.conf (not the one you are using)..\n");
    // cannot tests min_fan_speed since it is not set and thus auto-detected
    mu_assert("polling_interval is not 1 before SIGHUP", polling_interval == 1);
    raise(SIGHUP);
    mu_assert("min_fan_speed value is not 6200 after SIGHUP", fan->fan_min_speed == 6200);
    mu_assert("polling_interval is not 2 after SIGHUP", polling_interval == 2);
    retrieve_settings("./mbpfan.conf", fan);
    free(fan);
    return 0;
}


static const char *all_tests()
{
    mu_run_test(test_sensor_paths);
    mu_run_test(test_fan_paths);
    mu_run_test(test_get_temp);
    mu_run_test(test_config_file);
    mu_run_test(test_settings);
    mu_run_test(test_sighup_receive);
    mu_run_test(test_settings_reload);
    return 0;
}

int tests()
{
    check_requirements();

    printf("Starting the tests..\n");
    printf("It is normal for them to take a bit to finish.\n");
    
    const char *result = all_tests();

    if (result != 0) {
        printf("Error: %s \n", result);

    } else {
        printf("ALL TESTS PASSED\n");
    }

    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
