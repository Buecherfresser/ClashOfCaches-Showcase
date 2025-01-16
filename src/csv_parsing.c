#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "csv_parsing.h"
#include "main.h"

int isCSV(char *pathname)
{
    if (pathname == NULL)
    {
        fprintf(stderr, "Read csv failed, path is NULL \n");
        return 0;
    }

    size_t len = strlen(pathname);

    if (len < 4)
    { // if it isn´t long enough
        fprintf(stderr, "Read csv failed, path is not long enough \n");

        return 0;
    }

    if (strcmp(pathname + len - 4, ".csv") == 0) // if it doesnt end with .csv
    {
        return 1;
    }

    fprintf(stderr, "Read csv failed, doesn't end with .csv \n");

    return 0;
}

char *del_leading_zero(char *pointer)
{
    int i = 0;
    while (pointer[i] == '0')
    {
        if (i == 0 && (pointer[1] == 'x' || pointer[1] == 'X'))
        {
            return pointer;
        }
        i++;
    }
    if (pointer[i] == '\0')
    {
        return pointer + i - 1;
    }
    return pointer + i;
}

struct Request *readCSV(char *pathname, char *progname, unsigned long *numRequests)
{

    if (pathname == NULL)
    {
        fprintf(stderr, "There is no path \n");
        return NULL;
    }

    if (!isCSV(pathname))
    {
        fprintf(stderr, "This isn't a csv file. \n");
        print_help(progname);
        return NULL;
    }

    FILE *file = fopen(pathname, "r"); // open file structure

    if (!file)
    { // if fopen didn´t work
        fprintf(stderr, "File not found. You sure that file exists at <%s> ?\n", pathname);
        return NULL;
    }

    size_t req_growth_factor = 4;
    struct Request *requests = malloc(req_growth_factor * sizeof(struct Request)); // request array that will be returned
    size_t req_size = req_growth_factor;                                           // size of request-array

    size_t buffersize = 24;
    char *buffer = (char *)malloc(buffersize); // allocate 25 bytes
    size_t numRead;                            // number of items fread read
    int line = 0;

    if (buffer == NULL)
    { // memmory allocation was successfull
        fprintf(stderr, "Memory allocation failed while reading the csv-file .\n");
        return NULL;
    }

    while (fgets(buffer, buffersize, file) != NULL)
    {

        // declare variables
        line++;
        uint32_t addr;
        uint32_t data;
        char *addr_string = (char *)malloc(10);
        char *data_string = (char *)malloc(10);
        int we;

        if (addr_string == NULL || data_string == NULL)
        { // memmory allocation was successfull
            fprintf(stderr, "Memory allocation failed while reading the csv-file .\n");
            return NULL;
        }

        // iterator variables
        int i = 0;

        //---------------------- reading or writing --------------------
        switch (buffer[i])
        {
        case '\0':
            fprintf(stderr, "Error, wrong csv-file-format in line %d. Expected: (\"R\"|\"W\"), but got:  \n", line);
            return NULL;
        case 'R':
            we = 0;
            break;
        case 'W':
            we = 1;
            break;
        default:
            fprintf(stderr, "Error, wrong csv-file-format in line %d. Expected: (\"R\"|\"W\"), but got: %c \n", line, buffer[i]);
            return NULL;
        }

        i++;

        //---------------------- Komma --------------------
        if (buffer[i] != ';')
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d. Expected: (\";\").\n", line);
            return NULL;
        }

        i++;

        //------------------- addressReading ------------------
        int addr_char_counter = 0;

        while (addr_char_counter < 10 && buffer[i] != ';' && buffer[i] != '\0')
        {
            addr_string[addr_char_counter] = buffer[i];
            addr_char_counter++;

            i++;
        }

        if (addr_char_counter == 0)
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d, no address given.\n", line);
            return NULL;
        }

        //------------------- Komma ------------------

        if (buffer[i] != ';')
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d. Expected: (\";\").\n", line);
            return NULL;
        }

        i++;

        //---------------------- dataReading --------------------

        int data_char_counter = 0;

        while (data_char_counter < 10 && buffer[i] != '\n' && buffer[i] != '\0')
        {
            data_string[data_char_counter] = buffer[i];
            data_char_counter++;

            i++;
        }

        if (we == 0 && data_char_counter != 0)
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d, data given when specifying \"R\" or \";\" is missing here to clarify that there is no data input.\n", line);
            return NULL;
        }
        else if (we == 1 && data_char_counter == 0)
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d, there needs to be data when specifying \"W\".\n", line);
            return NULL;
        }

        //------------------- NewLine ------------------

        if (buffer[i] != '\n')
        {

            char *testbuffer = (char *)malloc(buffersize); // allocate 25 bytes
            if (testbuffer == NULL)
            { // memmory allocation was successfull
                fprintf(stderr, "Memory allocation failed while reading the csv-file .\n");
                return NULL;
            }
            // if this is the last line in the file -> the doesn´t have to be a new line, so...
            if (buffer[i] == '\0' && fgets(testbuffer, buffersize, file) == NULL)
            {
                i--;
            }
            else
            {
                fprintf(stderr, "Error, wrong csv-file-format in line %d, Expected: (\"\\n\"), but got %c .\n", line, buffer[i]);
                return NULL;
            }
        }

        i++;

        //----------------- EndByte -------------------

        if (buffer[i] != '\0')
        {
            fprintf(stderr, "Error, wrong csv-file-format in line %d (I literally can`t imagine how you could end up here).\n", line);
            return NULL;
        }

        //---------- parsing to Request-struct --------

        char *endptr;

        if (addr_char_counter < 10)
        {
            addr_string[addr_char_counter] = '\0';
        }
        if (data_char_counter < 10)
        {
            data_string[data_char_counter] = '\0';
        }

        addr_string = del_leading_zero(addr_string);

        // convert to addr
        addr = (uint32_t)strtoul(addr_string, &endptr, 0);

        if (*endptr != '\0')
        { // if converstion failed
            fprintf(stderr, "Error, wrong csv-file-format in line %d, can't convert address.\n", line);
            return NULL;
        }

        if (data_char_counter != 0)
        { // if data was given
            data_string = del_leading_zero(data_string);
            data = (uint32_t)strtoul(addr_string, &endptr, 0);

            if (*endptr != '\0')
            { // if converstion failed
                fprintf(stderr, "Error, wrong csv-file-format in line %d, can't convert data.\n", line);
                return NULL;
            }
        }
        else
        {
            data = 0;
        }

        // create request
        struct Request request = {addr, data, we};

        // realloc more space if the array is full
        if (line > req_size)
        {
            req_size = req_size * req_growth_factor;                                             // new array size
            struct Request *new_requests = realloc(requests, req_size * sizeof(struct Request)); // realloc more space

            if (!new_requests)
            { // if realloc failed
                fprintf(stderr, "Error, couldn't allocate Memmory.\n", line);
                return NULL;
            }
            requests = new_requests;
        }

        // add request to the array
        requests[line - 1] = request;

        // free some arrays
        free(addr_string);
        free(data_string);
    }

    if (line == 0)
    { // if
        fprintf(stderr, "Error, file is empty...\n", line);
        return NULL;
    }

    // cut the array to the perfect length
    struct Request *new_requests = realloc(requests, line * sizeof(struct Request));

    if (!new_requests)
    { // if realloc failed
        fprintf(stderr, "Error, couldn't allocate Memmory.\n", line);
        return NULL;
    }

    requests = new_requests;

    free(buffer);
    if (fclose(file) != 0)
    { // if closing the file didn´t work
        fprintf(stderr, "Error while closing the File.\n");
        return NULL;
    }

    if (numRequests == NULL)
    { // if the fiven pointer is null
        fprintf(stderr, "numRequests is Null.\n");
        return NULL;
    }
    *numRequests = line;

    return requests;
}