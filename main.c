//
// Programmer:      Matthew Todd Geiger
// Creation Date:   Wed May     8 11:06:30 PST 2019
//
// Filename:        main.c
// Syntax:          C; CURL
// Make:            gcc main.c -Wall -lcrypt -lcurl -lpthread -o main

/*
 *      Client Generation Tool For Freelancers
 */


// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

// lib includes
#include "lib/irc_err.h"
#include "lib/irc_mem.h"
#include "lib/irc_fcntl.h"
#include "lib/irc_types.h"
#include "lib/irc_curl.h"
#include "lib/irc_cl.h"

// Curl includes
#include <curl/curl.h>

int main(const int argc, const char *argv[]) {
    launch_data_miner(argc, argv);

    exit(EXIT_SUCCESS);
}