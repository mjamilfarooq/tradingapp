

set(UTILS_SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/AsynchronousQueue.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
   PARENT_SCOPE
)
set(UTILS_HEADERS
   ${CMAKE_CURRENT_SOURCE_DIR}/log.h
   ${CMAKE_CURRENT_SOURCE_DIR}/AsynchronousQueue.h
   ${CMAKE_CURRENT_SOURCE_DIR}/AsynchronousUnorderedMap.h
   ${CMAKE_CURRENT_SOURCE_DIR}/AsynchronousVector.h
   ${CMAKE_CURRENT_SOURCE_DIR}/ThreadPool.h
   PARENT_SCOPE
)
