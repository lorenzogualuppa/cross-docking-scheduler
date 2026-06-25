#!/usr/bin/env bash
# =============================================================================
# run_all.sh — Esegue tutte le istanze .dzn trovate nella cartella Instances/
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTANCES_DIR="${SCRIPT_DIR}/Instances"
OUTPUT="${SCRIPT_DIR}/results.txt"
LOG_FILE="${SCRIPT_DIR}/run_all.log"

# Trova il binario C++ compilato
BINARY=""
for candidate in "${SCRIPT_DIR}/SourceFiles/CD_Test" "${SCRIPT_DIR}/SourceFiles/CD_Test.exe" "${SCRIPT_DIR}/SourceFiles/cd_test"; do
    if [ -x "$candidate" ]; then
        BINARY="$candidate"
        break
    fi
done

if [ -z "$BINARY" ]; then
    echo "[ERRORE] Nessun binario trovato in SourceFiles/. Compila prima con: make"
    exit 1
fi

# Inizializza file di output
{
    echo "============================================================"
    echo "  Cross-Docking Scheduler — Risultati Test"
    echo "============================================================"
    printf "%-45s %8s %8s %8s %8s %8s\n" "Istanza" "LB" "Makespan" "Gap(%)" "WaitIn" "WaitOut"
    echo "--------------------------------------------------------------------------------------------"
} > "$OUTPUT"

> "$LOG_FILE"
TOT_OK=0
TOT_FAIL=0

echo "Inizio simulazione..."

# Esegue ogni file .dzn trovato nella cartella in ordine alfabetico
for INSTANCE in $(ls "$INSTANCES_DIR"/*.dzn | sort); do
    BASENAME=$(basename "$INSTANCE" .dzn)
    
    # Esegue il programma C++ e cattura l'output
    RAW=$("$BINARY" "$INSTANCE" 2>/dev/null) || STATUS=$?
    STATUS=${STATUS:-0}

    if [ "$STATUS" -ne 0 ] || [ -z "$RAW" ]; then
        printf "  [ERR]  %-43s esecuzione fallita\n" "${BASENAME}" | tee -a "$OUTPUT"
        TOT_FAIL=$((TOT_FAIL + 1))
        continue
    fi

    # Estrapola i valori dall'output del tuo C++
    LB=$(echo "$RAW"       | awk '/Lower Bound/{print $NF}')
    MAKESPAN=$(echo "$RAW" | awk '/Makespan/{print $NF}')
    GAP=$(echo "$RAW"      | awk '/Gap \(%\)/{gsub(/%/,"",$NF); print $NF}')
    WAIT_IN=$(echo "$RAW"  | awk '/Avg Wait In/{print $NF}')
    WAIT_OUT=$(echo "$RAW" | awk '/Avg Wait Out/{print $NF}')

    INIT_ARR=$(echo "$RAW" | grep -i "^Initial Arrival" || true)
    IN_SEQ=$(echo "$RAW" | grep -i "^Inbound sequence" || true)
    IN_DOOR=$(echo "$RAW" | grep -i "^Inbound doors" || true)
    OUT_SEQ=$(echo "$RAW" | grep -i "^Outbound sequence" || true)
    OUT_DOOR=$(echo "$RAW" | grep -i "^Outbound doors" || true)

    # Stampa i risultati formattati
    printf "  %-45s %8s %8s %8s %8s %8s\n" "${BASENAME}" "${LB:-???}" "${MAKESPAN:-???}" "${GAP:-???}" "${WAIT_IN:-???}" "${WAIT_OUT:-???}" | tee -a "$OUTPUT"
    
    # Mostra le sequenze
    echo "      ${INIT_ARR}" | tee -a "$OUTPUT"
    echo "      ${IN_SEQ}" | tee -a "$OUTPUT"
    echo "      ${IN_DOOR}" | tee -a "$OUTPUT"
    echo "      ${OUT_SEQ}" | tee -a "$OUTPUT"
    echo "      ${OUT_DOOR}" | tee -a "$OUTPUT"
    echo "--------------------------------------------------------------------------------------------" | tee -a "$OUTPUT"
    
    TOT_OK=$((TOT_OK + 1))
done

echo "============================================================" | tee -a "$OUTPUT"
echo " Simulazione terminata. OK: $TOT_OK | Fallite: $TOT_FAIL" | tee -a "$OUTPUT"
echo " Risultati salvati in: results.txt"