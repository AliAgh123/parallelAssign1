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
    int s = (HEIGHT/(world_size-1))+1;
    complex c;



    // create a type for struct complex
    const int nitems = 3;
    int blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
    MPI_Datatype mpi_complex_type;
    MPI_Aint offsets[3];
    offsets[0] = offsetof(complex, real);
    offsets[1] = offsetof(complex, imag);
    offsets[2] = offsetof(complex, color);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_complex_type);
    MPI_Type_commit(&mpi_complex_type);

    // done creating MPI_complex_type


    if(myRank == 0){

        printf("Master is here\n");
        clock_t start_time = clock();
        for(int i=0; i<WIDTH * HEIGHT; i++){
            MPI_Recv(&c, 1, mpi_complex_type, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            image[(int)c.imag][(int)c.real] = c.color;
        }
        printf("Done receiving data\n");
        clock_t end_time = clock();

        total_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
        save_pgm("mandelbrot_static.pgm", image);
        printf("Done saving the image\n");
        printf("The total Execution time is: %f ms\n", total_time*1000);
     
        
        
    } else {
        
        int start = (myRank-1)*s;
        int end = start + s;
        for(int i=0; i<WIDTH; i++){
            for(int j=start; j < end && j < HEIGHT; j++){
                c.real = (i - WIDTH / 2.0) * 4.0 / WIDTH;
                c.imag = (j - HEIGHT / 2.0) * 4.0 / HEIGHT;
                c.color = cal_pixel(c);
                c.real = i;
                c.imag = j;
                MPI_Send(&c, 1, mpi_complex_type, 0, 0, MPI_COMM_WORLD);
            }
        }
        printf("Done sending the data in process %d\n", myRank);
    
        
    }




    MPI_Type_free(&mpi_complex_type);
    MPI_Finalize();

    return 0;
}
