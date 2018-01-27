// ios wants one file for all platforms
// as it is quite hard to configure xcode to build separate files per platform

#if defined(__i386__)
  #include "asm/make_i386_sysv_macho_gas.S"
#elif defined(__x86_64__)
  #include "asm/make_x86_64_sysv_macho_gas.S"
#elif defined(__arm__)
  #include "asm/make_arm_aapcs_macho_gas.S"
#elif defined(__aarch64__)
  #include "asm/make_arm64_aapcs_macho_gas.S"
#else
  #error "Unknown iOS arch"
#endif
