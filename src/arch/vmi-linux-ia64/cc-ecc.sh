VMI_INCDIR="-I/opt/vmi-2.0b-3-gcc/include"
VMI_LIBDIR="-L/opt/vmi-2.0b-3-gcc/lib"
#
CMK_CPP_C="ecc -E $CMK_INCDIR $VMI_INCDIR "
CMK_CC="ecc $CMK_INCDIR $VMI_INCDIR "
CMK_CC_RELIABLE="ecc $CMK_INCDIR $VMI_INCDIR "
CMK_CC_FASTEST="ecc $CMK_INCDIR $VMI_INCDIR "
CMK_CXX="ecpc $CMK_INCDIR $VMI_INCDIR "
CMK_CXXPP="ecpc -E $CMK_INCDIR $VMI_INCDIR "
CMK_LD="ecc -rdynamic -pthread $VMI_LIBDIR "
CMK_LDXX="ecpc -rdynamic -pthread $VMI_LIBDIR "
CMK_LD_LIBRARY_PATH="-Wl,-rpath,$CHARMLIBSO/"
CMK_NATIVE_CC="ecc $CMK_INCDIR $VMI_INCDIR "
CMK_NATIVE_LD="ecc $CMK_INCDIR $VMI_INCDIR "
CMK_NATIVE_CXX="ecpc $CMK_INCDIR $VMI_INCDIR "
CMK_NATIVE_LDXX="ecpc $CMK_INCDIR $VMI_INCDIR "

