cmake_minimum_required(VERSION 3.20)

# FIXME: We do `go build -a` to refresh the cache build support
# if libmtdpartctl_if is updated then we will build with the new library.
# Maybe find a way to wrap that library in a dependency so it can be used
# without this hack in the future.

add_custom_target(mtdpartctl ALL
    COMMAND ${CMAKE_COMMAND} -E env
        CGO_ENABLED=1
	CC=${CMAKE_C_COMPILER}
	GOOS=linux
	GOARCH=${GO_ARCH}
	GOARM=${GO_ARM_LVL}
	GOROOT=${GO_SYMLINKED_DIR}
	CGO_CFLAGS=${GO_GLOBAL_CFLAGS}
	CGO_LDFLAGS=${GO_GLOBAL_LDFLAGS}
	${GO_EXECUTABLE} build -a -ldflags ${GO_GLOBAL_EXTLDFLAGS} -o ${CMAKE_BINARY_DIR}/user/mtdpartctl .
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM

	DEPENDS mtdpartctl_if
)

add_dependencies(mtdpartctl mtdpartctl_if generate_cgo_symlink)
