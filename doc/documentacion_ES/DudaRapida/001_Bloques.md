# Bloques
Este es un breve resumen de cómo son los bloques en Infernal.

## if
``` Infernal
if algo operador algo then
    hacer algo
fi
```
Se comienza con if, de condiciona y luego se pone "then" como en Lua. Luego termina en fi como en Bash.
En español, "if" significa "si", como una preposición condicional si lo comparamos con la gramática de la lengua.
"fi" significa "finish", por lo que al español significaría "terminar orden".

## while
``` Infernal
while algo comparador algo then
    hacer algo
fi
```
En vez de "do" como en Lua, utilizamos "then" para facilitar la curva de aprendizaje.
Por ese motivo también termine en "fi" como la condición "if".
Puedes poner:
``` Infernal
while true then
    hacer algo infinitamente
fi
```
Para hacer un bucle infinito que solo termine con "break".

## for
``` Infernal
for algo in algo then
    hacer algo
fi
```
Igual que en los 2 bloques explicados anteriormente: "then" para definir comienzo de definición de lo que hace el bloque y "fi" para terminarlo.
