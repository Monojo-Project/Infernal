# TODO de Infernal

Lista de trabajo obtenida de una revisión estática del código y de una compilación local. Los elementos están ordenados por prioridad estimada; no todos son errores confirmados mediante una batería de pruebas todavía.

## Estado

Los puntos resueltos en esta iteración se conservan marcados para que el archivo funcione también como registro de mantenimiento. Los puntos sin marcar requieren una decisión de diseño más amplia o una refactorización posterior.

## Crítico

- [x] Verificar la restauración del ámbito tras una función de usuario.
  La revisión completa confirmó que `current_scope = prev_scope` ya se ejecutaba antes del retorno; la observación inicial era errónea. Se añadió una prueba de funciones para impedir regresiones básicas.

- [x] Revisar y corregir los bloques con indentación engañosa de `src/runtime/evaluator.c`.
  La compilación con `-Wall -Wextra` detecta varios `if` y `for` sin llaves cuya sangría sugiere un comportamiento distinto. En particular, afectan a conversiones, referencias/listas y procesamiento de `flags`. Añadir llaves y pruebas de regresión antes de refactorizar.

- [ ] Añadir límites y comprobaciones de capacidad a los buffers de comandos de tamaño fijo (`char cmd[4096]`, `char buf[4096]`, etc.).
  Las concatenaciones con `strcat` pueden desbordarse con scripts, cadenas o argumentos grandes. Sustituir por construcción dinámica o funciones con límite explícito.

## Seguridad y ejecución de comandos

- [ ] Documentar y reducir la superficie de inyección al ejecutar comandos del sistema.
  Las sentencias shell y las asignaciones que capturan comandos pasan por `system()` o `popen()`, después de interpolar `$variables`. Definir claramente el modelo de seguridad y, cuando sea posible, ejecutar con `execve`/argumentos en lugar de una cadena de shell.

- [x] Corregir el directorio temporal de binarios embebidos.
  `set_embedded_tmp_dir()` almacena el directorio del script, pero la extracción usa el directorio de trabajo (`./.infernal_tmp`). Unificar la política, crear el directorio con seguridad y limpiar los temporales incluso ante señales o salidas tempranas.

- [x] Validar la escritura completa de los binarios embebidos.
  `write()` puede escribir menos bytes de los solicitados; usar un bucle que complete la escritura y manejar `EINTR`.

- [ ] Revisar nombres y rutas de módulos importados.
  Evitar truncamientos de los buffers de 512 bytes, definir una política para rutas relativas y validar nombres antes de construir `/usr/share/infernal/fire/<nombre>.fire`.

## Correctitud del lenguaje

- [x] Implementar o retirar de forma coherente los operadores `<` y `>`.
  El lexer y el evaluador los reconocen, pero el parser de comparaciones sólo construye AST para `==`, `!=`, `<=` y `>=`.

- [ ] Definir y probar semántica de tipos consistente.
  Aclarar conversiones implícitas (`int`, `float`, `bool`, `string`), el tipado fijo al reasignar y el comportamiento de parámetros tipados. Centralizar las conversiones en una única API.

- [ ] Revisar el comportamiento de listas y referencias.
  La asignación desde un índice puede crear `VAL_REFERENCE`, y las operaciones de inserción/eliminación de listas contienen bloques sin llaves. Especificar copia frente a referencia, índices inválidos y mutabilidad, y cubrirlo con pruebas.

- [ ] Completar o acotar el soporte de indexado de cadenas.
  Actualmente sólo se admite el índice `1`, que devuelve la cadena completa. Decidir si se implementa indexado real o se emite un error claro para cualquier índice.

- [x] Validar aridad de funciones.
  Los argumentos faltantes se convierten en nulo y los sobrantes no se rechazan explícitamente. Establecer una regla y mensajes de error consistentes.

- [ ] Revisar `flags` en los modos 0 y 1.
  Añadir validación de valor faltante, tipos `list`, booleanos, opciones agrupadas, opciones repetidas y el comportamiento del comodín `*`.

- [ ] Revisar la semántica de `repeat line` y portales.
  Definir qué ocurre al saltar desde bucles o funciones, si los portales deben registrarse antes de ejecutarse y cómo se evita un ciclo no intencionado.

## Calidad del código

- [ ] Eliminar código inalcanzable, duplicación y lógica de parser repetida.
  La detección de asignación/captura de comando aparece repetida para declaraciones locales, globales, tipadas y sin tipo. Extraer funciones auxiliares y simplificar el parser.

- [ ] Adoptar una política uniforme de propiedad y liberación de memoria.
  Hay numerosas asignaciones con `malloc`, `realloc` y `strdup` sin una destrucción visible del AST, tokens, ámbitos y valores. Implementar destructores y ejecutar el proyecto con ASan/Valgrind.

- [ ] Añadir manejo de errores de memoria y de llamadas del sistema.
  Comprobar resultados de `malloc`, `realloc`, `strdup`, `fopen`, `setenv`, `realpath`, `popen` y operaciones de archivo.

- [x] Establecer formato automático y reglas de compilación más estrictas.
  Añadir `.clang-format`, habilitar `-Werror` en CI cuando se hayan corregido los avisos y considerar `-fsanitize=address,undefined` para desarrollo.

- [ ] Mejorar el sistema de errores.
  Evitar posibles truncamientos de `exception_msg`, incluir contexto de importación/pila de llamadas y diferenciar errores léxicos, sintácticos y de ejecución.

## Pruebas y automatización

- [x] Crear una batería de pruebas automatizada.
  Cubrir lexer, parser, evaluador, errores, módulos, binarios embebidos, `flags`, ámbitos, bucles y comandos. Integrarla en `make test`.

- [x] Añadir pruebas de extremo a extremo con scripts de ejemplo.
  Incluir casos de éxito y fallo con su salida esperada, sin depender de herramientas del sistema que no estén garantizadas.

- [x] Añadir CI.
  Como mínimo: compilación limpia, pruebas, análisis estático y comprobación de formato en cada cambio.

- [x] Verificar compatibilidad de plataformas.
  El proyecto depende de APIs POSIX/GNU (`getline`, `fmemopen`, `realpath`, `mkstemp`, `fdatasync`, `setenv`). Declarar las plataformas soportadas y, si procede, añadir alternativas.

## Producto y documentación

- [ ] Mantener una especificación de lenguaje versionada.
  Incluir gramática, precedencia de operadores, tipos, ámbitos, módulos, `flags`, portales y semántica de errores; actualizarla con cada cambio de versión.

- [x] Ampliar `--help`.
  Incluir `--edition`, la sintaxis de argumentos y una referencia breve a la documentación local.

- [x] Añadir ejemplos autocontenidos en `examples/`.
  Proponer scripts para variables/listas, funciones, CLI con `flags`, módulos y binarios embebidos.

- [ ] Versionar la API/ABI de módulos embebidos y el formato de distribución.
  Aclarar cómo se instala un módulo externo, cómo se resuelven colisiones de nombres y cómo se mantiene compatibilidad entre versiones.

## Mantenimiento

- [x] Revisar las dependencias generadas por `Makefile` y separar los artefactos de compilación de los ficheros fuente.
  Confirmar que `make clean` y `make distclean` son idempotentes y que no eliminan recursos configurados por el usuario.

- [x] Añadir un `CHANGELOG.md` y una política de versionado.

- [x] Definir responsables, plantilla de incidencias y guía de contribución (`CONTRIBUTING.md`).
