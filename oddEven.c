#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>

void bubble_sort(int *a, int n);
int compute_partner(int phase, int id);

int main() {
    int numprocs, id;               // total number of process and process id
    int n, local_n;                 // total number to sort and its local (n / numprocs)
    double startTime, totalTime;    // record time

    int *arr;                       // sorted arr in process 0 (n)
    int *local_arr;                 // local arr for each process (local_n)
    int *arrNum;                    // arr to save number of digits in each process (numprocs)
    int *startIdx;                  // save the starting index of numbers from each process (numprocs)
    int *recv_arr;                  // save received arr
    int *temp_arr;

    // init
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    srand(time(0) * id );

    // record time
    startTime = MPI_Wtime();

    // ask user input for total number to sort
    if (id == 0) {
        printf("Enter total number to sort:");
        scanf("%d", &n);
    }
    // broadcast n to other process
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    // count how many number to sort for each process
    local_n = n / numprocs;
    if (id < n % numprocs) {
        local_n += 1;
    }

    // allocate mem for array
    recv_arr = (int *)malloc(sizeof(int) * local_n + 1);
    arr = (int *)malloc(sizeof(int) * n);
    local_arr = (int *)malloc(sizeof(int) * local_n);
    arrNum = (int *)malloc(sizeof(int) * numprocs);
    startIdx = (int *)malloc(sizeof(int) * numprocs);
    temp_arr = (int *)malloc(sizeof(int) * (local_n*2+2));

    // generate random number and sort
    for(int i = 0; i < local_n; i++){
        local_arr[i] = rand() % 100;
    }

    // sort number in each process
    bubble_sort(local_arr, local_n);

    MPI_Barrier(MPI_COMM_WORLD);

    // collect the sorted arr from processes to process 0
    int currIdx = 0;
    for(int i = 0; i < numprocs; i++){
        arrNum[i] = n / numprocs;
        if(i < n % numprocs)
            arrNum[i]++;
        startIdx[i] = currIdx;
        currIdx += arrNum[i];
    }
    MPI_Gatherv(local_arr, local_n, MPI_INT, arr, arrNum, startIdx, MPI_INT, 0, MPI_COMM_WORLD);

    // print sorted arr from each process
    if(id == 0) {
        printf("\n");
        for (int i = 0; i < numprocs; i++){
            printf("Sorted arr in process %d: ", i);
            for (int j = 0; j < arrNum[i]; j++) {
                printf("%d, ", arr[startIdx[i] + j]);
            }
            printf("\n");
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    // odd even
    for (int phase = 0; phase < numprocs; phase++) {
        int partner = compute_partner(phase, id);
        if (partner == -1 || partner == numprocs)
            continue;

        // communication
        int num_temp = n / numprocs;
        if (partner < n % numprocs)
            num_temp++;
        if (id % 2 !=0) {
            MPI_Send(local_arr, local_n, MPI_INT, partner, 0, MPI_COMM_WORLD);
            MPI_Recv(recv_arr, num_temp, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else {
            MPI_Recv(recv_arr, num_temp, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(local_arr, local_n, MPI_INT, partner, 0, MPI_COMM_WORLD);
        }

        // merge
        int m_i, r_i, t_i;
        m_i = r_i = t_i = 0;
        while (t_i < local_n + num_temp) {
            if (r_i >= num_temp){
                temp_arr[t_i] = local_arr[m_i];
                t_i++, m_i++;
            }
            else if (m_i >= local_n) {
                temp_arr[t_i] = recv_arr[r_i];
                t_i++, r_i++;
            }
            else if (local_arr[m_i] < recv_arr[r_i]) {
                temp_arr[t_i] = local_arr[m_i];
                t_i++, m_i++;
            }
            else {
                temp_arr[t_i] = recv_arr[r_i];
                t_i++, r_i++;
            }
        }
        for(int j = 0; j < local_n; j++){
            if (id > partner)
                local_arr[j] = temp_arr[num_temp + j];
            else
                local_arr[j] = temp_arr[j];
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    // gather the res
    MPI_Gatherv(local_arr, local_n, MPI_INT, arr, arrNum, startIdx, MPI_INT, 0, MPI_COMM_WORLD);

    if (id == 0) {
        totalTime = MPI_Wtime() - startTime;

        printf("Result: ");
        for (int i = 0; i < n; i++){
            printf("%d, ", arr[i]);
        }
        printf("\n");
        printf("Execution time: %f secs.\n", totalTime);
    }

    //free mem
    free(local_arr);
    free(arr);
    free(recv_arr);
    free(temp_arr);
    free(arrNum);
    free(startIdx);

    MPI_Finalize();
    
    return 0;
}

void bubble_sort(int *a, int n) {
    int list_length, i, temp;

    for (list_length = n; list_length >= 2; list_length--)
        for (i = 0; i < list_length - 1; i++)
            if (a[i] > a[i + 1]) {
                temp = a[i];
                a[i] = a[i + 1];
                a[i + 1] = temp;
            }
}

int compute_partner(int phase, int id) {
    int partner;

    if (phase % 2 == 0) {
        if (id % 2 != 0)
            partner = id - 1;
        else
            partner = id + 1;
    }
    else {
        if (id % 2 != 0)
            partner = id + 1;
        else
            partner = id - 1;
    }
    return partner;
}
