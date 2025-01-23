set (ZXPAC4_DECOMPRESSOR_ASM_LIST zxpac4_exe zxpac4_abs) 

foreach(BASE ${ZXPAC4_DECOMPRESSOR_ASM_LIST})
    string(CONCAT FILE_ASM ${BASE} ".asm")
    
    # large match length
    string(CONCAT FILE_BIN ${BASE} ".bin")
    string(CONCAT FILE_BIN_32K ${BASE} "_32k.bin")
    string(CONCAT FILE_H ${BASE} ".h")
    string(CONCAT FILE_32K_H ${BASE} "_32k.h")
    string(CONCAT FILE_LABEL ${BASE} "_bin")
    string(CONCAT FILE_LABEL_32K ${BASE} "_32k_bin")
    string(CONCAT FILE_TARGET ${BASE} "_target")
    string(CONCAT FILE_TARGET_32K ${BASE} "_target_32k")
    
    # short match length
    string(CONCAT FILE_BIN_255 ${BASE} ".bin")
    string(CONCAT FILE_BIN_255_32K ${BASE} "_255_32k.bin")
    string(CONCAT FILE_255_H ${BASE} "_255.h")
    string(CONCAT FILE_255_32K_H ${BASE} "_255_32k.h")
    string(CONCAT FILE_LABEL_255 ${BASE} "_255_bin")
    string(CONCAT FILE_LABEL_255_32K ${BASE} "_255_32k_bin")
    string(CONCAT FILE_TARGET_255 ${BASE} "_target_255")
    string(CONCAT FILE_TARGET_255_32K ${BASE} "_target_255_32k")

    # large match length
    add_custom_target(${FILE_TARGET}
        DEPENDS ${FILE_H}
        COMMENT "Target ${FILE_TARGET} for  ${FILE_H}"
        #VERBATIM
    )
    add_custom_target(${FILE_TARGET_32K}
        DEPENDS ${FILE_32K_H}
        COMMENT "Target ${FILE_TARGET_32K} for  ${FILE_32K_H}"
        #VERBATIM
    )
    
    # short match length
    add_custom_target(${FILE_TARGET_255}
        DEPENDS ${FILE_255_H}
        COMMENT "Target ${FILE_TARGET_255} for  ${FILE_255_H}"
        #VERBATIM
    )
    add_custom_target(${FILE_TARGET_255_32K}
        DEPENDS ${FILE_255_32K_H}
        COMMENT "Target ${FILE_TARGET_255_32K} for  ${FILE_255_32K_H}"
        #VERBATIM
    )
    
    # large
    add_dependencies(${TARGET} ${FILE_TARGET})
    add_dependencies(${TARGET} ${FILE_TARGET_32K})
    # short
    add_dependencies(${TARGET} ${FILE_TARGET_255})
    add_dependencies(${TARGET} ${FILE_TARGET_255_32K})

    # max match 65535, window 128k
    add_custom_command(
        OUTPUT ${FILE_H}
        COMMAND vasmm68k_mot -no-opt -DLARGE_MAX_MATCH -Fbin ${FILE_ASM} -o ${FILE_BIN}
        COMMAND python3 bin2c.py ${FILE_LABEL} ${FILE_BIN} ${FILE_H}
        COMMENT "Creating ${FILE_H} from ${FILE_ASM} and ${FILE_BIN} for target ${FILE_TARGET}"
        DEPENDS ${FILE_ASM}
        #VERBATIM
    )
    # max match 65535, window 32k
    add_custom_command(
        OUTPUT ${FILE_32K_H}
        COMMAND vasmm68k_mot -no-opt -DMAX32K_WIN -DLARGE_MAX_MATCH -Fbin ${FILE_ASM} -o ${FILE_BIN_32K}
        COMMAND python3 bin2c.py ${FILE_LABEL_32K} ${FILE_BIN_32K} ${FILE_32K_H}
        COMMENT "Creating ${FILE_32K_H} from ${FILE_ASM} and ${FILE_BIN_32K} for target ${FILE_TARGET_32K}"
        DEPENDS ${FILE_ASM}
        #VERBATIM
    )
    # max match 255, window 128k
    add_custom_command(
        OUTPUT ${FILE_255_H}
        COMMAND vasmm68k_mot -no-opt -Fbin ${FILE_ASM} -o ${FILE_BIN_255}
        COMMAND python3 bin2c.py ${FILE_LABEL_255} ${FILE_BIN_255} ${FILE_255_H}
        COMMENT "Creating ${FILE_255_H} from ${FILE_ASM} and ${FILE_BIN_255} for target ${FILE_TARGET_255}"
        DEPENDS ${FILE_ASM}
        #VERBATIM
    )

    # max match 255, window 32k
    add_custom_command(
        OUTPUT ${FILE_255_32K_H}
        COMMAND vasmm68k_mot -no-opt -DMAX32K_WIN -Fbin ${FILE_ASM} -o ${FILE_BIN_255_32K}
        COMMAND python3 bin2c.py ${FILE_LABEL_255_32K} ${FILE_BIN_255_32K} ${FILE_255_32K_H}
        COMMENT "Creating ${FILE_255_32K_H} from ${FILE_ASM} and ${FILE_BIN_255_32K} for target ${FILE_TARGET_255_32K}"
        DEPENDS ${FILE_ASM}
        #VERBATIM
    )
endforeach()

