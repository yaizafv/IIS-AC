/*
 * Main.cpp
 *
 *  Created on: Fall 2019
 */

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <CImg.h>
#include <stdlib.h>
#include <time.h>

using namespace cimg_library;

// Tipo de datos a usar (seg�n el enunciado)
typedef float data_t;

// Ficheros de entrada y salida
const char* SOURCE_IMG = "bailarina.bmp";
const char* SOURCE_IMG2 = "flores_3.bmp";
const char* DEST_IMG = "resultado_multihilo.bmp";

// Argumentos globales del filtro
typedef struct {
    data_t* pRsrc;
    data_t* pGsrc;
    data_t* pBsrc;
    data_t* pRsrc2;
    data_t* pGsrc2;
    data_t* pBsrc2;
    data_t* pRdst;
    data_t* pGdst;
    data_t* pBdst;
    long pixelCount;
} filter_args_t;

filter_args_t global_args;

// Estructura para cada hilo
typedef struct {
    long start;
    long end;
} thread_data_t;


// Filtro por segemento
void* filter_segment(void* arg)
{
    thread_data_t* data = (thread_data_t*)arg;

    for (long i = data->start; i < data->end; i++) {

        global_args.pRdst[i] =
            sqrt(global_args.pRsrc[i] * global_args.pRsrc[i] +
                global_args.pRsrc2[i] * global_args.pRsrc2[i]) / sqrt(2);

        global_args.pGdst[i] =
            sqrt(global_args.pGsrc[i] * global_args.pGsrc[i] +
                global_args.pGsrc2[i] * global_args.pGsrc2[i]) / sqrt(2);

        global_args.pBdst[i] =
            sqrt(global_args.pBsrc[i] * global_args.pBsrc[i] +
                global_args.pBsrc2[i] * global_args.pBsrc2[i]) / sqrt(2);
    }

    pthread_exit(NULL);
}



int main()
{
    // Cargar imágenes

    CImg<data_t> img1(SOURCE_IMG);
    CImg<data_t> img2(SOURCE_IMG2);

    long width = img1.width();
    long height = img1.height();
    long pixelCount = width * height;

    if (pixelCount != img2.width() * img2.height()) {
        printf("ERROR: Las im�genes tienen tama�os distintos.\n");
        return -1;
    }

    // Pointers a los componentes RGB
    global_args.pixelCount = pixelCount;

    global_args.pRsrc = img1.data();
    global_args.pGsrc = global_args.pRsrc + pixelCount;
    global_args.pBsrc = global_args.pGsrc + pixelCount;

    global_args.pRsrc2 = img2.data();
    global_args.pGsrc2 = global_args.pRsrc2 + pixelCount;
    global_args.pBsrc2 = global_args.pGsrc2 + pixelCount;

    // Imagen destino
    data_t* pDst = (data_t*)malloc(sizeof(data_t) * pixelCount * 3);
    global_args.pRdst = pDst;
    global_args.pGdst = pDst + pixelCount;
    global_args.pBdst = global_args.pGdst + pixelCount;

    // Multihilo
    int num_threads = 4; 
    pthread_t threads[num_threads];
    thread_data_t tdata[num_threads];

    long chunk = pixelCount / num_threads;

    struct timespec tStart, tEnd;
    clock_gettime(CLOCK_REALTIME, &tStart);

    // Crear hilos
    for (int t = 0; t < num_threads; t++) {

        tdata[t].start = t * chunk;
        tdata[t].end = (t == num_threads - 1) ? pixelCount : (t + 1) * chunk;

        pthread_create(&threads[t], NULL, filter_segment, &tdata[t]);
    }

    // Esperar a todos
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_REALTIME, &tEnd);

    // Tiempo
    double elapsed = (tEnd.tv_sec - tStart.tv_sec) +
        (tEnd.tv_nsec - tStart.tv_nsec) / 1e9;

    printf("Tiempo multihilo (pthread): %f segundos\n", elapsed);

    // Guardar resultado
    CImg<data_t> dst(pDst, width, height, 1, 3);
    dst.save(DEST_IMG);

    free(pDst);

    return 0;
}