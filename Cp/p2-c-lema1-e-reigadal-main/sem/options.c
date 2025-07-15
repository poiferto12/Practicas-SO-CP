#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "options.h"

static struct option long_options[] = {
    { .name = "barbers",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'b'},
    { .name = "customers",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 'c'},
    { .name = "cut_time",
      .has_arg = required_argument,
      .flag = NULL,
      .val = 't'},
    { .name = "help",
      .has_arg = no_argument,
      .flag = NULL,
      .val = 'h'},
    {0, 0, 0, 0}
};

static void usage(int i)
{
    printf(
        "Usage: barber [OPTION]\n"
        "Options:\n"
        "  -b n, --barbers=<n>: number of barber threads\n"
        "  -c n, --customers=<n>: number of customer threads\n"
        "  -t n, --cut_time=<n>: time that it takes to cut the hair\n"
        "  -h, --help: this message\n\n"
    );
    exit(i);
}

static int get_int(char *arg, int *value)
{
    char *end;
    *value = strtol(arg, &end, 10);

    return (end != NULL);
}

int handle_options(int argc, char **argv, struct options *opt)
{
    while (1) {
        int c;
        int option_index = 0;

        c = getopt_long (argc, argv, "ht:c:b:",
                 long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 't':
            if (!get_int(optarg, &opt->cut_time)
                || opt->cut_time < 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case 'b':
            if (!get_int(optarg, &opt->barbers)
                || opt->barbers <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case 'c':
            if (!get_int(optarg, &opt->customers)
                || opt->customers <= 0) {
                printf("'%s': is not a valid integer\n",
                       optarg);
                usage(-3);
            }
            break;

        case '?':
        case 'h':
            usage(0);
            break;

        default:
            printf ("?? getopt returned character code 0%o ??\n", c);
            usage(-1);
        }
    }
    return 0;
}

int read_options(int argc, char **argv, struct options *opt) {

    int result = handle_options(argc,argv,opt);

    if (result != 0)
        exit(result);

    if (argc - optind != 0) {
        printf ("Too many arguments\n\n");
        while (optind < argc)
            printf ("'%s' ", argv[optind++]);
        printf ("\n");
        usage(-2);
    }

    return 0;
}
