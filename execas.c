/*
 * Copyright (c)  2020  Fabrice Triboix
 *
 * Licensed under the MIT License
 */

#include <stdio.h>      // printf(3)
#include <stdlib.h>     // exit(3), malloc(3), free(3)
#include <assert.h>     // assert(3)
#include <sys/types.h>  // setuid(2)
#include <unistd.h>     // getopt(3), setuid(2), setgid(2), execve(2), access(2)
#include <errno.h>      // errno
#include <string.h>     // strerror(3), strcmp(3), strdup(3)
#include <sys/types.h>  // getpwent(3), getgrent(3)
#include <pwd.h>        // getpwent(3)
#include <grp.h>        // getgrent(3)

extern char** environ;


void usage(int exit_code)
{
    fprintf(stderr, "Usage: execas [-h] [-v] [-u USER] [-g GROUP] PRG {ARG}\n");
    fprintf(stderr, "  -h         Print this usage message\n");
    fprintf(stderr, "  -v         Print debug messages\n");
    fprintf(stderr, "  -u USER    User name or id to run PRG as\n");
    fprintf(stderr, "  -g GROUP   Group name of id to run PRG as\n");
    fprintf(stderr, "  PRG {ARG}  Program to run (and its arguments)\n");
    exit(exit_code);
}


int lookup_user(const char* user)
{
    const struct passwd* ent = getpwent();
    while (ent != NULL) {
        if (strcmp(ent->pw_name, user) == 0) {
            return (int)ent->pw_uid;
        }
        ent = getpwent();
    }
    endpwent();
    fprintf(stderr, "ERROR: User '%s' doesn't exist on this system", user);
    exit(3);
}


int lookup_group(const char* group)
{
    const struct group* ent = getgrent();
    while (ent != NULL) {
        if (strcmp(ent->gr_name, group) == 0) {
            return (int)ent->gr_gid;
        }
        ent = getgrent();
    }
    endgrent();
    fprintf(stderr, "ERROR: Group '%s' doesn't exist on this system", group);
    exit(3);
}


char* find_program(const char* prg)
{
    const char* path_env = getenv("PATH");
    if (path_env == NULL) {
        path_env = "/sbin:/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin";
    }
    char* path = strdup(path_env);

    char* token = strtok(path, ":");
    assert(token != NULL);
    while (token != NULL) {
        char* prg_path = (char*)malloc(strlen(token) + strlen(prg) + 2);
        sprintf(prg_path, "%s/%s", token, prg);
        if (access(prg_path, R_OK | X_OK) == 0) {
            free(path);
            return prg_path;
        }
        token = strtok(NULL, ":");
    }

    fprintf(stderr, "ERROR: Program `%s` not found in PATH: %s\n", prg, path_env);
    free(path);
    exit(9);
}


int main(int argc, char** argv)
{
    /* Parse arguments */

    const char* user = NULL;
    const char* group = NULL;
    int verbose = 0;
    int opt;

    while ((opt = getopt(argc, argv, "hvu:g:")) != -1) {
        switch ((char)opt) {
        case 'h':
            usage(0);
            break;

        case 'v':
            verbose = 1;
            break;

        case 'u':
            user = optarg;
            break;

        case 'g':
            group = optarg;
            break;

        default:
            usage(1);
            break;
        }
    }
    if (optind >= argc) {
        usage(1);
    }

    if ((geteuid() != 0) && verbose) {
        fprintf(stderr, "WARNING: You are not root, this will probably fail...\n");
    }

    /* Look up user and group */

    int uid = -1;
    if (user != NULL) {
        if (sscanf(user, "%u", &uid) != 1) {
            uid = lookup_user(user);
        } else if (uid < 0) {
            fprintf(stderr, "ERROR: User id can't be negative: %d\n", uid);
            exit(3);
        }
    }

    int gid = -1;
    if (group != NULL) {
        if (sscanf(group, "%u", &gid) != 1) {
            gid = lookup_group(group);
        } else if (gid < 0) {
            fprintf(stderr, "ERROR: Group id can't be negative: %d\n", gid);
            exit(3);
        }
    }

    if (verbose) {
        if (uid > 0) {
            fprintf(stderr, "User: %d\n", uid);
        }
        if (gid > 0) {
            fprintf(stderr, "Group: %d\n", gid);
        }
        fprintf(stderr, "Program: ");
        int i;
        for (i = optind; i < argc; i++) {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
    }

    /* Change uid and gid */

    if (gid >= 0) {
        if (setgid((gid_t)gid) < 0) {
            fprintf(stderr, "ERROR: Failed to change to group %d: [%d] %s\n",
                    gid, errno, strerror(errno));
            exit(7);
        }
    }

    if (uid >= 0) {
        if (setuid((uid_t)uid) < 0) {
            fprintf(stderr, "ERROR: Failed to change to user %d: [%d] %s\n",
                    uid, errno, strerror(errno));
            exit(7);
        }
    }

    /* Execute the command */

    char* prg;
    if (strchr(argv[optind], '/') != NULL) {
        // Absolute or relative path given
        prg = strdup(argv[optind]);
    } else {
        // Only program name given => Find it in the PATH
        prg = find_program(argv[optind]);
    }

    if (verbose) {
        fprintf(stderr, "Executing: %s ", prg);
        for (int i = optind + 1; i < argc; i++) {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
    }
    execve(prg, argv+optind, environ);
    fprintf(stderr, "ERROR: Failed to execute command `%s`: [%d] %s\n",
            argv[optind], errno, strerror(errno));
    exit(9);
}
