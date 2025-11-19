
function(add_mock_library TARGET_NAME APP_LIB)

execute_process(COMMAND bash -c "find /opt/microchip/mplabx -name ${TARGET_NAME}.h" 
OUTPUT_VARIABLE ${TARGET_NAME}_FILE
COMMAND_ERROR_IS_FATAL ANY)

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
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw/i2c_app.c
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw/modules.c
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw/tasks.c
${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw/timers.c
)

target_include_directories(${APP_LIB} PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../include ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(mock_${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../ ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../include ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../basefw)

target_link_libraries(mock_${TARGET_NAME} PUBLIC ${APP_LIB} Catch2::Catch2)

endfunction()


function(add_mocked_test TARGET_NAME TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PUBLIC  mock_${TARGET_NAME} Catch2::Catch2)
    add_test(${TEST_NAME} ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES 
    ENVIRONMENT "LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/lsan-suppressions.txt;TSAN_OPTIONS=verbosity=1:abort_on_error=1:suppressions=${PROJECT_SOURCE_DIR}/tsan-suppressions.txt;ASAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/asan-suppressions.txt:fast_unwind_on_malloc=0:verbosity=1:abort_on_error=1;UBSAN_OPTIONS=verbosity=1:abort_on_error=1;")
endfunction()