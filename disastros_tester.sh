#!/bin/bash

# File di output
output_file="output.txt"
quantum=100

# Pulisci il file di output all'inizio
> "$output_file"

# Compila il programma
make

echo -e "\nTesting Disastros...\n"

echo -e "Ran with 4 cores $quantum for quantum\n" >> "$output_file"

# Crea un array per salvare i file temporanei per ogni scheduler
declare -a temp_files

# Esegui il programma per N da 1 a 11 in modo asincrono
for N in {1..11}; do
    # Crea il file temporaneo prima di eseguire il comando in background
    temp_output=$(mktemp) # Crea un file temporaneo per ciascuna esecuzione
    temp_files[$N]=$temp_output  # Salva subito il riferimento al file nell'array

    # Esegui il processo in background
    {
        # Scrivi il numero del scheduler e le statistiche nel file temporaneo
        echo -e "Scheduler: $N" > "$temp_output"
        ./disastros 4 "$N" "$quantum" traces | grep "Turnaround\|Waiting\|Response\|Throughput\|CPU Used" | sed 's/\x1B\[[0-9;]*m//g' >> "$temp_output"
        echo -e "" >> "$temp_output"
    } &
done

# Attendi che tutte le istanze siano terminate
wait

# Una volta terminato, concatenare i file nell'ordine corretto
for N in {1..11}; do
    temp_output="${temp_files[$N]}"
    cat "$temp_output" >> "$output_file"
    rm "$temp_output"  # Rimuovi il file temporaneo dopo averlo scritto nell'output finale
done

# Cancella i file oggetto e l'eseguibile
make clean

# Stampa un messaggio di completamento
echo -e "\nDone. Output stored in $output_file.\n"
