# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")



# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(FREERTOS_KERNEL_PATH "D:/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(SistemaDeControle C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME} src/main.c src/ssd1306.c src/matriz_5x5.c
                src/sistema.c
                )

pico_set_program_name(${PROJECT_NAME} "Sistema de Controle de Acesso")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Define o diretório de saída apenas para os arquivos de cabeçalho
set(PIO_HEADER_OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/inc)

# Gera o arquivo PIO na pasta especificada apenas para headers
pico_generate_pio_header(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/InterfacePio.pio 
    OUTPUT_DIR ${PIO_HEADER_OUTPUT_DIR}
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        FreeRTOS-Kernel         # Kernel do FreeRTOS
        FreeRTOS-Kernel-Heap4   # Gerenciador de memoria
        )
# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME} 
        hardware_i2c
        hardware_pio
        hardware_pwm
        hardware_adc
        hardware_timer
        hardware_clocks
        )

pico_add_extra_outputs(${PROJECT_NAME})