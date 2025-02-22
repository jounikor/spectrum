set (ZXPAC4_DECOMPRESSOR_ASM_LIST z80tap) 

foreach(BASE ${ZXPAC4_DECOMPRESSOR_ASM_LIST})
    string(CONCAT FILE_ASM ${BASE} ".asm")
    
    # short match length
    string(CONCAT FILE_BIN_255_32K ${BASE} "_255_32k.bin")
    string(CONCAT FILE_255_32K_H ${BASE} "_255_32k.h")
    string(CONCAT FILE_LABEL_255_32K ${BASE} "_255_32k_bin")
    string(CONCAT FILE_TARGET_255_32K ${BASE} "_target_255_32k")

    add_custom_target(${FILE_TARGET_255_32K}
        DEPENDS ${FILE_255_32K_H}
        COMMENT "Target ${FILE_TARGET_255_32K} for  ${FILE_255_32K_H}"
        #VERBATIM
    )
    
    # short
    add_dependencies(${TARGET} ${FILE_TARGET_255_32K})

    # max match 255, window 32k, inplace decompression
    add_custom_command(
        OUTPUT ${FILE_255_32K_H}
        COMMAND pasmo --equ MAX32K_WIN=1 --equ INPLACE=1 --equ ASCII_LITERALS=0 --alocal ${FILE_ASM} ${FILE_BIN_255_32K}
        COMMAND python3 ../bin/bin2c.py ${FILE_LABEL_255_32K} ${FILE_BIN_255_32K} ${FILE_255_32K_H}
        #COMMAND python3 ../bin/bin2c.py \"\" ${FILE_BIN_255_32K} ${FILE_255_32K_H}
        COMMENT "Creating ${FILE_255_32K_H} from ${FILE_ASM} and ${FILE_BIN_255_32K} for target ${FILE_TARGET_255_32K}"
        DEPENDS ${FILE_ASM}
        #VERBATIM
    )
endforeach()

