#ifndef __IRC_CL_
#define __IRC_CL_

//
// Programmer:      Matthew Todd Geiger
// Creation Date:   Wed May     8 11:06:30 PST 2019
//
// Filename:        irc_cl.h
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
#include "irc_err.h"
#include "irc_mem.h"
#include "irc_fcntl.h"
#include "irc_types.h"
#include "irc_curl.h"

// Curl includes
#include <curl/curl.h>

// Defines
#define LINK_COUNT 4096

// Compile Options
//#define __THREAD_DEBUG_

// Launch functions
int launch_gig_scrapers(const char *, char **, char **, int, unsigned int);

// Library init/clean functions
void init_mem(void);
void clean_mem(void);

// Parsing functions
int parse_cl_main(const char *, const char *, char **);
int create_curl_dat(const char *, char *, char *);
int gather_gig_links(const char *, char **, char**, unsigned int);
int remove_link_copies(const char *, char **, unsigned int);

// gig functions
void *find_computer_gigs(void *);

// MLP functions
char **mlp_malloc(unsigned int, unsigned int);
void mlp_free(char **, unsigned int);

// Public link strings
const char cl_link_main[] = "https://www.craigslist.org/about/sites";

// Thread data structure
typedef struct thread_curl_data {
    char *link;
    char link_temp[256];
    char prefix[4096];
    char suffix[4096];
} TCD, *pTCD;

void launch_data_miner(const int argc, const char *argv[]) {
    printf("-- Craigslist Console Scraper --\n\n");

    if(getuid() != 0)
        error_exit("Please run as root\n");

    nice(-20);

    // Init memory manager
    init_mem();

    // Create vars
    int line_count = 0;
    int gig_link_count = 0;
    unsigned int thread_count = 0;

    // Loop through arguments
    for(int i = 0; i < argc; i++) {
        if(strstr(argv[i], "--thread-count") != NULL && argv[i + 1][0] != 0) {
            thread_count = atoi(argv[i + 1]);
        }
    }

    // Default value
    if(thread_count == 0)
        thread_count = 4;

    printf("Initializing program...\n");
    printf("Threads: %u\n", thread_count);
#ifdef __THREAD_DEBUG_
    printf("WARNING: THREAD DEBUG ENABLED!\n");
#else
    printf("Thread Debug: DISABLED\n\n");
#endif

    printf("Press enter to start scraper...");
    getchar();
    printf("\n");

    // Scrape main craigslist site to gather all availiable links
    if (curl_web(cl_link_main,
                    (CURL_GET_OPT | CURL_VERB_OPT | CURL_FILE_OPT | CURL_FOLLOW_OPT | CURL_SET_COOKIE_OPT | CURL_COOKIE_OPT),
                    "curl/cl_main.dat", "../html/scrapes/cl_main.html", NULL, NULL) != 0)
    {
        error_exit("Failed to scrape main craigslist link");
    }

    // Allocate mlp for links
    char **link_list = mlp_malloc(LINK_COUNT, 4096);
    char **file_list = mlp_malloc(LINK_COUNT, 4096);

    // Parse for site links
    if((line_count = parse_cl_main("../html/scrapes/cl_main.html", "../dat/links.dat", link_list)) <= 0) {
        error_exit("Failed to parse html file");
    }

    // Launch scraper threads
    if(launch_gig_scrapers("curl/cl_sites.dat", link_list, file_list, line_count, thread_count) != 0) {
        error_exit("Failed to scrape gig links");
    }

    mlp_free(link_list, LINK_COUNT);
    char **gig_list = mlp_malloc(LINK_COUNT, 4096);

    fflush(stdout);

    for(int i = 0; i < line_count; i++) {
        printf("FILE: %s\n", file_list[i]);
        fflush(stdout);
#ifdef __THREAD_DEBUG_
        break;
#endif
    }

    if((gig_link_count = gather_gig_links("../dat/gigs/unrefined_links.dat", file_list, gig_list, line_count)) <= 0) {
        error_exit("Failed to gather links from html file\n");
    }

    printf("\ngig_link_count: %u\n", gig_link_count);

    if((gig_link_count = remove_link_copies("../dat/gigs/refined_links.dat", gig_list, gig_link_count)) <= 0) {
        error_exit("Failed to remove link copies");
    }

    printf("Final Gig Amount: %u\n", gig_link_count);

    // Free list
    mlp_free(file_list, LINK_COUNT);
    mlp_free(gig_list, LINK_COUNT);

    fflush(stdout);

    // Clean up memory
    clean_mem();
    exit(EXIT_SUCCESS);
}

int remove_link_copies(const char *output_file, char **gig_list, unsigned int link_count) {
    if((strstr(output_file, "dat")) == NULL) {
        fprintf(stderr, "Invalid output file name\n");
        return -1;
    }

    // Count links applied to temp list
    unsigned int gig_counter = 0;

    // Create temp buffers
    char **temp_list = mlp_malloc(LINK_COUNT, 4096);
    char *temp_link = (char *)ec_malloc(sizeof(char) * 4096);

    // Open output file
    remove(output_file);
    int output_fd = ec_open(output_file, O_WRONLY | O_CREAT);

    // Main loop
    for(int i = 0; i < link_count; i++) {
        char *ptr = gig_list[i];

        // Cut link into basic string
        if((ptr = strstr(ptr, "/d/")) != NULL) {
            ptr = ptr + strlen("/d/");

            for(int j = 0; j < 4096 && *ptr != '/'; j++) {
                memset(temp_link + j, *ptr, 1);
                ptr++;
            }
        }

        bool found = FALSE;

        // Attempt to locate basic string in temp 
        for(int j = 0; j < link_count; j++) {
            if(strstr(temp_list[j], temp_link) != NULL) {
                found = TRUE;
                break;
            }
        }

        if(found == FALSE) {
            memcpy(temp_list[gig_counter], gig_list[i], strlen(gig_list[i]));
            printf("Final: %s\n", temp_list[gig_counter]);
            fflush(stdout);

            ec_write(output_fd, temp_list[gig_counter], strlen(temp_list[gig_counter]));
            ec_write(output_fd, "\n", 1);
            gig_counter++;
        }

        memset(temp_link, 0, 4096);
    }

    // Clean up
    close(output_fd);
    ec_free(temp_link);
    mlp_free(temp_list, LINK_COUNT);

    return gig_counter;
}

int gather_gig_links(const char *output_file, char **file_list, char **gig_list, unsigned int file_count) {
    // Assign constants
    const char main_index[] = "data-pid=\"";
    const char href_index[] = "href=\"";

    // Define link counter
    unsigned int link_count = 0;

    // Check for valid output name
    if(strstr(output_file, "dat") == NULL) {
        fprintf(stderr, "Invalid output file name\n");
        return -1;
    }

    // Open output file
    remove(output_file);
    int output_fd = ec_open(output_file, O_WRONLY | O_CREAT);

    // Create link buffer
    char *link = (char *)ec_malloc(sizeof(char) * 4096);

    // Loop through all files
    for(int i = 0; i < file_count; i++) {
        // Copy html file to buffer
        int html_fd = ec_open(file_list[i], O_RDONLY);
        char *html_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(html_fd) + 32);
        copy_to_memory(html_fd, html_buffer);
        close(html_fd);

        // Assign memory tracker
        char *ptr = html_buffer;

        // Main html loop
        while((ptr = strstr(ptr, main_index)) != NULL) {
            ptr = ptr + strlen(main_index);

            // Check for valid data-pid
            if(*ptr != '{' && *ptr != '#') {
                // Track to href
                if((ptr = strstr(ptr, href_index)) == NULL)
                    break;

                ptr = ptr + strlen(href_index);

                // Confirm http(s) link
                if(*ptr == 'h' || *ptr == 'H') {
                    // Null buffer
                    memset(link, 0, 4096);
                    
                    // Copy link
                    int i = 0;
                    for(; i < 4096 && *ptr != '\"'; i++) {
                        memset(link + i, *ptr, 1);
                        ptr++;
                    }

                    // Write to output file
                    ec_write(output_fd, link, strlen(link));
                    ec_write(output_fd, "\n", 1);

                    memcpy(gig_list[link_count], link, strlen(link) + 1);

                    printf("GIG: %s\n", gig_list[link_count]);
                    fflush(stdout);

                    link_count++;
                }
            }
        }

       ec_free(html_buffer);
    }

    ec_free(link);
    close(output_fd);

    return link_count;
}

// Function to run main gig scrapers
int launch_gig_scrapers(const char *dat_file, char **link_list, char **file_list, int line_count, unsigned int thread_count) {
    if(thread_count <= 0) {
        fprintf(stderr, "Invalid thread count\n");
        return -1;
    }

    // Allocate memory for dat file prefix and suffix
    char *suf = (char *)ec_malloc(sizeof(char) * 4096);
    char *pre = (char *)ec_malloc(sizeof(char) * 4096);

    // Create prefix and suffix
    create_curl_dat(dat_file, pre, suf);

    // Create thread id and attributes
    pthread_t tids[line_count];
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    // Create thread data structures
    TCD thread_data[line_count];

    printf("line_count: %d\n", line_count);

    // Create threads in groups of 128
    int closed = 0;
    for(int i = 0; i < line_count; i++) {
        // Copy data to thread structure
        memcpy(thread_data[i].prefix, pre, strlen(pre) + 1);
        memcpy(thread_data[i].suffix, suf, strlen(suf) + 1);
        
        // Splice link and copy
        char *ptr_temp = strstr(link_list[i], "/") + 2;

        int j = 0;
        for(; j < 4096 && *ptr_temp != '/'; j++) {
            memset(thread_data[i].link_temp + j, *ptr_temp, 1);
            ptr_temp++;
        }
        memset(thread_data[i].link_temp + j, 0, 1);

        // Complete link
        strcat(link_list[i], "/search/cpg?query=web&is_paid=all");
        thread_data[i].link = link_list[i];

        sprintf(file_list[i], "../html/scrapes/%s.html", thread_data[i].link_temp);

        // Launch thread
        if(pthread_create(&tids[i], &attr, find_computer_gigs, &thread_data[i]) < 0) {
            fprintf(stderr, "Failed to launch thread: %d\n", i);
            return -1;
        }
#ifdef __THREAD_DEBUG_
        break;
#endif

        // Only launch 128 threads at a time
        if(i % thread_count == 0) {
            int ref = i - thread_count;

            for(int h = ref; h < i; h++) {
                pthread_join(tids[i], NULL);
                closed++;
            }
        }
    }

    // Clean up memory
    ec_free(suf);
    ec_free(pre);

    fflush(stdout);

    // Close remaining threads
    for(int i = closed; i < 20; i++) {
        pthread_join(tids[i], NULL);
#ifdef __THREAD_DEBUG_
        break;
#endif
    }

    return 0;
}

// Thread function for computer gig curl
void *find_computer_gigs(void *args) {
    printf("Thread launched\n");
    fflush(stdout);

    // Define link
    pTCD data = (pTCD)args;

    char *dat_buffer = (char *)malloc(sizeof(char) * 4096);
    char *html_file = (char *)malloc(sizeof(char) * 4096);
    char *dat_file = (char *)malloc(sizeof(char) * 4096);

    data->link[strlen(data->link) - 1] = 0;

    sprintf(html_file, "../html/scrapes/%s.html", data->link_temp);
    sprintf(dat_buffer, "%s%s%s", data->prefix, data->link, data->suffix);
    sprintf(dat_file, "curl/%s.dat", data->link_temp);

    remove(html_file);
    remove(dat_file);

    printf("%s\n", dat_file);

    int dat_fd = ec_open(dat_file, O_RDWR | O_CREAT);
    ec_write(dat_fd, dat_buffer, strlen(dat_buffer));
    close(dat_fd);

    // Scrape main craigslist site to gather all availiable links
    while (curl_web(data->link,
                    (CURL_GET_OPT | CURL_VERB_OPT | CURL_FILE_OPT | CURL_FOLLOW_OPT | CURL_SET_COOKIE_OPT | CURL_COOKIE_OPT),
                    dat_file, html_file, NULL, NULL) != 0) {
        printf("Scanning %s\n", data->link);
    }

    free(html_file);
    free(dat_file);
    free(dat_buffer);

    pthread_exit(0);
}

// Create new dat file
int create_curl_dat(const char *dat_file, char *prefix, char *suffix) {
    // Check for valid dat file
    if(strstr(dat_file, "dat") == NULL) {
        fprintf(stderr, "Invalid dat file");
        return -1;
    }

    // Copy dat file to memory
    int fd = ec_open(dat_file, O_RDONLY);
    char *file_buffer = (char *)ec_malloc(sizeof(char) * get_file_length(fd) + 32);
    copy_to_memory(fd, file_buffer);
    close(fd);

    char *ptr = file_buffer;

    // Create prefix
    int i = 0;
    for(; i < 4096 && *ptr != '\''; i++) {
        memset(prefix + i, *ptr, 1);
        ptr++;
    }
    memset(prefix + i, '\'', 1);
    ptr++;

    if((ptr = strstr(ptr, "\'")) == NULL) {
        fprintf(stderr, "Failed to create suffix\n");

        ec_free(prefix);
        ec_free(suffix);
        ec_free(file_buffer);

        return -1;
    }

    // Create suffix
    i = 0;
    for(; i < 4096 && *ptr != 0; i++) {
        memset(suffix + i, *ptr, 1);
        ptr++;
    } 

    // Test output
    printf("%s%s\n", prefix, suffix);

    ec_free(file_buffer);

    return 0;
}

// Allocate multi level ptr
char **mlp_malloc(unsigned int string_count, unsigned int string_size) {
    char **temp = (char **)ec_malloc(sizeof(char *) * string_count);
    for(int i = 0; i <= string_count; i++)
        temp[i] = (char *)ec_malloc(sizeof(char) * string_size);

    return temp;
}

// Free multi level ptr
void mlp_free(char **list, unsigned int size) {
    for(int i = 0; i <= size; i++)
        ec_free(list[i]);

    ec_free((void *)list);
}

// Main craigslist link parser
int parse_cl_main(const char *input_file, const char *output_file, char **list) {
    // Check for html
    if(strstr(input_file, "html") == NULL) {
        fprintf(stderr, "Invalid html input file\n");
        return -1;
    }

    // Open input file
    int input_fd = ec_open(input_file, O_RDONLY);

    // Create buffer for html
    char *input_buffer = (char *)ec_malloc(sizeof(char *) * get_file_length(input_fd) + 32);
    
    // Copy html to buffer
    copy_to_memory(input_fd, input_buffer);

    // Close input file
    close(input_fd);

    // Remove output file
    remove(output_file);

    // Open output file
    int output_fd = ec_open(output_file, O_RDWR | O_CREAT);

    // Create ptr to traverse file
    char *ptr = input_buffer;

    // Find start index
    if((ptr = strstr(ptr, "<div class=\"box box_1\">")) == NULL) {
        fprintf(stderr, "Failed to find start index\n");
        return -1;
    }

    // Create buffer to hold link
    char *link = (char *)ec_malloc(sizeof(char) * 512);

    int line_count = 0;

    // Start main loop
    while((ptr = strstr(ptr, "href=\"")) != NULL) {
        ptr = ptr + strlen("href=\"");

        // Null out buffer
        memset(link, 0, 512);

        // Copy link
        int i = 0;
        for(; i < 512 && *ptr != '\"'; i++) {
            memset(link + i, *ptr, 1);
            ptr++;
        }
        memset(link + i, 0, 1);

        // if link data contains help break loop
        if(strstr(link, "help") != NULL)
            break;

        memcpy(list[line_count], link, i);

        printf("%s\n", list[line_count]);

        // Write link to file
        ec_write(output_fd, link, strlen(link));
        ec_write(output_fd, "\n", 1);

        line_count++;
    }

    // Cleanup
    ec_free(input_buffer);
    ec_free(link);
    close(output_fd);

    return line_count;
}

// Init memory library
void init_mem(void) {
    irc_mem_startup();
}

// Clean memory library
void clean_mem(void) {
    irc_mem_cleanup();
}

#endif