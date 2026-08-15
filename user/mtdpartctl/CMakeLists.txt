cmake_minimum_required(VERSION 3.20)

add_custom_target(mtdpartctl ALL
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_ENABLED=1
	CC=${CMAKE_C_COMPILER}
	GOOS=linux
	GOARCH=${GO_ARCH}
	GOARM=${GO_ARM_LVL}
	GOROOT=${GO_SYMLINKED_DIR}
	CGO_CFLAGS=${GO_UNIT_TEST_CFLAGS}
	CGO_LDFLAGS=${GO_UNIT_TEST_LDFLAGS}
	${GO_EXECUTABLE} build -o ${CMAKE_BINARY_DIR}/user/mtdpartctl .
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_dependencies(mtdpartctl mtdpartctl_if generate_cgo_symlink)
