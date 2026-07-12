El proyecto es un intérprete para el lenguaje de scripting **Infernal**. Está escrito en C y está organizado en módulos que cubren el análisis léxico, sintáctico, semántico, ejecución y bibliotecas estándar. A continuación se detalla la función de cada archivo.

---

## 📁 `core/` – Tipos, AST y valores fundamentales

| Archivo | Propósito y contenido |
|---------|------------------------|
| **`types.h`** | Define todas las estructuras de datos compartidas: enumeración de tipos de token (`TokenType`), estructura `Token`, nodos del AST (`ASTNode` con uniones para cada tipo de nodo), representación de valores (`Value` con unión para int, float, bool, string, list, reference), objetos de función (`FuncObject`), y la estructura `FlagSpec` para el procesamiento de flags. Es el corazón de tipos del intérprete. |
| **`memory.h` / `memory.c`** | Provee macros `infernal_malloc` y `infernal_free` como envoltorios de `malloc`/`free` (actualmente son solo placeholders, listos para futuras extensiones de trazado de memoria). |
| **`ast.h` / `ast.c`** | Funciones para manipular el AST: `node_create()` reserva e inicializa un nodo, y `nodelist_add()` añade un nodo a una lista dinámica (usada para bloques de sentencias). |
| **`value.h` / `value.c`** | Implementa la creación y manipulación de valores: `val_int()`, `val_float()`, `val_bool()`, `val_string()`, `val_list_empty()`, `val_list_append()`, `val_list_copy()`. También convierte entre tipos de valor y tipos de token (`valtype_to_tokentype()`) y crea referencias a elementos de listas (`val_reference()`). |

---

## 📁 `lexer/` – Análisis léxico

| Archivo | Propósito y contenido |
|---------|------------------------|
| **`keywords.h` / `keywords.c`** | Define una tabla de palabras clave del lenguaje (if, then, fi, while, for, function, return, int, float, bool, etc.) y la función `lookup_keyword()` que devuelve el `TokenType` correspondiente o `TOK_IDENT` si no es palabra reservada. |
| **`lexer.h` / `lexer.c`** | Implementa el escáner. Mantiene un `TokenStream` (lista dinámica de tokens) con funciones `ts_init()`, `ts_add()`, `ts_peek()`, `ts_advance()`, `ts_match()`, `ts_skip_newlines()`. `tokenize_file()` y `tokenize_buffer()` leen desde un archivo o memoria, respectivamente, y producen la secuencia de tokens. Maneja comentarios de línea (`#`) y bloques (`### ... ###`), números, identificadores (incluyendo `$` para variables), operadores compuestos (`==`, `!=`, `<=`, `>=`, `&&`, `||`, `>>`), y cadenas (con detección de cierre y error en caso de cadena sin cerrar). `extract_command_string()` extrae la parte derecha de una asignación para usarla como comando (cuando se detecta una asignación a una variable de tipo comando). |

---

## 📁 `parser/` – Análisis sintáctico (construcción del AST)

| Archivo | Propósito y contenido |
|---------|------------------------|
| **`parser.h` / `parser.c`** | Contiene el parser principal. `parse_block(terminator)` analiza un bloque de sentencias hasta encontrar un terminador (por ejemplo, `fi` o `}`). `parse_if_statement()` maneja `if`, `elseif`, `else`, `fi`. El parser reconoce todas las construcciones del lenguaje: asignaciones (con tipos opcionales, `local`/`global`), comandos de shell (`!...!`), funciones, bucles (`while`, `for` con inicialización/condición/incremento, y `for-in`), `return`, `break`, `continue`, `repeat` (con portal o con `line`), `import` (carga módulos embebidos o desde archivos), `try/catch`, `flags` (delega en `flags.c`), y `@` (portales). También maneja la detección de comandos cuando el lado derecho de una asignación no es una expresión válida. |
| **`flags.h` / `flags.c`** | Procesa la sintaxis de `flags(modo, especificaciones...)`. Construye una lista de `FlagSpec` (con nombres, alias, tipo y nombre de variable, y bloque de código). `parse_flag_body_tokens()` extrae los tokens del cuerpo del flag (entre llaves) para su ejecución posterior. |
| **`expression.h` / `expression.c`** | Implementa el análisis de expresiones. `parse_expression()` es el punto de entrada; maneja precedencia de operadores: lógicos (`and`, `or`), comparaciones (`==`, `!=`, `<=`, `>=` – **no** soporta `<` ni `>` sueltos, solo los compuestos), aritméticos (`+`, `-`, `*`, `/`, `%`), y el menos unario. También reconoce literales (números, strings, booleanos), identificadores (variables y llamadas a función con o sin paréntesis), listas (`[ ... ]`), acceso a índices (`[ ]`), y paréntesis para agrupación. |

---

## 📁 `runtime/` – Ejecución y entorno

| Archivo | Propósito y contenido |
|---------|------------------------|
| **`evaluator.h` / `evaluator.c`** | Motor de ejecución. `eval_expr()` evalúa un nodo de expresión y devuelve un `Value`. `exec_block()` ejecuta una lista de sentencias (`NodeList`), y `exec_block_from()` permite empezar desde un índice (usado para `repeat line`). Implementa la lógica de: asignaciones (con comprobación de tipos fijos y conversión automática de strings a números/booleanos), referencias a elementos de listas, operadores binarios (aritméticos, comparaciones, concatenación de strings, inserción/eliminación en listas con `+` y `-`), llamadas a funciones (nativas o de usuario), control de flujo (`if`, `while`, `for`, `for-in`, `return`, `break`, `continue`), `flags` (procesa los argumentos del script según el modo y ejecuta los bloques correspondientes), `import` (ejecuta el bloque del módulo en el ámbito global), `try/catch` (con `setjmp`/`longjmp`), portales (`@nombre` define un punto de salto), y `repeat` (salta a una línea o a un portal). También gestiona el límite de iteraciones para evitar bucles infinitos. |
| **`scope.h` / `scope.c`** | Gestión de ámbitos de variables y portales. `Scope` es una lista enlazada de variables (`VarEntry`) y portales (`PortalEntry`). `scope_new()` crea un ámbito con un padre. `scope_find()` busca una variable en el ámbito y sus padres. `scope_define()` añade una variable (con tipo opcional). `scope_assign()` asigna un valor, verificando el tipo fijo. Para portales: `portal_find()`, `portal_find_in_scope()`, `portal_define()`. |
| **`globals.h` / `globals.c`** | Declara y define variables globales del intérprete: ámbitos global y superglobal, ámbito actual, tabla de funciones (`func_table`), límite de iteraciones, buffer de error (`exception_env`, `exception_msg`), líneas fuente, estado de control de flujo (`control_flow`), valor de retorno, argumentos del script (`script_argc`, `script_argv`), y destino de `repeat line` (`repeat_line_target`). También provee funciones para registrar funciones (`func_register`, `func_register_builtin`) y buscarlas (`func_lookup`). |
| **`command.h` / `command.c`** | Manejo de comandos (shell y embebidos). `expand_command()` reemplaza variables (`$var` o `${var}`) por su valor actual. `execute_embedded()` ejecuta un comando embebido (binario incrustado en el ejecutable) extrayéndolo a un archivo temporal y llamando a `system()`. `popen_embedded_with_path()` similar pero para leer salida. `set_embedded_tmp_dir()` y `cleanup_embedded_temp_dir()` gestionan el directorio temporal para los binarios extraídos. |
| **`error.h` / `error.c`** | Función `error(line, fmt, ...)` que formatea un mensaje, lo guarda en `exception_msg`, activa `exception_raised` y hace `longjmp` al punto de captura (`setjmp` en `main` o en `try`). También provee la macro `error_missing_then()` para mensajes específicos. |

---

## 📁 `stdlib/` – Biblioteca estándar (funciones nativas)

| Archivo | Propósito y contenido |
|---------|------------------------|
| **`embedded.h` / `embedded.c`** | Define la estructura `EmbeddedModule` (nombre, datos binarios y tamaño). `embedded_find()` busca un módulo por nombre en una tabla (definida en otro archivo, probablemente generada por `xxd`) y devuelve sus datos. |
| **`system.h` / `system.c`** | Funciones nativas del sistema: `exit(code)`, `setlooplimit(limit)`, `getlooplimit()`. Se registran con `register_system_builtins()`. |
| **`io.h` / `io.c`** | Funciones de entrada/salida: `printAllVars()` (muestra todas las variables en el ámbito actual), `vartype(value)` (devuelve el tipo como string), `input(prompt)` (lee una línea desde stdin). Registradas con `register_io_builtins()`. |
| **`output.h` / `output.c`** | Funciones de salida con colores: `print` (normal), `warn` (amarillo), `error` (rojo), `success` (verde). Registradas con `register_output_builtins()`. |
| **`builtins.h` / `builtins.c`** | Función `register_all_builtins()` que llama a los registros de los módulos anteriores para que todas las funciones nativas estén disponibles. |

---

## 📄 `main.c` – Punto de entrada

Procesa argumentos de línea de comandos: si se pasa `--version` o `--help`, muestra metadatos incrustados (obtenidos con `get_metadata()`). Si no, espera un nombre de archivo de script. Inicializa los ámbitos, registra las funciones nativas, tokeniza y parsea el archivo, y luego ejecuta el bloque principal en un bucle que soporta `repeat line` (permite saltar a una línea específica mediante `repeat line <n>` o `repeat @portal`). También maneja excepciones con `setjmp`/`longjmp` para errores y limpieza de archivos temporales al finalizar.

---

## Resumen del flujo de ejecución

1. **`main`** lee el archivo, inicializa lexer y parser.
2. **Lexer** convierte el código en tokens.
3. **Parser** construye el AST.
4. **Evaluador** recorre el AST ejecutando sentencias.
5. Las funciones nativas (builtins) se llaman desde el evaluador.
6. **Comandos embebidos** se extraen a temporales y se ejecutan.
7. **Portales y `repeat`** permiten saltos de control no lineales.
8. **Manejo de errores** via `longjmp` capturado en `main` o en `try`.

Cada archivo cumple un rol específico en este proceso, manteniendo una separación clara de responsabilidades.
