cmake_minimum_required(VERSION 3.20)

find_program(GO_EXECUTABLE go REQUIRED)

set(CGO_CFLAGS
	"-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_SOURCE_DIR}/user/common"
)

set(CGO_LDFLAGS
    "-L${CMAKE_BINARY_DIR}/user/common -lmtdpartctl -static"
)

add_custom_target(get_info_golang_test ALL
    COMMAND
        ${CMAKE_COMMAND} -E env
        CGO_ENABLED=1
        CGO_CFLAGS=${CGO_CFLAGS}
        CGO_LDFLAGS=${CGO_LDFLAGS}
        ${GO_EXECUTABLE} build -o ${CMAKE_BINARY_DIR}/get_info_golang get_info.go
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_dependencies(get_info_golang_test mtdpartctl)
