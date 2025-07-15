int main(int argc, char *argv[]){
    int i, done = 0, n, count;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    while (!done) {
        printf("Enter the number of points: (0 quits) \n");
        scanf("%d",&n);
        if (n == 0) break;
        count = 0;
        for (i = 1; i <= n; i++) {
            x = ((double) rand()) / ((double) RAND_MAX);
            y = ((double) rand()) / ((double) RAND_MAX);
            z = sqrt((x*x)+(y*y));
            if(z <= 1.0)
            count++;
        }
    pi = ((double) count/(double) n)*4.0;
    printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
    }
}