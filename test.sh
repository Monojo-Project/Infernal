#!/usr/bin/env bash
set -euo pipefail

infernal=${1:-./infernal}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

run_case() {
    local name=$1 expected=$2
    shift 2
    local script="$tmpdir/$name.infernal"
    printf '%s\n' "$@" > "$script"
    local output
    output=$("$infernal" "$script")
    if [[ "$output" != "$expected" ]]; then
        printf 'Caso %s falló.\nEsperado:\n%s\nObtenido:\n%s\n' "$name" "$expected" "$output" >&2
        exit 1
    fi
}

run_case expressions $'true\ntrue\n5' \
    'print(2 < 3)' \
    'print(3 > 2)' \
    'print(2 + 3)'

run_case functions '7' \
    'function sumar(int a, int b)' \
    '    return a + b' \
    'fi' \
    'print(sumar(3, 4))'

run_case lists $'verde\namarillo' \
    'list colores = ["rojo", "verde"]' \
    'print(colores[2])' \
    'colores[2] = "amarillo"' \
    'print(colores[2])'

run_case string_index 'n' \
    'print("Infernal"[2])'

flags_script="$tmpdir/flags.infernal"
printf '%s\n' \
    'flags(0,' \
    '    --nombre = string nombre {' \
    '        print(nombre)' \
    '    }' \
    ')' > "$flags_script"
if [[ "$($infernal "$flags_script" --nombre Ada)" != "Ada" ]]; then
    printf 'Caso flags falló.\n' >&2
    exit 1
fi

printf 'OK: pruebas de Infernal\n'
