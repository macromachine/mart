
if (MART_GENMU)
	#set(LLVM_LINK_COMPONENTS core engine asmparser bitreader support)

	#execute_process(COMMAND git rev-parse --short=8 HEAD OUTPUT_VARIABLE GITVERSION)
	#string(REGEX REPLACE "\n$" "" GITVERSION "${GITVERSION}")
	#add_definitions(-DGIT_VERSION="${GITVERSION}")

	#add_executable(mart MART.cpp)
	#target_link_libraries(mart MART_GenMu)

	#install(TARGETS mart
	#	RUNTIME DESTINATION bin)
		
	add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ks_mutation.sh ${CMAKE_CURRENT_BINARY_DIR}/FaRM ${CMAKE_CURRENT_BINARY_DIR}/utils
                 POST_BUILD 
		 DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/ks_mutation.sh ${CMAKE_CURRENT_SOURCE_DIR}/FaRM ${CMAKE_CURRENT_SOURCE_DIR}/utils
                 COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/ks_mutation.sh ${CMAKE_CURRENT_BINARY_DIR}/ks_mutation.sh
		 COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/FaRM ${CMAKE_CURRENT_BINARY_DIR}/FaRM
		 COMMAND cp -rf ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_BINARY_DIR}/utils
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/)
    
    add_custom_target(
                 CopyScripts ALL
		 DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ks_mutation.sh ${CMAKE_CURRENT_BINARY_DIR}/FaRM ${CMAKE_CURRENT_BINARY_DIR}/utils
                )
                
endif (MART_GENMU)

#include_directories('../lib')
