# Source code

set(LLHTTP_SOURCES
  llhttp.c
  http.c
  api.c
)

set_source_files_properties(${LLHTTP_SOURCES} PROPERTIES LANGUAGE C)
add_library(llhttp SHARED ${LLHTTP_SOURCES})
