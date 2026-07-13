# Makefile para Infernal (intérprete modular)
# Uso:
#   make          - compila el intérprete y sus módulos .fire embebidos
#   make clean    - elimina objetos y ejecutable (NO borra nada en config/)
#   make distclean- limpia también los archivos generados de .fire y metadatos (NO borra config/)
#   make help     - muestra esta ayuda

# --------------------------------------------------------------------
# Configuración
# --------------------------------------------------------------------
CC       := gcc
CFLAGS   := -Wall -Wextra -g -std=c11 -D_GNU_SOURCE
LDFLAGS  :=
INCDIRS  := -Isrc

SRCDIR       := src
BUILDDIR     := build
FIRE_SRC_DIR := config/infernal/fire
BIN_SRC_DIR  := config/infernal/bins
FIRE_GEN_DIR := $(BUILDDIR)/gen_fire
BIN_GEN_DIR  := $(BUILDDIR)/gen_bins
META_DIR     := src/metadata
TARGET       := infernal

# --------------------------------------------------------------------
# Búsqueda automática de fuentes
# --------------------------------------------------------------------
SOURCES  := $(shell find $(SRCDIR) -name '*.c')
OBJECTS  := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))

# Módulos .fire (si existen)
FIRE_FILES    := $(wildcard $(FIRE_SRC_DIR)/*.fire)
FIRE_GEN_SRCS := $(patsubst $(FIRE_SRC_DIR)/%.fire, $(FIRE_GEN_DIR)/%.fire.c, $(FIRE_FILES))
FIRE_GEN_OBJS := $(FIRE_GEN_SRCS:.c=.o)

# Módulos binarios (bins)
BIN_FILES    := $(wildcard $(BIN_SRC_DIR)/*)
BIN_GEN_SRCS := $(patsubst $(BIN_SRC_DIR)/%, $(BIN_GEN_DIR)/%.c, $(BIN_FILES))
BIN_GEN_OBJS := $(BIN_GEN_SRCS:.c=.o)

# Tabla de módulos embebidos (auto-generada)
EMBED_TABLE_SRC := $(BUILDDIR)/embedded_table.c
EMBED_TABLE_OBJ := $(EMBED_TABLE_SRC:.c=.o)

# Metadatos embebidos
META_FILES    := VERSION HELP WELCOME EDITION
META_SRCS     := $(patsubst %, $(BUILDDIR)/metadata_%.c, $(META_FILES))
META_OBJS     := $(META_SRCS:.c=.o)
META_HUB_SRC  := $(BUILDDIR)/metadata.c
META_HUB_OBJ  := $(META_HUB_SRC:.c=.o)

# Todos los objetos finales
ALL_OBJS := $(OBJECTS) $(EMBED_TABLE_OBJ) $(META_OBJS) $(META_HUB_OBJ)
ifneq ($(FIRE_FILES),)
ALL_OBJS += $(FIRE_GEN_OBJS)
endif
ifneq ($(BIN_FILES),)
ALL_OBJS += $(BIN_GEN_OBJS)
endif

# Archivos de dependencias automáticas
DEPS := $(ALL_OBJS:.o=.d)

# --------------------------------------------------------------------
# Reglas principales
# --------------------------------------------------------------------
.PHONY: all clean distclean help

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	@echo " [LD] $@"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilación de fuentes del proyecto
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo " [CC] $<"
	$(CC) $(CFLAGS) $(INCDIRS) -MMD -MP -c $< -o $@

# --------------------------------------------------------------------
# Reglas para módulos .fire
# --------------------------------------------------------------------
ifneq ($(FIRE_FILES),)
$(FIRE_GEN_DIR)/%.fire.c: $(FIRE_SRC_DIR)/%.fire
	@mkdir -p $(dir $@)
	@echo " [OD] $< -> $@"
	@name=$$(basename $@ .fire.c); \
	sanename=$$(echo $${name} | tr '-' '_'); \
	echo "unsigned char config_infernal_fire_$${sanename}[] = {" > $@; \
	od -A n -t x1 -v < "$<" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]\{1,\}/, 0x/g' -e 's/^, //' -e 's/^/0x/' -e 's/$$/,/' >> $@; \
	echo "};" >> $@; \
	echo "unsigned int config_infernal_fire_$${sanename}_len = sizeof(config_infernal_fire_$${sanename});" >> $@

$(FIRE_GEN_DIR)/%.o: $(FIRE_GEN_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo " [CC] $< (embedded)"
	$(CC) $(CFLAGS) -c $< -o $@
endif

# --------------------------------------------------------------------
# Reglas para módulos binarios (bins)
# --------------------------------------------------------------------
ifneq ($(BIN_FILES),)
$(BIN_GEN_DIR)/%.c: $(BIN_SRC_DIR)/%
	@mkdir -p $(dir $@)
	@echo " [BIN] $< -> $@"
	@name=$$(basename $<); \
	sanename=$$(echo $${name} | tr '-' '_'); \
	echo "unsigned char config_infernal_bins_$${sanename}[] = {" > $@; \
	od -A n -t x1 -v < "$<" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]\{1,\}/, 0x/g' -e 's/^, //' -e 's/^/0x/' -e 's/$$/,/' >> $@; \
	echo "};" >> $@; \
	echo "unsigned int config_infernal_bins_$${sanename}_len = sizeof(config_infernal_bins_$${sanename});" >> $@

$(BIN_GEN_DIR)/%.o: $(BIN_GEN_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo " [CC] $< (embedded)"
	$(CC) $(CFLAGS) -c $< -o $@
endif

# --------------------------------------------------------------------
# Tabla de módulos embebidos
# --------------------------------------------------------------------
$(EMBED_TABLE_SRC): $(FIRE_FILES) $(BIN_FILES)
	@mkdir -p $(dir $@)
	@echo " [GEN] $@"
	@echo '// Auto-generated embedded module table' > $@
	@echo '#include <stddef.h>' >> $@
	@echo '#include "stdlib/embedded.h"' >> $@
	@for f in $(FIRE_FILES); do \
		name=$$(basename $$f .fire); \
		sanename=$$(echo $${name} | tr '-' '_'); \
		echo "extern unsigned char config_infernal_fire_$${sanename}[];" >> $@; \
		echo "extern unsigned int config_infernal_fire_$${sanename}_len;" >> $@; \
	done
	@for f in $(BIN_FILES); do \
		name=$$(basename $$f); \
		sanename=$$(echo $${name} | tr '-' '_'); \
		echo "extern unsigned char config_infernal_bins_$${sanename}[];" >> $@; \
		echo "extern unsigned int config_infernal_bins_$${sanename}_len;" >> $@; \
	done
	@echo '' >> $@
	@echo 'EmbeddedModule embedded_modules[] = {' >> $@
	@for f in $(FIRE_FILES); do \
		name=$$(basename $$f .fire); \
		sanename=$$(echo $${name} | tr '-' '_'); \
		echo "  {\"$$name\", config_infernal_fire_$${sanename}, &config_infernal_fire_$${sanename}_len}," >> $@; \
	done
	@for f in $(BIN_FILES); do \
		name=$$(basename $$f); \
		sanename=$$(echo $${name} | tr '-' '_'); \
		echo "  {\"$$name\", config_infernal_bins_$${sanename}, &config_infernal_bins_$${sanename}_len}," >> $@; \
	done
	@echo '  {NULL, NULL, NULL}' >> $@
	@echo '};' >> $@

$(EMBED_TABLE_OBJ): $(EMBED_TABLE_SRC)
	@echo " [CC] $<"
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

# --------------------------------------------------------------------
# Reglas para metadatos
# --------------------------------------------------------------------
$(BUILDDIR)/metadata_%.c: $(META_DIR)/%
	@mkdir -p $(dir $@)
	@echo " [META] $<"
	@name=$*; \
	sanename=$$(echo $${name} | tr '-' '_'); \
	echo "unsigned char metadata_$${sanename}[] = {" > $@; \
	od -A n -t x1 -v < "$<" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]\{1,\}/, 0x/g' -e 's/^, //' -e 's/^/0x/' -e 's/$$/,/' >> $@; \
	echo "0x00 };" >> $@; \
	echo "unsigned int metadata_$${sanename}_len = sizeof(metadata_$${sanename}) - 1;" >> $@

$(BUILDDIR)/metadata_%.o: $(BUILDDIR)/metadata_%.c
	@echo " [CC] $< (metadata)"
	$(CC) $(CFLAGS) -c $< -o $@

$(META_HUB_SRC): $(patsubst %, $(META_DIR)/%, $(META_FILES))
	@mkdir -p $(dir $@)
	@echo " [GEN] $@"
	@echo '// Auto-generated embedded metadata hub' > $@
	@echo '#include <string.h>' >> $@
	@echo '#include <stddef.h>' >> $@
	@for f in $(META_FILES); do \
		name=$$f; \
		sanename=$$(echo $${name} | tr '-' '_'); \
		echo "extern unsigned char metadata_$${sanename}[];" >> $@; \
		echo "extern unsigned int metadata_$${sanename}_len;" >> $@; \
	done
	@echo '' >> $@
	@echo 'const char* get_metadata(const char *type) {' >> $@
	@echo '    if (strcmp(type, "VERSION") == 0) return (const char*)metadata_VERSION;' >> $@
	@echo '    if (strcmp(type, "HELP") == 0) return (const char*)metadata_HELP;' >> $@
	@echo '    if (strcmp(type, "WELCOME") == 0) return (const char*)metadata_WELCOME;' >> $@
	@echo '    if (strcmp(type, "EDITION") == 0) return (const char*)metadata_EDITION;' >> $@
	@echo '    return NULL;' >> $@
	@echo '}' >> $@

$(META_HUB_OBJ): $(META_HUB_SRC)
	@echo " [CC] $<"
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

# Incluir dependencias automáticas si existen
-include $(DEPS)

# --------------------------------------------------------------------
# Limpieza (respeta config/)
# --------------------------------------------------------------------
clean:
	@echo " [CLEAN]"
	rm -rf $(BUILDDIR) $(TARGET)

distclean: clean
	@echo " [DISTCLEAN]"
	rm -f $(FIRE_GEN_SRCS) $(BIN_GEN_SRCS) $(EMBED_TABLE_SRC) $(META_SRCS) $(META_HUB_SRC)

help:
	@echo "Infernal Makefile"
	@echo "-----------------"
	@echo "Objetivos:"
	@echo "  all        : compila el intérprete (por defecto)"
	@echo "  clean      : borra objetos y ejecutable (no toca config/)"
	@echo "  distclean  : borra todo lo generado, incluyendo .fire.c y metadatos (no toca config/)"
	@echo "  help       : muestra esta ayuda"
	@echo ""
	@echo "Fuentes: $(words $(SOURCES)) archivos .c"
	@echo "Módulos .fire empaquetados: $(words $(FIRE_FILES))"
	@echo "Módulos binarios empaquetados: $(words $(BIN_FILES))"
