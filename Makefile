INC = ./
SUBINCS = $(shell ls $(INC)) 
INC_FLAGS += $(foreach n,$(SUBINCS),$(wildcard ./$(n)/*.h))
all:
	rm -f ./record_log/*
	mkdir -p bin
	cd ./build && make
clean:
	cd ./build && make clean 
test: 
	@echo $(INC_FLAGS)