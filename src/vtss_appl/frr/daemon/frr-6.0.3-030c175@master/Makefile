.PHONY: all clean mrproper c-ares json-c pcre yang frr

#MSCC_SDK_ARCH ?= mipsel
#MSCC_SDK_ARCH ?= arm64
#MSCC_SDK_ARCH ?= arm
#MSCC_SDK_ARCH ?= x86

ifeq ($(MSCC_SDK_ARCH),)
arch :=
else
arch := --arch $(MSCC_SDK_ARCH)
endif

all:
	@./build-quagga.rb $(arch)

# Remove ./build
clean:
	@./build-quagga.rb --clean

# Remove ./build and the download folder
mrproper:
	@./build-quagga.rb --clean
	@rm -rf dl

# Build c-ares (library for asynchronous DNS requests (including name resolves)
c-ares:
	@./build-quagga.rb --build c-ares $(arch)

# Build json-c (library for creating JSON objects in C)
json-c:
	@./build-quagga.rb --build json-c $(arch)

# Build pcre
pcre:
	@./build-quagga.rb --build pcre $(arch)

# Build yang
yang:
	@./build-quagga.rb --build yang $(arch)

# Build frr (zebra, zebra+ospf, zebra+ospf+rip)
frr:
	@./build-quagga.rb --build frr $(arch)

