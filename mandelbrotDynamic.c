#include <stdio.h>
#include <time.h>
#include <mpi.h>

#define WIDTH 640
#define HEIGHT 480
#define MAX_ITER 255

typedef struct complex{
    double real;
    double imag;
    int color;
} complex;

int cal_pixel(struct complex c) {
    double z_real = 0;
    double z_imag = 0;

    double z_real2, z_imag2, lengthsq;

    int iter = 0;
    do {
        z_real2 = z_real * z_real;
        z_imag2 = z_imag * z_imag;

        z_imag = 2 * z_real * z_imag + c.imag;
        z_real = z_real2 - z_imag2 + c.real;
        lengthsq =  z_real2 + z_imag2;
        iter++;
    }
    while ((iter < MAX_ITER) && (lengthsq < 4.0));

    return iter;

}
void save_pgm(const char *filename, int image[HEIGHT][WIDTH]) {
    FILE* pgmimg;
    int temp;
    pgmimg = fopen(filename, "wb");
    fprintf(pgmimg, "P2\n"); // Writing Magic Number to the File   
    fprintf(pgmimg, "%d %d\n", WIDTH, HEIGHT);  // Writing Width and Height
    fprintf(pgmimg, "255\n");  // Writing the maximum gray value 
    int count = 0;

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            temp = image[i][j];
            fprintf(pgmimg, "%d ", temp); // Writing the gray values in the 2D array to the file 
        }
        fprintf(pgmimg, "\n");
    }
    fclose(pgmimg);
}

int main(int argc, char const *argv[])
{
    int image[HEIGHT][WIDTH];
    double total_time;

    MPI_Init(NULL, NULL);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int myRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    complex c;

    if(myRank == 0){
        clock_t start_time = clock();
        int count = 0;
        int row = 0;
        for(int k=1; k<world_size; k++){
            MPI_Send(&row, 1, MPI_INT, k, 0, MPI_COMM_WORLD);
            count++;
            row++;
        }

        do{
            int slave = 0;
            int color[WIDTH+2];
            MPI_Recv(&color, WIDTH+2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            count --;
            if(row < HEIGHT){
                MPI_Send(&row, 1, MPI_INT, color[WIDTH], 0, MPI_COMM_WORLD);
                row++;
                count++;

            } else {

                MPI_Send(&row, 1, MPI_INT, color[WIDTH], 0, MPI_COMM_WORLD);
            }
            for(int k=0; k<WIDTH; k++){
                image[color[WIDTH+1]][k] = color[k];
            }

        }while( count > 0);
        printf("Done receiving data\n");
        save_pgm("mandelbrot_dynamic.pgm", image);
        printf("done saving the image\n");
        clock_t end_time = clock();

        total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        printf("The total Execution time is: %f ms\n", total_time*1000);

    } else {
        int row = 0;
        int color[WIDTH+2];
        MPI_Recv(&row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        while(row < HEIGHT){
            c.imag = (row - HEIGHT / 2.0) * 4.0 / HEIGHT;
            for(int i=0; i<WIDTH; i++){
                c.real = (i - WIDTH / 2.0) * 4.0 / WIDTH;
                color[i] = cal_pixel(c);
            }
            color[WIDTH] = myRank;
            color[WIDTH+1] = row;

            MPI_Send(&color, WIDTH+2, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Recv(&row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    MPI_Finalize();

    return 0;
}
