import random
with open("/home/deid/tesi/PIM-Clustering/DS/dataset/int/100k/int_2.csv", "w") as file:
    for i in range(0, 500000):
        lista = []
        for j in range(0, 2):
            n = random.randint(-500, 501)
            #n = round(n, 4)
            lista.append(n)
            file.write(str(n))
            if j != 1:
                file.write(",")
            elif j == 1:
                file.write("\n")

righe_lette = set()

def genera_riga_unica():
    #return f"{random.randint(-500, 501)},{random.randint(-500, 501)},{random.randint(-500, 501)}\n"
    return f"{random.randint(-500, 501)},{random.randint(-500, 501)}\n"

righe_lette = set()
nuove_righe = []

with open("/home/deid/tesi/PIM-Clustering/DS/dataset/int/100k/int_2.csv", "r") as file:
    for riga in file:
        if riga in righe_lette:
            nuova_riga = genera_riga_unica()
            print(f"La riga {riga} è già presente nel file")
            while nuova_riga in righe_lette:
                nuova_riga = genera_riga_unica()
            nuove_righe.append(nuova_riga)
            righe_lette.add(nuova_riga)
        else:
            nuove_righe.append(riga)
            righe_lette.add(riga)

# Sovrascrive il file con le nuove righe
with open("/home/deid/tesi/PIM-Clustering/DS/dataset/int/100k/int_2.csv", "w") as file:
    for riga in nuove_righe:
        file.write(riga)

print("Controllo e sostituzione completati.")
