# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\ChallengeDXCC_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ChallengeDXCC_autogen.dir\\ParseCache.txt"
  "ChallengeDXCC_autogen"
  )
endif()
