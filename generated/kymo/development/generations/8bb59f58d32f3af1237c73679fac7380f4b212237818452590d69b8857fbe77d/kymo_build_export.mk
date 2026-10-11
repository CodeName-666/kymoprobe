# Include this file by its immutable snapshot path. No Python/Qt is used.
KYMO_SNAPSHOT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
CPPFLAGS += -I"$(KYMO_SNAPSHOT_DIR)" -DKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\"
KYMO_GENERATED_SOURCES := $(KYMO_SNAPSHOT_DIR)/kymo_generated_config.cpp
# Use CPPFLAGS for EVERY Core and application translation unit.
# Add KYMO_GENERATED_SOURCES to application sources. Use distinct build directories per profile.
