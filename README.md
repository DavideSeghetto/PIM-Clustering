# Note sul codice

Il programma esegue l'algoritmo di Map Reduce Farthest-First Traversal per la risoluzione del problema del k-center clustering su un dataset di punti. È possibile scegliere tra due modalità di esecuzione
1. ```KCC```: il valore dei vari punti, la loro dimensione ed il numero di centri da estrarre sono generati automaticamente.<br>
2. ```DS```: il dataset da utilizzare e il numero di centri da estrarre viene passato da riga di comando.
I centri vengono calcolati sia dalle DPU che dall'host application così da poter verificare la correttezza dei valori ottenuti.

La struttura generale del codice segue le linee guide indicate nei [benchmark](https://github.com/CMU-SAFARI/prim-benchmarks) prodotti da SAFARI Research Group.<br>
La cartella host contiene il codice eseguito nella CPU centrale mentre la cartella dpu contiene il codice che viene eseguito nelle memorie.  

Vengono effettuati vari test istanziando un diverso numero di DPU e di tasklets.<br>
I risultati di ogni test sono riportati nella cartella ```/profile``` creata quando si esegue per la prima volta un'applicazione.
Vengono in seguito mostrati i grafici relativi ai tempi di trasmissione dati da CPU a DPUs e viceversa e dei tempi di esecuzione dell'algoritmo.

# Istruzioni per compilare ed eseguire

Per eseguire il programma da terminale:
1. Installare la [libreria fornita da upmem](https://sdk.upmem.com/).
2. Scaricare ed aprire da terminale la repo sovrastante.
3. Eseguire con ```source``` lo script ```upmem_env.sh``` contenuto nella libreria upmem specificando il parametro ```simulator```. Esempio:
   ```
   source ~/Scrivania/upmem-sdk/upmem_env.sh simulator
   ```
4. Modificare la varibile ```rootdir``` all'interno di ```run.py``` con il path della repo nel proprio dispositivo.
5. Per eseguire il programma:
   + su un dataset di punti casuali eseguire lo script ```run.py``` specificando l'applicazione da eseguire e la modalità d'esecuzione:
   ```
   python3 run.py KCC UINT32
   ```
   + su un database di punti già esistente eseguire lo script come indicato a seguire:
    ```
   python3 run.py DB 'full-path-to-db' 'type of dataset' '# of centers to extract'
   ```
   Il database in questione deve contenere un punto per ogni riga, con cordinate separate da virgole. Sono supportati valori di tipo ```float```.
