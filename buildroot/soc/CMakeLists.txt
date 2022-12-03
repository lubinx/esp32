set(component_path "${IDF_PATH}/components/soc")

idf_component_register(
    INCLUDE_DIRS
        "${component_path}/include"
        "${component_path}/${IDF_TARGET}/include"
    SRCS
        "${component_path}/lldesc.c"
        "${component_path}/dport_access_common.c"
    LDFRAGMENTS
        "${component_path}/linker.lf"
)

# REVIEW: in ${IDF_TARGET}/CMakeLists.txt
#  fix: target_include_directories(${COMPONENT_LIB} PUBLIC . include) using relative paths
add_subdirectory("${component_path}/${IDF_TARGET}"
    "${build_dir}/${prefix}/${component_name}/${IDF_TARGET}"
)

target_linker_script(${COMPONENT_LIB} INTERFACE
    "${component_path}/${IDF_TARGET}/ld/${IDF_TARGET}.peripherals.ld"
)
