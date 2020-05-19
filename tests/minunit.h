/**
 * This is the MinUnit testing framework - http://www.jera.com/techinfo/jtns/jtn002.html
 */

#ifndef _MINUNIT_H_
#define _MINUNIT_H_

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { const char *message = test(); tests_run++; \
                                if (message) return message; } while (0)

extern int tests_run;


static const char *test_sensor_paths();
static const char *test_fan_paths();
static const char *test_get_temp();
static const char *test_config_file();
static const char *test_settings();
static void handler(int signal);
static const char *test_sighup_receive();
static const char *test_settings_reload();
static const char *all_tests();

int tests();

#endif