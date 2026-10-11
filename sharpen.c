// Sam Siewert, July 16, 2020
// Modified for single-threaded benchmarking with syslog instrumentation
//
// Based on basic PSF convolution as documented in DSP Engineer's Handbook
// http://www.dspguide.com/pdfbook.htm
//
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <string.h>

typedef double FLOAT;

typedef unsigned int UINT32;
typedef unsigned long long int UINT64;
typedef unsigned char UINT8;

#define K 4.0
FLOAT PSF[9] = {-K/8.0, -K/8.0, -K/8.0, -K/8.0, K+1.0, -K/8.0, -K/8.0, -K/8.0, -K/8.0};

int main(int argc, char *argv[])
{
    int img_height, img_width, img_pixels;
    int fdin, fdout, bytesRead=0, bytesLeft, i, j, iter, iterations;
    UINT8 *header, *R, *G, *B, *convR, *convG, *convB;
    UINT64 microsecs=0, millisecs=0;
    FLOAT temp, fstart, fnow;
    struct timespec start, now, iter_start, iter_end;
    unsigned long iter_time_ns;
    char resolution_label[64];

    openlog("sharpen_bench", LOG_PID, LOG_USER);

    if(argc < 4)
    {
       fprintf(stderr, "Usage: sharpen <height> <width> <iterations> <input_file.ppm> <output_file.ppm>\n");
       fprintf(stderr, "Example: sharpen 300 400 10 input.ppm output.ppm\n");
       exit(-1);
    }

    img_height = atoi(argv[1]);
    img_width = atoi(argv[2]);
    iterations = atoi(argv[3]);
    img_pixels = img_height * img_width;

    snprintf(resolution_label, sizeof(resolution_label), "%dx%d_%d_pixels", 
             img_width, img_height, img_pixels);

    syslog(LOG_INFO, "=== START BENCHMARK %s, %d iterations ===", resolution_label, iterations);

    // Allocate arrays
    header = malloc(22);
    R = malloc(img_pixels);
    G = malloc(img_pixels);
    B = malloc(img_pixels);
    convR = malloc(img_pixels);
    convG = malloc(img_pixels);
    convB = malloc(img_pixels);

    if(!header || !R || !G || !B || !convR || !convG || !convB) {
        fprintf(stderr, "Memory allocation failed\n");
        syslog(LOG_ERR, "Memory allocation failed for %s", resolution_label);
        exit(-1);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    fstart = (FLOAT)start.tv_sec  + (FLOAT)start.tv_nsec / 1000000000.0;
    
    if((fdin = open(argv[4], O_RDONLY, 0644)) < 0)
    {
        fprintf(stderr, "Error opening %s\n", argv[4]);
        syslog(LOG_ERR, "Error opening input file: %s", argv[4]);
        exit(-1);
    }

    if((fdout = open(argv[5], (O_RDWR | O_CREAT | O_TRUNC), 0666)) < 0)
    {
        fprintf(stderr, "Error opening %s\n", argv[5]);
        syslog(LOG_ERR, "Error opening output file: %s", argv[5]);
        exit(-1);
    }

    bytesLeft = 21;

    do
    {
        bytesRead = read(fdin, (void *)header, bytesLeft);
        bytesLeft -= bytesRead;
    } while(bytesLeft > 0);

    header[21] = '\0';

    // Read RGB data
    for(i=0; i<img_pixels; i++)
    {
        read(fdin, (void *)&R[i], 1); convR[i]=R[i];
        read(fdin, (void *)&G[i], 1); convG[i]=G[i];
        read(fdin, (void *)&B[i], 1); convB[i]=B[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    fnow = (FLOAT)now.tv_sec  + (FLOAT)now.tv_nsec / 1000000000.0;
    printf("\n=== PSF SHARPEN BENCHMARK: %s ===\n", resolution_label);
    printf("File I/O complete at %.3f seconds\n", fnow-fstart);
    syslog(LOG_INFO, "File I/O complete, starting %d iterations", iterations);

    for(iter=0; iter < iterations; iter++)
    {
        clock_gettime(CLOCK_MONOTONIC, &iter_start);
        syslog(LOG_INFO, "ITER_START %s iter=%d", resolution_label, iter+1);

        // Skip first and last row, no neighbors to convolve with
        for(i=1; i<(img_height-1); i++)
        {
            // Skip first and last column, no neighbors to convolve with
            for(j=1; j<(img_width-1); j++)
            {
                // RED channel
                temp=0;
                temp += (PSF[0] * (FLOAT)R[((i-1)*img_width)+j-1]);
                temp += (PSF[1] * (FLOAT)R[((i-1)*img_width)+j]);
                temp += (PSF[2] * (FLOAT)R[((i-1)*img_width)+j+1]);
                temp += (PSF[3] * (FLOAT)R[((i)*img_width)+j-1]);
                temp += (PSF[4] * (FLOAT)R[((i)*img_width)+j]);
                temp += (PSF[5] * (FLOAT)R[((i)*img_width)+j+1]);
                temp += (PSF[6] * (FLOAT)R[((i+1)*img_width)+j-1]);
                temp += (PSF[7] * (FLOAT)R[((i+1)*img_width)+j]);
                temp += (PSF[8] * (FLOAT)R[((i+1)*img_width)+j+1]);
	        if(temp<0.0) temp=0.0;
	        if(temp>255.0) temp=255.0;
	        convR[(i*img_width)+j]=(UINT8)temp;

                // GREEN channel
                temp=0;
                temp += (PSF[0] * (FLOAT)G[((i-1)*img_width)+j-1]);
                temp += (PSF[1] * (FLOAT)G[((i-1)*img_width)+j]);
                temp += (PSF[2] * (FLOAT)G[((i-1)*img_width)+j+1]);
                temp += (PSF[3] * (FLOAT)G[((i)*img_width)+j-1]);
                temp += (PSF[4] * (FLOAT)G[((i)*img_width)+j]);
                temp += (PSF[5] * (FLOAT)G[((i)*img_width)+j+1]);
                temp += (PSF[6] * (FLOAT)G[((i+1)*img_width)+j-1]);
                temp += (PSF[7] * (FLOAT)G[((i+1)*img_width)+j]);
                temp += (PSF[8] * (FLOAT)G[((i+1)*img_width)+j+1]);
	        if(temp<0.0) temp=0.0;
	        if(temp>255.0) temp=255.0;
	        convG[(i*img_width)+j]=(UINT8)temp;

                // BLUE channel
                temp=0;
                temp += (PSF[0] * (FLOAT)B[((i-1)*img_width)+j-1]);
                temp += (PSF[1] * (FLOAT)B[((i-1)*img_width)+j]);
                temp += (PSF[2] * (FLOAT)B[((i-1)*img_width)+j+1]);
                temp += (PSF[3] * (FLOAT)B[((i)*img_width)+j-1]);
                temp += (PSF[4] * (FLOAT)B[((i)*img_width)+j]);
                temp += (PSF[5] * (FLOAT)B[((i)*img_width)+j+1]);
                temp += (PSF[6] * (FLOAT)B[((i+1)*img_width)+j-1]);
                temp += (PSF[7] * (FLOAT)B[((i+1)*img_width)+j]);
                temp += (PSF[8] * (FLOAT)B[((i+1)*img_width)+j+1]);
	        if(temp<0.0) temp=0.0;
	        if(temp>255.0) temp=255.0;
	        convB[(i*img_width)+j]=(UINT8)temp;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &iter_end);
        iter_time_ns = (iter_end.tv_sec - iter_start.tv_sec) * 1000000000ULL + 
                       (iter_end.tv_nsec - iter_start.tv_nsec);
        printf("  Iteration %2d: %.3f ms\n", iter+1, iter_time_ns/1000000.0);
        syslog(LOG_INFO, "ITER_END %s iter=%d time_ns=%lu time_ms=%.3f", 
               resolution_label, iter+1, iter_time_ns, iter_time_ns/1000000.0);
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    fnow = (FLOAT)now.tv_sec  + (FLOAT)now.tv_nsec / 1000000000.0;
    printf("\nTotal time for %d iterations: %.3f seconds\n", iterations, fnow-fstart);
    printf("Average per iteration: %.3f ms\n", (fnow-fstart)*1000.0/iterations);
    syslog(LOG_INFO, "=== COMPLETE %s total=%.3f sec avg=%.3f ms ===", 
           resolution_label, fnow-fstart, (fnow-fstart)*1000.0/iterations);

    write(fdout, (void *)header, 21);

    // Write RGB data
    for(i=0; i<img_pixels; i++)
    {
        write(fdout, (void *)&convR[i], 1);
        write(fdout, (void *)&convG[i], 1);
        write(fdout, (void *)&convB[i], 1);
    }

    close(fdin);
    close(fdout);
    
    free(header);
    free(R);
    free(G);
    free(B);
    free(convR);
    free(convG);
    free(convB);
    
    closelog();
    return 0;
}
