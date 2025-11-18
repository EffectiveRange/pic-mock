
function(add_mock_library TARGET_NAME)

execute_process(COMMAND bash -c "find /opt/microchip/mplabx -name ${TARGET_NAME}.h" OUTPUT_VARIABLE ${TARGET_NAME}_FILE)

if(NOT ${TARGET_NAME}_FILE)
    message(FATAL_ERROR "${TARGET_NAME}_FILE not found")
endif()

set(TARGET_FILE ${${TARGET_NAME}_FILE})

message("${TARGET_NAME}_FILE: ${TARGET_FILE}")

add_custom_command(
    OUTPUT ${TARGET_NAME}.h ${TARGET_NAME}.c 
    COMMAND ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../picpre.py --output ${CMAKE_CURRENT_BINARY_DIR} ${TARGET_FILE} 
    DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../picpre.py 
    )

add_custom_command(
    OUTPUT version.h
    COMMAND ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../fw_ver.py ${PROJECT_SOURCE_DIR}/version.h.in
    DEPENDS ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../fw_ver.py ${PROJECT_SOURCE_DIR}/version.h.in
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )

add_library(mock_${TARGET_NAME} SHARED 
${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.c  
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/mock_hw.cpp 
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/test_main.cpp
)

target_include_directories(mock_${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../include)

target_link_libraries(mock_${TARGET_NAME} PUBLIC Catch2::Catch2)

endfunction()