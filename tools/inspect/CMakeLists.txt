# Copyright (c) 2015 Hartmut Kaiser
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# add inpect executable

add_hpx_executable(inspect
  AUTOGLOB
  NOLIBS
  FOLDER "Tools/Inspect")

if(NOT Boost_REGEX_FOUND)
  hpx_error("HPX inspect tool requires Boost.Regex")
endif()

target_link_libraries(inspect ${HPX_TLL_PUBLIC} ${Boost_LIBRARIES})

# add dependencies to pseudo-target
add_hpx_pseudo_dependencies(tools.inspect inspect)

