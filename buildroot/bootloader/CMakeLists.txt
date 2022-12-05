cmake_minimum_required(VERSION 3.16)

if (NOT IDF_TARGET)
    message(FATAL_ERROR "this is bootloader for esp-idf project.")
endif()


project (bootloader LANGUAGES C)
set(component_path "${IDF_PATH}/components/bootloader_support")

add_executable(${PROJECT_NAME} "${IDF_TARGET}/entry.c")

# output
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

# compile & definitions
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS} ${IDF_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME} PRIVATE "BOOTLOADER_BUILD=1" ${IDF_COMPILE_DEFINITIONS})

# ld
target_linker_script(${PROJECT_NAME} INTERFACE "${IDF_TARGET}/bootloader.ld")
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_OPTIONS ${LINK_OPTIONS})

target_include_directories(${PROJECT_NAME} PRIVATE
    "include"
    "${component_path}/include"
    "${component_path}/private_include"
    "${component_path}/bootloader_flash/include"
    "${component_path}/bootloader_flash/include"
    ""
)

target_sources(${PROJECT_NAME} PRIVATE
    ""
)
