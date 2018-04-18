/*



  1 thread: 26.631
  2 threads: 16.625
  4 threads: 13.394
  8 threads: 13.333
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <omp.h>

int main(int argc, char* argv[])
{


    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);


  /* Parse the command line arguments. */
  if (argc != 8) {
    printf("Usage:   %s <xmin> <xmax> <ymin> <ymax> <maxiter> <xres> <out.ppm>\n", argv[0]);
    printf("Example: %s 0.27085 0.27100 0.004640 0.004810 1000 1024 pic.ppm\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* The window in the plane. */
  const double xmin = atof(argv[1]);
  const double xmax = atof(argv[2]);
  const double ymin = atof(argv[3]);
  const double ymax = atof(argv[4]);

  /* Maximum number of iterations, at most 65535. */
  const uint16_t maxiter = (unsigned short)atoi(argv[5]);

  /* Image size, width is given, height is computed. */
  const int xres = atoi(argv[6]);
  const int yres = (xres*(ymax-ymin))/(xmax-xmin);

  /* The output file name */
  const char* filename = argv[7];

  /* Open the file and write the header. */
  FILE * fp = fopen(filename,"wb");
  char *comment="# Mandelbrot set";/* comment should start with # */

  /*write ASCII header to the file*/
  fprintf(fp,
          "P6\n# Mandelbrot, xmin=%lf, xmax=%lf, ymin=%lf, ymax=%lf, maxiter=%d\n%d\n%d\n%d\n",
          xmin, xmax, ymin, ymax, maxiter, xres, yres, (maxiter < 256 ? 256 : maxiter));

  /* Precompute pixel width and height. */
  double dx=(xmax-xmin)/xres;       //no change
  double dy=(ymax-ymin)/yres;       //no change

  double x, y; /* Coordinates of the current point in the complex plane. */     //changes, but not self referential
  int i,j; /* Pixel counters */ //loop counter
  int k; /* Iteration counter */    //loop counter

  printf("hi\n");
  printf("%d\n", xres);
  printf("%d\n", yres);

  int * storage;
  storage = malloc(sizeof (int) * xres * yres);

  printf("bye\n");
  fflush(stdout);

  //valid point for threading
  //everything past this point is self contained

  omp_set_num_threads(8);
  #pragma omp parallel
  for (j = 0; j < yres; j++) {  //loop 1
    y = ymax - j * dy;
    //printf("j\n");


    for(i = 0; i < xres; i++) {   //loop 2
      double u = 0.0;
      double v= 0.0;
      double u2 = u * u;
      double v2 = v*v;
      x = xmin + i * dx;
      /* iterate the point */
      for (k = 1; k < maxiter && (u2 + v2 < 4.0); k++) {
            v = 2 * u * v + y;
            u = u2 - v2 + x;
            u2 = u * u;
            v2 = v * v;
      };
      /* compute  pixel color and write it to file */
      if (k >= maxiter) {
          storage[j*xres + i] = maxiter;
        /* interior */
      }
      else {
        /* exterior */

        storage[j*xres + i] = k;

      }

  }     //end loop 2
}    //end loop 1


//stop time computation

clock_gettime(CLOCK_MONOTONIC, &finish);

elapsed = (finish.tv_sec - start.tv_sec);
elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

printf("%f\n", elapsed);


for (j = 0; j < yres; j++) {

  for(i = 0; i < xres; i++) {

     int k = storage[j * xres + i];

     if(k >= maxiter) {
         const unsigned char black[] = {0, 0, 0, 0, 0, 0};
         fwrite (black, 6, 1, fp);
     } else {

         unsigned char color[6];
         color[0] = k >> 8;
         color[1] = k & 255;
         color[2] = k >> 8;
         color[3] = k & 255;
         color[4] = k >> 8;
         color[5] = k & 255;
         fwrite(color, 6, 1, fp);
     }

  }
}





  fclose(fp);
  return 0;
}
