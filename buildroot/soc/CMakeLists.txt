set(idf_path "${IDF_PATH}/components/soc")

idf_component_register(
    INCLUDE_DIRS
        "${idf_path}/include"
        "${idf_path}/${IDF_TARGET}/include"
    SRCS
        "${idf_path}/lldesc.c"
        "${idf_path}/dport_access_common.c"
    LDFRAGMENTS
        "${idf_path}/linker.lf"
)

# REVIEW: in ${IDF_TARGET}/CMakeLists.txt
#  fix: target_include_directories(${COMPONENT_LIB} PUBLIC . include) using relative paths
add_subdirectory("${idf_path}/${IDF_TARGET}"
    "${build_dir}/${prefix}/${component_name}/${IDF_TARGET}"
)

target_linker_script(${COMPONENT_LIB} INTERFACE
    "${idf_path}/${IDF_TARGET}/ld/${IDF_TARGET}.peripherals.ld"
)
