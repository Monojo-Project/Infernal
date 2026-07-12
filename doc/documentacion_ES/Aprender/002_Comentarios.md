# Comentarios

## Qué son
Los comentarios, como en cualquier lenguaje de programación, son texto que el intérprete/compilador no leen. Sirven para guardar notas en el código sin que den error de sintaxis.
También sirve para desactivar partes del código.

 
## Cómo se ponen
Los comentarios en Infernal se ponen igual que en Python o Bash: con un #.
``` Infernal
#esto es un comentario
# Esto es otro
```


## Comentarios grandes (bloques de comentarios)
En Infernal, si quieres comentar (desactivar) una parte grande del código (no solo una línea), puedes hacer lo siguiente:
``` Infernal
###
Aquí dentro van los comentarios
Puedes usarlo para comentar grandes bloques de código o para añadir licencia y créditos
###
```
Esto en Python se hace con """ """, pero en Infernal es con ### ###.



## Dudas frecuentes 
### ¿Qué pasaría si pongo ### en vez de #?
Pues el ### si no termina (es decir, no tiene otro ### que declare su final), es un comentario normal.
