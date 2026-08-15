cmake_minimum_required(VERSION 3.20)

add_custom_target(get_info_golang_test ALL
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_ENABLED=1
	CC=${CMAKE_C_COMPILER}
	GOOS=linux
	GOARCH=${GO_ARCH}
	GOARM=${GO_ARM_LVL}
	GOROOT=${GO_SYMLINKED_DIR}
	CGO_CFLAGS=${GO_GLOBAL_CFLAGS}
	CGO_LDFLAGS=${GO_GLOBAL_LDFLAGS}
	${GO_EXECUTABLE} build -ldflags ${GO_GLOBAL_EXTLDFLAGS} -o ${CMAKE_BINARY_DIR}/user/get_info_golang get_info.go
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_dependencies(get_info_golang_test mtdpartctl_if generate_cgo_symlink)

# C utests

set(MTDPARTCTL_UTESTS
    add_invalid_context_part
    add_invalid_new_part
    add_overlapping_context_parts
    add_part_no_context
    create_and_list_nandsim_parts
    create_empty_context
    create_nandsim_parts_while_holding_ref
    create_nandsim_parts
    del_non_existing_part
    del_part
    get_info
    invalid_ioctl
    restart_context
    restart_mtd_while_holding_ref
    restart_mtd
)

foreach(t ${MTDPARTCTL_UTESTS})
    add_executable(${t} ${t}.c)

    target_include_directories(${t} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/user/common
        ${CMAKE_SOURCE_DIR}/user/utests
    )

    target_compile_options(${t} PRIVATE -Wall -Wextra -Werror)
    target_link_options(${t} PRIVATE -static)
    target_link_libraries(${t} PRIVATE mtdpartctl_if)

    add_dependencies(${t} mtdpartctl_if)

    # Optional: debug build support
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${t} PRIVATE -O0 -g)
    endif()
endforeach()
