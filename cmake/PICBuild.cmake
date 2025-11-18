find_program(MCHP_PROJ_GEN prjMakefilesGenerator.sh REQUIRED)

message(STATUS "MCHP Project generator at: ${MCHP_PROJ_GEN}")


function(add_pic_fw_target SOURCE_DIR PROJECT_FILE)

    execute_process(COMMAND ${MCHP_PROJ_GEN} ${SOURCE_DIR}
        OUTPUT_VARIABLE OUT_VAR
        ERROR_VARIABLE ERR_VAR
        RESULT_VARIABLE RES_VAR
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        COMMAND_ERROR_IS_FATAL ANY
        )

    add_custom_command(
        SOURCE_DIR ${TARGET_NAME}.h ${TARGET_NAME}.c 
        COMMAND ${MCHP_PROJ_GEN} ${SOURCE_DIR} 
        DEPENDS ${SOURCE_DIR}/${PROJECT_FILE} ${SOURCE_DIR}/nbproject/project.xml ${SOURCE_DIR}/nbproject/configurations.xml
        )

endfunction()