#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <dpu.h>


#include "../support/params.h"
#include "../support/common.h"
#include "../support/timer.h"

#define DPU_BINARY1 "./bin/dpu_code1"
#define DPU_BINARY2 "./bin/dpu_code2"

/*FARTHEST FIRST TRAVERSAL ALGORITHM
L'input è l'insieme P di N punti da uno spazio metrico (M, d) e un intero k >= 1. L'output invece è un insieme S di k centri che è una buona soluzione per il
problema del k center. L'algoritmo inizia scegliendo un punto arbitrario dal dataset come primo centro. Da questo scelgo il punto più lontano che sarà il mio secondo
centro. Da ora in avanti scelgo come centro il punto con distanza maggiore dai centri già scelti (vedi slide 48) finchè i centri non sono k.
L'algoritmo aumenta di un fattore due la soluzione ottima per il problema del k center clustering. RICORDA DI SCRIVERE NELLA TESI che la fft è molto sensibile a 
dataset con rumore (OUTLIERS). La FFT richiede k - 1 scan del dataset P: impraticabile per P massivi e k non piccoli. Come possiamo quindi dare una buona soluzione
al problema del k center quando abbiamo un dataset troppo grande per una singola macchina? Utilizziamo la composable coreset techinque. Suddividiamo il dataset in
l coreset e a ciascuno applichiamo la FFT. Da ciascun coreset ottengo k centri. Li riunisco tutti in un sottoinsieme T (con l per k punti) di P al quale applico
per l'ultima volta la FFT. Versione di FFT nota come MapReduce FFT. Tale versione aumenta di un fattore 4 la soluzione ottima del problema k center. La MRFFT la si
dovrebbe usare su massivi dataset*/

/*Il k-center clustering è un problema di ottimizzazione. La soluzione è un sottoinsieme dell'intero dataset, che contiene k punti corrispondenti ai vari centri
L'obiettivo dell'algoritmo è minimizzare la distanza massima tra un punto appartenente ad un cluster e il centro di quel cluster
L'algoritmo consiste in un dataset al quale viene applicato l'algoritmo k center clustering. Per dataset di grandi dimensioni suddivido il dataset tra le DPU e su ciascuna
applico l'algoritmo. Sulla CPU faccio la stessa cosa per un controllo di velocità e inoltre applico l'algoritmo all'intero dataset senza spezzettarlo.
Descrizione dei risultati ottenuti:
DPU cost: costo dell'algoritmo spezzettando il dataset
Linear cost: costo dell'algoritmo facendolo linearmente sulla CPU
L'approccio lineare, ossia senza spezzettare il dataset deve avere un costo minore, deve essere cioè più preciso perchè appunto non applico "approssimazioni" al dataset 
(spezzettamenti). Il problema è NP Hard (ricontrolla questa terminologia), quindi si può risolvere solo in un tempo esponenziale. L'algoritmo lineare K center clustering
è quindi un'approssimazione. Se ad esempio il risultato ottimo del problema è 15, l'approccio lineare mi darà nel migliore dei casi un risultato pari a 30. Quindi
la correttezza diminuisce di un fattore 2. Mentre con l'approccio fft (spezzettato) la correttezza diminuisce di un fattore 4 (arrivo nel migliore dei casi a 60).
In sostanza il costo, più piccolo è meglio è. Il costo dell'algoritmo è la distanza più grande di un punto dal suo centro (spiegazione minuto 39 - 40 audio).
CPU-DPU è il tempo di caricamento dei dati nelle DPU
DPU Kernel è il tempo di esecuzione dell'algoritmo spezzettato nelle DPU
DPU - CPU e centri finali è il tempo di spostamento dei centri calcolati nelle DPU alla CPU e il calcolo dell'ultima passata nella CPU per il calcolo finale dei centri
Il tempo totale dell'algoritmo spezzettato (che quindi include il caricamento dei dati nelle DPU, l'esecuzione nelle varie DPU, lo spostamento dei centri intermedi nella CPU
e l'esecuzione dell'ultima passata nella CPU) è la somma di questi tre tempi
CPU invece è il tempo per fare l'algoritmo sempre con spezzettamento ma nella CPU (ovviamente stessi parametri, stesso dataset, stesse partizioni)
Il tempo di esecuzione dell'algoritmo completo senza spezzettamento nella CPU non è calcolato, si potrebbe aggiungere se utile
Outputs are equal mi indica che l'algoritmo runnato sulle DPU e quello runnato sulle CPU ha dato lo stesso risultato, ovvero gli stessi centri*/

//Buffer contenente tutti i punti.
static T* P; 

//Buffer per i centri calcolati dalle dpu.
static T* C; //Buffer centri finali dpu. C LO OTTENGO APPLICANDO FFT AD R
static T* R; //Buffer centri INTERMEDI calcolati da ogni dpu.

//Buffer per i centri calcolati dall'host per verificare i risultati.
static T* H; //Buffer centri finali host.
static T* M; //Buffer "centri intermedi" host.


//Legge i valori dal database fornito.
static void init_dataset(T* points_buffer, char *path) {
    FILE *ds = fopen(path, "r");

    int i = 0;
    char row[100];
    char *num;

    while (feof(ds) != true) {
        num = fgets(row, 100, ds);
        num = strtok(row, ",");
        while(num != NULL) {
            if(i < 20) printf("Il punto è: %s\t", num);
            points_buffer[i++] = atoi(num);

            num = strtok(NULL, ",");
        }
        if(i < 21) printf("\n");

    }  
    #if defined FLOAT || defined DOUBLE 
        for(int i = 0; i < 20; i += 2) printf("Punto %d: %f, %f\n\n", i + 1, points_buffer[i], points_buffer[i + 1]);
    #elif defined INT32
        for(int i = 0; i < 20; i += 2) printf("Punto %d: %d, %d\n\n", i + 1, points_buffer[i], points_buffer[i + 1]);
    #elif defined INT64
        for(int i = 0; i < 20; i += 2) printf("Punto %d: %ld, %ld\n\n", i + 1, points_buffer[i], points_buffer[i + 1]);
    #endif
    
    fclose(ds);
}


//Estrae "n_points" (k) centri da "points_buffer" (insieme di punti che gli passo) inserrendoli in "centers_buffer".
static void get_centers(T* point_buffer, T* centers_buffer, uint32_t n_points, uint32_t n_centers, uint32_t dim, uint32_t first_offset) {
    
    //Scelgo "a caso" il primo centro.
    uint32_t n_centers_found = 1;
    for (unsigned int i = 0; i < dim; i++) {
        centers_buffer[i] = point_buffer[i + first_offset];
    }

    while (n_centers_found < n_centers) {
        D candidate_center_dist = 0;

        for (unsigned int i = 0; i < n_points; i++) {
            uint32_t point_index = i*dim;
            D min_center_dist = INIT_VAL;
            
            for(unsigned int j = 0; j < n_centers_found; j++) {
                uint32_t center_index = j*dim;
                D dist = 0;
            
                for (unsigned int k = 0; k < dim; k++) {
                    //CALCOLO DISTANZA TRA DUE PUNTI. IL FATTO DI AVER MODIFICATO IL CALCOLO DELLA DISTANZA MI PERTMETTE DI AVERE DATASET CON DIMENSIONI MOLTO PIÙ GRANDI
                    dist += distance(point_buffer[point_index+k], centers_buffer[center_index+k]); //TODO: non gestisce overflow.
                }

                min_center_dist = (dist < min_center_dist) ? dist : min_center_dist;
            }

            if (candidate_center_dist <= min_center_dist) {
                candidate_center_dist = min_center_dist;
                for (unsigned int k = 0; k < dim; k++) {
                    centers_buffer[n_centers_found*dim + k] = point_buffer[point_index+k];
                }
            }
        }

        n_centers_found++;
    }

}

//Esecuzione dell'algoritmo spezzettato solo sulla CPU. NON CALCOLO IL COSTO PERCHÈ SE RESTITUISCE GLI STESSI CENTRI CHE RESTITUISCONO LE DPU IL COSTO È UGUALE A QUELLO 
//CALCOLATO DALLE DPU
//Calcola i centri, partendo dagli stessi sottoinsiemi di punti assegnati alle DPU.
static void k_clustering_host(uint32_t n_point_dpu, uint32_t n_points_last_dpu, uint32_t n_centers, uint32_t dim, uint32_t first_offset) {

    //Calcolo i centri dai vari sottoinsiemi di P.
    uint32_t points_offset = 0;
    uint32_t centers_set_offset = 0;
    for (int i = 0; i < NR_DPUS-1; i++) {
        points_offset = i*n_point_dpu*dim;
        centers_set_offset = i*n_centers*dim;

        get_centers(P + points_offset, M + centers_set_offset, n_point_dpu, n_centers, dim, first_offset);
    }
    points_offset = (NR_DPUS-1)*n_point_dpu*dim;
    centers_set_offset = (NR_DPUS-1)*n_centers*dim;
    //CALCOLO DEI CENTRI DALL'ULTIMO INSIEME DI PUNTI CHE POTREBBE AVERE PARAMETRI DIVERSI NEL CASO IN CUI IL NUMERO DI PUNTI NON SIA MULTIPLO DEL NUMERO DI DPU
    get_centers(P + points_offset, M + centers_set_offset, n_points_last_dpu, n_centers, dim, first_offset);
    
    //Calcolo i centri finali (ULTIMA PASSATA)
    get_centers(M, H, n_centers*NR_DPUS, n_centers, dim, 0);
}

//Calcola il costo del clustering effettuato linearmente su tutti i punti. EFFETTUA L'ALGORITMO SENZA SPEZZETTAMENTO E CALCOLO IL COSTO
static D get_linear_cost(uint32_t n_points, uint32_t n_centers, uint32_t dim, uint32_t first_offset) {
    
    T centers_set[n_centers*dim];

    get_centers(P, centers_set, n_points, n_centers, dim, first_offset);

    D clustering_cost = 0;
    for (unsigned int i = 0; i < n_points; i++) {
        uint32_t point_index = i*dim;
        D min_center_dist = INIT_VAL;
        
        for(unsigned int j = 0; j < n_centers; j++) {
            uint32_t center_index = j*dim;
            D dist = 0;
            D temp = 0;
        
            for (unsigned int k = 0; k < dim; k++) {
                temp = distance(P[point_index+k], centers_set[center_index+k]); //TODO: non gestisce overflow.
                //dist += distance2(P[point_index+k], centers_set[center_index+k], i); //TODO: non gestisce overflow.
                dist += temp;
                //if(i < 10) {
                    #if defined (FLOAT) || defined (DOUBLE)
                        printf("Le coordinate che uso per la distanza sono: %f e %f\n", P[point_index+k], centers_set[center_index+k]);
                        printf("La distanza trovata per queste due coordinate è: %f\n", temp);
                    
                    #elif defined INT32
                        printf("Le coordinate che uso per la distanza sono: %d e %d\n", P[point_index+k], centers_set[center_index+k]);
                        printf("La distanza trovata per queste due coordinate è: %ld\n", temp);
                    
                    #elif defined INT64
                        printf("Le coordinate che uso per la distanza sono: %ld e %ld\n", P[point_index+k], centers_set[center_index+k]);
                        printf("La distanza trovata per queste due coordinate è: %ld\n", temp);
                    
                    #endif
                //}
            }
            //if(i < 10) 
            printf("La distanza trovata è: %lu\n\n", dist);
            min_center_dist = (dist < min_center_dist) ? dist : min_center_dist;
        }

        if (clustering_cost <= min_center_dist) clustering_cost = min_center_dist;
    }
    #ifdef FLOAT
        printf("Il costo del clustering lineare è: %f\n\n", clustering_cost);
    #else
        printf("Il costo del clustering lineare è: %lu\n\n", clustering_cost);
    #endif
    return clustering_cost;
}


int main(int argc, char **argv) {

    //Carico i parametri nella struttura
    //Parametri per l'esecuzione del benchmark: #punti, #centri, #dimensione dello spazio e #ripetizioni efettuate.
    struct Params p = input_params(argc, argv);
    if (p.n_points == 0) return -1; //Il file non è stato aperto correttamente.

    struct dpu_set_t dpu_set, dpu;

    Timer timer;

    //Alloco le DPU.
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));

    //Suddivisione punti per ogni DPU.
    //La dimensione del blocco assegnato ad ogni DPU deve essere allineata su 8 bytes.
    uint32_t points_per_dpu = p.n_points/NR_DPUS;
    uint32_t points_per_last_dpu = p.n_points-points_per_dpu*(NR_DPUS-1);
    uint32_t mem_block_per_dpu = points_per_last_dpu*p.dim;
    uint32_t mem_block_per_dpu_8bytes = ((mem_block_per_dpu*sizeof(T) % 8) == 0) ? mem_block_per_dpu : roundup(mem_block_per_dpu, 8);

    //Inizializzo buffer dei punti usato dalle DPU e successivamente dall'host.
    P = malloc(points_per_dpu*p.dim*sizeof(T)*(NR_DPUS-1) + mem_block_per_dpu_8bytes*sizeof(T));
    init_dataset(P, p.ds);
    
    //Indici usati per accedere in MRAM e recuperare i centri intermedi dalle DPU.
    //La dimensione del blocco che vado a copiare deve essere allineata su 8 bytes.
    uint32_t dpu_center_set_addr = mem_block_per_dpu_8bytes*sizeof(T);
    uint32_t data_length = p.n_centers*p.dim;
    uint32_t data_length_8bytes = ((data_length*sizeof(T)) % 8 == 0) ? data_length : roundup(data_length, 8);
    
    //Inizializzo buffer per contenere i centri intermedi ed i centri finali delle DPU.
    R = malloc(data_length_8bytes*sizeof(T)*NR_DPUS);
    C = malloc(data_length*sizeof(T));

    //Inizializzo i buffer per contenere i centri intermedi ed i centri finali dell'host.
    H = malloc(p.n_centers*p.dim*sizeof(T));
    M = malloc(p.n_centers*p.dim*sizeof(T)*NR_DPUS);

    //Parametri per le varie DPU.
    struct dpu_arguments_t  input_arguments[NR_DPUS];
    for (int i = 0; i < NR_DPUS - 1; i++) {
        input_arguments[i].n_points_dpu_i = points_per_dpu;
        input_arguments[i].n_centers = p.n_centers;
        input_arguments[i].dim = p.dim;
        input_arguments[i].mem_size = mem_block_per_dpu_8bytes*sizeof(T);
    }
    input_arguments[NR_DPUS-1].n_points_dpu_i = points_per_last_dpu;
    input_arguments[NR_DPUS-1].n_centers = p.n_centers;
    input_arguments[NR_DPUS-1].dim = p.dim;
    input_arguments[NR_DPUS-1].mem_size = mem_block_per_dpu_8bytes*sizeof(T);

    //Eseguo più volte il calcolo dei centri misurando il tempo di esecuzione.
    for (unsigned int rep = 0; rep < p.n_warmup + p.n_reps; rep++) {

        //Indice randomico del primo centro di ogni DPU. Forzo un multiplo di 2 per allineamento su 8 bytes. 
        srand(time(NULL));
        uint32_t index = rand()%(points_per_last_dpu/2)*2;
        uint32_t offset = p.rnd_first*index*p.dim;
        for (unsigned int i = 0; i < NR_DPUS; i++) {
            input_arguments[i].first_center_offset = offset*sizeof(T);
        }

        //Cronometro caricamento dati CPU-DPU.
        if (rep >= p.n_warmup) {
            start(&timer, 0, rep - p.n_warmup);
        }

        //Carico il programma per calcolare i centri.
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY1, NULL));
        
        //Carico i parametri di input per ogni DPU.
        unsigned int i = 0;
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(input_arguments[0]), DPU_XFER_DEFAULT));
        
        //Carico l'insieme di punti per ogni DPU.
        i = 0;
        DPU_FOREACH(dpu_set, dpu, i) {
                DPU_ASSERT(dpu_prepare_xfer(dpu, P + i*points_per_dpu*p.dim));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, mem_block_per_dpu_8bytes*sizeof(T), DPU_XFER_DEFAULT));

        //Stop timer CPU-DPU
        //Cronometro tempo esecuzione nelle DPU.
        if (rep >= p.n_warmup) {
            stop(&timer, 0);
            start(&timer, 1, rep - p.n_warmup);
        }

        //Avvio l'esecuzione delle DPU.
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        
        //Stop timer tempo esecuzione nelle DPU.
        //Cronometro trasferimento DPU-CPU e calcolo centri finali.
        if (rep >= p.n_warmup) {
            stop(&timer, 1);
            start(&timer, 2, rep - p.n_warmup);
        }

        //Recupero i dati.
        i = 0;
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, R + i*data_length_8bytes));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, dpu_center_set_addr, data_length_8bytes*sizeof(T), DPU_XFER_DEFAULT));
        
        if (data_length*sizeof(T) % 8 != 0) {
            //Devo ricompattare il buffer.    
            for (unsigned int i = 0; i < NR_DPUS; i++) {
                for (unsigned int j = 0; j < data_length; j++) {
                    R[i*data_length + j] = R[i*data_length_8bytes + j];
                }
            }
        }

        //Calcolo i centri finali partendo dai centri trovati dalle DPU.
        get_centers(R, C, p.n_centers*NR_DPUS, p.n_centers, p.dim, 0);

        //Stop timer trasferimento DPU-CPU e calcolo centri finali.
        //Cronometro esecuzione algoritmo su host.
        if (rep >= p.n_warmup) {
            stop(&timer, 2);
            start(&timer, 3, rep - p.n_warmup);
        }

        //Calcolo dei centri finali da parte dell'host.
        k_clustering_host(points_per_dpu, points_per_last_dpu, p.n_centers, p.dim, offset);

        //Stop timer esecuzione algoritmo su host.
        if (rep >= p.n_warmup) {
            stop(&timer, 3);
        }
        
        //DA QUI IN POI INIZIO IL CALCOLO DEL COSTO DEL CLUSTERING CON SPEZZETTAMENTO SU DPU
        //Carico il programma per calcolare il costo del clustering.
        DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY2, NULL));
        
        //Carico i parametri.
        i=0;
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(input_arguments[0]), DPU_XFER_DEFAULT));

        //Carico i centri finali calcolati.
        i = 0;
        uint32_t center_set_size = (p.n_centers*p.dim*sizeof(T) % 8) == 0 ? p.n_centers*p.dim*sizeof(T) : roundup(p.n_centers*p.dim*sizeof(T), 8);
        DPU_FOREACH(dpu_set, dpu, i) {
            //LA COSA INTERESSANTE È CHE OGNI DPU HA ANCORA IN MEMORIA LA SUA PARTIZIONI DI P QUINDI ADESSO BASTA CHE PASSO C E CIASCUNA DPU PUÒ QUINDI FARE IL COSTO
                DPU_ASSERT(dpu_prepare_xfer(dpu, C)); //PRIMA PASSAVO PORZIONE DI P ORA PASSO C PERCHÈ HO GIÀ CALCOLATO I CENTRI MEMORIZZATI C
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, dpu_center_set_addr, center_set_size, DPU_XFER_DEFAULT));
        
        //Avvio l'esecuzione delle DPU.
        DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
        
        //Recupero i costi calcolati.
        i = 0;
        D costs[NR_DPUS];
        DPU_FOREACH(dpu_set, dpu, i) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &costs[i]));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, dpu_center_set_addr, sizeof(D), DPU_XFER_DEFAULT));

        //Calcolo costo del clustering.
        D dpu_cost = 0;
        for (int j = 0; j < NR_DPUS; j++) {
            if (costs[j] > dpu_cost)
                dpu_cost = costs[j];
        }

        //Calcolo il costo del clustering effettuato linearmente su tutti i punti.
        D cpu_cost = get_linear_cost(p.n_points, p.n_centers, p.dim, (rand()%p.n_points)*p.rnd_first);

        //Verifico se i risultati ottenuti in DPUs e CPU sono uguali.
        bool status = true;
        for (unsigned int j = 0; j < p.n_centers; j++) {
                if(!status) break;
                for (unsigned int k = 0; k < p.dim; k++){
                    if (H[j*p.dim + k] != C[j*p.dim + k]) {
                        status = false;
                        break;
                    }
                }
        }
        
        print_res(status, rep, dpu_cost, cpu_cost);
    }

    //Stampo media tempi di esecuzione
    printf("\n\nTempi medi:\n");
    printf("CPU-DPU: ");
    print(&timer, 0, p.n_reps);
    printf("\tDPU Kernel: ");
    print(&timer, 1, p.n_reps);
    printf("\tDPU-CPU e centri finali: ");
    print(&timer, 2, p.n_reps);
    printf("\tCPU: ");
    print(&timer, 3, p.n_reps);

    printf("\n\nCentri finali:\n");
    print_points(C, p.n_centers, p.dim);
    printf("\n--------------------------------------------------\n\n");

    free(H);
    free(M);
    free(C);
    free(R);
    free(P);
    DPU_ASSERT(dpu_free(dpu_set));
    return 0;
}
