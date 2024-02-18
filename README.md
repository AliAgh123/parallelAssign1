# Running:
## Sequential Code
To run the files above run the following commands for the sequential code:
```
gcc -o main main.c
```
Then
```
./main
```

## Parallelized code:
Run the following commands for the parallelized code:
```
mpicc -o main main.c
```
and then
```
mpirun --oversubscribe -np 5 main
```

