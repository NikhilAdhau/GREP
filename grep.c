/****************************GREP****************************/
/*
 *  Author : Nikhil Ambadas Adhau
 *  MIS : 111803035
*/

#define _GNU_SOURCE 1       /* for getline() */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h> 
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>


#define SIZE 100
#define BUFSIZE  512 

char *myname;       /* for error messages */
int igrep = 0;      /* -i option: ignore case */
int vgrep = 0;      /* -v option: print non-matching lines */
int cgrep = 0;      /* -c option: print the number of line */
int wgrep = 0;      /* -w option: print line with matching word */
int hgrep = 0;      /* -h option: doesn't print the filename */
int Hgrep = 0;      /* -H option: prints filename for any no of files */
int mgrep = 0;      /* -m option: prints only num matching / non - matching lines */
int fgrep = 0;      /* -f option: takes input from file */
int egrep = 0;      /* -e option: takes multiple inputes */
int bgrep = 0;      /* -b option: prints offset */
int qgrep = 0;      /* -q option: it exits the program when a match is found*/
int rgrep = 0;      /* -r option: it searches pattern in directory */
int errors = 0;     /* number of errors */
int arg_error = 0;  /* error flag for options with argument */
int no_files = 0;   /* number of files to be searched */

regex_t pattern;            /* pattern to match */
regmatch_t match[100];      /* to store the offset info of matched pattern */
int no_match;               /* to store the number of matches in a line */
int ccount = 0;             /* to store number of lines (matched / unmatched) */
int prev_length = 0;        /* to store the lengh of line */
long long int mcount = 0;   /* for -m option */
long long int mcount1 = 0;
char *ffiles[100];
char *efiles[100];

/* compiles the pattern */
void compile_pattern(char *pat);

/* searches the compiled pattern  */                      
void process(char *name, FILE *fp);

/* handles the multiple pattern given by -f and -e option */
char* multi_patterns (char *arg_vector);

/* processes files to be searched given in the command line argument */
int process_files (char **arg_vector, int arg_count);

/* processes directories when -r option is used  */
void process_dir (char *dir_name);

/* prints output */
void print_line (char *file_name, char * line);

/* prints error message */
void usage(void);


/* main --- process options */

int main(int argc, char **argv) {
    int c;
    int x;
    char *eptr, *endptr;
    myname = argv[0];
    while ((c = getopt(argc, argv, ":wivchHbqf:e:rm:")) != -1) {
        switch (c) {
        case 'i':
            igrep = 1;
            break;
        case 'v':
            vgrep = 1;
            break;
        case 'c':
            cgrep = 1;
            break;
        case 'w':
            wgrep = 1;
            break;
        case 'f':
            fgrep++;
            ffiles[fgrep - 1] = (char *) malloc (strlen (optarg)); // storing f arguments in a array
            strcpy (ffiles[fgrep - 1], optarg);
            break;    
        case 'e':
            egrep++;
            efiles[egrep - 1] = (char *) malloc (strlen (optarg)); // storing e arguments in a array
            strcpy (efiles[egrep - 1], optarg);
            break;   
        case 'b':
            bgrep = 1;
            break;
        case 'm':
            mgrep = 1;
            mcount = strtoll (optarg, &eptr, 10);
            endptr = &optarg [strlen (optarg)];
            if ((errno == ERANGE && (mcount == LONG_MAX || mcount == LONG_MIN))
                   || (errno != 0 && mcount == 0)) {
               perror("strtol");
               exit(EXIT_FAILURE);
            }
            if (endptr != eptr) {
               fprintf(stderr, "%s: Invalid max count\n", myname);
               exit (1); 
            }
            mcount1 = mcount;
            break;
        case 'r':
            rgrep = 1;
            break;
        case 'q':
            qgrep = 1;
            break;    
        case 'h':
            hgrep = 1;
            if (Hgrep)
                Hgrep = 0;
            break;
        case 'H':
            Hgrep = 1;
            if (hgrep)
                hgrep = 0;
            break;                          
        case '?':
            usage();
            break;
        case ':':
            arg_error = 1;
            usage();
            break;
        }
    }

    if (optind == argc && !fgrep)      /* sanity check */
        usage();

    if (!fgrep && !egrep) {
        compile_pattern(argv[optind]);  /* compile the pattern */
        if (errors)          //compile failed 
            return 1;
        else
            optind++;
    }
    else {
        compile_pattern(multi_patterns (argv[0]));  /* compile the pattern */
        if (errors)          //compile failed 
            return 1;
    }
    x = process_files (argv, argc);
    if (x) 
        return 0;
}

/* multi_patterns ---- handles multiple patterns */

char* multi_patterns (char *arg_vector) {
    int i;
    FILE *fp;
    char *line;
    char *rm;
    size_t size;
    int len = 0;
    int len1 = 0;
    char *store = NULL;     //to store multiple strings for -f and -e option
    int width = 1000;
    store = (char *) malloc (width);
    if (!store)
        exit (1);
    strcpy (store, "(");
    if (fgrep) {
        for (i = 0; i < fgrep; i++) { 
            if ((fp = fopen(ffiles[i], "r+")) != NULL) {
                while (getline(&line, &size, fp) != -1) {
                    len1 = strlen (line);
                    printf("%d\n", len1);
                    len = len1 + 10 + len;
                    if (len > width) {
                        width = 2 * width;
                        store = realloc (store, width);
                        if (!store)
                            exit (1);
                    }
                    //if -w option is present
                    if (wgrep)
                        strcat (store, "\\b");
                    // remove '\n' from the line
                    if ((rm = strchr (line, '\n')) != NULL)
                        *rm = '\0';
                    strcat(store, line);
                    if (wgrep)
                        strcat (store, "\\b");
                    strcat (store, "|");
                }
                fclose(fp);
            } 
            else {
                fprintf(stderr, "%s: %s: could not open: %s\n",
                    arg_vector, ffiles[i], strerror(errno));
                errors++;
            }
        }
   
    }
    if (egrep) {
        for (i = 0; i < egrep; i++) {
                len1 = strlen (efiles[i]);
                len = len + len1 + 10;
                if (len > width) {
                        width = 2 * width;
                        store = realloc (store, width);
                        if (!store)
                            exit (1);
                }
                if (wgrep)
                    strcat (store, "\\b");
                strcat(store, efiles[i]);
                if (wgrep)
                    strcat (store, "\\b");
                strcat (store, "|");
        }
    
    }
    len = strlen (store);
    store [len - 1] = '\0';
    strcat (store, ")");
    //printf("%s\n", store);
    return (store);
}

/* process_files --- opens and process them  */ 

int process_files (char **arg_vector, int arg_count) {
    FILE *fp;
    int i;
    // no files, default to stdin 
    if (optind == arg_count) {
        if (rgrep)
            process_dir (".");       //search the current directory
        else     
            process("standard input", stdin);
    }

    else {      /* loop over files */
        if (arg_count - optind > 1)
            no_files = 1;
        for (i = optind; i < arg_count; i++) {
            if (strcmp(arg_vector[i], "-") == 0)
                process("standard input", stdin);
            else if ((fp = fopen(arg_vector[i], "r+")) != NULL) {
                process(arg_vector[i], fp);
                fclose(fp);
            } 
            else {
                if (rgrep)
                    process_dir (arg_vector[i]);
                else {
                    fprintf(stderr, "%s: %s: could not open: %s\n",
                        arg_vector[0], arg_vector[i], strerror(errno));
                    errors++;
                }   
            }
        }
    }
    regfree(& pattern);
    return errors != 0;
}

 /* process_dir --- opens and process directories recursively */

void process_dir (char *dir_name) {
    char file_name [1000];
    struct dirent *entry;
    FILE *fp;
    DIR *dir;
    dir = opendir (dir_name);
    //if directory dosen't open
    if (!dir) {
        fprintf(stderr, "%s: %s: could not open: %s\n",
            myname, dir_name, strerror(errno));
        errors++;

    }
    no_files = 1;
    while ((entry = readdir(dir)) != NULL) {
        //skip self and parent entry
        if (strcmp (entry -> d_name, ".") != 0 && strcmp (entry -> d_name, "..") != 0) {
            //if (strcmp (dir_name, ".")) 
                strcpy (file_name, dir_name);
            strcat (file_name, "/");
            strcat (file_name, entry -> d_name);
            // if the entry is directory
            if (entry -> d_type == DT_DIR) {
                process_dir (file_name);   
            }
            else {
                if ((fp = fopen(file_name, "r")) != NULL) {
                   process (file_name, fp);
                   fclose (fp);
                }
               else {
                   printf("%s: %s: could not open: %s\n",
                       myname, dir_name, strerror(errno));
                   errors++;
               }
            }
        }
                       
    }
    closedir (dir);

}

/* compile_pattern --- compile the pattern */

void compile_pattern(char *pat) {
    int flags;
    int ret;
    char *temp;
    temp = (char *) malloc (100);
    char error[BUFSIZE];

    /* for -i option */
    if (igrep)     
        flags = REG_ICASE;

    // for -f and -e option 
    if (fgrep || egrep)
        flags |= REG_EXTENDED;
    
    /* for -w option */
    /* we are modifiying the given pattern as "\\b[pattern]\\b", 
        which will look for the word [pattern] */
    if (wgrep && !fgrep) {
        temp[0] = '\0';
        strcpy (temp, "\\b");
        strcat (temp, pat);
        strcat (temp, "\\b");
        pat = temp;
    }
    // to compile the pattern
    ret = regcomp(& pattern, pat, flags);
    if (ret != 0) {
        (void) regerror(ret, & pattern, error, sizeof error);
        fprintf(stderr, "%s: pattern `%s': %s\n", myname, pat, error);
        errors++;
    }
    free (temp);
}

/* process --- read lines of text and match against the pattern */

void process (char *name, FILE *fp) {
    char *line = NULL;
    char *rm = NULL;
    size_t size = 0;
    char error[BUFSIZE];
    int ret;
    int offset = 0;
    int eflags = 0;
    int length;
    int temp_offset;
    //to reset the value of prev_length
    prev_length = 0;
    // to reset the value of ccount;
    ccount = 0;
    // to reset the value of mcount
    mcount = mcount1;
    
    while (getline(& line, &size, fp) != -1) {
        length = strlen (line);
        // to remove '\n' from the string
        if ((rm = strchr (line, '\n')) != NULL) {
            *rm = '\0';
        }
        no_match = 0;
        offset = 0;
        temp_offset = 0;
        //  using regexec recursively to find all the matches in line
        while ((ret = regexec(& pattern, line + offset, 1, &match[no_match], eflags)) == 0) {
            // for -q option
            if (qgrep)
                exit (0);
            // do not let ^ match again.
            eflags = REG_NOTBOL;
            //storing prev offset value
            temp_offset = offset;
            //changing the offset
            offset += match[no_match].rm_eo;
            match[no_match].rm_so = temp_offset + match[no_match].rm_so;
            match[no_match].rm_eo = temp_offset + match[no_match].rm_eo;
            //a match can be a zero-length match, we must not fail
            if (match[no_match].rm_so == match[no_match].rm_eo) {
                offset += 1;
            }  
            no_match++;
            if (offset > (length - 1
                ))
                break;     
        }

        if (no_match == 0) {
            if (ret == REG_NOMATCH && vgrep == 1) {
                if (mgrep) 
                    mcount--;
                if (cgrep)
                    ccount++;
                else
                    print_line (name, line);    /* print non-matching lines */
            }
            else if (ret != REG_NOMATCH){
                (void) regerror(ret, & pattern, error, sizeof error);
                fprintf(stderr, "%s: file %s: %s\n", myname, name, error);
                free(line);
                errors++;
                return;
            }
        }
        else if (vgrep == 0) {
            if (mgrep)
                mcount--;
            if (cgrep)
                ccount++;
            else
                print_line(name, line); /* print matching lines */
        }
        if (bgrep)
            prev_length += length;
        //when mcount becomes zero
        if (mgrep && !mcount)
            return;
    }
    if (cgrep){
        cgrep = 2;
        print_line(name, line); /* print number of  matching lines */
    }
  
    free(line);
    
}

/* printline --- print output */

void print_line (char *file_name, char * line) {
    char *temp = line;
    int i;
    int start, end;
    // to print the filename
    if ((no_files == 1 || Hgrep == 1) && hgrep == 0)
        printf("\033[0;35m%s\033[0;36m:\033[0m", file_name);
    //for -b option
    if (bgrep && !cgrep)
        printf("\033[0;32m%d\033[0;36m:\033[0m", prev_length);
    // to print the no of lines if option -c is given
    // -c manipulates other option so matching line won't print
    if  (cgrep == 2) {
        printf("%d\n", ccount);
    }
    // to print matching line
    else if (vgrep == 0) {
        for (i = 0; i <= no_match; i++) {
            if (i == no_match) {
                printf("%s\n", temp);
                break;
            }
            start = match[i].rm_so;
            end = match[i].rm_eo;
            printf("%.*s", (int)(&line[start] - temp), temp);
            temp = &line[start];
            printf("\033[1;31m%.*s\033[0m", (int)(&line[end] - &line[start]) , temp);
            temp = &line[end];   
        }
    }
    // to print non-matching lines
    // If option -v is given
    else {                      
        printf("%s\n", line); 
    }
    // for -m option
    return ;
}

/* usage --- print usage message and exit */

void usage(void) {
    if (arg_error)
        fprintf(stderr, "%s:  option requires an argument -- %c\n", myname, optopt);
    fprintf(stderr, "usage: %s [OPTION]... PATTERNS [FILE]...\n", myname);
    exit(1);
}