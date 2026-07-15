# Sistema de Compresión Gzip en Infernal

## Resumen Ejecutivo

El sistema de compresión gzip **funciona en la mayoría de casos** pero tiene **2 vulnerabilidades críticas** que deben corregirse antes de producción:

1. 🔴 **Versión de zlib hardcodeada** → Crash con versiones diferentes
2. 🔴 **Sin límite de descompresión** → Vulnerable a DoS

---

## 1. Flujo General del Sistema

```
┌─── COMPILACIÓN ───┐
│                   │
│ config/gzip-embedded.bool
│      (true)        │
│        ↓           │
│ Makefile lee       │
│ configuración      │
│        ↓           │
│ binarios se        │
│ comprimen con      │
│ gzip -c            │
│        ↓           │
│ Tabla genera flag  │
│ compressed=1       │
└─────────────────────┘
       ↓
┌─── EJECUCIÓN ───┐
│                 │
│ embedded_find() │
│  devuelve datos │
│ + compressed    │
│        ↓        │
│ Si compressed=1:│
│  gunzip_data()  │
│        ↓        │
│ dlopen libz.so  │
│ inflateInit2_() │
│ inflate()       │
│ inflateEnd()    │
│        ↓        │
│ Escribe a       │
│ temporal        │
│        ↓        │
│ Ejecuta         │
│        ↓        │
│ Limpia temp     │
└─────────────────┘
```

---

## 2. Análisis de Correctitud por Componente

### ✅ Componentes Correctos

| Componente | Ubicación | Estado | Notas |
|-----------|-----------|--------|-------|
| Lectura de config | Makefile 27-54 | ✅ | Fallback sensato a true |
| Compresión binarios | Makefile 141-151 | ✅ | `gzip -c` funciona correctamente |
| Generación tabla | Makefile 185-190 | ✅ | Flags de compresión correctos |
| Búsqueda modular | embedded.c 8-18 | ✅ | Defensivo (valida punteros) |
| Escritura temporal | command.c 240-252 | ✅ | Maneja writes parciales e EINTR |
| Limpieza temp | command.c 320-337 | ✅ | Segura y robusta |

### 🔴 Problemas Críticos

#### Problema 1: Versión de Zlib Hardcodeada

**Ubicación**: `src/runtime/command.c` línea 153

```c
if (p_inflateInit2(&strm, 16 + MAX_WBITS, "1.2.13", sizeof(strm)) != Z_OK)
    return NULL;
```

**Riesgo**: El string `"1.2.13"` es fijo. Si el sistema tiene `libz 1.2.11` u otra versión, fallaría.

**Severidad**: 🔴 **CRÍTICO**

**Solución**:
```c
// Detectar versión en tiempo de ejecución
typedef const char* (*zlibVersion_t)(void);
zlibVersion_t p_zlibVersion = (zlibVersion_t)dlsym(zlib_handle, "zlibVersion");
const char *version = p_zlibVersion ? p_zlibVersion() : "1.2.0";

if (p_inflateInit2(&strm, 16 + MAX_WBITS, version, sizeof(strm)) != Z_OK) {
    fprintf(stderr, "Error: no se pudo inicializar zlib\n");
    return NULL;
}
```

---

#### Problema 2: Sin Límite de Descompresión (DoS)

**Ubicación**: `src/runtime/command.c` líneas 156-178

```c
size_t buf_size = compressed_len * 4 + 1024;
unsigned char *out = malloc(buf_size);

int ret;
while ((ret = p_inflate(&strm, Z_FINISH)) != Z_STREAM_END) {
    // ...
    buf_size *= 2;  // 🔴 Crece sin límite
    unsigned char *tmp = realloc(out, buf_size);
    // ...
}
```

**Riesgo**: Un archivo gzip corrupto o malicioso podría consumir toda la memoria RAM.

**Severidad**: 🔴 **CRÍTICO** - Vulnerabilidad DoS

**Ejemplo de ataque**:
```bash
# Archivo gzip especial que pretende descomprimir a 1TB
echo 'malformed gzip' | gzip > bomb.gz
# Cuando Infernal intente descomprimir, puede consumir OOM
```

**Solución**:
```c
#define MAX_DECOMPRESSED_SIZE (500 * 1024 * 1024)  // 500MB

while ((ret = p_inflate(&strm, Z_FINISH)) != Z_STREAM_END) {
    if (ret != Z_OK) {
        free(out);
        p_inflateEnd(&strm);
        return NULL;
    }
    
    // NUEVO: Verificar límite
    if (strm.total_out > MAX_DECOMPRESSED_SIZE) {
        fprintf(stderr, "Error: binario descomprimido excede límite de %zu bytes\n",
                MAX_DECOMPRESSED_SIZE);
        free(out);
        p_inflateEnd(&strm);
        return NULL;
    }
    
    size_t used = strm.next_out - out;
    buf_size *= 2;
    unsigned char *tmp = realloc(out, buf_size);
    if (!tmp) { free(out); p_inflateEnd(&strm); return NULL; }
    out = tmp;
    strm.next_out = out + used;
    strm.avail_out = buf_size - used;
}
```

---

### 🟡 Problemas Menores

**Problema 3**: Módulos `.fire` no se comprimen (comentario engañoso)
- **Severidad**: 🟡 BAJO
- **Solución**: Aclarar intención o agregar soporte de compresión

**Problema 4**: `dlopen()` sin `dlclose()`
- **Severidad**: 🟡 BAJO
- **Solución**: Aceptable mantener handle abierto para performance

**Problema 5**: Errores de descompresión sin contexto
- **Severidad**: 🟡 BAJO
- **Solución**: Agregar mensajes de error más descriptivos

---

## 3. Matriz de Impacto

| # | Problema | Severidad | Impacto | Esfuerzo | Urgencia |
|---|----------|-----------|---------|----------|----------|
| 1 | zlib version | 🔴 CRÍTICO | Crash en sistemas con zlib≠1.2.13 | 30 min | URGENTE |
| 2 | DoS memory | 🔴 CRÍTICO | Consumo OOM con archivos malformados | 15 min | URGENTE |
| 3 | .fire compress | 🟡 BAJO | Ejecutable más grande | 1 h | FUTURO |
| 4 | dlopen leak | 🟡 BAJO | Leak mínimo de recursos | 10 min | OPCIONAL |
| 5 | Error messages | 🟡 BAJO | Debugging difícil | 20 min | MEJORA |

---

## 4. Recomendaciones Urgentes

### Antes de mergear a `main`:

✅ **Fix 1**: Detectar versión de zlib (30 min)
✅ **Fix 2**: Agregar límite de descompresión (15 min)  
✅ **Fix 3**: Mejorar mensajes de error (20 min)

**Tiempo total**: ~1 hora

### Próxima versión (opcional):

- Considerar compresión también para módulos `.fire`
- Documentar política de límites de memoria
- Agregar tests de fuzzing para gzip

---

## 5. Casos de Prueba Críticos

```bash
# Test 1: Compresión básica
$ make clean && make
$ infernal demo-scripts-infernal/listas.infernal
# Debe ejecutar sin problemas

# Test 2: Sin zlib
$ mv /usr/lib/libz.so.1 /tmp/
$ ./infernal script.infernal
# Debe mostrar error claro
$ mv /tmp/libz.so.1 /usr/lib/

# Test 3: Gzip corrupto (simulación)
$ dd if=/dev/urandom of=/tmp/corrupt.gz bs=1 count=50
# Modificar Makefile para incluirlo y recompilar
# Debe fallar gracefully sin crashear
```

---

## 6. Conclusión

**Veredicto**: Sistema **FUNCIONA EN CASOS NORMALES** pero tiene **2 BUGS CRÍTICOS**.

### Estado por Aspecto

| Aspecto | Estado | Veredicto |
|---------|--------|----------|
| Compilación | ✅ | Bien implementada |
| Compresión | ✅ | Correcta |
| Descompresión | 🔴 | **BUGS CRÍTICOS** |
| Temporales | ✅ | Segura |
| Seguridad | 🔴 | **Vulnerable a DoS** |

### Recomendación Final

🚫 **NO MERGEAR** a `main` hasta corregir los 2 bugs críticos.

✅ **SÍ MERGEAR** después de aplicar Fix 1 + Fix 2.

⏱️ **Tiempo estimado para corregir**: 1 hora
