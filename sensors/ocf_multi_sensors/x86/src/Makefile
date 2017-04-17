ifneq ($(VERSION_MAJOR), 1)
$(error This app works only with Zephyr 1.7 or above)
endif

ifneq ($(shell test $(VERSION_MINOR) -gt 6; echo $$?),0)
$(error This app works only with Zephyr 1.7 or above)
endif

ccflags-y += -I$(ZEPHYR_BASE)/include/drivers
ccflags-y += -I$(ZEPHYR_BASE)/drivers
ccflags-y +=-I. -I${IOTIVITY_CONSTRAINED_BASE}/include \
		-I${IOTIVITY_CONSTRAINED_BASE} \
		-DDTLSV12 -DWITH_SHA256 -DDTLS_CHECK_CONTENTTYPE -DWITH_OCF \
		-I${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls -DNDEBUG

ifeq ($(SECURE),1)
ccflags-y += -DOC_SECURITY
endif

ifeq ($(DEBUG),1)
ccflags-y += -DOC_DEBUG
endif

ifeq ($(CONFIG_NET_L2_BLUETOOTH),y)
# TODO: Instead of using ipss sample module, implement Bluetooth init functionality.
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth
obj-y += ipss.o
src/ipss.c: ${ZEPHYR_BASE}/samples/bluetooth/gatt/ipss.c
	cp $< $@
endif

ccflags-y +=-I${IOTIVITY_CONSTRAINED_BASE}/deps/tinycbor/src
obj-y += cborencoder_close_container_checked.o \
	 cborencoder.o \
	 cborparser.o
src/%.c: ${IOTIVITY_CONSTRAINED_BASE}/deps/tinycbor/src/%.c
	cp $< $@

ccflags-y +=-I${IOTIVITY_CONSTRAINED_BASE}/messaging/coap
obj-y += engine.o \
	 coap.o \
	 transactions.o \
	 separate.o \
	 observe.o
src/%.c: ${IOTIVITY_CONSTRAINED_BASE}/messaging/coap/%.c
	cp $< $@

ccflags-y += -I${IOTIVITY_CONSTRAINED_BASE}/util
obj-y += oc_memb.o \
	 oc_etimer.o \
	 oc_list.o \
	 oc_process.o \
	 oc_timer.o \
	 oc_mmem.o
src/%.c: ${IOTIVITY_CONSTRAINED_BASE}/util/%.c
	 cp $< $@

ccflags-y +=-I${IOTIVITY_CONSTRAINED_BASE}/api
obj-y += oc_discovery.o \
	 oc_main.o \
	 oc_buffer.o \
	 oc_server_api.o \
	 oc_client_api.o \
	 oc_helpers.o \
	 oc_ri.o \
	 oc_rep.o \
	 oc_uuid.o \
	 oc_core_res.o \
	 oc_network_events.o \
	 oc_blockwise.o \
	 oc_base64.o
src/%.c: ${IOTIVITY_CONSTRAINED_BASE}/api/%.c
	cp $< $@

ccflags-y +=-I${IOTIVITY_CONSTRAINED_BASE}/port/zephyr/src
obj-y += ipadapter.o \
	 random.o \
	 clock.o \
	 abort.o
src/%.c: ${IOTIVITY_CONSTRAINED_BASE}/port/zephyr/src/%.c 
	cp $< $@

ifeq ($(SECURE),1)
ccflags-y +=-I${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls
obj-y += ccm.o \
	 hmac.o \
	 netq.o \
	 peer.o \
	 dtls_time.o \
	 session.o \
	 sha2/sha2.o \
	 ecc/ecc.o \
	 aes/rijndael.o \
	 crypto.o \
	 dtls.o
src/%c: ${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls/%.c
	cp $< $@
src/%c: ${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls/sha2/sha2.c
	cp $< $@
src/%c: ${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls/ecc/ecc.c
	cp $< $@
src/%c: ${IOTIVITY_CONSTRAINED_BASE}/deps/tinydtls/aes/rijndael.c
	cp $< $@

ccflags-y += -I${IOTIVITY_CONSTRAINED_BASE}/deps/security
obj-y += oc_acl.o \
	 oc_cred.o \
	 oc_doxm.o \
	 oc_dtls.o \
	 oc_pstat.o \
	 oc_pstat.o \
	 oc_store.o \
	 oc_svr.o
src/%c: -I${IOTIVITY_CONSTRAINED_BASE}/deps/security/%.c
	cp $< $@

obj-y += storage.o
src/%c: -I${IOTIVITY_CONSTRAINED_BASE}/port/zephyr/src/storage.c
	cp $< $@
endif

ccflags-y += -DOC_SERVER
obj-y += main.o
