# Especificación operativa de Infernal

Esta especificación describe la edición implementada por el intérprete incluido en este repositorio.

## Ejecución

`infernal <script.infernal> [argumentos...]` ejecuta el script. Los argumentos se exponen a `flags`; las variables de entorno `INFERNAL_VAR_*` se cargan como cadenas en el ámbito superglobal.

## Tipos y conversiones

Los tipos son `int`, `float`, `bool`, `string`, `list` y `null`. Las operaciones aritméticas mezclan enteros y flotantes promoviendo el resultado a `float`; la concatenación convierte enteros, flotantes y booleanos a texto. Las asignaciones tipadas rechazan valores incompatibles salvo las conversiones explícitas admitidas por `try_convert_value`.

## Operadores

La precedencia, de mayor a menor, es: indexado y llamadas; `* / %`; `+ -`; comparaciones (`< > <= >= == !=`); `and`; `or`. Las comparaciones numéricas aceptan `int` y `float`.

## Ámbitos y funciones

Los ámbitos anidados buscan variables desde el ámbito actual hacia sus padres. `local` crea una variable en el ámbito actual y `global` en el ámbito global del script. Las funciones de usuario exigen aridad exacta; una devolución ausente produce `null`.

## Listas y cadenas

Los índices son base 1. Un índice de lista fuera de rango es un error. El indexado de cadenas devuelve una cadena de un carácter y también valida el rango.

## Control de flujo

`if`, `while`, `for` y `for-in` crean ámbitos de bloque. Los bucles están limitados por `max_loop_iterations` (10 000 por defecto). `repeat line N` reinicia la ejecución en la primera sentencia cuya línea sea al menos `N`; `repeat @portal` usa el destino registrado por el portal.

## Comandos

`!comando!` sólo ejecuta binarios embebidos. Las sentencias de shell y las capturas de salida (`= !...!` o asignaciones de comando) siguen usando el shell del sistema y, por tanto, no deben recibir texto no confiable sin validación del programa.

## Módulos

`import nombre` busca un módulo embebido o `/usr/share/infernal/fire/nombre.fire`. Los nombres embebidos sólo contienen letras, números, `_` y `-`; las rutas explícitas no pueden contener `..` ni saltos de línea.

## Errores

Los errores incluyen archivo y línea cuando están disponibles. `try ... catch ... fi` captura errores de ejecución producidos dentro del bloque `try`.
