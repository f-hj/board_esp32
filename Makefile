#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := gatt_server_demos

COMPONENT_ADD_INCLUDEDIRS := components/include
COMPONENT_SRCDIRS := main main/gatts_services

include $(IDF_PATH)/make/project.mk
