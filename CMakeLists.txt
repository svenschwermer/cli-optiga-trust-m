cmake_minimum_required(VERSION 3.13)

project(trustm)

set(LIB_DIR trustm_lib)
set(LIB_SRC_FILES
    ${LIB_DIR}/optiga/cmd/optiga_cmd.c
    ${LIB_DIR}/optiga/common/optiga_lib_common.c
    ${LIB_DIR}/optiga/common/optiga_lib_logger.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c_config.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c_data_link_layer.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c_physical_layer.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c_presentation_layer.c
    ${LIB_DIR}/optiga/comms/ifx_i2c/ifx_i2c_transport_layer.c
    ${LIB_DIR}/optiga/comms/optiga_comms_ifx_i2c.c
    ${LIB_DIR}/optiga/crypt/optiga_crypt.c
    ${LIB_DIR}/optiga/util/optiga_util.c
)
set(PAL_SRC_FILES
    ${LIB_DIR}/pal/linux/pal_logger.c
    ${LIB_DIR}/pal/linux/pal_gpio.c
    ${LIB_DIR}/pal/linux/pal_os_lock.c
    ${LIB_DIR}/pal/linux/target/rpi3/pal_ifx_i2c_config.c
    ${LIB_DIR}/pal/linux/pal.c
    ${LIB_DIR}/pal/linux/pal_os_datastore.c
    ${LIB_DIR}/pal/linux/pal_os_timer.c
    ${LIB_DIR}/pal/linux/pal_os_memory.c
    ${LIB_DIR}/pal/linux/pal_i2c.c
)

add_library(trustm SHARED ${LIB_SRC_FILES} ${PAL_SRC_FILES})

target_include_directories(trustm
  PUBLIC
    ${LIB_DIR}/optiga/include
  PRIVATE
    ${LIB_DIR}/pal/linux
)

set(CRYPTO "OPENSSL" CACHE STRING "Select crypto backend")
set_property(CACHE CRYPTO PROPERTY STRINGS "OPENSSL" "MBEDTLS")
if(CRYPTO STREQUAL "OPENSSL")
    target_sources(trustm PRIVATE ${LIB_DIR}/pal/pal_crypt_openssl.c)
    target_link_libraries(trustm INTERFACE crypto)
else()
    target_sources(trustm PRIVATE ${LIB_DIR}/pal/pal_crypt_mbedtls.c)
    target_link_libraries(trustm INTERFACE mbedcrypto)
endif()

set_target_properties(trustm PROPERTIES SOVERSION "0")
install(TARGETS trustm LIBRARY)
install(DIRECTORY ${LIB_DIR}/optiga/include/optiga TYPE INCLUDE)


set(ENGINE_DIR trustm_engine)
set(ENGINE_SRC_FILES
    ${ENGINE_DIR}/trustm_engine.c
    ${ENGINE_DIR}/trustm_engine_ec.c
    ${ENGINE_DIR}/trustm_engine_ipc_lock.c
    ${ENGINE_DIR}/trustm_engine_rand.c
    ${ENGINE_DIR}/trustm_engine_rsa.c
)
set(HELPER_DIR trustm_helper)
set(HELPER_SRC_FILES
    ${HELPER_DIR}/trustm_helper_ipc_lock.c
    ${HELPER_DIR}/trustm_helper.c
)
add_library(trustm_engine SHARED
    ${ENGINE_SRC_FILES}
    ${HELPER_SRC_FILES}
    ${LIB_DIR}/pal/linux/pal_os_event.c
)
set_target_properties(trustm_engine PROPERTIES PREFIX "")
target_include_directories(trustm_engine
  PRIVATE
    ${ENGINE_DIR}
    ${HELPER_DIR}/include
    ${LIB_DIR}/optiga/include/optiga
    ${LIB_DIR}/optiga/include/optiga/common
    ${LIB_DIR}/optiga/include/optiga/comms
    ${LIB_DIR}/optiga/include/optiga/ifx_i2c
)
target_link_libraries(trustm_engine PUBLIC trustm rt)
install(TARGETS trustm_engine DESTINATION ${CMAKE_INSTALL_LIBDIR}/engines-1.1)
