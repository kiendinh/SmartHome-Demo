ifneq ($(VERSION_MAJOR), 1)
$(error This app works only with Zephyr 1.7 or above)
endif

ifneq ($(shell test $(VERSION_MINOR) -gt 6; echo $$?),0)
$(error This app works only with Zephyr 1.7 or above)
endif

ccflags-y += ${PROJECTINCLUDE} \
		-I$(ZEPHYR_BASE)/include/drivers \
		-I$(ZEPHYR_BASE)/drivers

obj-y += main.o
