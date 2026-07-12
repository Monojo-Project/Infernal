# Variables

> Nota: VAR significa variable y las que se ponen así en mayúsculas son que son variables de ejemplo.

## Tipos de variables
Las variables en Infernal utilizan algo llamado tipado fijo. Es decir, si una variable es de un tipo, NUNCA va a dejar de ser de ese tipo.
Los tipos de variables son:
**Tipo**       **Qué hace**
int        Guarda un número entero.
float      Guarda un número decimal.
string     Guarda una cadena de texto.
bool       Guarda solo 2 valores: positivo (true) y negativo (false).
list       Guarda una lista con strings.

## Definir variables con tipado explícito 
Para definir una variable de tipado explícito (eliges si tipo al definirlas para controlar mejor su tipo) tienes que hacer así:
``` Demostración
TIPO VARIABLE = ALGO
```
Cómo se define cada tipo (el nombre de la variable se llama x en los ejemplos):
Cómo se definen:
int x = 3 #un simple número entero
float x = 1.24 #un número decimal con punto
string x = "algo" #una cadena de texto dentro de comillas dobles
bool x = true #una palabra (true o false) cuya primera letra puede ser mayúscula (True o False)
list x = ["algo", "alga"] #una lista de strings que entra en detalle en la documentación "Listas"

## Definir variables de tipado automático
Las variables que no son tipado explícito hacen tipado automático detectando si es true o false, si es un número sin comillas o algo con comillas y si es texto sin comillas, es un comando de shell cuya salida de redirecciona al valor de la variable. 
Ejemplos:
**Código**	       **Resultado**
x = 42	       x es un int
x = 3.14	   x es un float
x = true	   x es un bool
x = "hola"	   x es un string
x = ls	       Ejecuta ls con el shell y guarda su salida y el valor lo asigna con tipado automático

## La influencia del tipado fijo
El tipado fijo es una característica que hace que las variables nunca cambien de tipo. Es decir, si tu variable es un int, nunca podrá ser un string o un float. Ejemplo:
``` Infernal
int x = 13
string x = "miau"
```
Devuelve:
``` Output
Línea: string x = "miau"
Error: Tipado fijo: la variable 'x' es de tipo int, no se puede asignar un valor de tipo string (línea 2)
```
Incluso si no pones el texto string antes de definir la variable, también te da el error.

## Nombres prohibidos
No puedes crear variables que en su nombre tengan:
@ lo explicolamos en la documentación Portales
$ por lo que explicaremos a continuación
[ porque es para listas 
] también es para listas 
( es para funciones
) también es para funciones
{ es para lo que explicaremos a continuación 
} también a continuación


## Clonado de variables
Clonar/copiar el valor y el de variables es fácil en Infernal.
x = $y
x = {y}

Los 2 hacen los mismo, ya que son alias (sinónimos, eliges el que te es más cómodo). En los 2, el valor de x es el mismo valor de y, pero si y cambia de valor, el valor de x se mantiene.
Pero claro, si la variable ya tiene tipo, da error de tipado fijo. Y si es del mismo tipo, entonces copia el valor y lo reemplaza. Si la variable no existe, la crea limpiamente.

### Ejemplos de para qué sirve clonar
``` Infernal
x = 13
y = $x
y = y + 2 #suma 2 al valor de y
print(x) #devuelve: 13
print(y) #devuelve: 15 (13 (el valor de x) + 2)
```
Otro ejemplo pero ahora con texto:
``` Infernal
x = "hola"
y = $x

print(x) #devuelve: hola
print(y) #devuelve: hola

y = "miau"
print(x) #devuelve: hola
print(y) #devuelve: miau
```

### Nota importante 
No puedes poner:
Var1 = Var
Aunque en otros lenguajes de programación automáticamente se convierten en un alias, aquí no es así porque básicamente "me parece inútil". Esa línea intentaría ejecutar "Var" con el shell.




## Depuración.
Las funciones de depuración de variables son:
vartype(VARIABLE)
printAllVars()

### Función vartype(VARIABLE):
Esta función sirva para definir una variable con un string cuyo valor es el tipo de la variable entre comillas. Me explico con un ejemplo:
``` Infernal
x = 13
TIPO = vartype(x)
```
La variable TIPO tiene como valor "int" porque x es un int. TIPO es un string, por cierto.

### Función printAllVars()
Esta función es un poco loca porque imprime TODAS las variables con su nombre, tipo y valor.
Ejemplo:
``` Infernal
a = hostname #si el hostname no son solo números, es un string. Si no, un int. Esto ejecuta un comando de shell
x = "13" #nota: como lleva comillas, no es un número, es un string
printAllVars()
```
Salida:
``` Output
Variables accesibles:
    x: "13" (string)
    a: "lyndsos" (string)
```
Imprimió el nombre, el valor y el tipo de todas las variables.
