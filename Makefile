#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ESP_DRC

EXTRA_COMPONENT_DIRS += ./libs  

include $(IDF_PATH)/make/project.mk

