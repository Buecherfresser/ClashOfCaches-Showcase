#ifndef EXPANDED_METHOD_H
#define EXPANDED_METHOD_H

/* read sepecified csv-file
Returns: Request arr or Null on faliure */
struct Request *readCSV(char *pathname, char *progname, unsigned long *numRequests);

#endif
