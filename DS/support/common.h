#ifndef _COMMON_H
#define _COMMON_H

    //Definisco i vari tipi.
    /*#ifdef UINT32
    #define T uint32_t
    #define T_SHIFT 2
    #define D uint64_t
    #elif UINT64
    #define T uint64_t
    #define T_SHIFT 3
    #define D uint64_t*/
    #ifdef INT32
    #define T int32_t
    #define T_SHIFT 2
    #define D uint64_t
    #elif INT64
    #define T int64_t
    #define T_SHIFT 3
    #define D uint64_t
    #elif FLOAT
    #define T float
    #define T_SHIFT 2
    #define D double
    #elif DOUBLE
    #define T double
    #define T_SHIFT 3
    #define D double
    #endif
    //Arrotonda "n" al successivo multiplo di "m".
    #define roundup(n,m) ((n/m)*m + m)

    //Struttura per passaggio parametri tra host e le DPU.
    struct dpu_arguments_t {
        uint32_t n_points_dpu_i;
        uint32_t n_centers;
        uint32_t dim;
        uint32_t mem_size;
        uint32_t first_center_offset;

    };

    D distance(T a, T b) {
        D base = (a >= b) ? a-b : b-a;
        return base * base;
    }

    //Ritorna il multiplo di point_dim più vicino ad BLOCK_SIZE e divisibile per 8.
    uint32_t get_block_size(uint32_t point_dim) {

        if ((BLOCK_SIZE % point_dim) == 0) {
            return BLOCK_SIZE;
        }

        uint32_t lcm = (point_dim > 8) ? point_dim : 8;

        while (true) {
            if ((lcm % 8 == 0) && (lcm % point_dim == 0)) break;

            lcm++;
        }

        for(uint32_t tmp=lcm; lcm < BLOCK_SIZE; lcm+=tmp);

        return lcm;
    }


    //Funzioni adattate al tipo run time.
    #if defined FLOAT
        #define INIT_VAL 18446744073709551616.0

        void print_points(T* points_set, uint32_t n_centers, uint32_t dim) {
        for (unsigned int i=0; i < n_centers; i++) {
                printf("Centro: %d [", i);
                for (unsigned int k = 0; k < dim; k++) {
                    if(k == 0) {
                        printf("%f", points_set[i*dim + k]);
                        continue;
                    }
                    printf(", %f", points_set[i*dim + k]);
                }
                printf("]\n");
            }
        }

        //Wrapper di printf per stampare i risultati finali.
        void print_res(bool s, unsigned int r, double dpu_cost, double cpu_cost) {
            if(s) printf("Rep %d: Outputs are equal\t DPU cost: %f\t Linear cost: %f\n", r, dpu_cost, cpu_cost);
            else  printf("Rep %d: Outputs are different!\t DPU cost: %f\t Linear cost: %f\n", r, dpu_cost, cpu_cost);
        }

    #elif defined DOUBLE
        #define INIT_VAL 18446744073709551616.0

        void print_points(T* points_set, uint32_t n_centers, uint32_t dim) {
        for (unsigned int i=0; i < n_centers; i++) {
                printf("Centro: %d [", i);
                for (unsigned int k = 0; k < dim; k++) {
                    if(k == 0) {
                        printf("%.14lf", points_set[i*dim + k]);
                        continue;
                    }
                    printf(", %.14lf", points_set[i*dim + k]);
                }
                printf("]\n");
            }
        }

        //Wrapper di printf per stampare i risultati finali.
        void print_res(bool s, unsigned int r, double dpu_cost, double cpu_cost) {
            if(s) printf("Rep %d: Outputs are equal\t DPU cost: %.14lf\t Linear cost: %.14lf\n", r, dpu_cost, cpu_cost);
            else  printf("Rep %d: Outputs are different!\t DPU cost: %.14lf\t Linear cost: %.14lf\n", r, dpu_cost, cpu_cost);
        }

    #elif defined INT32
        #define _INIT_VAL_(c) c ## ULL
        #define INIT_VAL (_INIT_VAL_(18446744073709551615))

        void print_points(T* points_set, uint32_t n_centers, uint32_t dim) {
        for (unsigned int i=0; i < n_centers; i++) {
                printf("Centro: %d [", i);
                for (unsigned int k = 0; k < dim; k++) {
                    if(k == 0) {
                        printf("%d", points_set[i*dim + k]);
                        continue;
                    }
                    printf(", %d", points_set[i*dim + k]);
                }
                printf("]\n");
            }
        }

        //Wrapper di printf per stampare i risultati finali.
        void print_res(bool s, unsigned int r, uint64_t dpu_cost, uint64_t cpu_cost) {
            if(s) printf("Rep %d: Outputs are equal\t DPU cost: %ld\t Linear cost: %ld\n", r, dpu_cost, cpu_cost);
            else  printf("Rep %d: Outputs are different!\t DPU cost: %ld\t Linear cost: %ld\n", r, dpu_cost, cpu_cost);
        }

    #elif defined INT64
        #define _INIT_VAL_(c) c ## ULL
        #define INIT_VAL (_INIT_VAL_(18446744073709551615))

        void print_points(T* points_set, uint32_t n_centers, uint32_t dim) {
        for (unsigned int i=0; i < n_centers; i++) {
                printf("Centro: %d [", i);
                for (unsigned int k = 0; k < dim; k++) {
                    if(k == 0) {
                        printf("%ld", points_set[i*dim + k]);
                        continue;
                    }
                    printf(", %ld", points_set[i*dim + k]);
                }
                printf("]\n");
            }
        }

        //Wrapper di printf per stampare i risultati finali.
        void print_res(bool s, unsigned int r, uint64_t dpu_cost, uint64_t cpu_cost) {
            if(s) printf("Rep %d: Outputs are equal\t DPU cost: %ld\t Linear cost: %ld\n", r, dpu_cost, cpu_cost);
            else  printf("Rep %d: Outputs are different!\t DPU cost: %ld\t Linear cost: %ld\n", r, dpu_cost, cpu_cost);
        }

    #endif

#endif