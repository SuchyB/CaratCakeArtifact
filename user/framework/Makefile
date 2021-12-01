# 
#  Copyright (c) 2017 Peter Dinda 
#
# Framework code for building "user" executables for Nautilus
#

NAUTILUS_DIR = ../..

TARGET_NAME=hello

TARGET = $(TARGET_NAME).exe

USER_OBJ = $(TARGET_NAME).o

BLOB_OBJ = blob.o
BLOB_BC = blob.bc
BLOB_LL = blob.ll
BLOB_SIMPLIFY_LL = blob_simplify.ll
BLOB_OPT_LL = blob_opt.ll
BLOB_KOBJ = blob.kobj.o

//CC = gcc

CC = gclang 
ME = opt
FINALCC = clang
LD = ld
NLD = ./nld-noelle-ubuntu
NSIGN = ./nsign
EXTRACTBC = extract-bc
LLVMDIS   = llvm-dis

KARAT_TRANS = cp

# Flags must be compatible with the NK kernel flags
# CFLAGS = -O2 \
# 	 -fno-omit-frame-pointer \
# 	 -ffreestanding \
# 	 -fno-stack-protector \
# 	 -fno-strict-aliasing \
#          -fno-strict-overflow \
# 	 -fno-delete-null-pointer-checks \
#          -fno-exceptions \
# 	 -mno-red-zone \
# 	 -mcmodel=large \
#          -fno-common \
#          -Wstrict-overflow=5

CFLAGS = -O2 -mno-red-zone -fPIC -Wall
NORMFLAGS = -loop-simplify -lcssa

# For C++ add
#         -fno-rtti \

# Add these for more recent compilers to avoid having
# the compiler insert surprise ud2 instructions for you should
# you derefence address zero
#
#                           -fno-isolate-erroneous-paths-attribute \
#                           -fno-isolate-erroneous-paths-dereference \
#
#

all: $(TARGET) #$(BLOB_LL)

$(TARGET:.exe=.o):
	$(CC) $(CFLAGS) -c $(TARGET:.exe=.c) -I../../include -o $(TARGET:.exe=.o)	

framework_low_level.o: framework_low_level.S
	$(CC) $(CFLAGS) -c framework_low_level.S

framework.o : framework.c
	$(CC) $(CFLAGS)  -I../../include -c framework.c
	$(CC) -c -emit-llvm -O1 -Xclang -disable-llvm-passes -mcmodel=large -fPIC framework_link.c

glue.o : glue.c
	$(CC) $(CFLAGS) -c glue.c

framework: glue.o framework_low_level.o framework.o
	#nop

# $(BLOB_BC) : $(BLOB_OBJ)
# 	$(EXTRACTBC) $(BLOB_OBJ)
# 	mv $(BLOB_OBJ).bc $(BLOB_BC)

# $(BLOB_LL) : $(BLOB_BC)
# 	$(LLVMDIS) $(BLOB_BC)

# $(BLOB_SIMPLIFY_LL): $(BLOB_LL)
# 	# Run select loop simplification passes
# 	$(ME) $(NORMFLAGS) -S $< -o $@

# $(BLOB_OPT_LL): ~/CAT/lib/KARAT.so $(BLOB_SIMPLIFY_LL)
# 	# Run KARAT pass	
# 	$(ME) -load $< -karat -S $(BLOB_SIMPLIFY_LL) -o $@ &> karat.out 

# $(BLOB_KOBJ) : $(BLOB_OPT_LL)
# 	$(FINALCC) $(CFLAGS) -c $(BLOB_OPT_LL) -o $(BLOB_KOBJ)

# $(BLOB_OBJ) : $(TARGET:.exe=.o) framework.o glue.o
# 	$(LD) $^ -r -o $(BLOB_OBJ)

# $(TARGET:.exe=.unsigned) : $(BLOB_KOBJ) framework_low_level.o
# 	$(NLD) $^ -o $(TARGET:.exe=.unsigned)

$(TARGET:.exe=.unsigned) : $(TARGET:.exe=.o) framework
	$(NLD) $(TARGET:.exe=.o) -o $(TARGET:.exe=.unsigned)

$(TARGET) : $(TARGET:.exe=.unsigned)
	$(NSIGN) $(TARGET:.exe=.unsigned) $(TARGET)

nokarat: $(TARGET:.exe=.o) framework
	$(NLD) -nokarat $(TARGET:.exe=.o) -o $(TARGET:.exe=.unsigned)
	$(NSIGN) $(TARGET:.exe=.unsigned) $(TARGET)

clean: 
	rm -f $(TARGET) *.o .*.bc *.bc *.ll *.out *.unsigned *.exe 

