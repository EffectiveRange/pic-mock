

function(pic_fw_get_semver OUTPUT_NAME)
    execute_process(
        COMMAND ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../fw_ver.py --semver
        RESULT_VARIABLE fw_ver_result
        OUTPUT_VARIABLE fw_ver 
        COMMAND_ERROR_IS_FATAL ANY
    )
    string(STRIP ${fw_ver} fw_ver)
    set(${OUTPUT_NAME} ${fw_ver} PARENT_SCOPE)    
endfunction()