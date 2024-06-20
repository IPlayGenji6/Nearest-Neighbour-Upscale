CC = gcc

BASEFLAGS = -Wall -Wextra -fopenmp
NODEBUG_FLAGS = -dNDEBUG 
DEBUG_FLAGS = -g

LDLIBS = -lm

BUILD_DIR = build
OBJS = $(BUILD_DIR)/NearestNeighbourUpscale.o $(BUILD_DIR)/lodepng.o $(BUILD_DIR)/NearestNeighbourUpscaleDriver.o

EXE = $(BUILD_DIR)/NearestNeighbourUpscale

debug: CFLAGS = $(BASEFLAGS) $(DEBUG_FLAGS)
debug: $(EXE)

release: CFLAGS = $(BASEFLAGS) $(NODEBUG_FLAGS) 
release: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(EXE) $(LDLIBS)

$(BUILD_DIR)/NearestNeighbourUpscale.o: NearestNeighbourUpscale.c NearestNeighbourUpscale.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c NearestNeighbourUpscale.c -o $(BUILD_DIR)/NearestNeighbourUpscale.o

$(BUILD_DIR)/NearestNeighbourUpscaleDriver.o: NearestNeighbourUpscaleDriver.c NearestNeighbourUpscaleDriver.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c NearestNeighbourUpscaleDriver.c -o $(BUILD_DIR)/NearestNeighbourUpscaleDriver.o

$(BUILD_DIR)/lodepng.o: LODEPNG/lodepng.c LODEPNG/lodepng.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c LODEPNG/lodepng.c -o $(BUILD_DIR)/lodepng.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	-rm -f $(OBJS)
	-rm -f *~
	-rm -f $(EXE)
	-rm -f $(EXE)_d
