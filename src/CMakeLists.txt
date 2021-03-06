# Copyright 2019 Triad National Security, LLC. All rights reserved.
# 
# This file is part of the MSTK project. Please see the license file at
# the root of this repository or at
# https://github.com/MeshToolkit/MSTK/blob/master/LICENSE

# Copyright 2019 Triad National Security, LLC. All rights reserved.
#
# This file is part of the MSTK project. Please see the license file at
# the root of this repository or at
# https://github.com/MeshToolkit/MSTK/blob/master/LICENSE

file (GLOB FILES base/*.c)
foreach (FILE ${FILES})
  set (basefiles ${basefiles} ${FILE})
endforeach ()

file (GLOB FILES hilev/*.c)
foreach (FILE ${FILES})
  set (hilevfiles ${hilevfiles} ${FILE})
endforeach ()

file (GLOB FILES io/*.c)
foreach (FILE ${FILES})
  get_filename_component(RELFILE ${FILE} NAME) 
  if (${RELFILE} MATCHES ExodusII OR ${RELFILE} MATCHES NemesisI)
    if (ENABLE_ExodusII)
      set (iofiles ${iofiles} ${FILE})
    endif ()
  else ()
    set (iofiles ${iofiles} ${FILE})
  endif ()
endforeach ()

file (GLOB FILES misc/*.c)
foreach (FILE ${FILES})
  set (miscfiles ${miscfiles} ${FILE})
endforeach ()

file (GLOB FILES util/*.c)
foreach (FILE ${FILES})
  set (utilfiles ${utilfiles} ${FILE})
endforeach ()

file (GLOB FILES mod/*.c)
foreach (FILE ${FILES})
  set (modfiles ${modfiles} ${FILE})
endforeach ()

if (ENABLE_PARALLEL)
  file (GLOB FILES par/*.c)
  foreach (FILE ${FILES})
    get_filename_component(RELFILE ${FILE} NAME) 
    if (${RELFILE} MATCHES Metis)
      if (ENABLE_METIS)
        set (parfiles ${parfiles} ${FILE})
      endif ()
    elseif (${RELFILE} MATCHES Zoltan)
      if (ENABLE_ZOLTAN)
        set (parfiles ${parfiles} ${FILE})
      endif ()
    else ()
      set (parfiles ${parfiles} ${FILE})
    endif ()
  endforeach ()
endif ()

set(mstkfiles ${basefiles} ${hilevfiles} ${iofiles} ${miscfiles} ${utilfiles} ${modfiles} ${parfiles} PARENT_SCOPE)

