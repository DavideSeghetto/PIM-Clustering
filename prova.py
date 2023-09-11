from matplotlib import pyplot as plt
from matplotlib.ticker import FormatStrFormatter


#NR_DPUS = [1, 4, 16, 64]
#dpu_kernel = [472.5972, 213.608, 137.5486, 141.809]
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

NR_DPUS = [1, 4, 16, 64]
dpu_kernel = [472.5972, 213.608, 137.5486, 141.809]

plt.plot(NR_DPUS, dpu_kernel, color='red')
plt.scatter(NR_DPUS, dpu_kernel, color='red')

plt.title("Curva del DPU kernel")
plt.xlabel("NR_DPUS")
plt.ylabel("DPU-KERNEL")

# Trova l'indice del valore massimo e minimo
max_index = dpu_kernel.index(max(dpu_kernel))
min_index = dpu_kernel.index(min(dpu_kernel))

# Aggiungi l'etichetta del valore massimo
plt.annotate("(" + str(NR_DPUS[max_index]) + ", " + str(dpu_kernel[max_index]) + ")", xy=(NR_DPUS[max_index], dpu_kernel[max_index]), xytext=(-10, 5),
             textcoords='offset points', color='black')

# Aggiungi l'etichetta del valore minimo
plt.annotate("(" + str(NR_DPUS[max_index]) + ", " + str(dpu_kernel[min_index]) + ")", xy=(NR_DPUS[min_index], dpu_kernel[min_index]), xytext=(-10, 20),
             textcoords='offset points', color='black')

plt.grid()
plt.show()
