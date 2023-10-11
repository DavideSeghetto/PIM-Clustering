import os
import sys
import random
import re


rootdir = "/home/deid/tesi/PIM-Clustering" #Path della repo "Clustering" sul dispositivo

#-B in makefile è usata per forzare la ricompilazione di tutti i target a prescindere che siano stati modificati o meno dall'ultima compilazione
# w = warmpup   e = executions    p = dataset path    n = # of points    k = # number of centers    d = dimension of points   r if first point random
applications = {"KCC"   : ["NR_DPUS=X NR_TASKLETS=W BL=Z TYPE=F make -B all", "./bin/host_app -w 1 -e 3 -n #points -k #centers -d #dimensions -r"],
                "DS"    : ["NR_DPUS=X NR_TASKLETS=W BL=Z TYPE=F make -B all", "./bin/host_app -w 1 -e 5 -p #ds-path -k #centers -r"]}

types = ["INT32", "INT64", "FLOAT", "DOUBLE"]

def run_app(app_name, run_type):

    if (app_name not in applications):
        print("Application: '" + app_name + "' not avaible.")
        print("Avaible applications:")
        for key, _ in applications.items():
            print(key)
        return

    if (run_type not in types):
        print("Run type: '" + run_type + "' not avaible.")
        print("Avaible types:")
        for t in types:
            print(t)
        return
        
    NR_DPUS = [1, 4, 16, 64]
    NR_TASKLETS = [1, 2, 4, 8, 16]
    BLOCK_LENGTH = ["1024", "512", "256", "128", "64"]

    make_cmd = applications[app_name][0]
    run_cmd_print = applications[app_name][1]

    os.chdir(rootdir + "/" + app_name)
    
    os.system("make clean")
    
    os.mkdir(rootdir + "/" + app_name + "/bin")
    os.mkdir(rootdir + "/" + app_name + "/profile")


    for d in NR_DPUS:
        for i, t in enumerate(NR_TASKLETS):
            m = make_cmd.replace("X", str(d))
            m = m.replace("W", str(t))
            m = m.replace("Z", BLOCK_LENGTH[i])
            m = m.replace("F", run_type)
            
            os.system(m)

            if (d == 1 or d == 4):
                n_points =  random.randint(1000, 2000)
                n_centers = random.randint(5, 15)
                #n_dimensions = random.randint(1,5)
                n_dimensions = 7
            else:
                n_points =  random.randint(1000, 10000)
                n_centers = random.randint(5, 15)
                #n_dimensions = random.randint(1,5)
                n_dimensions = 7

            run_cmd_print = run_cmd_print.replace("#points", str(n_points))
            run_cmd_print = run_cmd_print.replace("#centers", str(n_centers))
            run_cmd_print = run_cmd_print.replace("#dimensions", str(n_dimensions))
            
            file_name = rootdir + "/" + app_name + "/profile/results_dpu"+str(d)+"_tl"+str(t)+".txt"

            f = open(file_name, "a")
            f.write("Allocated " + str(d) + " DPU(s)\n")
            f.write("NR_TASKLETS " + str(t) + " BLOCK_LENGTH " + BLOCK_LENGTH[i] + "\n")
            f.write("Params: -n " + str(n_points) + " -k " + str(n_centers) + " -d " + str(n_dimensions) + "\n\n")
            f.close()

            print("Running: " + run_cmd_print + f" DPUs {d} TASKLET {t} and BLOCK LENGTH {BLOCK_LENGTH[i]}\n")
            r_cmd = run_cmd_print + " >> " + file_name
            os.system(run_cmd_print)


NR_DPUS = [1, 2, 5, 10, 15, 20, 30, 40, 50, 55, 64]
NR_TASKLETS = [24]
BLOCK_LENGTH = ["128"]

def run_ds(ds, run_type, centers):
    if (run_type not in types):
        print("Run type: '" + run_type + "' not avaible.")
        print("Avaible types:")
        for t in types:
            print(t)
        exit(1)

    make_cmd = applications["DS"][0]
    run_cmd_print = applications["DS"][1]

    os.chdir(rootdir + "/DS")
    
    os.system("make clean")
    
    os.mkdir(rootdir + "/DS/bin")
    os.mkdir(rootdir + "/DS/profile")


    for d in NR_DPUS:
        for t in NR_TASKLETS:
            for i in BLOCK_LENGTH:
            
                m = make_cmd.replace("X", str(d))
                m = m.replace("W", str(t))
                m = m.replace("Z", str(i))
                m = m.replace("F", run_type)
                os.system(m)
                
                file_name = rootdir + "/DS/profile/ds_results.txt"
    
                f = open(file_name, "a")
                f.write("Allocated " + str(d) + " DPU(s)\n")
                f.write("NR_TASKLETS " + str(t) + " BLOCK_LENGTH " + str(i) + "\n")
                f.write("Centers -k " + str(centers) + "\n")
                f.write("Dataset path: " + ds + "\n\n")
                f.close()
    
                run_cmd_print = run_cmd_print.replace("#centers", centers)
                run_cmd_print = run_cmd_print.replace("#ds-path", ds)
    
                print("Running: " + run_cmd_print + f" DPUs {d} TASKLET {t} and BLOCK LENGTH {i}\n")
                run_cmd = run_cmd_print + " >> " + file_name
                os.system(run_cmd)


def main():
    app = sys.argv[1]

    if (len(sys.argv) != 3 and app == "KCC"):
        print("Usage: python3 " + sys.argv[0] + " " + app + " 'type'")
        exit(1)
    
    elif (len(sys.argv) != 5 and app == "DS"):
        print("Usage: python3 " + sys.argv[0] + " " + app + " 'ds-path' 'type' '# centers'")
        exit(1)
    
    if (app == "DS"):
        run_ds(sys.argv[2], sys.argv[3], sys.argv[4])

        cpu_dpu = []
        dpu_kernel = []
        dpu_cpu_and = []
        totale = []
        cpu = []

        pattern_line = re.compile(r"\bCPU-DPU: ")
        pattern_data = re.compile(r"\d+[.]\d+")

        with open ("/home/deid/tesi/PIM-Clustering/DS/profile/ds_results.txt") as file:
            for line in file:
                if pattern_line.search(line) != None:
                    lista = pattern_data.findall(line)
                    if len(lista):
                        cpu_dpu.append(float(lista[0]))
                        dpu_kernel.append(float(lista[1]))  
                        dpu_cpu_and.append(float(lista[2]))
                        totale.append(float(lista[3]))
                        cpu.append(float(lista[4]))
        
        with open ("/home/deid/tesi/PIM-Clustering/DS/profile/ds_results.txt", "a") as file:
            file.write("CPU-DPU\n")
            i = 0
            for data in cpu_dpu:
                if i % 10 == 0:
                    file.write("\n")
                file.write(f"{data}\t")
                i = i + 1
            file.write("\n--------------------------------------------------\n\n")
            
            file.write("DPU-Kernel\n") 
            i = 0
            for data in dpu_kernel:
                if i % 10 == 0:
                    file.write("\n")
                file.write(f"{data}\t")
                i = i + 1
            file.write("\n--------------------------------------------------\n\n")
            
            file.write("DPU-CPU e centri finali\n")
            i = 0
            for data in dpu_cpu_and:
                if i % 10 == 0:
                    file.write("\n")
                file.write(f"{data}\t")
                i = i + 1
            file.write("\n--------------------------------------------------\n\n")

            file.write("Tempo totale\n")
            i = 0
            for data in totale:
                if i % 10 == 0:
                    file.write("\n")
                file.write(f"{data}\t")
                i = i + 1
            file.write("\n--------------------------------------------------\n\n")

            file.write("CPU\n")
            i = 0
            for data in cpu:
                if i % 10 == 0:
                    file.write("\n")
                file.write(f"{data}\t")
                i = i + 1
            file.write("\n--------------------------------------------------\n\n")
        
        print(f"I tempi di CPU_DPU sono:\n{cpu_dpu}\n")
        print(f"I tempi di dpu_kernel sono:\n{dpu_kernel}\n")
        print(f"I tempi di DPU_CPU_AND sono:\n{dpu_cpu_and}\n")
        print(f"I tempi del totale sono:\n{totale}\n")
        print(f"I tempi di cpu sono:\n{cpu}")
    else:
        run_app(app, sys.argv[2])
    

if __name__ == "__main__":
    main()