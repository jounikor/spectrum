set (ZXPAC4_DECOMPRESSOR_ASM_LIST zxpac4_exe zxpac4_32k_exe zxpac4_abs zxpac4_32k_abs) 

foreach(BASE ${ZXPAC4_DECOMPRESSOR_ASM_LIST})
    string(CONCAT FILE_ASM ${BASE} ".asm")
    string(CONCAT FILE_BIN ${BASE} ".bin")
    string(CONCAT FILE_H ${BASE} ".h")
    string(CONCAT FILE_LABEL ${BASE} "_bin")
    string(CONCAT FILE_TARGET ${BASE} "_target")

    add_custom_target(${FILE_TARGET}
        DEPENDS ${FILE_H}
        COMMENT "Target ${FILE_TARGET} for  ${FILE_H}"
        VERBATIM
    )
    
    add_dependencies(${TARGET} ${FILE_TARGET})
    
    add_custom_command(
        OUTPUT ${FILE_H}
        COMMAND vasmm68k_mot -Fbin ${FILE_ASM} -o ${FILE_BIN}
        COMMAND python3 bin2c.py ${FILE_LABEL} ${FILE_BIN} ${FILE_H}
        #WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/m68k/
        COMMENT "Creating ${FILE_H} from ${FILE_ASM} and ${FILE_BIN} for target ${FILE_TARGET}"
        DEPENDS ${FILE_ASM}
        VERBATIM
    )
endforeach()

