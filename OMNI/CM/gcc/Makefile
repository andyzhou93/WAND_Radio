TARGET_CHIP := NRF51822_QFAA_CA
BOARD := BOARD_PCA10001

C_SOURCE_FILES += main_cm.c
C_SOURCE_FILES += radio_config.c
C_SOURCE_FILES += nrf_delay.c
C_SOURCE_FILES += data_fifo.c
C_SOURCE_FILES += command_fifo.c
C_SOURCE_FILES += timer.c
C_SOURCE_FILES += spi.c
C_SOURCE_FILES += spi_fifo.c
C_SOURCE_FILES += spi_slave.c

OUTPUT_FILENAME := cm_radio
SDK_PATH = ../../../../


#Uncomment correct line if you have s110 programmed on the chip.
DEVICE_VARIANT := xxaa
#DEVICE_VARIANT := xxab

#USE_SOFTDEVICE := s110
#USE_SOFTDEVICE := s210

include $(SDK_PATH)Source/templates/gcc/Makefile.common

INCLUDEPATHS += -I"$(SDK_PATH)Include/app_common"
INCLUDEPATHS += -I"$(SDK_PATH)Include/sd_common"
INCLUDEPATHS += -I"$(SDK_PATH)Include/s110"
