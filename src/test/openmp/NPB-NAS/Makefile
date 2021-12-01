LDFLAGS += --allow-multiple-definition
CFLAGS += -O3 
#-fnoopenmp-use-tls
obj-y += math/
obj-y += common/\
	rand/
#EP and MG conflicts with others

obj-y += \
	 SP/\
	 LU/\
	 IS/\
	 FT/\
         CG/\
         EP/\
         MG/\
        BT/
#obj-y+= common/\
	rand/\
	FT/

#obj-y += common/\
	rand/\
	SP/

#obj-y += common/\
         rand/\
	 BT/

#obj-y += common/\
	rand/\
	EP/

#obj-y += common/\
	rand/\
	MG/

#obj-y += CG/\
	common/\
	rand/

#obj-y += LU/\
	common/\
	rand/

#IS
#obj-y += IS/

obj-$(NAUT_CONFIG_ASPACE_PAGING) += paging_benchmark.o
