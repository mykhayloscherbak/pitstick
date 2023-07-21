cmake_minimum_required(VERSION 3.16)
project(pitstick)
enable_language (C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    
    	sources/project/bl/src/bll.c
    	sources/project/bl/src/heartbeat.c
    	sources/project/bl/src/bll.c
    	sources/project/bl/src/led_control.c
    	sources/project/bl/src/prng.c
		sources/project/dl/src/led_strip.c
		sources/project/dl/src/rgbw.c
		sources/project/hal/src/adc.c
		sources/project/hal/src/gpio.c
		sources/project/hal/src/buttons.c
		sources/project/hal/src/clock.c
		sources/project/hal/src/eeemu.c
		sources/project/hal/src/watchdog.c
		sources/project/hal/src/timer_dma.c
		sources/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.S
		sources/project/main.c
		sources/project/syscalls.c
)

set(EXECUTABLE ${PROJECT_NAME}.elf)
add_executable(${EXECUTABLE} ${SOURCES})

set(LDSCRIPT ${CMAKE_SOURCE_DIR}/scripts/STM32F103C8_FLASH.ld )
target_compile_definitions(${EXECUTABLE} PRIVATE
		-DSTM32F103xB
		)

target_include_directories(${EXECUTABLE} PRIVATE
		sources/project/bl/include
		sources/project/conf
		sources/project/dl/include
		sources/project/hal/include
		sources/CMSIS/Include
		sources/CMSIS/Device/ST/STM32F1xx/Include
		)
    
target_compile_options(${EXECUTABLE} PRIVATE
        ${EXECUTION_ENVIRONMENT}
        -mthumb 
	-mcpu=cortex-m3
        -fno-builtin 
        -Wextra 
        -Werror
        -ffunction-sections 
        -fdata-sections 
        -fomit-frame-pointer 
        -mabi=aapcs
         $<$<CONFIG:Debug>:-Og>
        )

target_link_options(${EXECUTABLE} PRIVATE 
        -T${LDSCRIPT}                     
        -mcpu=cortex-m4
        -Wl,-Map=${PROJECT_NAME}.map      
        -Wl,--gc-sections                 
        -static                           
        -mthumb                           
        -Wl,--start-group                 
        -lgcc                             
        -Wl,--end-group                   
        -Wl,--print-memory-usage   
        -Wl,--no-warn-rwx-segment       
        )                                 
                                          
add_custom_command(TARGET ${EXECUTABLE}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin)     

set_target_properties(${EXECUTABLE} PROPERTIES ADDITIONAL_CLEAN_FILES "${PROJECT_NAME}.bin;${PROJECT_NAME}.hex;${PROJECT_NAME}.map")
