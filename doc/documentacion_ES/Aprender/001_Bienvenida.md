# Bienvenida
¡Bienvenido a **Infernal**!

## Qué es Infernal
Infernal es un lenguaje de programación desarrollado por **Lynds Corp**. para el proyecto de los **Aros Legendarios**. Su creador es **David Baña Szymaniak** (yo :) ).

## Por qué deberías usarlo
Porque Infernal es una fusión entre:
Bash
Lua 
Python
Juntando lo mejor de los 3 mundos en uno solo, haciendo que sea muy fácil de aprender si vienes de alguno de estos.

## Cuales son las características principales
Las características principales de Infernal son:
Interpretado (no compilado como C o C++)
Ligero (el intérprete pesa menos que una foto y las únicas dependencias son tener un kernel Unix-Like funcionando y libc)
Escrito en C (rápido y confiable)
Configurable (cuando compilas el intérprete, puedes endeberle librerías)
Variables de tipado fijo (se explica en la documentación de Variables)
> PD: Si muchas de estas palabras te asustan, tranquilo, en las siguientes documentaciones aprenderás todo esto.

## A qué me refiero con una fusión entre Bash, Lua y Python
Me refiero a que hereda (básicamente se copia) de otros lenguajes de programación para ser compatible (aunque no mucho) y facilitar la curva de aprendizaje.
### Bash:
De Bash hereda la facilidad de ejecutar comandos del sistema.
Por ejemplo:
``` Bash
cat /etc/hostname
```
``` Infernal
cat /etc/hostname
```
Hacen lo mismo, solo que Infernal lo ejecuta con el shell del sistema (/bin/sh) que puede apuntar as Dash en vez de Bash, mientras que Bash lo hace con su propio intérprete (/bin/bash).
También hereda que los bloques terminen en fi.
Si quieres, puedes hacer:
``` Infernal
user = whoami
id = cat /etc/id-machine
```
Y las variables guardarán el output de esos comandos.
## Lua
De Lua hereda la sintaxis fácil que es como hablar en inglés.
``` Lua
local x = 2
if x < 4 then
    print(x)
end
```
``` Infernal
x = 2
if x <= 4 then
    print(x)
fi
```
Solo cambia el operador y el bloque que en vez de terminar en end, termina en fi para ahorrar un byte en el peso del archivo.
### Python
De Python no se inspira tan directamente.
Lo que hace más bien es añadir alias (sinónimos) a las cosas para que sean como en Python y así facilitar el aprendizaje. Por ejemplo:
En los valores booleanos:
True → true (puedes poner los 2, el que más te guste)
False → false (lo mismo que con true)
Porque en Lua la primera letra es minúscula pero en Python es mayúscula, pero aquí da igual.
En la función input():
Es literalmente la misma función. En Infernal puedes usar input() como en Python y funcionará. Muy útil para apps en CLI.
También, a diferencia de Python, en Lua no tienes que preocuparte por los espacios entre bloques; son solo decoración. Lo que define los bloques son los "fi".
