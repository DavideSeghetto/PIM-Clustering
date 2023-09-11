from matplotlib import pyplot as plt
from matplotlib.ticker import FormatStrFormatter

NR_DPUS = [10, 13, 20, 23, 50, 53, 99, 100, 103, 199, 200, 203, 499, 500, 503, 999, 1000, 1003, 1999, 2000, 2003, 2519]
dpu_kernel = [853.5068, 656.925, 427.6354, 371.9356, 172.1074, 162.3098, 87.1562, 86.8956, 83.6378, 44.9566, 43.58, 42.4268, 18.1866, 18.1812, 18.1812, 18.1452, 8.9808, 18.1726, 8.997, 6.0426, 26.097, 24.9826]

#plt.yticks(dpu_kernel)
#plt.plot(NR_DPUS, dpu_kernel)
#plt.scatter(NR_DPUS, dpu_kernel, color = "red")
#plt.title("Curva del DPU kernel")
#plt.xlabel("NR_DPUS")
#plt.ylabel("DPU-KERNEL")
#plt.grid()
#plt.xticks([])
#plt.yticks([])
#formatter = FormatStrFormatter('%.4f')
#plt.gca().yaxis.set_major_formatter(formatter)
#plt.show()

plt.plot(NR_DPUS, dpu_kernel, color='red')
plt.scatter(NR_DPUS, dpu_kernel, color='red')

plt.title("Curva del DPU kernel")
plt.xlabel("NR_DPUS")
plt.ylabel("DPU-KERNEL")

# Trova l'indice del valore massimo e minimo di DPU kernel
max_index = dpu_kernel.index(max(dpu_kernel))
min_index = dpu_kernel.index(min(dpu_kernel))

# Trova l'indice del valore massimo di NR_DPU
max_index2 = NR_DPUS.index(max(NR_DPUS))

# Aggiungi l'etichetta del valore massimo
plt.annotate("(" + str(NR_DPUS[max_index]) + ", " + str(dpu_kernel[max_index]) + ")", xy=(NR_DPUS[max_index], dpu_kernel[max_index]), xytext=(-10, 5),
             textcoords='offset points', color='black')

# Aggiungi l'etichetta del valore massimo
plt.annotate("(" + str(NR_DPUS[min_index]) + ", " + str(dpu_kernel[min_index]) + ")", xy=(NR_DPUS[min_index], dpu_kernel[min_index]), xytext=(-60, -20),
             textcoords='offset points', color='black')

# Aggiungi l'etichetta dell'ultimo valore
plt.annotate("(" + str(NR_DPUS[max_index2]) + ", " + str(dpu_kernel[max_index2]) + ")", xy=(NR_DPUS[max_index2], dpu_kernel[max_index2]), xytext=(-60, 20),
             textcoords='offset points', color='black')

plt.grid()
plt.show()
