cmake_minimum_required(VERSION 3.20)

find_program(GO_EXECUTABLE go REQUIRED)

set(GO_UNIT_TEST_CFLAGS
	"-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/user/common"
)

set(GO_UNIT_TEST_LDFLAGS
	"-L${CMAKE_BINARY_DIR}/user/common -lmtdpartctl -static"
)

message("Build for ${KERNEL_ARCH}, ${CMAKE_C_COMPILER}, ${SYSROOT_PATH}")

# FIXME: Sadly, the buildroot GCC will abort because paths like
# /usr/lib/go/src/runtime/cgo is considered a leak of host libraries into
# our compilation environment.
# Bypass this by putting a symlink which masquerades this fact from the compiler.
set(GO_SYMLINKED_DIR "${CMAKE_BINARY_DIR}/go-cgo")

execute_process(
    COMMAND go env GOROOT
    OUTPUT_VARIABLE GO_ROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Go root directory: ${GO_ROOT}")

add_custom_command(
    OUTPUT ${GO_SYMLINKED_DIR}
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${GO_ROOT}
	    ${GO_SYMLINKED_DIR}
    COMMENT "Creating local Go cgo include symlink"
)

add_custom_target(get_info_golang_test ALL
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_ENABLED=1
	CC=${CMAKE_C_COMPILER}
	GOOS=linux
	GOARCH=${GO_ARCH}
	GOARM=${GO_ARM_LVL}
	GOROOT=${GO_SYMLINKED_DIR}
	CGO_CFLAGS=${GO_UNIT_TEST_CFLAGS}
	CGO_LDFLAGS=${GO_UNIT_TEST_LDFLAGS}
	${GO_EXECUTABLE} build -o ${CMAKE_BINARY_DIR}/user/get_info_golang get_info.go
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
    DEPENDS ${GO_SYMLINKED_DIR}
)

add_dependencies(get_info_golang_test mtdpartctl)
