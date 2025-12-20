

find_package(Catch2 REQUIRED)

add_library(app_customization_test SHARED 
    app.c 
    i2c_regs.c 
    ../../mcc_generated_files/i2c_client/src/i2c1.c
)

target_include_directories(app_customization_test PUBLIC . ../../)

add_mock_library(pic18f16q20_pictest app_customization_test) 

add_mocked_test(pic18f16q20_pictest test_timers)
add_mocked_test(pic18f16q20_pictest test_i2c_client)

target_compile_definitions(test_timers PRIVATE PIC_HW_MAIN_ENTRY_POINT=hw_main_timers_only)
