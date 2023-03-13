# dump_headers:
# 	clang++ --std=c++17 -x c++-header example.cpp -Xclang -emit-pch -o example.pch 
dump_flags:
	clang -### -c example.cpp
 
# dump: example.cpp
	# clang++ -fdump-record-layouts example.cpp -o example -O3 
	# clang -cc1 -x c++ -include-pch example.pch  -std=c++17 -v -fdump-record-layouts -fdump-vtable-layouts -D CHECKING_LAYOUT example.cpp -emit-llvm
	# clang -cc1 -x c++ -std=c++17  \
	# "-stdlib=libc++" "-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1" "-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/local/include" "-internal-isystem" "/Library/Developer/CommandLineTools/usr/lib/clang/14.0.0/include" "-internal-externc-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include" "-internal-externc-isystem" "/Library/Developer/CommandLineTools/usr/include" \
	# -v -fdump-record-layouts -fdump-vtable-layouts example.cpp -emit-llvm
	# clang++ -cc1 -fdump-vtable-layouts -emit-llvm example.cpp 
	# "-resource-dir" "/Library/Developer/CommandLineTools/usr/lib/clang/14.0.0" "-isysroot" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" "-I/usr/local/include" "-stdlib=libc++" "-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1" "-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/local/include" "-internal-isystem" "/Library/Developer/CommandLineTools/usr/lib/clang/14.0.0/include" "-internal-externc-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include" "-internal-externc-isystem" "/Library/Developer/CommandLineTools/usr/include" 
# USE dump_flags to generate all the flags of this command, make user to reuse the last line with "-x   c++  -v -fdump-record-layouts -fdump-vtable-layouts  example.cpp" 
dump: example.cpp
	/Library/Developer/CommandLineTools/usr/bin/clang "-cc1" "-triple" "arm64-apple-macosx12.0.0" \
	-std=c++17 \
	"-Wundef-prefix=TARGET_OS_" \
	"-Wdeprecated-objc-isa-usage" "-Werror=deprecated-objc-isa-usage" "-Werror=implicit-function-declaration" "-emit-obj" "-mrelax-all" \
	"--mrelax-relocations" "-disable-free" "-clear-ast-before-backend" "-disable-llvm-verifier" "-discard-value-names" "-main-file-name" \
	"example.cpp" "-mrelocation-model" "pic" "-pic-level" "2" "-mframe-pointer=non-leaf" "-fno-strict-return" "-fno-rounding-math" \
	"-funwind-tables=2" "-fobjc-msgsend-selector-stubs" "-target-sdk-version=13.0" "-fvisibility-inlines-hidden-static-local-var" \
	"-target-cpu" "apple-m1" "-target-feature" "+v8.5a" "-target-feature" "+fp-armv8" "-target-feature" "+neon" "-target-feature" \
	"+crc" "-target-feature" "+crypto" "-target-feature" "+dotprod" "-target-feature" "+fp16fml" "-target-feature" "+ras" \
	"-target-feature" "+lse" "-target-feature" "+rdm" "-target-feature" "+rcpc" "-target-feature" "+zcm" "-target-feature" "+zcz" \
	"-target-feature" "+fullfp16" "-target-feature" "+sm4" "-target-feature" "+sha3" "-target-feature" "+sha2" "-target-feature" "+aes" \
	"-target-abi" "darwinpcs" "-fallow-half-arguments-and-returns" "-debugger-tuning=lldb" "-target-linker-version" "820.1" \
	"-resource-dir" "/Library/Developer/CommandLineTools/usr/lib/clang/14.0.0" "-isysroot" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" \
	"-I/usr/local/include" "-stdlib=libc++" "-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1" \
	"-internal-isystem" "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/local/include" \
	"-internal-isystem" "/Library/Developer/CommandLineTools/usr/lib/clang/14.0.0/include" "-internal-externc-isystem" \
	"/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include" "-internal-externc-isystem" \
	"/Library/Developer/CommandLineTools/usr/include" "-Wno-reorder-init-list" "-Wno-implicit-int-float-conversion" \
	"-Wno-c99-designator" "-Wno-final-dtor-non-final-class" "-Wno-extra-semi-stmt" "-Wno-misleading-indentation" \
	"-Wno-quoted-include-in-framework-header" "-Wno-implicit-fallthrough" "-Wno-enum-enum-conversion" "-Wno-enum-float-conversion" \
	"-Wno-elaborated-enum-base" "-Wno-reserved-identifier" "-Wno-gnu-folding-constant" "-Wno-cast-function-type" "-Wno-bitwise-instead-of-logical" \
	"-fdeprecated-macro" "-fdebug-compilation-dir=/Users/mapg/misc/testStuff/testVirtInh" "-ferror-limit" "19" "-stack-protector" "1" \
	"-fstack-check" "-mdarwin-stkchk-strong-link" "-fblocks" "-fencode-extended-block-signature" "-fregister-global-dtors-with-atexit" \
	"-fgnuc-version=4.2.1" "-fno-cxx-modules" "-fcxx-exceptions" "-fexceptions" "-fmax-type-align=16" "-fcommon" "-fcolor-diagnostics" \
	"-clang-vendor-feature=+messageToSelfInClassMethodIdReturnType" "-clang-vendor-feature=+disableInferNewAvailabilityFromInit" \
	"-clang-vendor-feature=+disableNonDependentMemberExprInCurrentInstantiation" "-fno-odr-hash-protocols" \
	"-clang-vendor-feature=+enableAggressiveVLAFolding" "-clang-vendor-feature=+revert09abecef7bbf" \
	"-clang-vendor-feature=+thisNoAlignAttr" "-clang-vendor-feature=+thisNoNullAttr" \
	"-mllvm" "-disable-aligned-alloc-awareness=1" "-D__GCC_HAVE_DWARF2_CFI_ASM=1" \
	"-x" "c++" -v -fdump-record-layouts -fdump-vtable-layouts "example.cpp" 
	# -fdump-record-layouts 
	# -emit-llvm

build17: example.cpp
	# -Woverloaded-virtual 
	clang++ --std=c++17 -fdiagnostics-show-template-tree -fno-elide-type -g -O0 example.cpp -o example 
	
build11: example.cpp
	# -Woverloaded-virtual 
	clang++ --std=c++11 -fdiagnostics-show-template-tree -fno-elide-type -g -O0 example.cpp -o example 
