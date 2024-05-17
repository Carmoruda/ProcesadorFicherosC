#!/bin/bash

father_dir=""
suc_dir=""

while read -r line || [[ -n "$line" ]]; do
    # Saltar líneas en blanco y comentarios
    if [[ -z "$line" || "$line" == \#* ]]; then
        continue
    fi

    # Dividir la línea en clave y valor usando '=' como delimitador
    IFS='=' read -r key value <<< "$line"

    # Eliminar espacios en blanco alrededor de la clave y valor
    key=$(echo "$key" | xargs)
    value=$(echo "$value" | xargs)

    # Almacenar valores en variables correspondientes
    if [[ "$key" == "FATHER_DIR" ]]; then
        father_dir="$value"
    elif [[ "$key" == "SUC_DIR" ]]; then
        suc_dir="$value"
    fi
done < ../conf/fp.conf



eval "mkdir -p $father_dir/$suc_dir" 2> /dev/null
